// 步驟 10 介面：互動事件文字的彈出框（畫面中下）。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Layout/Anchors.h"
#include "EscapePopupWidget.generated.h"

class UTextBlock;

UCLASS()
class UEscapePopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 文字最大寬度（USizeBox 只限寬度，高度不限）、字級、允許的最多行數（最長的事件文字在這個寬度自動換行後不可以超過） */
	static constexpr float MaxTextWidth = 720.0f;
	static constexpr int32 FontSize = 22;
	static constexpr int32 MaxLines = 6;

	/** HUD 把這個 widget 放到視口的哪裡（畫面中下）：AddToViewport 後 SetAnchorsInViewport／SetAlignmentInViewport 用這兩個值 */
	static FAnchors ViewportAnchors();
	static FVector2D ViewportAlignment();

	virtual bool Initialize() override;
	void SetPopupText(const FString& Text);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBlock;
};
