// 步驟 5 大門密碼、步驟 6 變身、步驟 7 場景狀態：組合層。UE 端只跟它說話。純邏輯，只依賴 Core。
// 成就計數（8）還沒做。
#pragma once

#include "CoreMinimal.h"
#include "Logic/InteractionResolver.h"
#include "Logic/TeamState.h"

enum class EInteractKind : uint8
{
	Rejected,       // 玩家不存在或已結束：什麼都不做
	Blocked,        // 被擋下（步驟 6：物品被別人占用）
	NeedsPassword,  // 要輸入密碼，狀態沒改
	Resolved,       // 互動成立，狀態已更新
};

struct FInteractOutcome
{
	EInteractKind Kind = EInteractKind::Rejected;
	FInteractionResult Result;
	TArray<FString> Messages;  // 依序顯示的文字
	TArray<FTeamEvent> Events;
	bool bHpChanceHit = false;
	// 步驟 6：變身。bTransformed 為 true 時 Result.bIsDefault 一定是 false（碰可變身物品的預設變身不是嘲諷）；
	// 真的預設嘲諷是 Result.bIsDefault == true。
	bool bTransformed = false;
	FString FromForm;           // 變身前的形態（例 F00）
	FString ToForm;             // 變身後的形態
	FString EnteredLocationId;  // 這次互動進入的特殊地點（L01／L02），沒有進入則為空
};

class FEscapeGameLogic
{
public:
	/** 載入五份 CSV 文字。機率扣血的列（X017）文字必須剛好有兩段「…」，否則回傳 false 並寫出互動 ID。 */
	static bool Load(const FString& InteractionsCsv, const FString& FormsCsv, const FString& SceneObjectsCsv,
		const FString& ResponsesCsv, const FString& LocationsCsv, int32 Seed, FEscapeGameLogic& Out, FString& OutError);

	/** 玩家出生點也是 E05 重啟後回去的位置；人類身體一開始就在出生點 */
	void AddPlayer(const FString& PlayerId, const FVector& Spawn);

	// 位置（步驟 6）。Id 是 F 或 S；F 的註冊位置也是 E05 重啟時回去的位置。
	void RegisterObjectLocation(const FString& Id, const FVector& Location);
	/** 移動物件（F 或 S）。物品被玩家占用時，玩家位置不跟著動。 */
	void SetObjectLocation(const FString& Id, const FVector& Location);
	/** 移動玩家；玩家目前的物品形態跟著動，人類身體不動 */
	void SetPlayerLocation(const FString& PlayerId, const FVector& Location);

	FInteractOutcome Interact(const FString& PlayerId, const FString& TargetId);
	/** 在大門輸入四位數密碼。玩家不是人類形態（F00）→ Rejected */
	FInteractOutcome SubmitPassword(const FString& PlayerId, const FString& Code);

	/** 變回人類：回到人類身體的位置，離開特殊地點，放開物品（留在原地；在特殊地點裡就放在入口）。
	 *  已是人類、玩家不存在、已結束 → false。 */
	bool ReturnToHuman(const FString& PlayerId);

	TArray<FTeamEvent> Tick(float Dt);

	// 形態與位置的查詢（玩家不存在回傳空字串／零向量／false）
	FString GetPlayerForm(const FString& PlayerId) const;
	FVector GetPlayerLocation(const FString& PlayerId) const;
	FVector GetBodyLocation(const FString& PlayerId) const;
	/** "" 或 L01／L02 */
	FString GetPlayerLocationId(const FString& PlayerId) const;
	/** 形態的 Ability 含「整張地圖」（監視器） */
	bool CanSeeWholeMap(const FString& PlayerId) const;
	FVector GetItemLocation(const FString& ItemId) const;
	/** 物品已經出現在場景中 */
	bool IsItemPresent(const FString& ItemId) const;
	/** 目前占用（正在當它）的玩家，沒有則為空 */
	FString GetItemOccupant(const FString& ItemId) const;
	/** 物品所在的特殊地點："" 或 L01／L02 */
	FString GetItemLocationId(const FString& ItemId) const;

	// 場景狀態（步驟 7）：全隊共用，來源是 Team 的已觸發 X；E05 重啟（新的一局）才會清掉。
	/** 該門的開門互動（S19: X036、S05: X005、S09: X013）已觸發。不在門表的 ID（含 S01 大門）一律 false。 */
	bool IsDoorOpen(const FString& DoorId) const;
	/** 目前已開的門，依門表順序（給 UE 端同步） */
	TArray<FString> GetOpenDoors() const;
	/** 總電閘（X022）已拉下：不可逆，只有 E05 重啟才恢復。停電後 SubmitPassword 一律當輸錯。 */
	bool IsPowerCut() const;

	const FTeamState& GetTeam() const { return Team; }
	const FInteractionResolver& GetResolver() const { return Resolver; }

private:
	struct FPlayerInfo
	{
		FString PlayerId;
		FString FormId = TEXT("F00");
		FString LocationId;  // "" 或 L01／L02
		FVector Location = FVector::ZeroVector;
		FVector BodyLocation = FVector::ZeroVector;  // 人類身體
		FVector SpawnLocation = FVector::ZeroVector;
	};

	/** 一個可變身的物品（DT_Forms 的 F01～F12）：靜態資料加目前狀態 */
	struct FItemState
	{
		FString ItemId;
		bool bStartsPresent = false;  // DT_Forms 的 Acquisition 是「一開始就在」
		FString HomeLocationId;       // DT_Forms 的 Location 等於某個特殊地點的名稱時，那個地點
		bool bSeesWholeMap = false;   // Ability 含「整張地圖」
		bool bExists = false;
		FVector Location = FVector::ZeroVector;
		FVector InitialLocation = FVector::ZeroVector;
		FString OccupiedBy;
		FString LocationId;
	};

	struct FLocationInfo
	{
		FString LocationId;
		FString EntranceId;
		TArray<FString> AllowedForms;
	};

	const FPlayerInfo* FindPlayer(const FString& PlayerId) const;
	FPlayerInfo* FindPlayer(const FString& PlayerId);
	const FItemState* FindItem(const FString& ItemId) const;
	FItemState* FindItem(const FString& ItemId);
	const FLocationInfo* FindLocation(const FString& LocationId) const;
	/** 場景物件（S）的位置；沒有註冊回傳 Fallback */
	FVector GetObjectLocation(const FString& Id, const FVector& Fallback) const;
	/** 玩家離開物品形態：放開物品；在特殊地點裡就放在入口並離開地點，否則留在玩家目前位置 */
	void ReleasePlayerForm(FPlayerInfo& Player);
	/** 結局事件：結束的玩家放開物品；E05 → 重啟 */
	void HandleEvents(const TArray<FTeamEvent>& Events);
	void RestartRound();
	FInteractOutcome Apply(const FString& PlayerId, const FInteractionResult& Result);

	FInteractionResolver Resolver;
	FTeamState Team;
	FString PasswordTargetId;  // 密碼鎖的對象（S01），從 Resolver 資料找
	TArray<FPlayerInfo> Players;
	TArray<FItemState> Items;
	TArray<FLocationInfo> Locations;
	TSet<FString> KnownIds;                  // DT_Forms 與 DT_SceneObjects 的 ID
	TMap<FString, FVector> ObjectLocations;  // 場景物件（S）的位置
};
