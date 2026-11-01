// 步驟 10 組合層：GameMode。
#include "Core/EscapeGameMode.h"
#include "Core/EscapeGameState.h"
#include "Core/EscapePlayerController.h"
#include "Core/EscapePlayerState.h"
#include "Forms/EscapeFormPawn.h"
#include "Forms/EscapeObjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "UI/EscapeHUD.h"
#include "EscapeGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool ReadDataFile(const TCHAR* Name, FString& Out, FString& OutError)
	{
		const FString Path = FPaths::ProjectContentDir() / TEXT("Data") / Name;
		if (!FFileHelper::LoadFileToString(Out, *Path))
		{
			OutError = FString::Printf(TEXT("讀不到 %s"), *Path);
			return false;
		}
		return true;
	}

	/** F01～F12（可變身物品）；F00 是人類身體，不在這裡 */
	bool IsItemId(const FString& Id)
	{
		return Id.Len() == 3 && Id[0] == TEXT('F') && Id != TEXT("F00");
	}
}

AEscapeGameMode::AEscapeGameMode()
{
	PlayerControllerClass = AEscapePlayerController::StaticClass();
	GameStateClass = AEscapeGameState::StaticClass();
	PlayerStateClass = AEscapePlayerState::StaticClass();
	HUDClass = AEscapeHUD::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonCharacter(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonCharacter.Class != nullptr) { DefaultPawnClass = ThirdPersonCharacter.Class; }

	PrimaryActorTick.bCanEverTick = true;
}

bool AEscapeGameMode::LoadLogic(FString& OutError)
{
	bLogicLoaded = false;
	FString Interactions, Forms, SceneObjects, Responses, Locations;
	if (!ReadDataFile(TEXT("DT_Interactions.csv"), Interactions, OutError)
		|| !ReadDataFile(TEXT("DT_Forms.csv"), Forms, OutError)
		|| !ReadDataFile(TEXT("DT_SceneObjects.csv"), SceneObjects, OutError)
		|| !ReadDataFile(TEXT("DT_Responses.csv"), Responses, OutError)
		|| !ReadDataFile(TEXT("DT_Locations.csv"), Locations, OutError))
	{
		return false;
	}

	int32 Seed = static_cast<int32>(FDateTime::Now().GetTicks());
#if !UE_BUILD_SHIPPING
	if (bHasSeedOverride) { Seed = SeedOverride; }
#endif
	FEscapeGameLogic NewLogic;
	if (!FEscapeGameLogic::Load(Interactions, Forms, SceneObjects, Responses, Locations, Seed, NewLogic, OutError)) { return false; }
	Logic = MoveTemp(NewLogic);
	bLogicLoaded = true;
	return true;
}

void AEscapeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	FString Error;
	if (!LoadLogic(Error)) { UE_LOG(LogEscapeGame, Error, TEXT("載入遊戲資料失敗：%s"), *Error); }
}

void AEscapeGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!bLogicLoaded || !GetWorld()) { return; }

	// 關卡裡每個物件的位置交給 logic（F 的位置也是 E05 重啟時回去的位置）
	for (TActorIterator<AEscapeObjectActor> It(GetWorld()); It; ++It)
	{
		if (!It->ObjectId.IsEmpty()) { Logic.RegisterObjectLocation(It->ObjectId, It->GetActorLocation()); }
	}
	for (TActorIterator<AEscapeFormPawn> It(GetWorld()); It; ++It)
	{
		if (IsItemId(It->FormId)) { Logic.RegisterObjectLocation(It->FormId, It->GetActorLocation()); }
	}
	SyncSceneActors();
}

FString AEscapeGameMode::PlayerIdOf(const AEscapePlayerController* Controller)
{
	return Controller ? Controller->GetEscapePlayerId() : FString();
}

void AEscapeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AEscapePlayerController* Controller = Cast<AEscapePlayerController>(NewPlayer);
	if (!bLogicLoaded || !Controller) { return; }

	// 出生點：pawn 已經生出來就用它的位置，否則用 PlayerStart
	FVector Spawn = FVector::ZeroVector;
	if (const APawn* Pawn = Controller->GetPawn()) { Spawn = Pawn->GetActorLocation(); }
	else if (const AActor* Start = ChoosePlayerStart(Controller)) { Spawn = Start->GetActorLocation(); }

	const FString PlayerId = PlayerIdOf(Controller);
	Logic.AddPlayer(PlayerId, Spawn);
	Controllers.Add(PlayerId, Controller);
	SyncGameState();
}

void AEscapeGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLogicLoaded || Controllers.Num() == 0) { return; }  // 還沒有玩家：時鐘不走

	FInteractOutcome TickResult;  // 只借它的 Events 與 NewAchievements 欄位
	TickResult.Events = Logic.Tick(DeltaSeconds);
	TickResult.NewAchievements = Logic.PopNewAchievements();
	if (TickResult.Events.Num() > 0 || TickResult.NewAchievements.Num() > 0)
	{
		SyncSceneActors();
		Dispatch(FString(), TickResult);
	}
	SyncGameState();
}

FInteractOutcome AEscapeGameMode::HandleInteract(AEscapePlayerController* Controller, const FString& TargetId)
{
	if (!bLogicLoaded) { return FInteractOutcome(); }
	const FString PlayerId = PlayerIdOf(Controller);
	const FInteractOutcome Outcome = Logic.Interact(PlayerId, TargetId);
	Logic.PopNewAchievements();  // 這次互動的成就已在 Outcome.NewAchievements，不要在下一個 Tick 再送一次
	SyncSceneActors();
	SyncGameState();
	Dispatch(PlayerId, Outcome);
	return Outcome;
}

FInteractOutcome AEscapeGameMode::HandleSubmitPassword(AEscapePlayerController* Controller, const FString& Code)
{
	if (!bLogicLoaded) { return FInteractOutcome(); }
	const FString PlayerId = PlayerIdOf(Controller);
	const FInteractOutcome Outcome = Logic.SubmitPassword(PlayerId, Code);
	Logic.PopNewAchievements();
	SyncSceneActors();
	SyncGameState();
	Dispatch(PlayerId, Outcome);
	return Outcome;
}

bool AEscapeGameMode::HandleReturnToHuman(AEscapePlayerController* Controller)
{
	if (!bLogicLoaded) { return false; }
	const bool bChanged = Logic.ReturnToHuman(PlayerIdOf(Controller));
	SyncSceneActors();
	SyncGameState();
	return bChanged;
}

void AEscapeGameMode::Dispatch(const FString& ActorPlayerId, const FInteractOutcome& Outcome)
{
	for (const TPair<FString, TWeakObjectPtr<AEscapePlayerController>>& Pair : Controllers)
	{
		AEscapePlayerController* Controller = Pair.Value.Get();
		if (!Controller) { continue; }
		const FString& PlayerId = Pair.Key;

		TArray<FString> EndingIds;
		for (const FTeamEvent& Event : Outcome.Events)
		{
			if (Event.AffectedPlayers.Contains(PlayerId)) { EndingIds.Add(Event.EndingId); }
		}
		TArray<FString> AchievementIds;
		for (const FAchievementUnlock& Unlock : Outcome.NewAchievements)
		{
			if (Unlock.PlayerId == PlayerId) { AchievementIds.Add(Unlock.AchievementId); }
		}

		if (PlayerId == ActorPlayerId)
		{
			Controller->ReceiveResult(Outcome.Kind, Outcome.Messages, EndingIds, AchievementIds);
		}
		else if (EndingIds.Num() > 0 || AchievementIds.Num() > 0)
		{
			// 別人的行動或時間到：只有結局與成就，沒有文字（Kind 用 Rejected＝不彈出框、不開鍵盤）
			Controller->ReceiveResult(EInteractKind::Rejected, TArray<FString>(), EndingIds, AchievementIds);
		}
	}
}

void AEscapeGameMode::SyncGameState()
{
	if (AEscapeGameState* State = GetGameState<AEscapeGameState>())
	{
		State->Hp = Logic.GetTeam().GetHp();
		State->RemainingSeconds = Logic.GetTeam().GetRemainingSeconds();
	}
	for (const TPair<FString, TWeakObjectPtr<AEscapePlayerController>>& Pair : Controllers)
	{
		const AEscapePlayerController* Controller = Pair.Value.Get();
		AEscapePlayerState* PlayerState = Controller ? Controller->GetPlayerState<AEscapePlayerState>() : nullptr;
		if (!PlayerState) { continue; }
		PlayerState->FormId = Logic.GetPlayerForm(Pair.Key);
		PlayerState->FormName = Logic.GetResolver().GetDisplayName(PlayerState->FormId);
	}
}

void AEscapeGameMode::SyncSceneActors()
{
	UWorld* World = GetWorld();
	if (!bLogicLoaded || !World) { return; }

	for (TActorIterator<AEscapeObjectActor> It(World); It; ++It)
	{
		AEscapeObjectActor* Actor = *It;
		if (Actor->bIsDoor)
		{
			const bool bOpen = Logic.IsDoorOpen(Actor->ObjectId);
			if (Actor->IsDoorOpen() != bOpen) { Actor->SetDoorOpen(bOpen); }
		}
		else if (IsItemId(Actor->ObjectId))
		{
			const bool bPresent = Logic.IsItemPresent(Actor->ObjectId);
			if (Actor->IsItemPresent() != bPresent) { Actor->SetItemPresent(bPresent); }
		}
	}
	for (TActorIterator<AEscapeFormPawn> It(World); It; ++It)
	{
		AEscapeFormPawn* Pawn = *It;
		if (!IsItemId(Pawn->FormId)) { continue; }
		const bool bPresent = Logic.IsItemPresent(Pawn->FormId);
		Pawn->SetActorHiddenInGame(!bPresent);
		if (Pawn->Mesh) { Pawn->Mesh->SetCollisionEnabled(bPresent ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision); }
	}
}
