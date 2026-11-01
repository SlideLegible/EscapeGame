// 步驟 10 組合層：伺服器持有 FEscapeGameLogic 的 GameMode（伺服器權威）。
// 注意：舊的 AEscapeGameGameMode（EscapeGameGameMode.h，ThirdPerson 範本留下的 abstract 類別）是另一個類別，不要混淆。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Logic/EscapeGameLogic.h"
#include "EscapeGameMode.generated.h"

class AEscapePlayerController;

UCLASS()
class AEscapeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEscapeGameMode();

	/** 從 Content/Data 讀五份 CSV（DT_Interactions、DT_Forms、DT_SceneObjects、DT_Responses、DT_Locations）建 FEscapeGameLogic。
	 *  成功 true；失敗 false 並寫 OutError。InitGame 會呼叫它；測試也可以直接呼叫。 */
	bool LoadLogic(FString& OutError);
	bool IsLogicLoaded() const { return bLogicLoaded; }
	FEscapeGameLogic& GetLogic() { return Logic; }
	const FEscapeGameLogic& GetLogic() const { return Logic; }

#if !UE_BUILD_SHIPPING
	/** 測試用：覆寫種子（要在 LoadLogic 之前呼叫）。預設種子是 FDateTime::Now().GetTicks()。 */
	void SetSeedForTesting(int32 Seed) { SeedOverride = Seed; bHasSeedOverride = true; }
#endif

	// ---- 三個 Server RPC 的本體（controller 呼叫）：動 logic → 同步場景 → 把結果分送給各玩家 ----
	FInteractOutcome HandleInteract(AEscapePlayerController* Controller, const FString& TargetId);
	FInteractOutcome HandleSubmitPassword(AEscapePlayerController* Controller, const FString& Code);
	bool HandleReturnToHuman(AEscapePlayerController* Controller);

	/** 把場景裡的 Actor 對齊 logic：還沒出現的 F 物品隱藏且無碰撞、出現的顯示；門依 IsDoorOpen 開關。 */
	void SyncSceneActors();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** 一次 logic 呼叫的結果依玩家分送：行動的玩家拿到全部（文字、鍵盤…），所有玩家拿到影響自己的結局與自己的成就 */
	void Dispatch(const FString& ActorPlayerId, const FInteractOutcome& Outcome);
	/** GameState 的生命值與剩餘秒數、每位玩家 PlayerState 的形態 */
	void SyncGameState();
	static FString PlayerIdOf(const AEscapePlayerController* Controller);

	FEscapeGameLogic Logic;
	bool bLogicLoaded = false;
	int32 SeedOverride = 0;
	bool bHasSeedOverride = false;

	/** PlayerId（PlayerState 的 GetPlayerId 轉字串）→ controller */
	TMap<FString, TWeakObjectPtr<AEscapePlayerController>> Controllers;
};
