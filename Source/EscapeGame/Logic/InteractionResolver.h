// 步驟 2 互動判定：純邏輯，只依賴 Core。不改任何狀態、不擲骰（機率只記錄）。
#pragma once

#include "CoreMinimal.h"

/** 玩家形態欄的規則 */
enum class EInteractionFormRule : uint8
{
	Exact,      // Fxx 精確符合
	AnyForm,    // 任一形態
	OtherForm,  // 其他任一形態：同一對象沒有精確列的形態
	NonHuman,   // 非人類形態：F00 以外
};

/** 互動對象欄的種類 */
enum class EInteractionTargetKind : uint8
{
	Id,               // Sxx 或 Fxx
	Self,             // （自身），查詢時對象 ID 為 "SELF"
	AnyDoor,          // 任何門（門的清單在 DoorIds）
	AllDoorsTouched,  // 碰過所有門（X034，只能當 AnyDoor 列的額外觸發）
};

enum class EInteractionConditionKind : uint8
{
	Triggered,           // Arg 這個 X 已觸發
	NotTriggered,        // Arg 這個 X 未觸發
	Flag,                // 具名旗標 Arg（例：Tried1031）
	AllDoorsTouchedVia,  // Arg（X033）碰過全部的門
};

/** 對象欄「（密碼鎖）」要求的密碼輸入結果 */
enum class EPasswordRequirement : uint8 { NotRequired, Correct, Wrong };

/** 查詢時的密碼輸入結果 */
enum class EPasswordInput : uint8 { None, Correct, Wrong };

struct FInteractionCondition
{
	EInteractionConditionKind Kind = EInteractionConditionKind::Triggered;
	FString Arg;
};

/** 一列互動資料：保留 CSV 原始文字，另有解析後的欄位 */
struct FInteractionRow
{
	// CSV 原始文字
	FString Id, RawPlayerForm, RawTarget, RawEventType, RawEventText, RawTransformResult;
	FString RawSpawnedItem, RawHpChange, RawAchievement, RawPrecondition, RawNotes;

	// 解析後
	EInteractionFormRule FormRule = EInteractionFormRule::Exact;
	FString FormId;  // FormRule == Exact 時有值
	EInteractionTargetKind TargetKind = EInteractionTargetKind::Id;
	FString TargetId;  // TargetKind == Id 時有值
	TArray<FString> DoorIds;  // TargetKind == AnyDoor 時有值
	TArray<FInteractionCondition> Conditions;  // 前置條件加對象欄括號限定
	EPasswordRequirement PasswordRequirement = EPasswordRequirement::NotRequired;

	// 結果
	FString EventText;
	int32 HpDelta = 0;
	bool bHasHpChance = false;
	int32 HpChanceNumerator = 0;
	int32 HpChanceDenominator = 0;
	int32 HpChanceDelta = 0;
	FString SpawnedItemId;
	FString TransformFormId;
	TArray<FString> AchievementIds;  // A 開頭
	FString EndingId;                // E 開頭
	int32 SourceOrder = 0;
};

/** 狀態快照 */
struct FInteractionState
{
	TSet<FString> TriggeredIds;  // 已觸發的 X ID
	TSet<FString> Flags;         // 具名旗標，例 Tried1031
	TSet<FString> TouchedDoors;  // 鑰匙圈已碰過的門
	EPasswordInput Password = EPasswordInput::None;
};

struct FInteractionQuery
{
	FString FormId;    // 玩家目前形態，例 F00
	FString TargetId;  // Sxx、Fxx，或 "SELF"
	FInteractionState State;
};

/** 額外觸發（例：X034 鑰匙收藏家） */
struct FExtraTrigger
{
	FString InteractionId;
	TArray<FString> AchievementIds;
	FString EndingId;
};

struct FInteractionResult
{
	FString InteractionId;  // 預設回應時為空
	bool bIsDefault = false;
	bool bNeedsPassword = false;  // 人類碰密碼鎖但還沒有輸入結果
	FString Text;
	int32 HpDelta = 0;
	bool bHasHpChance = false;  // 例 X017：只記錄機率，不擲骰
	int32 HpChanceNumerator = 0;
	int32 HpChanceDenominator = 0;
	int32 HpChanceDelta = 0;
	FString SpawnedItemId;
	FString TransformFormId;
	TArray<FString> AchievementIds;
	FString EndingId;
	TArray<FExtraTrigger> ExtraTriggers;
};

class FInteractionResolver
{
public:
	/** 從四份 CSV 文字載入。任何一列無法解析 → 回傳 false，OutError 寫出互動 ID 與原因。 */
	static bool Load(const FString& InteractionsCsv, const FString& FormsCsv, const FString& SceneObjectsCsv,
		const FString& ResponsesCsv, FInteractionResolver& OutResolver, FString& OutError);

	/** 純函式，不改任何狀態。優先序：鑰匙圈碰門 → 精確 → 其他／非人類 → 任一 → 預設。 */
	FInteractionResult Resolve(const FInteractionQuery& Query) const;

	/** ID → 名稱（形態名稱、場景物件名稱）；查不到回傳 ID 本身 */
	FString GetDisplayName(const FString& Id) const;

	const TArray<FInteractionRow>& GetRows() const { return Rows; }
	const TArray<FString>& GetDoorIds() const { return DoorIds; }

private:
	TArray<FInteractionRow> Rows;
	TArray<FString> DoorIds;
	TMap<FString, FString> Names;
	FString DefaultResponse;
};
