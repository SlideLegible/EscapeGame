// 步驟 5 大門密碼 → VERIFY STEP 5
// 整個檔案只在非 Shipping 編譯：測試要用 GetPasswordForDevOnly()（Shipping 沒有任何途徑拿到密碼）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/Csv/CsvParser.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

namespace Step5Spec
{
	const TCHAR* const WrongText = TEXT("門紋絲不動，好像有點死了");
	const TCHAR* const CrampText = TEXT("手指抽筋了");

	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	bool MakeLogic(int32 Seed, FEscapeGameLogic& Out, FString& Error)
	{
		return FEscapeGameLogic::Load(LoadText(TEXT("DT_Interactions.csv")), LoadText(TEXT("DT_Forms.csv")),
			LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), LoadText(TEXT("DT_Locations.csv")),
			Seed, Out, Error);
	}

	/** 把密碼的第 Digit 位加 1（mod 10）：保證和密碼只錯一位 */
	FString ChangeOneDigit(const FString& Password, int32 Digit)
	{
		if (Password.Len() != 4) { return TEXT("ZZZZ"); }  // 沒有合法密碼時，回傳一定錯的碼（不讓測試自己當掉）
		FString Out = Password;
		Out[Digit] = static_cast<TCHAR>(TEXT('0') + ((Out[Digit] - TEXT('0')) + 1) % 10);
		return Out;
	}

	FString Offset(const FString& Password, int32 Delta)
	{
		if (Password.Len() != 4) { return TEXT("ZZZZ"); }
		const int32 Value = ((FCString::Atoi(*Password) + Delta) % 10000 + 10000) % 10000;
		return FString::Printf(TEXT("%04d"), Value);
	}

	bool IsFourDigits(const FString& S)
	{
		if (S.Len() != 4) { return false; }
		for (TCHAR C : S) { if (!FChar::IsDigit(C)) { return false; } }
		return true;
	}

	TArray<FString> SceneObjectIds()
	{
		TArray<FString> Ids;
		FCsvParser Parser(LoadText(TEXT("DT_SceneObjects.csv")));
		const FCsvParser::FRows& Rows = Parser.GetRows();
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			if (Rows[R].Num() > 0 && FCString::Strlen(Rows[R][0]) > 0) { Ids.Add(FString(Rows[R][0])); }
		}
		return Ids;
	}

	FInteractionResult FakeEnding(const FString& EndingId)
	{
		FInteractionResult R;
		R.InteractionId = TEXT("X900");
		R.EndingId = EndingId;
		return R;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep5Spec, "EscapeGame.Step5",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep5Spec)

void FEscapeGameStep5Spec::Define()
{
	using namespace Step5Spec;

	Describe(TEXT("VERIFY STEP 5"), [this]()
	{
		It(TEXT("每局開始 → 產生一組 0000～9999 的四位數密碼"), [this]()
		{
			TSet<FString> Distinct;
			for (int32 Seed = 0; Seed < 200; ++Seed)
			{
				FTeamState Team(Seed);
				const FString Password = Team.GetPasswordForDevOnly();
				TestTrue(*FString::Printf(TEXT("種子 %d 的密碼是四位數字"), Seed), IsFourDigits(Password));
				TestEqual(TEXT("同種子同密碼"), FTeamState(Seed).GetPasswordForDevOnly(), Password);
				TestEqual(TEXT("密碼自己輸入是 Correct"), Team.CheckPassword(Password), EPasswordInput::Correct);
				Distinct.Add(Password);
			}
			TestTrue(TEXT("不同種子有不同密碼（不是固定值）"), Distinct.Num() > 100);

			// 無參數版本（舊測試用）也有合法密碼
			TestTrue(TEXT("無參數版本也有密碼"), IsFourDigits(FTeamState().GetPasswordForDevOnly()));

			// 組合層：載入後每局都有密碼
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(12345, Logic, Error));
			TestTrue(TEXT("組合層的密碼是四位數字"), IsFourDigits(Logic.GetTeam().GetPasswordForDevOnly()));
		});

		It(TEXT("用固定種子產生 10,000 局 → 沒有一局的密碼是 1031"), [this]()
		{
			// 做法一：10,000 個種子各一局
			int32 Bad = 0;
			int32 Below = 0, Above = 0;
			for (int32 Seed = 0; Seed < 10000; ++Seed)
			{
				const FString P = FTeamState(Seed).GetPasswordForDevOnly();
				if (!IsFourDigits(P) || P == TEXT("1031")) { ++Bad; }
				const int32 V = FCString::Atoi(*P);
				if (V < 1031) { ++Below; } else if (V > 1031) { ++Above; }
			}
			TestEqual(TEXT("10,000 個種子：沒有壞密碼、沒有 1031"), Bad, 0);
			TestTrue(TEXT("1031 兩邊的數字都出得來"), Below > 500 && Above > 5000);

			// 做法二：同一個種子連續開 10,000 局
			FTeamState Team(2026);
			int32 Bad2 = 0;
			TSet<FString> Distinct;
			for (int32 Round = 0; Round < 10000; ++Round)
			{
				Team.StartNewRoundForTesting();
				const FString P = Team.GetPasswordForDevOnly();
				if (!IsFourDigits(P) || P == TEXT("1031")) { ++Bad2; }
				Distinct.Add(P);
			}
			TestEqual(TEXT("同一個種子 10,000 局：沒有壞密碼、沒有 1031"), Bad2, 0);
			TestTrue(TEXT("每局重新產生（不是同一組）"), Distinct.Num() > 5000);
			TestEqual(TEXT("輸入 1031 一定是 Wrong"), Team.CheckPassword(TEXT("1031")), EPasswordInput::Wrong);
		});

		It(TEXT("人類在大門輸入正確密碼（X016）→ 大門打開，該玩家觸發 E02"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(777, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(100, 0, 0));

			// 先碰大門：要輸入密碼，狀態沒改
			const FInteractOutcome Touch = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestTrue(TEXT("碰大門 → 要輸入密碼"), Touch.Kind == EInteractKind::NeedsPassword);
			TestEqual(TEXT("沒有文字"), Touch.Messages.Num(), 0);
			TestEqual(TEXT("沒有事件"), Touch.Events.Num(), 0);
			TestEqual(TEXT("沒有觸發任何 X"), Logic.GetTeam().GetTriggeredIds().Num(), 0);
			TestFalse(TEXT("P1 還沒逃出"), Logic.GetTeam().HasEscaped(TEXT("P1")));

			const FString Password = Logic.GetTeam().GetPasswordForDevOnly();
			const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Password);
			TestTrue(TEXT("輸入成立"), Out.Kind == EInteractKind::Resolved);
			TestEqual(TEXT("是 X016"), Out.Result.InteractionId, FString(TEXT("X016")));
			TestEqual(TEXT("結局 E02"), Out.Result.EndingId, FString(TEXT("E02")));
			TestFalse(TEXT("沒有擲機率"), Out.bHpChanceHit);
			TestEqual(TEXT("剛好一個事件"), Out.Events.Num(), 1);
			if (Out.Events.Num() == 1)
			{
				TestEqual(TEXT("事件是 E02"), Out.Events[0].EndingId, FString(TEXT("E02")));
				TestEqual(TEXT("只有 P1 受影響"), Out.Events[0].AffectedPlayers, TArray<FString>({ TEXT("P1") }));
			}
			TestTrue(TEXT("P1 結束"), Logic.GetTeam().HasEscaped(TEXT("P1")));
			TestEqual(TEXT("P1 的結局是 E02"), Logic.GetTeam().GetEscapeEnding(TEXT("P1")), FString(TEXT("E02")));
			TestFalse(TEXT("P2 還在局裡"), Logic.GetTeam().HasEscaped(TEXT("P2")));
			TestEqual(TEXT("還在局裡的只有 P2"), Logic.GetTeam().GetRemainingPlayers(), TArray<FString>({ TEXT("P2") }));
			TestTrue(TEXT("X016 已觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X016")));
			TestEqual(TEXT("生命值沒變"), Logic.GetTeam().GetHp(), FTeamState::StartingHp);

			// P1 結束後再操作 → 被拒絕；P2 照常可以輸入
			TestTrue(TEXT("已結束的玩家 Interact 被拒絕"), Logic.Interact(TEXT("P1"), TEXT("S01")).Kind == EInteractKind::Rejected);
			TestTrue(TEXT("已結束的玩家 SubmitPassword 被拒絕"), Logic.SubmitPassword(TEXT("P1"), Password).Kind == EInteractKind::Rejected);
			const FInteractOutcome P2Out = Logic.SubmitPassword(TEXT("P2"), Password);
			TestEqual(TEXT("P2 也能用同一組密碼逃出"), P2Out.Result.EndingId, FString(TEXT("E02")));
			TestEqual(TEXT("P2 的結局是 E02"), Logic.GetTeam().GetEscapeEnding(TEXT("P2")), FString(TEXT("E02")));
		});

		It(TEXT("輸入錯誤 →「門紋絲不動，好像有點死了」，不顯示大小、不顯示哪幾位對"), [this]()
		{
			const int32 Seed = 777;
			FString Error;
			FString Password;
			{
				FEscapeGameLogic Probe;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(Seed, Probe, Error));
				Password = Probe.GetTeam().GetPasswordForDevOnly();
			}
			TArray<FString> Codes = { TEXT("0000"), TEXT("9999"), TEXT("1031"), Offset(Password, 1), Offset(Password, -1),
				ChangeOneDigit(Password, 0), ChangeOneDigit(Password, 3), TEXT(""), TEXT("123"), TEXT("12345"), TEXT("abcd"), TEXT("10 31") };
			int32 Tried = 0;
			for (const FString& Code : Codes)
			{
				if (Code == Password) { continue; }  // 剛好等於密碼就不是錯碼
				++Tried;
				FEscapeGameLogic Logic;  // 同種子 → 同密碼、同一顆骰子
				TestTrue(TEXT("CSV 載入"), MakeLogic(Seed, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Code);
				const FString Label = FString::Printf(TEXT("錯碼「%s」"), *Code);
				TestTrue(*(Label + TEXT(" 成立")), Out.Kind == EInteractKind::Resolved);
				TestEqual(*(Label + TEXT(" 是 X017")), Out.Result.InteractionId, FString(TEXT("X017")));
				TestFalse(*(Label + TEXT(" 這次沒擲中（固定種子）")), Out.bHpChanceHit);
				TestEqual(*(Label + TEXT(" 文字剛好一則")), Out.Messages, TArray<FString>({ FString(WrongText) }));
				if (Out.Messages.Num() == 1)
				{
					for (TCHAR C : Out.Messages[0]) { TestFalse(*(Label + TEXT(" 文字沒有數字")), FChar::IsDigit(C)); }
					TestFalse(*(Label + TEXT(" 沒有大小提示")), Out.Messages[0].Contains(TEXT("大")) || Out.Messages[0].Contains(TEXT("小")));
					TestFalse(*(Label + TEXT(" 沒有哪幾位對的提示")), Out.Messages[0].Contains(TEXT("位")) || Out.Messages[0].Contains(TEXT("對")));
				}
				TestEqual(*(Label + TEXT(" 沒有事件")), Out.Events.Num(), 0);
				TestEqual(*(Label + TEXT(" 生命值沒變")), Logic.GetTeam().GetHp(), FTeamState::StartingHp);
				TestFalse(*(Label + TEXT(" 沒有逃出")), Logic.GetTeam().HasEscaped(TEXT("P1")));
				TestEqual(*(Label + TEXT(" 結局欄是空的")), Out.Result.EndingId, FString());
			}
			TestTrue(TEXT("至少測了 8 種錯碼"), Tried >= 8);
		});

		It(TEXT("用固定種子連續輸錯 100,000 次 →「手指抽筋了」扣血的次數在 70～130 之間（期望 100）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(20261031, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);

			int32 Hits = 0, NotResolved = 0, NotX017 = 0, BadMessages = 0, Cramps = 0;
			const TArray<FString> TwoMessages = { FString(WrongText), FString(CrampText) };
			const TArray<FString> OneMessage = { FString(WrongText) };
			for (int32 Attempt = 0; Attempt < 100000; ++Attempt)
			{
				// 保證錯的碼：當局密碼換一位（E05 重啟後密碼會換，所以每次重讀）
				const FString Code = ChangeOneDigit(Logic.GetTeam().GetPasswordForDevOnly(), Attempt % 4);
				const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Code);
				if (Out.Kind != EInteractKind::Resolved) { ++NotResolved; continue; }
				if (Out.Result.InteractionId != TEXT("X017")) { ++NotX017; }
				if (Out.bHpChanceHit)
				{
					++Hits;
					if (Out.Messages != TwoMessages) { ++BadMessages; }
				}
				else if (Out.Messages != OneMessage) { ++BadMessages; }
				for (const FString& M : Out.Messages) { if (M == CrampText) { ++Cramps; } }
			}
			TestEqual(TEXT("100,000 次都成立（沒有被拒絕、沒有上限）"), NotResolved, 0);
			TestEqual(TEXT("100,000 次都是 X017"), NotX017, 0);
			TestEqual(TEXT("文字都對：沒擲中一則、擲中兩則"), BadMessages, 0);
			TestEqual(TEXT("「手指抽筋了」出現的次數 = 擲中次數"), Cramps, Hits);
			AddInfo(FString::Printf(TEXT("100,000 次輸錯，手指抽筋 %d 次，E05 重啟 %d 次"), Hits, Logic.GetTeam().GetRestartCount()));
			TestTrue(*FString::Printf(TEXT("扣血次數 %d 在 70～130 之間"), Hits), Hits >= 70 && Hits <= 130);
			// 每 5 次扣血重啟一次（生命值 5，每次 -1；沒有其他扣血來源）
			TestEqual(TEXT("E05 重啟次數 = 扣血次數 / 5"), Logic.GetTeam().GetRestartCount(), Hits / 5);
			TestEqual(TEXT("生命值 = 5 - 扣血次數 % 5"), Logic.GetTeam().GetHp(), FTeamState::StartingHp - Hits % 5);
		});

		It(TEXT("輸入 1031 失敗後再看月曆（X035）→ 顯示「旁邊多了一行小字：老師生日🎂」；沒試過 1031 時看月曆（X032）→ 沒有這行字"), [this]()
		{
			const FString Birthday = TEXT("旁邊多了一行小字：老師生日🎂");
			FString Error;
			// 沒試過 1031：X032
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(777, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S18"));
				TestEqual(TEXT("沒試過 → X032"), Out.Result.InteractionId, FString(TEXT("X032")));
				TestEqual(TEXT("一則文字"), Out.Messages.Num(), 1);
				for (const FString& M : Out.Messages) { TestFalse(TEXT("沒有那行小字"), M.Contains(Birthday)); }
				TestFalse(TEXT("沒有旗標"), Logic.GetTeam().HasFlag(TEXT("Tried1031")));
			}
			// 試過別的錯碼（0000）不算試過 1031
			{
				FEscapeGameLogic Logic;
				TestTrue(TEXT("CSV 載入"), MakeLogic(777, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				Logic.SubmitPassword(TEXT("P1"), TEXT("0000"));
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S18"));
				TestEqual(TEXT("試過 0000 → 還是 X032"), Out.Result.InteractionId, FString(TEXT("X032")));
				for (const FString& M : Out.Messages) { TestFalse(TEXT("沒有那行小字"), M.Contains(Birthday)); }
			}
			// 輸入 1031 失敗：X035
			{
				FEscapeGameLogic Logic;
				TestTrue(TEXT("CSV 載入"), MakeLogic(777, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				TestNotEqual(TEXT("當局密碼不是 1031"), Logic.GetTeam().GetPasswordForDevOnly(), FString(TEXT("1031")));
				const FInteractOutcome Fail = Logic.SubmitPassword(TEXT("P1"), TEXT("1031"));
				TestEqual(TEXT("1031 是輸錯（X017）"), Fail.Result.InteractionId, FString(TEXT("X017")));
				TestTrue(TEXT("旗標 Tried1031"), Logic.GetTeam().HasFlag(TEXT("Tried1031")));
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S18"));
				TestEqual(TEXT("試過 1031 → X035"), Out.Result.InteractionId, FString(TEXT("X035")));
				TestEqual(TEXT("一則文字"), Out.Messages.Num(), 1);
				if (Out.Messages.Num() == 1) { TestTrue(TEXT("有那行小字"), Out.Messages[0].Contains(Birthday)); }
			}
		});

		It(TEXT("不可以 遊戲內任何文字、畫面、log（Shipping 版）出現當局密碼"), [this]()
		{
			// 文字：對每個種子，人類碰每個 S 物件、各種錯碼、正確碼，收集所有顯示文字，都不可以含當局密碼
			const TArray<FString> Objects = SceneObjectIds();
			TestTrue(TEXT("讀到場景物件"), Objects.Num() >= 15);
			int32 Leaks = 0, Collected = 0;
			FString Error;
			for (int32 Seed = 0; Seed < 30; ++Seed)
			{
				TArray<FString> Texts;
				FString Password;
				auto Collect = [&](const FInteractOutcome& Out)
				{
					Texts.Add(Out.Result.Text);
					Texts.Append(Out.Messages);
				};
				for (const FString& Object : Objects)
				{
					FEscapeGameLogic Logic;
					if (!MakeLogic(Seed, Logic, Error)) { AddError(Error); return; }
					Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
					Password = Logic.GetTeam().GetPasswordForDevOnly();
					Collect(Logic.Interact(TEXT("P1"), Object));
				}
				{
					FEscapeGameLogic Logic;
					MakeLogic(Seed, Logic, Error);
					Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
					Logic.AddPlayer(TEXT("P2"), FVector::ZeroVector);
					const FString Real = Logic.GetTeam().GetPasswordForDevOnly();
					for (const FString& Code : { TEXT("0000"), TEXT("1031"), TEXT("9999"), TEXT(""), TEXT("abc") })
					{
						if (Code != Real) { Collect(Logic.SubmitPassword(TEXT("P1"), Code)); }
					}
					Collect(Logic.SubmitPassword(TEXT("P1"), Offset(Real, 1)));
					Collect(Logic.SubmitPassword(TEXT("P1"), Real));  // 正確碼
					Collect(Logic.Interact(TEXT("P2"), TEXT("S01")));
					Collect(Logic.Interact(TEXT("P2"), TEXT("S18")));
				}
				for (const FString& T : Texts)
				{
					++Collected;
					if (T.Contains(Password)) { ++Leaks; }
				}
			}
			TestTrue(TEXT("確實收集了很多文字"), Collected > 600);
			TestEqual(TEXT("沒有任何一則文字含當局密碼"), Leaks, 0);
		});

		It(TEXT("不可以 輸入次數有上限"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(4242, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			int32 Rejected = 0;
			for (int32 Attempt = 0; Attempt < 3000; ++Attempt)
			{
				const FString Code = ChangeOneDigit(Logic.GetTeam().GetPasswordForDevOnly(), Attempt % 4);
				if (Logic.SubmitPassword(TEXT("P1"), Code).Kind != EInteractKind::Resolved) { ++Rejected; }
			}
			TestEqual(TEXT("輸錯 3000 次，每一次都照常回應"), Rejected, 0);
			// 輸錯很多次之後，正確密碼照樣能開門
			const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Logic.GetTeam().GetPasswordForDevOnly());
			TestEqual(TEXT("之後輸入正確密碼仍是 X016"), Out.Result.InteractionId, FString(TEXT("X016")));
			TestEqual(TEXT("結局 E02"), Logic.GetTeam().GetEscapeEnding(TEXT("P1")), FString(TEXT("E02")));
		});

		// ---- PLANS 詮釋（不在 VERIFY 裡）----

		It(TEXT("E05 重啟後密碼重新產生、Tried1031 旗標清掉（PLANS 詮釋）"), [this]()
		{
			FTeamState Team(7);
			const FString First = Team.GetPasswordForDevOnly();
			Team.AddFlag(TEXT("Tried1031"));
			TestTrue(TEXT("旗標已加"), Team.HasFlag(TEXT("Tried1031")));
			TestTrue(TEXT("快照帶旗標"), Team.MakeSnapshot().Flags.Contains(TEXT("Tried1031")));

			const TArray<FTeamEvent> Events = Team.ApplyHpDelta(-5);
			TestEqual(TEXT("E05"), Events.Num(), 1);
			TestEqual(TEXT("重啟一次"), Team.GetRestartCount(), 1);
			TestFalse(TEXT("旗標清掉"), Team.HasFlag(TEXT("Tried1031")));
			TestFalse(TEXT("快照沒有旗標"), Team.MakeSnapshot().Flags.Contains(TEXT("Tried1031")));

			const FString Second = Team.GetPasswordForDevOnly();
			TestTrue(TEXT("新密碼是四位數字"), IsFourDigits(Second));
			TestNotEqual(TEXT("新的一局換了密碼"), Second, First);
			TestNotEqual(TEXT("新密碼也不是 1031"), Second, FString(TEXT("1031")));
			TestEqual(TEXT("舊密碼現在是輸錯"), Team.CheckPassword(First), EPasswordInput::Wrong);
			TestEqual(TEXT("新密碼是正確"), Team.CheckPassword(Second), EPasswordInput::Correct);

			// 重新產生是從同一個種子接著抽：和「同種子再開一局」一樣
			FTeamState Twin(7);
			Twin.StartNewRoundForTesting();
			TestEqual(TEXT("同種子第二局的密碼一樣"), Twin.GetPasswordForDevOnly(), Second);
		});

		It(TEXT("E01～E03 只影響該玩家（PLANS 詮釋）"), [this]()
		{
			FTeamState Team(1);
			Team.AddPlayer(TEXT("P1"));
			Team.AddPlayer(TEXT("P2"));
			Team.AddPlayer(TEXT("P3"));

			TArray<FTeamEvent> Events = Team.ApplyInteraction(FakeEnding(TEXT("E01")), TEXT("P2"));
			TestEqual(TEXT("一個事件"), Events.Num(), 1);
			if (Events.Num() == 1)
			{
				TestEqual(TEXT("E01"), Events[0].EndingId, FString(TEXT("E01")));
				TestEqual(TEXT("只有 P2"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P2") }));
			}
			TestEqual(TEXT("P2 結局 E01"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
			TestFalse(TEXT("P1 還在"), Team.HasEscaped(TEXT("P1")));
			TestFalse(TEXT("P3 還在"), Team.HasEscaped(TEXT("P3")));

			Events = Team.ApplyInteraction(FakeEnding(TEXT("E03")), TEXT("P3"));
			TestEqual(TEXT("E03 一個事件"), Events.Num(), 1);
			if (Events.Num() == 1) { TestEqual(TEXT("只有 P3"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P3") })); }
			TestEqual(TEXT("P3 結局 E03"), Team.GetEscapeEnding(TEXT("P3")), FString(TEXT("E03")));
			TestEqual(TEXT("還在局裡的只有 P1"), Team.GetRemainingPlayers(), TArray<FString>({ TEXT("P1") }));

			// 已結束的玩家、空的、不存在的 PlayerId → 沒有事件，結局不被改寫
			TestEqual(TEXT("已結束的 P2 再 E02 → 沒有事件"), Team.ApplyInteraction(FakeEnding(TEXT("E02")), TEXT("P2")).Num(), 0);
			TestEqual(TEXT("P2 結局仍是 E01"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
			TestEqual(TEXT("PlayerId 空 → 沒有事件"), Team.ApplyInteraction(FakeEnding(TEXT("E02")), FString()).Num(), 0);
			TestEqual(TEXT("不存在的玩家 → 沒有事件"), Team.ApplyInteraction(FakeEnding(TEXT("E02")), TEXT("P9")).Num(), 0);
			TestEqual(TEXT("P1 還在"), Team.GetRemainingPlayers(), TArray<FString>({ TEXT("P1") }));

			Events = Team.ApplyInteraction(FakeEnding(TEXT("E02")), TEXT("P1"));
			TestEqual(TEXT("最後 P1 用 E02 結束"), Events.Num(), 1);
			TestEqual(TEXT("沒有人還在局裡"), Team.GetRemainingPlayers().Num(), 0);
		});

		It(TEXT("機率扣血只在擲中時套用（PLANS 詮釋）"), [this]()
		{
			FInteractionResult Always;
			Always.InteractionId = TEXT("X900");
			Always.bHasHpChance = true;
			Always.HpChanceNumerator = 1;
			Always.HpChanceDenominator = 1;
			Always.HpChanceDelta = -1;
			FInteractionResult Never = Always;
			Never.HpChanceNumerator = 0;

			FTeamState Team(5);
			bool bHit = true;
			Team.ApplyInteraction(Never, TEXT("P1"), &bHit);
			TestFalse(TEXT("機率 0 不會中"), bHit);
			TestEqual(TEXT("生命值沒變"), Team.GetHp(), 5);
			Team.ApplyInteraction(Always, TEXT("P1"), &bHit);
			TestTrue(TEXT("機率 1 一定中"), bHit);
			TestEqual(TEXT("生命值 -1"), Team.GetHp(), 4);

			// 擲中後生命值歸零 → 照常 E05
			FTeamState Low(5);
			Low.AddPlayer(TEXT("P1"));
			Low.ApplyHpDelta(-4);
			TestEqual(TEXT("剩 1"), Low.GetHp(), 1);
			const TArray<FTeamEvent> Events = Low.ApplyInteraction(Always, TEXT("P1"), &bHit);
			TestTrue(TEXT("擲中"), bHit);
			TestEqual(TEXT("觸發 E05"), Events.Num(), 1);
			if (Events.Num() == 1) { TestEqual(TEXT("是 E05"), Events[0].EndingId, FString(TEXT("E05"))); }
			TestEqual(TEXT("重啟後生命值 5"), Low.GetHp(), 5);

			// 沒有機率事件時 OutChanceHit 是 false，舊的呼叫方式也能編譯
			bHit = true;
			FInteractionResult Plain;
			Plain.InteractionId = TEXT("X901");
			Team.ApplyInteraction(Plain, FString(), &bHit);
			TestFalse(TEXT("沒有機率事件 → false"), bHit);
			Team.ApplyInteraction(Plain);
			Team.ApplyInteraction(Plain, TEXT("P1"));
		});

		It(TEXT("載入時檢查機率扣血列剛好有兩段「…」，否則報錯並寫出互動 ID（PLANS 詮釋）"), [this]()
		{
			const FString Original = LoadText(TEXT("DT_Interactions.csv"));
			const FString Broken = Original.Replace(TEXT("。觸發機率事件時：「手指抽筋了」"), TEXT(""));
			TestTrue(TEXT("測試用的壞資料真的改到了"), Broken != Original);
			FEscapeGameLogic Logic;
			FString Error;
			const bool bLoaded = FEscapeGameLogic::Load(Broken, LoadText(TEXT("DT_Forms.csv")), LoadText(TEXT("DT_SceneObjects.csv")),
				LoadText(TEXT("DT_Responses.csv")), LoadText(TEXT("DT_Locations.csv")), 1, Logic, Error);
			TestFalse(TEXT("只有一段「…」→ 載入失敗"), bLoaded);
			TestTrue(*FString::Printf(TEXT("錯誤訊息有互動 ID：%s"), *Error), Error.Contains(TEXT("X017")));
		});

		It(TEXT("Interact 的其他情形與 Tick（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(3, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);

			TestTrue(TEXT("不存在的玩家 → Rejected"), Logic.Interact(TEXT("P9"), TEXT("S01")).Kind == EInteractKind::Rejected);
			TestTrue(TEXT("不存在的玩家輸入密碼 → Rejected"), Logic.SubmitPassword(TEXT("P9"), TEXT("0000")).Kind == EInteractKind::Rejected);

			// 一般互動：Resolved、Messages 就是 Result.Text 一則
			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S18"));
			TestTrue(TEXT("碰月曆 → Resolved"), Out.Kind == EInteractKind::Resolved);
			TestEqual(TEXT("Messages 是 Result.Text"), Out.Messages, TArray<FString>({ Out.Result.Text }));
			TestTrue(TEXT("X032 已觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X032")));

			TestEqual(TEXT("Tick 前 1800"), Logic.GetTeam().GetRemainingSeconds(), 1800);
			TestEqual(TEXT("Tick 沒有事件"), Logic.Tick(1.0f).Num(), 0);
			TestEqual(TEXT("Tick 轉給 TeamState"), Logic.GetTeam().GetRemainingSeconds(), 1799);
			TestTrue(TEXT("可以取得 Resolver"), Logic.GetResolver().GetRows().Num() > 30);
		});
	});
}

#endif  // !UE_BUILD_SHIPPING
