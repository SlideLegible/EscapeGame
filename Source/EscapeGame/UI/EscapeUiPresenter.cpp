// 步驟 10 介面：presenter。純 C++（Core + Logic/），不碰 UMG／Slate；計時與鍵盤狀態都在這裡，所以不用開畫面就能測。
#include "UI/EscapeUiPresenter.h"
#include "Serialization/Csv/CsvParser.h"

namespace
{
	/** 浮點誤差容差：dt 是 float，累加 2.9f + 0.1f 之類可能差一點點 */
	constexpr double TimerEpsilon = 1e-4;

	/** 第一欄當 key：key → (欄名 → 值)。表頭或資料列欄位對不上回傳 false。 */
	bool ParseTable(const FString& Csv, const TCHAR* TableName, const TArray<FString>& RequiredColumns,
		TMap<FString, TMap<FString, FString>>& Out, FString& OutError)
	{
		FCsvParser Parser(Csv);
		const FCsvParser::FRows& Rows = Parser.GetRows();
		if (Rows.Num() < 2)
		{
			OutError = FString::Printf(TEXT("%s 是空的或只有表頭"), TableName);
			return false;
		}
		TArray<FString> Header;
		for (const TCHAR* Cell : Rows[0]) { Header.Add(Cell); }
		for (const FString& Column : RequiredColumns)
		{
			if (!Header.Contains(Column))
			{
				OutError = FString::Printf(TEXT("%s 缺少欄位 %s"), TableName, *Column);
				return false;
			}
		}
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			if (Rows[R].Num() == 0) { continue; }
			TMap<FString, FString> Row;
			for (int32 C = 0; C < Header.Num() && C < Rows[R].Num(); ++C) { Row.Add(Header[C], Rows[R][C]); }
			if (Row.Num() < Header.Num())
			{
				OutError = FString::Printf(TEXT("%s 第 %d 列欄位不足"), TableName, R + 1);
				return false;
			}
			Out.Add(Rows[R][0], Row);
		}
		return true;
	}
}

bool FEscapeUiPresenter::LoadTexts(const FString& UiTextCsv, const FString& EndingsCsv, const FString& AchievementsCsv, FString& OutError)
{
	OutError.Empty();
	TMap<FString, TMap<FString, FString>> UiTable, EndingTable, AchievementTable;
	if (!ParseTable(UiTextCsv, TEXT("DT_UiText"), { TEXT("Name"), TEXT("Text") }, UiTable, OutError)) { return false; }
	if (!ParseTable(EndingsCsv, TEXT("DT_Endings"), { TEXT("Name"), TEXT("Title"), TEXT("Story") }, EndingTable, OutError)) { return false; }
	if (!ParseTable(AchievementsCsv, TEXT("DT_Achievements"), { TEXT("Name"), TEXT("Title") }, AchievementTable, OutError)) { return false; }

	static const TCHAR* const RequiredUiNames[] = {
		TEXT("HudHp"), TEXT("HudForm"), TEXT("KeypadTitle"), TEXT("KeypadDelete"), TEXT("KeypadConfirm"), TEXT("EndingContinue"), TEXT("ToastPrefix") };
	TMap<FString, FString> NewUi;
	for (const auto& Pair : UiTable) { NewUi.Add(Pair.Key, Pair.Value.FindRef(TEXT("Text"))); }
	for (const TCHAR* Name : RequiredUiNames)
	{
		const FString* Text = NewUi.Find(Name);
		if (!Text || Text->IsEmpty())
		{
			OutError = FString::Printf(TEXT("DT_UiText 缺少必要的文字：%s"), Name);
			return false;
		}
	}
	if (!NewUi[TEXT("HudHp")].Contains(TEXT("{0}"))) { OutError = TEXT("DT_UiText 的 HudHp 沒有 {0}"); return false; }
	if (!NewUi[TEXT("HudForm")].Contains(TEXT("{0}"))) { OutError = TEXT("DT_UiText 的 HudForm 沒有 {0}"); return false; }

	TMap<FString, FEndingText> NewEndings;
	for (const auto& Pair : EndingTable)
	{
		NewEndings.Add(Pair.Key, { Pair.Value.FindRef(TEXT("Title")), Pair.Value.FindRef(TEXT("Story")) });
	}
	TMap<FString, FString> NewTitles;
	for (const auto& Pair : AchievementTable) { NewTitles.Add(Pair.Key, Pair.Value.FindRef(TEXT("Title"))); }

	UiTexts = MoveTemp(NewUi);
	Endings = MoveTemp(NewEndings);
	AchievementTitles = MoveTemp(NewTitles);
	return true;
}

FString FEscapeUiPresenter::GetUiText(const FString& Name) const
{
	const FString* Text = UiTexts.Find(Name);
	return Text ? *Text : FString();
}

FHudView FEscapeUiPresenter::MakeHud(int32 Hp, int32 RemainingSeconds, const FString& FormDisplayName) const
{
	FHudView View;
	View.HpText = GetUiText(TEXT("HudHp")).Replace(TEXT("{0}"), *FString::FromInt(Hp));
	View.TimeText = FTeamState::FormatClock(RemainingSeconds);
	View.FormText = GetUiText(TEXT("HudForm")).Replace(TEXT("{0}"), *FormDisplayName);
	return View;
}

void FEscapeUiPresenter::OnOutcome(const FInteractOutcome& Outcome, const FString& LocalPlayerId)
{
	switch (Outcome.Kind)
	{
	case EInteractKind::NeedsPassword:
		if (!bKeypadVisible)
		{
			bKeypadVisible = true;
			KeypadEntry.Empty();
		}
		break;
	case EInteractKind::Resolved:
		for (const FString& Message : Outcome.Messages)
		{
			if (Message.IsEmpty()) { continue; }
			if (PopupQueue.Num() == 0) { PopupElapsed = 0.0; }
			PopupQueue.Add(Message);
		}
		break;
	case EInteractKind::Blocked:
	case EInteractKind::Rejected:
	default:
		return;  // 什麼都不做
	}
	ShowEndingFor(Outcome.Events, LocalPlayerId);
	QueueToasts(Outcome.NewAchievements, LocalPlayerId);
}

void FEscapeUiPresenter::OnTeamEvents(const TArray<FTeamEvent>& Events, const FString& LocalPlayerId)
{
	ShowEndingFor(Events, LocalPlayerId);
}

void FEscapeUiPresenter::OnAchievements(const TArray<FAchievementUnlock>& Unlocks, const FString& LocalPlayerId)
{
	QueueToasts(Unlocks, LocalPlayerId);
}

void FEscapeUiPresenter::ShowEndingFor(const TArray<FTeamEvent>& Events, const FString& LocalPlayerId)
{
	if (bEndingVisible) { return; }  // 已經在看結局：不蓋掉
	for (const FTeamEvent& Event : Events)
	{
		if (!Event.AffectedPlayers.Contains(LocalPlayerId)) { continue; }
		const FEndingText* Text = Endings.Find(Event.EndingId);
		bEndingVisible = true;
		EndingId = Event.EndingId;
		EndingTitle = Text ? Text->Title : Event.EndingId;
		EndingStory = Text ? Text->Story : FString();
		return;
	}
}

void FEscapeUiPresenter::QueueToasts(const TArray<FAchievementUnlock>& Unlocks, const FString& LocalPlayerId)
{
	for (const FAchievementUnlock& Unlock : Unlocks)
	{
		if (Unlock.PlayerId != LocalPlayerId) { continue; }
		const FString* Title = AchievementTitles.Find(Unlock.AchievementId);
		if (!Title || Title->IsEmpty()) { continue; }
		if (ToastQueue.Num() == 0) { ToastElapsed = 0.0; }
		ToastQueue.Add(*Title);
	}
}

void FEscapeUiPresenter::AdvancePopup()
{
	if (PopupQueue.Num() > 0) { PopupQueue.RemoveAt(0); }
	PopupElapsed = 0.0;
}

void FEscapeUiPresenter::AdvanceToast()
{
	if (ToastQueue.Num() > 0) { ToastQueue.RemoveAt(0); }
	ToastElapsed = 0.0;
}

void FEscapeUiPresenter::Tick(float Dt)
{
	if (Dt <= 0.0f) { return; }
	// 每一則只算「自己顯示的時間」：這一格的多餘時間不帶給下一則（每則都顯示滿 3 秒）
	if (PopupQueue.Num() > 0)
	{
		PopupElapsed += Dt;
		if (PopupElapsed >= PopupSeconds - TimerEpsilon) { AdvancePopup(); }
	}
	if (ToastQueue.Num() > 0)
	{
		ToastElapsed += Dt;
		if (ToastElapsed >= ToastSeconds - TimerEpsilon) { AdvanceToast(); }
	}
}

void FEscapeUiPresenter::OnAnyKey()
{
	if (bKeypadVisible) { return; }
	if (PopupQueue.Num() > 0) { AdvancePopup(); }
}

bool FEscapeUiPresenter::IsPopupVisible() const { return PopupQueue.Num() > 0; }
FString FEscapeUiPresenter::GetPopupText() const { return PopupQueue.Num() > 0 ? PopupQueue[0] : FString(); }

bool FEscapeUiPresenter::IsKeypadVisible() const { return bKeypadVisible; }
FString FEscapeUiPresenter::GetKeypadEntry() const { return KeypadEntry; }

void FEscapeUiPresenter::KeypadDigit(int32 Digit)
{
	if (!bKeypadVisible || Digit < 0 || Digit > 9 || KeypadEntry.Len() >= KeypadDigits) { return; }
	KeypadEntry.AppendChar(static_cast<TCHAR>(TEXT('0') + Digit));
}

void FEscapeUiPresenter::KeypadDelete()
{
	if (!bKeypadVisible || KeypadEntry.IsEmpty()) { return; }
	KeypadEntry.LeftChopInline(1);
}

bool FEscapeUiPresenter::KeypadConfirm(FString& OutCode)
{
	if (!bKeypadVisible || KeypadEntry.Len() != KeypadDigits) { return false; }
	OutCode = KeypadEntry;
	KeypadEntry.Empty();
	bKeypadVisible = false;
	return true;
}

void FEscapeUiPresenter::KeypadCancel()
{
	bKeypadVisible = false;
	KeypadEntry.Empty();
}

bool FEscapeUiPresenter::IsEndingVisible() const { return bEndingVisible; }
FString FEscapeUiPresenter::GetEndingId() const { return EndingId; }
FString FEscapeUiPresenter::GetEndingTitle() const { return EndingTitle; }
FString FEscapeUiPresenter::GetEndingStory() const { return EndingStory; }

bool FEscapeUiPresenter::IsToastVisible() const { return ToastQueue.Num() > 0; }
FString FEscapeUiPresenter::GetToastText() const { return ToastQueue.Num() > 0 ? ToastQueue[0] : FString(); }
