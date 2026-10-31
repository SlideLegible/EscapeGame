// 步驟 2 互動判定：CSV 載入與優先序判定。純函式，不改狀態、不擲骰。
#include "Logic/InteractionResolver.h"
#include "Serialization/Csv/CsvParser.h"

namespace
{
	using FTable = TArray<TMap<FString, FString>>;

	const TCHAR* const SelfTarget = TEXT("SELF");
	const TCHAR* const HumanForm = TEXT("F00");

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

	bool IsId(const FString& S, TCHAR Prefix, int32 Digits)
	{
		if (S.Len() != Digits + 1 || S[0] != Prefix) { return false; }
		for (int32 I = 1; I < S.Len(); ++I) { if (!FChar::IsDigit(S[I])) { return false; } }
		return true;
	}

	bool IsAllDigits(const FString& S)
	{
		if (S.IsEmpty()) { return false; }
		for (TCHAR C : S) { if (!FChar::IsDigit(C)) { return false; } }
		return true;
	}

	/** 在字串裡找所有 <Prefix>+<Digits> 個數字 */
	TArray<FString> FindIds(const FString& S, TCHAR Prefix, int32 Digits)
	{
		TArray<FString> Out;
		for (int32 I = 0; I + Digits < S.Len(); ++I)
		{
			if (S[I] == Prefix && IsId(S.Mid(I, Digits + 1), Prefix, Digits)) { Out.Add(S.Mid(I, Digits + 1)); }
		}
		return Out;
	}

	void AddCondition(FInteractionRow& Row, EInteractionConditionKind Kind, const FString& Arg)
	{
		for (const FInteractionCondition& C : Row.Conditions) { if (C.Kind == Kind && C.Arg == Arg) { return; } }
		FInteractionCondition NewCond;
		NewCond.Kind = Kind;
		NewCond.Arg = Arg;
		Row.Conditions.Add(NewCond);
	}

	FString Trim(FString S) { S.TrimStartAndEndInline(); return S; }

	/**
	 * 前置條件文法：
	 *   條件   := 子句 { "，且" 子句 }          （"，且" = 而且，全部要成立）
	 *   子句   := [ "已觸發過 " | "未觸發 " | "尚未觸發 " ] Xnnn
	 *           | Xnnn " 碰過全部的門"           （該 X 的 AnyDoor 列，所有門都碰過）
	 *           | "曾在大門輸入 " dddd " 且失敗"  （旗標 Tried<dddd>）
	 * 空字串 = 沒有條件。其他任何文字都是錯誤。
	 */
	bool ParsePrecondition(FInteractionRow& Row, FString& Err)
	{
		const FString Pre = Trim(Row.RawPrecondition);
		if (Pre.IsEmpty()) { return true; }
		TArray<FString> Clauses;
		Pre.ParseIntoArray(Clauses, TEXT("，且"), false);
		for (FString Clause : Clauses)
		{
			Clause = Trim(Clause);
			bool bOk = false;
			if (Clause.StartsWith(TEXT("曾在大門輸入 ")) && Clause.EndsWith(TEXT(" 且失敗")))
			{
				const FString Digits = Clause.Mid(7, Clause.Len() - 7 - 4);
				if (IsAllDigits(Digits)) { AddCondition(Row, EInteractionConditionKind::Flag, TEXT("Tried") + Digits); bOk = true; }
			}
			else if (Clause.EndsWith(TEXT(" 碰過全部的門")))
			{
				const FString Id = Trim(Clause.LeftChop(7));
				if (IsId(Id, TEXT('X'), 3)) { AddCondition(Row, EInteractionConditionKind::AllDoorsTouchedVia, Id); bOk = true; }
			}
			else
			{
				EInteractionConditionKind Kind = EInteractionConditionKind::Triggered;
				FString Rest = Clause;
				for (const TCHAR* P : { TEXT("已觸發過 "), TEXT("尚未觸發 "), TEXT("未觸發 ") })
				{
					const FString Prefix(P);
					if (Clause.StartsWith(Prefix))
					{
						Rest = Clause.Mid(Prefix.Len());
						if (Prefix != TEXT("已觸發過 ")) { Kind = EInteractionConditionKind::NotTriggered; }
						break;
					}
				}
				if (IsId(Trim(Rest), TEXT('X'), 3)) { AddCondition(Row, Kind, Trim(Rest)); bOk = true; }
			}
			if (!bOk)
			{
				Err = FString::Printf(TEXT("%s：前置條件無法解析：「%s」"), *Row.Id, *Row.RawPrecondition);
				return false;
			}
		}
		return true;
	}

	bool ParseHp(FInteractionRow& Row, FString& Err)
	{
		const FString Hp = Trim(Row.RawHpChange);
		if (Hp.IsEmpty()) { return true; }
		FString Digits = Hp;
		if (Digits.StartsWith(TEXT("+")) || Digits.StartsWith(TEXT("-"))) { Digits = Digits.Mid(1); }
		if (IsAllDigits(Digits)) { Row.HpDelta = FCString::Atoi(*Hp); return true; }
		// 「每次輸錯有 1/1000 機率 -1」
		int32 Slash = INDEX_NONE, Chance = INDEX_NONE;
		if (Hp.FindChar(TEXT('/'), Slash) && Hp.Find(TEXT(" 機率 ")) != INDEX_NONE)
		{
			Chance = Hp.Find(TEXT(" 機率 "));
			int32 NumStart = Slash;
			while (NumStart > 0 && FChar::IsDigit(Hp[NumStart - 1])) { --NumStart; }
			const FString Num = Hp.Mid(NumStart, Slash - NumStart);
			int32 DenEnd = Slash + 1;
			while (DenEnd < Hp.Len() && FChar::IsDigit(Hp[DenEnd])) { ++DenEnd; }
			const FString Den = Hp.Mid(Slash + 1, DenEnd - Slash - 1);
			FString Delta = Trim(Hp.Mid(Chance + 4));
			FString DeltaDigits = Delta.StartsWith(TEXT("+")) || Delta.StartsWith(TEXT("-")) ? Delta.Mid(1) : Delta;
			if (IsAllDigits(Num) && IsAllDigits(Den) && FCString::Atoi(*Den) > 0 && IsAllDigits(DeltaDigits))
			{
				Row.bHasHpChance = true;
				Row.HpChanceNumerator = FCString::Atoi(*Num);
				Row.HpChanceDenominator = FCString::Atoi(*Den);
				Row.HpChanceDelta = FCString::Atoi(*Delta);
				return true;
			}
		}
		Err = FString::Printf(TEXT("%s：生命值變化無法解析：「%s」"), *Row.Id, *Row.RawHpChange);
		return false;
	}

	bool PasswordTable(const FString& Id, EPasswordRequirement& Out)
	{
		// 設計文件沒有機器可讀的欄位：X016 輸入正確、X017 輸入錯誤（待人確認）
		if (Id == TEXT("X016")) { Out = EPasswordRequirement::Correct; return true; }
		if (Id == TEXT("X017")) { Out = EPasswordRequirement::Wrong; return true; }
		return false;
	}

	struct FPendingSecondTime { int32 RowIndex; };

	bool ParseRow(const TMap<FString, FString>& Cells, int32 Order, FInteractionRow& Row, bool& bSecondTime, FString& Err)
	{
		auto Get = [&](const TCHAR* Col) { return Trim(Cells[FString(Col)]); };
		Row.Id = Get(TEXT("Name"));
		Row.RawPlayerForm = Get(TEXT("PlayerForm"));
		Row.RawTarget = Get(TEXT("Target"));
		Row.RawEventType = Get(TEXT("EventType"));
		Row.RawEventText = Get(TEXT("EventText"));
		Row.RawTransformResult = Get(TEXT("TransformResult"));
		Row.RawSpawnedItem = Get(TEXT("SpawnedItem"));
		Row.RawHpChange = Get(TEXT("HpChange"));
		Row.RawAchievement = Get(TEXT("Achievement"));
		Row.RawPrecondition = Get(TEXT("Precondition"));
		Row.RawNotes = Get(TEXT("Notes"));
		Row.SourceOrder = Order;
		Row.EventText = Row.RawEventText;
		bSecondTime = false;

		if (!IsId(Row.Id, TEXT('X'), 3)) { Err = FString::Printf(TEXT("%s：互動 ID 格式錯誤"), *Row.Id); return false; }

		// 玩家形態
		if (Row.RawPlayerForm == TEXT("任一形態")) { Row.FormRule = EInteractionFormRule::AnyForm; }
		else if (Row.RawPlayerForm == TEXT("其他任一形態")) { Row.FormRule = EInteractionFormRule::OtherForm; }
		else if (Row.RawPlayerForm == TEXT("非人類形態")) { Row.FormRule = EInteractionFormRule::NonHuman; }
		else
		{
			Row.FormRule = EInteractionFormRule::Exact;
			Row.FormId = Row.RawPlayerForm.Left(3);
			if (!IsId(Row.FormId, TEXT('F'), 2) || (Row.RawPlayerForm.Len() > 3 && Row.RawPlayerForm[3] != TEXT(' ')))
			{
				Err = FString::Printf(TEXT("%s：玩家形態無法解析：「%s」"), *Row.Id, *Row.RawPlayerForm);
				return false;
			}
		}

		// 互動對象
		const FString& T = Row.RawTarget;
		if (T.StartsWith(TEXT("（自身）"))) { Row.TargetKind = EInteractionTargetKind::Self; Row.TargetId = SelfTarget; }
		else if (T.StartsWith(TEXT("任何門")))
		{
			Row.TargetKind = EInteractionTargetKind::AnyDoor;
			Row.DoorIds = FindIds(T, TEXT('S'), 2);
			if (Row.DoorIds.Num() == 0) { Err = FString::Printf(TEXT("%s：「任何門」沒有列出門的 ID"), *Row.Id); return false; }
		}
		else if (T == TEXT("碰過所有門")) { Row.TargetKind = EInteractionTargetKind::AllDoorsTouched; }
		else
		{
			Row.TargetKind = EInteractionTargetKind::Id;
			Row.TargetId = T.Left(3);
			if (!(IsId(Row.TargetId, TEXT('S'), 2) || IsId(Row.TargetId, TEXT('F'), 2)))
			{
				Err = FString::Printf(TEXT("%s：互動對象無法解析：「%s」"), *Row.Id, *T);
				return false;
			}
			int32 Open = INDEX_NONE;
			if (T.FindChar(TEXT('（'), Open))
			{
				int32 Close = INDEX_NONE;
				if (!T.FindChar(TEXT('）'), Close) || Close < Open) { Err = FString::Printf(TEXT("%s：互動對象括號不成對：「%s」"), *Row.Id, *T); return false; }
				const FString Qual = Trim(T.Mid(Open + 1, Close - Open - 1));
				if (Qual == TEXT("第一次")) { AddCondition(Row, EInteractionConditionKind::NotTriggered, Row.Id); }
				else if (Qual == TEXT("第二次以後")) { bSecondTime = true; }
				else if (Qual.StartsWith(TEXT("已試過 ")) && IsAllDigits(Qual.Mid(4))) { AddCondition(Row, EInteractionConditionKind::Flag, TEXT("Tried") + Qual.Mid(4)); }
				else if (Qual == TEXT("密碼鎖"))
				{
					if (!PasswordTable(Row.Id, Row.PasswordRequirement)) { Err = FString::Printf(TEXT("%s：密碼鎖沒有對應的密碼結果"), *Row.Id); return false; }
				}
				else { Err = FString::Printf(TEXT("%s：互動對象的括號限定無法解析：「%s」"), *Row.Id, *Qual); return false; }
			}
		}

		if (!ParsePrecondition(Row, Err) || !ParseHp(Row, Err)) { return false; }

		// 出現物品、變身結果、成就
		if (!Row.RawSpawnedItem.IsEmpty())
		{
			Row.SpawnedItemId = Row.RawSpawnedItem.Left(3);
			if (!IsId(Row.SpawnedItemId, TEXT('F'), 2)) { Err = FString::Printf(TEXT("%s：出現物品無法解析：「%s」"), *Row.Id, *Row.RawSpawnedItem); return false; }
		}
		if (!Row.RawTransformResult.IsEmpty())
		{
			Row.TransformFormId = Row.RawTransformResult;
			if (!IsId(Row.TransformFormId, TEXT('F'), 2)) { Err = FString::Printf(TEXT("%s：變身結果無法解析：「%s」"), *Row.Id, *Row.RawTransformResult); return false; }
		}
		if (!Row.RawAchievement.IsEmpty())
		{
			TArray<FString> Parts;
			Row.RawAchievement.ParseIntoArray(Parts, TEXT("、"), false);
			for (FString P : Parts)
			{
				P = Trim(P);
				if (IsId(P, TEXT('A'), 2)) { Row.AchievementIds.Add(P); }
				else if (IsId(P, TEXT('E'), 2) && Row.EndingId.IsEmpty()) { Row.EndingId = P; }
				else { Err = FString::Printf(TEXT("%s：成就欄無法解析：「%s」"), *Row.Id, *Row.RawAchievement); return false; }
			}
		}
		return true;
	}

	// ---- 判定 ----
	struct FEval
	{
		const FInteractionState& State;
		const TSet<FString>& Touched;  // 已碰過的門（含這次碰的）
		const TArray<FInteractionRow>& Rows;

		bool Holds(const FInteractionCondition& C) const
		{
			switch (C.Kind)
			{
			case EInteractionConditionKind::Triggered: return State.TriggeredIds.Contains(C.Arg);
			case EInteractionConditionKind::NotTriggered: return !State.TriggeredIds.Contains(C.Arg);
			case EInteractionConditionKind::Flag: return State.Flags.Contains(C.Arg);
			case EInteractionConditionKind::AllDoorsTouchedVia:
				for (const FInteractionRow& R : Rows)
				{
					if (R.Id == C.Arg && R.TargetKind == EInteractionTargetKind::AnyDoor)
					{
						for (const FString& D : R.DoorIds) { if (!Touched.Contains(D)) { return false; } }
						return true;
					}
				}
				return false;
			}
			return false;
		}

		bool AllHold(const FInteractionRow& Row) const
		{
			for (const FInteractionCondition& C : Row.Conditions) { if (!Holds(C)) { return false; } }
			return true;
		}
	};

	bool ExistsExactRow(const TArray<FInteractionRow>& Rows, const FString& Form, const FString& Target)
	{
		for (const FInteractionRow& R : Rows)
		{
			if (R.FormRule == EInteractionFormRule::Exact && R.FormId == Form
				&& (R.TargetKind == EInteractionTargetKind::Id || R.TargetKind == EInteractionTargetKind::Self) && R.TargetId == Target)
			{
				return true;
			}
		}
		return false;
	}

	bool FormMatches(const TArray<FInteractionRow>& Rows, const FInteractionRow& Row, const FString& Form, const FString& Target)
	{
		switch (Row.FormRule)
		{
		case EInteractionFormRule::Exact: return Row.FormId == Form;
		case EInteractionFormRule::AnyForm: return true;
		case EInteractionFormRule::NonHuman: return Form != HumanForm;
		case EInteractionFormRule::OtherForm: return !ExistsExactRow(Rows, Form, Target);
		}
		return false;
	}

	int32 Specificity(const FInteractionRow& R) { return R.Conditions.Num() + (R.PasswordRequirement != EPasswordRequirement::NotRequired ? 1 : 0); }

	bool Better(const FInteractionRow& A, const FInteractionRow* BestSoFar)
	{
		if (!BestSoFar) { return true; }
		if (Specificity(A) != Specificity(*BestSoFar)) { return Specificity(A) > Specificity(*BestSoFar); }
		return A.SourceOrder < BestSoFar->SourceOrder;
	}

	void FillFromRow(FInteractionResult& Out, const FInteractionRow& Row)
	{
		Out.InteractionId = Row.Id;
		Out.bIsDefault = false;
		Out.Text = Row.EventText;
		Out.HpDelta = Row.HpDelta;
		Out.bHasHpChance = Row.bHasHpChance;
		Out.HpChanceNumerator = Row.HpChanceNumerator;
		Out.HpChanceDenominator = Row.HpChanceDenominator;
		Out.HpChanceDelta = Row.HpChanceDelta;
		Out.SpawnedItemId = Row.SpawnedItemId;
		Out.TransformFormId = Row.TransformFormId;
		Out.AchievementIds = Row.AchievementIds;
		Out.EndingId = Row.EndingId;
	}
}

bool FInteractionResolver::Load(const FString& InteractionsCsv, const FString& FormsCsv, const FString& SceneObjectsCsv,
	const FString& ResponsesCsv, FInteractionResolver& OutResolver, FString& OutError)
{
	FInteractionResolver NewResolver;
	FTable Forms, Objects, Responses, Interactions;
	if (!ParseTable(FormsCsv, { TEXT("Name"), TEXT("FormName") }, Forms, OutError, TEXT("DT_Forms"))) { return false; }
	if (!ParseTable(SceneObjectsCsv, { TEXT("Name"), TEXT("ObjectName") }, Objects, OutError, TEXT("DT_SceneObjects"))) { return false; }
	if (!ParseTable(ResponsesCsv, { TEXT("Name"), TEXT("Text") }, Responses, OutError, TEXT("DT_Responses"))) { return false; }
	if (!ParseTable(InteractionsCsv, { TEXT("Name"), TEXT("PlayerForm"), TEXT("Target"), TEXT("EventType"), TEXT("EventText"),
		TEXT("TransformResult"), TEXT("SpawnedItem"), TEXT("HpChange"), TEXT("Achievement"), TEXT("Precondition"), TEXT("Notes") },
		Interactions, OutError, TEXT("DT_Interactions"))) { return false; }

	for (const auto& R : Forms) { NewResolver.Names.Add(R[TEXT("Name")], R[TEXT("FormName")]); }
	for (const auto& R : Objects) { NewResolver.Names.Add(R[TEXT("Name")], R[TEXT("ObjectName")]); }
	for (const auto& R : Responses) { if (R[TEXT("Name")] == TEXT("Default")) { NewResolver.DefaultResponse = R[TEXT("Text")]; } }
	if (NewResolver.DefaultResponse.IsEmpty()) { OutError = TEXT("DT_Responses：沒有 Default 列"); return false; }

	TArray<bool> SecondTime;
	TSet<FString> SeenIds;
	for (int32 I = 0; I < Interactions.Num(); ++I)
	{
		FInteractionRow Row;
		bool bSecond = false;
		if (!ParseRow(Interactions[I], I, Row, bSecond, OutError)) { return false; }
		if (SeenIds.Contains(Row.Id)) { OutError = FString::Printf(TEXT("%s：互動 ID 重複"), *Row.Id); return false; }
		SeenIds.Add(Row.Id);
		NewResolver.Rows.Add(MoveTemp(Row));
		SecondTime.Add(bSecond);
	}

	// 第二輪：引用的 ID 都要存在；「（第二次以後）」對應同對象的「（第一次）」列
	for (int32 I = 0; I < NewResolver.Rows.Num(); ++I)
	{
		FInteractionRow& Row = NewResolver.Rows[I];
		if (Row.FormRule == EInteractionFormRule::Exact && !NewResolver.Names.Contains(Row.FormId)) { OutError = FString::Printf(TEXT("%s：形態 %s 不在 DT_Forms"), *Row.Id, *Row.FormId); return false; }
		if (Row.TargetKind == EInteractionTargetKind::Id && !NewResolver.Names.Contains(Row.TargetId)) { OutError = FString::Printf(TEXT("%s：對象 %s 不在 DT_Forms／DT_SceneObjects"), *Row.Id, *Row.TargetId); return false; }
		for (const FString& D : Row.DoorIds)
		{
			if (!NewResolver.Names.Contains(D)) { OutError = FString::Printf(TEXT("%s：門 %s 不在 DT_SceneObjects"), *Row.Id, *D); return false; }
			NewResolver.DoorIds.AddUnique(D);
		}
		for (const FString& F : { Row.TransformFormId, Row.SpawnedItemId })
		{
			if (!F.IsEmpty() && !NewResolver.Names.Contains(F)) { OutError = FString::Printf(TEXT("%s：形態 %s 不在 DT_Forms"), *Row.Id, *F); return false; }
		}
		if (SecondTime[I])
		{
			const FInteractionRow* First = nullptr;
			for (const FInteractionRow& Other : NewResolver.Rows)
			{
				if (Other.Id != Row.Id && Other.TargetId == Row.TargetId && Other.TargetKind == Row.TargetKind && Other.RawTarget.Contains(TEXT("（第一次）"))) { First = &Other; }
			}
			if (!First) { OutError = FString::Printf(TEXT("%s：「（第二次以後）」找不到同對象的「（第一次）」列"), *Row.Id); return false; }
			AddCondition(Row, EInteractionConditionKind::Triggered, First->Id);
		}
		for (const FInteractionCondition& C : Row.Conditions)
		{
			if (C.Kind == EInteractionConditionKind::Flag) { continue; }
			if (!SeenIds.Contains(C.Arg)) { OutError = FString::Printf(TEXT("%s：前置條件引用了不存在的 %s"), *Row.Id, *C.Arg); return false; }
		}
	}

	OutResolver = MoveTemp(NewResolver);
	OutError.Empty();
	return true;
}

FString FInteractionResolver::GetDisplayName(const FString& Id) const
{
	const FString* Name = Names.Find(Id);
	return Name ? *Name : Id;
}

FInteractionResult FInteractionResolver::Resolve(const FInteractionQuery& Query) const
{
	FInteractionResult Result;
	const FString& Form = Query.FormId;
	const FString& Target = Query.TargetId;

	// 1. 碰門的鑰匙圈（門的清單、形態都來自資料）
	{
		TSet<FString> Touched = Query.State.TouchedDoors;
		Touched.Add(Target);
		const FEval Eval{ Query.State, Touched, Rows };
		const FInteractionRow* Best = nullptr;
		for (const FInteractionRow& R : Rows)
		{
			if (R.TargetKind == EInteractionTargetKind::AnyDoor && R.DoorIds.Contains(Target)
				&& FormMatches(Rows, R, Form, Target) && Eval.AllHold(R) && Better(R, Best))
			{
				Best = &R;
			}
		}
		if (Best)
		{
			FillFromRow(Result, *Best);
			for (const FInteractionRow& R : Rows)
			{
				// 「碰過所有門」：把這扇門算進去之後全部碰過 → 額外觸發
				if (R.TargetKind == EInteractionTargetKind::AllDoorsTouched && FormMatches(Rows, R, Form, Target) && Eval.AllHold(R))
				{
					FExtraTrigger Extra;
					Extra.InteractionId = R.Id;
					Extra.AchievementIds = R.AchievementIds;
					Extra.EndingId = R.EndingId;
					Result.ExtraTriggers.Add(Extra);
				}
			}
			return Result;
		}
	}

	// 2. 精確　3. 其他任一形態／非人類形態　4. 任一形態　（每層只看前置條件成立的列）
	const FEval Eval{ Query.State, Query.State.TouchedDoors, Rows };
	bool bNeedsPassword = false;
	for (int32 Tier = 0; Tier < 3; ++Tier)
	{
		const FInteractionRow* Best = nullptr;
		for (const FInteractionRow& R : Rows)
		{
			const bool bTierOk = Tier == 0 ? R.FormRule == EInteractionFormRule::Exact
				: Tier == 1 ? (R.FormRule == EInteractionFormRule::OtherForm || R.FormRule == EInteractionFormRule::NonHuman)
				: R.FormRule == EInteractionFormRule::AnyForm;
			if (!bTierOk) { continue; }
			const bool bTargetOk = (R.TargetKind == EInteractionTargetKind::Id || R.TargetKind == EInteractionTargetKind::Self) && R.TargetId == Target;
			if (!bTargetOk || !FormMatches(Rows, R, Form, Target)) { continue; }
			if (R.PasswordRequirement != EPasswordRequirement::NotRequired)
			{
				if (Query.State.Password == EPasswordInput::None) { bNeedsPassword = true; continue; }
				const bool bSame = (R.PasswordRequirement == EPasswordRequirement::Correct) == (Query.State.Password == EPasswordInput::Correct);
				if (!bSame) { continue; }
			}
			if (Eval.AllHold(R) && Better(R, Best)) { Best = &R; }
		}
		if (Best)
		{
			FillFromRow(Result, *Best);
			return Result;
		}
	}

	if (bNeedsPassword)
	{
		Result.bNeedsPassword = true;
		return Result;
	}

	// 5. 預設回應
	Result.bIsDefault = true;
	const FString NameId = Target == SelfTarget ? Form : Target;
	Result.Text = DefaultResponse.Replace(TEXT("[對象]"), *GetDisplayName(NameId));
	return Result;
}
