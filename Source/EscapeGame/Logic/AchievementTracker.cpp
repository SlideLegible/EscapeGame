// 步驟 8 成就：每位玩家各自的已解鎖與計數，全部只在記憶體（沒有存檔、檔案、設定寫入）
#include "Logic/AchievementTracker.h"

bool FAchievementTracker::Unlock(const FString& PlayerId, const FString& AchievementId)
{
	if (PlayerId.IsEmpty() || AchievementId.IsEmpty()) { return false; }
	FRecord& Record = Records.FindOrAdd(PlayerId);
	if (Record.Unlocked.Contains(AchievementId)) { return false; }
	Record.Unlocked.Add(AchievementId);
	return true;
}

bool FAchievementTracker::IsUnlocked(const FString& PlayerId, const FString& AchievementId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record && Record->Unlocked.Contains(AchievementId);
}

TArray<FString> FAchievementTracker::GetUnlocked(const FString& PlayerId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record ? Record->Unlocked : TArray<FString>();
}

int32 FAchievementTracker::AddTaunt(const FString& PlayerId)
{
	return ++Records.FindOrAdd(PlayerId).TauntCount;
}

int32 FAchievementTracker::GetTauntCount(const FString& PlayerId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record ? Record->TauntCount : 0;
}

bool FAchievementTracker::HasReachedTauntThreshold(const FString& PlayerId) const
{
	return GetTauntCount(PlayerId) >= TauntThreshold;
}

int32 FAchievementTracker::AddTransform(const FString& PlayerId)
{
	return ++Records.FindOrAdd(PlayerId).TransformCount;
}

int32 FAchievementTracker::GetTransformCount(const FString& PlayerId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record ? Record->TransformCount : 0;
}

bool FAchievementTracker::HasReachedTransformThreshold(const FString& PlayerId) const
{
	return GetTransformCount(PlayerId) >= TransformThreshold;
}

void FAchievementTracker::AddItemFormSeconds(const FString& PlayerId, double Seconds)
{
	if (Seconds > 0.0) { Records.FindOrAdd(PlayerId).ItemFormSeconds += Seconds; }
}

void FAchievementTracker::ResetItemFormSeconds(const FString& PlayerId)
{
	if (FRecord* Record = Records.Find(PlayerId)) { Record->ItemFormSeconds = 0.0; }
}

double FAchievementTracker::GetItemFormSeconds(const FString& PlayerId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record ? Record->ItemFormSeconds : 0.0;
}

bool FAchievementTracker::HasExceededItemFormThreshold(const FString& PlayerId) const
{
	// 要「大於」600 秒：加容差，600 秒整（含 1/60 累加誤差）不算，再多一格才算
	return GetItemFormSeconds(PlayerId) > ItemFormSecondsThreshold + SecondsTolerance;
}

void FAchievementTracker::AddTouchedDoor(const FString& PlayerId, const FString& DoorId)
{
	if (!PlayerId.IsEmpty() && !DoorId.IsEmpty()) { Records.FindOrAdd(PlayerId).TouchedDoors.Add(DoorId); }
}

TSet<FString> FAchievementTracker::GetTouchedDoors(const FString& PlayerId) const
{
	const FRecord* Record = Find(PlayerId);
	return Record ? Record->TouchedDoors : TSet<FString>();
}
