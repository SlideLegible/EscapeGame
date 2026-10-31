// 步驟 10 介面：繁體中文字型。執行時用 FCompositeFont 從 Content/Fonts 的檔案載入（不做 UFont 資產）：
// 預設字型 NotoSansCJK-Regular.ttc 的 SubFaceIndex 3（Noto Sans CJK TC），表情符號 NotoColorEmoji.ttf 當 fallback。
// 10a：空殼，GetFont 回傳沒有 CompositeFont 的預設 FSlateFontInfo；10b 實作。
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

struct FEscapeFonts
{
	static constexpr int32 TcSubFaceIndex = 3;
	static const TCHAR* CjkFontFile() { return TEXT("NotoSansCJK-Regular.ttc"); }
	static const TCHAR* EmojiFontFile() { return TEXT("NotoColorEmoji.ttf"); }

	/** 指定大小的字型；同一個大小回傳同一份。CompositeFont 一定有效（檔案缺少時也不可以崩潰）。 */
	static const FSlateFontInfo& GetFont(int32 Size);
};
