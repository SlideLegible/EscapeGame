// 步驟 10 介面：互動事件文字的彈出框（畫面中下）。USizeBox 只限最大寬度；文字自動換行、高度隨內容長。
#include "UI/EscapePopupWidget.h"
#include "UI/EscapeUiBuild.h"

FAnchors UEscapePopupWidget::ViewportAnchors() { return FAnchors(0.5f, 1.0f, 0.5f, 1.0f); }
FVector2D UEscapePopupWidget::ViewportAlignment() { return FVector2D(0.5, 1.0); }

bool UEscapePopupWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree) { return false; }

	UBorder* Root = EscapeUiBuild::MakeBorder(WidgetTree, TEXT("PopupBorder"), FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), FMargin(20.0f, 14.0f));
	WidgetTree->RootWidget = Root;
	USizeBox* Limit = EscapeUiBuild::MakeWidthLimitedBox(WidgetTree, TEXT("PopupWidthLimit"), MaxTextWidth);
	Root->SetContent(Limit);
	TextBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("PopupText"), FontSize);
	Limit->SetContent(TextBlock);
	return true;
}

void UEscapePopupWidget::SetPopupText(const FString& Text)
{
	if (TextBlock) { TextBlock->SetText(FText::FromString(Text)); }
}
