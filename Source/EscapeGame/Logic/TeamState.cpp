// 步驟 3：空殼（測試先紅）
#include "Logic/TeamState.h"

FTeamState::FTeamState() { Hp = 0; }
TArray<FTeamEvent> FTeamState::ApplyHpDelta(int32) { return {}; }
void FTeamState::AddPlayer(const FString&) {}
bool FTeamState::HasPlayer(const FString&) const { return false; }
bool FTeamState::MarkEscaped(const FString&, const FString&) { return false; }
bool FTeamState::HasEscaped(const FString&) const { return false; }
FString FTeamState::GetEscapeEnding(const FString&) const { return FString(); }
TArray<FString> FTeamState::GetRemainingPlayers() const { return {}; }
TArray<FTeamEvent> FTeamState::ApplyInteraction(const FInteractionResult&) { return {}; }
FInteractionState FTeamState::MakeSnapshot() const { return FInteractionState(); }
