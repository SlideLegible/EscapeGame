// 步驟 10 介面：四位數密碼鍵盤（0～9、刪除、確認、目前輸入）。10a：空殼；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeKeypadWidget.generated.h"

UCLASS()
class UEscapeKeypadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	/** 顯示目前輸入的數字（presenter 的 GetKeypadEntry） */
	void SetEntry(const FString& Entry);
};
