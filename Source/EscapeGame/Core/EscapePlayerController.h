// 步驟 10 組合層：互動、輸入密碼、變回人類的入口。
// 做法：三個 Server RPC 在伺服器呼叫 AEscapeGameMode 的 Handle* → 動 FEscapeGameLogic → 把結果依玩家分送
// （AEscapeGameMode::Dispatch）→ 每位玩家的 controller 的 ReceiveResult：本機 controller（單機、listen server 的主機）直接餵
// 自己 HUD 的 presenter；遠端 controller 用 Client RPC（ClientReceiveOutcome，參數是 FEscapeOutcomeNet）送過去再餵。
#pragma once

#include "CoreMinimal.h"
#include "EscapeGamePlayerController.h"
#include "Logic/EscapeGameLogic.h"
#include "EscapePlayerController.generated.h"

class AEscapeHUD;

/** 送給遠端玩家的結果（只含這位玩家看得到的部分） */
USTRUCT()
struct FEscapeOutcomeNet
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Kind = 0;  // EInteractKind

	UPROPERTY()
	TArray<FString> Messages;

	/** 影響這位玩家的結局（E01～E05） */
	UPROPERTY()
	TArray<FString> EndingIds;

	/** 這位玩家新解鎖的成就 */
	UPROPERTY()
	TArray<FString> AchievementIds;
};

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

	/** 伺服器把結果交給這位玩家：本機直接進 presenter，遠端走 ClientReceiveOutcome */
	void ReceiveResult(EInteractKind Kind, const TArray<FString>& Messages, const TArray<FString>& EndingIds, const TArray<FString>& AchievementIds);

	UFUNCTION(Client, Reliable)
	void ClientReceiveOutcome(const FEscapeOutcomeNet& Net);

	/** 本機鍵盤的一個鍵：0～9、UEscapeKeypadWidget::KeyDelete、KeyConfirm（滑鼠點按鈕和步驟 11 的鍵盤輸入共用）。
	 *  確認且剛好 4 位 → ServerSubmitPassword。 */
	void KeypadKey(int32 Key);

	/** 這位玩家的 ID 字串（PlayerState 的 PlayerId）；沒有 PlayerState 回傳空字串 */
	FString GetEscapePlayerId() const;
	AEscapeHUD* GetEscapeHud() const;
};
