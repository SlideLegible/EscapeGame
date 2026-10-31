// 步驟 6 修正 特殊地點只能碰目前在裡面的物品 → VERIFY STEP 6
// 整個檔案只在非 Shipping 編譯（跟 Step6_Transform.spec.cpp 一樣）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

namespace Step6FixSpec
{
	const FVector PosS02(1000, 0, 0);   // 垃圾桶
	const FVector PosS03(2000, 0, 0);   // 飲料機
	const FVector PosS04(3000, 0, 0);   // 飲料機底下
	const FVector PosS01(4000, 0, 0);   // 學院大門
	const FVector PosElsewhere(7000, 0, 0);

	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	bool MakeLogic(int32 Seed, FEscapeGameLogic& Out, FString& Error)
	{
		if (!FEscapeGameLogic::Load(LoadText(TEXT("DT_Interactions.csv")), LoadText(TEXT("DT_Forms.csv")),
			LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), LoadText(TEXT("DT_Locations.csv")),
			Seed, Out, Error))
		{
			return false;
		}
		Out.RegisterObjectLocation(TEXT("S02"), PosS02);
		Out.RegisterObjectLocation(TEXT("S03"), PosS03);
		Out.RegisterObjectLocation(TEXT("S04"), PosS04);
		Out.RegisterObjectLocation(TEXT("S01"), PosS01);
		return true;
	}

	/** 被碰的物品與碰它的玩家，互動前後的狀態 */
	struct FSnapshot
	{
		FVector ItemLocation;
		FString ItemLocationId;
		FString ItemOccupant;
		FString PlayerForm;
		FString PlayerLocationId;
		FVector PlayerLocation;
		int32 Hp = 0;
		TSet<FString> Triggered;
	};

	FSnapshot Take(const FEscapeGameLogic& Logic, const FString& ItemId, const FString& PlayerId)
	{
		FSnapshot S;
		S.ItemLocation = Logic.GetItemLocation(ItemId);
		S.ItemLocationId = Logic.GetItemLocationId(ItemId);
		S.ItemOccupant = Logic.GetItemOccupant(ItemId);
		S.PlayerForm = Logic.GetPlayerForm(PlayerId);
		S.PlayerLocationId = Logic.GetPlayerLocationId(PlayerId);
		S.PlayerLocation = Logic.GetPlayerLocation(PlayerId);
		S.Hp = Logic.GetTeam().GetHp();
		S.Triggered = Logic.GetTeam().GetTriggeredIds();
		return S;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep6FixSpec, "EscapeGame.Step6Fix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)

	void ExpectRejectedUnchanged(FEscapeGameLogic& Logic, const FString& PlayerId, const FString& ItemId);
END_DEFINE_SPEC(FEscapeGameStep6FixSpec)

void FEscapeGameStep6FixSpec::ExpectRejectedUnchanged(FEscapeGameLogic& Logic, const FString& PlayerId, const FString& ItemId)
{
	using namespace Step6FixSpec;
	const FSnapshot Before = Take(Logic, ItemId, PlayerId);
	const FInteractOutcome Out = Logic.Interact(PlayerId, ItemId);
	const FSnapshot After = Take(Logic, ItemId, PlayerId);

	TestTrue(TEXT("Kind 是 Rejected"), Out.Kind == EInteractKind::Rejected);
	TestFalse(TEXT("沒有變身"), Out.bTransformed);
	TestEqual(TEXT("沒有進入地點"), Out.EnteredLocationId, FString());
	TestEqual(TEXT("沒有文字"), Out.Messages.Num(), 0);
	TestEqual(TEXT("物品位置不變"), After.ItemLocation, Before.ItemLocation);
	TestEqual(TEXT("物品的 LocationId 不變"), After.ItemLocationId, Before.ItemLocationId);
	TestEqual(TEXT("占用者不變"), After.ItemOccupant, Before.ItemOccupant);
	TestEqual(TEXT("玩家形態不變"), After.PlayerForm, Before.PlayerForm);
	TestEqual(TEXT("玩家地點不變"), After.PlayerLocationId, Before.PlayerLocationId);
	TestEqual(TEXT("玩家位置不變"), After.PlayerLocation, Before.PlayerLocation);
	TestEqual(TEXT("生命值不變"), After.Hp, Before.Hp);
	TestTrue(TEXT("已觸發集合不變"), After.Triggered.Num() == Before.Triggered.Num() && After.Triggered.Includes(Before.Triggered));
}

void FEscapeGameStep6FixSpec::Define()
{
	using namespace Step6FixSpec;

	Describe(TEXT("VERIFY STEP 6"), [this]()
	{
		It(TEXT("特殊地點裡只能碰目前在裡面的物品：飲料被帶出去後，再進 L01 碰不到它（Rejected、不移動、不變身）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			// P1：垃圾桶 → 飲料罐垃圾 → 進 L01 → 變成飲料（帶出 L01）→ 走到別處 → 變回人類，飲料留在外面
			Logic.Interact(TEXT("P1"), TEXT("S02"));
			TestTrue(TEXT("P1 變成飲料罐垃圾"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("P1 進入 L01"), Logic.Interact(TEXT("P1"), TEXT("S03")).EnteredLocationId, FString(TEXT("L01")));
			TestTrue(TEXT("P1 在 L01 裡變成飲料"), Logic.Interact(TEXT("P1"), TEXT("F02")).bTransformed);
			Logic.SetPlayerLocation(TEXT("P1"), PosElsewhere);
			TestTrue(TEXT("P1 變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestEqual(TEXT("飲料留在別處"), Logic.GetItemLocation(TEXT("F02")), PosElsewhere);
			TestEqual(TEXT("飲料不在 L01 裡"), Logic.GetItemLocationId(TEXT("F02")), FString());
			TestEqual(TEXT("飲料沒人占用"), Logic.GetItemOccupant(TEXT("F02")), FString());

			// P2：撿起放在入口的飲料罐垃圾、進 L01，碰在外面的飲料
			TestTrue(TEXT("P2 變成飲料罐垃圾"), Logic.Interact(TEXT("P2"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("P2 進入 L01"), Logic.Interact(TEXT("P2"), TEXT("S03")).EnteredLocationId, FString(TEXT("L01")));

			ExpectRejectedUnchanged(Logic, TEXT("P2"), TEXT("F02"));
			TestEqual(TEXT("飲料還在別處"), Logic.GetItemLocation(TEXT("F02")), PosElsewhere);
			TestEqual(TEXT("P2 還在 L01"), Logic.GetPlayerLocationId(TEXT("P2")), FString(TEXT("L01")));
		});

		It(TEXT("特殊地點裡只能碰目前在裡面的物品：學生證被帶到大門旁後，再進 L02 拿不到它"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			// P1：垃圾桶 → 飲料罐垃圾 → 進 L02 → 變成學生證（帶出）→ 走到大門逃出，學生證留在大門旁
			Logic.Interact(TEXT("P1"), TEXT("S02"));
			TestTrue(TEXT("P1 變成飲料罐垃圾"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("P1 進入 L02"), Logic.Interact(TEXT("P1"), TEXT("S04")).EnteredLocationId, FString(TEXT("L02")));
			TestTrue(TEXT("P1 在 L02 裡變成學生證"), Logic.Interact(TEXT("P1"), TEXT("F03")).bTransformed);
			Logic.SetPlayerLocation(TEXT("P1"), PosS01);
			TestEqual(TEXT("P1 逃出（E01）"), Logic.Interact(TEXT("P1"), TEXT("S01")).Result.EndingId, FString(TEXT("E01")));
			TestEqual(TEXT("學生證留在大門旁"), Logic.GetItemLocation(TEXT("F03")), PosS01);
			TestEqual(TEXT("學生證不在 L02 裡"), Logic.GetItemLocationId(TEXT("F03")), FString());
			TestEqual(TEXT("學生證沒人占用"), Logic.GetItemOccupant(TEXT("F03")), FString());

			// P2：撿起放在 L02 入口的飲料罐垃圾、進 L02，碰在大門旁的學生證
			TestTrue(TEXT("P2 變成飲料罐垃圾"), Logic.Interact(TEXT("P2"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("P2 進入 L02"), Logic.Interact(TEXT("P2"), TEXT("S04")).EnteredLocationId, FString(TEXT("L02")));

			ExpectRejectedUnchanged(Logic, TEXT("P2"), TEXT("F03"));
			TestEqual(TEXT("學生證還在大門旁"), Logic.GetItemLocation(TEXT("F03")), PosS01);
			TestEqual(TEXT("P2 還在 L02"), Logic.GetPlayerLocationId(TEXT("P2")), FString(TEXT("L02")));
		});

		It(TEXT("對照：飲料還在 L01 裡時，從 L01 裡碰得到它（X004 變身）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			Logic.Interact(TEXT("P1"), TEXT("S02"));
			Logic.Interact(TEXT("P1"), TEXT("F01"));
			TestEqual(TEXT("P1 進入 L01"), Logic.Interact(TEXT("P1"), TEXT("S03")).EnteredLocationId, FString(TEXT("L01")));
			TestEqual(TEXT("飲料在 L01 裡"), Logic.GetItemLocationId(TEXT("F02")), FString(TEXT("L01")));

			const FInteractOutcome Drink = Logic.Interact(TEXT("P1"), TEXT("F02"));
			TestTrue(TEXT("互動成立"), Drink.Kind == EInteractKind::Resolved);
			TestTrue(TEXT("變成飲料"), Drink.bTransformed && Drink.ToForm == TEXT("F02"));
			TestEqual(TEXT("是 X004"), Drink.Result.InteractionId, FString(TEXT("X004")));
			TestEqual(TEXT("玩家形態 F02"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F02")));
			TestEqual(TEXT("離開 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
		});
	});
}

#endif // !UE_BUILD_SHIPPING
