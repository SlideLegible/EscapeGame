// 步驟 10 介面：彈出框 widget 空殼（10a），10b 實作。
#include "UI/EscapePopupWidget.h"

FAnchors UEscapePopupWidget::ViewportAnchors() { return FAnchors(0.0f, 0.0f); }
FVector2D UEscapePopupWidget::ViewportAlignment() { return FVector2D(0.0, 0.0); }
bool UEscapePopupWidget::Initialize() { return Super::Initialize(); }
void UEscapePopupWidget::SetPopupText(const FString&) {}
