// 步驟 10 介面：字型空殼（10a），10b 實作。
#include "UI/EscapeFonts.h"

const FSlateFontInfo& FEscapeFonts::GetFont(int32 Size)
{
	static FSlateFontInfo Placeholder;
	Placeholder.Size = Size;
	return Placeholder;
}
