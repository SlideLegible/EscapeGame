// 步驟 3 生命值、步驟 4 時間、步驟 5 大門密碼：全隊共用狀態，純邏輯，只依賴 Core。
// 變身（6）、場景狀態（7）、成就計數（8）、網路複製（13）都還沒做。
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
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

	/** 無參數版本：固定種子 0（舊測試用） */
	FTeamState();
	/** 亂數（密碼、機率扣血）都從這個種子來，結果固定 */
	explicit FTeamState(int32 Seed);

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
	 *  E01～E03：只有 PlayerId 標成結束並回傳事件（PlayerId 空、不存在或已結束就沒有事件）。
	 *  bHasHpChance（X017）：用內部亂數擲 RandHelper(Den) < Num，擲中才套 HpChanceDelta；
	 *  OutChanceHit 回報有沒有擲中（沒有機率事件時為 false）。 */
	TArray<FTeamEvent> ApplyInteraction(const FInteractionResult& Result, const FString& PlayerId = FString(), bool* OutChanceHit = nullptr);

	// 大門密碼（步驟 5）：每局一組 0000～9999 的四位數，不是 1031。Shipping 沒有任何途徑讀出密碼。
	/** 等於當局密碼 → Correct；其他（包括不是 4 位數字）→ Wrong */
	EPasswordInput CheckPassword(const FString& Code) const;

	// 旗標（例：Tried1031）；E05 重啟清空
	void AddFlag(const FString& Flag);
	bool HasFlag(const FString& Flag) const;
	const TSet<FString>& GetFlags() const { return Flags; }

#if !UE_BUILD_SHIPPING
	/** 只給開發版測試模式（步驟 12）與自動化測試 */
	FString GetPasswordForDevOnly() const;
	/** 測試用：開新的一局的密碼與旗標（不動生命值、時間） */
	void StartNewRoundForTesting();
#endif

	const TSet<FString>& GetTriggeredIds() const { return TriggeredIds; }
	const TSet<FString>& GetUnlockedAchievements() const { return UnlockedAchievements; }

	/** 給 Resolver 用的狀態快照（已觸發的 X、旗標） */
	FInteractionState MakeSnapshot() const;

private:
	/** 新的一局：重新產生密碼、清空旗標 */
	void StartNewRound();
	void GeneratePassword();

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
	TSet<FString> Flags;
	FRandomStream Stream;
	FString Password;  // 當局密碼：只存在這裡，不 log、不複製
};
