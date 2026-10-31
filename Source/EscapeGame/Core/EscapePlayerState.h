// 步驟 10 組合層：每位玩家目前的形態。10a：空殼；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "EscapePlayerState.generated.h"

UCLASS()
class AEscapePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** 例 F00 */
	UPROPERTY(BlueprintReadOnly, Category = "Escape")
	FString FormId;

	/** 例 人類（DT_Forms 的 FormName） */
	UPROPERTY(BlueprintReadOnly, Category = "Escape")
	FString FormName;
};
