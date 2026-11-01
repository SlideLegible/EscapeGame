// 步驟 10 介面：結局畫面（全畫面深色底：標題＋劇情置中）。劇情文字在 MaxTextWidth 自動換行，不限高度。
#include "UI/EscapeEndingWidget.h"
#include "UI/EscapeUiBuild.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

bool UEscapeEndingWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree) { return false; }

	UBorder* Root = EscapeUiBuild::MakeBorder(WidgetTree, TEXT("EndingBorder"), FLinearColor(0.0f, 0.0f, 0.0f, 0.92f), FMargin(40.0f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;
	USizeBox* Limit = EscapeUiBuild::MakeWidthLimitedBox(WidgetTree, TEXT("EndingWidthLimit"), MaxTextWidth);
	Root->SetContent(Limit);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EndingColumn"));
	Limit->SetContent(Column);

	TitleBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("EndingTitle"), FontSize + 12, FString(), ETextJustify::Center);
	TitleBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.4f, 1.0f)));
	StoryBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("EndingStory"), FontSize);
	Column->AddChildToVerticalBox(TitleBlock)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	Column->AddChildToVerticalBox(StoryBlock);
	return true;
}

void UEscapeEndingWidget::SetEnding(const FString& Title, const FString& Story)
{
	if (TitleBlock) { TitleBlock->SetText(FText::FromString(Title)); }
	if (StoryBlock) { StoryBlock->SetText(FText::FromString(Story)); }
}
