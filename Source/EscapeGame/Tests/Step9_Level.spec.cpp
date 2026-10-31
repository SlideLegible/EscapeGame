// 步驟 9 灰盒關卡 → VERIFY STEP 9
//
// 在編輯器世界載入主關卡 /Game/Maps/Lvl_Academy，掃描 Actor（區域盒子、物件、出生點），
// 再用玩家膠囊（半徑 42、半高 96）做 2D 洪水填充檢查可達性。
// 整個檔案只在編輯器編譯：載入地圖要 UnrealEd。-nullrhi 的 UnrealEditor-Cmd 也跑得了（EditorContext）。
#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "FileHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Core/EscapeAreaVolume.h"
#include "Forms/EscapeObjectActor.h"
#include "Forms/EscapeFormPawn.h"

namespace Step9Spec
{
	const TCHAR* MapPath = TEXT("/Game/Maps/Lvl_Academy");

	// ---- 可達性檢查的參數（照 step9_contract：格子 25 cm、膠囊 42／96） ----
	constexpr float CellSize = 25.0f;
	constexpr float CapsuleRadius = 42.0f;
	constexpr float CapsuleHalfHeight = 96.0f;
	constexpr float CapsuleLift = 2.0f;        // 膠囊離地 2 cm，避免和地板剛好貼著
	constexpr float FloorTolerance = 20.0f;    // 地板頂面要在「區域底」上下 20 cm 內
	constexpr float TraceUp = 100.0f;          // 往下打的起點：區域底上方 100 cm
	constexpr float TraceDown = 50.0f;         // 終點：區域底下方 50 cm

	// ---- 區域對照表：設計文件的位置文字 → 區域 ID（測試自己的規格，和 test_step9_layout.py 各寫一份） ----
	const TMap<FString, FName>& LocationToArea()
	{
		static const TMap<FString, FName> Map = {
			{ TEXT("密碼學教室"), TEXT("Classroom") },
			{ TEXT("走廊"), TEXT("Corridor") },
			{ TEXT("走廊（花盆裡）"), TEXT("Corridor") },
			{ TEXT("辦公室"), TEXT("Office") },
			{ TEXT("廁所"), TEXT("Restroom") },
			{ TEXT("廁所（每間都有）"), TEXT("Restroom") },
			{ TEXT("廁所儲物間內部"), TEXT("Storage") },
			{ TEXT("警衛室"), TEXT("GuardRoom") },
			{ TEXT("飲料機內部"), TEXT("L01") },
			{ TEXT("飲料機底下"), TEXT("L02") },
		};
		return Map;
	}

	const TArray<FName>& AllAreaIds()
	{
		static const TArray<FName> Ids = {
			TEXT("Classroom"), TEXT("Corridor"), TEXT("Office"), TEXT("Restroom"),
			TEXT("Storage"), TEXT("GuardRoom"), TEXT("L01"), TEXT("L02") };
		return Ids;
	}

	// ---- 讀 Content/Data 的 CSV（簡單的 RFC4180：支援雙引號與欄位內逗號） ----
	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	TArray<TArray<FString>> ParseCsv(const FString& Text)
	{
		TArray<TArray<FString>> Rows;
		TArray<FString> Row;
		FString Field;
		bool bQuoted = false;
		for (int32 i = 0; i < Text.Len(); ++i)
		{
			const TCHAR C = Text[i];
			if (bQuoted)
			{
				if (C == TEXT('"'))
				{
					if (i + 1 < Text.Len() && Text[i + 1] == TEXT('"')) { Field.AppendChar(TEXT('"')); ++i; }
					else { bQuoted = false; }
				}
				else { Field.AppendChar(C); }
			}
			else if (C == TEXT('"')) { bQuoted = true; }
			else if (C == TEXT(',')) { Row.Add(Field); Field.Reset(); }
			else if (C == TEXT('\n') || C == TEXT('\r'))
			{
				if (C == TEXT('\r') && i + 1 < Text.Len() && Text[i + 1] == TEXT('\n')) { ++i; }
				Row.Add(Field); Field.Reset();
				if (Row.Num() > 1 || !Row[0].IsEmpty()) { Rows.Add(Row); }
				Row.Reset();
			}
			else { Field.AppendChar(C); }
		}
		if (!Field.IsEmpty() || Row.Num() > 0) { Row.Add(Field); Rows.Add(Row); }
		return Rows;
	}

	/** Id → 設計文件寫的位置文字。F01～F12 來自 DT_Forms（Name 欄 0、Location 欄 3），S01～S20 來自 DT_SceneObjects（Location 欄 2）。F00 不放。 */
	TMap<FString, FString> DesignLocations()
	{
		TMap<FString, FString> Out;
		const TArray<TArray<FString>> Forms = ParseCsv(LoadText(TEXT("DT_Forms.csv")));
		for (int32 i = 1; i < Forms.Num(); ++i)
		{
			if (Forms[i].Num() > 3 && Forms[i][0] != TEXT("F00")) { Out.Add(Forms[i][0], Forms[i][3].TrimStartAndEnd()); }
		}
		const TArray<TArray<FString>> Scene = ParseCsv(LoadText(TEXT("DT_SceneObjects.csv")));
		for (int32 i = 1; i < Scene.Num(); ++i)
		{
			if (Scene[i].Num() > 2) { Out.Add(Scene[i][0], Scene[i][2].TrimStartAndEnd()); }
		}
		return Out;
	}

	TArray<FString> ExpectedObjectIds()
	{
		TArray<FString> Ids;
		for (int32 n = 1; n <= 12; ++n) { Ids.Add(FString::Printf(TEXT("F%02d"), n)); }
		for (int32 n = 1; n <= 20; ++n) { Ids.Add(FString::Printf(TEXT("S%02d"), n)); }
		return Ids;
	}

	// ---- 載入地圖 ----
	/** 地圖不存在 → 回傳 nullptr（不載入，避免 log 一堆錯誤）；存在 → 載入並回傳編輯器世界 */
	UWorld* LoadAcademy()
	{
		if (!FPackageName::DoesPackageExist(MapPath)) { return nullptr; }
		return UEditorLoadingAndSavingUtils::LoadMap(MapPath);
	}

	// ---- 找 Actor ----
	TMap<FName, AEscapeAreaVolume*> FindAreas(UWorld* World)
	{
		TMap<FName, AEscapeAreaVolume*> Out;
		for (TActorIterator<AEscapeAreaVolume> It(World); It; ++It) { Out.Add(It->AreaId, *It); }
		return Out;
	}

	AEscapeObjectActor* FindDoor(UWorld* World, const FString& Id)
	{
		for (TActorIterator<AEscapeObjectActor> It(World); It; ++It)
		{
			if (It->ObjectId == Id && It->bIsDoor) { return *It; }
		}
		return nullptr;
	}

	void SetAllDoors(UWorld* World, bool bOpen)
	{
		for (TActorIterator<AEscapeObjectActor> It(World); It; ++It)
		{
			if (It->bIsDoor) { It->SetDoorOpen(bOpen); }
		}
	}

	// ---- 洪水填充 ----
	struct FWalkContext
	{
		UWorld* World = nullptr;
		FCollisionQueryParams Params;
		TArray<FBox> AreaBoxes;   // 所有區域盒子（世界座標）
		float GlobalFloorZ = 0.0f;  // 所有區域盒子底部的最小 Z；格子不在任何區域裡時拿它當「區域底」
	};

	/** 格子中心的「區域底」Z：XY 落在某個區域盒子裡就用它的底，否則用全域最小底 */
	float FloorReferenceZ(const FWalkContext& Ctx, const FVector2D& XY)
	{
		for (const FBox& B : Ctx.AreaBoxes)
		{
			if (XY.X >= B.Min.X && XY.X <= B.Max.X && XY.Y >= B.Min.Y && XY.Y <= B.Max.Y) { return B.Min.Z; }
		}
		return Ctx.GlobalFloorZ;
	}

	/**
	 * 這一格站得住嗎？從「區域底上方 100」往下打 ECC_Visibility 的線，要在區域底上下 FloorTolerance 內打到地板（沒有起點就在牆或物件裡 → 不行）；
	 * 然後把膠囊放在地板上方（中心＝地板 + 96 + 2），ECC_Pawn 不可以有擋到的東西。OutCapsuleZ 是膠囊中心高度。
	 */
	bool IsStandable(const FWalkContext& Ctx, const FVector2D& XY, float& OutCapsuleZ)
	{
		const float RefZ = FloorReferenceZ(Ctx, XY);
		FHitResult Hit;
		if (!Ctx.World->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, RefZ + TraceUp), FVector(XY.X, XY.Y, RefZ - TraceDown),
				ECC_Visibility, Ctx.Params))
		{
			return false;
		}
		if (Hit.bStartPenetrating || FMath::Abs(Hit.ImpactPoint.Z - RefZ) > FloorTolerance) { return false; }
		OutCapsuleZ = Hit.ImpactPoint.Z + CapsuleHalfHeight + CapsuleLift;
		return !Ctx.World->OverlapBlockingTestByChannel(FVector(XY.X, XY.Y, OutCapsuleZ), FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight), Ctx.Params);
	}

	FVector2D CellCenter(const FBox2D& Bounds, const FIntPoint& Cell)
	{
		return FVector2D(Bounds.Min.X + (Cell.X + 0.5f) * CellSize, Bounds.Min.Y + (Cell.Y + 0.5f) * CellSize);
	}

	/**
	 * 從 Start（世界座標，通常是 PlayerStart）所在的格子開始，在 Bounds（XY）內做 4 方向洪水填充，回傳可達格。
	 * 格子 = CellSize，格子 (I,J) 的中心 = Bounds.Min + ((I,J) + 0.5) * CellSize。
	 * 相鄰格之間用 SweepTestByChannel(ECC_Pawn) 的膠囊從一格中心掃到另一格中心；沒被擋到才算走得過去。
	 * 起點的格子站不住 → 回傳空集合。Ignored：不算在內的 Actor（人類身體 Pawn、PlayerStart）。
	 */
	TSet<FIntPoint> FloodFill(UWorld* World, const FVector& Start, const FBox2D& Bounds, const TArray<FBox>& AreaBoxes,
		const TArray<AActor*>& Ignored)
	{
		FWalkContext Ctx;
		Ctx.World = World;
		Ctx.Params = FCollisionQueryParams(SCENE_QUERY_STAT(EscapeStep9Reach), false);
		Ctx.Params.AddIgnoredActors(Ignored);
		Ctx.AreaBoxes = AreaBoxes;
		Ctx.GlobalFloorZ = AreaBoxes.Num() > 0 ? AreaBoxes[0].Min.Z : 0.0f;
		for (const FBox& B : AreaBoxes) { Ctx.GlobalFloorZ = FMath::Min(Ctx.GlobalFloorZ, B.Min.Z); }

		const FIntPoint GridSize(FMath::CeilToInt((Bounds.Max.X - Bounds.Min.X) / CellSize),
			FMath::CeilToInt((Bounds.Max.Y - Bounds.Min.Y) / CellSize));
		auto InGrid = [&GridSize](const FIntPoint& C) { return C.X >= 0 && C.Y >= 0 && C.X < GridSize.X && C.Y < GridSize.Y; };

		TSet<FIntPoint> Reached;
		TMap<FIntPoint, TOptional<float>> StandableZ;  // 每格只算一次：站得住 → 膠囊中心 Z；站不住 → 空
		auto GetStandable = [&](const FIntPoint& C) -> TOptional<float>
		{
			if (const TOptional<float>* Found = StandableZ.Find(C)) { return *Found; }
			float Z = 0.0f;
			TOptional<float> R;
			if (IsStandable(Ctx, CellCenter(Bounds, C), Z)) { R = Z; }
			StandableZ.Add(C, R);
			return R;
		};

		const FIntPoint StartCell(FMath::FloorToInt((Start.X - Bounds.Min.X) / CellSize), FMath::FloorToInt((Start.Y - Bounds.Min.Y) / CellSize));
		if (!InGrid(StartCell) || !GetStandable(StartCell).IsSet()) { return Reached; }

		TArray<FIntPoint> Stack;
		Stack.Add(StartCell);
		Reached.Add(StartCell);
		const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
		const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
		while (Stack.Num() > 0)
		{
			const FIntPoint Cur = Stack.Pop(EAllowShrinking::No);
			const FVector2D CurXY = CellCenter(Bounds, Cur);
			const float CurZ = GetStandable(Cur).GetValue();
			for (const FIntPoint& D : Dirs)
			{
				const FIntPoint N = Cur + D;
				if (!InGrid(N) || Reached.Contains(N)) { continue; }
				const TOptional<float> NZ = GetStandable(N);
				if (!NZ.IsSet()) { continue; }
				const FVector2D NXY = CellCenter(Bounds, N);
				if (World->SweepTestByChannel(FVector(CurXY.X, CurXY.Y, CurZ), FVector(NXY.X, NXY.Y, NZ.GetValue()), FQuat::Identity,
						ECC_Pawn, Capsule, Ctx.Params))
				{
					continue;  // 被擋住
				}
				Reached.Add(N);
				Stack.Add(N);
			}
		}
		return Reached;
	}

	/** 從 PlayerStart 開始，目前門的狀態下，走得到的區域 ID 集合。失敗（沒有 PlayerStart 等）→ false 並寫 Error */
	bool ComputeReachableAreas(UWorld* World, TSet<FName>& OutAreas, int32& OutCellCount, FString& Error)
	{
		OutAreas.Reset();
		OutCellCount = 0;
		const TMap<FName, AEscapeAreaVolume*> Areas = FindAreas(World);
		if (Areas.Num() == 0) { Error = TEXT("地圖裡沒有任何 AEscapeAreaVolume"); return false; }

		TArray<FBox> Boxes;
		FBox2D Bounds(ForceInit);
		for (const TPair<FName, AEscapeAreaVolume*>& P : Areas)
		{
			if (!P.Value->Box) { Error = FString::Printf(TEXT("區域 %s 沒有 Box 元件"), *P.Key.ToString()); return false; }
			const FBox B = P.Value->Box->Bounds.GetBox();
			Boxes.Add(B);
			Bounds += FVector2D(B.Min.X, B.Min.Y);
			Bounds += FVector2D(B.Max.X, B.Max.Y);
		}

		TArray<AActor*> Ignored;
		APlayerStart* PlayerStart = nullptr;
		for (TActorIterator<APlayerStart> It(World); It; ++It) { if (!PlayerStart) { PlayerStart = *It; } Ignored.Add(*It); }
		for (TActorIterator<AEscapeFormPawn> It(World); It; ++It) { Ignored.Add(*It); }
		if (!PlayerStart) { Error = TEXT("地圖裡沒有 APlayerStart"); return false; }

		const TSet<FIntPoint> Cells = FloodFill(World, PlayerStart->GetActorLocation(), Bounds, Boxes, Ignored);
		OutCellCount = Cells.Num();
		for (const TPair<FName, AEscapeAreaVolume*>& P : Areas)
		{
			for (const FIntPoint& C : Cells)
			{
				const FVector2D XY = CellCenter(Bounds, C);
				// 用區域盒子的中心高度比 XY，避免受格子高度影響
				if (P.Value->ContainsPoint(FVector(XY.X, XY.Y, P.Value->GetActorLocation().Z))) { OutAreas.Add(P.Key); break; }
			}
		}
		return true;
	}

	FString JoinNames(const TSet<FName>& Set)
	{
		TArray<FString> Names;
		for (const FName& N : Set) { Names.Add(N.ToString()); }
		Names.Sort();
		return FString::Join(Names, TEXT("、"));
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep9Spec, "EscapeGame.Step9",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* World = nullptr;
	/** 區域 Id 是否都走得到／走不到；Reached 是 ComputeReachableAreas 的結果 */
	void ExpectReach(const TSet<FName>& Reached, const TCHAR* Situation, const TArray<FName>& MustReach, const TArray<FName>& MustNotReach);
END_DEFINE_SPEC(FEscapeGameStep9Spec)

void FEscapeGameStep9Spec::ExpectReach(const TSet<FName>& Reached, const TCHAR* Situation, const TArray<FName>& MustReach,
	const TArray<FName>& MustNotReach)
{
	for (const FName& A : MustReach)
	{
		TestTrue(*FString::Printf(TEXT("%s：走得到 %s（實際走得到：%s）"), Situation, *A.ToString(), *Step9Spec::JoinNames(Reached)), Reached.Contains(A));
	}
	for (const FName& A : MustNotReach)
	{
		TestFalse(*FString::Printf(TEXT("%s：不可以走得到 %s"), Situation, *A.ToString()), Reached.Contains(A));
	}
}

void FEscapeGameStep9Spec::Define()
{
	using namespace Step9Spec;

	Describe(TEXT("VERIFY STEP 9"), [this]()
	{
		BeforeEach([this]()
		{
			World = LoadAcademy();
		});

		AfterEach([this]()
		{
			// 門恢復成關（重新載入地圖本來就會重置，這裡是保險）
			if (World) { SetAllDoors(World, false); }
			World = nullptr;
		});

		It(TEXT("開啟主關卡 → 有密碼學教室、走廊、辦公室、廁所、儲物間、警衛室、飲料機內部、飲料機底下 8 個區域"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			TMap<FName, int32> Count;
			for (TActorIterator<AEscapeAreaVolume> It(World); It; ++It) { Count.FindOrAdd(It->AreaId)++; }
			TestEqual(TEXT("區域盒子的總數"), Count.Num(), 8);
			for (const FName& Id : AllAreaIds())
			{
				const int32* N = Count.Find(Id);
				TestEqual(*FString::Printf(TEXT("區域 %s 剛好一個"), *Id.ToString()), N ? *N : 0, 1);
			}
		});

		It(TEXT("自動化測試掃描主關卡 → F01～F12 每個可變身物品、S01～S20 每個場景物件都剛好放了 1 個（馬桶可以多個），而且放在設計文件寫的區域"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			const TMap<FName, AEscapeAreaVolume*> Areas = FindAreas(World);
			TestEqual(TEXT("先要有 8 個區域"), Areas.Num(), 8);

			TMap<FString, TArray<AEscapeObjectActor*>> Placed;
			for (TActorIterator<AEscapeObjectActor> It(World); It; ++It) { Placed.FindOrAdd(It->ObjectId).Add(*It); }

			const TMap<FString, FString> Design = DesignLocations();
			const TArray<FString> Expected = ExpectedObjectIds();
			for (const FString& Id : Expected)
			{
				const FString* Text = Design.Find(Id);
				const FName* WantArea = Text ? LocationToArea().Find(*Text) : nullptr;
				if (!WantArea)
				{
					AddError(FString::Printf(TEXT("%s 的設計位置文字 %s 不在區域對照表裡"), *Id, Text ? **Text : TEXT("（DT_Forms／DT_SceneObjects 裡找不到）")));
					continue;
				}
				const TArray<AEscapeObjectActor*>* List = Placed.Find(Id);
				const int32 N = List ? List->Num() : 0;
				if (Id == TEXT("S11")) { TestTrue(TEXT("S11 馬桶至少放 1 個"), N >= 1); }
				else { TestEqual(*FString::Printf(TEXT("%s 剛好放 1 個"), *Id), N, 1); }

				const AEscapeAreaVolume* const* Vol = Areas.Find(*WantArea);
				if (!Vol) { AddError(FString::Printf(TEXT("%s 要在區域 %s，但地圖沒有這個區域"), *Id, *WantArea->ToString())); continue; }
				for (int32 i = 0; i < N; ++i)
				{
					TestTrue(*FString::Printf(TEXT("%s（第 %d 個）在區域 %s 裡（%s）"), *Id, i + 1, *WantArea->ToString(), **Text),
						(*Vol)->ContainsPoint((*List)[i]->GetActorLocation()));
				}
			}
			for (const TPair<FString, TArray<AEscapeObjectActor*>>& P : Placed)
			{
				if (!Expected.Contains(P.Key)) { AddError(FString::Printf(TEXT("多出不認得的物件 Id：%s"), *P.Key)); }
			}
		});

		It(TEXT("開局玩家出生在密碼學教室"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			TArray<APlayerStart*> Starts;
			for (TActorIterator<APlayerStart> It(World); It; ++It) { Starts.Add(*It); }
			if (!TestEqual(TEXT("PlayerStart 剛好 1 個"), Starts.Num(), 1)) { return; }
			const TMap<FName, AEscapeAreaVolume*> Areas = FindAreas(World);
			const AEscapeAreaVolume* const* Classroom = Areas.Find(TEXT("Classroom"));
			if (!TestTrue(TEXT("有 Classroom 區域"), Classroom != nullptr)) { return; }
			TestTrue(TEXT("PlayerStart 在密碼學教室裡"), (*Classroom)->ContainsPoint(Starts[0]->GetActorLocation()));
		});

		It(TEXT("不可以 教室門撞開前，人類能走出教室"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			SetAllDoors(World, false);
			TSet<FName> Reached;
			int32 Cells = 0;
			FString Error;
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), ComputeReachableAreas(World, Reached, Cells, Error))) { return; }
			TestTrue(*FString::Printf(TEXT("PlayerStart 站得住、至少走得到 1 格（%d 格）"), Cells), Cells > 0);
			ExpectReach(Reached, TEXT("門都關著"), { TEXT("Classroom") },
				{ TEXT("Corridor"), TEXT("Office"), TEXT("Restroom"), TEXT("Storage"), TEXT("GuardRoom"), TEXT("L01"), TEXT("L02") });
		});

		It(TEXT("不可以 沒開辦公室門、儲物間門之前能走進去"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			AEscapeObjectActor* S19 = FindDoor(World, TEXT("S19"));
			AEscapeObjectActor* S05 = FindDoor(World, TEXT("S05"));
			AEscapeObjectActor* S09 = FindDoor(World, TEXT("S09"));
			TestNotNull(TEXT("有教室門 S19（bIsDoor）"), S19);
			TestNotNull(TEXT("有辦公室門 S05（bIsDoor）"), S05);
			TestNotNull(TEXT("有儲物間門 S09（bIsDoor）"), S09);
			if (!S19 || !S05 || !S09) { return; }
			SetAllDoors(World, false);

			TSet<FName> Reached;
			int32 Cells = 0;
			FString Error;

			// 只開教室門：走得到走廊、警衛室、廁所；辦公室、儲物間、L01、L02 還不行
			S19->SetDoorOpen(true);
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), ComputeReachableAreas(World, Reached, Cells, Error))) { return; }
			ExpectReach(Reached, TEXT("只開教室門"), { TEXT("Classroom"), TEXT("Corridor"), TEXT("GuardRoom"), TEXT("Restroom") },
				{ TEXT("Office"), TEXT("Storage"), TEXT("L01"), TEXT("L02") });

			// 再開辦公室門：走得到辦公室；儲物間還不行
			S05->SetDoorOpen(true);
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), ComputeReachableAreas(World, Reached, Cells, Error))) { return; }
			ExpectReach(Reached, TEXT("開了教室門與辦公室門"), { TEXT("Office") }, { TEXT("Storage"), TEXT("L01"), TEXT("L02") });

			// 再開儲物間門：走得到儲物間；L01、L02 永遠走不到
			S09->SetDoorOpen(true);
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), ComputeReachableAreas(World, Reached, Cells, Error))) { return; }
			ExpectReach(Reached, TEXT("三扇門都開"), { TEXT("Office"), TEXT("Storage") }, { TEXT("L01"), TEXT("L02") });

			SetAllDoors(World, false);
		});
	});

	Describe(TEXT("PLANS 詮釋"), [this]()
	{
		BeforeEach([this]()
		{
			World = LoadAcademy();
		});

		AfterEach([this]()
		{
			if (World) { SetAllDoors(World, false); }
			World = nullptr;
		});

		It(TEXT("三個門 S19 S05 S09 都在關卡裡，而且都是 bIsDoor 的物件"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			for (const TCHAR* Id : { TEXT("S19"), TEXT("S05"), TEXT("S09") })
			{
				TestNotNull(*FString::Printf(TEXT("%s 是門"), Id), FindDoor(World, Id));
			}
		});

		It(TEXT("門開了又關上 → IsDoorOpen 跟著變，關上之後又擋住路"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			AEscapeObjectActor* S19 = FindDoor(World, TEXT("S19"));
			if (!TestNotNull(TEXT("有教室門 S19"), S19)) { return; }
			TestFalse(TEXT("一開始是關的"), S19->IsDoorOpen());
			S19->SetDoorOpen(true);
			TestTrue(TEXT("開了"), S19->IsDoorOpen());
			TestTrue(TEXT("開了就隱藏"), S19->IsHidden());
			TestEqual(TEXT("開了就沒有碰撞"), S19->Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			S19->SetDoorOpen(false);
			TestFalse(TEXT("關上"), S19->IsDoorOpen());
			TestFalse(TEXT("關上就顯示"), S19->IsHidden());
			TestNotEqual(TEXT("關上就恢復碰撞"), S19->Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

			TSet<FName> Reached;
			int32 Cells = 0;
			FString Error;
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), ComputeReachableAreas(World, Reached, Cells, Error))) { return; }
			ExpectReach(Reached, TEXT("開了又關上的教室門"), { TEXT("Classroom") }, { TEXT("Corridor") });
		});

		It(TEXT("門全開也走不到飲料機內部 L01 與飲料機底下 L02"), [this]()
		{
			if (!TestTrue(TEXT("找得到地圖 /Game/Maps/Lvl_Academy"), World != nullptr)) { return; }
			SetAllDoors(World, true);
			TSet<FName> Reached;
			int32 Cells = 0;
			FString Error;
			const bool bOk = ComputeReachableAreas(World, Reached, Cells, Error);
			SetAllDoors(World, false);
			if (!TestTrue(*FString::Printf(TEXT("算得出可達區域：%s"), *Error), bOk)) { return; }
			ExpectReach(Reached, TEXT("門全開"), { TEXT("Classroom"), TEXT("Corridor"), TEXT("Office"), TEXT("Restroom"), TEXT("Storage"), TEXT("GuardRoom") },
				{ TEXT("L01"), TEXT("L02") });
		});
	});
}

#endif  // WITH_EDITOR
