// 步驟 8 成就 → VERIFY STEP 8
// 整個檔案只在非 Shipping 編譯：E02 要用 GetPasswordForDevOnly()（Shipping 沒有任何途徑拿到密碼）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/AchievementTracker.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

namespace Step8Spec
{
	constexpr float Frame = 1.0f / 60.0f;  // 固定一格

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

	/** 清單裡有沒有「這位玩家的這個成就」 */
	int32 CountOf(const TArray<FAchievementUnlock>& List, const FString& Player, const FString& Id)
	{
		int32 N = 0;
		for (const FAchievementUnlock& U : List) { if (U.PlayerId == Player && U.AchievementId == Id) { ++N; } }
		return N;
	}

	bool Has(const FInteractOutcome& Out, const FString& Player, const FString& Id)
	{
		return CountOf(Out.NewAchievements, Player, Id) > 0;
	}

	/** 人類碰講桌（S16）：互動表沒有人類碰講桌的列，一定是預設嘲諷 */
	FInteractOutcome Taunt(FEscapeGameLogic& Logic, const FString& Player)
	{
		return Logic.Interact(Player, TEXT("S16"));
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

	/** E03 路線：X009 → F04 → X005 → 變回人類 → X006 → F05 → X013 → F07。成功時玩家是長竿子（F07）。 */
	bool BecomeLongPole(FEscapeGameLogic& Logic, const FString& Player)
	{
		const auto Becomes = [&](const FString& Target) { return Logic.Interact(Player, Target).bTransformed && Logic.GetPlayerForm(Player) == Target; };
		return BecomeOfficeKey(Logic, Player) && Logic.Interact(Player, TEXT("S05")).Result.InteractionId == TEXT("X005")
			&& Logic.ReturnToHuman(Player) && Logic.Interact(Player, TEXT("S06")).Result.InteractionId == TEXT("X006") && Becomes(TEXT("F05"))
			&& Logic.Interact(Player, TEXT("S09")).Result.InteractionId == TEXT("X013") && Becomes(TEXT("F07"));
	}

	/** 讓 Player 變成椅子（如果還不是），再連碰配電箱（X025 每次 -1）直到全隊生命值歸零。回傳最後一次互動的結果。 */
	FInteractOutcome DriveToE05(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.GetPlayerForm(Player) != TEXT("F12")) { Logic.Interact(Player, TEXT("F12")); }
		FInteractOutcome Last;
		for (int32 I = 0; I < FTeamState::StartingHp + 1 && Logic.GetTeam().GetHp() > 0; ++I)
		{
			Last = Logic.Interact(Player, TEXT("S15"));
			bool bE05 = false;
			for (const FTeamEvent& E : Last.Events) { if (E.EndingId == TEXT("E05")) { bE05 = true; } }
			if (bE05) { break; }
		}
		return Last;
	}

	bool HasEvent(const TArray<FTeamEvent>& Events, const FString& Ending)
	{
		for (const FTeamEvent& E : Events) { if (E.EndingId == Ending) { return true; } }
		return false;
	}

	/** 讓 Player 在 Logic 裡以學生證逃出（E01） */
	bool EscapeWithStudentCard(FEscapeGameLogic& Logic, const FString& Player)
	{
		return BecomeStudentCard(Logic, Player) && Logic.Interact(Player, TEXT("S01")).Result.EndingId == TEXT("E01") && Logic.GetTeam().HasEscaped(Player);
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep8Spec, "EscapeGame.Step8",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep8Spec)

void FEscapeGameStep8Spec::Define()
{
	using namespace Step8Spec;

	Describe(TEXT("VERIFY STEP 8"), [this]()
	{
		It(TEXT("觸發預設嘲諷回應第 10 次 → 解鎖 A01「被嘲笑專家」；第 9 次時還沒解鎖"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			for (int32 I = 1; I <= 9; ++I)
			{
				const FInteractOutcome Out = Taunt(Logic, TEXT("P1"));
				TestTrue(*FString::Printf(TEXT("第 %d 次：互動成立"), I), Out.Kind == EInteractKind::Resolved);
				TestTrue(*FString::Printf(TEXT("第 %d 次：是預設嘲諷"), I), Out.Result.bIsDefault);
				TestFalse(*FString::Printf(TEXT("第 %d 次：還沒解鎖 A01"), I), Has(Out, TEXT("P1"), TEXT("A01")));
			}
			TestEqual(TEXT("9 次嘲諷"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 9);
			TestFalse(TEXT("第 9 次時 A01 還沒解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("全隊聯集也沒有 A01"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A01")));

			const FInteractOutcome Tenth = Taunt(Logic, TEXT("P1"));
			TestTrue(TEXT("第 10 次還是預設嘲諷"), Tenth.Result.bIsDefault);
			TestTrue(TEXT("第 10 次：這次新解鎖 A01"), Has(Tenth, TEXT("P1"), TEXT("A01")));
			TestEqual(TEXT("只解鎖一個"), Tenth.NewAchievements.Num(), 1);
			TestTrue(TEXT("P1 的 A01 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestTrue(TEXT("全隊聯集有 A01"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A01")));

			const FInteractOutcome Eleventh = Taunt(Logic, TEXT("P1"));
			TestFalse(TEXT("第 11 次：NewAchievements 不再有 A01"), Has(Eleventh, TEXT("P1"), TEXT("A01")));
			TestEqual(TEXT("第 11 次：沒有任何新解鎖"), Eleventh.NewAchievements.Num(), 0);
			TestTrue(TEXT("A01 仍然是解鎖的"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
		});

		It(TEXT("累計變身第 20 次 → 解鎖 A02「我是誰」"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			// 椅子（F12）與骰子（F08）來回變身；每 4 次中間變回人類一次（變回人類不算變身）
			for (int32 I = 1; I <= 20; ++I)
			{
				if (I % 4 == 0) { TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1"))); }
				const FString Target = (I % 2 == 1) ? TEXT("F12") : TEXT("F08");
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), Target);
				TestTrue(*FString::Printf(TEXT("第 %d 次變身成功"), I), Out.bTransformed);
				TestEqual(*FString::Printf(TEXT("第 %d 次後的變身次數"), I), Logic.GetAchievements().GetTransformCount(TEXT("P1")), I);
				if (I < 20)
				{
					TestFalse(*FString::Printf(TEXT("第 %d 次：還沒解鎖 A02"), I), Has(Out, TEXT("P1"), TEXT("A02")));
					TestFalse(*FString::Printf(TEXT("第 %d 次後 A02 還沒解鎖"), I), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A02")));
				}
				else
				{
					TestTrue(TEXT("第 20 次：解鎖 A02"), Has(Out, TEXT("P1"), TEXT("A02")));
				}
			}
			TestTrue(TEXT("P1 的 A02 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A02")));
			TestEqual(TEXT("變身不是預設嘲諷：嘲諷次數 0"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 0);
			TestFalse(TEXT("A01 沒有被誤解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
		});

		It(TEXT("人類身體連續離開（處於物品形態）超過 10 分鐘 → 解鎖 A03「魂不附體」；9 分 59 秒時還沒解鎖"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);

			// 9 分 59 秒 = 599 秒 = 35940 格
			for (int32 I = 0; I < 599 * 60; ++I) { Logic.Tick(Frame); }
			TestTrue(*FString::Printf(TEXT("累計約 599 秒，實際 %f"), Logic.GetAchievements().GetItemFormSeconds(TEXT("P1"))),
				FMath::IsNearlyEqual(Logic.GetAchievements().GetItemFormSeconds(TEXT("P1")), 599.0, 0.01));
			TestFalse(TEXT("9 分 59 秒時還沒解鎖 A03"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A03")));
			TestEqual(TEXT("沒有新解鎖"), Logic.PopNewAchievements().Num(), 0);

			// 600 秒整還不算「超過」
			for (int32 I = 0; I < 60; ++I) { Logic.Tick(Frame); }
			TestFalse(TEXT("剛好 10 分鐘還沒解鎖 A03（要超過）"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A03")));

			// 再多一格
			Logic.Tick(Frame);
			TestTrue(TEXT("超過 10 分鐘 → 解鎖 A03"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A03")));
			TestTrue(TEXT("全隊聯集有 A03"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A03")));
			const TArray<FAchievementUnlock> Fresh = Logic.PopNewAchievements();
			TestEqual(TEXT("新解鎖剛好一個"), Fresh.Num(), 1);
			TestEqual(TEXT("是 P1 的 A03"), CountOf(Fresh, TEXT("P1"), TEXT("A03")), 1);
			TestFalse(TEXT("人類形態的 P2 沒有 A03"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A03")));

			// 之後繼續累計也不會重複回報
			for (int32 I = 0; I < 600; ++I) { Logic.Tick(Frame); }
			TestEqual(TEXT("不再回報 A03"), Logic.PopNewAchievements().Num(), 0);
		});

		It(TEXT("拆開牛皮紙包（X008）→ 解鎖 A04"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			TestTrue(TEXT("P1 變成辦公室鑰匙"), BecomeOfficeKey(Logic, TEXT("P1")));
			TestEqual(TEXT("開辦公室門（X005）"), Logic.Interact(TEXT("P1"), TEXT("S05")).Result.InteractionId, FString(TEXT("X005")));
			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestFalse(TEXT("拆之前沒有 A04"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A04")));

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S08"));
			TestEqual(TEXT("是 X008"), Out.Result.InteractionId, FString(TEXT("X008")));
			TestTrue(TEXT("這次新解鎖 A04（給做這個互動的玩家）"), Has(Out, TEXT("P1"), TEXT("A04")));
			TestTrue(TEXT("P1 的 A04 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A04")));
			TestFalse(TEXT("P2 沒有 A04"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A04")));
			TestTrue(TEXT("全隊聯集有 A04"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A04")));
		});

		It(TEXT("鑰匙圈碰過 S01、S05、S09、S19 四扇門 → 解鎖 A07；只碰過三扇 → 還沒解鎖"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestTrue(TEXT("P1 變成鑰匙圈（F11）"), Logic.Interact(TEXT("P1"), TEXT("F11")).bTransformed);

			// 同一扇門碰很多次不算多扇；S01 碰兩次、S05、S09 → 只有三扇
			for (const TCHAR* Door : { TEXT("S01"), TEXT("S01"), TEXT("S05"), TEXT("S09") })
			{
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), Door);
				TestEqual(*FString::Printf(TEXT("碰 %s 是 X033"), Door), Out.Result.InteractionId, FString(TEXT("X033")));
				TestEqual(*FString::Printf(TEXT("碰 %s 還沒有額外觸發"), Door), Out.Result.ExtraTriggers.Num(), 0);
				TestFalse(*FString::Printf(TEXT("碰 %s 沒有新成就"), Door), Has(Out, TEXT("P1"), TEXT("A07")));
			}
			TestEqual(TEXT("碰過三扇不同的門"), Logic.GetAchievements().GetTouchedDoors(TEXT("P1")).Num(), 3);
			TestFalse(TEXT("只碰過三扇 → 還沒解鎖 A07"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A07")));
			TestFalse(TEXT("X034 還沒觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X034")));

			const FInteractOutcome Fourth = Logic.Interact(TEXT("P1"), TEXT("S19"));
			TestEqual(TEXT("第四扇還是 X033"), Fourth.Result.InteractionId, FString(TEXT("X033")));
			TestEqual(TEXT("額外觸發 X034"), Fourth.Result.ExtraTriggers.Num(), 1);
			TestTrue(TEXT("第四扇 → 這次新解鎖 A07"), Has(Fourth, TEXT("P1"), TEXT("A07")));
			TestTrue(TEXT("P1 的 A07 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A07")));
			TestTrue(TEXT("X034 已觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X034")));
			TestTrue(TEXT("全隊聯集有 A07"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A07")));

			// 之後再碰門仍會觸發 X034，但不會再回報 A07
			const FInteractOutcome Again = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestFalse(TEXT("再碰門不再回報 A07"), Has(Again, TEXT("P1"), TEXT("A07")));
		});

		It(TEXT("觸發 E01 結局 → 解鎖同名的結局成就 E01（學生證碰大門）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 走到學生證"), BecomeStudentCard(Logic, TEXT("P1")));
			TestFalse(TEXT("逃出之前沒有 E01"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E01")));

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestTrue(TEXT("E01 事件"), HasEvent(Out.Events, TEXT("E01")));
			TestTrue(TEXT("P1 這次新解鎖 E01"), Has(Out, TEXT("P1"), TEXT("E01")));
			TestTrue(TEXT("P1 的 E01 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E01")));
			TestFalse(TEXT("P2 沒有 E01（E01 只影響逃出的那位）"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("E01")));
			TestTrue(TEXT("全隊聯集有 E01"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E01")));
		});

		It(TEXT("觸發 E02 結局 → 解鎖同名的結局成就 E02（輸入正確密碼）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(7, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Logic.GetTeam().GetPasswordForDevOnly());
			TestTrue(TEXT("E02 事件"), HasEvent(Out.Events, TEXT("E02")));
			TestTrue(TEXT("P1 這次新解鎖 E02"), Has(Out, TEXT("P1"), TEXT("E02")));
			TestTrue(TEXT("P1 的 E02 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E02")));
			TestFalse(TEXT("P2 沒有 E02"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("E02")));
			TestTrue(TEXT("全隊聯集有 E02"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E02")));
		});

		It(TEXT("觸發 E03 結局 → 解鎖同名的結局成就 E03（長竿子開高窗）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 走完 X009、F04、X005、X006、F05、X013、F07"), BecomeLongPole(Logic, TEXT("P1")));
			TestFalse(TEXT("逃出之前沒有 E03"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E03")));

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S13"));
			TestEqual(TEXT("是 X015"), Out.Result.InteractionId, FString(TEXT("X015")));
			TestTrue(TEXT("E03 事件"), HasEvent(Out.Events, TEXT("E03")));
			TestTrue(TEXT("P1 這次新解鎖 E03"), Has(Out, TEXT("P1"), TEXT("E03")));
			TestTrue(TEXT("P1 的 E03 已解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E03")));
			TestFalse(TEXT("P2 沒有 E03"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("E03")));
			TestTrue(TEXT("全隊聯集有 E03"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E03")));
		});

		It(TEXT("觸發 E04 結局 → 解鎖同名的結局成就 E04（時鐘 X021 撥快指針）：還沒逃出的玩家都有，已逃出的沒有"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.AddPlayer(TEXT("P3"), FVector(0, 100, 0));
			TestTrue(TEXT("P3 先用學生證逃出（E01）"), EscapeWithStudentCard(Logic, TEXT("P3")));
			TestTrue(TEXT("P1 變成時鐘（F09）"), Logic.Interact(TEXT("P1"), TEXT("F09")).bTransformed);
			Logic.PopNewAchievements();

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("SELF"));
			TestEqual(TEXT("是 X021"), Out.Result.InteractionId, FString(TEXT("X021")));
			TestTrue(TEXT("E04 事件"), HasEvent(Out.Events, TEXT("E04")));
			TestTrue(TEXT("P1 這次新解鎖 E04"), Has(Out, TEXT("P1"), TEXT("E04")));
			TestTrue(TEXT("P2 這次新解鎖 E04（套用到所有還沒逃出的玩家）"), Has(Out, TEXT("P2"), TEXT("E04")));
			TestFalse(TEXT("P3 已逃出，沒有 E04"), Logic.GetAchievements().IsUnlocked(TEXT("P3"), TEXT("E04")));
			TestTrue(TEXT("P3 仍然有自己的 E01"), Logic.GetAchievements().IsUnlocked(TEXT("P3"), TEXT("E01")));
			TestTrue(TEXT("A06 給做互動的 P1"), Has(Out, TEXT("P1"), TEXT("A06")));
			TestFalse(TEXT("P2 沒有 A06"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A06")));
			TestTrue(TEXT("全隊聯集有 E04"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E04")));
			TestTrue(TEXT("全隊聯集有 A06"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A06")));
		});

		It(TEXT("觸發 E04 結局 → 解鎖同名的結局成就 E04（30 分鐘時間到）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			for (int32 I = 0; I < 1800 * 60 - 60; ++I) { Logic.Tick(Frame); }
			TestFalse(TEXT("還沒到時間：沒有 E04"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E04")));
			for (int32 I = 0; I < 61 && !Logic.GetTeam().HasEscaped(TEXT("P1")); ++I) { Logic.Tick(Frame); }
			TestTrue(TEXT("時間到"), Logic.GetTeam().HasEscaped(TEXT("P1")));
			TestTrue(TEXT("P1 有 E04"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E04")));
			TestTrue(TEXT("P2 有 E04"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("E04")));
			TestFalse(TEXT("時間到不解鎖 A06"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A06")));
		});

		It(TEXT("觸發 E05 結局 → 解鎖同名的結局成就 E05（血量歸零）：還沒逃出的玩家都有，已逃出的沒有"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.AddPlayer(TEXT("P3"), FVector(0, 100, 0));
			TestTrue(TEXT("P3 先用學生證逃出（E01）"), EscapeWithStudentCard(Logic, TEXT("P3")));
			TestFalse(TEXT("歸零之前沒有 E05"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E05")));

			const FInteractOutcome Last = DriveToE05(Logic, TEXT("P1"));
			TestTrue(TEXT("E05 事件"), HasEvent(Last.Events, TEXT("E05")));
			TestTrue(TEXT("P1 這次新解鎖 E05"), Has(Last, TEXT("P1"), TEXT("E05")));
			TestTrue(TEXT("P2 這次新解鎖 E05"), Has(Last, TEXT("P2"), TEXT("E05")));
			TestFalse(TEXT("P3 已逃出，沒有 E05"), Logic.GetAchievements().IsUnlocked(TEXT("P3"), TEXT("E05")));
			TestEqual(TEXT("遊戲重啟，生命值回 5"), Logic.GetTeam().GetHp(), FTeamState::StartingHp);
			TestTrue(TEXT("E05 重啟後成就還在"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E05")));
			TestTrue(TEXT("全隊聯集有 E05"), Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E05")));
		});

		It(TEXT("不可以 同一個成就解鎖兩次（不重複跳提示）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			// A01：嘲諷 15 次，只回報一次
			for (int32 I = 0; I < 15; ++I) { Taunt(Logic, TEXT("P1")); }
			// A04：拆兩次牛皮紙包，只回報一次（X008 第二次仍然成立，但成就已經有了）
			TestTrue(TEXT("P1 變成辦公室鑰匙"), BecomeOfficeKey(Logic, TEXT("P1")));
			Logic.Interact(TEXT("P1"), TEXT("S05"));
			Logic.ReturnToHuman(TEXT("P1"));
			const FInteractOutcome FirstOpen = Logic.Interact(TEXT("P1"), TEXT("S08"));
			const FInteractOutcome SecondOpen = Logic.Interact(TEXT("P1"), TEXT("S08"));
			TestEqual(TEXT("第二次還是 X008"), SecondOpen.Result.InteractionId, FString(TEXT("X008")));
			TestTrue(TEXT("第一次拆開 → A04"), Has(FirstOpen, TEXT("P1"), TEXT("A04")));
			TestFalse(TEXT("第二次拆開 → 不再回報 A04"), Has(SecondOpen, TEXT("P1"), TEXT("A04")));
			TestEqual(TEXT("第二次沒有任何新解鎖"), SecondOpen.NewAchievements.Num(), 0);

			const TArray<FAchievementUnlock> Fresh = Logic.PopNewAchievements();
			TestEqual(TEXT("佇列裡 A01 只有一筆"), CountOf(Fresh, TEXT("P1"), TEXT("A01")), 1);
			TestEqual(TEXT("佇列裡 A04 只有一筆"), CountOf(Fresh, TEXT("P1"), TEXT("A04")), 1);

			// 追蹤器本身：第一次 true，之後 false；清單沒有重複
			FAchievementTracker Tracker;
			TestTrue(TEXT("第一次解鎖回傳 true"), Tracker.Unlock(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("第二次解鎖回傳 false"), Tracker.Unlock(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("第三次解鎖回傳 false"), Tracker.Unlock(TEXT("P1"), TEXT("A01")));
			TestEqual(TEXT("已解鎖清單只有一筆"), Tracker.GetUnlocked(TEXT("P1")).Num(), 1);
			TestFalse(TEXT("空的玩家 ID 不能解鎖"), Tracker.Unlock(FString(), TEXT("A01")));
			TestFalse(TEXT("空的成就 ID 不能解鎖"), Tracker.Unlock(TEXT("P1"), FString()));
		});

		It(TEXT("不可以 成就寫進磁碟或任何永久儲存：新建的 FEscapeGameLogic 沒有上一個實例的成就（沒有 static 或全域狀態）"), [this]()
		{
			FString Error;
			{
				FEscapeGameLogic First;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, First, Error));
				First.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				for (int32 I = 0; I < 10; ++I) { Taunt(First, TEXT("P1")); }
				TestTrue(TEXT("第一個實例：A01 已解鎖"), First.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
				TestTrue(TEXT("第一個實例：全隊聯集有 A01"), First.GetTeam().GetUnlockedAchievements().Contains(TEXT("A01")));
			}  // 第一個實例結束（相當於關閉或重新整理網頁）

			FEscapeGameLogic Second;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Second, Error));
			Second.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestFalse(TEXT("新實例沒有上一個實例的 A01"), Second.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestEqual(TEXT("新實例沒有任何已解鎖成就"), Second.GetAchievements().GetUnlocked(TEXT("P1")).Num(), 0);
			TestEqual(TEXT("新實例的全隊聯集是空的"), Second.GetTeam().GetUnlockedAchievements().Num(), 0);
			TestEqual(TEXT("新實例的嘲諷次數歸零"), Second.GetAchievements().GetTauntCount(TEXT("P1")), 0);
			TestEqual(TEXT("新實例沒有待取出的成就"), Second.PopNewAchievements().Num(), 0);

			FAchievementTracker Fresh;
			TestFalse(TEXT("新的追蹤器沒有任何成就"), Fresh.IsUnlocked(TEXT("P1"), TEXT("A01")));
		});
	});

	Describe(TEXT("PLANS 步驟 8 的詮釋"), [this]()
	{
		It(TEXT("成就與計數跨 E05 重啟保留：嘲諷 6 次 → E05 → 再 4 次就解鎖 A01；E05 本身也解鎖"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			for (int32 I = 0; I < 6; ++I) { Taunt(Logic, TEXT("P1")); }
			TestEqual(TEXT("E05 之前嘲諷 6 次"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 6);
			TestFalse(TEXT("還沒有 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));

			const FInteractOutcome Last = DriveToE05(Logic, TEXT("P1"));
			TestTrue(TEXT("E05 事件"), HasEvent(Last.Events, TEXT("E05")));
			TestEqual(TEXT("重啟後 P1 是人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestTrue(TEXT("E05 本身解鎖"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E05")));
			TestEqual(TEXT("嘲諷次數跨 E05 保留"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 6);
			TestEqual(TEXT("變身次數跨 E05 保留（變成椅子 1 次）"), Logic.GetAchievements().GetTransformCount(TEXT("P1")), 1);

			for (int32 I = 1; I <= 4; ++I)
			{
				const FInteractOutcome Out = Taunt(Logic, TEXT("P1"));
				TestTrue(*FString::Printf(TEXT("E05 後第 %d 次：是預設嘲諷"), I), Out.Result.bIsDefault);
				TestTrue(*FString::Printf(TEXT("E05 後第 %d 次：A01 解鎖與否應為 %d"), I, I == 4), Has(Out, TEXT("P1"), TEXT("A01")) == (I == 4));
			}
			TestTrue(TEXT("再 4 次就解鎖 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestTrue(TEXT("E05 的成就還在"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("E05")));
			TestTrue(TEXT("全隊聯集有 E05 與 A01"),
				Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("E05")) && Logic.GetTeam().GetUnlockedAchievements().Contains(TEXT("A01")));
		});

		It(TEXT("鑰匙圈碰過的門跨 E05 保留"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestTrue(TEXT("變成鑰匙圈"), Logic.Interact(TEXT("P1"), TEXT("F11")).bTransformed);
			for (const TCHAR* Door : { TEXT("S01"), TEXT("S05"), TEXT("S09") }) { Logic.Interact(TEXT("P1"), Door); }
			TestEqual(TEXT("碰過三扇"), Logic.GetAchievements().GetTouchedDoors(TEXT("P1")).Num(), 3);

			// 鑰匙圈（非人類）碰配電箱 -1，連碰到 E05
			FInteractOutcome Last;
			for (int32 I = 0; I < FTeamState::StartingHp && !HasEvent(Last.Events, TEXT("E05")); ++I) { Last = Logic.Interact(TEXT("P1"), TEXT("S15")); }
			TestTrue(TEXT("E05 事件"), HasEvent(Last.Events, TEXT("E05")));
			TestEqual(TEXT("碰過的門跨 E05 保留"), Logic.GetAchievements().GetTouchedDoors(TEXT("P1")).Num(), 3);
			TestFalse(TEXT("E05 清掉已觸發：X033 不再算已觸發"), Logic.GetTeam().GetTriggeredIds().Contains(TEXT("X033")));

			TestTrue(TEXT("重啟後再變成鑰匙圈"), Logic.Interact(TEXT("P1"), TEXT("F11")).bTransformed);
			const FInteractOutcome Fourth = Logic.Interact(TEXT("P1"), TEXT("S19"));
			TestTrue(TEXT("重啟後只碰第四扇 → 解鎖 A07"), Has(Fourth, TEXT("P1"), TEXT("A07")));
		});

		It(TEXT("成就是每位玩家各自的：P1 的嘲諷不算 P2 的"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			// 交錯：P1 嘲諷 6 次、P2 嘲諷 4 次 → 合計 10 次，但各自都不到
			for (int32 I = 0; I < 6; ++I) { Taunt(Logic, TEXT("P1")); }
			for (int32 I = 0; I < 4; ++I) { Taunt(Logic, TEXT("P2")); }
			TestEqual(TEXT("P1 嘲諷 6 次"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 6);
			TestEqual(TEXT("P2 嘲諷 4 次"), Logic.GetAchievements().GetTauntCount(TEXT("P2")), 4);
			TestFalse(TEXT("P1 沒有 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("P2 沒有 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A01")));

			for (int32 I = 0; I < 4; ++I) { Taunt(Logic, TEXT("P1")); }
			TestTrue(TEXT("P1 到 10 次 → 解鎖 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("P2 還是沒有 A01"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A01")));
			TestEqual(TEXT("P2 沒有任何成就"), Logic.GetAchievements().GetUnlocked(TEXT("P2")).Num(), 0);

			// 變身次數、門也各自
			TestTrue(TEXT("P2 變成鑰匙圈"), Logic.Interact(TEXT("P2"), TEXT("F11")).bTransformed);
			Logic.Interact(TEXT("P2"), TEXT("S01"));
			TestEqual(TEXT("P2 碰過 1 扇門"), Logic.GetAchievements().GetTouchedDoors(TEXT("P2")).Num(), 1);
			TestEqual(TEXT("P1 沒碰過門"), Logic.GetAchievements().GetTouchedDoors(TEXT("P1")).Num(), 0);
			TestEqual(TEXT("P1 變身 0 次"), Logic.GetAchievements().GetTransformCount(TEXT("P1")), 0);
			TestEqual(TEXT("P2 變身 1 次"), Logic.GetAchievements().GetTransformCount(TEXT("P2")), 1);
		});

		It(TEXT("PopNewAchievements 收到 Interact、Tick 與事件來的解鎖，取出後清空"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestEqual(TEXT("一開始佇列是空的"), Logic.PopNewAchievements().Num(), 0);

			// Interact：A01
			for (int32 I = 0; I < 10; ++I) { Taunt(Logic, TEXT("P1")); }
			const TArray<FAchievementUnlock> FromInteract = Logic.PopNewAchievements();
			TestEqual(TEXT("Interact 來的：只有 P1 的 A01"), FromInteract.Num(), 1);
			TestEqual(TEXT("Interact 來的：P1 的 A01"), CountOf(FromInteract, TEXT("P1"), TEXT("A01")), 1);
			TestEqual(TEXT("取出後清空"), Logic.PopNewAchievements().Num(), 0);

			// Tick：A03；再 Tick 到時間到：E04（事件來的，兩位玩家各一筆）
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			Logic.PopNewAchievements();  // 變身沒有新成就；清掉以防萬一
			for (int32 I = 0; I < 601 * 60; ++I) { Logic.Tick(Frame); }
			const TArray<FAchievementUnlock> FromTick = Logic.PopNewAchievements();
			TestEqual(TEXT("Tick 來的：只有一筆"), FromTick.Num(), 1);
			TestEqual(TEXT("Tick 來的：P1 的 A03"), CountOf(FromTick, TEXT("P1"), TEXT("A03")), 1);
			TestEqual(TEXT("取出後清空"), Logic.PopNewAchievements().Num(), 0);

			for (int32 I = 0; I < 1800 * 60 && !Logic.GetTeam().HasEscaped(TEXT("P2")); ++I) { Logic.Tick(Frame); }
			const TArray<FAchievementUnlock> FromEvent = Logic.PopNewAchievements();
			TestEqual(TEXT("事件來的：P1 與 P2 各一筆 E04"), FromEvent.Num(), 2);
			TestEqual(TEXT("事件來的：P1 的 E04"), CountOf(FromEvent, TEXT("P1"), TEXT("E04")), 1);
			TestEqual(TEXT("事件來的：P2 的 E04"), CountOf(FromEvent, TEXT("P2"), TEXT("E04")), 1);
			TestEqual(TEXT("取出後清空"), Logic.PopNewAchievements().Num(), 0);
		});

		It(TEXT("Interact 的 NewAchievements 和佇列一致，事件與互動資料來的解鎖都在"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 變成時鐘"), Logic.Interact(TEXT("P1"), TEXT("F09")).bTransformed);
			Logic.PopNewAchievements();

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("SELF"));
			const TArray<FAchievementUnlock> Queue = Logic.PopNewAchievements();
			TestEqual(TEXT("A06 與兩位玩家的 E04，共 3 筆"), Out.NewAchievements.Num(), 3);
			TestEqual(TEXT("佇列和 NewAchievements 筆數相同"), Queue.Num(), Out.NewAchievements.Num());
			TestEqual(TEXT("佇列有 P1 的 A06"), CountOf(Queue, TEXT("P1"), TEXT("A06")), 1);
			TestEqual(TEXT("佇列有 P1 的 E04"), CountOf(Queue, TEXT("P1"), TEXT("E04")), 1);
			TestEqual(TEXT("佇列有 P2 的 E04"), CountOf(Queue, TEXT("P2"), TEXT("E04")), 1);
		});

		It(TEXT("A03：物品換物品不歸零；變回人類歸零；E05 重啟歸零"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			// 物品換物品：椅子 300 秒 → 骰子 301 秒，累計 601 秒 → 解鎖
			TestTrue(TEXT("變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			for (int32 I = 0; I < 300 * 60; ++I) { Logic.Tick(Frame); }
			TestTrue(TEXT("椅子直接變成骰子"), Logic.Interact(TEXT("P1"), TEXT("F08")).bTransformed);
			TestTrue(TEXT("換物品沒有歸零（約 300 秒）"), Logic.GetAchievements().GetItemFormSeconds(TEXT("P1")) > 299.0);
			for (int32 I = 0; I < 301 * 60; ++I) { Logic.Tick(Frame); }
			TestTrue(TEXT("物品換物品不中斷：累計超過 10 分鐘 → A03"), Logic.GetAchievements().IsUnlocked(TEXT("P1"), TEXT("A03")));

			// 另一位玩家：500 秒 → 變回人類 → 再變身 200 秒（沒歸零的話會超過 600）
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P2 變成鑰匙圈"), Logic.Interact(TEXT("P2"), TEXT("F11")).bTransformed);
			for (int32 I = 0; I < 500 * 60; ++I) { Logic.Tick(Frame); }
			TestTrue(TEXT("P2 變回人類"), Logic.ReturnToHuman(TEXT("P2")));
			TestEqual(TEXT("變回人類 → 歸零"), Logic.GetAchievements().GetItemFormSeconds(TEXT("P2")), 0.0);
			TestTrue(TEXT("P2 再變成鑰匙圈"), Logic.Interact(TEXT("P2"), TEXT("F11")).bTransformed);
			for (int32 I = 0; I < 200 * 60; ++I) { Logic.Tick(Frame); }
			TestFalse(TEXT("歸零後只累計 200 秒 → 沒有 A03"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A03")));
			TestTrue(TEXT("人類形態時不累計：變回人類之後的秒數在 200 秒左右"),
				FMath::IsNearlyEqual(Logic.GetAchievements().GetItemFormSeconds(TEXT("P2")), 200.0, 0.1));
		});

		It(TEXT("A03：E05 重啟後連續秒數歸零"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P2 變成鑰匙圈"), Logic.Interact(TEXT("P2"), TEXT("F11")).bTransformed);
			for (int32 I = 0; I < 500 * 60; ++I) { Logic.Tick(Frame); }
			TestTrue(TEXT("P2 累計約 500 秒"), Logic.GetAchievements().GetItemFormSeconds(TEXT("P2")) > 499.0);

			const FInteractOutcome Last = DriveToE05(Logic, TEXT("P1"));
			TestTrue(TEXT("E05 事件"), HasEvent(Last.Events, TEXT("E05")));
			TestEqual(TEXT("P2 被帶回人類"), Logic.GetPlayerForm(TEXT("P2")), FString(TEXT("F00")));
			TestEqual(TEXT("E05 → P2 的連續秒數歸零"), Logic.GetAchievements().GetItemFormSeconds(TEXT("P2")), 0.0);

			TestTrue(TEXT("P2 再變成鑰匙圈"), Logic.Interact(TEXT("P2"), TEXT("F11")).bTransformed);
			for (int32 I = 0; I < 200 * 60; ++I) { Logic.Tick(Frame); }
			TestFalse(TEXT("重啟後只累計 200 秒 → 沒有 A03"), Logic.GetAchievements().IsUnlocked(TEXT("P2"), TEXT("A03")));
		});

		It(TEXT("被擋下、被拒絕、需要密碼、變身都不算預設嘲諷"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			TestEqual(TEXT("變身不算嘲諷"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 0);

			TestTrue(TEXT("P2 碰 P1 正在當的椅子 → Blocked"), Logic.Interact(TEXT("P2"), TEXT("F12")).Kind == EInteractKind::Blocked);
			TestTrue(TEXT("P1 碰自己的椅子 → Rejected"), Logic.Interact(TEXT("P1"), TEXT("F12")).Kind == EInteractKind::Rejected);
			TestTrue(TEXT("碰不存在的 ID → Rejected"), Logic.Interact(TEXT("P2"), TEXT("S99")).Kind == EInteractKind::Rejected);
			TestTrue(TEXT("人類碰大門 → 需要密碼"), Logic.Interact(TEXT("P2"), TEXT("S01")).Kind == EInteractKind::NeedsPassword);
			TestEqual(TEXT("P1 嘲諷 0 次"), Logic.GetAchievements().GetTauntCount(TEXT("P1")), 0);
			TestEqual(TEXT("P2 嘲諷 0 次"), Logic.GetAchievements().GetTauntCount(TEXT("P2")), 0);
			TestEqual(TEXT("P2 變身 0 次"), Logic.GetAchievements().GetTransformCount(TEXT("P2")), 0);

			// 真的預設嘲諷才算
			TestTrue(TEXT("P2 碰講桌 → 預設嘲諷"), Taunt(Logic, TEXT("P2")).Result.bIsDefault);
			TestEqual(TEXT("P2 嘲諷 1 次"), Logic.GetAchievements().GetTauntCount(TEXT("P2")), 1);
		});

		It(TEXT("FAchievementTracker：每位玩家各自的解鎖與計數，門檻是具名常數，全部只在記憶體"), [this]()
		{
			FAchievementTracker Tracker;
			TestEqual(TEXT("A01 門檻 10"), FAchievementTracker::TauntThreshold, 10);
			TestEqual(TEXT("A02 門檻 20"), FAchievementTracker::TransformThreshold, 20);
			TestEqual(TEXT("A03 門檻 600 秒"), FAchievementTracker::ItemFormSecondsThreshold, 600.0);

			TestTrue(TEXT("P1 解鎖 A01"), Tracker.Unlock(TEXT("P1"), TEXT("A01")));
			TestTrue(TEXT("P2 也可以解鎖自己的 A01"), Tracker.Unlock(TEXT("P2"), TEXT("A01")));
			TestTrue(TEXT("P1 解鎖 A02"), Tracker.Unlock(TEXT("P1"), TEXT("A02")));
			TestTrue(TEXT("P1 的 A01"), Tracker.IsUnlocked(TEXT("P1"), TEXT("A01")));
			TestFalse(TEXT("P2 沒有 A02"), Tracker.IsUnlocked(TEXT("P2"), TEXT("A02")));
			TestFalse(TEXT("沒出現過的玩家沒有成就"), Tracker.IsUnlocked(TEXT("P9"), TEXT("A01")));
			const TArray<FString> P1 = Tracker.GetUnlocked(TEXT("P1"));
			TestEqual(TEXT("P1 依解鎖順序：A01、A02"), FString::Join(P1, TEXT(",")), FString(TEXT("A01,A02")));
			TestEqual(TEXT("沒出現過的玩家清單是空的"), Tracker.GetUnlocked(TEXT("P9")).Num(), 0);

			for (int32 I = 1; I < FAchievementTracker::TauntThreshold; ++I) { Tracker.AddTaunt(TEXT("P3")); }
			TestFalse(TEXT("9 次還沒到門檻"), Tracker.HasReachedTauntThreshold(TEXT("P3")));
			TestEqual(TEXT("AddTaunt 回傳新的次數"), Tracker.AddTaunt(TEXT("P3")), 10);
			TestTrue(TEXT("10 次到門檻"), Tracker.HasReachedTauntThreshold(TEXT("P3")));
			for (int32 I = 1; I < FAchievementTracker::TransformThreshold; ++I) { Tracker.AddTransform(TEXT("P3")); }
			TestFalse(TEXT("19 次還沒到門檻"), Tracker.HasReachedTransformThreshold(TEXT("P3")));
			TestEqual(TEXT("AddTransform 回傳新的次數"), Tracker.AddTransform(TEXT("P3")), 20);
			TestTrue(TEXT("20 次到門檻"), Tracker.HasReachedTransformThreshold(TEXT("P3")));

			Tracker.AddItemFormSeconds(TEXT("P3"), 600.0);
			TestFalse(TEXT("剛好 600 秒不算超過"), Tracker.HasExceededItemFormThreshold(TEXT("P3")));
			Tracker.AddItemFormSeconds(TEXT("P3"), 1.0 / 60.0);
			TestTrue(TEXT("多一格就超過"), Tracker.HasExceededItemFormThreshold(TEXT("P3")));
			Tracker.ResetItemFormSeconds(TEXT("P3"));
			TestEqual(TEXT("歸零"), Tracker.GetItemFormSeconds(TEXT("P3")), 0.0);

			Tracker.AddTouchedDoor(TEXT("P3"), TEXT("S01"));
			Tracker.AddTouchedDoor(TEXT("P3"), TEXT("S01"));
			TestEqual(TEXT("同一扇門只算一次"), Tracker.GetTouchedDoors(TEXT("P3")).Num(), 1);
			TestEqual(TEXT("別人沒碰過門"), Tracker.GetTouchedDoors(TEXT("P1")).Num(), 0);
		});
	});
}

#endif  // !UE_BUILD_SHIPPING
