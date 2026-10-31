// 步驟 3 生命值、步驟 4 時間、步驟 5 大門密碼：全隊共用狀態（純邏輯）
#include "Logic/TeamState.h"

namespace
{
	// 設計文件把 1031 當紅鯡魚（老師生日），所以大門密碼絕不是它
	constexpr int32 RedHerringCode = 1031;
	constexpr int32 PasswordCount = 10000;
}

FTeamState::FTeamState() : FTeamState(0) {}

FTeamState::FTeamState(int32 Seed) : Stream(Seed)
{
	GeneratePassword();
}

void FTeamState::GeneratePassword()
{
	// 在 0～9998 均勻抽一個，>= 1031 的加 1：剛好跳過 1031，其餘 9999 個密碼機率相同，不用重抽迴圈
	int32 Code = Stream.RandRange(0, PasswordCount - 2);
	if (Code >= RedHerringCode) { ++Code; }
	Password = FString::Printf(TEXT("%04d"), Code);
}

void FTeamState::StartNewRound()
{
	Flags.Empty();
	GeneratePassword();
}

EPasswordInput FTeamState::CheckPassword(const FString& Code) const
{
	return Code == Password ? EPasswordInput::Correct : EPasswordInput::Wrong;
}

void FTeamState::AddFlag(const FString& Flag)
{
	if (!Flag.IsEmpty()) { Flags.Add(Flag); }
}

bool FTeamState::HasFlag(const FString& Flag) const
{
	return Flags.Contains(Flag);
}

#if !UE_BUILD_SHIPPING
FString FTeamState::GetPasswordForDevOnly() const
{
	return Password;
}

void FTeamState::StartNewRoundForTesting()
{
	StartNewRound();
}
#endif

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
		// 重啟＝新的一局：倒數回到 30:00、旗標清空、重新產生密碼
		ElapsedSeconds = 0.0;
		bDawnTriggered = false;
		StartNewRound();
	}
	return Events;
}

namespace
{
	// 無條件進位前先扣掉的容差：1/60 秒累加 108000 次的浮點誤差遠小於這個值
	constexpr double ClockEpsilon = 1e-4;
}

TArray<FTeamEvent> FTeamState::Tick(float Dt)
{
	TArray<FTeamEvent> Events;
	if (bDawnTriggered || Dt <= 0.0f) { return Events; }
	// 有玩家、而且全都結束了 → 不倒數
	if (Players.Num() > 0 && GetRemainingPlayers().Num() == 0) { return Events; }

	ElapsedSeconds += static_cast<double>(Dt);
	if (GetRemainingSeconds() == 0)
	{
		Events.Add(EndRoundWithDawn());
	}
	return Events;
}

int32 FTeamState::GetRemainingSeconds() const
{
	const double Remaining = static_cast<double>(RoundSeconds) - ElapsedSeconds;
	return FMath::Clamp(static_cast<int32>(FMath::CeilToDouble(Remaining - ClockEpsilon)), 0, RoundSeconds);
}

FString FTeamState::FormatClock(int32 Seconds)
{
	Seconds = FMath::Max(0, Seconds);
	return FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60);
}

FTeamEvent FTeamState::EndRoundWithDawn()
{
	FTeamEvent E4;
	E4.EndingId = TEXT("E04");
	E4.AffectedPlayers = GetRemainingPlayers();  // 已經結束的玩家不會再收到
	for (const FString& Id : E4.AffectedPlayers) { MarkEscaped(Id, E4.EndingId); }
	bDawnTriggered = true;  // 這一局的 E04 已發生：Tick 不再倒數，也不會再有 E04
	return E4;
}

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

TArray<FTeamEvent> FTeamState::ApplyInteraction(const FInteractionResult& Result, const FString& PlayerId, bool* OutChanceHit)
{
	if (OutChanceHit) { *OutChanceHit = false; }
	if (!Result.InteractionId.IsEmpty()) { TriggeredIds.Add(Result.InteractionId); }
	for (const FExtraTrigger& Extra : Result.ExtraTriggers)
	{
		if (!Extra.InteractionId.IsEmpty()) { TriggeredIds.Add(Extra.InteractionId); }
		for (const FString& A : Extra.AchievementIds) { UnlockedAchievements.Add(A); }
	}
	for (const FString& A : Result.AchievementIds) { UnlockedAchievements.Add(A); }

	TArray<FTeamEvent> Events;
	// X021 這類 E04：同時間到。這一局的 E04 只發生一次。
	if (Result.EndingId == TEXT("E04") && !bDawnTriggered)
	{
		Events.Add(EndRoundWithDawn());
	}
	// E01～E03：只有這位玩家結束（玩家空、不存在、已結束 → 沒有事件）
	if ((Result.EndingId == TEXT("E01") || Result.EndingId == TEXT("E02") || Result.EndingId == TEXT("E03"))
		&& !PlayerId.IsEmpty() && MarkEscaped(PlayerId, Result.EndingId))
	{
		FTeamEvent Ending;
		Ending.EndingId = Result.EndingId;
		Ending.AffectedPlayers.Add(PlayerId);
		Events.Add(MoveTemp(Ending));
	}
	Events.Append(ApplyHpDelta(Result.HpDelta));
	// X017 這類機率扣血：用內部亂數擲骰，擲中才扣
	if (Result.bHasHpChance && Result.HpChanceDenominator > 0)
	{
		const bool bHit = Stream.RandHelper(Result.HpChanceDenominator) < Result.HpChanceNumerator;
		if (OutChanceHit) { *OutChanceHit = bHit; }
		if (bHit) { Events.Append(ApplyHpDelta(Result.HpChanceDelta)); }
	}
	return Events;
}

FInteractionState FTeamState::MakeSnapshot() const
{
	FInteractionState S;
	S.TriggeredIds = TriggeredIds;
	S.Flags = Flags;
	return S;
}
