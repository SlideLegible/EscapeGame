// 步驟 10 介面：密碼鍵盤 widget。標題、目前輸入、3x4 按鈕（1～9、刪除、0、確認）。
#include "UI/EscapeKeypadWidget.h"
#include "UI/EscapeUiBuild.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UEscapeKeypadButtonProxy::HandleClicked()
{
	if (UEscapeKeypadWidget* Widget = Owner.Get()) { Widget->HandleButton(Key); }
}

UButton* UEscapeKeypadWidget::AddKey(UUniformGridPanel* Grid, int32 Key, const TCHAR* Name, const FString& Label, int32 Row, int32 Column)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
	UTextBlock* Text = EscapeUiBuild::MakeText(WidgetTree, *FString::Printf(TEXT("%sText"), Name), FontSize, Label, ETextJustify::Center);
	Button->SetContent(Text);
	UUniformGridSlot* Slot = Grid->AddChildToUniformGrid(Button, Row, Column);
	Slot->SetHorizontalAlignment(HAlign_Fill);
	Slot->SetVerticalAlignment(VAlign_Fill);

	UEscapeKeypadButtonProxy* Proxy = NewObject<UEscapeKeypadButtonProxy>(this);
	Proxy->Key = Key;
	Proxy->Owner = this;
	Proxies.Add(Proxy);
	Button->OnClicked.AddDynamic(Proxy, &UEscapeKeypadButtonProxy::HandleClicked);

	if (Key == KeyDelete) { DeleteBlock = Text; }
	if (Key == KeyConfirm) { ConfirmBlock = Text; }
	return Button;
}

bool UEscapeKeypadWidget::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (!WidgetTree) { return false; }

	UBorder* Root = EscapeUiBuild::MakeBorder(WidgetTree, TEXT("KeypadBorder"), FLinearColor(0.0f, 0.0f, 0.0f, 0.88f), FMargin(24.0f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("KeypadColumn"));
	Root->SetContent(Column);

	TitleBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("KeypadTitleText"), FontSize, TEXT("請輸入四位數密碼"), ETextJustify::Center);
	EntryBlock = EscapeUiBuild::MakeText(WidgetTree, TEXT("KeypadEntryText"), FontSize + 12, TEXT("____"), ETextJustify::Center);
	EntryBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.4f, 1.0f)));
	Column->AddChildToVerticalBox(TitleBlock)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	Column->AddChildToVerticalBox(EntryBlock)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("KeypadGrid"));
	Grid->SetSlotPadding(FMargin(4.0f));
	Grid->SetMinDesiredSlotWidth(96.0f);
	Grid->SetMinDesiredSlotHeight(64.0f);
	Column->AddChildToVerticalBox(Grid);
	for (int32 Digit = 1; Digit <= 9; ++Digit)
	{
		AddKey(Grid, Digit, *FString::Printf(TEXT("Key%d"), Digit), FString::FromInt(Digit), (Digit - 1) / 3, (Digit - 1) % 3);
	}
	AddKey(Grid, KeyDelete, TEXT("KeyDelete"), TEXT("刪除"), 3, 0);
	AddKey(Grid, 0, TEXT("Key0"), TEXT("0"), 3, 1);
	AddKey(Grid, KeyConfirm, TEXT("KeyConfirm"), TEXT("確認"), 3, 2);
	return true;
}

void UEscapeKeypadWidget::SetEntry(const FString& Entry)
{
	if (!EntryBlock) { return; }
	FString Shown = Entry;
	while (Shown.Len() < 4) { Shown.AppendChar(TEXT('_')); }
	EntryBlock->SetText(FText::FromString(Shown));
}

void UEscapeKeypadWidget::SetLabels(const FString& Title, const FString& DeleteLabel, const FString& ConfirmLabel)
{
	if (TitleBlock && !Title.IsEmpty()) { TitleBlock->SetText(FText::FromString(Title)); }
	if (DeleteBlock && !DeleteLabel.IsEmpty()) { DeleteBlock->SetText(FText::FromString(DeleteLabel)); }
	if (ConfirmBlock && !ConfirmLabel.IsEmpty()) { ConfirmBlock->SetText(FText::FromString(ConfirmLabel)); }
}

void UEscapeKeypadWidget::HandleButton(int32 Key)
{
	OnKey.ExecuteIfBound(Key);
}
