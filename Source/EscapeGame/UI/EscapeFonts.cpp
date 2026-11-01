// 步驟 10 介面：繁體中文字型。不做 UFont 資產：執行時用 FCompositeFont 從 Content/Fonts 的檔案載入。
#include "UI/EscapeFonts.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"

namespace
{
	FString FontPath(const TCHAR* FileName)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts") / FileName);
	}

	TSharedRef<FStandaloneCompositeFont> MakeComposite()
	{
		// 預設字型：NotoSansCJK-Regular.ttc 的 SubFaceIndex 3（Noto Sans CJK TC）
		TSharedRef<FStandaloneCompositeFont> Composite = MakeShared<FStandaloneCompositeFont>();
		FTypefaceEntry& Cjk = Composite->DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Cjk.Name = TEXT("NotoSansCJKtc");
		Cjk.Font = FFontData(FontPath(FEscapeFonts::CjkFontFile()), EFontHinting::Default, EFontLoadingPolicy::LazyLoad, FEscapeFonts::TcSubFaceIndex);

		// 表情符號（🎂 等）：CJK 字型沒有這些字，掉到 fallback 的 NotoColorEmoji.ttf（彩色點陣，Slate 支援 BGRA）
		FTypefaceEntry& Emoji = Composite->FallbackTypeface.Typeface.Fonts.AddDefaulted_GetRef();
		Emoji.Name = TEXT("NotoColorEmoji");
		Emoji.Font = FFontData(FontPath(FEscapeFonts::EmojiFontFile()), EFontHinting::Default, EFontLoadingPolicy::LazyLoad, 0);
		return Composite;
	}
}

const FSlateFontInfo& FEscapeFonts::GetFont(int32 Size)
{
	static FCriticalSection Lock;
	// 刻意不釋放：FStandaloneCompositeFont 是 FGCObject，程式結束時靜態解構順序不保證 GC 還在
	static TSharedPtr<FStandaloneCompositeFont>* Composite = new TSharedPtr<FStandaloneCompositeFont>(MakeComposite());
	static TMap<int32, TSharedRef<FSlateFontInfo>>* Fonts = new TMap<int32, TSharedRef<FSlateFontInfo>>();

	FScopeLock ScopeLock(&Lock);
	// 值放 TSharedRef：TMap 長大搬動時，之前回傳出去的參考仍然有效
	if (const TSharedRef<FSlateFontInfo>* Found = Fonts->Find(Size)) { return **Found; }
	TSharedRef<FSlateFontInfo> Info = MakeShared<FSlateFontInfo>(Composite->ToSharedRef(), static_cast<float>(Size));
	Fonts->Add(Size, Info);
	return *Info;
}
