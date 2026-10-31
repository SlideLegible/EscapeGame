// 步驟 10 組合層：互動、輸入密碼、變回人類的入口。10a：空殼（RPC 本體是空的）；10b 實作。
// 10b 另外加「結果送回本機」的 Client RPC（參數形式由 10b 決定，但要經過 controller 再進 HUD 的 presenter）。
#pragma once

#include "CoreMinimal.h"
#include "EscapeGamePlayerController.h"
#include "EscapePlayerController.generated.h"

UCLASS()
class AEscapePlayerController : public AEscapeGamePlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void ServerInteract(const FString& TargetId);

	UFUNCTION(Server, Reliable)
	void ServerSubmitPassword(const FString& Code);

	UFUNCTION(Server, Reliable)
	void ServerReturnToHuman();
};
