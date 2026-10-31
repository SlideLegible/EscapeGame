// 步驟 7 場景狀態 → VERIFY STEP 7
// 整個檔案只在非 Shipping 編譯：有幾條要用 GetPasswordForDevOnly()（Shipping 沒有任何途徑拿到密碼）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

namespace Step7Spec
{
	const TCHAR* const PowerCutText = TEXT("喀——整棟樓瞬間安靜了下來，門禁機的燈也熄了");
	const TCHAR* const ShockText = TEXT("滋滋滋——你觸電了");
	const TCHAR* const LockedClassroomText = TEXT("門被鎖住了，也許可以找個夠重的東西把它撞開");

	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	bool LoadWith(const FString& InteractionsCsv, int32 Seed, FEscapeGameLogic& Out, FString& Error)
	{
		return FEscapeGameLogic::Load(InteractionsCsv, LoadText(TEXT("DT_Forms.csv")),
			LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), LoadText(TEXT("DT_Locations.csv")),
			Seed, Out, Error);
	}

	bool MakeLogic(int32 Seed, FEscapeGameLogic& Out, FString& Error)
	{
		return LoadWith(LoadText(TEXT("DT_Interactions.csv")), Seed, Out, Error);
	}

	/** 把 DT_Interactions.csv 裡以 "<Id>," 開頭的那一列的 Find 換成 Replace；找不到那一列或那段字回傳空字串 */
	FString BreakRow(const FString& RowId, const FString& Find, const FString& Replace)
	{
		TArray<FString> Lines;
		LoadText(TEXT("DT_Interactions.csv")).ParseIntoArrayLines(Lines, false);
		bool bChanged = false;
		for (FString& Line : Lines)
		{
			if (Line.StartsWith(RowId + TEXT(",")) && Line.Contains(Find))
			{
				Line = Line.Replace(*Find, *Replace);
				bChanged = true;
			}
		}
		return bChanged ? FString::Join(Lines, TEXT("\n")) : FString();
	}

	/** 刪掉以 "<Id>," 開頭的那一列 */
	FString DropRow(const FString& RowId)
	{
		TArray<FString> Lines, Kept;
		LoadText(TEXT("DT_Interactions.csv")).ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			if (!Line.StartsWith(RowId + TEXT(","))) { Kept.Add(Line); }
		}
		return Kept.Num() == Lines.Num() ? FString() : FString::Join(Kept, TEXT("\n"));
	}

	/** 這次互動的所有文字（Messages 串起來） */
	FString AllText(const FInteractOutcome& Out)
	{
		return FString::Join(Out.Messages, TEXT("\n"));
	}

	/** 人類 Player 走完 X001、碰飲料罐垃圾、進 L02、變成學生證（F03） */
	bool BecomeStudentCard(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.Interact(Player, TEXT("S02")).Kind != EInteractKind::Resolved) { return false; }
		if (!Logic.Interact(Player, TEXT("F01")).bTransformed) { return false; }
		if (Logic.Interact(Player, TEXT("S04")).EnteredLocationId != TEXT("L02")) { return false; }
		return Logic.Interact(Player, TEXT("F03")).bTransformed && Logic.GetPlayerForm(Player) == TEXT("F03");
	}

	/** 人類 Player 打翻花盆（X009）並變成辦公室鑰匙（F04） */
	bool BecomeOfficeKey(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.Interact(Player, TEXT("S10")).Result.InteractionId != TEXT("X009")) { return false; }
		return Logic.Interact(Player, TEXT("F04")).bTransformed && Logic.GetPlayerForm(Player) == TEXT("F04");
	}

	/** 讓玩家從人類走到指定形態（F00～F12）。走真的互動路線，不直接設定形態。 */
	bool BecomeForm(FEscapeGameLogic& Logic, const FString& Player, const FString& Form)
	{
		const auto Becomes = [&](const FString& Target) { return Logic.Interact(Player, Target).bTransformed && Logic.GetPlayerForm(Player) == Target; };
		if (Form == TEXT("F00")) { return Logic.GetPlayerForm(Player) == Form; }
		if (Form == TEXT("F01")) { return Logic.Interact(Player, TEXT("S02")).Kind == EInteractKind::Resolved && Becomes(Form); }
		if (Form == TEXT("F02"))
		{
			return BecomeForm(Logic, Player, TEXT("F01")) && Logic.Interact(Player, TEXT("S03")).EnteredLocationId == TEXT("L01") && Becomes(Form);
		}
		if (Form == TEXT("F03")) { return BecomeStudentCard(Logic, Player); }
		if (Form == TEXT("F04")) { return BecomeOfficeKey(Logic, Player); }
		if (Form == TEXT("F05"))
		{
			// 辦公室鑰匙開辦公室門（X005）→ 變回人類 → 開抽屜（X006）→ 變成那支鑰匙
			return BecomeOfficeKey(Logic, Player) && Logic.Interact(Player, TEXT("S05")).Result.InteractionId == TEXT("X005")
				&& Logic.ReturnToHuman(Player) && Logic.Interact(Player, TEXT("S06")).Result.InteractionId == TEXT("X006") && Becomes(Form);
		}
		if (Form == TEXT("F06"))
		{
			return BecomeOfficeKey(Logic, Player) && Logic.Interact(Player, TEXT("S05")).Result.InteractionId == TEXT("X005")
				&& Logic.ReturnToHuman(Player) && Logic.Interact(Player, TEXT("S08")).Result.InteractionId == TEXT("X008") && Becomes(Form);
		}
		if (Form == TEXT("F07"))
		{
			return BecomeForm(Logic, Player, TEXT("F05")) && Logic.Interact(Player, TEXT("S09")).Result.InteractionId == TEXT("X013") && Becomes(Form);
		}
		// F08～F12：一開始就在場景裡
		return Becomes(Form);
	}

	const TArray<FString>& AllForms()
	{
		static const TArray<FString> Forms = { TEXT("F00"), TEXT("F01"), TEXT("F02"), TEXT("F03"), TEXT("F04"), TEXT("F05"), TEXT("F06"),
			TEXT("F07"), TEXT("F08"), TEXT("F09"), TEXT("F10"), TEXT("F11"), TEXT("F12") };
		return Forms;
	}

	/** 一定不是當局密碼的四位數 */
	FString WrongCode(const FString& Password)
	{
		if (Password.Len() != 4) { return TEXT("ZZZZ"); }
		FString Out = Password;
		Out[0] = static_cast<TCHAR>(TEXT('0') + ((Out[0] - TEXT('0')) + 1) % 10);
		return Out;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep7Spec, "EscapeGame.Step7",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep7Spec)

void FEscapeGameStep7Spec::Define()
{
	using namespace Step7Spec;

	Describe(TEXT("VERIFY STEP 7"), [this]()
	{
		It(TEXT("人類拉下總電閘（X022）→ 顯示「喀——整棟樓瞬間安靜了下來，門禁機的燈也熄了」"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestFalse(TEXT("一開始有電"), Logic.IsPowerCut());

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S15"));
			TestTrue(TEXT("互動成立"), Out.Kind == EInteractKind::Resolved);
			TestEqual(TEXT("是 X022"), Out.Result.InteractionId, FString(TEXT("X022")));
			TestFalse(TEXT("不是預設嘲諷"), Out.Result.bIsDefault);
			TestEqual(TEXT("顯示一則文字"), Out.Messages.Num(), 1);
			TestTrue(*FString::Printf(TEXT("文字是停電那句，實際：%s"), *AllText(Out)), AllText(Out).Contains(PowerCutText));
			TestEqual(TEXT("沒有扣血"), Logic.GetTeam().GetHp(), FTeamState::StartingHp);
			TestEqual(TEXT("沒有事件"), Out.Events.Num(), 0);
			TestTrue(TEXT("X022 已觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X022")));
			TestTrue(TEXT("停電"), Logic.IsPowerCut());
			TestEqual(TEXT("電閘是全隊共用的，不管誰拉的：P1 還是人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestFalse(TEXT("拉電閘不開任何門"), Logic.GetOpenDoors().Num() > 0);
		});

		It(TEXT("拉下電閘後，學生證碰大門 → 大門不開，不觸發 E01；輸入正確密碼 → 大門不開，不觸發 E02"), [this]()
		{
			FString Error;
			// 對照：沒拉電閘時同一條路線確實會 E01（確認這條測試測得到東西）
			{
				FEscapeGameLogic Control;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Control, Error));
				Control.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				TestTrue(TEXT("對照組：P1 變成學生證"), BecomeStudentCard(Control, TEXT("P1")));
				const FInteractOutcome Open = Control.Interact(TEXT("P1"), TEXT("S01"));
				TestEqual(TEXT("對照組：沒停電時學生證碰大門是 X010"), Open.Result.InteractionId, FString(TEXT("X010")));
				TestTrue(TEXT("對照組：E01"), Control.GetTeam().HasEscaped(TEXT("P1")));
			}

			// 學生證路線
			FEscapeGameLogic Logic;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 走到學生證（F03）"), BecomeStudentCard(Logic, TEXT("P1")));
			TestEqual(TEXT("P2 拉下電閘"), Logic.Interact(TEXT("P2"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
			TestTrue(TEXT("停電"), Logic.IsPowerCut());

			const FInteractOutcome Card = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestTrue(TEXT("學生證碰大門：互動有處理"), Card.Kind == EInteractKind::Resolved);
			TestTrue(TEXT("學生證碰大門：預設回應（X010 失效）"), Card.Result.bIsDefault);
			TestEqual(TEXT("不是 X010"), Card.Result.InteractionId, FString());
			TestTrue(TEXT("結局不是 E01"), Card.Result.EndingId.IsEmpty());
			TestEqual(TEXT("沒有事件"), Card.Events.Num(), 0);
			TestFalse(TEXT("P1 沒有結束"), Logic.GetTeam().HasEscaped(TEXT("P1")));
			TestEqual(TEXT("P1 仍是學生證"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F03")));
			TestFalse(TEXT("X010 沒有觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X010")));
			TestFalse(TEXT("大門不在門表，不會開"), Logic.IsDoorOpen(TEXT("S01")));

			// 密碼路線：P2 是人類，輸入正確密碼
			const FString Password = Logic.GetTeam().GetPasswordForDevOnly();
			TestEqual(TEXT("密碼是四位數"), Password.Len(), 4);
			const FInteractOutcome Code = Logic.SubmitPassword(TEXT("P2"), Password);
			TestTrue(TEXT("輸入密碼：互動有處理"), Code.Kind == EInteractKind::Resolved);
			TestNotEqual(TEXT("不是 X016"), Code.Result.InteractionId, FString(TEXT("X016")));
			TestTrue(TEXT("結局不是 E02"), Code.Result.EndingId.IsEmpty());
			for (const FTeamEvent& Event : Code.Events) { TestNotEqual(TEXT("沒有 E02"), Event.EndingId, FString(TEXT("E02"))); }
			TestFalse(TEXT("P2 沒有結束"), Logic.GetTeam().HasEscaped(TEXT("P2")));
			TestFalse(TEXT("X016 沒有觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X016")));
			TestFalse(TEXT("大門仍然不開"), Logic.IsDoorOpen(TEXT("S01")));

			// 對照：沒停電時，同一個種子的正確密碼確實 E02
			FEscapeGameLogic Control2;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Control2, Error));
			Control2.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			const FInteractOutcome Open2 = Control2.SubmitPassword(TEXT("P2"), Control2.GetTeam().GetPasswordForDevOnly());
			TestEqual(TEXT("對照組：沒停電時正確密碼是 X016"), Open2.Result.InteractionId, FString(TEXT("X016")));
			TestTrue(TEXT("對照組：E02"), Control2.GetTeam().HasEscaped(TEXT("P2")));
		});

		It(TEXT("非人類形態碰配電箱（X025）→「滋滋滋——你觸電了」，生命值 -1"), [this]()
		{
			FString Error;
			const TArray<FString> NonHuman = { TEXT("F08"), TEXT("F09"), TEXT("F10"), TEXT("F11"), TEXT("F12") };
			for (const FString& Form : NonHuman)
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				TestTrue(*FString::Printf(TEXT("變成 %s"), *Form), BecomeForm(Logic, TEXT("P1"), Form));
				const int32 HpBefore = Logic.GetTeam().GetHp();

				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S15"));
				TestTrue(*FString::Printf(TEXT("%s：互動成立"), *Form), Out.Kind == EInteractKind::Resolved);
				TestEqual(*FString::Printf(TEXT("%s：是 X025"), *Form), Out.Result.InteractionId, FString(TEXT("X025")));
				TestTrue(*FString::Printf(TEXT("%s：文字，實際：%s"), *Form, *AllText(Out)), AllText(Out).Contains(ShockText));
				TestEqual(*FString::Printf(TEXT("%s：生命值 -1"), *Form), Logic.GetTeam().GetHp(), HpBefore - 1);
				TestFalse(*FString::Printf(TEXT("%s：沒有拉下電閘"), *Form), Logic.IsPowerCut());
			}

			// 也包含要走真路線才能變成的物品（鑰匙收藏以外的形態）
			for (const FString& Form : { FString(TEXT("F01")), FString(TEXT("F03")), FString(TEXT("F04")) })
			{
				FEscapeGameLogic Logic;
				MakeLogic(1, Logic, Error);
				Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				TestTrue(*FString::Printf(TEXT("變成 %s"), *Form), BecomeForm(Logic, TEXT("P1"), Form));
				const int32 HpBefore = Logic.GetTeam().GetHp();
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S15"));
				TestEqual(*FString::Printf(TEXT("%s：是 X025"), *Form), Out.Result.InteractionId, FString(TEXT("X025")));
				TestEqual(*FString::Printf(TEXT("%s：生命值 -1"), *Form), Logic.GetTeam().GetHp(), HpBefore - 1);
			}
		});

		It(TEXT("椅子撞開教室門（X036）之後 → 所有玩家都可以前往走廊；別的形態再碰教室門 → 不再顯示 X037"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.AddPlayer(TEXT("P3"), FVector(0, 80, 0));
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			TestTrue(TEXT("P3 變成骰子"), Logic.Interact(TEXT("P3"), TEXT("F08")).bTransformed);
			TestFalse(TEXT("撞開之前門是鎖的"), Logic.IsDoorOpen(TEXT("S19")));

			// 撞開之前：別的形態碰教室門 → X037
			const FInteractOutcome HumanBefore = Logic.Interact(TEXT("P2"), TEXT("S19"));
			TestEqual(TEXT("人類碰教室門 → X037"), HumanBefore.Result.InteractionId, FString(TEXT("X037")));
			TestTrue(TEXT("X037 的文字"), AllText(HumanBefore).Contains(LockedClassroomText));
			const FInteractOutcome DiceBefore = Logic.Interact(TEXT("P3"), TEXT("S19"));
			TestEqual(TEXT("骰子碰教室門 → X037"), DiceBefore.Result.InteractionId, FString(TEXT("X037")));
			TestFalse(TEXT("X037 不會開門"), Logic.IsDoorOpen(TEXT("S19")));

			// 椅子撞開
			const FInteractOutcome Bang = Logic.Interact(TEXT("P1"), TEXT("S19"));
			TestEqual(TEXT("椅子碰教室門 → X036"), Bang.Result.InteractionId, FString(TEXT("X036")));
			TestTrue(TEXT("砰！"), AllText(Bang).Contains(TEXT("砰！")));
			TestTrue(TEXT("教室門開了"), Logic.IsDoorOpen(TEXT("S19")));
			TestEqual(TEXT("已開的門只有教室門"), Logic.GetOpenDoors(), TArray<FString>({ TEXT("S19") }));
			TestFalse(TEXT("辦公室門沒開"), Logic.IsDoorOpen(TEXT("S05")));
			TestFalse(TEXT("儲物間門沒開"), Logic.IsDoorOpen(TEXT("S09")));
			TestFalse(TEXT("大門沒開"), Logic.IsDoorOpen(TEXT("S01")));

			// 門是全隊共用：沒撞門的玩家（人類、骰子）也看到門開了，再碰教室門不再是 X037
			const FInteractOutcome HumanAfter = Logic.Interact(TEXT("P2"), TEXT("S19"));
			TestNotEqual(TEXT("人類再碰教室門：不是 X037"), HumanAfter.Result.InteractionId, FString(TEXT("X037")));
			TestTrue(TEXT("人類再碰教室門：預設回應"), HumanAfter.Result.bIsDefault);
			TestFalse(TEXT("人類再碰教室門：不再顯示 X037 的文字"), AllText(HumanAfter).Contains(LockedClassroomText));
			const FInteractOutcome DiceAfter = Logic.Interact(TEXT("P3"), TEXT("S19"));
			TestNotEqual(TEXT("骰子再碰教室門：不是 X037"), DiceAfter.Result.InteractionId, FString(TEXT("X037")));
			TestTrue(TEXT("骰子再碰教室門：預設回應"), DiceAfter.Result.bIsDefault);
			TestFalse(TEXT("骰子再碰教室門：不再顯示 X037 的文字"), AllText(DiceAfter).Contains(LockedClassroomText));
			TestTrue(TEXT("門還是開的"), Logic.IsDoorOpen(TEXT("S19")));
		});

		It(TEXT("辦公室鑰匙開過辦公室門（X005）之後 → 之後所有玩家都能進辦公室"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			TestFalse(TEXT("一開始辦公室門是鎖的"), Logic.IsDoorOpen(TEXT("S05")));
			const FInteractOutcome Pot = Logic.Interact(TEXT("P1"), TEXT("S10"));
			TestEqual(TEXT("人類碰花盆 → X009"), Pot.Result.InteractionId, FString(TEXT("X009")));
			TestTrue(TEXT("P1 變成辦公室鑰匙"), Logic.Interact(TEXT("P1"), TEXT("F04")).bTransformed);
			TestEqual(TEXT("P1 形態 F04"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F04")));
			TestFalse(TEXT("還沒開門"), Logic.IsDoorOpen(TEXT("S05")));

			const FInteractOutcome Open = Logic.Interact(TEXT("P1"), TEXT("S05"));
			TestEqual(TEXT("辦公室鑰匙碰辦公室門 → X005"), Open.Result.InteractionId, FString(TEXT("X005")));
			TestTrue(TEXT("辦公室門開了"), Logic.IsDoorOpen(TEXT("S05")));
			TestEqual(TEXT("已開的門"), Logic.GetOpenDoors(), TArray<FString>({ TEXT("S05") }));
			TestFalse(TEXT("教室門沒開"), Logic.IsDoorOpen(TEXT("S19")));

			// 全隊共用：沒開門的 P2 也能用辦公室裡的東西（X006 的前置條件是 X005）
			const FInteractOutcome Drawer = Logic.Interact(TEXT("P2"), TEXT("S06"));
			TestEqual(TEXT("P2（人類）打開辦公桌抽屜 → X006"), Drawer.Result.InteractionId, FString(TEXT("X006")));
			TestTrue(TEXT("P2 看到的辦公室門也是開的"), Logic.IsDoorOpen(TEXT("S05")));
		});

		It(TEXT("不可以 電閘拉下後再推回去（任何形態碰配電箱都不會恢復）"), [this]()
		{
			FString Error;
			for (const FString& Form : AllForms())
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
				TestTrue(*FString::Printf(TEXT("P1 變成 %s"), *Form), BecomeForm(Logic, TEXT("P1"), Form));
				TestEqual(*FString::Printf(TEXT("%s：P2 拉下電閘"), *Form), Logic.Interact(TEXT("P2"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
				TestTrue(*FString::Printf(TEXT("%s：停電"), *Form), Logic.IsPowerCut());

				const FInteractOutcome Touch = Logic.Interact(TEXT("P1"), TEXT("S15"));
				TestTrue(*FString::Printf(TEXT("%s：碰配電箱有處理"), *Form), Touch.Kind == EInteractKind::Resolved);
				TestTrue(*FString::Printf(TEXT("%s：碰了還是停電"), *Form), Logic.IsPowerCut());
				TestTrue(*FString::Printf(TEXT("%s：X022 仍是已觸發"), *Form), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X022")));

				// 人類再拉一次（資料如此：仍是 X022）也不會恢復
				if (Logic.GetPlayerForm(TEXT("P2")) == TEXT("F00"))
				{
					TestEqual(*FString::Printf(TEXT("%s：人類再碰配電箱仍是 X022（資料如此）"), *Form),
						Logic.Interact(TEXT("P2"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
					TestTrue(*FString::Printf(TEXT("%s：人類再拉一次仍是停電"), *Form), Logic.IsPowerCut());
				}
				// 變回人類再碰也一樣
				if (Logic.ReturnToHuman(TEXT("P1")))
				{
					Logic.Interact(TEXT("P1"), TEXT("S15"));
					TestTrue(*FString::Printf(TEXT("%s：變回人類再碰還是停電"), *Form), Logic.IsPowerCut());
				}
			}
		});
	});

	Describe(TEXT("PLANS 詮釋"), [this]()
	{
		It(TEXT("停電後輸入正確密碼的回應和輸錯完全一樣"), [this]()
		{
			FString Error;
			// 同一個種子的兩局：一局停電後輸正確密碼，另一局停電後輸錯的密碼；亂數序列一樣，所以回應必須逐項相同
			FEscapeGameLogic Right, Wrong;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(7, Right, Error));
			TestTrue(TEXT("CSV 載入"), MakeLogic(7, Wrong, Error));
			for (FEscapeGameLogic* L : { &Right, &Wrong })
			{
				L->AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				TestEqual(TEXT("拉下電閘"), L->Interact(TEXT("P1"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
			}
			const FString Password = Right.GetTeam().GetPasswordForDevOnly();
			TestEqual(TEXT("同種子同密碼"), Wrong.GetTeam().GetPasswordForDevOnly(), Password);

			const FInteractOutcome A = Right.SubmitPassword(TEXT("P1"), Password);
			const FInteractOutcome B = Wrong.SubmitPassword(TEXT("P1"), WrongCode(Password));
			TestTrue(TEXT("正確密碼：Resolved"), A.Kind == EInteractKind::Resolved);
			TestTrue(TEXT("輸錯：Resolved"), B.Kind == EInteractKind::Resolved);
			TestEqual(TEXT("停電後正確密碼走 X017"), A.Result.InteractionId, FString(TEXT("X017")));
			TestEqual(TEXT("輸錯走 X017"), B.Result.InteractionId, FString(TEXT("X017")));
			TestEqual(TEXT("Messages 完全一樣"), A.Messages, B.Messages);
			TestTrue(TEXT("Messages 不是空的"), A.Messages.Num() > 0);
			TestEqual(TEXT("Result.Text 一樣"), A.Result.Text, B.Result.Text);
			TestEqual(TEXT("bIsDefault 一樣"), A.Result.bIsDefault, B.Result.bIsDefault);
			TestEqual(TEXT("結局都沒有"), A.Result.EndingId, B.Result.EndingId);
			TestEqual(TEXT("事件數一樣"), A.Events.Num(), B.Events.Num());
			TestEqual(TEXT("擲骰結果一樣"), A.bHpChanceHit, B.bHpChanceHit);
			TestEqual(TEXT("生命值一樣"), Right.GetTeam().GetHp(), Wrong.GetTeam().GetHp());
			TestEqual(TEXT("已觸發的 X 一樣"), Right.GetTeam().GetTriggeredIds().Num(), Wrong.GetTeam().GetTriggeredIds().Num());
			TestFalse(TEXT("兩邊都沒有結束"), Right.GetTeam().HasEscaped(TEXT("P1")) || Wrong.GetTeam().HasEscaped(TEXT("P1")));

			// 停電後連續輸入：正確密碼每次都和輸錯一樣，仍然照常擲 1/1000
			FEscapeGameLogic C, D;
			MakeLogic(11, C, Error);
			MakeLogic(11, D, Error);
			for (FEscapeGameLogic* L : { &C, &D }) { L->AddPlayer(TEXT("P1"), FVector(0, 0, 0)); L->Interact(TEXT("P1"), TEXT("S15")); }
			const FString PasswordC = C.GetTeam().GetPasswordForDevOnly();
			for (int32 I = 0; I < 200; ++I)
			{
				const FInteractOutcome X = C.SubmitPassword(TEXT("P1"), PasswordC);
				const FInteractOutcome Y = D.SubmitPassword(TEXT("P1"), WrongCode(PasswordC));
				if (X.Messages != Y.Messages || X.Result.InteractionId != Y.Result.InteractionId || X.Result.InteractionId != TEXT("X017"))
				{
					AddError(FString::Printf(TEXT("第 %d 次：停電後正確密碼的回應和輸錯不同"), I));
					break;
				}
			}
			TestEqual(TEXT("200 次後兩邊生命值一樣"), C.GetTeam().GetHp(), D.GetTeam().GetHp());
			TestFalse(TEXT("200 次後都沒有結局"), C.GetTeam().HasEscaped(TEXT("P1")) || D.GetTeam().HasEscaped(TEXT("P1")));
		});

		It(TEXT("停電後輸入 1031 仍會記 Tried1031 旗標，月曆照常變成 X035"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(3, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.Interact(TEXT("P1"), TEXT("S15"));
			TestTrue(TEXT("停電"), Logic.IsPowerCut());
			TestFalse(TEXT("還沒旗標"), Logic.GetTeam().HasFlag(TEXT("Tried1031")));
			const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), TEXT("1031"));
			TestEqual(TEXT("走 X017"), Out.Result.InteractionId, FString(TEXT("X017")));
			TestTrue(TEXT("記了旗標"), Logic.GetTeam().HasFlag(TEXT("Tried1031")));
			TestEqual(TEXT("月曆是 X035"), Logic.Interact(TEXT("P1"), TEXT("S18")).Result.InteractionId, FString(TEXT("X035")));
		});

		It(TEXT("門表以外的 ID（大門、一般物件、物品、空字串）IsDoorOpen 一律 false；門表順序固定"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestEqual(TEXT("一開始沒有開的門"), Logic.GetOpenDoors().Num(), 0);

			// 三扇門全開：教室門（椅子）、辦公室門（鑰匙）、儲物間門（不知道用來做甚麼的鑰匙）
			FEscapeGameLogic Full;
			MakeLogic(1, Full, Error);
			Full.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Full.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Full.AddPlayer(TEXT("P3"), FVector(0, 80, 0));
			TestTrue(TEXT("P1 變成 F05"), BecomeForm(Full, TEXT("P1"), TEXT("F05")));
			TestTrue(TEXT("P2 變成椅子"), BecomeForm(Full, TEXT("P2"), TEXT("F12")));
			TestEqual(TEXT("椅子撞教室門"), Full.Interact(TEXT("P2"), TEXT("S19")).Result.InteractionId, FString(TEXT("X036")));
			TestEqual(TEXT("F05 開儲物間門"), Full.Interact(TEXT("P1"), TEXT("S09")).Result.InteractionId, FString(TEXT("X013")));
			TestTrue(TEXT("三扇門都開了"), Full.IsDoorOpen(TEXT("S19")) && Full.IsDoorOpen(TEXT("S05")) && Full.IsDoorOpen(TEXT("S09")));
			TestEqual(TEXT("GetOpenDoors 依門表順序"), Full.GetOpenDoors(), TArray<FString>({ TEXT("S19"), TEXT("S05"), TEXT("S09") }));
			for (const FString& Other : { FString(TEXT("S01")), FString(TEXT("S02")), FString(TEXT("S15")), FString(TEXT("F12")), FString(TEXT("X036")), FString(TEXT("")), FString(TEXT("S99")) })
			{
				TestFalse(*FString::Printf(TEXT("「%s」不是門表裡的門"), *Other), Full.IsDoorOpen(Other));
			}
		});

		It(TEXT("E05 重啟後門重新鎖上、電恢復（新的一局）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestEqual(TEXT("P2 拉下電閘"), Logic.Interact(TEXT("P2"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			TestEqual(TEXT("椅子撞教室門"), Logic.Interact(TEXT("P1"), TEXT("S19")).Result.InteractionId, FString(TEXT("X036")));
			TestTrue(TEXT("重啟前：停電"), Logic.IsPowerCut());
			TestTrue(TEXT("重啟前：教室門開了"), Logic.IsDoorOpen(TEXT("S19")));

			// 椅子一直碰配電箱（X025，每次 -1），第 5 次生命值歸零 → E05
			bool bRestarted = false;
			for (int32 I = 0; I < FTeamState::StartingHp && !bRestarted; ++I)
			{
				const FInteractOutcome Shock = Logic.Interact(TEXT("P1"), TEXT("S15"));
				for (const FTeamEvent& Event : Shock.Events) { bRestarted = bRestarted || Event.EndingId == TEXT("E05"); }
			}
			TestTrue(TEXT("扣到 E05 重啟"), bRestarted);
			TestEqual(TEXT("重啟次數 1"), Logic.GetTeam().GetRestartCount(), 1);
			TestFalse(TEXT("重啟後：電恢復"), Logic.IsPowerCut());
			TestFalse(TEXT("重啟後：教室門重新鎖上"), Logic.IsDoorOpen(TEXT("S19")));
			TestEqual(TEXT("重啟後：沒有開著的門"), Logic.GetOpenDoors().Num(), 0);
			TestEqual(TEXT("重啟後：生命值回到 5"), Logic.GetTeam().GetHp(), FTeamState::StartingHp);

			// 新的一局：電閘又可以拉、教室門又是 X037
			TestEqual(TEXT("P1 重啟後變回人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestEqual(TEXT("教室門又是 X037"), Logic.Interact(TEXT("P2"), TEXT("S19")).Result.InteractionId, FString(TEXT("X037")));
			TestEqual(TEXT("電閘可以再拉"), Logic.Interact(TEXT("P1"), TEXT("S15")).Result.InteractionId, FString(TEXT("X022")));
			TestTrue(TEXT("又停電了"), Logic.IsPowerCut());
		});

		It(TEXT("門表與 X022 的載入檢查：X 不存在或對象不是那扇門（那個電閘）→ 載入失敗並寫出 ID"), [this]()
		{
			FString Error;
			// 先確認原始資料載得進來
			FEscapeGameLogic Ok;
			TestTrue(*FString::Printf(TEXT("原始 CSV 載入 %s"), *Error), MakeLogic(1, Ok, Error));

			// 對象換成別的物件
			struct FCase { const TCHAR* RowId; const TCHAR* Find; const TCHAR* Replace; };
			const FCase Cases[] = {
				{ TEXT("X036"), TEXT("S19 教室門"), TEXT("S20 講桌抽屜") },
				{ TEXT("X005"), TEXT("S05 辦公室門"), TEXT("S06 辦公桌抽屜") },
				{ TEXT("X013"), TEXT("S09 儲物間門"), TEXT("S11 馬桶") },
				{ TEXT("X022"), TEXT("S15 配電箱"), TEXT("S14 黑板") },
			};
			for (const FCase& Case : Cases)
			{
				const FString Broken = BreakRow(Case.RowId, Case.Find, Case.Replace);
				if (Broken.IsEmpty()) { AddError(FString::Printf(TEXT("測試資料有問題：找不到 %s 那一列的「%s」"), Case.RowId, Case.Find)); continue; }
				FEscapeGameLogic Logic;
				FString E;
				TestFalse(*FString::Printf(TEXT("%s 對象改掉 → 載入失敗"), Case.RowId), LoadWith(Broken, 1, Logic, E));
				TestTrue(*FString::Printf(TEXT("錯誤訊息要有 %s，實際：%s"), Case.RowId, *E), E.Contains(Case.RowId));
			}

			// X022 整列不存在
			const FString NoPower = DropRow(TEXT("X022"));
			if (NoPower.IsEmpty()) { AddError(TEXT("測試資料有問題：找不到 X022 那一列")); }
			else
			{
				FEscapeGameLogic Logic;
				FString E;
				TestFalse(TEXT("X022 不存在 → 載入失敗"), LoadWith(NoPower, 1, Logic, E));
				TestTrue(*FString::Printf(TEXT("錯誤訊息要有 X022，實際：%s"), *E), E.Contains(TEXT("X022")));
			}
		});
	});
}

#endif  // !UE_BUILD_SHIPPING
