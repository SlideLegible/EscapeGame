// 步驟 3 生命值 → VERIFY STEP 3
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/InteractionResolver.h"
#include "Logic/TeamState.h"

namespace Step3Spec
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
	FInteractionResult Interact(const FCtx& C, FTeamState& Team, const FString& Form, const FString& Target, TArray<FTeamEvent>* OutEvents = nullptr)
	{
		FInteractionQuery Q;
		Q.FormId = Form;
		Q.TargetId = Target;
		Q.State = Team.MakeSnapshot();
		const FInteractionResult R = C.Resolver.Resolve(Q);
		TArray<FTeamEvent> Events = Team.ApplyInteraction(R);
		if (OutEvents) { *OutEvents = MoveTemp(Events); }
		return R;
	}

	FInteractionResult HpOnly(int32 Delta)
	{
		FInteractionResult R;
		R.InteractionId = TEXT("X999");
		R.HpDelta = Delta;
		return R;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep3Spec, "EscapeGame.Step3",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
	TSharedPtr<Step3Spec::FCtx> Ctx;
END_DEFINE_SPEC(FEscapeGameStep3Spec)

void FEscapeGameStep3Spec::Define()
{
	using namespace Step3Spec;
	Ctx = MakeShared<FCtx>();
	TSharedRef<FCtx> C = Ctx.ToSharedRef();
	C->bLoaded = FInteractionResolver::Load(LoadText(TEXT("DT_Interactions.csv")), LoadText(TEXT("DT_Forms.csv")),
		LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), C->Resolver, C->Error);

	Describe(TEXT("VERIFY STEP 3"), [this, C]()
	{
		It(TEXT("開局 → 全隊生命值 5"), [this, C]()
		{
			FTeamState Team;
			TestEqual(TEXT("生命值"), Team.GetHp(), 5);
			TestEqual(TEXT("重啟次數"), Team.GetRestartCount(), 0);
			TestEqual(TEXT("已觸發為空"), Team.GetTriggeredIds().Num(), 0);
		});

		It(TEXT("第一次變成飲料（X004）→ 生命值 +1，可以超過 5（例：5 → 6）"), [this, C]()
		{
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *C->Error), C->bLoaded);
			if (!C->bLoaded) { return; }
			FTeamState Team;
			TestEqual(TEXT("開局"), Team.GetHp(), 5);
			TestEqual(TEXT("X002 鑽進飲料機"), Interact(*C, Team, TEXT("F01"), TEXT("S03")).InteractionId, FString(TEXT("X002")));
			TestEqual(TEXT("X002 不改生命值"), Team.GetHp(), 5);
			TestEqual(TEXT("變成飲料"), Interact(*C, Team, TEXT("F00"), TEXT("F02")).InteractionId, FString(TEXT("X004")));
			TestEqual(TEXT("5 → 6"), Team.GetHp(), 6);
			TestTrue(TEXT("X004 已觸發"), Team.GetTriggeredIds().Contains(TEXT("X004")));
		});

		It(TEXT("第二次以後變成飲料（X026）→「喝太多，肚子痛」，生命值 -1"), [this, C]()
		{
			TestTrue(TEXT("CSV 載入"), C->bLoaded);
			if (!C->bLoaded) { return; }
			FTeamState Team;
			Interact(*C, Team, TEXT("F01"), TEXT("S03"));
			Interact(*C, Team, TEXT("F00"), TEXT("F02"));
			TestEqual(TEXT("第一次後"), Team.GetHp(), 6);
			const FInteractionResult R = Interact(*C, Team, TEXT("F00"), TEXT("F02"));
			TestEqual(TEXT("第二次是 X026"), R.InteractionId, FString(TEXT("X026")));
			TestTrue(TEXT("文字"), R.Text.Contains(TEXT("喝太多，肚子痛")));
			TestEqual(TEXT("6 → 5"), Team.GetHp(), 5);
			TestEqual(TEXT("第三次仍是 X026"), Interact(*C, Team, TEXT("F00"), TEXT("F02")).InteractionId, FString(TEXT("X026")));
			TestEqual(TEXT("5 → 4"), Team.GetHp(), 4);
		});

		It(TEXT("每次變成期中考考卷（X027）→ 生命值 -1，變幾次扣幾次"), [this, C]()
		{
			TestTrue(TEXT("CSV 載入"), C->bLoaded);
			if (!C->bLoaded) { return; }
			FTeamState Team;
			Interact(*C, Team, TEXT("F00"), TEXT("S10"));  // X009 打翻花盆，-1
			TestEqual(TEXT("X009 扣 1"), Team.GetHp(), 4);
			Interact(*C, Team, TEXT("F04"), TEXT("S05"));  // X005 開辦公室門
			TestEqual(TEXT("X008 拆牛皮紙包"), Interact(*C, Team, TEXT("F00"), TEXT("S08")).InteractionId, FString(TEXT("X008")));
			TestEqual(TEXT("X008 不改生命值"), Team.GetHp(), 4);
			for (int32 Times = 1; Times <= 3; ++Times)
			{
				TestEqual(TEXT("變成考卷"), Interact(*C, Team, TEXT("F00"), TEXT("F06")).InteractionId, FString(TEXT("X027")));
				TestEqual(*FString::Printf(TEXT("第 %d 次後"), Times), Team.GetHp(), 4 - Times);
			}
			TestEqual(TEXT("重啟次數仍是 0"), Team.GetRestartCount(), 0);
		});

		It(TEXT("生命值從 1 再扣 1 → 變成 0，觸發 E05，所有還沒逃出的玩家都看到 E05，遊戲回到第一關、生命值回到 5"), [this, C]()
		{
			FTeamState Team;
			Team.AddPlayer(TEXT("P1"));
			Team.AddPlayer(TEXT("P2"));
			Team.AddPlayer(TEXT("P3"));
			TestTrue(TEXT("P2 逃出"), Team.MarkEscaped(TEXT("P2"), TEXT("E01")));
			TestTrue(TEXT("P2 已逃出"), Team.HasEscaped(TEXT("P2")));
			TestEqual(TEXT("P2 的結局"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
			TestEqual(TEXT("還沒逃出的玩家"), Team.GetRemainingPlayers(), TArray<FString>({ TEXT("P1"), TEXT("P3") }));

			FInteractionResult Seed;
			Seed.InteractionId = TEXT("X100");
			Seed.HpDelta = -4;
			TestEqual(TEXT("扣到 1 沒有事件"), Team.ApplyInteraction(Seed).Num(), 0);
			TestEqual(TEXT("生命值 1"), Team.GetHp(), 1);
			TestTrue(TEXT("X100 已觸發"), Team.GetTriggeredIds().Contains(TEXT("X100")));

			const TArray<FTeamEvent> Events = Team.ApplyInteraction(HpOnly(-1));
			TestEqual(TEXT("一個事件"), Events.Num(), 1);
			if (Events.Num() == 1)
			{
				TestEqual(TEXT("是 E05"), Events[0].EndingId, FString(TEXT("E05")));
				TestEqual(TEXT("受影響的正好是還沒逃出的"), Events[0].AffectedPlayers, TArray<FString>({ TEXT("P1"), TEXT("P3") }));
			}
			TestEqual(TEXT("生命值回到 5"), Team.GetHp(), 5);
			TestEqual(TEXT("已觸發集合是空的"), Team.GetTriggeredIds().Num(), 0);
			TestEqual(TEXT("重啟次數 +1"), Team.GetRestartCount(), 1);
			TestTrue(TEXT("已逃出的玩家保留"), Team.HasEscaped(TEXT("P2")));
			TestEqual(TEXT("已逃出玩家的結局保留"), Team.GetEscapeEnding(TEXT("P2")), FString(TEXT("E01")));
			TestEqual(TEXT("還沒逃出的名單不變"), Team.GetRemainingPlayers().Num(), 2);
		});

		It(TEXT("不可以 生命值變成負數"), [this, C]()
		{
			FTeamState Team;
			Team.AddPlayer(TEXT("P1"));
			Team.ApplyInteraction(HpOnly(-4));
			TestEqual(TEXT("生命值 1"), Team.GetHp(), 1);
			const TArray<FTeamEvent> Events = Team.ApplyInteraction(HpOnly(-2));
			TestEqual(TEXT("只觸發一次 E05"), Events.Num(), 1);
			TestEqual(TEXT("重啟次數 1"), Team.GetRestartCount(), 1);
			TestEqual(TEXT("重啟後回到 5，不是負數"), Team.GetHp(), 5);

			// 重啟前的瞬間：直接用大額扣血確認不會低於 0
			FTeamState Big;
			const TArray<FTeamEvent> BigEvents = Big.ApplyHpDelta(-100);
			TestEqual(TEXT("大額扣血只觸發一次"), BigEvents.Num(), 1);
			TestTrue(TEXT("生命值不為負"), Big.GetHp() >= 0);
			TestEqual(TEXT("重啟次數 1"), Big.GetRestartCount(), 1);
		});

		It(TEXT("不可以 E05 重啟後清掉已解鎖的成就"), [this, C]()
		{
			TestTrue(TEXT("CSV 載入"), C->bLoaded);
			if (!C->bLoaded) { return; }
			FTeamState Team;
			Interact(*C, Team, TEXT("F00"), TEXT("S10"));
			Interact(*C, Team, TEXT("F04"), TEXT("S05"));
			Interact(*C, Team, TEXT("F00"), TEXT("S08"));  // X008 解鎖 A04
			TestTrue(TEXT("A04 已解鎖"), Team.GetUnlockedAchievements().Contains(TEXT("A04")));
			TestEqual(TEXT("X009 扣 1 後是 4"), Team.GetHp(), 4);
			const TArray<FTeamEvent> Events = Team.ApplyInteraction(HpOnly(-4));
			TestEqual(TEXT("E05"), Events.Num(), 1);
			TestEqual(TEXT("重啟"), Team.GetRestartCount(), 1);
			TestEqual(TEXT("已觸發清空"), Team.GetTriggeredIds().Num(), 0);
			TestTrue(TEXT("A04 還在"), Team.GetUnlockedAchievements().Contains(TEXT("A04")));
		});
	});
}
