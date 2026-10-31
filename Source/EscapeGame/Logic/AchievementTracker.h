// 步驟 8 成就：每位玩家各自的已解鎖成就與計數。純邏輯，只依賴 Core。
// 全部只在記憶體：沒有存檔、沒有檔案、沒有設定寫入（BRIEF：成就不寫入永久儲存；關閉或重新整理就清空）。
#pragma once

#include "CoreMinimal.h"

class FAchievementTracker
{
public:
	// 搞怪成就的 ID 與門檻（設計文件 §8）
	static constexpr const TCHAR* TauntAchievementId = TEXT("A01");     // 被嘲笑專家：預設嘲諷第 10 次
	static constexpr const TCHAR* TransformAchievementId = TEXT("A02"); // 我是誰：累計變身第 20 次
	static constexpr const TCHAR* SoulAchievementId = TEXT("A03");      // 魂不附體：連續處於物品形態超過 10 分鐘
	static constexpr const TCHAR* KeyRingAchievementId = TEXT("A07");   // 鑰匙收藏家（來自互動資料 X034）
	static constexpr int32 TauntThreshold = 10;
	static constexpr int32 TransformThreshold = 20;
	static constexpr double ItemFormSecondsThreshold = 600.0;  // 要「大於」才解鎖
	/** 1/60 秒累加的浮點誤差遠小於這個值（600 秒約 3e-4），遠小於一格 0.0167 秒：600 秒整不算超過、再多一格才算 */
	static constexpr double SecondsTolerance = 1e-3;

	// 解鎖
	/** 第一次解鎖回傳 true；已經解鎖過（或玩家、成就 ID 是空的）回傳 false */
	bool Unlock(const FString& PlayerId, const FString& AchievementId);
	bool IsUnlocked(const FString& PlayerId, const FString& AchievementId) const;
	/** 依解鎖順序 */
	TArray<FString> GetUnlocked(const FString& PlayerId) const;

	// A01：預設嘲諷次數
	/** 次數 +1，回傳新的次數 */
	int32 AddTaunt(const FString& PlayerId);
	int32 GetTauntCount(const FString& PlayerId) const;
	bool HasReachedTauntThreshold(const FString& PlayerId) const;

	// A02：變成物品的次數（不含變回人類）
	int32 AddTransform(const FString& PlayerId);
	int32 GetTransformCount(const FString& PlayerId) const;
	bool HasReachedTransformThreshold(const FString& PlayerId) const;

	// A03：連續處於物品形態的秒數（double 累計）
	void AddItemFormSeconds(const FString& PlayerId, double Seconds);
	void ResetItemFormSeconds(const FString& PlayerId);
	double GetItemFormSeconds(const FString& PlayerId) const;
	/** 大於 600 秒（加極小容差） */
	bool HasExceededItemFormThreshold(const FString& PlayerId) const;

	// A07：鑰匙圈碰過的門
	void AddTouchedDoor(const FString& PlayerId, const FString& DoorId);
	TSet<FString> GetTouchedDoors(const FString& PlayerId) const;

private:
	struct FRecord
	{
		TArray<FString> Unlocked;  // 解鎖順序
		int32 TauntCount = 0;
		int32 TransformCount = 0;
		double ItemFormSeconds = 0.0;
		TSet<FString> TouchedDoors;
	};

	const FRecord* Find(const FString& PlayerId) const { return Records.Find(PlayerId); }

	TMap<FString, FRecord> Records;
};
