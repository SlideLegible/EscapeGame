// 步驟 2 互動判定 → VERIFY STEP 2
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/Csv/CsvParser.h"
#include "Logic/InteractionResolver.h"

namespace Step2Spec
{
	/** 測試端自己從 CSV 原始文字解讀出的一列（不使用 Resolver 的解析結果） */
	struct FCase
	{
		FString Id, PlayerForm, Target, EventText, Transform, Spawn, Hp, Ach, Pre;
		EInteractionFormRule Rule = EInteractionFormRule::Exact;
		FString FormId;
		EInteractionTargetKind Kind = EInteractionTargetKind::Id;
		FString TargetId;
		FString Qualifier;
		TArray<FString> Doors;
	};

	struct FCtx
	{
		FString Interactions, Forms, Objects, Responses;
		FInteractionResolver Resolver;
		FString Error;
		bool bLoaded = false;
		TArray<FString> FormIds;
		TArray<FCase> Cases;
		TArray<FString> Doors;
		FString KeyForm;
	};

	TArray<TMap<FString, FString>> ParseTable(const FString& Text)
	{
		TArray<TMap<FString, FString>> Out;
		FCsvParser Parser(Text);
		const FCsvParser::FRows& Rows = Parser.GetRows();
		if (Rows.Num() == 0) { return Out; }
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			TMap<FString, FString> Row;
			for (int32 C = 0; C < Rows[0].Num() && C < Rows[R].Num(); ++C)
			{
				Row.Add(FString(Rows[0][C]), FString(Rows[R][C]));
			}
			Out.Add(MoveTemp(Row));
		}
		return Out;
	}

	TArray<FString> FindIds(const FString& S, TCHAR Prefix, int32 Digits)
	{
		TArray<FString> Out;
		for (int32 I = 0; I + Digits < S.Len(); ++I)
		{
			if (S[I] != Prefix) { continue; }
			bool bOk = true;
			for (int32 D = 1; D <= Digits; ++D) { bOk = bOk && FChar::IsDigit(S[I + D]); }
			if (bOk) { Out.Add(S.Mid(I, Digits + 1)); }
		}
		return Out;
	}

	FCase MakeCase(const TMap<FString, FString>& Row)
	{
		FCase C;
		C.Id = Row[TEXT("Name")];
		C.PlayerForm = Row[TEXT("PlayerForm")];
		C.Target = Row[TEXT("Target")];
		C.EventText = Row[TEXT("EventText")];
		C.Transform = Row[TEXT("TransformResult")];
		C.Spawn = Row[TEXT("SpawnedItem")];
		C.Hp = Row[TEXT("HpChange")];
		C.Ach = Row[TEXT("Achievement")];
		C.Pre = Row[TEXT("Precondition")];
		if (C.PlayerForm == TEXT("任一形態")) { C.Rule = EInteractionFormRule::AnyForm; }
		else if (C.PlayerForm == TEXT("其他任一形態")) { C.Rule = EInteractionFormRule::OtherForm; }
		else if (C.PlayerForm == TEXT("非人類形態")) { C.Rule = EInteractionFormRule::NonHuman; }
		else { C.Rule = EInteractionFormRule::Exact; C.FormId = C.PlayerForm.Left(3); }

		if (C.Target.StartsWith(TEXT("（自身）"))) { C.Kind = EInteractionTargetKind::Self; C.TargetId = TEXT("SELF"); }
		else if (C.Target.StartsWith(TEXT("任何門"))) { C.Kind = EInteractionTargetKind::AnyDoor; C.Doors = FindIds(C.Target, TEXT('S'), 2); }
		else if (C.Target.StartsWith(TEXT("碰過所有門"))) { C.Kind = EInteractionTargetKind::AllDoorsTouched; }
		else
		{
			C.Kind = EInteractionTargetKind::Id;
			C.TargetId = C.Target.Left(3);
			int32 Paren = INDEX_NONE;
			if (C.Target.FindChar(TEXT('（'), Paren)) { C.Qualifier = C.Target.Mid(Paren); }
		}
		return C;
	}

	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	/** 這一列的形態規則是否接受形態 F（不管前置條件） */
	bool AcceptsForm(const FCase& Row, const FString& F, const FString& Target, const TArray<FCase>& All)
	{
		switch (Row.Rule)
		{
		case EInteractionFormRule::Exact: return Row.FormId == F;
		case EInteractionFormRule::AnyForm: return true;
		case EInteractionFormRule::NonHuman: return F != TEXT("F00");
		case EInteractionFormRule::OtherForm:
			for (const FCase& Q : All)
			{
				if (Q.Rule == EInteractionFormRule::Exact && Q.Kind != EInteractionTargetKind::AnyDoor && Q.TargetId == Target && Q.FormId == F) { return false; }
			}
			return true;
		}
		return false;
	}

	int32 Tier(EInteractionFormRule R)
	{
		return R == EInteractionFormRule::Exact ? 0 : (R == EInteractionFormRule::AnyForm ? 2 : 1);
	}

	FString TargetOf(const FCase& C)
	{
		if (C.Kind == EInteractionTargetKind::AnyDoor || C.Kind == EInteractionTargetKind::AllDoorsTouched) { return C.Doors.Num() ? C.Doors[0] : TEXT("S01"); }
		return C.TargetId;
	}

	/**
	 * 挑形態：Exact 用該列的形態。其餘（任一／其他任一／非人類）從形態表依序挑第一個
	 * 「該列接受、而且沒有更高層（更精確）的另一列接受」的形態，也避開鑰匙圈（碰門時 X033 優先）。
	 */
	FString ChooseForm(const FCtx& Ctx, const FCase& C)
	{
		if (C.Rule == EInteractionFormRule::Exact) { return C.FormId; }
		const FString Target = TargetOf(C);
		for (const FString& F : Ctx.FormIds)
		{
			if (Ctx.Doors.Contains(Target) && F == Ctx.KeyForm) { continue; }
			if (!AcceptsForm(C, F, Target, Ctx.Cases)) { continue; }
			bool bIntercepted = false;
			for (const FCase& Q : Ctx.Cases)
			{
				const bool bSameTarget = (Q.Kind == EInteractionTargetKind::Id || Q.Kind == EInteractionTargetKind::Self) && Q.TargetId == Target;
				if (bSameTarget && Q.Id != C.Id && Tier(Q.Rule) < Tier(C.Rule) && AcceptsForm(Q, F, Target, Ctx.Cases)) { bIntercepted = true; }
			}
			if (!bIntercepted) { return F; }
		}
		return FString();
	}

	bool IsNegativeClause(const FString& Clause) { return Clause.StartsWith(TEXT("未觸發")) || Clause.StartsWith(TEXT("尚未觸發")); }

	/** 依前置條件與對象欄括號，設定能讓條件成立的最小狀態 */
	void ApplyState(const FCase& C, FInteractionState& S)
	{
		TArray<FString> Clauses;
		C.Pre.ParseIntoArray(Clauses, TEXT("，且"));
		for (FString Clause : Clauses)
		{
			Clause.TrimStartAndEndInline();
			if (Clause.Contains(TEXT("1031"))) { S.Flags.Add(TEXT("Tried1031")); }
			else if (!IsNegativeClause(Clause) && !Clause.Contains(TEXT("碰過全部的門")))
			{
				for (const FString& X : FindIds(Clause, TEXT('X'), 3)) { S.TriggeredIds.Add(X); }
			}
		}
		if (C.Qualifier.Contains(TEXT("第二次以後"))) { S.TriggeredIds.Add(TEXT("X004")); }
		if (C.Qualifier.Contains(TEXT("已試過 1031"))) { S.Flags.Add(TEXT("Tried1031")); }
		if (C.Qualifier.Contains(TEXT("密碼鎖"))) { S.Password = (C.Id == TEXT("X017")) ? EPasswordInput::Wrong : EPasswordInput::Correct; }
	}

	bool HasPositiveCondition(const FCase& C)
	{
		TArray<FString> Clauses;
		C.Pre.ParseIntoArray(Clauses, TEXT("，且"));
		for (FString Clause : Clauses)
		{
			Clause.TrimStartAndEndInline();
			if (!Clause.IsEmpty() && !IsNegativeClause(Clause)) { return true; }
		}
		return false;
	}

	FInteractionQuery MakeQuery(const FString& Form, const FString& Target)
	{
		FInteractionQuery Q;
		Q.FormId = Form;
		Q.TargetId = Target;
		return Q;
	}

	TArray<FString> Split(const FString& S, const TCHAR* Delim)
	{
		TArray<FString> Out;
		S.ParseIntoArray(Out, Delim);
		for (FString& O : Out) { O.TrimStartAndEndInline(); }
		return Out;
	}

	FString Sanitize(FString S)
	{
		S.ReplaceInline(TEXT("."), TEXT(""));
		return S;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep2Spec, "EscapeGame.Step2",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
	TSharedPtr<Step2Spec::FCtx> Ctx;
END_DEFINE_SPEC(FEscapeGameStep2Spec)

void FEscapeGameStep2Spec::Define()
{
	using namespace Step2Spec;
	Ctx = MakeShared<FCtx>();
	TSharedRef<FCtx> C = Ctx.ToSharedRef();
	C->Interactions = LoadText(TEXT("DT_Interactions.csv"));
	C->Forms = LoadText(TEXT("DT_Forms.csv"));
	C->Objects = LoadText(TEXT("DT_SceneObjects.csv"));
	C->Responses = LoadText(TEXT("DT_Responses.csv"));
	C->bLoaded = FInteractionResolver::Load(C->Interactions, C->Forms, C->Objects, C->Responses, C->Resolver, C->Error);
	for (const auto& Row : ParseTable(C->Forms)) { C->FormIds.Add(Row[TEXT("Name")]); }
	for (const auto& Row : ParseTable(C->Interactions)) { C->Cases.Add(MakeCase(Row)); }
	for (const FCase& Case : C->Cases)
	{
		if (Case.Kind == EInteractionTargetKind::AnyDoor) { C->Doors = Case.Doors; C->KeyForm = Case.FormId; }
	}

	// 沒載入成功時先讓每條測試都有明確的失敗原因
	auto RequireLoaded = [this, C]() -> bool
	{
		if (!C->bLoaded) { AddError(FString::Printf(TEXT("載入失敗：%s"), *C->Error)); }
		return C->bLoaded;
	};

	Describe(TEXT("CSV 每列 X 一個測試案例"), [this, C, RequireLoaded]()
	{
		for (const FCase& Case : C->Cases)
		{
			const FString Name = Sanitize(FString::Printf(TEXT("%s %s 碰 %s 命中該列"), *Case.Id, *Case.PlayerForm, *Case.Target));
			It(Name, [this, C, RequireLoaded, Case]()
			{
				if (!RequireLoaded()) { return; }
				FInteractionQuery Q;
				bool bAllDoors = Case.Kind == EInteractionTargetKind::AllDoorsTouched;
				if (bAllDoors)
				{
					// X034：F11 碰最後一扇沒碰過的門，四扇門就全碰過了
					Q = MakeQuery(C->KeyForm, C->Doors.Last());
					for (int32 I = 0; I + 1 < C->Doors.Num(); ++I) { Q.State.TouchedDoors.Add(C->Doors[I]); }
				}
				else
				{
					const FString Form = ChooseForm(*C, Case);
					TestFalse(TEXT("找得到不被更精確的列攔截的形態"), Form.IsEmpty());
					Q = MakeQuery(Form, TargetOf(Case));
					ApplyState(Case, Q.State);
				}
				const FInteractionResult R = C->Resolver.Resolve(Q);

				if (bAllDoors)
				{
					TestEqual(TEXT("同時命中 X033"), R.InteractionId, FString(TEXT("X033")));
					const FExtraTrigger* Extra = R.ExtraTriggers.FindByPredicate([&](const FExtraTrigger& E) { return E.InteractionId == Case.Id; });
					TestNotNull(TEXT("額外觸發清單有該列"), Extra);
					if (Extra) { TestTrue(TEXT("成就和 CSV 一致"), Extra->AchievementIds == Split(Case.Ach, TEXT("、"))); }
					return;
				}

				TestEqual(TEXT("命中那一列"), R.InteractionId, Case.Id);
				TestFalse(TEXT("不是預設回應"), R.bIsDefault);
				TestEqual(TEXT("文字"), R.Text, Case.EventText);
				TestEqual(TEXT("出現物品"), R.SpawnedItemId, Case.Spawn.IsEmpty() ? FString() : Case.Spawn.Left(3));
				TestEqual(TEXT("變身結果"), R.TransformFormId, Case.Transform);

				TArray<FString> ExpectedAch;
				FString ExpectedEnding;
				for (const FString& A : Split(Case.Ach, TEXT("、")))
				{
					if (A.StartsWith(TEXT("E"))) { ExpectedEnding = A; } else { ExpectedAch.Add(A); }
				}
				TestTrue(TEXT("成就"), R.AchievementIds == ExpectedAch);
				TestEqual(TEXT("結局"), R.EndingId, ExpectedEnding);

				if (Case.Hp.Contains(TEXT("機率")))
				{
					// 「每次輸錯有 1/1000 機率 -1」：只記錄機率，不擲骰，HpDelta 保持 0
					TestEqual(TEXT("確定的生命值變化為 0"), R.HpDelta, 0);
					TestTrue(TEXT("有機率扣血"), R.bHasHpChance);
					TestEqual(TEXT("分子"), R.HpChanceNumerator, 1);
					TestEqual(TEXT("分母"), R.HpChanceDenominator, 1000);
					TestEqual(TEXT("機率發生時的變化"), R.HpChanceDelta, -1);
				}
				else
				{
					TestFalse(TEXT("沒有機率扣血"), R.bHasHpChance);
					const int32 Expected = Case.Hp.IsEmpty() ? 0 : FCString::Atoi(*Case.Hp);
					TestEqual(TEXT("生命值變化"), R.HpDelta, Expected);
				}
			});
		}
	});

	Describe(TEXT("VERIFY STEP 2"), [this, C, RequireLoaded]()
	{
		It(TEXT("變成一大串鑰匙圈（F11）碰任何一扇門（S01、S05、S09、S19）→ 得到 X033，即使該門有其他回應"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			TestEqual(TEXT("門有四扇"), C->Doors.Num(), 4);
			for (const FString& Door : C->Doors)
			{
				FInteractionQuery Q = MakeQuery(TEXT("F11"), Door);
				// 讓各門的其他回應都有機會成立
				for (const TCHAR* X : { TEXT("X009"), TEXT("X005"), TEXT("X006"), TEXT("X013"), TEXT("X003") }) { Q.State.TriggeredIds.Add(X); }
				Q.State.Password = EPasswordInput::Correct;
				const FInteractionResult R = C->Resolver.Resolve(Q);
				TestEqual(*FString::Printf(TEXT("%s 命中 X033"), *Door), R.InteractionId, FString(TEXT("X033")));
				TestTrue(TEXT("文字"), R.Text.Contains(TEXT("你試了每一支鑰匙……沒有一支插得進去")));
				TestFalse(TEXT("不是預設回應"), R.bIsDefault);
			}
			// 合成資料：就算另外有 F11 對 S19 的精確列，X033 還是優先
			FString Extra = C->Interactions;
			if (!Extra.EndsWith(TEXT("\n"))) { Extra += TEXT("\n"); }
			Extra += TEXT("X998,F11 一大串鑰匙圈,S19 教室門,搞怪,精確列,,,,,,\n");
			FInteractionResolver Synth;
			FString Err;
			const bool bOk = FInteractionResolver::Load(Extra, C->Forms, C->Objects, C->Responses, Synth, Err);
			TestTrue(*FString::Printf(TEXT("合成資料載入成功 %s"), *Err), bOk);
			if (bOk) { TestEqual(TEXT("X033 優先於精確列"), Synth.Resolve(MakeQuery(TEXT("F11"), TEXT("S19"))).InteractionId, FString(TEXT("X033"))); }
		});

		It(TEXT("變成椅子（F12）以外的形態碰教室門（S19），且教室門還沒撞開 → 得到 X037，不是預設回應"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			for (const FString& F : C->FormIds)
			{
				if (F == TEXT("F12") || F == TEXT("F11")) { continue; }  // F11 碰門依第 2 條得到 X033
				const FInteractionResult R = C->Resolver.Resolve(MakeQuery(F, TEXT("S19")));
				TestEqual(*FString::Printf(TEXT("%s 得到 X037"), *F), R.InteractionId, FString(TEXT("X037")));
				TestFalse(TEXT("不是預設回應"), R.bIsDefault);
			}
			FInteractionQuery Chair = MakeQuery(TEXT("F12"), TEXT("S19"));
			TestEqual(TEXT("椅子得到 X036"), C->Resolver.Resolve(Chair).InteractionId, FString(TEXT("X036")));
			FInteractionQuery Opened = MakeQuery(TEXT("F00"), TEXT("S19"));
			Opened.State.TriggeredIds.Add(TEXT("X036"));
			TestTrue(TEXT("撞開後不再是 X037"), C->Resolver.Resolve(Opened).InteractionId != TEXT("X037"));
		});

		It(TEXT("任何形態碰講桌抽屜（S20）→ 得到 X030"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			for (const FString& F : C->FormIds)
			{
				TestEqual(*FString::Printf(TEXT("%s 得到 X030"), *F), C->Resolver.Resolve(MakeQuery(F, TEXT("S20"))).InteractionId, FString(TEXT("X030")));
			}
		});

		It(TEXT("人類碰電腦（S07）→ 得到 X007「電腦需要密碼才能登入，打不開」"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			FInteractionQuery Q = MakeQuery(TEXT("F00"), TEXT("S07"));
			Q.State.TriggeredIds.Add(TEXT("X005"));  // X007 的前置條件：辦公室門已打開
			const FInteractionResult R = C->Resolver.Resolve(Q);
			TestEqual(TEXT("命中 X007"), R.InteractionId, FString(TEXT("X007")));
			TestEqual(TEXT("文字"), R.Text, FString(TEXT("電腦需要密碼才能登入，打不開")));
		});

		It(TEXT("骰子（F08）碰月曆（S18）這種表上沒有的組合 → 得到預設回應「月曆紋絲不動，好像在嘲笑你：「就這？你是認真的嗎？」」，生命值、形態都不變"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			const FInteractionResult R = C->Resolver.Resolve(MakeQuery(TEXT("F08"), TEXT("S18")));
			TestTrue(TEXT("是預設回應"), R.bIsDefault);
			TestTrue(TEXT("沒有互動 ID"), R.InteractionId.IsEmpty());
			TestEqual(TEXT("文字"), R.Text, FString(TEXT("月曆紋絲不動，好像在嘲笑你：「就這？你是認真的嗎？」")));
			TestEqual(TEXT("生命值不變"), R.HpDelta, 0);
			TestFalse(TEXT("沒有機率扣血"), R.bHasHpChance);
			TestTrue(TEXT("形態不變"), R.TransformFormId.IsEmpty());
			TestTrue(TEXT("沒有出現物品"), R.SpawnedItemId.IsEmpty());
			TestTrue(TEXT("沒有成就"), R.AchievementIds.Num() == 0 && R.EndingId.IsEmpty());
		});

		It(TEXT("不可以 前置條件沒滿足時觸發該互動（例：還沒觸發 X013，長竿子不存在，不可以觸發 X015）"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			TestTrue(TEXT("X013 未觸發時 X015 不會命中"), C->Resolver.Resolve(MakeQuery(TEXT("F07"), TEXT("S13"))).InteractionId != TEXT("X015"));
			FInteractionQuery Ok = MakeQuery(TEXT("F07"), TEXT("S13"));
			Ok.State.TriggeredIds.Add(TEXT("X013"));
			TestEqual(TEXT("X013 已觸發時命中 X015"), C->Resolver.Resolve(Ok).InteractionId, FString(TEXT("X015")));

			// 所有有「必須已成立」條件的列：空狀態時都不可以命中
			for (const FCase& Case : C->Cases)
			{
				if (Case.Kind == EInteractionTargetKind::AllDoorsTouched || !HasPositiveCondition(Case)) { continue; }
				const FInteractionResult R = C->Resolver.Resolve(MakeQuery(ChooseForm(*C, Case), TargetOf(Case)));
				TestTrue(*FString::Printf(TEXT("%s 空狀態不會命中"), *Case.Id), R.InteractionId != Case.Id);
			}
			// X034：只碰第一扇門時不會有額外觸發
			TestEqual(TEXT("沒碰過全部的門 → 沒有 X034"), C->Resolver.Resolve(MakeQuery(TEXT("F11"), TEXT("S01"))).ExtraTriggers.Num(), 0);
		});
	});

	Describe(TEXT("補充"), [this, C, RequireLoaded]()
	{
		It(TEXT("人類碰大門（密碼鎖）但還沒有密碼輸入結果 → 回傳需要輸入密碼，不是預設回應"), [this, C, RequireLoaded]()
		{
			if (!RequireLoaded()) { return; }
			const FInteractionResult R = C->Resolver.Resolve(MakeQuery(TEXT("F00"), TEXT("S01")));
			TestTrue(TEXT("需要輸入密碼"), R.bNeedsPassword);
			TestFalse(TEXT("不是預設回應"), R.bIsDefault);
		});

		It(TEXT("前置條件無法解析時載入會報錯，不會悄悄當成沒有條件"), [this, C]()
		{
			FString Bad = C->Interactions;
			if (!Bad.EndsWith(TEXT("\n"))) { Bad += TEXT("\n"); }
			Bad += TEXT("X999,F00 人類,S02 垃圾桶,搞怪,測試,,,,,天亮之後,\n");
			FInteractionResolver R;
			FString Err;
			TestFalse(TEXT("載入失敗"), FInteractionResolver::Load(Bad, C->Forms, C->Objects, C->Responses, R, Err));
			TestTrue(TEXT("錯誤訊息指出互動 ID"), Err.Contains(TEXT("X999")));
		});
	});
}
