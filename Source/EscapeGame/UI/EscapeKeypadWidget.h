// 步驟 10 介面：四位數密碼鍵盤（0～9、刪除、確認、目前輸入）。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeKeypadWidget.generated.h"

class UButton;
class UTextBlock;
class UEscapeKeypadWidget;

/** 按下的鍵：0～9 是數字，其餘是 KeyDelete／KeyConfirm */
DECLARE_DELEGATE_OneParam(FEscapeKeypadKeyDelegate, int32 /*Key*/);

/** 一顆按鈕的點擊轉接（UButton 的 OnClicked 只能綁 UFUNCTION，所以每顆按鈕一個小物件記住自己的鍵） */
UCLASS()
class UEscapeKeypadButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	int32 Key = 0;
	TWeakObjectPtr<UEscapeKeypadWidget> Owner;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class UEscapeKeypadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 FontSize = 28;
	static constexpr int32 KeyDelete = -1;
	static constexpr int32 KeyConfirm = -2;

	virtual bool Initialize() override;

	/** 顯示目前輸入的數字（presenter 的 GetKeypadEntry）；沒輸入的位數用底線補到 4 位 */
	void SetEntry(const FString& Entry);
	/** 標題與「刪除」「確認」的字（來自 UiText；Initialize 先放預設，UiText 載入後 HUD 會覆寫） */
	void SetLabels(const FString& Title, const FString& DeleteLabel, const FString& ConfirmLabel);

	/** 滑鼠點了某一顆按鈕 */
	FEscapeKeypadKeyDelegate OnKey;

	void HandleButton(int32 Key);

private:
	UButton* AddKey(class UUniformGridPanel* Grid, int32 Key, const TCHAR* Name, const FString& Label, int32 Row, int32 Column);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleBlock;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryBlock;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DeleteBlock;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConfirmBlock;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UEscapeKeypadButtonProxy>> Proxies;
};
