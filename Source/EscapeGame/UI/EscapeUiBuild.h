// 步驟 10 介面：五個 widget 共用的小工具（WidgetTree 建 UTextBlock／UBorder／USizeBox）。
// 規則：所有 UTextBlock 自動換行、不用省略號、用 FEscapeFonts 的字型；USizeBox 只限最大寬度，不設任何高度；不設裁切。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/EscapeFonts.h"

namespace EscapeUiBuild
{
	inline UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, int32 FontSize, const FString& Initial = FString(),
		ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
		Text->SetFont(FEscapeFonts::GetFont(FontSize));
		Text->SetAutoWrapText(true);
		Text->SetTextOverflowPolicy(ETextOverflowPolicy::Clip);  // 不是省略號；自動換行下文字不會超出寬度
		Text->SetJustification(Justify);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetText(FText::FromString(Initial));
		return Text;
	}

	inline UBorder* MakeBorder(UWidgetTree* Tree, const TCHAR* Name, const FLinearColor& Background, const FMargin& Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name));
		Border->SetBrushColor(Background);
		Border->SetPadding(Padding);
		return Border;
	}

	/** 只限最大寬度（高度完全不限） */
	inline USizeBox* MakeWidthLimitedBox(UWidgetTree* Tree, const TCHAR* Name, float MaxWidth)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(Name));
		Box->SetMaxDesiredWidth(MaxWidth);
		return Box;
	}

	/** 由 Initialize() 呼叫：Super 回傳 false（已經初始化過）或沒有 WidgetTree 就不再建 */
	inline UTextBlock* FindText(UWidgetTree* Tree, const TCHAR* Name)
	{
		return Tree ? Cast<UTextBlock>(Tree->FindWidget(FName(Name))) : nullptr;
	}
}
