// 步驟 10 介面：建立 HUD、彈出框、鍵盤、結局、成就提示五個 widget，每格讀 presenter 更新。
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
class UUserWidget;

UCLASS()
class AEscapeHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEscapeHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 本機玩家的 presenter（由 HUD 持有；controller 把 logic 結果餵給它） */
	FEscapeUiPresenter& GetPresenter() { return Presenter; }
	const FEscapeUiPresenter& GetPresenter() const { return Presenter; }

private:
	void KeypadKeyPressed(int32 Key);
	/** 依 presenter 的狀態顯示／隱藏、更新文字 */
	void RefreshWidgets();
	static void SetShown(UUserWidget* Widget, bool bShown, ESlateVisibility ShownVisibility);

	FEscapeUiPresenter Presenter;
	bool bTextsLoaded = false;

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
