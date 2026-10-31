// 步驟 3 生命值、步驟 4 時間：全隊共用狀態，純邏輯，只依賴 Core。
// 密碼（5）、變身（6）、場景旗標（7）、成就計數（8）、網路複製（13）都還沒做。
#pragma once

#include "CoreMinimal.h"
#include "Logic/InteractionResolver.h"

/** 一次狀態變化產生的全隊事件（生命值歸零的 E05、時間到或時鐘自身互動的 E04） */
struct FTeamEvent
{
	FString EndingId;                // 例 "E05"、"E04"
	TArray<FString> AffectedPlayers;  // 當下所有還沒逃出的玩家
};

class FTeamState
{
public:
	static constexpr int32 StartingHp = 5;
	static constexpr int32 RoundSeconds = 1800;  // 一局 30 分鐘

	FTeamState();

	// 生命值
	int32 GetHp() const { return Hp; }
	int32 GetRestartCount() const { return RestartCount; }

	/** 改變生命值：沒有上限、最低 0。從大於 0 變成 0 時觸發 E05 並重啟。 */
	TArray<FTeamEvent> ApplyHpDelta(int32 Delta);

	// 時間
	/** 倒數 Dt 秒。還沒結束的玩家都結束時（至少有一位玩家）不倒數。
	 *  剩餘時間跑到 0 → 回傳一次 E04（受影響＝還沒結束的玩家），之後時鐘停在 0。 */
	TArray<FTeamEvent> Tick(float Dt);
	/** 剩餘整數秒：無條件進位（加極小容差，避免 1/60 累加誤差） */
	int32 GetRemainingSeconds() const;
	/** 例 1800 → "30:00"、59 → "00:59" */
	static FString FormatClock(int32 Seconds);

	// 玩家名單
	void AddPlayer(const FString& PlayerId);
	bool HasPlayer(const FString& PlayerId) const;
	/** 標成已逃出並記下結局；玩家不存在或已逃出回傳 false */
	bool MarkEscaped(const FString& PlayerId, const FString& EndingId);
	bool HasEscaped(const FString& PlayerId) const;
	FString GetEscapeEnding(const FString& PlayerId) const;
	/** 還沒逃出的玩家，依加入順序 */
	TArray<FString> GetRemainingPlayers() const;

	// 互動結果
	/** 記錄已觸發（含 ExtraTriggers）、套用 HpDelta、加入成就，回傳這次發生的事件。
	 *  Result.EndingId == "E04"（X021）→ 同時間到：還沒結束的玩家全部結束，回傳 E04。
	 *  E01～E03（只有 PlayerId 結束）在步驟 5 才做。
	 *  bHasHpChance（X017）在步驟 5 才擲骰，這裡不處理。 */
	TArray<FTeamEvent> ApplyInteraction(const FInteractionResult& Result, const FString& PlayerId = FString());

	const TSet<FString>& GetTriggeredIds() const { return TriggeredIds; }
	const TSet<FString>& GetUnlockedAchievements() const { return UnlockedAchievements; }

	/** 給 Resolver 用的狀態快照（目前只帶已觸發的 X） */
	FInteractionState MakeSnapshot() const;

private:
	/** 把還沒結束的玩家全部標成結局 E04，回傳事件 */
	FTeamEvent EndRoundWithDawn();

	struct FPlayerEntry
	{
		FString PlayerId;
		bool bEscaped = false;
		FString EndingId;
	};

	int32 Hp = StartingHp;
	int32 RestartCount = 0;
	double ElapsedSeconds = 0.0;  // 這一局已經過的秒數（E05 重啟歸零）
	bool bDawnTriggered = false;  // 這一局的 E04 已發生，時鐘停在 0
	TArray<FPlayerEntry> Players;
	TSet<FString> TriggeredIds;
	TSet<FString> UnlockedAchievements;
};
