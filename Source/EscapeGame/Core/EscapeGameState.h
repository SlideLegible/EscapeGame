// 步驟 10 組合層：給 HUD 的全隊狀態（生命值、剩餘秒數）。步驟 13 才處理網路複製。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EscapeGameState.generated.h"

UCLASS()
class AEscapeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Escape")
	int32 Hp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Escape")
	int32 RemainingSeconds = 0;
};
