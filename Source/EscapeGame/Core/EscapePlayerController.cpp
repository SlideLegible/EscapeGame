// 步驟 10 組合層：PlayerController。
#include "Core/EscapePlayerController.h"
#include "Core/EscapeGameMode.h"
#include "GameFramework/PlayerState.h"
#include "UI/EscapeHUD.h"
#include "UI/EscapeKeypadWidget.h"
#include "Engine/World.h"

namespace
{
	AEscapeGameMode* GetAuthMode(const AActor* Context)
	{
		UWorld* World = Context ? Context->GetWorld() : nullptr;
		return World ? World->GetAuthGameMode<AEscapeGameMode>() : nullptr;
	}
}

FString AEscapePlayerController::GetEscapePlayerId() const
{
	return PlayerState ? FString::FromInt(PlayerState->GetPlayerId()) : FString();
}

AEscapeHUD* AEscapePlayerController::GetEscapeHud() const
{
	return Cast<AEscapeHUD>(GetHUD());
}

void AEscapePlayerController::ServerInteract_Implementation(const FString& TargetId)
{
	if (AEscapeGameMode* Mode = GetAuthMode(this)) { Mode->HandleInteract(this, TargetId); }
}

void AEscapePlayerController::ServerSubmitPassword_Implementation(const FString& Code)
{
	if (AEscapeGameMode* Mode = GetAuthMode(this)) { Mode->HandleSubmitPassword(this, Code); }
}

void AEscapePlayerController::ServerReturnToHuman_Implementation()
{
	if (AEscapeGameMode* Mode = GetAuthMode(this)) { Mode->HandleReturnToHuman(this); }
}

void AEscapePlayerController::ReceiveResult(EInteractKind Kind, const TArray<FString>& Messages,
	const TArray<FString>& EndingIds, const TArray<FString>& AchievementIds)
{
	if (!IsLocalController())
	{
		FEscapeOutcomeNet Net;
		Net.Kind = static_cast<uint8>(Kind);
		Net.Messages = Messages;
		Net.EndingIds = EndingIds;
		Net.AchievementIds = AchievementIds;
		ClientReceiveOutcome(Net);
		return;
	}

	AEscapeHUD* Hud = GetEscapeHud();
	if (!Hud) { return; }
	const FString Me = GetEscapePlayerId();

	FInteractOutcome Outcome;
	Outcome.Kind = Kind;
	Outcome.Messages = Messages;
	for (const FString& EndingId : EndingIds)
	{
		FTeamEvent Event;
		Event.EndingId = EndingId;
		Event.AffectedPlayers = { Me };
		Outcome.Events.Add(Event);
	}
	for (const FString& AchievementId : AchievementIds) { Outcome.NewAchievements.Add({ Me, AchievementId }); }

	if (Kind == EInteractKind::Resolved || Kind == EInteractKind::NeedsPassword)
	{
		Hud->GetPresenter().OnOutcome(Outcome, Me);
	}
	else
	{
		// 別人（或 Tick）造成的結局與成就：只有這兩類，沒有文字
		Hud->GetPresenter().OnTeamEvents(Outcome.Events, Me);
		Hud->GetPresenter().OnAchievements(Outcome.NewAchievements, Me);
	}
}

void AEscapePlayerController::ClientReceiveOutcome_Implementation(const FEscapeOutcomeNet& Net)
{
	ReceiveResult(static_cast<EInteractKind>(Net.Kind), Net.Messages, Net.EndingIds, Net.AchievementIds);
}

void AEscapePlayerController::KeypadKey(int32 Key)
{
	AEscapeHUD* Hud = GetEscapeHud();
	if (!Hud) { return; }
	FEscapeUiPresenter& Ui = Hud->GetPresenter();
	if (Key >= 0 && Key <= 9) { Ui.KeypadDigit(Key); }
	else if (Key == UEscapeKeypadWidget::KeyDelete) { Ui.KeypadDelete(); }
	else if (Key == UEscapeKeypadWidget::KeyConfirm)
	{
		FString Code;
		if (Ui.KeypadConfirm(Code)) { ServerSubmitPassword(Code); }
	}
}
