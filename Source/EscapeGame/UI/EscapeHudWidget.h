// 步驟 10 介面：常駐 HUD（左上：生命值、時間、形態）。C++ 用 WidgetTree 建，不做 WBP。
// 10a：空殼（Initialize 不建任何 widget）；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/EscapeUiPresenter.h"
#include "EscapeHudWidget.generated.h"

UCLASS()
class UEscapeHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 在這裡（不是 NativeOnInitialized：沒有 PlayerContext 時它不會被呼叫）用 WidgetTree 建好所有子 widget */
	virtual bool Initialize() override;

	/** 照 presenter 的 FHudView 更新三行字 */
	void SetView(const FHudView& View);
};
