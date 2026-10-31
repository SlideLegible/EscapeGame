// 步驟 10 介面：成就提示（右上角）。10a：空殼；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Layout/Anchors.h"
#include "EscapeToastWidget.generated.h"

UCLASS()
class UEscapeToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 提示文字的最大寬度、字級、允許的最多行數（最長的成就名稱加前綴在這個寬度自動換行後不可以超過） */
	static constexpr float MaxTextWidth = 420.0f;
	static constexpr int32 FontSize = 20;
	static constexpr int32 MaxLines = 3;

	/** HUD 把這個 widget 放到視口的哪裡（右上角）：AddToViewport 後 SetAnchorsInViewport／SetAlignmentInViewport 用這兩個值 */
	static FAnchors ViewportAnchors();
	static FVector2D ViewportAlignment();

	virtual bool Initialize() override;
	/** 顯示「成就解鎖：名稱」之類的一行（前綴來自 UiText ToastPrefix，由呼叫端組好） */
	void SetToastText(const FString& Text);
};
