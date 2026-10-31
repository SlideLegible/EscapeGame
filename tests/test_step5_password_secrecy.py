# 步驟 5 大門密碼 → VERIFY STEP 5（不可以 遊戲內任何文字、畫面、log（Shipping 版）出現當局密碼）
#
# 靜態檢查 Source/EscapeGame/ 底下所有 .h／.cpp：
#   1. UE_LOG／AddOnScreenDebugMessage／printf 這類輸出的整個陳述句，不可以出現密碼相關的名稱
#      （Password、GetPasswordForDevOnly…；EPasswordInput、PasswordRequirement 這種列舉型別名稱除外）。
#   2. 名稱含 DevOnly 的函式（GetPasswordForDevOnly），每一處出現（宣告、定義、呼叫）都在
#      `#if !UE_BUILD_SHIPPING` 區塊裡，Shipping 版沒有任何途徑拿到密碼。
# 檢查器本身也有測試，會抓到故意寫壞的範例（像 test_step2_logic_isolation.py 那樣）。
import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "Source" / "EscapeGame"

# 輸出類呼叫（大小寫要對：FString::Printf 不是輸出）
OUTPUT_RE = re.compile(
    r"\b(UE_LOG\w*|UE_CLOG\w*|UE_VLOG\w*|UE_LOGFMT\w*|AddOnScreenDebugMessage|GLog|"
    r"printf|fprintf|puts|OutputDebugString\w*|LowLevelOutputDebugString\w*|Debugf|ClientMessage)\b")
PASSWORD_RE = re.compile(r"password", re.I)
# 這些是列舉型別或對象 ID，只提到「有密碼這件事」，不會帶出密碼本身
ALLOWED_NAMES = ("EPasswordInput", "EPasswordRequirement", "PasswordInput", "PasswordRequirement", "PasswordTargetId")
DEVONLY_RE = re.compile(r"\b\w*DevOnly\w*\b")
SAFE_CONDITIONS = {"!UE_BUILD_SHIPPING", "!(UE_BUILD_SHIPPING)", "UE_BUILD_SHIPPING==0", "UE_BUILD_SHIPPING == 0"}
SHIPPING_CONDITIONS = {"UE_BUILD_SHIPPING", "(UE_BUILD_SHIPPING)", "UE_BUILD_SHIPPING==1", "UE_BUILD_SHIPPING == 1"}


def strip_comments(text, blank_strings=False):
    """拿掉 // 與 /* */ 註解（保留換行，行號不變）。字串字面值裡的 // 不算註解。
    blank_strings=True 時，字串／字元字面值的內容換成空白（引號保留）。"""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:end]))
            i = end
        elif c == '"' or (c == "'" and not (out and (out[-1][-1:].isalnum()))):
            quote = c
            out.append(quote)
            i += 1
            while i < n and text[i] != quote and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n:
                    out.append("  " if blank_strings else text[i:i + 2])
                    i += 2
                    continue
                out.append(" " if blank_strings else text[i])
                i += 1
            if i < n and text[i] == quote:
                out.append(quote)
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def output_statements(text):
    """回傳 [(行號, 陳述句文字)]：每個輸出類呼叫從名稱到結尾的 ';'（字串內容已清掉，所以字串裡的 ';' 不會截斷）。"""
    code = strip_comments(text, blank_strings=True)
    found = []
    for m in OUTPUT_RE.finditer(code):
        end = code.find(";", m.start())
        end = len(code) if end < 0 else end + 1
        found.append((code.count("\n", 0, m.start()) + 1, code[m.start():end]))
    return found


def find_password_in_logs(text):
    """回傳違規的 [(行號, 陳述句)]。"""
    bad = []
    for line, stmt in output_statements(text):
        cleaned = stmt
        for name in ALLOWED_NAMES:
            cleaned = cleaned.replace(name, "")
        if PASSWORD_RE.search(cleaned):
            bad.append((line, " ".join(stmt.split())))
    return bad


def find_unguarded_devonly(text):
    """回傳沒有被 `#if !UE_BUILD_SHIPPING` 包住的 DevOnly 名稱 [(行號, 名稱)]。"""
    code = strip_comments(text)
    stack = []  # 每層 [種類, 目前這個分支是否只在非 Shipping 編譯]；種類：nonship／ship／other
    bad = []
    for no, line in enumerate(code.split("\n"), 1):
        d = re.match(r"\s*#\s*(\w+)\s*(.*)", line)
        if d:
            word, rest = d.group(1), d.group(2).strip()
            if word == "if":
                if rest in SAFE_CONDITIONS:
                    stack.append(["nonship", True])
                elif rest in SHIPPING_CONDITIONS:
                    stack.append(["ship", False])
                else:
                    stack.append(["other", False])
            elif word in ("ifdef", "ifndef"):
                stack.append(["other", False])
            elif word == "elif":
                if stack:
                    stack[-1][1] = False
            elif word == "else":
                if stack:
                    stack[-1][1] = stack[-1][0] == "ship"
            elif word == "endif":
                if stack:
                    stack.pop()
            continue
        for m in DEVONLY_RE.finditer(line):
            if not any(safe for _, safe in stack):
                bad.append((no, m.group(0)))
    return bad


def source_files():
    return sorted(list(SRC.rglob("*.h")) + list(SRC.rglob("*.cpp")))


class PasswordSecrecyTest(unittest.TestCase):
    def test_no_password_in_log_or_screen_output(self):
        """不可以 遊戲內任何文字、畫面、log（Shipping 版）出現當局密碼：輸出類呼叫不碰密碼名稱"""
        files = source_files()
        self.assertGreater(len(files), 10)
        bad = []
        for f in files:
            for line, stmt in find_password_in_logs(f.read_text(encoding="utf-8")):
                bad.append(f"{f.relative_to(SRC)}:{line}: {stmt}")
        self.assertEqual(bad, [])

    def test_dev_only_functions_are_not_in_shipping(self):
        """Shipping 沒有任何公開途徑拿到密碼：名稱含 DevOnly 的函式每一處都在 #if !UE_BUILD_SHIPPING 裡"""
        bad = []
        for f in source_files():
            for line, name in find_unguarded_devonly(f.read_text(encoding="utf-8")):
                bad.append(f"{f.relative_to(SRC)}:{line}: {name}")
        self.assertEqual(bad, [])

    def test_password_getter_declared_and_defined_in_team_state(self):
        """GetPasswordForDevOnly 在 TeamState.h 宣告、TeamState.cpp 定義（改名或搬走這個檢查會失敗，不會默默放行）"""
        for name in ("TeamState.h", "TeamState.cpp"):
            text = strip_comments((SRC / "Logic" / name).read_text(encoding="utf-8"))
            self.assertIn("GetPasswordForDevOnly", text, name)
            self.assertEqual(find_unguarded_devonly(text), [], name)

    def test_checker_catches_password_in_logs(self):
        """檢查器本身會抓到：UE_LOG、跨行 UE_LOG、螢幕訊息、printf 帶出密碼；不誤抓正常的程式"""
        bad_samples = [
            'UE_LOG(LogTemp, Log, TEXT("pw %s"), *Password);',
            'UE_LOG(LogTemp, Warning, TEXT("pw %s"),\n    *Team.GetPasswordForDevOnly());',
            'GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Password);',
            'printf("%s", TCHAR_TO_UTF8(*Password));',
            'UE_LOG(LogTemp, Log, TEXT("ws://127.0.0.1 %s"), *CurrentPassword);',  # 字串裡的 // 不是註解
            'UE_LOG(LogTemp, Log, TEXT("a;b %s"), *Password);',  # 字串裡的 ; 不截斷
            'UE_LOG(LogTemp, Log, TEXT("x"), *Team.CheckPassword(Code)); /* ok */',
        ]
        for sample in bad_samples:
            self.assertNotEqual(find_password_in_logs(sample), [], sample)

        good_samples = [
            'UE_LOG(LogTemp, Log, TEXT("password check failed"));',  # 只在字串裡提到
            '// UE_LOG(LogTemp, Log, TEXT("%s"), *Password);',  # 註解
            '/* UE_LOG(LogTemp, Log, TEXT("%s"), *Password); */',
            'UE_LOG(LogTemp, Log, TEXT("%d"), static_cast<int32>(EPasswordInput::Correct));',  # 列舉型別名稱
            'Password = FString::Printf(TEXT("%04d"), N);',  # FString::Printf 不是輸出
            'UE_LOG(LogTemp, Log, TEXT("hp %d"), Hp);\nFString Password = Make();',  # 下一行才有密碼
        ]
        for sample in good_samples:
            self.assertEqual(find_password_in_logs(sample), [], sample)

    def test_checker_catches_unguarded_dev_only(self):
        """檢查器本身會抓到：沒有保護、#else 分支、#ifdef、#if UE_BUILD_SHIPPING 的主分支、#elif；放行正確的保護"""
        bad_samples = [
            "FString GetPasswordForDevOnly() const;",
            "#if !UE_BUILD_SHIPPING\nint X;\n#endif\nFString GetPasswordForDevOnly() const;",  # 已經離開保護
            "#if !UE_BUILD_SHIPPING\nint X;\n#else\nFString GetPasswordForDevOnly() const;\n#endif",
            "#ifdef WITH_EDITOR\nFString GetPasswordForDevOnly() const;\n#endif",
            "#if UE_BUILD_SHIPPING\nFString GetPasswordForDevOnly() const;\n#endif",
            "#if !UE_BUILD_SHIPPING\n#elif 1\nFString GetPasswordForDevOnly() const;\n#endif",
            "#if !UE_BUILD_SHIPPING\n#endif\nTeam.GetPasswordForDevOnly();",
        ]
        for sample in bad_samples:
            self.assertNotEqual(find_unguarded_devonly(sample), [], sample)

        good_samples = [
            "#if !UE_BUILD_SHIPPING\nFString GetPasswordForDevOnly() const;\n#endif",
            "#if !UE_BUILD_SHIPPING\n#if WITH_X\nFString GetPasswordForDevOnly() const;\n#endif\n#endif",
            "#if UE_BUILD_SHIPPING\nint X;\n#else\nFString GetPasswordForDevOnly() const;\n#endif",
            "// GetPasswordForDevOnly 只在註解裡\nint X;",
        ]
        for sample in good_samples:
            self.assertEqual(find_unguarded_devonly(sample), [], sample)

    def test_checker_strip_comments(self):
        """註解拿掉後行號不變；字串裡的 // 與 /* 不算註解"""
        text = 'int A; // c\n/* x\ny */ int B;\nconst char* S = "a//b /* c */";\n'
        stripped = strip_comments(text)
        self.assertEqual(stripped.count("\n"), text.count("\n"))
        self.assertNotIn("c\n", stripped.split("\n")[0].replace("int A;", ""))
        self.assertIn('"a//b /* c */"', stripped)
        self.assertIn("int B;", stripped)


if __name__ == "__main__":
    unittest.main()
