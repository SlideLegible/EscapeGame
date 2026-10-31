// 步驟 10 介面：presenter 空殼（10a）。所有函式回傳預設值，10b 實作。
#include "UI/EscapeUiPresenter.h"

bool FEscapeUiPresenter::LoadTexts(const FString&, const FString&, const FString&, FString& OutError)
{
	OutError = TEXT("尚未實作");
	return false;
}

FHudView FEscapeUiPresenter::MakeHud(int32, int32, const FString&) const { return FHudView(); }
void FEscapeUiPresenter::OnOutcome(const FInteractOutcome&, const FString&) {}
void FEscapeUiPresenter::OnTeamEvents(const TArray<FTeamEvent>&, const FString&) {}
void FEscapeUiPresenter::OnAchievements(const TArray<FAchievementUnlock>&, const FString&) {}
void FEscapeUiPresenter::Tick(float) {}
void FEscapeUiPresenter::OnAnyKey() {}
bool FEscapeUiPresenter::IsPopupVisible() const { return false; }
FString FEscapeUiPresenter::GetPopupText() const { return FString(); }
bool FEscapeUiPresenter::IsKeypadVisible() const { return false; }
FString FEscapeUiPresenter::GetKeypadEntry() const { return FString(); }
void FEscapeUiPresenter::KeypadDigit(int32) {}
void FEscapeUiPresenter::KeypadDelete() {}
bool FEscapeUiPresenter::KeypadConfirm(FString&) { return false; }
void FEscapeUiPresenter::KeypadCancel() {}
bool FEscapeUiPresenter::IsEndingVisible() const { return false; }
FString FEscapeUiPresenter::GetEndingId() const { return FString(); }
FString FEscapeUiPresenter::GetEndingTitle() const { return FString(); }
FString FEscapeUiPresenter::GetEndingStory() const { return FString(); }
bool FEscapeUiPresenter::IsToastVisible() const { return false; }
FString FEscapeUiPresenter::GetToastText() const { return FString(); }
