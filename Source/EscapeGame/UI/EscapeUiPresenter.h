// 步驟 10 介面：把邏輯層的結果轉成「畫面該顯示什麼」。純 C++（只用 Core + Logic/ 標頭），不 include UMG／Slate。
// Widget 只照著它畫；彈出框、成就提示的 3 秒計時、鍵盤輸入狀態都在這裡，所以不用開畫面就能測。
// 10a：只有宣告與空殼（回傳預設值），10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Logic/EscapeGameLogic.h"
#include "Logic/TeamState.h"

/** 常駐 HUD 的三行字 */
struct FHudView
{
	FString HpText;    // UiText HudHp，{0} 換成生命值
	FString TimeText;  // FTeamState::FormatClock，例 "29:59"
	FString FormText;  // UiText HudForm，{0} 換成形態名稱
};

class FEscapeUiPresenter
{
public:
	static constexpr float PopupSeconds = 3.0f;  // 彈出框顯示滿幾秒自動關
	static constexpr float ToastSeconds = 3.0f;  // 成就提示每則顯示幾秒（VERIFY：2～4 秒）
	static constexpr int32 KeypadDigits = 4;

	/** 載入 DT_UiText、DT_Endings、DT_Achievements 三份 CSV 文字。缺必要的 Name 或格式錯誤 → false 並寫 OutError。 */
	bool LoadTexts(const FString& UiTextCsv, const FString& EndingsCsv, const FString& AchievementsCsv, FString& OutError);

	FHudView MakeHud(int32 Hp, int32 RemainingSeconds, const FString& FormDisplayName) const;

	/** 一次互動的結果：NeedsPassword → 開鍵盤；Resolved → 每則 Messages 依序排進彈出框佇列；Blocked／Rejected → 不動；
	 *  Events 裡受影響玩家含 LocalPlayerId 的結局 → 結局畫面；NewAchievements 裡 PlayerId＝LocalPlayerId 的 → 成就提示佇列。 */
	void OnOutcome(const FInteractOutcome& Outcome, const FString& LocalPlayerId);
	/** Tick 來的隊伍事件（E04 等）：受影響玩家含 LocalPlayerId → 結局畫面 */
	void OnTeamEvents(const TArray<FTeamEvent>& Events, const FString& LocalPlayerId);
	/** Tick 來的新成就（A03 等）：PlayerId＝LocalPlayerId 的 → 成就提示佇列 */
	void OnAchievements(const TArray<FAchievementUnlock>& Unlocks, const FString& LocalPlayerId);

	void Tick(float Dt);
	/** 關掉目前的彈出框（接著顯示下一則）。鍵盤開著時不作用。 */
	void OnAnyKey();

	// 彈出框
	bool IsPopupVisible() const;
	FString GetPopupText() const;

	// 四位數鍵盤
	bool IsKeypadVisible() const;
	FString GetKeypadEntry() const;
	/** 0～9，最多 4 位，第 5 位忽略；鍵盤沒開時不作用 */
	void KeypadDigit(int32 Digit);
	void KeypadDelete();
	/** 剛好 4 位 → true、OutCode 是那 4 位、鍵盤關閉並清空；不足 4 位 → false，鍵盤不關 */
	bool KeypadConfirm(FString& OutCode);
	void KeypadCancel();

	// 結局畫面
	bool IsEndingVisible() const;
	FString GetEndingId() const;
	FString GetEndingTitle() const;  // DT_Endings 的 Title
	FString GetEndingStory() const;  // DT_Endings 的 Story

	// 成就提示（右上角）
	bool IsToastVisible() const;
	FString GetToastText() const;  // 成就名稱（DT_Achievements 的 Title）
};
