// 步驟 6 變身 → VERIFY STEP 6
// 整個檔案只在非 Shipping 編譯：有幾條要用 GetPasswordForDevOnly()（Shipping 沒有任何途徑拿到密碼）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

namespace Step6Spec
{
	// 場景裡各物件的位置（測試自己決定，彼此不同才看得出誰搬到哪）
	const FVector PosS02(1000, 0, 0);   // 垃圾桶
	const FVector PosS03(2000, 0, 0);   // 飲料機
	const FVector PosS04(3000, 0, 0);   // 飲料機底下
	const FVector PosS01(4000, 0, 0);   // 學院大門
	const FVector PosS10(5000, 0, 0);   // 花盆
	const FVector PosF08(8000, 0, 0);
	const FVector PosF09(9000, 0, 0);
	const FVector PosF10(10000, 0, 0);
	const FVector PosF11(11000, 0, 0);
	const FVector PosF12(300, 0, 0);    // 椅子

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
		Out.RegisterObjectLocation(TEXT("S10"), PosS10);
		Out.RegisterObjectLocation(TEXT("F08"), PosF08);
		Out.RegisterObjectLocation(TEXT("F09"), PosF09);
		Out.RegisterObjectLocation(TEXT("F10"), PosF10);
		Out.RegisterObjectLocation(TEXT("F11"), PosF11);
		Out.RegisterObjectLocation(TEXT("F12"), PosF12);
		return true;
	}

	/** 一開始就在場景裡的物品 */
	const TArray<FString>& StartItems()
	{
		static const TArray<FString> Items = { TEXT("F08"), TEXT("F09"), TEXT("F10"), TEXT("F11"), TEXT("F12") };
		return Items;
	}

	/** 人類 Player 走完 X001、碰飲料罐垃圾、進 L02、變成學生證（F03）。回傳 false 表示中途失敗。 */
	bool BecomeStudentCard(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.Interact(Player, TEXT("S02")).Kind != EInteractKind::Resolved) { return false; }
		if (!Logic.Interact(Player, TEXT("F01")).bTransformed) { return false; }
		if (Logic.Interact(Player, TEXT("S04")).EnteredLocationId != TEXT("L02")) { return false; }
		return Logic.Interact(Player, TEXT("F03")).bTransformed && Logic.GetPlayerForm(Player) == TEXT("F03");
	}

	/** 取出 "停在了 N 點" 的 N；找不到回傳 -1 */
	int32 ParseDice(const FString& Text)
	{
		const FString Marker = TEXT("停在了 ");
		const int32 At = Text.Find(Marker);
		if (At == INDEX_NONE) { return -1; }
		const FString Rest = Text.Mid(At + Marker.Len());
		return Rest.Len() > 0 && FChar::IsDigit(Rest[0]) ? FCString::Atoi(*Rest) : -1;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep6Spec, "EscapeGame.Step6",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep6Spec)

void FEscapeGameStep6Spec::Define()
{
	using namespace Step6Spec;

	Describe(TEXT("VERIFY STEP 6"), [this]()
	{
		It(TEXT("人類與可變身的物品互動 → 玩家變成該物品；人類身體留在原地"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(100, 0, 0));

			TestEqual(TEXT("一開始是人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestTrue(TEXT("椅子一開始就在"), Logic.IsItemPresent(TEXT("F12")));
			TestEqual(TEXT("椅子沒人占用"), Logic.GetItemOccupant(TEXT("F12")), FString());

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("F12"));
			TestTrue(TEXT("互動成立"), Out.Kind == EInteractKind::Resolved);
			TestTrue(TEXT("有變身"), Out.bTransformed);
			TestEqual(TEXT("從人類"), Out.FromForm, FString(TEXT("F00")));
			TestEqual(TEXT("變成椅子"), Out.ToForm, FString(TEXT("F12")));
			TestFalse(TEXT("碰可變身物品的變身不是預設嘲諷"), Out.Result.bIsDefault);
			TestEqual(TEXT("沒有嘲諷文字"), Out.Messages.Num(), 0);
			TestEqual(TEXT("形態 F12"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F12")));
			TestEqual(TEXT("人類身體留在原地"), Logic.GetBodyLocation(TEXT("P1")), FVector(100, 0, 0));
			TestEqual(TEXT("玩家位置是椅子的位置"), Logic.GetPlayerLocation(TEXT("P1")), PosF12);
			TestEqual(TEXT("椅子被 P1 占用"), Logic.GetItemOccupant(TEXT("F12")), FString(TEXT("P1")));
			TestEqual(TEXT("不在特殊地點"), Logic.GetPlayerLocationId(TEXT("P1")), FString());

			// 之後移動玩家：身體不動
			Logic.SetPlayerLocation(TEXT("P1"), FVector(500, 0, 0));
			TestEqual(TEXT("身體還在原地"), Logic.GetBodyLocation(TEXT("P1")), FVector(100, 0, 0));
			TestEqual(TEXT("玩家位置更新"), Logic.GetPlayerLocation(TEXT("P1")), FVector(500, 0, 0));
			TestEqual(TEXT("玩家就是椅子：椅子跟著動"), Logic.GetItemLocation(TEXT("F12")), FVector(500, 0, 0));

			// 一開始就在的每個物品都能變身
			for (const FString& Item : StartItems())
			{
				FEscapeGameLogic L;
				MakeLogic(1, L, Error);
				L.AddPlayer(TEXT("P1"), FVector(7, 8, 9));
				const FInteractOutcome O = L.Interact(TEXT("P1"), Item);
				TestTrue(*FString::Printf(TEXT("%s 可以變身"), *Item), O.bTransformed && O.ToForm == Item);
				TestEqual(*FString::Printf(TEXT("%s 的身體在原地"), *Item), L.GetBodyLocation(TEXT("P1")), FVector(7, 8, 9));
			}
		});

		It(TEXT("物品形態選「變回人類」→ 玩家回到人類身體的位置，變回人類"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(100, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 100, 0));

			TestFalse(TEXT("已經是人類 → false"), Logic.ReturnToHuman(TEXT("P1")));
			TestTrue(TEXT("變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			Logic.SetPlayerLocation(TEXT("P1"), FVector(700, 0, 0));

			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestEqual(TEXT("形態 F00"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestEqual(TEXT("回到人類身體的位置"), Logic.GetPlayerLocation(TEXT("P1")), FVector(100, 0, 0));
			TestEqual(TEXT("身體位置不變"), Logic.GetBodyLocation(TEXT("P1")), FVector(100, 0, 0));
			TestEqual(TEXT("椅子放開了"), Logic.GetItemOccupant(TEXT("F12")), FString());
			TestEqual(TEXT("椅子留在放開時的位置"), Logic.GetItemLocation(TEXT("F12")), FVector(700, 0, 0));
			TestTrue(TEXT("椅子還在場景裡"), Logic.IsItemPresent(TEXT("F12")));
			TestFalse(TEXT("再變回人類 → false"), Logic.ReturnToHuman(TEXT("P1")));
			TestFalse(TEXT("不存在的玩家 → false"), Logic.ReturnToHuman(TEXT("P9")));

			// 放開之後別人可以變成椅子
			TestTrue(TEXT("P2 可以變成椅子"), Logic.Interact(TEXT("P2"), TEXT("F12")).bTransformed);
			TestEqual(TEXT("椅子是 P2 的"), Logic.GetItemOccupant(TEXT("F12")), FString(TEXT("P2")));
		});

		It(TEXT("從飲料罐垃圾直接變成學生證 → 飲料罐垃圾留在原地、維持當時的狀態"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			TestFalse(TEXT("垃圾桶還沒翻過：沒有飲料罐垃圾"), Logic.IsItemPresent(TEXT("F01")));
			TestTrue(TEXT("X001 翻垃圾桶"), Logic.Interact(TEXT("P1"), TEXT("S02")).Result.InteractionId == TEXT("X001"));
			TestTrue(TEXT("飲料罐垃圾出現"), Logic.IsItemPresent(TEXT("F01")));
			TestEqual(TEXT("飲料罐垃圾出現在垃圾桶的位置"), Logic.GetItemLocation(TEXT("F01")), PosS02);
			TestTrue(TEXT("變成飲料罐垃圾"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);

			// 路線一：在飲料機底下（L02）直接變成學生證
			Logic.SetPlayerLocation(TEXT("P1"), FVector(2950, 0, 0));
			const FInteractOutcome Enter = Logic.Interact(TEXT("P1"), TEXT("S04"));
			TestEqual(TEXT("進入 L02"), Enter.EnteredLocationId, FString(TEXT("L02")));
			const FInteractOutcome Swap = Logic.Interact(TEXT("P1"), TEXT("F03"));
			TestTrue(TEXT("直接變成學生證"), Swap.bTransformed);
			TestEqual(TEXT("從飲料罐垃圾"), Swap.FromForm, FString(TEXT("F01")));
			TestEqual(TEXT("變成學生證"), Swap.ToForm, FString(TEXT("F03")));
			TestEqual(TEXT("玩家形態 F03"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F03")));
			TestTrue(TEXT("飲料罐垃圾還在場景中"), Logic.IsItemPresent(TEXT("F01")));
			TestEqual(TEXT("飲料罐垃圾放開了"), Logic.GetItemOccupant(TEXT("F01")), FString());
			TestEqual(TEXT("飲料罐垃圾留在飲料機底下的入口"), Logic.GetItemLocation(TEXT("F01")), PosS04);
			TestEqual(TEXT("飲料罐垃圾在地點外"), Logic.GetItemLocationId(TEXT("F01")), FString());
			TestEqual(TEXT("學生證被 P1 占用"), Logic.GetItemOccupant(TEXT("F03")), FString(TEXT("P1")));

			// 路線二：在外面直接從飲料罐垃圾變成椅子，飲料罐垃圾留在原地
			FEscapeGameLogic Logic2;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Logic2, Error));
			Logic2.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic2.Interact(TEXT("P1"), TEXT("S02"));
			Logic2.Interact(TEXT("P1"), TEXT("F01"));
			Logic2.SetPlayerLocation(TEXT("P1"), FVector(1500, 20, 0));
			TestTrue(TEXT("直接變成椅子"), Logic2.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			TestEqual(TEXT("飲料罐垃圾留在原地"), Logic2.GetItemLocation(TEXT("F01")), FVector(1500, 20, 0));
			TestEqual(TEXT("飲料罐垃圾放開了"), Logic2.GetItemOccupant(TEXT("F01")), FString());
			TestEqual(TEXT("人類身體仍在出生點"), Logic2.GetBodyLocation(TEXT("P1")), FVector(0, 0, 0));
		});

		It(TEXT("變成飲料罐垃圾碰飲料機（X002）→ 進入飲料機內部（L01），看到飲料；碰飲料機底下（X003）→ 進入 L02，看到學生證"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			TestTrue(TEXT("X001"), Logic.Interact(TEXT("P1"), TEXT("S02")).Result.InteractionId == TEXT("X001"));
			TestTrue(TEXT("變成飲料罐垃圾"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);
			TestFalse(TEXT("還沒進去：看不到飲料"), Logic.IsItemPresent(TEXT("F02")));
			TestFalse(TEXT("還沒進去：看不到學生證"), Logic.IsItemPresent(TEXT("F03")));

			// L01
			const FInteractOutcome In1 = Logic.Interact(TEXT("P1"), TEXT("S03"));
			TestTrue(TEXT("互動成立"), In1.Kind == EInteractKind::Resolved);
			TestEqual(TEXT("是 X002"), In1.Result.InteractionId, FString(TEXT("X002")));
			TestFalse(TEXT("不是預設嘲諷"), In1.Result.bIsDefault);
			TestEqual(TEXT("進入 L01"), In1.EnteredLocationId, FString(TEXT("L01")));
			TestFalse(TEXT("進入地點不是變身"), In1.bTransformed);
			TestEqual(TEXT("玩家在 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L01")));
			TestEqual(TEXT("形態還是飲料罐垃圾"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F01")));
			TestTrue(TEXT("看到飲料"), Logic.IsItemPresent(TEXT("F02")));
			TestEqual(TEXT("飲料在 L01 裡"), Logic.GetItemLocationId(TEXT("F02")), FString(TEXT("L01")));
			TestFalse(TEXT("L01 裡沒有學生證"), Logic.IsItemPresent(TEXT("F03")));

			// 回到人類、再變成飲料罐垃圾，碰飲料機底下（L02）
			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestEqual(TEXT("離開 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
			TestTrue(TEXT("再變成飲料罐垃圾"), Logic.Interact(TEXT("P1"), TEXT("F01")).ToForm == TEXT("F01"));
			const FInteractOutcome In2 = Logic.Interact(TEXT("P1"), TEXT("S04"));
			TestEqual(TEXT("是 X003"), In2.Result.InteractionId, FString(TEXT("X003")));
			TestEqual(TEXT("進入 L02"), In2.EnteredLocationId, FString(TEXT("L02")));
			TestEqual(TEXT("玩家在 L02"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L02")));
			TestTrue(TEXT("看到學生證"), Logic.IsItemPresent(TEXT("F03")));
			TestEqual(TEXT("學生證在 L02 裡"), Logic.GetItemLocationId(TEXT("F03")), FString(TEXT("L02")));
		});

		It(TEXT("監視器（X023）→ 變成監視器後看得到整張地圖"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			TestFalse(TEXT("人類看不到整張地圖"), Logic.CanSeeWholeMap(TEXT("P1")));
			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("F10"));
			TestEqual(TEXT("是 X023"), Out.Result.InteractionId, FString(TEXT("X023")));
			TestTrue(TEXT("變成監視器"), Out.bTransformed && Out.ToForm == TEXT("F10"));
			TestEqual(TEXT("形態 F10"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F10")));
			TestTrue(TEXT("看得到整張地圖"), Logic.CanSeeWholeMap(TEXT("P1")));
			TestFalse(TEXT("別的玩家（人類）看不到"), Logic.CanSeeWholeMap(TEXT("P2")));
			TestFalse(TEXT("不存在的玩家看不到"), Logic.CanSeeWholeMap(TEXT("P9")));

			TestTrue(TEXT("P2 變成椅子"), Logic.Interact(TEXT("P2"), TEXT("F12")).bTransformed);
			TestFalse(TEXT("椅子看不到整張地圖"), Logic.CanSeeWholeMap(TEXT("P2")));

			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestFalse(TEXT("變回人類後看不到"), Logic.CanSeeWholeMap(TEXT("P1")));
		});

		It(TEXT("不可以 兩位玩家同時變成同一個物品：第二位玩家互動時，該物品的回應和「別人變成的物品」一樣不能互動"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.AddPlayer(TEXT("P3"), FVector(0, 80, 0));
			TestTrue(TEXT("P3 先變成鑰匙圈"), Logic.Interact(TEXT("P3"), TEXT("F11")).bTransformed);

			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			const int32 TriggeredBefore = Logic.GetTeam().GetTriggeredIds().Num();
			const int32 HpBefore = Logic.GetTeam().GetHp();

			// P2（人類）碰椅子
			const FInteractOutcome Blocked = Logic.Interact(TEXT("P2"), TEXT("F12"));
			TestTrue(TEXT("第二位玩家 → Blocked"), Blocked.Kind == EInteractKind::Blocked);
			TestEqual(TEXT("沒有文字"), Blocked.Messages.Num(), 0);
			TestEqual(TEXT("結果是空的"), Blocked.Result.InteractionId, FString());
			TestEqual(TEXT("結果沒有文字"), Blocked.Result.Text, FString());
			TestFalse(TEXT("不是預設嘲諷"), Blocked.Result.bIsDefault);
			TestFalse(TEXT("沒有變身"), Blocked.bTransformed);
			TestEqual(TEXT("沒有事件"), Blocked.Events.Num(), 0);
			TestEqual(TEXT("P2 還是人類"), Logic.GetPlayerForm(TEXT("P2")), FString(TEXT("F00")));
			TestEqual(TEXT("椅子仍是 P1 的"), Logic.GetItemOccupant(TEXT("F12")), FString(TEXT("P1")));
			TestEqual(TEXT("P1 還是椅子"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F12")));
			TestEqual(TEXT("不算任何已觸發"), Logic.GetTeam().GetTriggeredIds().Num(), TriggeredBefore);
			TestEqual(TEXT("生命值沒變"), Logic.GetTeam().GetHp(), HpBefore);

			// 「別人變成的物品」都一樣：換成別的物品、換成別的形態去碰，回應都是同一個 Blocked
			for (const FString& Form : { FString(TEXT("F08")), FString(TEXT("F09")), FString(TEXT("F10")) })
			{
				FEscapeGameLogic L;
				MakeLogic(1, L, Error);
				L.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
				L.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
				L.Interact(TEXT("P1"), Form);
				const FInteractOutcome O = L.Interact(TEXT("P2"), Form);
				TestTrue(*FString::Printf(TEXT("%s：第二位玩家 → Blocked"), *Form), O.Kind == EInteractKind::Blocked);
				TestTrue(*FString::Printf(TEXT("%s：沒有文字也沒有結果"), *Form),
					O.Messages.Num() == 0 && O.Result.Text.IsEmpty() && O.Result.InteractionId.IsEmpty() && O.Events.Num() == 0);
				TestEqual(*FString::Printf(TEXT("%s：仍是 P1 的"), *Form), L.GetItemOccupant(Form), FString(TEXT("P1")));
			}

			// P3（鑰匙圈形態，不是人類）碰椅子，也是同一個 Blocked
			const FInteractOutcome FromKeys = Logic.Interact(TEXT("P3"), TEXT("F12"));
			TestTrue(TEXT("非人類形態碰別人的椅子 → Blocked"), FromKeys.Kind == EInteractKind::Blocked);
			TestTrue(TEXT("一樣沒有文字也沒有結果"),
				FromKeys.Messages.Num() == 0 && FromKeys.Result.Text.IsEmpty() && FromKeys.Result.InteractionId.IsEmpty());
			TestEqual(TEXT("P3 仍是鑰匙圈"), Logic.GetPlayerForm(TEXT("P3")), FString(TEXT("F11")));

			// 放開後才能被別人變
			TestTrue(TEXT("P1 變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestTrue(TEXT("P2 現在可以變成椅子"), Logic.Interact(TEXT("P2"), TEXT("F12")).bTransformed);
		});

		It(TEXT("不可以 飲料罐垃圾以外的形態（包含人類）進入特殊地點（例：學生證碰飲料機 → 預設回應，不進入 L01）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestTrue(TEXT("P1 走完路線變成學生證"), BecomeStudentCard(Logic, TEXT("P1")));
			TestEqual(TEXT("學生證不在地點裡（變身時被帶出來）"), Logic.GetPlayerLocationId(TEXT("P1")), FString());

			for (const FString& Entrance : { FString(TEXT("S03")), FString(TEXT("S04")) })
			{
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), Entrance);
				TestTrue(*FString::Printf(TEXT("學生證碰 %s → Resolved"), *Entrance), Out.Kind == EInteractKind::Resolved);
				TestTrue(*FString::Printf(TEXT("學生證碰 %s → 預設回應"), *Entrance), Out.Result.bIsDefault);
				TestEqual(*FString::Printf(TEXT("學生證碰 %s → 有一則嘲諷文字"), *Entrance), Out.Messages.Num(), 1);
				TestEqual(*FString::Printf(TEXT("學生證碰 %s → 沒有進入地點"), *Entrance), Out.EnteredLocationId, FString());
				TestFalse(*FString::Printf(TEXT("學生證碰 %s → 沒有變身"), *Entrance), Out.bTransformed);
				TestEqual(*FString::Printf(TEXT("學生證碰 %s → 玩家不在地點"), *Entrance), Logic.GetPlayerLocationId(TEXT("P1")), FString());
				TestEqual(TEXT("形態不變"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F03")));
			}

			// 人類、其他一開始就在的物品形態都一樣
			TArray<FString> Forms = { FString(TEXT("F00")) };
			Forms.Append(StartItems());
			for (const FString& Form : Forms)
			{
				for (const FString& Entrance : { FString(TEXT("S03")), FString(TEXT("S04")) })
				{
					FEscapeGameLogic L;
					MakeLogic(1, L, Error);
					L.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
					if (Form != TEXT("F00")) { L.Interact(TEXT("P1"), Form); }
					const FInteractOutcome Out = L.Interact(TEXT("P1"), Entrance);
					TestTrue(*FString::Printf(TEXT("%s 碰 %s → 預設回應"), *Form, *Entrance), Out.Kind == EInteractKind::Resolved && Out.Result.bIsDefault);
					TestEqual(*FString::Printf(TEXT("%s 碰 %s → 不進入"), *Form, *Entrance), Out.EnteredLocationId, FString());
					TestEqual(*FString::Printf(TEXT("%s 碰 %s → 玩家不在地點"), *Form, *Entrance), L.GetPlayerLocationId(TEXT("P1")), FString());
					TestFalse(*FString::Printf(TEXT("%s 碰 %s → 沒有東西出現"), *Form, *Entrance), L.IsItemPresent(TEXT("F02")) || L.IsItemPresent(TEXT("F03")));
				}
			}
		});
	});

	Describe(TEXT("PLANS 詮釋"), [this]()
	{
		It(TEXT("L01／L02 完整路線：垃圾桶 → 飲料罐垃圾 → 飲料機 → 變回人類 → 再變成飲料罐垃圾 → 飲料機底下"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));

			TestTrue(TEXT("人類碰垃圾桶（X001）"), Logic.Interact(TEXT("P1"), TEXT("S02")).Result.InteractionId == TEXT("X001"));
			TestTrue(TEXT("碰飲料罐垃圾 → 變身"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("碰飲料機（X002）→ 進入 L01"), Logic.Interact(TEXT("P1"), TEXT("S03")).EnteredLocationId, FString(TEXT("L01")));
			TestTrue(TEXT("飲料在 L01"), Logic.IsItemPresent(TEXT("F02")) && Logic.GetItemLocationId(TEXT("F02")) == TEXT("L01"));

			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestEqual(TEXT("回到身體位置"), Logic.GetPlayerLocation(TEXT("P1")), FVector(0, 0, 0));
			TestEqual(TEXT("玩家不在 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
			TestEqual(TEXT("飲料罐垃圾放在飲料機入口"), Logic.GetItemLocation(TEXT("F01")), PosS03);
			TestEqual(TEXT("飲料罐垃圾沒人占用"), Logic.GetItemOccupant(TEXT("F01")), FString());
			TestEqual(TEXT("飲料罐垃圾在地點外"), Logic.GetItemLocationId(TEXT("F01")), FString());

			TestTrue(TEXT("再碰飲料罐垃圾 → 變身"), Logic.Interact(TEXT("P1"), TEXT("F01")).bTransformed);
			TestEqual(TEXT("玩家位置是飲料罐垃圾的位置"), Logic.GetPlayerLocation(TEXT("P1")), PosS03);
			const FInteractOutcome In2 = Logic.Interact(TEXT("P1"), TEXT("S04"));
			TestEqual(TEXT("碰飲料機底下（X003）→ 進入 L02"), In2.EnteredLocationId, FString(TEXT("L02")));
			TestTrue(TEXT("學生證在 L02"), Logic.IsItemPresent(TEXT("F03")) && Logic.GetItemLocationId(TEXT("F03")) == TEXT("L02"));
			TestEqual(TEXT("學生證出現在飲料機底下的位置"), Logic.GetItemLocation(TEXT("F03")), PosS04);
		});

		It(TEXT("在特殊地點變身會帶著新物品離開，舊物品放在入口（E01 路線走得通）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.Interact(TEXT("P1"), TEXT("S02"));
			Logic.Interact(TEXT("P1"), TEXT("F01"));
			Logic.Interact(TEXT("P1"), TEXT("S03"));
			TestEqual(TEXT("P1 在 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L01")));
			const int32 HpBefore = Logic.GetTeam().GetHp();

			// 在 L01 裡碰飲料 → 變成飲料並離開 L01
			const FInteractOutcome Drink = Logic.Interact(TEXT("P1"), TEXT("F02"));
			TestTrue(TEXT("變成飲料"), Drink.bTransformed && Drink.ToForm == TEXT("F02") && Drink.FromForm == TEXT("F01"));
			TestEqual(TEXT("是 X004"), Drink.Result.InteractionId, FString(TEXT("X004")));
			TestEqual(TEXT("第一次變成飲料 +1 血"), Logic.GetTeam().GetHp(), HpBefore + 1);
			TestEqual(TEXT("玩家離開 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
			TestEqual(TEXT("飲料被帶出來，放在飲料機入口"), Logic.GetItemLocation(TEXT("F02")), PosS03);
			TestEqual(TEXT("飲料不在地點裡"), Logic.GetItemLocationId(TEXT("F02")), FString());
			TestEqual(TEXT("飲料被 P1 占用"), Logic.GetItemOccupant(TEXT("F02")), FString(TEXT("P1")));
			TestEqual(TEXT("玩家位置在入口"), Logic.GetPlayerLocation(TEXT("P1")), PosS03);
			TestEqual(TEXT("飲料罐垃圾也放在入口"), Logic.GetItemLocation(TEXT("F01")), PosS03);
			TestEqual(TEXT("飲料罐垃圾沒人占用"), Logic.GetItemOccupant(TEXT("F01")), FString());
			TestEqual(TEXT("飲料罐垃圾在地點外"), Logic.GetItemLocationId(TEXT("F01")), FString());

			// 變回人類：飲料留在原地，不會被放回 L01；已存在的物品不重產、不移動
			TestTrue(TEXT("變回人類"), Logic.ReturnToHuman(TEXT("P1")));
			TestEqual(TEXT("飲料留在入口"), Logic.GetItemLocation(TEXT("F02")), PosS03);
			TestEqual(TEXT("飲料沒人占用"), Logic.GetItemOccupant(TEXT("F02")), FString());
			TestTrue(TEXT("P2 變成飲料罐垃圾（放在入口的那個）"), Logic.Interact(TEXT("P2"), TEXT("F01")).bTransformed);
			const FInteractOutcome Again = Logic.Interact(TEXT("P2"), TEXT("S03"));
			TestEqual(TEXT("P2 進入 L01"), Again.EnteredLocationId, FString(TEXT("L01")));
			TestEqual(TEXT("已經出現過的飲料不重產、不移回 L01"), Logic.GetItemLocationId(TEXT("F02")), FString());
			TestEqual(TEXT("飲料位置沒動"), Logic.GetItemLocation(TEXT("F02")), PosS03);

			// 學生證：L02 裡變成學生證 → 帶著離開，再碰大門 → E01
			FEscapeGameLogic Logic2;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Logic2, Error));
			Logic2.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			TestTrue(TEXT("P1 走完路線變成學生證"), BecomeStudentCard(Logic2, TEXT("P1")));
			TestEqual(TEXT("P1 不在 L02"), Logic2.GetPlayerLocationId(TEXT("P1")), FString());
			TestEqual(TEXT("學生證放在飲料機底下入口"), Logic2.GetItemLocation(TEXT("F03")), PosS04);
			Logic2.SetPlayerLocation(TEXT("P1"), PosS01);
			const FInteractOutcome Gate = Logic2.Interact(TEXT("P1"), TEXT("S01"));
			TestEqual(TEXT("學生證碰大門是 X010"), Gate.Result.InteractionId, FString(TEXT("X010")));
			TestEqual(TEXT("結局 E01"), Gate.Result.EndingId, FString(TEXT("E01")));
			TestTrue(TEXT("P1 逃出"), Logic2.GetTeam().HasEscaped(TEXT("P1")));
		});

		It(TEXT("在特殊地點只能碰這個地點的東西和自己；從外面碰不到特殊地點裡的東西（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Logic.Interact(TEXT("P1"), TEXT("S02"));
			Logic.Interact(TEXT("P1"), TEXT("F01"));
			Logic.Interact(TEXT("P1"), TEXT("S03"));  // L01：飲料出現
			Logic.ReturnToHuman(TEXT("P1"));
			Logic.Interact(TEXT("P1"), TEXT("F01"));
			Logic.Interact(TEXT("P1"), TEXT("S04"));  // L02：學生證出現
			TestEqual(TEXT("P1 在 L02"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L02")));

			const int32 TriggeredBefore = Logic.GetTeam().GetTriggeredIds().Num();
			for (const TCHAR* Target : { TEXT("F02"), TEXT("S03"), TEXT("S04"), TEXT("S01"), TEXT("F12"), TEXT("F08") })
			{
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), Target);
				TestTrue(*FString::Printf(TEXT("在 L02 裡碰 %s → Rejected"), Target), Out.Kind == EInteractKind::Rejected);
				TestEqual(*FString::Printf(TEXT("%s：沒有文字"), Target), Out.Messages.Num(), 0);
			}
			TestEqual(TEXT("Rejected 沒改任何狀態"), Logic.GetTeam().GetTriggeredIds().Num(), TriggeredBefore);
			TestEqual(TEXT("還在 L02"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L02")));
			TestEqual(TEXT("形態不變"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F01")));
			TestTrue(TEXT("碰自己可以"), Logic.Interact(TEXT("P1"), TEXT("SELF")).Kind == EInteractKind::Resolved);

			// 從外面（P2 是人類，不在任何地點）碰不到地點裡的東西
			TestTrue(TEXT("學生證還在 L02 裡"), Logic.GetItemLocationId(TEXT("F03")) == TEXT("L02"));
			TestTrue(TEXT("P2 碰 L02 裡的學生證 → Rejected"), Logic.Interact(TEXT("P2"), TEXT("F03")).Kind == EInteractKind::Rejected);
			TestEqual(TEXT("P2 還是人類"), Logic.GetPlayerForm(TEXT("P2")), FString(TEXT("F00")));
			TestEqual(TEXT("學生證沒人占用"), Logic.GetItemOccupant(TEXT("F03")), FString());
			TestTrue(TEXT("飲料還在 L01 裡"), Logic.GetItemLocationId(TEXT("F02")) == TEXT("L01"));
			TestTrue(TEXT("P2 碰 L01 裡的飲料 → Rejected"), Logic.Interact(TEXT("P2"), TEXT("F02")).Kind == EInteractKind::Rejected);

			// 在地點裡可以碰這個地點的東西
			TestTrue(TEXT("P1 在 L02 裡碰學生證 → 變身"), Logic.Interact(TEXT("P1"), TEXT("F03")).bTransformed);
		});

		It(TEXT("還沒出現的物品、不存在的 ID、自己占用的物品 → Rejected，不改狀態（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(100, 0, 0));
			const int32 HpBefore = Logic.GetTeam().GetHp();

			auto CheckRejected = [&](const FString& Target, const TCHAR* Label)
			{
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), Target);
				TestTrue(*FString::Printf(TEXT("%s → Rejected"), Label), Out.Kind == EInteractKind::Rejected);
				TestEqual(*FString::Printf(TEXT("%s：沒有文字"), Label), Out.Messages.Num(), 0);
				TestEqual(*FString::Printf(TEXT("%s：沒有事件"), Label), Out.Events.Num(), 0);
				TestFalse(*FString::Printf(TEXT("%s：沒有變身"), Label), Out.bTransformed);
				TestEqual(*FString::Printf(TEXT("%s：沒有觸發任何 X"), Label), Logic.GetTeam().GetTriggeredIds().Num(), 0);
				TestEqual(*FString::Printf(TEXT("%s：生命值沒變"), Label), Logic.GetTeam().GetHp(), HpBefore);
			};
			// 還沒出現
			for (const TCHAR* Id : { TEXT("F01"), TEXT("F02"), TEXT("F03"), TEXT("F04"), TEXT("F05"), TEXT("F06"), TEXT("F07") })
			{
				CheckRejected(Id, Id);
				TestFalse(*FString::Printf(TEXT("%s 仍沒出現"), Id), Logic.IsItemPresent(Id));
			}
			// 不存在的 ID、F00 不能當對象、空字串
			CheckRejected(TEXT("S99"), TEXT("S99"));
			CheckRejected(TEXT("F99"), TEXT("F99"));
			CheckRejected(TEXT("F00"), TEXT("F00"));
			CheckRejected(TEXT(""), TEXT("空字串"));
			TestEqual(TEXT("形態不變"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestEqual(TEXT("位置不變"), Logic.GetPlayerLocation(TEXT("P1")), FVector(100, 0, 0));

			// 自己占用的物品
			TestTrue(TEXT("變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			const FInteractOutcome Own = Logic.Interact(TEXT("P1"), TEXT("F12"));
			TestTrue(TEXT("碰自己正在當的椅子 → Rejected"), Own.Kind == EInteractKind::Rejected);
			TestEqual(TEXT("沒有文字"), Own.Messages.Num(), 0);
			TestEqual(TEXT("仍是椅子"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F12")));
			TestEqual(TEXT("椅子仍是 P1 的"), Logic.GetItemOccupant(TEXT("F12")), FString(TEXT("P1")));

			// 對象是 SELF 可以；人類碰 SELF → 預設回應
			FEscapeGameLogic Human;
			MakeLogic(1, Human, Error);
			Human.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			const FInteractOutcome Self = Human.Interact(TEXT("P1"), TEXT("SELF"));
			TestTrue(TEXT("人類碰 SELF → Resolved 的預設回應"), Self.Kind == EInteractKind::Resolved && Self.Result.bIsDefault);
			TestEqual(TEXT("有一則文字"), Self.Messages.Num(), 1);
		});

		It(TEXT("骰子自身互動（X020）的點數在 1～6，固定種子可重現"), [this]()
		{
			auto Roll = [this](int32 Seed, TArray<int32>& Out)
			{
				FEscapeGameLogic Logic;
				FString Error;
				if (!MakeLogic(Seed, Logic, Error)) { AddError(Error); return; }
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				const FInteractOutcome Became = Logic.Interact(TEXT("P1"), TEXT("F08"));
				TestTrue(TEXT("變成骰子"), Became.bTransformed && Became.ToForm == TEXT("F08"));
				for (int32 I = 0; I < 120; ++I)
				{
					const FInteractOutcome O = Logic.Interact(TEXT("P1"), TEXT("SELF"));
					TestTrue(TEXT("骰子自身互動 → Resolved"), O.Kind == EInteractKind::Resolved);
					TestEqual(TEXT("是 X020"), O.Result.InteractionId, FString(TEXT("X020")));
					TestEqual(TEXT("一則文字"), O.Messages.Num(), 1);
					if (O.Messages.Num() == 1)
					{
						TestFalse(TEXT("文字裡沒有留下 [ 隨機] 的記號"), O.Messages[0].Contains(TEXT("隨機")) || O.Messages[0].Contains(TEXT("[")));
						Out.Add(ParseDice(O.Messages[0]));
						TestEqual(TEXT("Result.Text 與顯示文字一致"), O.Result.Text, O.Messages[0]);
					}
				}
			};
			TArray<int32> A, B, C;
			Roll(20261031, A);
			Roll(20261031, B);
			Roll(7, C);
			TestEqual(TEXT("擲了 120 次"), A.Num(), 120);
			TestEqual(TEXT("同種子點數完全相同"), A, B);
			TSet<int32> Seen;
			bool bAllInRange = true;
			for (int32 V : A) { if (V < 1 || V > 6) { bAllInRange = false; } Seen.Add(V); }
			TestTrue(TEXT("每次都在 1～6"), bAllInRange);
			TestEqual(TEXT("六個點數都出現過"), Seen.Num(), 6);
			TestTrue(TEXT("不同種子的序列不同"), A != C);

			FTeamState Team(5);
			bool bTeamOk = true;
			for (int32 I = 0; I < 200; ++I) { const int32 V = Team.RollRange(3, 9); if (V < 3 || V > 9) { bTeamOk = false; } }
			TestTrue(TEXT("RollRange(3, 9) 在範圍內"), bTeamOk);
			TestEqual(TEXT("RollRange(4, 4) 只有 4"), Team.RollRange(4, 4), 4);
		});

		It(TEXT("逃出的玩家放開物品，物品留在原地（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 走完路線變成學生證"), BecomeStudentCard(Logic, TEXT("P1")));
			Logic.SetPlayerLocation(TEXT("P1"), PosS01);

			const FInteractOutcome Gate = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestEqual(TEXT("E01"), Gate.Result.EndingId, FString(TEXT("E01")));
			TestTrue(TEXT("P1 逃出"), Logic.GetTeam().HasEscaped(TEXT("P1")));
			TestEqual(TEXT("學生證放開了"), Logic.GetItemOccupant(TEXT("F03")), FString());
			TestEqual(TEXT("學生證留在大門旁"), Logic.GetItemLocation(TEXT("F03")), PosS01);
			TestTrue(TEXT("學生證還在場景中"), Logic.IsItemPresent(TEXT("F03")));
			TestTrue(TEXT("已逃出者不能再互動"), Logic.Interact(TEXT("P1"), TEXT("F12")).Kind == EInteractKind::Rejected);
			TestFalse(TEXT("已逃出者不能變回人類"), Logic.ReturnToHuman(TEXT("P1")));

			// P2 撿同一張學生證再逃一次
			TestTrue(TEXT("P2 碰學生證 → 變身"), Logic.Interact(TEXT("P2"), TEXT("F03")).bTransformed);
			const FInteractOutcome Gate2 = Logic.Interact(TEXT("P2"), TEXT("S01"));
			TestEqual(TEXT("P2 也 E01"), Gate2.Result.EndingId, FString(TEXT("E01")));
			TestEqual(TEXT("學生證又放開了"), Logic.GetItemOccupant(TEXT("F03")), FString());

			// 時間到（E04）：所有還沒結束的玩家放開物品
			FEscapeGameLogic Timed;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Timed, Error));
			Timed.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Timed.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			Timed.Interact(TEXT("P1"), TEXT("F12"));
			Timed.Interact(TEXT("P2"), TEXT("F11"));
			Timed.SetPlayerLocation(TEXT("P1"), FVector(1, 2, 3));
			const TArray<FTeamEvent> Events = Timed.Tick(1801.0f);
			TestEqual(TEXT("時間到 E04"), Events.Num(), 1);
			TestEqual(TEXT("椅子放開了"), Timed.GetItemOccupant(TEXT("F12")), FString());
			TestEqual(TEXT("椅子留在原地"), Timed.GetItemLocation(TEXT("F12")), FVector(1, 2, 3));
			TestEqual(TEXT("鑰匙圈放開了"), Timed.GetItemOccupant(TEXT("F11")), FString());
			TestEqual(TEXT("鑰匙圈留在原地"), Timed.GetItemLocation(TEXT("F11")), PosF11);

			// 時鐘自身互動（X021）
			FEscapeGameLogic Clock;
			TestTrue(TEXT("CSV 載入"), MakeLogic(1, Clock, Error));
			Clock.AddPlayer(TEXT("P1"), FVector(0, 0, 0));
			Clock.Interact(TEXT("P1"), TEXT("F09"));
			const FInteractOutcome Dawn = Clock.Interact(TEXT("P1"), TEXT("SELF"));
			TestEqual(TEXT("X021 → E04"), Dawn.Result.EndingId, FString(TEXT("E04")));
			TestEqual(TEXT("時鐘放開了"), Clock.GetItemOccupant(TEXT("F09")), FString());
		});

		It(TEXT("E05 重啟後玩家變回人類回到出生點，物品回到初始狀態（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(10, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(20, 0, 0));
			Logic.AddPlayer(TEXT("P3"), FVector(30, 0, 0));

			// P3 先用密碼逃出（E02）；之後不會被重啟帶回
			Logic.SetPlayerLocation(TEXT("P3"), FVector(999, 0, 0));
			const FString Password = Logic.GetTeam().GetPasswordForDevOnly();
			TestEqual(TEXT("P3 輸入正確密碼 → E02"), Logic.SubmitPassword(TEXT("P3"), Password).Result.EndingId, FString(TEXT("E02")));

			// P1 變成椅子、搬走；垃圾桶出現飲料罐垃圾並被 P1 之後也能碰（先讓 F01 出現）
			TestTrue(TEXT("P1 變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			Logic.SetPlayerLocation(TEXT("P1"), FVector(777, 0, 0));
			Logic.SetObjectLocation(TEXT("F11"), FVector(1234, 0, 0));
			TestTrue(TEXT("P2 翻垃圾桶"), Logic.Interact(TEXT("P2"), TEXT("S02")).Result.InteractionId == TEXT("X001"));
			TestTrue(TEXT("飲料罐垃圾出現了"), Logic.IsItemPresent(TEXT("F01")));
			Logic.SetPlayerLocation(TEXT("P2"), FVector(60, 0, 0));

			// P2 一直打翻花盆（X009，每次 -1）直到 E05；中途 F04 出現
			FInteractOutcome Last;
			for (int32 I = 0; I < FTeamState::StartingHp; ++I)
			{
				Last = Logic.Interact(TEXT("P2"), TEXT("S10"));
			}
			TestEqual(TEXT("最後一次 → 一個事件"), Last.Events.Num(), 1);
			if (Last.Events.Num() == 1) { TestEqual(TEXT("E05"), Last.Events[0].EndingId, FString(TEXT("E05"))); }
			TestEqual(TEXT("生命值回到 5"), Logic.GetTeam().GetHp(), FTeamState::StartingHp);

			TestEqual(TEXT("P1 變回人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestEqual(TEXT("P1 回到出生點"), Logic.GetPlayerLocation(TEXT("P1")), FVector(10, 0, 0));
			TestEqual(TEXT("P1 的身體也在出生點"), Logic.GetBodyLocation(TEXT("P1")), FVector(10, 0, 0));
			TestEqual(TEXT("P2 回到出生點"), Logic.GetPlayerLocation(TEXT("P2")), FVector(20, 0, 0));
			TestEqual(TEXT("P2 的身體也在出生點"), Logic.GetBodyLocation(TEXT("P2")), FVector(20, 0, 0));
			TestEqual(TEXT("P1 不在任何地點"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
			// 已逃出的 P3 沒有被帶回
			TestTrue(TEXT("P3 仍是已逃出"), Logic.GetTeam().HasEscaped(TEXT("P3")));
			TestEqual(TEXT("P3 的位置沒動"), Logic.GetPlayerLocation(TEXT("P3")), FVector(999, 0, 0));

			// 物品回到初始狀態
			TestEqual(TEXT("椅子沒人占用"), Logic.GetItemOccupant(TEXT("F12")), FString());
			TestEqual(TEXT("椅子回到註冊位置"), Logic.GetItemLocation(TEXT("F12")), PosF12);
			TestEqual(TEXT("鑰匙圈回到註冊位置"), Logic.GetItemLocation(TEXT("F11")), PosF11);
			for (const FString& Item : StartItems())
			{
				TestTrue(*FString::Printf(TEXT("%s 還在場景中"), *Item), Logic.IsItemPresent(Item));
			}
			TestFalse(TEXT("飲料罐垃圾要重新翻出來"), Logic.IsItemPresent(TEXT("F01")));
			TestFalse(TEXT("辦公室鑰匙要重新打翻花盆"), Logic.IsItemPresent(TEXT("F04")));

			// 重啟後可以重新變身
			TestTrue(TEXT("P1 可以再變成椅子"), Logic.Interact(TEXT("P1"), TEXT("F12")).bTransformed);
			TestEqual(TEXT("身體在出生點"), Logic.GetBodyLocation(TEXT("P1")), FVector(10, 0, 0));
		});

		It(TEXT("在特殊地點時 E05 重啟，玩家離開地點、物品回到初始（PLANS 詮釋）"), [this]()
		{
			FEscapeGameLogic Logic;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			Logic.AddPlayer(TEXT("P1"), FVector(10, 0, 0));
			Logic.AddPlayer(TEXT("P2"), FVector(20, 0, 0));
			Logic.Interact(TEXT("P1"), TEXT("S02"));
			Logic.Interact(TEXT("P1"), TEXT("F01"));
			Logic.Interact(TEXT("P1"), TEXT("S03"));
			TestEqual(TEXT("P1 在 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString(TEXT("L01")));
			for (int32 I = 0; I < FTeamState::StartingHp; ++I) { Logic.Interact(TEXT("P2"), TEXT("S10")); }

			TestEqual(TEXT("P1 變回人類"), Logic.GetPlayerForm(TEXT("P1")), FString(TEXT("F00")));
			TestEqual(TEXT("P1 離開 L01"), Logic.GetPlayerLocationId(TEXT("P1")), FString());
			TestFalse(TEXT("飲料罐垃圾沒了"), Logic.IsItemPresent(TEXT("F01")));
			TestFalse(TEXT("飲料沒了"), Logic.IsItemPresent(TEXT("F02")));
			TestEqual(TEXT("飲料罐垃圾沒人占用"), Logic.GetItemOccupant(TEXT("F01")), FString());
			// 重新來一次
			TestTrue(TEXT("X001 重新可以翻"), Logic.Interact(TEXT("P1"), TEXT("S02")).Result.InteractionId == TEXT("X001"));
			TestTrue(TEXT("飲料罐垃圾又出現"), Logic.IsItemPresent(TEXT("F01")));
		});
	});
}

#endif  // !UE_BUILD_SHIPPING
