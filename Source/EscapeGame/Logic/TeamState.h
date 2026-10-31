// 步驟 3 生命值：全隊共用狀態，純邏輯，只依賴 Core。
// 時間（步驟 4）、密碼（5）、變身（6）、場景旗標（7）、成就計數（8）、網路複製（13）都還沒做。
#pragma once

#include "CoreMinimal.h"
#include "Logic/InteractionResolver.h"

/** 一次狀態變化產生的全隊事件（目前只有生命值歸零的 E05） */
struct FTeamEvent
{
	FString EndingId;                // 例 "E05"
	TArray<FString> AffectedPlayers;  // 當下所有還沒逃出的玩家
};

class FTeamState
{
public:
	static constexpr int32 StartingHp = 5;

	FTeamState();

	// 生命值
	int32 GetHp() const { return Hp; }
	int32 GetRestartCount() const { return RestartCount; }

	/** 改變生命值：沒有上限、最低 0。從大於 0 變成 0 時觸發 E05 並重啟。 */
	TArray<FTeamEvent> ApplyHpDelta(int32 Delta);

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
	 *  bHasHpChance（X017）在步驟 5 才擲骰，這裡不處理。 */
	TArray<FTeamEvent> ApplyInteraction(const FInteractionResult& Result);

	const TSet<FString>& GetTriggeredIds() const { return TriggeredIds; }
	const TSet<FString>& GetUnlockedAchievements() const { return UnlockedAchievements; }

	/** 給 Resolver 用的狀態快照（目前只帶已觸發的 X） */
	FInteractionState MakeSnapshot() const;

private:
	struct FPlayerEntry
	{
		FString PlayerId;
		bool bEscaped = false;
		FString EndingId;
	};

	int32 Hp = StartingHp;
	int32 RestartCount = 0;
	TArray<FPlayerEntry> Players;
	TSet<FString> TriggeredIds;
	TSet<FString> UnlockedAchievements;
};
