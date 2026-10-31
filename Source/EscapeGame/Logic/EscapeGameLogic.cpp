// 步驟 5 大門密碼、步驟 6 變身：組合層。Resolver 判定 → 形態／位置 → TeamState 套用 → 整理成要顯示的文字與事件。
#include "Logic/EscapeGameLogic.h"
#include "Serialization/Csv/CsvParser.h"

namespace
{
	const TCHAR* const HumanForm = TEXT("F00");
	const TCHAR* const SelfTarget = TEXT("SELF");  // 與 Resolver 的「（自身）」對象 ID 相同
	const TCHAR* const StartsPresentText = TEXT("一開始就在");  // DT_Forms 的 Acquisition
	const TCHAR* const WholeMapAbility = TEXT("整張地圖");      // DT_Forms 的 Ability（監視器）
	const TCHAR* const RandomMarker = TEXT("隨機");
	const TCHAR* const Tried1031Flag = TEXT("Tried1031");  // 與 Resolver 解析「曾在大門輸入 1031 且失敗」產生的旗標同名
	const TCHAR* const RedHerringCode = TEXT("1031");

	using FTable = TArray<TMap<FString, FString>>;

	/** 讀 CSV：第一列是表頭，必須有 RequiredColumns；欄位數不對或空 CSV → false 並寫出原因 */
	bool ParseTable(const FString& Text, const TArray<FString>& RequiredColumns, FTable& Out, FString& Err, const TCHAR* Label)
	{
		FCsvParser Parser(Text);
		const FCsvParser::FRows& Rows = Parser.GetRows();
		if (Rows.Num() == 0) { Err = FString::Printf(TEXT("%s：空的 CSV"), Label); return false; }
		TSet<FString> Header;
		for (const TCHAR* H : Rows[0]) { Header.Add(FString(H)); }
		for (const FString& Col : RequiredColumns)
		{
			if (!Header.Contains(Col)) { Err = FString::Printf(TEXT("%s：缺少欄位 %s"), Label, *Col); return false; }
		}
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			if (Rows[R].Num() == 1 && FCString::Strlen(Rows[R][0]) == 0) { continue; }  // 空行
			if (Rows[R].Num() != Rows[0].Num())
			{
				Err = FString::Printf(TEXT("%s：第 %d 列欄位數 %d，應為 %d"), Label, R + 1, Rows[R].Num(), Rows[0].Num());
				return false;
			}
			TMap<FString, FString> Row;
			for (int32 C = 0; C < Rows[0].Num(); ++C) { Row.Add(FString(Rows[0][C]), FString(Rows[R][C])); }
			Out.Add(MoveTemp(Row));
		}
		return true;
	}

	/** 取出文字裡所有「Prefix 加兩位以上數字」的 ID（例 "F01 飲料罐垃圾" → F01；"S03 飲料機" → S03），前一個字不能是英數字 */
	TArray<FString> ExtractIds(const FString& Text, TCHAR Prefix)
	{
		TArray<FString> Ids;
		for (int32 I = 0; I < Text.Len(); ++I)
		{
			if (Text[I] != Prefix || (I > 0 && FChar::IsAlnum(Text[I - 1]))) { continue; }
			int32 End = I + 1;
			while (End < Text.Len() && FChar::IsDigit(Text[End])) { ++End; }
			if (End - I - 1 >= 2) { Ids.Add(Text.Mid(I, End - I)); }
			I = End - 1;
		}
		return Ids;
	}

	/** 文字裡的 [a～b 隨機] 換成用 Team 的亂數抽出的整數（骰子 X020） */
	FString ReplaceRandomTokens(const FString& Text, FTeamState& Team)
	{
		FString Out;
		int32 Cursor = 0;
		while (Cursor < Text.Len())
		{
			const int32 Open = Text.Find(TEXT("["), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
			if (Open == INDEX_NONE) { break; }
			const int32 Close = Text.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
			if (Close == INDEX_NONE) { break; }
			const FString Inner = Text.Mid(Open + 1, Close - Open - 1);
			const int32 Tilde = Inner.Find(TEXT("～"), ESearchCase::CaseSensitive);
			if (Tilde != INDEX_NONE && Inner.Contains(RandomMarker))
			{
				Out += Text.Mid(Cursor, Open - Cursor);
				Out += FString::FromInt(Team.RollRange(FCString::Atoi(*Inner.Left(Tilde)), FCString::Atoi(*Inner.Mid(Tilde + 1))));
			}
			else
			{
				Out += Text.Mid(Cursor, Close + 1 - Cursor);
			}
			Cursor = Close + 1;
		}
		Out += Text.Mid(Cursor);
		return Out;
	}

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

	// 步驟 6：形態（DT_Forms）、場景物件（DT_SceneObjects）、特殊地點（DT_Locations）
	FTable Forms, Objects, LocationRows;
	if (!ParseTable(FormsCsv, { TEXT("Name"), TEXT("Acquisition"), TEXT("Location"), TEXT("Ability") }, Forms, OutError, TEXT("DT_Forms"))) { return false; }
	if (!ParseTable(SceneObjectsCsv, { TEXT("Name") }, Objects, OutError, TEXT("DT_SceneObjects"))) { return false; }
	if (!ParseTable(InLocationsCsv, { TEXT("Name"), TEXT("LocationName"), TEXT("Entrance"), TEXT("AllowedForms") }, LocationRows, OutError, TEXT("DT_Locations"))) { return false; }

	for (const auto& Row : Objects) { NewLogic.KnownIds.Add(Row[TEXT("Name")]); }
	TMap<FString, FString> LocationIdByName;  // 地點名稱 → 地點 ID
	for (const auto& Row : LocationRows)
	{
		LocationIdByName.Add(Row[TEXT("LocationName")].TrimStartAndEnd(), Row[TEXT("Name")]);
	}
	for (const auto& Row : Forms)
	{
		const FString& Id = Row[TEXT("Name")];
		if (Id == HumanForm || !Id.StartsWith(TEXT("F"))) { continue; }  // 人類不是物品
		FItemState Item;
		Item.ItemId = Id;
		Item.bStartsPresent = Row[TEXT("Acquisition")].TrimStartAndEnd() == StartsPresentText;
		Item.bExists = Item.bStartsPresent;
		Item.bSeesWholeMap = Row[TEXT("Ability")].Contains(WholeMapAbility);
		if (const FString* Home = LocationIdByName.Find(Row[TEXT("Location")].TrimStartAndEnd())) { Item.HomeLocationId = *Home; }
		NewLogic.Items.Add(MoveTemp(Item));
		NewLogic.KnownIds.Add(Id);
	}
	for (const auto& Row : LocationRows)
	{
		FLocationInfo Info;
		Info.LocationId = Row[TEXT("Name")];
		const TArray<FString> Entrances = ExtractIds(Row[TEXT("Entrance")], TEXT('S'));
		if (Entrances.Num() != 1 || !NewLogic.KnownIds.Contains(Entrances[0]))
		{
			OutError = FString::Printf(TEXT("%s：入口「%s」要剛好是一個 DT_SceneObjects 裡的 S ID"), *Info.LocationId, *Row[TEXT("Entrance")]);
			return false;
		}
		Info.EntranceId = Entrances[0];
		Info.AllowedForms = ExtractIds(Row[TEXT("AllowedForms")], TEXT('F'));
		for (const FString& Form : Info.AllowedForms)
		{
			if (!NewLogic.FindItem(Form))
			{
				OutError = FString::Printf(TEXT("%s：可進入形態 %s 不在 DT_Forms"), *Info.LocationId, *Form);
				return false;
			}
		}
		NewLogic.Locations.Add(MoveTemp(Info));
	}
	for (const FInteractionRow& Row : NewLogic.Resolver.GetRows())
	{
		if (!Row.SpawnedItemId.IsEmpty() && !NewLogic.FindItem(Row.SpawnedItemId))
		{
			OutError = FString::Printf(TEXT("%s：出現物品 %s 不在 DT_Forms"), *Row.Id, *Row.SpawnedItemId);
			return false;
		}
	}

	NewLogic.Team = FTeamState(Seed);
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
	Info.BodyLocation = Spawn;  // 人類身體一開始在出生點
	Info.SpawnLocation = Spawn;
	Players.Add(MoveTemp(Info));
	Team.AddPlayer(PlayerId);
}

const FEscapeGameLogic::FPlayerInfo* FEscapeGameLogic::FindPlayer(const FString& PlayerId) const
{
	return Players.FindByPredicate([&](const FPlayerInfo& P) { return P.PlayerId == PlayerId; });
}

FEscapeGameLogic::FPlayerInfo* FEscapeGameLogic::FindPlayer(const FString& PlayerId)
{
	return Players.FindByPredicate([&](const FPlayerInfo& P) { return P.PlayerId == PlayerId; });
}

const FEscapeGameLogic::FItemState* FEscapeGameLogic::FindItem(const FString& ItemId) const
{
	return Items.FindByPredicate([&](const FItemState& I) { return I.ItemId == ItemId; });
}

FEscapeGameLogic::FItemState* FEscapeGameLogic::FindItem(const FString& ItemId)
{
	return Items.FindByPredicate([&](const FItemState& I) { return I.ItemId == ItemId; });
}

const FEscapeGameLogic::FLocationInfo* FEscapeGameLogic::FindLocation(const FString& LocationId) const
{
	return Locations.FindByPredicate([&](const FLocationInfo& L) { return L.LocationId == LocationId; });
}

FVector FEscapeGameLogic::GetObjectLocation(const FString& Id, const FVector& Fallback) const
{
	if (const FItemState* Item = FindItem(Id)) { return Item->Location; }
	if (const FVector* Found = ObjectLocations.Find(Id)) { return *Found; }
	return Fallback;
}

// ---- 位置 ----

void FEscapeGameLogic::RegisterObjectLocation(const FString& Id, const FVector& Location)
{
	if (FItemState* Item = FindItem(Id))
	{
		Item->InitialLocation = Location;
		Item->Location = Location;
	}
	else
	{
		ObjectLocations.Add(Id, Location);
	}
}

void FEscapeGameLogic::SetObjectLocation(const FString& Id, const FVector& Location)
{
	if (FItemState* Item = FindItem(Id)) { Item->Location = Location; }
	else { ObjectLocations.Add(Id, Location); }
}

void FEscapeGameLogic::SetPlayerLocation(const FString& PlayerId, const FVector& Location)
{
	FPlayerInfo* Player = FindPlayer(PlayerId);
	if (!Player) { return; }
	Player->Location = Location;
	// 玩家就是那個物品：物品跟著走，人類身體不動
	if (FItemState* Item = FindItem(Player->FormId); Item && Item->OccupiedBy == PlayerId) { Item->Location = Location; }
}

// ---- 互動 ----

FInteractOutcome FEscapeGameLogic::Interact(const FString& PlayerId, const FString& TargetId)
{
	FInteractOutcome Outcome;  // 預設 Rejected
	FPlayerInfo* Player = FindPlayer(PlayerId);
	if (!Player || Team.HasEscaped(PlayerId)) { return Outcome; }

	// 對象：SELF，或 DT_Forms／DT_SceneObjects 裡的 ID（F00 不能當對象）
	const bool bSelf = TargetId == SelfTarget;
	FItemState* TargetItem = nullptr;
	if (!bSelf)
	{
		if (TargetId == HumanForm || !KnownIds.Contains(TargetId)) { return Outcome; }
		TargetItem = FindItem(TargetId);
	}
	if (TargetItem)
	{
		if (!TargetItem->bExists) { return Outcome; }  // 還沒出現
		if (!TargetItem->OccupiedBy.IsEmpty())
		{
			// 別人正在當它：什麼都不顯示、不改狀態；自己正在當它：不能互動
			if (TargetItem->OccupiedBy != PlayerId) { Outcome.Kind = EInteractKind::Blocked; }
			return Outcome;
		}
	}
	// 特殊地點：裡面只能碰這個地點的東西和自己；外面碰不到還在地點裡的東西
	if (!Player->LocationId.IsEmpty())
	{
		if (!bSelf && !(TargetItem && TargetItem->HomeLocationId == Player->LocationId)) { return Outcome; }
	}
	else if (TargetItem && !TargetItem->HomeLocationId.IsEmpty() && TargetItem->LocationId == TargetItem->HomeLocationId)
	{
		return Outcome;
	}

	FInteractionQuery Query;
	Query.FormId = Player->FormId;
	Query.TargetId = TargetId;
	Query.State = Team.MakeSnapshot();  // Password 維持 None
	FInteractionResult Result = Resolver.Resolve(Query);
	if (Result.bNeedsPassword)
	{
		Outcome.Kind = EInteractKind::NeedsPassword;
		Outcome.Result = Result;
		return Outcome;
	}

	const FVector TargetLocation = bSelf ? Player->Location : GetObjectLocation(TargetId, Player->Location);

	// 出現物品：第一次出現才產生，位置是這次互動對象的位置；已存在的不重產、不移動
	if (!Result.SpawnedItemId.IsEmpty())
	{
		FItemState* Spawned = FindItem(Result.SpawnedItemId);
		if (Spawned && !Spawned->bExists)
		{
			Spawned->bExists = true;
			Spawned->Location = TargetLocation;
			Spawned->LocationId = Spawned->HomeLocationId;
		}
	}

	// 進入特殊地點：形態在可進入名單、對象是入口、而且不是預設回應
	FString EnteredLocationId;
	if (!Result.bIsDefault && !bSelf && Player->LocationId.IsEmpty())
	{
		for (const FLocationInfo& Loc : Locations)
		{
			if (Loc.EntranceId == TargetId && Loc.AllowedForms.Contains(Player->FormId)) { EnteredLocationId = Loc.LocationId; break; }
		}
	}

	// 變身：Resolver 給的變身結果，或任何形態碰可變身物品（預設回應 → 變成它，不是嘲諷）
	FString ToForm = Result.TransformFormId;
	if (ToForm.IsEmpty() && TargetItem && Result.bIsDefault)
	{
		ToForm = TargetItem->ItemId;
		Result.bIsDefault = false;
		Result.Text.Empty();
		Result.TransformFormId = ToForm;
	}
	const FString FromForm = Player->FormId;
	bool bTransformed = false;
	FItemState* NewItem = ToForm.IsEmpty() || ToForm == FromForm ? nullptr : FindItem(ToForm);
	if (NewItem && NewItem->bExists && NewItem->OccupiedBy.IsEmpty())
	{
		const bool bWasInLocation = !Player->LocationId.IsEmpty();
		const FLocationInfo* Loc = FindLocation(Player->LocationId);
		const FVector Exit = Loc ? GetObjectLocation(Loc->EntranceId, Player->Location) : Player->Location;
		if (FromForm == HumanForm) { Player->BodyLocation = Player->Location; }  // 人類身體留在原地
		else { ReleasePlayerForm(*Player); }                                       // 舊物品留在原地（在地點裡就放在入口）
		if (bWasInLocation)
		{
			// 帶著新物品離開特殊地點
			NewItem->Location = Exit;
			NewItem->LocationId.Empty();
		}
		NewItem->OccupiedBy = PlayerId;
		Player->FormId = ToForm;
		Player->Location = NewItem->Location;
		bTransformed = true;
	}
	if (!bTransformed && !EnteredLocationId.IsEmpty()) { Player->LocationId = EnteredLocationId; }

	Result.Text = ReplaceRandomTokens(Result.Text, Team);

	Outcome = Apply(PlayerId, Result);
	Outcome.bTransformed = bTransformed;
	if (bTransformed)
	{
		Outcome.FromForm = FromForm;
		Outcome.ToForm = ToForm;
	}
	else
	{
		Outcome.EnteredLocationId = EnteredLocationId;
	}
	return Outcome;
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
	HandleEvents(Outcome.Events);

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
	TArray<FTeamEvent> Events = Team.Tick(Dt);
	HandleEvents(Events);
	return Events;
}

// ---- 形態 ----

void FEscapeGameLogic::ReleasePlayerForm(FPlayerInfo& Player)
{
	if (Player.FormId == HumanForm) { return; }
	FItemState* Item = FindItem(Player.FormId);
	if (!Item || Item->OccupiedBy != Player.PlayerId) { return; }
	Item->OccupiedBy.Empty();
	if (!Player.LocationId.IsEmpty())
	{
		// 設計文件沒寫清楚的地方：在特殊地點裡放開物品，物品放在入口、不留在地點裡（否則飲料罐垃圾會卡在飲料機裡）
		const FLocationInfo* Loc = FindLocation(Player.LocationId);
		const FVector Exit = Loc ? GetObjectLocation(Loc->EntranceId, Player.Location) : Player.Location;
		Item->Location = Exit;
		Item->LocationId.Empty();
		Player.Location = Exit;
		Player.LocationId.Empty();
	}
	else
	{
		Item->Location = Player.Location;
	}
}

bool FEscapeGameLogic::ReturnToHuman(const FString& PlayerId)
{
	FPlayerInfo* Player = FindPlayer(PlayerId);
	if (!Player || Team.HasEscaped(PlayerId) || Player->FormId == HumanForm) { return false; }
	ReleasePlayerForm(*Player);
	Player->FormId = HumanForm;
	Player->LocationId.Empty();
	Player->Location = Player->BodyLocation;
	return true;
}

void FEscapeGameLogic::HandleEvents(const TArray<FTeamEvent>& Events)
{
	bool bRestart = false;
	for (const FTeamEvent& Event : Events)
	{
		if (Event.EndingId == TEXT("E05")) { bRestart = true; continue; }
		// 結束的玩家放開物品，物品留在當時的位置
		for (const FString& Id : Event.AffectedPlayers)
		{
			if (FPlayerInfo* Player = FindPlayer(Id)) { ReleasePlayerForm(*Player); }
		}
	}
	if (bRestart) { RestartRound(); }
}

void FEscapeGameLogic::RestartRound()
{
	// 新的一局：物品回到初始狀態，還沒結束的玩家變回人類、回到出生點；已結束的玩家不動
	for (FItemState& Item : Items)
	{
		Item.bExists = Item.bStartsPresent;
		Item.Location = Item.InitialLocation;
		Item.OccupiedBy.Empty();
		Item.LocationId.Empty();
	}
	for (FPlayerInfo& Player : Players)
	{
		if (Team.HasEscaped(Player.PlayerId)) { continue; }
		Player.FormId = HumanForm;
		Player.LocationId.Empty();
		Player.Location = Player.SpawnLocation;
		Player.BodyLocation = Player.SpawnLocation;
	}
}

// ---- 查詢 ----

FString FEscapeGameLogic::GetPlayerForm(const FString& PlayerId) const
{
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	return Player ? Player->FormId : FString();
}

FVector FEscapeGameLogic::GetPlayerLocation(const FString& PlayerId) const
{
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	return Player ? Player->Location : FVector::ZeroVector;
}

FVector FEscapeGameLogic::GetBodyLocation(const FString& PlayerId) const
{
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	return Player ? Player->BodyLocation : FVector::ZeroVector;
}

FString FEscapeGameLogic::GetPlayerLocationId(const FString& PlayerId) const
{
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	return Player ? Player->LocationId : FString();
}

bool FEscapeGameLogic::CanSeeWholeMap(const FString& PlayerId) const
{
	const FPlayerInfo* Player = FindPlayer(PlayerId);
	const FItemState* Item = Player ? FindItem(Player->FormId) : nullptr;
	return Item && Item->bSeesWholeMap;
}

FVector FEscapeGameLogic::GetItemLocation(const FString& ItemId) const
{
	const FItemState* Item = FindItem(ItemId);
	return Item ? Item->Location : FVector::ZeroVector;
}

bool FEscapeGameLogic::IsItemPresent(const FString& ItemId) const
{
	const FItemState* Item = FindItem(ItemId);
	return Item && Item->bExists;
}

FString FEscapeGameLogic::GetItemOccupant(const FString& ItemId) const
{
	const FItemState* Item = FindItem(ItemId);
	return Item ? Item->OccupiedBy : FString();
}

FString FEscapeGameLogic::GetItemLocationId(const FString& ItemId) const
{
	const FItemState* Item = FindItem(ItemId);
	return Item ? Item->LocationId : FString();
}
