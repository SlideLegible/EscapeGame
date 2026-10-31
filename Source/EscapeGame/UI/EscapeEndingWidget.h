// 步驟 10 介面：結局畫面（全畫面：標題＋劇情）。10a：空殼；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeEndingWidget.generated.h"

UCLASS()
class UEscapeEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 劇情文字的最大寬度、字級、允許的最多行數（最長的結局劇情在這個寬度自動換行後不可以超過） */
	static constexpr float MaxTextWidth = 900.0f;
	static constexpr int32 FontSize = 24;
	static constexpr int32 MaxLines = 12;

	virtual bool Initialize() override;
	void SetEnding(const FString& Title, const FString& Story);
};
