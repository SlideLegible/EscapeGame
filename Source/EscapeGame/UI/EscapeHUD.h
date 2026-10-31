// 步驟 10 介面：建立 HUD、彈出框、鍵盤、結局、成就提示五個 widget，每格讀 presenter 更新。10a：空殼；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/EscapeUiPresenter.h"
#include "EscapeHUD.generated.h"

class UEscapeHudWidget;
class UEscapePopupWidget;
class UEscapeKeypadWidget;
class UEscapeEndingWidget;
class UEscapeToastWidget;

UCLASS()
class AEscapeHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEscapeHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	/** 本機玩家的 presenter（由 HUD 持有；controller 把 logic 結果餵給它） */
	FEscapeUiPresenter& GetPresenter() { return Presenter; }
	const FEscapeUiPresenter& GetPresenter() const { return Presenter; }

private:
	FEscapeUiPresenter Presenter;

	UPROPERTY(Transient)
	TObjectPtr<UEscapeHudWidget> HudWidget;
	UPROPERTY(Transient)
	TObjectPtr<UEscapePopupWidget> PopupWidget;
	UPROPERTY(Transient)
	TObjectPtr<UEscapeKeypadWidget> KeypadWidget;
	UPROPERTY(Transient)
	TObjectPtr<UEscapeEndingWidget> EndingWidget;
	UPROPERTY(Transient)
	TObjectPtr<UEscapeToastWidget> ToastWidget;
};
