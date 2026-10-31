// 步驟 3 生命值、步驟 4 時間：全隊共用狀態（純邏輯）
#include "Logic/TeamState.h"

FTeamState::FTeamState() {}

TArray<FTeamEvent> FTeamState::ApplyHpDelta(int32 Delta)
{
	TArray<FTeamEvent> Events;
	const int32 Before = Hp;
	Hp = FMath::Max(0, Hp + Delta);  // 沒有上限，最低 0
	if (Before > 0 && Hp == 0)
	{
		FTeamEvent E5;
		E5.EndingId = TEXT("E05");
		E5.AffectedPlayers = GetRemainingPlayers();
		Events.Add(MoveTemp(E5));

		// 重啟回到第一關：生命值 5、已觸發清空；成就與已逃出的玩家保留
		Hp = StartingHp;
		TriggeredIds.Empty();
		++RestartCount;
	}
	return Events;
}

// 步驟 4 還沒實作（紅）：先放空殼
TArray<FTeamEvent> FTeamState::Tick(float Dt) { return TArray<FTeamEvent>(); }
int32 FTeamState::GetRemainingSeconds() const { return 0; }
FString FTeamState::FormatClock(int32 Seconds) { return FString(); }
FTeamEvent FTeamState::EndRoundWithDawn() { return FTeamEvent(); }

void FTeamState::AddPlayer(const FString& PlayerId)
{
	if (!HasPlayer(PlayerId))
	{
		FPlayerEntry Entry;
		Entry.PlayerId = PlayerId;
		Players.Add(MoveTemp(Entry));
	}
}

bool FTeamState::HasPlayer(const FString& PlayerId) const
{
	return Players.ContainsByPredicate([&](const FPlayerEntry& P) { return P.PlayerId == PlayerId; });
}

bool FTeamState::MarkEscaped(const FString& PlayerId, const FString& EndingId)
{
	FPlayerEntry* P = Players.FindByPredicate([&](const FPlayerEntry& E) { return E.PlayerId == PlayerId; });
	if (!P || P->bEscaped) { return false; }
	P->bEscaped = true;
	P->EndingId = EndingId;
	return true;
}

bool FTeamState::HasEscaped(const FString& PlayerId) const
{
	const FPlayerEntry* P = Players.FindByPredicate([&](const FPlayerEntry& E) { return E.PlayerId == PlayerId; });
	return P && P->bEscaped;
}

FString FTeamState::GetEscapeEnding(const FString& PlayerId) const
{
	const FPlayerEntry* P = Players.FindByPredicate([&](const FPlayerEntry& E) { return E.PlayerId == PlayerId; });
	return (P && P->bEscaped) ? P->EndingId : FString();
}

TArray<FString> FTeamState::GetRemainingPlayers() const
{
	TArray<FString> Out;
	for (const FPlayerEntry& P : Players)
	{
		if (!P.bEscaped) { Out.Add(P.PlayerId); }
	}
	return Out;
}

TArray<FTeamEvent> FTeamState::ApplyInteraction(const FInteractionResult& Result, const FString& PlayerId)
{
	if (!Result.InteractionId.IsEmpty()) { TriggeredIds.Add(Result.InteractionId); }
	for (const FExtraTrigger& Extra : Result.ExtraTriggers)
	{
		if (!Extra.InteractionId.IsEmpty()) { TriggeredIds.Add(Extra.InteractionId); }
		for (const FString& A : Extra.AchievementIds) { UnlockedAchievements.Add(A); }
	}
	for (const FString& A : Result.AchievementIds) { UnlockedAchievements.Add(A); }

	// X017 的機率扣血（bHasHpChance）在步驟 5 才擲骰，這裡不處理。
	return ApplyHpDelta(Result.HpDelta);
}

FInteractionState FTeamState::MakeSnapshot() const
{
	FInteractionState S;
	S.TriggeredIds = TriggeredIds;
	return S;
}
