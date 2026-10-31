// 步驟 10 組合層：伺服器持有 FEscapeGameLogic 的 GameMode。10a：空殼（LoadLogic 回傳 false）；10b 實作。
// 注意：舊的 AEscapeGameGameMode（EscapeGameGameMode.h，ThirdPerson 範本留下的 abstract 類別）是另一個類別，不要混淆。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Logic/EscapeGameLogic.h"
#include "EscapeGameMode.generated.h"

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

private:
	FEscapeGameLogic Logic;
	bool bLogicLoaded = false;
	int32 SeedOverride = 0;
	bool bHasSeedOverride = false;
};
