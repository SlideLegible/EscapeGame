// 步驟 10 介面：HUD widget。左上角三行：生命值、時間、形態。
#include "UI/EscapeHudWidget.h"
#include "UI/EscapeUiBuild.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

bool UEscapeHudWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree) { return false; }

	UBorder* Root = EscapeUiBuild::MakeBorder(WidgetTree, TEXT("HudBorder"), FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), FMargin(14.0f, 8.0f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudColumn"));
	Root->SetContent(Column);

	HpBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("HpText"), FontSize);
	TimeBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("TimeText"), FontSize);
	FormBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("FormText"), FontSize);
	for (UTextBlock* Block : { HpBlock.Get(), TimeBlock.Get(), FormBlock.Get() })
	{
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Block);
		Slot->SetPadding(FMargin(0.0f, 2.0f));
	}
	return true;
}

void UEscapeHudWidget::SetView(const FHudView& View)
{
	if (HpBlock) { HpBlock->SetText(FText::FromString(View.HpText)); }
	if (TimeBlock) { TimeBlock->SetText(FText::FromString(View.TimeText)); }
	if (FormBlock) { FormBlock->SetText(FText::FromString(View.FormText)); }
}
