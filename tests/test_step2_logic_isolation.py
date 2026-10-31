# 步驟 2 互動判定 → AGENTS.md「Logic 只依賴 Core」
import re
import unittest
from pathlib import Path

LOGIC = Path(__file__).resolve().parent.parent / "Source" / "EscapeGame" / "Logic"
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)

# 白名單：Core 模組的標頭（路徑前綴或完整名稱）與 Logic 自己的標頭
ALLOWED_EXACT = {"CoreMinimal.h", "CoreTypes.h"}
ALLOWED_PREFIXES = ("Containers/", "Misc/", "Math/", "Templates/", "HAL/", "Algo/",
                    "Internationalization/", "Serialization/", "Logging/", "Async/",
                    "Delegates/", "Logic/", "String/")
# 黑名單：就算前綴剛好符合也不行
FORBIDDEN_PREFIXES = ("Engine/", "GameFramework/", "UObject/", "Components/", "Blueprint/", "UMG",
                      "Kismet/", "Slate", "Net/", "EnhancedInput", "Camera/", "Animation/")


def check_include(path, own_headers):
    """回傳違規原因，合法回傳 None。"""
    if path.endswith(".generated.h"):
        return "不可 include .generated.h"
    if path.startswith(FORBIDDEN_PREFIXES) or path.split("/")[0].startswith(("Engine", "UMG")):
        return "不可 include Engine／UObject／UMG 類標頭"
    if path in ALLOWED_EXACT or path.startswith(ALLOWED_PREFIXES) or path in own_headers:
        return None
    return "不在白名單"


def scan():
    own = {p.name for p in LOGIC.glob("*.h")}
    bad = []
    for f in sorted(list(LOGIC.glob("*.h")) + list(LOGIC.glob("*.cpp"))):
        for m in INCLUDE_RE.finditer(f.read_text(encoding="utf-8")):
            why = check_include(m.group(2), own)
            if why:
                bad.append(f"{f.name}: #include {m.group(2)}（{why}）")
    return bad


class LogicIsolationTest(unittest.TestCase):
    def test_logic_exists(self):
        """Logic 資料夾有 InteractionResolver.h 與 .cpp"""
        self.assertTrue((LOGIC / "InteractionResolver.h").is_file())
        self.assertTrue((LOGIC / "InteractionResolver.cpp").is_file())

    def test_logic_only_includes_core(self):
        """Logic 底下的 #include 只允許 Core 和 Logic 自己的標頭"""
        self.assertEqual(scan(), [])

    def test_checker_rejects_bad_includes(self):
        """檢查器本身會擋 Engine／GameFramework／UObject／Components／Blueprint／UMG／.generated.h"""
        for bad in ["Engine/World.h", "GameFramework/Actor.h", "UObject/Object.h", "Components/ActorComponent.h",
                    "Blueprint/UserWidget.h", "UMG.h", "Foo.generated.h", "Engine.h", "Random/Unknown.h"]:
            self.assertIsNotNone(check_include(bad, set()), bad)
        for ok in ["CoreMinimal.h", "Serialization/Csv/CsvParser.h", "Containers/Map.h", "InteractionResolver.h"]:
            self.assertIsNone(check_include(ok, {"InteractionResolver.h"}), ok)


if __name__ == "__main__":
    unittest.main()
