// 步驟 10 介面：HUD。建五個 widget（HUD 常駐，其餘依 presenter 顯示／隱藏）。
#include "UI/EscapeHUD.h"
#include "UI/EscapeHudWidget.h"
#include "UI/EscapePopupWidget.h"
#include "UI/EscapeKeypadWidget.h"
#include "UI/EscapeEndingWidget.h"
#include "UI/EscapeToastWidget.h"
#include "Core/EscapeGameState.h"
#include "Core/EscapePlayerState.h"
#include "Core/EscapePlayerController.h"
#include "EscapeGame.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// 疊放順序：高的在上面。成就提示在最上面（結局畫面出現時也看得到）
	constexpr int32 ZHud = 0;
	constexpr int32 ZPopup = 10;
	constexpr int32 ZKeypad = 20;
	constexpr int32 ZEnding = 100;
	constexpr int32 ZToast = 200;

	FString LoadDataText(const TCHAR* Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	template <typename T>
	T* AddWidget(APlayerController* Owner, int32 ZOrder, const FAnchors* Anchors, const FVector2D* Alignment, const FVector2D& Position)
	{
		T* Widget = CreateWidget<T>(Owner, T::StaticClass());
		if (!Widget) { return nullptr; }
		Widget->AddToViewport(ZOrder);
		if (Anchors) { Widget->SetAnchorsInViewport(*Anchors); }
		if (Alignment) { Widget->SetAlignmentInViewport(*Alignment); }
		if (Anchors) { Widget->SetPositionInViewport(Position, false); }
		return Widget;
	}
}

AEscapeHUD::AEscapeHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEscapeHUD::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	bTextsLoaded = Presenter.LoadTexts(LoadDataText(TEXT("DT_UiText.csv")), LoadDataText(TEXT("DT_Endings.csv")), LoadDataText(TEXT("DT_Achievements.csv")), Error);
	if (!bTextsLoaded) { UE_LOG(LogEscapeGame, Error, TEXT("載入介面文字失敗：%s"), *Error); }

	APlayerController* Owner = GetOwningPlayerController();
	if (!Owner) { return; }

	// 左上角（從視口左上角內縮 16）
	const FAnchors TopLeft(0.0f, 0.0f, 0.0f, 0.0f);
	const FVector2D TopLeftAlign(0.0, 0.0);
	HudWidget = AddWidget<UEscapeHudWidget>(Owner, ZHud, &TopLeft, &TopLeftAlign, FVector2D(16.0, 16.0));

	// 畫面中下（離視口底邊 48）
	const FAnchors PopupAnchors = UEscapePopupWidget::ViewportAnchors();
	const FVector2D PopupAlign = UEscapePopupWidget::ViewportAlignment();
	PopupWidget = AddWidget<UEscapePopupWidget>(Owner, ZPopup, &PopupAnchors, &PopupAlign, FVector2D(0.0, -48.0));

	// 正中央
	const FAnchors Center(0.5f, 0.5f, 0.5f, 0.5f);
	const FVector2D CenterAlign(0.5, 0.5);
	KeypadWidget = AddWidget<UEscapeKeypadWidget>(Owner, ZKeypad, &Center, &CenterAlign, FVector2D::ZeroVector);
	if (KeypadWidget)
	{
		KeypadWidget->OnKey.BindUObject(this, &AEscapeHUD::KeypadKeyPressed);
		KeypadWidget->SetLabels(Presenter.GetUiText(TEXT("KeypadTitle")), Presenter.GetUiText(TEXT("KeypadDelete")), Presenter.GetUiText(TEXT("KeypadConfirm")));
	}

	// 全畫面（預設錨點）
	EndingWidget = AddWidget<UEscapeEndingWidget>(Owner, ZEnding, nullptr, nullptr, FVector2D::ZeroVector);

	// 右上角（離視口右上角內縮 16）
	const FAnchors ToastAnchors = UEscapeToastWidget::ViewportAnchors();
	const FVector2D ToastAlign = UEscapeToastWidget::ViewportAlignment();
	ToastWidget = AddWidget<UEscapeToastWidget>(Owner, ZToast, &ToastAnchors, &ToastAlign, FVector2D(-16.0, 16.0));

	RefreshWidgets();
}

void AEscapeHUD::SetShown(UUserWidget* Widget, bool bShown, ESlateVisibility ShownVisibility)
{
	if (!Widget) { return; }
	const ESlateVisibility Wanted = bShown ? ShownVisibility : ESlateVisibility::Collapsed;
	if (Widget->GetVisibility() != Wanted) { Widget->SetVisibility(Wanted); }
}

void AEscapeHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Presenter.Tick(DeltaSeconds);
	RefreshWidgets();
}

void AEscapeHUD::RefreshWidgets()
{
	if (HudWidget)
	{
		const UWorld* World = GetWorld();
		const AEscapeGameState* State = World ? World->GetGameState<AEscapeGameState>() : nullptr;
		const AEscapePlayerState* PlayerState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AEscapePlayerState>() : nullptr;
		HudWidget->SetView(Presenter.MakeHud(State ? State->Hp : 0, State ? State->RemainingSeconds : 0, PlayerState ? PlayerState->FormName : FString()));
	}

	const bool bPopup = Presenter.IsPopupVisible();
	SetShown(PopupWidget, bPopup, ESlateVisibility::HitTestInvisible);
	if (PopupWidget && bPopup) { PopupWidget->SetPopupText(Presenter.GetPopupText()); }

	const bool bKeypad = Presenter.IsKeypadVisible();
	SetShown(KeypadWidget, bKeypad, ESlateVisibility::Visible);
	if (KeypadWidget && bKeypad) { KeypadWidget->SetEntry(Presenter.GetKeypadEntry()); }

	const bool bEnding = Presenter.IsEndingVisible();
	SetShown(EndingWidget, bEnding, ESlateVisibility::HitTestInvisible);
	if (EndingWidget && bEnding) { EndingWidget->SetEnding(Presenter.GetEndingTitle(), Presenter.GetEndingStory()); }

	const bool bToast = Presenter.IsToastVisible();
	SetShown(ToastWidget, bToast, ESlateVisibility::HitTestInvisible);
	if (ToastWidget && bToast) { ToastWidget->SetToastText(Presenter.GetUiText(TEXT("ToastPrefix")) + Presenter.GetToastText()); }
}

void AEscapeHUD::KeypadKeyPressed(int32 Key)
{
	if (AEscapePlayerController* Controller = Cast<AEscapePlayerController>(GetOwningPlayerController()))
	{
		Controller->KeypadKey(Key);
	}
}
