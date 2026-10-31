// 步驟 4 時間 → VERIFY STEP 4
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/InteractionResolver.h"
#include "Logic/TeamState.h"

namespace Step4Spec
{
	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	struct FCtx
	{
		FInteractionResolver Resolver;
		FString Error;
		bool bLoaded = false;
	};

	/** 走真的流程：用 TeamState 的快照組 query、Resolve、再 ApplyInteraction 回去 */
	FInteractionResult Interact(const FCtx& C, FTeamState& Team, const FString& PlayerId, const FString& Form, const FString& Target,
		TArray<FTeamEvent>* OutEvents = nullptr)
	{
		FInteractionQuery Q;
		Q.FormId = Form;
		Q.TargetId = Target;
		Q.State = Team.MakeSnapshot();
		const FInteractionResult R = C.Resolver.Resolve(Q);
		TArray<FTeamEvent> Events = Team.ApplyInteraction(R, PlayerId);
		if (OutEvents) { *OutEvents = MoveTemp(Events); }
		return R;
	}

	void AddThreePlayers(FTeamState& Team)
	{
		Team.AddPlayer(TEXT("P1"));
		Team.AddPlayer(TEXT("P2"));
		Team.AddPlayer(TEXT("P3"));
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep4Spec, "EscapeGame.Step4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
	TSharedPtr<Step4Spec::FCtx> Ctx;
END_DEFINE_SPEC(FEscapeGameStep4Spec)

void FEscapeGameStep4Spec::Define()
{
	using namespace Step4Spec;
	Ctx = MakeShared<FCtx>();
	TSharedRef<FCtx> C = Ctx.ToSharedRef();
	C->bLoaded = FInteractionResolver::Load(LoadText(TEXT("DT_Interactions.csv")), LoadText(TEXT("DT_Forms.csv")),
		LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), C->Resolver, C->Error);

	Describe(TEXT("VERIFY STEP 4"), [this, C]()
	{
		It(TEXT("開局 → 剩餘時間 30:00，每過 1 秒少 1 秒"), [this, C]()
		{
			FTeamState Team;
			TestEqual(TEXT("一局 1800 秒"), FTeamState::RoundSeconds, 1800);
			TestEqual(TEXT("開局剩 1800 秒"), Team.GetRemainingSeconds(), 1800);
			TestEqual(TEXT("顯示 30:00"), FTeamState::FormatClock(Team.GetRemainingSeconds()), FString(TEXT("30:00")));

			// 固定 dt 1/60，跑 60 格 = 1 秒 → 剛好 1799
			for (int32 Frame = 0; Frame < 60; ++Frame) { TestEqual(TEXT("沒有事件"), Team.Tick(1.0f / 60.0f).Num(), 0); }
			TestEqual(TEXT("1 秒後 1799"), Team.GetRemainingSeconds(), 1799);
			TestEqual(TEXT("顯示 29:59"), FTeamState::FormatClock(Team.GetRemainingSeconds()), FString(TEXT("29:59")));

			// 再 9 秒 → 1790，顯示 29:50
			for (int32 Frame = 0; Frame < 60 * 9; ++Frame) { Team.Tick(1.0f / 60.0f); }
			TestEqual(TEXT("10 秒後 1790"), Team.GetRemainingSeconds(), 1790);
			TestEqual(TEXT("顯示 29:50"), FTeamState::FormatClock(Team.GetRemainingSeconds()), FString(TEXT("29:50")));

			// 無條件進位：不到 1 秒仍顯示原本的秒數，湊滿 1 秒才少 1
			FTeamState Half;
			Half.Tick(0.5f);
			TestEqual(TEXT("過 0.5 秒仍是 1800"), Half.GetRemainingSeconds(), 1800);
			Half.Tick(0.5f);
			TestEqual(TEXT("過 1 秒是 1799"), Half.GetRemainingSeconds(), 1799);

			TestEqual(TEXT("59 → 00:59"), FTeamState::FormatClock(59), FString(TEXT("00:59")));
			TestEqual(TEXT("0 → 00:00"), FTeamState::FormatClock(0), FString(TEXT("00:00")));
			TestEqual(TEXT("1800 → 30:00"), FTeamState::FormatClock(1800), FString(TEXT("30:00")));
		});

		It(TEXT("時間跑到 0 → 觸發 E04「天亮」，套用到所有還沒逃出的玩家"), [this, C]()
		{
			FTeamState Team;
			AddThreePlayers(Team);
			for (int32 Second = 1; Second < FTeamState::RoundSeconds; ++Second)
			{
				TestEqual(*FString::Printf(TEXT("第 %d 秒沒有事件"), Second), Team.Tick(1.0f).Num(), 0);
			}
			TestEqual(TEXT("剩 1 秒"), Team.GetRemainingSeconds(), 1);

			const TArray<FTeamEvent> Events = Team.Tick(1.0f);
			TestEqual(TEXT("剛好一個事件"), Events.Num(), 1);
			if (Events.Num() == 1)
			{
				TestEqual(TEXT("是 E04"), Events[0].EndingId, FString(TEXT("E04")));
				TestEqual(TEXT("三位玩家都受影響"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P1"), TEXT("P2"), TEXT("P3") }));
			}
			TestEqual(TEXT("時鐘 0"), Team.GetRemainingSeconds(), 0);
			TestEqual(TEXT("顯示 00:00"), FTeamState::FormatClock(Team.GetRemainingSeconds()), FString(TEXT("00:00")));
			for (const TCHAR* Id : { TEXT("P1"), TEXT("P2"), TEXT("P3") })
			{
				TestTrue(TEXT("玩家已結束"), Team.HasEscaped(Id));
				TestEqual(TEXT("結局是 E04"), Team.GetEscapeEnding(Id), FString(TEXT("E04")));
			}
			TestEqual(TEXT("沒有還在局裡的玩家"), Team.GetRemainingPlayers().Num(), 0);

			// 之後時鐘停在 0，再 Tick 不會再有 E04
			TestEqual(TEXT("再 Tick 沒有事件"), Team.Tick(1.0f).Num(), 0);
			TestEqual(TEXT("時鐘停在 0"), Team.GetRemainingSeconds(), 0);
		});

		It(TEXT("時間跑到 0 → 固定 dt 1/60 跑完整局，只有一個 E04"), [this, C]()
		{
			FTeamState Team;
			Team.AddPlayer(TEXT("P1"));
			int32 EventCount = 0;
			int32 DawnFrame = 0;
			const int32 Frames = FTeamState::RoundSeconds * 60 + 600;  // 多跑 10 秒
			for (int32 Frame = 1; Frame <= Frames; ++Frame)
			{
				const TArray<FTeamEvent> Events = Team.Tick(1.0f / 60.0f);
				for (const FTeamEvent& E : Events)
				{
					TestEqual(TEXT("是 E04"), E.EndingId, FString(TEXT("E04")));
					++EventCount;
					DawnFrame = Frame;
				}
			}
			TestEqual(TEXT("只有一個 E04"), EventCount, 1);
			TestTrue(TEXT("在第 1800 秒（±1 格）發生"), FMath::Abs(DawnFrame - FTeamState::RoundSeconds * 60) <= 1);
			TestEqual(TEXT("時鐘停在 0"), Team.GetRemainingSeconds(), 0);
		});

		It(TEXT("變成時鐘（F09）與自己互動（X021）→ 立刻觸發 E04，並解鎖 A06「時間管理大師」"), [this, C]()
		{
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *C->Error), C->bLoaded);
			if (!C->bLoaded) { return; }
			FTeamState Team;
			AddThreePlayers(Team);
			TArray<FTeamEvent> Events;
			const FInteractionResult R = Interact(*C, Team, TEXT("P1"), TEXT("F09"), TEXT("SELF"), &Events);
			TestEqual(TEXT("是 X021"), R.InteractionId, FString(TEXT("X021")));
			TestEqual(TEXT("結局 E04"), R.EndingId, FString(TEXT("E04")));
			TestEqual(TEXT("剛好一個事件"), Events.Num(), 1);
			if (Events.Num() == 1)
			{
				TestEqual(TEXT("是 E04"), Events[0].EndingId, FString(TEXT("E04")));
				TestEqual(TEXT("三位玩家都受影響"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P1"), TEXT("P2"), TEXT("P3") }));
			}
			TestTrue(TEXT("A06 已解鎖"), Team.GetUnlockedAchievements().Contains(TEXT("A06")));
			TestTrue(TEXT("X021 已觸發"), Team.GetTriggeredIds().Contains(TEXT("X021")));
			TestEqual(TEXT("沒有經過任何 Tick，剩餘時間沒變"), Team.GetRemainingSeconds(), 1800);
			for (const TCHAR* Id : { TEXT("P1"), TEXT("P2"), TEXT("P3") })
			{
				TestTrue(TEXT("玩家已結束"), Team.HasEscaped(Id));
				TestEqual(TEXT("結局是 E04"), Team.GetEscapeEnding(Id), FString(TEXT("E04")));
			}
			TestEqual(TEXT("之後 Tick 不會再有 E04"), Team.Tick(1.0f).Num(), 0);
		});

		It(TEXT("不可以 已經逃出的玩家再收到 E04"), [this, C]()
		{
			// 路徑一：時間到
			{
				FTeamState Team;
				AddThreePlayers(Team);
				TestTrue(TEXT("P2 逃出"), Team.MarkEscaped(TEXT("P2"), TEXT("E01")));
				TArray<FTeamEvent> All;
				for (int32 Second = 0; Second < FTeamState::RoundSeconds; ++Second) { All.Append(Team.Tick(1.0f)); }
				TestEqual(TEXT("時間到只有一個事件"), All.Num(), 1);
				if (All.Num() == 1)
				{
					TestEqual(TEXT("是 E04"), All[0].EndingId, FString(TEXT("E04")));
					TestEqual(TEXT("受影響的只有 P1、P3"), All[0].AffectedPlayers, TArray<FString>({ TEXT("P1"), TEXT("P3") }));
				}
				TestEqual(TEXT("P2 的結局仍是 E01"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
				TestEqual(TEXT("P1 的結局是 E04"), Team.GetEscapeEnding(TEXT("P1")), FString(TEXT("E04")));
				TestEqual(TEXT("P3 的結局是 E04"), Team.GetEscapeEnding(TEXT("P3")), FString(TEXT("E04")));
			}

			// 路徑二：時鐘自己互動（X021）
			TestTrue(TEXT("CSV 載入"), C->bLoaded);
			if (!C->bLoaded) { return; }
			{
				FTeamState Team;
				AddThreePlayers(Team);
				TestTrue(TEXT("P2 逃出"), Team.MarkEscaped(TEXT("P2"), TEXT("E01")));
				TArray<FTeamEvent> Events;
				Interact(*C, Team, TEXT("P1"), TEXT("F09"), TEXT("SELF"), &Events);
				TestEqual(TEXT("一個事件"), Events.Num(), 1);
				if (Events.Num() == 1)
				{
					TestEqual(TEXT("是 E04"), Events[0].EndingId, FString(TEXT("E04")));
					TestEqual(TEXT("受影響的只有 P1、P3"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P1"), TEXT("P3") }));
				}
				TestEqual(TEXT("P2 的結局仍是 E01"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
				TestEqual(TEXT("P3 的結局是 E04"), Team.GetEscapeEnding(TEXT("P3")), FString(TEXT("E04")));
			}
		});

		It(TEXT("所有玩家都已結束（至少有一位玩家）→ Tick 不倒數；沒有玩家時照常倒數（PLANS 詮釋）"), [this, C]()
		{
			FTeamState Team;
			Team.AddPlayer(TEXT("P1"));
			Team.AddPlayer(TEXT("P2"));
			Team.Tick(100.0f);
			TestEqual(TEXT("過 100 秒"), Team.GetRemainingSeconds(), 1700);
			TestTrue(TEXT("P1 逃出"), Team.MarkEscaped(TEXT("P1"), TEXT("E01")));
			Team.Tick(100.0f);
			TestEqual(TEXT("還有 P2 在局裡，繼續倒數"), Team.GetRemainingSeconds(), 1600);
			TestTrue(TEXT("P2 逃出"), Team.MarkEscaped(TEXT("P2"), TEXT("E02")));
			TestEqual(TEXT("沒有事件"), Team.Tick(100.0f).Num(), 0);
			TestEqual(TEXT("全員結束後不倒數"), Team.GetRemainingSeconds(), 1600);

			FTeamState Empty;
			Empty.Tick(10.0f);
			TestEqual(TEXT("沒有玩家時照常倒數"), Empty.GetRemainingSeconds(), 1790);
		});

		It(TEXT("E05 重啟後剩餘時間回到 30:00（PLANS 詮釋）"), [this, C]()
		{
			FTeamState Team;
			Team.AddPlayer(TEXT("P1"));
			Team.Tick(100.0f);
			TestEqual(TEXT("過 100 秒"), Team.GetRemainingSeconds(), 1700);
			const TArray<FTeamEvent> Events = Team.ApplyHpDelta(-5);
			TestEqual(TEXT("E05"), Events.Num(), 1);
			TestEqual(TEXT("重啟次數"), Team.GetRestartCount(), 1);
			TestEqual(TEXT("時間回到 1800"), Team.GetRemainingSeconds(), 1800);
			TestEqual(TEXT("顯示 30:00"), FTeamState::FormatClock(Team.GetRemainingSeconds()), FString(TEXT("30:00")));
			Team.Tick(1.0f);
			TestEqual(TEXT("重啟後繼續倒數"), Team.GetRemainingSeconds(), 1799);
		});
	});
}
