// 步驟 10 介面：常駐 HUD（左上：生命值、時間、形態）。C++ 用 WidgetTree 建，不做 WBP。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/EscapeUiPresenter.h"
#include "EscapeHudWidget.generated.h"

class UTextBlock;

UCLASS()
class UEscapeHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 FontSize = 26;

	/** 在這裡（不是 NativeOnInitialized：沒有 PlayerContext 時它不會被呼叫）用 WidgetTree 建好所有子 widget */
	virtual bool Initialize() override;

	/** 照 presenter 的 FHudView 更新三行字 */
	void SetView(const FHudView& View);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HpBlock;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeBlock;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FormBlock;
};
