// 步驟 10 介面 → VERIFY STEP 10
//
// 可測的部分都在純 C++ 的 FEscapeUiPresenter（HUD 文字、彈出框、鍵盤、結局、成就提示的狀態與 3 秒計時）：
// 用真的 CSV 與 FEscapeGameLogic 產生 outcome 餵給它。Widget 另外檢查結構（自動換行、不裁切、不限高度、字型）。
// 「不可以截斷」：-nullrhi 的 UnrealEditor-Cmd 下 FSlateApplication 與字型量測可用（10a 實測），
// 所以真的用 STextBlock 的 WrapTextAt 量最長文字換行後的行數，再加上 widget 結構檢查。
// 整個檔案只在非 Shipping 編譯：E02 要用 GetPasswordForDevOnly()（Shipping 沒有任何途徑拿到密碼）。
#if !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/Csv/CsvParser.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/StrongObjectPtr.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"
#include "UI/EscapeUiPresenter.h"
#include "UI/EscapeFonts.h"
#include "UI/EscapeHudWidget.h"
#include "UI/EscapePopupWidget.h"
#include "UI/EscapeKeypadWidget.h"
#include "UI/EscapeEndingWidget.h"
#include "UI/EscapeToastWidget.h"
#include "UI/EscapeHUD.h"
#include "Core/EscapeGameMode.h"
#include "Core/EscapeGameState.h"
#include "Core/EscapePlayerState.h"
#include "Core/EscapePlayerController.h"

namespace Step10Spec
{
	// ---- CSV 與 Logic ----

	FString LoadText(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data") / Name));
		return Text;
	}

	/** 第一欄當 key：key → (欄名 → 值)。檔案不存在或空的 → 空表。 */
	TMap<FString, TMap<FString, FString>> LoadTable(const FString& Name)
	{
		TMap<FString, TMap<FString, FString>> Table;
		const FString Text = LoadText(Name);
		FCsvParser Parser(Text);
		const FCsvParser::FRows& Rows = Parser.GetRows();
		if (Rows.Num() < 2) { return Table; }
		TArray<FString> Header;
		for (const TCHAR* Cell : Rows[0]) { Header.Add(Cell); }
		for (int32 R = 1; R < Rows.Num(); ++R)
		{
			if (Rows[R].Num() == 0) { continue; }
			TMap<FString, FString> Row;
			for (int32 C = 0; C < Header.Num() && C < Rows[R].Num(); ++C) { Row.Add(Header[C], Rows[R][C]); }
			Table.Add(Rows[R][0], Row);
		}
		return Table;
	}

	bool MakeLogic(int32 Seed, FEscapeGameLogic& Out, FString& Error)
	{
		return FEscapeGameLogic::Load(LoadText(TEXT("DT_Interactions.csv")), LoadText(TEXT("DT_Forms.csv")),
			LoadText(TEXT("DT_SceneObjects.csv")), LoadText(TEXT("DT_Responses.csv")), LoadText(TEXT("DT_Locations.csv")),
			Seed, Out, Error);
	}

	bool MakeUi(FEscapeUiPresenter& Out, FString& Error)
	{
		return Out.LoadTexts(LoadText(TEXT("DT_UiText.csv")), LoadText(TEXT("DT_Endings.csv")), LoadText(TEXT("DT_Achievements.csv")), Error);
	}

	FString UiText(const FString& Name)
	{
		const auto Table = LoadTable(TEXT("DT_UiText.csv"));
		const TMap<FString, FString>* Row = Table.Find(Name);
		return Row ? Row->FindRef(TEXT("Text")) : FString();
	}

	FString FormName(const FString& FormId)
	{
		const auto Table = LoadTable(TEXT("DT_Forms.csv"));
		const TMap<FString, FString>* Row = Table.Find(FormId);
		return Row ? Row->FindRef(TEXT("FormName")) : FString();
	}

	// ---- 走到各結局的路線（照 Step8 的做法，各檔案自己寫一份） ----

	/** 人類 Player 走完 X001、碰飲料罐垃圾、進 L02、變成學生證（F03） */
	bool BecomeStudentCard(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.Interact(Player, TEXT("S02")).Kind != EInteractKind::Resolved) { return false; }
		if (!Logic.Interact(Player, TEXT("F01")).bTransformed) { return false; }
		if (Logic.Interact(Player, TEXT("S04")).EnteredLocationId != TEXT("L02")) { return false; }
		return Logic.Interact(Player, TEXT("F03")).bTransformed && Logic.GetPlayerForm(Player) == TEXT("F03");
	}

	/** E03 路線：X009 → F04 → X005 → 變回人類 → X006 → F05 → X013 → F07。成功時玩家是長竿子（F07）。 */
	bool BecomeLongPole(FEscapeGameLogic& Logic, const FString& Player)
	{
		const auto Becomes = [&](const FString& Target) { return Logic.Interact(Player, Target).bTransformed && Logic.GetPlayerForm(Player) == Target; };
		return Logic.Interact(Player, TEXT("S10")).Result.InteractionId == TEXT("X009") && Becomes(TEXT("F04"))
			&& Logic.Interact(Player, TEXT("S05")).Result.InteractionId == TEXT("X005")
			&& Logic.ReturnToHuman(Player) && Logic.Interact(Player, TEXT("S06")).Result.InteractionId == TEXT("X006") && Becomes(TEXT("F05"))
			&& Logic.Interact(Player, TEXT("S09")).Result.InteractionId == TEXT("X013") && Becomes(TEXT("F07"));
	}

	/** 讓 Player 變成椅子，再連碰配電箱（X025 每次 -1）直到全隊生命值歸零。回傳 E05 發生的那次互動結果。 */
	FInteractOutcome DriveToE05(FEscapeGameLogic& Logic, const FString& Player)
	{
		if (Logic.GetPlayerForm(Player) != TEXT("F12")) { Logic.Interact(Player, TEXT("F12")); }
		FInteractOutcome Last;
		for (int32 I = 0; I < FTeamState::StartingHp + 1; ++I)
		{
			Last = Logic.Interact(Player, TEXT("S15"));
			for (const FTeamEvent& E : Last.Events) { if (E.EndingId == TEXT("E05")) { return Last; } }
		}
		return Last;
	}

	bool HasEvent(const FInteractOutcome& Out, const FString& Ending)
	{
		for (const FTeamEvent& E : Out.Events) { if (E.EndingId == Ending) { return true; } }
		return false;
	}

	/** 這個 outcome 裡 PlayerId 的新成就 ID，依原本順序 */
	TArray<FString> AchievementsOf(const FInteractOutcome& Out, const FString& PlayerId)
	{
		TArray<FString> Ids;
		for (const FAchievementUnlock& U : Out.NewAchievements) { if (U.PlayerId == PlayerId) { Ids.Add(U.AchievementId); } }
		return Ids;
	}

	/** 改掉當局密碼的一位 → 保證是錯的 */
	FString WrongCode(const FString& Password)
	{
		FString Wrong = Password;
		Wrong[0] = (Wrong[0] == TEXT('0')) ? TEXT('1') : TEXT('0');
		return Wrong;
	}

	// ---- Widget ----

	struct FWidgetBundle
	{
		TArray<UWidget*> All;
		TArray<UTextBlock*> Texts;
		TArray<UButton*> Buttons;
		TArray<USizeBox*> Boxes;
		TArray<FString> TextStrings;
	};

	/** 建 widget（不需要 PlayerController）並收集 WidgetTree 裡的所有東西 */
	template <typename T>
	T* MakeWidget(UObject* Outer, TArray<TStrongObjectPtr<UObject>>& Keep)
	{
		T* W = NewObject<T>(Outer);
		Keep.Add(TStrongObjectPtr<UObject>(W));
		W->Initialize();
		return W;
	}

	FWidgetBundle Collect(UUserWidget* W)
	{
		FWidgetBundle B;
		if (W && W->WidgetTree) { W->WidgetTree->GetAllWidgets(B.All); }
		for (UWidget* Child : B.All)
		{
			if (UTextBlock* T = Cast<UTextBlock>(Child)) { B.Texts.Add(T); B.TextStrings.Add(T->GetText().ToString()); }
			if (UButton* Btn = Cast<UButton>(Child)) { B.Buttons.Add(Btn); }
			if (USizeBox* Box = Cast<USizeBox>(Child)) { B.Boxes.Add(Box); }
		}
		return B;
	}

	/** 量 Text 在 MaxWidth 自動換行後有幾行（用 FEscapeFonts 的字型與 STextBlock 的 WrapTextAt） */
	int32 CountLines(const FString& Text, int32 FontSize, float MaxWidth)
	{
		const FSlateFontInfo& Font = FEscapeFonts::GetFont(FontSize);
		TSharedRef<STextBlock> One = SNew(STextBlock).Text(FText::FromString(TEXT("人"))).Font(Font);
		One->SlatePrepass(1.0f);
		const float LineHeight = One->GetDesiredSize().Y;
		TSharedRef<STextBlock> Block = SNew(STextBlock).Text(FText::FromString(Text)).Font(Font).WrapTextAt(MaxWidth);
		Block->SlatePrepass(1.0f);
		return (LineHeight > 0.0f) ? FMath::RoundToInt(Block->GetDesiredSize().Y / LineHeight) : 0;
	}

	/** 檢查一個 widget 的「不截斷」結構：文字自動換行、不是省略號、沒有裁切、沒有固定高度、用我們的字型 */
	void CheckNoTruncationStructure(FAutomationTestBase& T, const FString& Label, const FWidgetBundle& B, int32 MinTexts, int32 FontSize)
	{
		T.TestTrue(*FString::Printf(TEXT("%s：WidgetTree 有內容（至少 %d 個 TextBlock，實際 %d）"), *Label, MinTexts, B.Texts.Num()), B.Texts.Num() >= MinTexts);
		const FSlateFontInfo& Ours = FEscapeFonts::GetFont(FontSize);
		for (UTextBlock* Text : B.Texts)
		{
			const FString Name = FString::Printf(TEXT("%s／%s"), *Label, *Text->GetName());
			T.TestTrue(*(Name + TEXT(" 自動換行")), Text->GetAutoWrapText());
			T.TestTrue(*(Name + TEXT(" 溢位時不用省略號取代文字")),
				Text->GetTextOverflowPolicy() != ETextOverflowPolicy::Ellipsis && Text->GetTextOverflowPolicy() != ETextOverflowPolicy::MultilineEllipsis);
			T.TestTrue(*(Name + TEXT(" 用 FEscapeFonts 的字型（TC＋表情符號）")),
				Text->GetFont().CompositeFont.IsValid() && Text->GetFont().CompositeFont == Ours.CompositeFont);
		}
		for (UWidget* Child : B.All)
		{
			T.TestTrue(*FString::Printf(TEXT("%s／%s 沒有裁切（Clipping＝Inherit）"), *Label, *Child->GetName()), Child->GetClipping() == EWidgetClipping::Inherit);
		}
		for (USizeBox* Box : B.Boxes)
		{
			const FString Name = FString::Printf(TEXT("%s／%s"), *Label, *Box->GetName());
			T.TestTrue(*(Name + TEXT(" 沒有固定高度")), Box->GetHeightOverride() <= 0.0f);
			T.TestTrue(*(Name + TEXT(" 沒有最小高度")), Box->GetMinDesiredHeight() <= 0.0f);
			T.TestTrue(*(Name + TEXT(" 沒有最大高度")), Box->GetMaxDesiredHeight() <= 0.0f);
		}
	}

	bool HasMaxWidthBox(const FWidgetBundle& B, float Width)
	{
		for (USizeBox* Box : B.Boxes) { if (FMath::IsNearlyEqual(Box->GetMaxDesiredWidth(), Width)) { return true; } }
		return false;
	}

	bool HasText(const FWidgetBundle& B, const FString& Exact)
	{
		return B.TextStrings.Contains(Exact);
	}

	bool AnyTextContains(const FWidgetBundle& B, const FString& Part)
	{
		for (const FString& S : B.TextStrings) { if (S.Contains(Part)) { return true; } }
		return false;
	}

	/** 字型裡找檔名結尾符合的子字型 */
	const FTypefaceEntry* FindEntry(const FTypeface& Face, const FString& FileName)
	{
		for (const FTypefaceEntry& E : Face.Fonts) { if (E.Font.GetFontFilename().EndsWith(FileName)) { return &E; } }
		return nullptr;
	}
}

BEGIN_DEFINE_SPEC(FEscapeGameStep10Spec, "EscapeGame.Step10",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep10Spec)

void FEscapeGameStep10Spec::Define()
{
	using namespace Step10Spec;

	Describe(TEXT("VERIFY STEP 10"), [this]()
	{
		// ================= 1. HUD =================

		It(TEXT("畫面上常駐顯示生命值、剩餘時間（mm:ss）、目前形態名稱，都是繁體中文、沒有方塊字"), [this]()
		{
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("載入文字 %s"), *Error), MakeUi(Ui, Error));

			const FString Human = FormName(TEXT("F00"));
			TestEqual(TEXT("DT_Forms 的 F00 叫人類"), Human, FString(TEXT("人類")));
			const FHudView Hud = Ui.MakeHud(5, 1799, Human);
			const FString HpFormat = UiText(TEXT("HudHp"));
			const FString FormFormat = UiText(TEXT("HudForm"));
			TestTrue(TEXT("UiText HudHp 含 {0}"), HpFormat.Contains(TEXT("{0}")));
			TestTrue(TEXT("UiText HudForm 含 {0}"), FormFormat.Contains(TEXT("{0}")));
			TestTrue(TEXT("生命值文字含「5」"), Hud.HpText.Contains(TEXT("5")));
			TestEqual(TEXT("生命值文字＝UiText 格式代入 5"), Hud.HpText, HpFormat.Replace(TEXT("{0}"), TEXT("5")));
			TestEqual(TEXT("1799 秒顯示 29:59"), Hud.TimeText, FString(TEXT("29:59")));
			TestEqual(TEXT("時間文字＝FormatClock"), Hud.TimeText, FTeamState::FormatClock(1799));
			TestTrue(TEXT("形態文字含「人類」"), Hud.FormText.Contains(TEXT("人類")));
			TestEqual(TEXT("形態文字＝UiText 格式代入形態名稱"), Hud.FormText, FormFormat.Replace(TEXT("{0}"), *Human));
			TestTrue(TEXT("生命值文字有中文（繁體）"), Hud.HpText.Contains(TEXT("生命")));
			TestTrue(TEXT("形態文字有中文（繁體）"), Hud.FormText.Contains(TEXT("形態")));

			const FHudView Other = Ui.MakeHud(1, 59, FormName(TEXT("F09")));
			TestEqual(TEXT("59 秒顯示 00:59"), Other.TimeText, FString(TEXT("00:59")));
			TestTrue(TEXT("生命值 1"), Other.HpText.Contains(TEXT("1")));
			TestTrue(TEXT("形態名稱是 DT_Forms 的 F09 名稱"), !FormName(TEXT("F09")).IsEmpty() && Other.FormText.Contains(FormName(TEXT("F09"))));
		});

		It(TEXT("字型：預設字型是 Noto Sans CJK TC（TTC 的 SubFaceIndex 3），表情符號字型當 fallback，字型檔存在"), [this]()
		{
			const FSlateFontInfo& Font = FEscapeFonts::GetFont(24);
			TestTrue(TEXT("GetFont(24) 的 CompositeFont 有效"), Font.CompositeFont.IsValid());
			TestEqual(TEXT("字級 24"), Font.Size, 24.0f);
			TestTrue(TEXT("同一個大小回傳同一份 CompositeFont"), Font.CompositeFont == FEscapeFonts::GetFont(24).CompositeFont);
			TestEqual(TEXT("字級 30"), FEscapeFonts::GetFont(30).Size, 30.0f);
			if (!Font.CompositeFont.IsValid()) { return; }

			const FCompositeFont& Composite = *Font.CompositeFont;
			const FTypefaceEntry* Cjk = FindEntry(Composite.DefaultTypeface, FEscapeFonts::CjkFontFile());
			TestNotNull(TEXT("預設字型有 NotoSansCJK-Regular.ttc"), Cjk);
			if (Cjk)
			{
				TestEqual(TEXT("用 TTC 的第 3 個字型（Noto Sans CJK TC）"), Cjk->Font.GetSubFaceIndex(), 3);
				TestEqual(TEXT("SubFaceIndex 常數"), FEscapeFonts::TcSubFaceIndex, 3);
				TestTrue(TEXT("CJK 字型檔存在"), FPaths::FileExists(Cjk->Font.GetFontFilename()));
			}

			const FTypefaceEntry* Emoji = FindEntry(Composite.DefaultTypeface, FEscapeFonts::EmojiFontFile());
			if (!Emoji) { Emoji = FindEntry(Composite.FallbackTypeface.Typeface, FEscapeFonts::EmojiFontFile()); }
			for (const FCompositeSubFont& Sub : Composite.SubTypefaces)
			{
				if (!Emoji) { Emoji = FindEntry(Sub.Typeface, FEscapeFonts::EmojiFontFile()); }
			}
			TestNotNull(TEXT("字型組合裡有 NotoColorEmoji.ttf（DefaultTypeface、FallbackTypeface 或 SubTypefaces）"), Emoji);
			if (Emoji) { TestTrue(TEXT("表情符號字型檔存在"), FPaths::FileExists(Emoji->Font.GetFontFilename())); }
		});

		// ================= 2. 彈出框 =================

		It(TEXT("互動時 → 彈出文字框顯示事件文字，3 秒後關閉（差一點到還在、滿 3 秒關）"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			TestFalse(TEXT("一開始沒有彈出框"), Ui.IsPopupVisible());

			// Blocked／Rejected 什麼都不做
			FInteractOutcome Blocked;
			Blocked.Kind = EInteractKind::Blocked;
			Blocked.Messages = { TEXT("不該顯示的文字") };
			Ui.OnOutcome(Blocked, TEXT("P1"));
			FInteractOutcome Rejected;
			Rejected.Kind = EInteractKind::Rejected;
			Rejected.Messages = { TEXT("不該顯示的文字") };
			Ui.OnOutcome(Rejected, TEXT("P1"));
			TestFalse(TEXT("Blocked 與 Rejected 不彈出"), Ui.IsPopupVisible());

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S14"));
			TestEqual(TEXT("人類碰黑板是 X018"), Out.Result.InteractionId, FString(TEXT("X018")));
			TestTrue(TEXT("Resolved 且有文字"), Out.Kind == EInteractKind::Resolved && Out.Messages.Num() >= 1);
			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("彈出框出現"), Ui.IsPopupVisible());
			TestTrue(TEXT("顯示 X018 的文字（黑板上的密文）"), Ui.GetPopupText().Contains(TEXT("WKH SDVVZRUG LV QRW KHUH")));
			TestTrue(TEXT("顯示 X018 的文字（開頭）"), Ui.GetPopupText().Contains(TEXT("黑板上寫著一串奇怪的字")));
			if (Out.Messages.Num() >= 1) { TestEqual(TEXT("文字＝outcome 的第一則"), Ui.GetPopupText(), Out.Messages[0]); }
			TestEqual(TEXT("彈出框秒數是 3"), FEscapeUiPresenter::PopupSeconds, 3.0f);

			Ui.Tick(2.9f);
			TestTrue(TEXT("2.9 秒還在"), Ui.IsPopupVisible());
			Ui.Tick(0.1f);
			TestFalse(TEXT("3.0 秒後關閉"), Ui.IsPopupVisible());
			TestEqual(TEXT("關閉後文字是空的"), Ui.GetPopupText(), FString());
		});

		It(TEXT("互動時 → 彈出文字框顯示事件文字，按任意鍵立刻關閉"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);

			Ui.OnAnyKey();  // 沒有彈出框時按鍵：什麼都不會發生
			TestFalse(TEXT("沒有彈出框時按任意鍵沒事"), Ui.IsPopupVisible());

			Ui.OnOutcome(Logic.Interact(TEXT("P1"), TEXT("S14")), TEXT("P1"));
			TestTrue(TEXT("彈出框出現"), Ui.IsPopupVisible());
			Ui.Tick(1.0f);
			TestTrue(TEXT("1 秒時還在"), Ui.IsPopupVisible());
			Ui.OnAnyKey();
			TestFalse(TEXT("按任意鍵立刻關閉"), Ui.IsPopupVisible());
			Ui.Tick(5.0f);
			TestFalse(TEXT("之後也不會再出現"), Ui.IsPopupVisible());
		});

		It(TEXT("互動產生兩則文字（X017 擲中「手指抽筋了」）→ 依序各顯示 3 秒，按任意鍵跳到下一則"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(20261031, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);

			FInteractOutcome Hit;
			for (int32 I = 0; I < 100000 && Hit.Messages.Num() < 2; ++I)
			{
				Hit = Logic.SubmitPassword(TEXT("P1"), WrongCode(Logic.GetTeam().GetPasswordForDevOnly()));
			}
			TestEqual(TEXT("找到擲中的那一次：兩則文字"), Hit.Messages.Num(), 2);
			if (Hit.Messages.Num() < 2) { return; }
			TestTrue(TEXT("第一則是「門紋絲不動」"), Hit.Messages[0].Contains(TEXT("門紋絲不動")));
			TestTrue(TEXT("第二則是「手指抽筋了」"), Hit.Messages[1].Contains(TEXT("手指抽筋了")));

			Ui.OnOutcome(Hit, TEXT("P1"));
			TestTrue(TEXT("彈出框出現"), Ui.IsPopupVisible());
			TestEqual(TEXT("先顯示第一則"), Ui.GetPopupText(), Hit.Messages[0]);
			Ui.Tick(2.9f);
			TestEqual(TEXT("2.9 秒還是第一則"), Ui.GetPopupText(), Hit.Messages[0]);
			Ui.Tick(0.2f);
			TestTrue(TEXT("3 秒後換第二則"), Ui.IsPopupVisible());
			TestEqual(TEXT("第二則文字"), Ui.GetPopupText(), Hit.Messages[1]);
			Ui.Tick(2.9f);
			TestEqual(TEXT("第二則也顯示滿 3 秒"), Ui.GetPopupText(), Hit.Messages[1]);
			Ui.Tick(0.2f);
			TestFalse(TEXT("兩則都顯示完 → 關閉"), Ui.IsPopupVisible());

			// 按任意鍵跳到下一則
			Ui.OnOutcome(Hit, TEXT("P1"));
			TestEqual(TEXT("再來一次：第一則"), Ui.GetPopupText(), Hit.Messages[0]);
			Ui.OnAnyKey();
			TestTrue(TEXT("按任意鍵後還有彈出框（下一則）"), Ui.IsPopupVisible());
			TestEqual(TEXT("按任意鍵後顯示第二則"), Ui.GetPopupText(), Hit.Messages[1]);
			Ui.OnAnyKey();
			TestFalse(TEXT("再按一次 → 全部關閉"), Ui.IsPopupVisible());
		});

		// ================= 3. 鍵盤 =================

		It(TEXT("人類碰大門 → 出現四位數密碼鍵盤，可以輸入 0～9、刪除、確認"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(3, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			TestFalse(TEXT("一開始沒有鍵盤"), Ui.IsKeypadVisible());

			Ui.KeypadDigit(7);  // 鍵盤沒開時輸入不作用
			TestEqual(TEXT("鍵盤沒開時輸入不作用"), Ui.GetKeypadEntry(), FString());

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestTrue(TEXT("人類碰大門 → NeedsPassword"), Out.Kind == EInteractKind::NeedsPassword);
			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("鍵盤出現"), Ui.IsKeypadVisible());
			TestEqual(TEXT("一開始輸入是空的"), Ui.GetKeypadEntry(), FString());

			for (int32 D = 1; D <= 5; ++D) { Ui.KeypadDigit(D); }
			TestEqual(TEXT("輸入 1、2、3、4、5 → 只留 1234（第 5 位忽略）"), Ui.GetKeypadEntry(), FString(TEXT("1234")));
			Ui.KeypadDelete();
			TestEqual(TEXT("刪除 → 123"), Ui.GetKeypadEntry(), FString(TEXT("123")));

			FString Code = TEXT("untouched");
			TestFalse(TEXT("3 位確認 → false"), Ui.KeypadConfirm(Code));
			TestTrue(TEXT("確認失敗後鍵盤還開著"), Ui.IsKeypadVisible());
			TestEqual(TEXT("確認失敗後輸入還在"), Ui.GetKeypadEntry(), FString(TEXT("123")));

			Ui.KeypadDigit(4);
			TestTrue(TEXT("補一位後確認 → true"), Ui.KeypadConfirm(Code));
			TestEqual(TEXT("得到 4 位碼 1234"), Code, FString(TEXT("1234")));
			TestFalse(TEXT("確認後鍵盤關閉"), Ui.IsKeypadVisible());
			TestEqual(TEXT("確認後輸入清空"), Ui.GetKeypadEntry(), FString());
		});

		It(TEXT("鍵盤可以輸入 0～9 每個數字（含 0000）、刪除到空再刪不會壞、取消會關閉、按任意鍵不會關掉鍵盤"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(3, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S01"));
			TestTrue(TEXT("NeedsPassword"), Out.Kind == EInteractKind::NeedsPassword);

			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("鍵盤出現"), Ui.IsKeypadVisible());
			Ui.OnAnyKey();
			TestTrue(TEXT("按任意鍵不會關掉鍵盤"), Ui.IsKeypadVisible());
			Ui.Tick(10.0f);
			TestTrue(TEXT("鍵盤不會因為時間關閉"), Ui.IsKeypadVisible());

			Ui.KeypadDelete();
			Ui.KeypadDelete();
			TestEqual(TEXT("空的時候刪除沒事"), Ui.GetKeypadEntry(), FString());
			for (int32 D : { 0, 1, 2, 3 }) { Ui.KeypadDigit(D); }
			TestEqual(TEXT("0123"), Ui.GetKeypadEntry(), FString(TEXT("0123")));
			Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete();
			for (int32 D : { 4, 5, 6, 7 }) { Ui.KeypadDigit(D); }
			TestEqual(TEXT("4567"), Ui.GetKeypadEntry(), FString(TEXT("4567")));
			Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete();
			for (int32 D : { 8, 9, 0, 0 }) { Ui.KeypadDigit(D); }
			TestEqual(TEXT("8900"), Ui.GetKeypadEntry(), FString(TEXT("8900")));
			Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete(); Ui.KeypadDelete();
			for (int32 D : { 0, 0, 0, 0 }) { Ui.KeypadDigit(D); }
			FString Code;
			TestTrue(TEXT("0000 可以確認"), Ui.KeypadConfirm(Code));
			TestEqual(TEXT("0000 的碼"), Code, FString(TEXT("0000")));

			Ui.OnOutcome(Out, TEXT("P1"));
			Ui.KeypadDigit(5);
			Ui.KeypadCancel();
			TestFalse(TEXT("取消 → 鍵盤關閉"), Ui.IsKeypadVisible());
			Ui.OnOutcome(Out, TEXT("P1"));
			TestEqual(TEXT("再開時輸入是空的"), Ui.GetKeypadEntry(), FString());
		});

		It(TEXT("人類碰大門 → 鍵盤輸入正確密碼並確認 → 顯示 E02 結局畫面，鍵盤關閉"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(11, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);

			Ui.OnOutcome(Logic.Interact(TEXT("P1"), TEXT("S01")), TEXT("P1"));
			TestTrue(TEXT("鍵盤出現"), Ui.IsKeypadVisible());
			const FString Password = Logic.GetTeam().GetPasswordForDevOnly();
			for (TCHAR C : Password) { Ui.KeypadDigit(C - TEXT('0')); }
			FString Code;
			TestTrue(TEXT("確認"), Ui.KeypadConfirm(Code));
			TestEqual(TEXT("碼＝當局密碼"), Code, Password);
			TestFalse(TEXT("確認後鍵盤關閉"), Ui.IsKeypadVisible());

			const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Code);
			TestTrue(TEXT("E02 事件"), HasEvent(Out, TEXT("E02")));
			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("結局畫面出現"), Ui.IsEndingVisible());
			TestEqual(TEXT("結局 E02"), Ui.GetEndingId(), FString(TEXT("E02")));
			TestFalse(TEXT("鍵盤沒有再開"), Ui.IsKeypadVisible());
		});

		// ================= 4. 結局 =================

		It(TEXT("觸發結局 E01～E03 → 顯示對應的結局畫面（標題是結局成就名稱）與「夢醒」劇情文字"), [this]()
		{
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			const auto Endings = LoadTable(TEXT("DT_Endings.csv"));
			TestEqual(TEXT("DT_Endings 有 5 個結局"), Endings.Num(), 5);
			TestFalse(TEXT("一開始沒有結局畫面"), Ui.IsEndingVisible());

			const auto Check = [&](const TCHAR* Id, const TCHAR* Title, const FInteractOutcome& Out, const FEscapeUiPresenter& P)
			{
				TestTrue(*FString::Printf(TEXT("%s：HasEvent"), Id), HasEvent(Out, Id));
				TestTrue(*FString::Printf(TEXT("%s：結局畫面出現"), Id), P.IsEndingVisible());
				TestEqual(*FString::Printf(TEXT("%s：結局 ID"), Id), P.GetEndingId(), FString(Id));
				TestEqual(*FString::Printf(TEXT("%s：標題是結局成就名稱"), Id), P.GetEndingTitle(), FString(Title));
				const TMap<FString, FString>* Row = Endings.Find(Id);
				TestNotNull(*FString::Printf(TEXT("%s：DT_Endings 有這列"), Id), Row);
				if (Row)
				{
					TestEqual(*FString::Printf(TEXT("%s：標題＝DT_Endings 的 Title"), Id), P.GetEndingTitle(), Row->FindRef(TEXT("Title")));
					TestEqual(*FString::Printf(TEXT("%s：劇情＝DT_Endings 的 Story"), Id), P.GetEndingStory(), Row->FindRef(TEXT("Story")));
				}
				TestTrue(*FString::Printf(TEXT("%s：劇情是「夢醒」，有老師說的那句"), Id), P.GetEndingStory().Contains(TEXT("好，那我們今天就上到這邊，下課。")));
				TestFalse(*FString::Printf(TEXT("%s：不是 E04 的劇情"), Id), P.GetEndingStory().Contains(TEXT("睡得很熟嘛")));
			};

			// E01：P2 先用密碼逃出 → P1 的畫面不顯示 P2 的結局；P1 再用學生證逃出 → 顯示
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
				FEscapeUiPresenter P1Ui;
				TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(P1Ui, Error));
				const FInteractOutcome P2Out = Logic.SubmitPassword(TEXT("P2"), Logic.GetTeam().GetPasswordForDevOnly());
				TestTrue(TEXT("P2 逃出 E02"), HasEvent(P2Out, TEXT("E02")));
				P1Ui.OnOutcome(P2Out, TEXT("P1"));
				TestFalse(TEXT("別人的結局不會讓我的畫面出現結局"), P1Ui.IsEndingVisible());

				TestTrue(TEXT("P1 變成學生證"), BecomeStudentCard(Logic, TEXT("P1")));
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S01"));
				P1Ui.OnOutcome(Out, TEXT("P1"));
				Check(TEXT("E01"), TEXT("撿到就是我的"), Out, P1Ui);
			}
			// E02
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(7, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				FEscapeUiPresenter P;
				TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(P, Error));
				const FInteractOutcome Out = Logic.SubmitPassword(TEXT("P1"), Logic.GetTeam().GetPasswordForDevOnly());
				P.OnOutcome(Out, TEXT("P1"));
				Check(TEXT("E02"), TEXT("密室逃脫大神"), Out, P);
			}
			// E03
			{
				FEscapeGameLogic Logic;
				TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
				Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
				FEscapeUiPresenter P;
				TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(P, Error));
				TestTrue(TEXT("P1 變成長竿子"), BecomeLongPole(Logic, TEXT("P1")));
				const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("S13"));
				P.OnOutcome(Out, TEXT("P1"));
				Check(TEXT("E03"), TEXT("誰說一定要走正門"), Out, P);
			}
		});

		It(TEXT("觸發結局 E04（時鐘自身互動 X021）→ 結局畫面與「被叫醒」劇情文字"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 變成時鐘"), Logic.Interact(TEXT("P1"), TEXT("F09")).bTransformed);
			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("SELF"));
			TestTrue(TEXT("E04 事件"), HasEvent(Out, TEXT("E04")));

			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("結局畫面出現"), Ui.IsEndingVisible());
			TestEqual(TEXT("結局 E04"), Ui.GetEndingId(), FString(TEXT("E04")));
			TestEqual(TEXT("標題是 E04 的結局成就名稱"), Ui.GetEndingTitle(), FString(TEXT("跟無數個趕作業的日子一樣，又看到了清晨的太陽")));
			TestTrue(TEXT("劇情：老師說的那句"), Ui.GetEndingStory().Contains(TEXT("同學，下課了喔，睡得很熟嘛。")));
			TestTrue(TEXT("劇情：窗外的陽光"), Ui.GetEndingStory().Contains(TEXT("窗外透進清晨的陽光")));
			const auto Endings = LoadTable(TEXT("DT_Endings.csv"));
			const TMap<FString, FString>* Row = Endings.Find(TEXT("E04"));
			TestNotNull(TEXT("DT_Endings 有 E04"), Row);
			if (Row)
			{
				TestEqual(TEXT("標題＝DT_Endings 的 Title"), Ui.GetEndingTitle(), Row->FindRef(TEXT("Title")));
				TestEqual(TEXT("劇情＝DT_Endings 的 Story"), Ui.GetEndingStory(), Row->FindRef(TEXT("Story")));
			}
		});

		It(TEXT("觸發結局 E05（生命值歸零）→ 結局畫面與「遊戲重啟」文字"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			const FInteractOutcome Out = DriveToE05(Logic, TEXT("P1"));
			TestTrue(TEXT("E05 事件"), HasEvent(Out, TEXT("E05")));

			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("結局畫面出現"), Ui.IsEndingVisible());
			TestEqual(TEXT("結局 E05"), Ui.GetEndingId(), FString(TEXT("E05")));
			TestEqual(TEXT("標題"), Ui.GetEndingTitle(), FString(TEXT("上課睡覺的代價")));
			TestEqual(TEXT("劇情"), Ui.GetEndingStory(), FString(TEXT("遊戲重啟，從第一關重新開始。")));
		});

		It(TEXT("時間到的 E04 與 Tick 來的事件／成就：OnTeamEvents 顯示結局畫面、OnAchievements 只跳我的成就"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			// 只影響 P2 的事件：P1 的畫面不顯示
			FTeamEvent OnlyP2;
			OnlyP2.EndingId = TEXT("E04");
			OnlyP2.AffectedPlayers = { TEXT("P2") };
			Ui.OnTeamEvents({ OnlyP2 }, TEXT("P1"));
			TestFalse(TEXT("只影響 P2 → P1 沒有結局畫面"), Ui.IsEndingVisible());

			const TArray<FTeamEvent> Events = Logic.Tick(static_cast<float>(FTeamState::RoundSeconds) + 1.0f);
			const TArray<FAchievementUnlock> Unlocks = Logic.PopNewAchievements();
			TestTrue(TEXT("Tick 到時間 → E04 事件"), Events.Num() == 1 && Events[0].EndingId == TEXT("E04"));
			TestEqual(TEXT("兩位玩家各一筆 E04 成就"), Unlocks.Num(), 2);

			Ui.OnTeamEvents(Events, TEXT("P1"));
			TestTrue(TEXT("結局畫面出現"), Ui.IsEndingVisible());
			TestEqual(TEXT("結局 E04"), Ui.GetEndingId(), FString(TEXT("E04")));
			TestTrue(TEXT("劇情是被叫醒"), Ui.GetEndingStory().Contains(TEXT("睡得很熟嘛")));

			Ui.OnAchievements(Unlocks, TEXT("P1"));
			TestTrue(TEXT("成就提示出現（P1 的 E04）"), Ui.IsToastVisible());
			TestEqual(TEXT("提示是 E04 的成就名稱"), Ui.GetToastText(), FString(TEXT("跟無數個趕作業的日子一樣，又看到了清晨的太陽")));
			Ui.Tick(3.1f);
			TestFalse(TEXT("P2 的 E04 不會跳給 P1：3 秒後沒有下一則"), Ui.IsToastVisible());

			// 手動給：A03（P1）、A03（P2）→ 只跳 P1 的
			Ui.OnAchievements({ { TEXT("P2"), TEXT("A03") }, { TEXT("P1"), TEXT("A03") } }, TEXT("P1"));
			TestTrue(TEXT("A03 提示出現"), Ui.IsToastVisible());
			TestEqual(TEXT("A03「魂不附體」"), Ui.GetToastText(), FString(TEXT("魂不附體")));
			Ui.Tick(3.1f);
			TestFalse(TEXT("只有一則"), Ui.IsToastVisible());
		});

		// ================= 5. 成就提示 =================

		It(TEXT("解鎖成就 → 右上角跳出成就名稱，3 秒後消失（2 秒內不消失、4 秒後一定消失），多個依序顯示"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));
			TestTrue(TEXT("P1 變成時鐘"), Logic.Interact(TEXT("P1"), TEXT("F09")).bTransformed);
			Logic.PopNewAchievements();
			TestFalse(TEXT("一開始沒有提示"), Ui.IsToastVisible());

			const FInteractOutcome Out = Logic.Interact(TEXT("P1"), TEXT("SELF"));
			const TArray<FString> Mine = AchievementsOf(Out, TEXT("P1"));
			TestEqual(TEXT("X021：P1 解鎖 A06 與 E04 兩個"), Mine.Num(), 2);
			TestEqual(TEXT("P2 也解鎖了一個 E04（不該跳給 P1）"), AchievementsOf(Out, TEXT("P2")).Num(), 1);
			TestTrue(TEXT("提示秒數在 2～4 秒之間"), FEscapeUiPresenter::ToastSeconds >= 2.0f && FEscapeUiPresenter::ToastSeconds <= 4.0f);
			if (Mine.Num() != 2) { return; }

			const auto Achievements = LoadTable(TEXT("DT_Achievements.csv"));
			const auto TitleOf = [&](const FString& Id) { const TMap<FString, FString>* Row = Achievements.Find(Id); return Row ? Row->FindRef(TEXT("Title")) : FString(); };
			TestEqual(TEXT("A06 的名稱「時間管理大師」"), TitleOf(TEXT("A06")), FString(TEXT("時間管理大師")));
			TestEqual(TEXT("E04 的名稱"), TitleOf(TEXT("E04")), FString(TEXT("跟無數個趕作業的日子一樣，又看到了清晨的太陽")));

			Ui.OnOutcome(Out, TEXT("P1"));
			TestTrue(TEXT("提示出現"), Ui.IsToastVisible());
			TestEqual(TEXT("先跳第一個（照 outcome 的順序）"), Ui.GetToastText(), TitleOf(Mine[0]));
			TestTrue(TEXT("第一個的名稱不是空的"), !Ui.GetToastText().IsEmpty());
			Ui.Tick(1.9f);
			TestTrue(TEXT("1.9 秒還在"), Ui.IsToastVisible());
			TestEqual(TEXT("1.9 秒還是第一個"), Ui.GetToastText(), TitleOf(Mine[0]));
			Ui.Tick(1.2f);
			TestTrue(TEXT("3 秒後換下一個"), Ui.IsToastVisible());
			TestEqual(TEXT("第二個提示"), Ui.GetToastText(), TitleOf(Mine[1]));
			Ui.Tick(1.9f);
			TestEqual(TEXT("第二個也顯示滿 3 秒"), Ui.GetToastText(), TitleOf(Mine[1]));
			Ui.Tick(1.2f);
			TestFalse(TEXT("全部顯示完 → 消失（P2 的 E04 不會跳給 P1）"), Ui.IsToastVisible());
			TestEqual(TEXT("消失後文字是空的"), Ui.GetToastText(), FString());
		});

		It(TEXT("已經解鎖過的成就不會再跳提示（outcome 的 NewAchievements 是空的 → 沒有提示）；成就提示和彈出框互不影響"), [this]()
		{
			FEscapeGameLogic Logic;
			FEscapeUiPresenter Ui;
			FString Error;
			TestTrue(*FString::Printf(TEXT("CSV 載入 %s"), *Error), MakeLogic(1, Logic, Error));
			TestTrue(*FString::Printf(TEXT("文字載入 %s"), *Error), MakeUi(Ui, Error));
			Logic.AddPlayer(TEXT("P1"), FVector::ZeroVector);
			Logic.AddPlayer(TEXT("P2"), FVector(0, 50, 0));

			FInteractOutcome Taunt;
			for (int32 I = 0; I < 10; ++I) { Taunt = Logic.Interact(TEXT("P1"), TEXT("S16")); }  // 第 10 次預設嘲諷 → A01
			TestEqual(TEXT("第 10 次嘲諷解鎖 A01"), AchievementsOf(Taunt, TEXT("P1")), TArray<FString>({ TEXT("A01") }));
			Ui.OnOutcome(Taunt, TEXT("P1"));
			TestTrue(TEXT("提示出現"), Ui.IsToastVisible());
			TestEqual(TEXT("A01「被嘲笑專家」"), Ui.GetToastText(), FString(TEXT("被嘲笑專家")));
			TestTrue(TEXT("同時也有彈出框（嘲諷文字）"), Ui.IsPopupVisible());
			Ui.OnAnyKey();
			TestFalse(TEXT("按任意鍵關掉彈出框"), Ui.IsPopupVisible());
			TestTrue(TEXT("成就提示不受按鍵影響"), Ui.IsToastVisible());
			Ui.Tick(3.1f);
			TestFalse(TEXT("3 秒後提示消失"), Ui.IsToastVisible());

			const FInteractOutcome Eleventh = Logic.Interact(TEXT("P1"), TEXT("S16"));
			TestEqual(TEXT("第 11 次沒有新成就"), Eleventh.NewAchievements.Num(), 0);
			Ui.OnOutcome(Eleventh, TEXT("P1"));
			TestFalse(TEXT("沒有新成就 → 沒有提示"), Ui.IsToastVisible());
			TestTrue(TEXT("但嘲諷文字照常彈出"), Ui.IsPopupVisible());
		});

		It(TEXT("成就提示出現在右上角，彈出框在畫面中下（視口錨點）"), [this]()
		{
			const FAnchors ToastAnchors = UEscapeToastWidget::ViewportAnchors();
			TestTrue(TEXT("成就提示錨點在右上角 (1,0)"), ToastAnchors.Minimum == FVector2D(1.0, 0.0) && ToastAnchors.Maximum == FVector2D(1.0, 0.0));
			TestTrue(TEXT("成就提示對齊右上角 (1,0)"), UEscapeToastWidget::ViewportAlignment() == FVector2D(1.0, 0.0));
			const FAnchors PopupAnchors = UEscapePopupWidget::ViewportAnchors();
			TestTrue(TEXT("彈出框錨點在下方中間 (0.5,1)"), PopupAnchors.Minimum == FVector2D(0.5, 1.0) && PopupAnchors.Maximum == FVector2D(0.5, 1.0));
			TestTrue(TEXT("彈出框對齊下方中間 (0.5,1)"), UEscapePopupWidget::ViewportAlignment() == FVector2D(0.5, 1.0));
		});

		// ================= 6. 不可以截斷 =================

		It(TEXT("不可以：任何文字超出文字框或被截斷（widget 結構：自動換行、不裁切、不限高度、用 TC＋表情符號字型）"), [this]()
		{
			FTestWorldWrapper World;
			TestTrue(TEXT("建立測試世界"), World.CreateTestWorld(EWorldType::Game));
			UObject* Outer = World.GetTestWorld() ? static_cast<UObject*>(World.GetTestWorld()) : static_cast<UObject*>(GetTransientPackage());
			TArray<TStrongObjectPtr<UObject>> Keep;

			UEscapeHudWidget* Hud = MakeWidget<UEscapeHudWidget>(Outer, Keep);
			UEscapePopupWidget* Popup = MakeWidget<UEscapePopupWidget>(Outer, Keep);
			UEscapeKeypadWidget* Keypad = MakeWidget<UEscapeKeypadWidget>(Outer, Keep);
			UEscapeEndingWidget* Ending = MakeWidget<UEscapeEndingWidget>(Outer, Keep);
			UEscapeToastWidget* Toast = MakeWidget<UEscapeToastWidget>(Outer, Keep);

			const FWidgetBundle HudB = Collect(Hud);
			const FWidgetBundle PopupB = Collect(Popup);
			const FWidgetBundle KeypadB = Collect(Keypad);
			const FWidgetBundle EndingB = Collect(Ending);
			const FWidgetBundle ToastB = Collect(Toast);

			// HUD 三行字（沒有固定字級常數：用 widget 自己第一個 TextBlock 的字級）
			const int32 HudSize = HudB.Texts.Num() > 0 ? FMath::RoundToInt(HudB.Texts[0]->GetFont().Size) : 24;
			CheckNoTruncationStructure(*this, TEXT("HUD"), HudB, 3, HudSize);
			CheckNoTruncationStructure(*this, TEXT("彈出框"), PopupB, 1, UEscapePopupWidget::FontSize);
			const int32 KeypadSize = KeypadB.Texts.Num() > 0 ? FMath::RoundToInt(KeypadB.Texts[0]->GetFont().Size) : 24;
			CheckNoTruncationStructure(*this, TEXT("鍵盤"), KeypadB, 14, KeypadSize);  // 標題＋輸入＋0～9＋刪除＋確認
			CheckNoTruncationStructure(*this, TEXT("結局"), EndingB, 2, UEscapeEndingWidget::FontSize);
			CheckNoTruncationStructure(*this, TEXT("成就提示"), ToastB, 1, UEscapeToastWidget::FontSize);

			TestTrue(TEXT("彈出框的 USizeBox 只限寬度（MaxDesiredWidth＝MaxTextWidth）"), HasMaxWidthBox(PopupB, UEscapePopupWidget::MaxTextWidth));
			TestTrue(TEXT("結局的 USizeBox 只限寬度"), HasMaxWidthBox(EndingB, UEscapeEndingWidget::MaxTextWidth));
			TestTrue(TEXT("成就提示的 USizeBox 只限寬度"), HasMaxWidthBox(ToastB, UEscapeToastWidget::MaxTextWidth));
		});

		It(TEXT("Widget 內容：HUD 三行字、彈出框文字、鍵盤 0～9／刪除／確認／目前輸入、結局標題與劇情、成就提示，都照 presenter 給的字顯示"), [this]()
		{
			FTestWorldWrapper World;
			TestTrue(TEXT("建立測試世界"), World.CreateTestWorld(EWorldType::Game));
			UObject* Outer = World.GetTestWorld() ? static_cast<UObject*>(World.GetTestWorld()) : static_cast<UObject*>(GetTransientPackage());
			TArray<TStrongObjectPtr<UObject>> Keep;

			UEscapeHudWidget* Hud = MakeWidget<UEscapeHudWidget>(Outer, Keep);
			FHudView View;
			View.HpText = TEXT("生命值：5");
			View.TimeText = TEXT("29:59");
			View.FormText = TEXT("形態：人類");
			Hud->SetView(View);
			const FWidgetBundle HudB = Collect(Hud);
			TestTrue(TEXT("HUD 有生命值文字"), HasText(HudB, View.HpText));
			TestTrue(TEXT("HUD 有時間文字"), HasText(HudB, View.TimeText));
			TestTrue(TEXT("HUD 有形態文字"), HasText(HudB, View.FormText));

			UEscapePopupWidget* Popup = MakeWidget<UEscapePopupWidget>(Outer, Keep);
			Popup->SetPopupText(TEXT("黑板上寫著一串奇怪的字"));
			TestTrue(TEXT("彈出框有事件文字"), HasText(Collect(Popup), TEXT("黑板上寫著一串奇怪的字")));

			UEscapeKeypadWidget* Keypad = MakeWidget<UEscapeKeypadWidget>(Outer, Keep);
			Keypad->SetEntry(TEXT("1234"));
			const FWidgetBundle KeypadB = Collect(Keypad);
			TestTrue(TEXT("鍵盤至少 12 個按鈕（0～9、刪除、確認）"), KeypadB.Buttons.Num() >= 12);
			for (int32 D = 0; D <= 9; ++D) { TestTrue(*FString::Printf(TEXT("鍵盤有數字 %d"), D), HasText(KeypadB, FString::FromInt(D))); }
			TestTrue(TEXT("鍵盤有「刪除」"), HasText(KeypadB, TEXT("刪除")));
			TestTrue(TEXT("鍵盤有「確認」"), HasText(KeypadB, TEXT("確認")));
			TestTrue(TEXT("鍵盤顯示目前輸入 1234"), AnyTextContains(KeypadB, TEXT("1234")));

			UEscapeEndingWidget* Ending = MakeWidget<UEscapeEndingWidget>(Outer, Keep);
			Ending->SetEnding(TEXT("密室逃脫大神"), TEXT("眼前一片白光。"));
			const FWidgetBundle EndingB = Collect(Ending);
			TestTrue(TEXT("結局畫面有標題"), HasText(EndingB, TEXT("密室逃脫大神")));
			TestTrue(TEXT("結局畫面有劇情"), HasText(EndingB, TEXT("眼前一片白光。")));

			UEscapeToastWidget* Toast = MakeWidget<UEscapeToastWidget>(Outer, Keep);
			Toast->SetToastText(TEXT("成就解鎖：時間管理大師"));
			TestTrue(TEXT("成就提示有文字"), HasText(Collect(Toast), TEXT("成就解鎖：時間管理大師")));
		});

		It(TEXT("不可以：任何文字超出文字框或被截斷（用 Slate 字型量測：最長的事件文字、結局劇情、成就提示在最大寬度自動換行後的行數不超過允許行數）"), [this]()
		{
			if (!FSlateApplication::IsInitialized())
			{
				AddError(TEXT("FSlateApplication 沒有初始化，無法量測（10a 實測 -nullrhi 下可用）"));
				return;
			}
			const FSlateFontInfo& Probe = FEscapeFonts::GetFont(UEscapePopupWidget::FontSize);
			if (!Probe.CompositeFont.IsValid())
			{
				AddError(TEXT("FEscapeFonts::GetFont 的 CompositeFont 無效，無法用我們的字型量測"));
				return;
			}

			// 量測本身可信：100 個全形字在 720 寬一定要換成多行
			const int32 Sanity = CountLines(FString::ChrN(100, TEXT('人')), UEscapePopupWidget::FontSize, UEscapePopupWidget::MaxTextWidth);
			TestTrue(*FString::Printf(TEXT("量測自我檢查：100 個全形字在 %.0f 寬換成 %d 行（至少 3 行）"), UEscapePopupWidget::MaxTextWidth, Sanity), Sanity >= 3);

			// 彈出框：DT_Interactions 的所有事件文字（整格，比單一則更長）與預設嘲諷（[對象] 換成最長的名稱）
			int32 PopupMax = 0;
			FString PopupLongest;
			int32 PopupCount = 0;
			const auto Interactions = LoadTable(TEXT("DT_Interactions.csv"));
			for (const auto& Pair : Interactions)
			{
				const FString Text = Pair.Value.FindRef(TEXT("EventText"));
				if (Text.IsEmpty()) { continue; }
				++PopupCount;
				const int32 Lines = CountLines(Text, UEscapePopupWidget::FontSize, UEscapePopupWidget::MaxTextWidth);
				if (Lines > PopupMax) { PopupMax = Lines; PopupLongest = Text; }
			}
			FString LongestName;
			for (const auto& Pair : LoadTable(TEXT("DT_Forms.csv"))) { const FString N = Pair.Value.FindRef(TEXT("FormName")); if (N.Len() > LongestName.Len()) { LongestName = N; } }
			for (const auto& Pair : LoadTable(TEXT("DT_SceneObjects.csv"))) { const FString N = Pair.Value.FindRef(TEXT("Name")); if (N.Len() > LongestName.Len()) { LongestName = N; } }
			const auto Responses = LoadTable(TEXT("DT_Responses.csv"));
			const TMap<FString, FString>* Default = Responses.Find(TEXT("Default"));
			TestNotNull(TEXT("DT_Responses 有 Default"), Default);
			if (Default)
			{
				const FString Taunt = Default->FindRef(TEXT("Text")).Replace(TEXT("[對象]"), *LongestName);
				++PopupCount;
				const int32 Lines = CountLines(Taunt, UEscapePopupWidget::FontSize, UEscapePopupWidget::MaxTextWidth);
				if (Lines > PopupMax) { PopupMax = Lines; PopupLongest = Taunt; }
			}
			TestTrue(*FString::Printf(TEXT("彈出框文字至少 30 筆（實際 %d）"), PopupCount), PopupCount >= 30);
			AddInfo(FString::Printf(TEXT("彈出框最長：%d 行（上限 %d）：%s"), PopupMax, UEscapePopupWidget::MaxLines, *PopupLongest));
			TestTrue(*FString::Printf(TEXT("彈出框最長的文字 %d 行 ≤ %d 行"), PopupMax, UEscapePopupWidget::MaxLines), PopupMax >= 1 && PopupMax <= UEscapePopupWidget::MaxLines);

			// 結局畫面：每個結局的標題與劇情
			int32 EndingMax = 0;
			int32 EndingCount = 0;
			for (const auto& Pair : LoadTable(TEXT("DT_Endings.csv")))
			{
				++EndingCount;
				for (const TCHAR* Column : { TEXT("Title"), TEXT("Story") })
				{
					const FString Text = Pair.Value.FindRef(Column);
					TestFalse(*FString::Printf(TEXT("%s 的 %s 不是空的"), *Pair.Key, Column), Text.IsEmpty());
					EndingMax = FMath::Max(EndingMax, CountLines(Text, UEscapeEndingWidget::FontSize, UEscapeEndingWidget::MaxTextWidth));
				}
			}
			TestEqual(TEXT("結局 5 筆"), EndingCount, 5);
			AddInfo(FString::Printf(TEXT("結局最長：%d 行（上限 %d）"), EndingMax, UEscapeEndingWidget::MaxLines));
			TestTrue(*FString::Printf(TEXT("結局最長的文字 %d 行 ≤ %d 行"), EndingMax, UEscapeEndingWidget::MaxLines), EndingMax >= 1 && EndingMax <= UEscapeEndingWidget::MaxLines);

			// 成就提示：前綴＋成就名稱
			int32 ToastMax = 0;
			int32 ToastCount = 0;
			const FString Prefix = UiText(TEXT("ToastPrefix"));
			TestFalse(TEXT("UiText 有 ToastPrefix"), Prefix.IsEmpty());
			for (const auto& Pair : LoadTable(TEXT("DT_Achievements.csv")))
			{
				++ToastCount;
				ToastMax = FMath::Max(ToastMax, CountLines(Prefix + Pair.Value.FindRef(TEXT("Title")), UEscapeToastWidget::FontSize, UEscapeToastWidget::MaxTextWidth));
			}
			TestEqual(TEXT("成就 10 筆"), ToastCount, 10);
			AddInfo(FString::Printf(TEXT("成就提示最長：%d 行（上限 %d）"), ToastMax, UEscapeToastWidget::MaxLines));
			TestTrue(*FString::Printf(TEXT("成就提示最長 %d 行 ≤ %d 行"), ToastMax, UEscapeToastWidget::MaxLines), ToastMax >= 1 && ToastMax <= UEscapeToastWidget::MaxLines);
		});

		// ================= 7. presenter 載入文字 =================

		It(TEXT("presenter 載入文字：真的 CSV 成功；缺少必要的 Name（HudHp）→ false 並在錯誤訊息指出 HudHp"), [this]()
		{
			FEscapeUiPresenter Good;
			FString Error;
			TestTrue(*FString::Printf(TEXT("真的 CSV 載入 %s"), *Error), MakeUi(Good, Error));

			FEscapeUiPresenter Bad;
			const FString Missing = TEXT("Name,Text\nHudForm,形態：{0}\nKeypadTitle,密碼\nKeypadDelete,刪除\nKeypadConfirm,確認\nEndingContinue,繼續\nToastPrefix,成就解鎖：\n");
			FString BadError;
			TestFalse(TEXT("缺 HudHp → false"), Bad.LoadTexts(Missing, LoadText(TEXT("DT_Endings.csv")), LoadText(TEXT("DT_Achievements.csv")), BadError));
			TestTrue(*FString::Printf(TEXT("錯誤訊息指出 HudHp（實際：%s）"), *BadError), BadError.Contains(TEXT("HudHp")));

			FEscapeUiPresenter Empty;
			FString EmptyError;
			TestFalse(TEXT("空字串 → false"), Empty.LoadTexts(FString(), FString(), FString(), EmptyError));
			TestFalse(TEXT("空字串 → 有錯誤訊息"), EmptyError.IsEmpty());
		});

		// ================= 8. 組合層 =================

		It(TEXT("組合層：AEscapeGameMode 的預設類別是 EscapePlayerController／GameState／PlayerState／HUD 與 BP_ThirdPersonCharacter，設定檔指到它"), [this]()
		{
			const AEscapeGameMode* Mode = GetDefault<AEscapeGameMode>();
			TestNotNull(TEXT("CDO"), Mode);
			if (!Mode) { return; }
			TestTrue(TEXT("PlayerControllerClass＝AEscapePlayerController"), Mode->PlayerControllerClass == AEscapePlayerController::StaticClass());
			TestTrue(TEXT("GameStateClass＝AEscapeGameState"), Mode->GameStateClass == AEscapeGameState::StaticClass());
			TestTrue(TEXT("PlayerStateClass＝AEscapePlayerState"), Mode->PlayerStateClass == AEscapePlayerState::StaticClass());
			TestTrue(TEXT("HUDClass＝AEscapeHUD"), Mode->HUDClass == AEscapeHUD::StaticClass());
			TestNotNull(TEXT("DefaultPawnClass 有設"), Mode->DefaultPawnClass.Get());
			TestTrue(TEXT("DefaultPawnClass 是 BP_ThirdPersonCharacter"),
				Mode->DefaultPawnClass != nullptr && Mode->DefaultPawnClass->GetPathName().Contains(TEXT("BP_ThirdPersonCharacter")));

			FString IniMode;
			GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultGameMode"), IniMode, GEngineIni);
			TestEqual(TEXT("DefaultEngine.ini 的 GlobalDefaultGameMode"), IniMode, FString(TEXT("/Script/EscapeGame.EscapeGameMode")));

			const UFunction* Interact = AEscapePlayerController::StaticClass()->FindFunctionByName(TEXT("ServerInteract"));
			const UFunction* Submit = AEscapePlayerController::StaticClass()->FindFunctionByName(TEXT("ServerSubmitPassword"));
			const UFunction* Return = AEscapePlayerController::StaticClass()->FindFunctionByName(TEXT("ServerReturnToHuman"));
			TestTrue(TEXT("ServerInteract 是 Server Reliable"), Interact && Interact->HasAnyFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
			TestTrue(TEXT("ServerSubmitPassword 是 Server Reliable"), Submit && Submit->HasAnyFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
			TestTrue(TEXT("ServerReturnToHuman 是 Server Reliable"), Return && Return->HasAnyFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
		});

		It(TEXT("組合層：AEscapeGameMode 能從 Content/Data 載入 logic（固定種子可重現），載入後可以互動"), [this]()
		{
			FTestWorldWrapper World;
			TestTrue(TEXT("建立測試世界"), World.CreateTestWorld(EWorldType::Game));
			if (!World.GetTestWorld()) { return; }
			AEscapeGameMode* Mode = World.GetTestWorld()->SpawnActor<AEscapeGameMode>();
			TestNotNull(TEXT("spawn AEscapeGameMode"), Mode);
			if (!Mode) { return; }
			TestFalse(TEXT("載入前 IsLogicLoaded 是 false"), Mode->IsLogicLoaded());

			Mode->SetSeedForTesting(5);
			FString Error;
			TestTrue(*FString::Printf(TEXT("LoadLogic %s"), *Error), Mode->LoadLogic(Error));
			TestTrue(TEXT("載入後 IsLogicLoaded"), Mode->IsLogicLoaded());
			TestEqual(TEXT("生命值 5"), Mode->GetLogic().GetTeam().GetHp(), FTeamState::StartingHp);
			TestEqual(TEXT("剩餘時間 30 分鐘"), Mode->GetLogic().GetTeam().GetRemainingSeconds(), FTeamState::RoundSeconds);

			FEscapeGameLogic Reference;
			TestTrue(*FString::Printf(TEXT("參考 logic 載入 %s"), *Error), MakeLogic(5, Reference, Error));
			TestEqual(TEXT("同種子 → 同密碼"), Mode->GetLogic().GetTeam().GetPasswordForDevOnly(), Reference.GetTeam().GetPasswordForDevOnly());

			Mode->GetLogic().AddPlayer(TEXT("P1"), FVector::ZeroVector);
			TestEqual(TEXT("人類碰黑板是 X018"), Mode->GetLogic().Interact(TEXT("P1"), TEXT("S14")).Result.InteractionId, FString(TEXT("X018")));
		});
	});
}

#endif  // !UE_BUILD_SHIPPING
