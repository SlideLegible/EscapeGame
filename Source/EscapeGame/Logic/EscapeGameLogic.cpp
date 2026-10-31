// 步驟 5 大門密碼：組合層。Resolver 判定 → TeamState 套用 → 整理成要顯示的文字與事件。
#include "Logic/EscapeGameLogic.h"

namespace
{
	const TCHAR* const HumanForm = TEXT("F00");
	const TCHAR* const Tried1031Flag = TEXT("Tried1031");  // 與 Resolver 解析「曾在大門輸入 1031 且失敗」產生的旗標同名
	const TCHAR* const RedHerringCode = TEXT("1031");

	/** 依序取出文字裡每一段「…」的內文（不含括號）；括號不成對或巢狀時回傳 false */
	bool ExtractQuotedSegments(const FString& Text, TArray<FString>& Out)
	{
		Out.Reset();
		int32 Cursor = 0;
		while (Cursor < Text.Len())
		{
			const int32 Open = Text.Find(TEXT("「"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
			if (Open == INDEX_NONE) { break; }
			const int32 Close = Text.Find(TEXT("」"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
			if (Close == INDEX_NONE) { return false; }
			const int32 NextOpen = Text.Find(TEXT("「"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
			if (NextOpen != INDEX_NONE && NextOpen < Close) { return false; }
			Out.Add(Text.Mid(Open + 1, Close - Open - 1));
			Cursor = Close + 1;
		}
		// 多出來的「」」也算不成對
		int32 Closers = 0;
		for (TCHAR C : Text) { if (C == TEXT('」')) { ++Closers; } }
		return Closers == Out.Num();
	}
}

bool FEscapeGameLogic::Load(const FString& InteractionsCsv, const FString& FormsCsv, const FString& SceneObjectsCsv,
	const FString& ResponsesCsv, const FString& InLocationsCsv, int32 Seed, FEscapeGameLogic& Out, FString& OutError)
{
	FEscapeGameLogic NewLogic;
	if (!FInteractionResolver::Load(InteractionsCsv, FormsCsv, SceneObjectsCsv, ResponsesCsv, NewLogic.Resolver, OutError)) { return false; }

	for (const FInteractionRow& Row : NewLogic.Resolver.GetRows())
	{
		// 機率扣血的列：第一段是照常顯示的文字，第二段是擲中時另外顯示的文字
		if (Row.bHasHpChance)
		{
			TArray<FString> Segments;
			if (!ExtractQuotedSegments(Row.EventText, Segments) || Segments.Num() != 2)
			{
				OutError = FString::Printf(TEXT("%s：機率扣血的文字應剛好有兩段「…」（第一段輸錯、第二段擲中），實際：「%s」"), *Row.Id, *Row.EventText);
				return false;
			}
		}
		// 密碼鎖的對象：資料裡所有密碼鎖列必須是同一個對象（S01）
		if (Row.PasswordRequirement != EPasswordRequirement::NotRequired)
		{
			if (NewLogic.PasswordTargetId.IsEmpty()) { NewLogic.PasswordTargetId = Row.TargetId; }
			else if (NewLogic.PasswordTargetId != Row.TargetId)
			{
				OutError = FString::Printf(TEXT("%s：密碼鎖的對象 %s 和其他密碼鎖列的 %s 不同"), *Row.Id, *Row.TargetId, *NewLogic.PasswordTargetId);
				return false;
			}
		}
	}
	if (NewLogic.PasswordTargetId.IsEmpty())
	{
		OutError = TEXT("DT_Interactions：找不到密碼鎖的列（X016／X017）");
		return false;
	}

	NewLogic.Team = FTeamState(Seed);
	NewLogic.LocationsCsv = InLocationsCsv;  // 步驟 6 才解析
	Out = MoveTemp(NewLogic);
	OutError.Empty();
	return true;
}

void FEscapeGameLogic::AddPlayer(const FString& PlayerId, const FVector& Spawn)
{
	if (PlayerId.IsEmpty() || FindPlayer(PlayerId)) { return; }
	FPlayerInfo Info;
	Info.PlayerId = PlayerId;
	Info.Location = Spawn;
	Players.Add(MoveTemp(Info));
	Team.AddPlayer(PlayerId);
}

const FEscapeGameLogic::FPlayerInfo* FEscapeGameLogic::FindPlayer(const FString& PlayerId) const
{
	return Players.FindByPredicate([&](const FPlayerInfo& P) { return P.PlayerId == PlayerId; });
}

FInteractOutcome FEscapeGameLogic::Interact(const FString& PlayerId, const FString& TargetId)
{
	FInteractOutcome Outcome;  // 預設 Rejected
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	if (!Player || Team.HasEscaped(PlayerId)) { return Outcome; }

	FInteractionQuery Query;
	Query.FormId = Player->FormId;
	Query.TargetId = TargetId;
	Query.State = Team.MakeSnapshot();  // Password 維持 None
	const FInteractionResult Result = Resolver.Resolve(Query);
	if (Result.bNeedsPassword)
	{
		Outcome.Kind = EInteractKind::NeedsPassword;
		Outcome.Result = Result;
		return Outcome;
	}
	return Apply(PlayerId, Result);
}

FInteractOutcome FEscapeGameLogic::SubmitPassword(const FString& PlayerId, const FString& Code)
{
	FInteractOutcome Outcome;  // 預設 Rejected
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	if (!Player || Team.HasEscaped(PlayerId) || Player->FormId != HumanForm) { return Outcome; }

	FInteractionQuery Query;
	Query.FormId = Player->FormId;
	Query.TargetId = PasswordTargetId;
	Query.State = Team.MakeSnapshot();
	Query.State.Password = Team.CheckPassword(Code);
	const FInteractionResult Result = Resolver.Resolve(Query);
	if (Result.bNeedsPassword) { return Outcome; }  // 資料裡沒有對應的密碼結果列：不應發生

	// 試過 1031 又失敗 → 之後月曆多一行小字。先記旗標再套用：萬一這次輸錯剛好扣到 E05，新的一局就從沒旗標開始。
	if (Query.State.Password == EPasswordInput::Wrong && Code == RedHerringCode) { Team.AddFlag(Tried1031Flag); }
	return Apply(PlayerId, Result);
}

FInteractOutcome FEscapeGameLogic::Apply(const FString& PlayerId, const FInteractionResult& Result)
{
	FInteractOutcome Outcome;
	Outcome.Kind = EInteractKind::Resolved;
	Outcome.Result = Result;
	Outcome.Events = Team.ApplyInteraction(Result, PlayerId, &Outcome.bHpChanceHit);

	if (Result.bHasHpChance)
	{
		TArray<FString> Segments;
		ExtractQuotedSegments(Result.Text, Segments);  // Load 已檢查剛好兩段
		if (Segments.Num() > 0) { Outcome.Messages.Add(Segments[0]); }
		if (Outcome.bHpChanceHit && Segments.Num() > 1) { Outcome.Messages.Add(Segments[1]); }
	}
	else if (!Result.Text.IsEmpty())
	{
		Outcome.Messages.Add(Result.Text);
	}
	return Outcome;
}

TArray<FTeamEvent> FEscapeGameLogic::Tick(float Dt)
{
	return Team.Tick(Dt);
}
