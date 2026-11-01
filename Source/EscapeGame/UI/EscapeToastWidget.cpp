// 步驟 10 介面：成就提示（右上角）。
#include "UI/EscapeToastWidget.h"
#include "UI/EscapeUiBuild.h"

FAnchors UEscapeToastWidget::ViewportAnchors() { return FAnchors(1.0f, 0.0f, 1.0f, 0.0f); }
FVector2D UEscapeToastWidget::ViewportAlignment() { return FVector2D(1.0, 0.0); }

bool UEscapeToastWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree) { return false; }

	UBorder* Root = EscapeUiBuild::MakeBorder(WidgetTree, TEXT("ToastBorder"), FLinearColor(0.10f, 0.08f, 0.02f, 0.85f), FMargin(16.0f, 10.0f));
	WidgetTree->RootWidget = Root;
	USizeBox* Limit = EscapeUiBuild::MakeWidthLimitedBox(WidgetTree, TEXT("ToastWidthLimit"), MaxTextWidth);
	Root->SetContent(Limit);
	TextBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("ToastText"), FontSize);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.4f, 1.0f)));
	Limit->SetContent(TextBlock);
	return true;
}

void UEscapeToastWidget::SetToastText(const FString& Text)
{
	if (TextBlock) { TextBlock->SetText(FText::FromString(Text)); }
}
