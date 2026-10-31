// 步驟 10 介面：成就提示 widget 空殼（10a），10b 實作。
#include "UI/EscapeToastWidget.h"

FAnchors UEscapeToastWidget::ViewportAnchors() { return FAnchors(0.0f, 0.0f); }
FVector2D UEscapeToastWidget::ViewportAlignment() { return FVector2D(0.0, 0.0); }
bool UEscapeToastWidget::Initialize() { return Super::Initialize(); }
void UEscapeToastWidget::SetToastText(const FString&) {}
