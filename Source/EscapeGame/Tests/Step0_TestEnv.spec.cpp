// 步驟 0c UE 專案與 UE 測試環境 → VERIFY STEP 0（第 3、4 條）
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"

BEGIN_DEFINE_SPEC(FEscapeGameStep0Spec, "EscapeGame.Step0",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FEscapeGameStep0Spec)

void FEscapeGameStep0Spec::Define()
{
	Describe(TEXT("headless 自動化測試環境"), [this]()
	{
		It(TEXT("裝好 UE 5.8 後，tools/run_tests.sh 也跑 UE 自動化測試，列出 EscapeGame. 開頭的測試"), [this]()
		{
			// 這條測試能被列出並執行，就代表 EscapeGame 專案的測試註冊成功。
			TestTrue(TEXT("測試路徑以 EscapeGame.Step0 開頭"), GetTestName().StartsWith(TEXT("EscapeGame.Step0")));
			TestEqual(TEXT("專案名稱是 EscapeGame"), FString(FApp::GetProjectName()), FString(TEXT("EscapeGame")));
		});

		It(TEXT("不可以 跑測試時需要螢幕或 GPU：-nullrhi 下引擎不會渲染"), [this]()
		{
			TestFalse(TEXT("FApp::CanEverRender() 在 -nullrhi 下應為 false"), FApp::CanEverRender());
			TestTrue(TEXT("命令列帶 -nullrhi"), FParse::Param(FCommandLine::Get(), TEXT("nullrhi")));
		});
	});
}
