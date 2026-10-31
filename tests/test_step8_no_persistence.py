# 步驟 8 成就 → VERIFY STEP 8（不可以 成就寫進磁碟或任何永久儲存）
#
# 靜態檢查 Source/EscapeGame/ 底下所有 .h／.cpp（含測試）：不可以出現永久儲存相關的 API
#   SaveGame／USaveGame／SaveGameToSlot、FFileHelper::Save*、GConfig->Set／Flush、SaveConfig、
#   IFileManager／IPlatformFile／FPlatformFileManager（寫檔）、FArchive、CreateFileWriter、
#   標準函式庫的檔案寫入（ofstream、fopen、fwrite…）。
# 讀檔（例：測試用 FFileHelper::LoadFileToString 讀 CSV）不算。
# 檢查器本身也有測試，會抓到故意寫壞的範例（像 test_step5_password_secrecy.py 那樣）。
import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "Source" / "EscapeGame"

# (名稱, 正規表示式)：在「拿掉註解與字串內容」之後的程式碼上比對
FORBIDDEN = [
    ("SaveGame", re.compile(r"\b(USaveGame|SaveGameToSlot|SaveGameToMemory|AsyncSaveGameToSlot|CreateSaveGameObject|LoadGameFromSlot|DoesSaveGameExist)\b|\bSaveGame\b")),
    ("FFileHelper::Save", re.compile(r"\bFFileHelper\s*::\s*(Save\w*|Create\w*Writer\w*|Append\w*)")),
    ("GConfig 寫入", re.compile(r"\bGConfig\s*->\s*(Set\w*|Flush|EmptySection\w*|RemoveKey|Remove\w*)\b")),
    ("SaveConfig", re.compile(r"\b(SaveConfig|TryUpdateDefaultConfigFile|UpdateDefaultConfigFile|UpdateGlobalUserConfigFile|UpdateSinglePropertyInConfigFile)\b")),
    ("FConfigCacheIni", re.compile(r"\bFConfigCacheIni\b|\bFConfigFile\b")),
    ("IFileManager／IPlatformFile", re.compile(r"\b(IFileManager|IPlatformFile|FPlatformFileManager|FFileManagerGeneric)\b")),
    ("FArchive 檔案寫入", re.compile(r"\b(FArchive|FArchiveFileWriterGeneric|FStructuredArchive\w*|FBufferArchive\w*|FMemoryWriter\w*)\b")),
    ("CreateFileWriter", re.compile(r"\bCreateFileWriter\b|\bOpenWrite\b|\bOpenAppend\b")),
    ("標準函式庫寫檔", re.compile(r"\b(std\s*::\s*(ofstream|fstream|wofstream)|fopen|fwrite|fputs|fprintf|freopen|_wfopen)\b")),
    ("瀏覽器儲存", re.compile(r"\b(localStorage|sessionStorage|IndexedDB|document\.cookie)\b")),
]


def strip_comments_and_strings(text):
    """拿掉 // 與 /* */ 註解、把字串與字元字面值的內容換成空白（保留換行，行號不變）。"""
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
        elif c == '"' or (c == "'" and not (out and out[-1][-1:].isalnum())):
            quote = c
            out.append(quote)
            i += 1
            while i < n and text[i] != quote and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n:
                    out.append("  ")
                    i += 2
                    continue
                out.append(" ")
                i += 1
            if i < n and text[i] == quote:
                out.append(quote)
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def find_persistence(text):
    """回傳違規的 [(行號, 名稱, 該行程式碼)]。"""
    code = strip_comments_and_strings(text)
    bad = []
    for no, line in enumerate(code.split("\n"), 1):
        for name, rx in FORBIDDEN:
            if rx.search(line):
                bad.append((no, name, line.strip()))
    return bad


def source_files():
    return sorted(list(SRC.rglob("*.h")) + list(SRC.rglob("*.cpp")))


class NoPersistenceTest(unittest.TestCase):
    def test_sources_do_not_use_persistent_storage(self):
        """不可以 成就寫進磁碟或任何永久儲存：Source/EscapeGame/ 全部 .h／.cpp 沒有存檔、寫檔、寫設定的 API"""
        files = source_files()
        self.assertGreater(len(files), 10)
        bad = []
        for f in files:
            for line, name, code in find_persistence(f.read_text(encoding="utf-8")):
                bad.append(f"{f.relative_to(SRC)}:{line}: {name}: {code}")
        self.assertEqual(bad, [])

    def test_scan_covers_achievement_tracker(self):
        """掃描範圍有包含成就追蹤器（改名或搬走這個檢查會失敗，不會默默放行）"""
        scanned = {f.relative_to(SRC).as_posix() for f in source_files()}
        self.assertIn("Logic/AchievementTracker.h", scanned)
        self.assertIn("Logic/AchievementTracker.cpp", scanned)
        self.assertIn("Logic/EscapeGameLogic.cpp", scanned)

    def test_checker_catches_persistence_apis(self):
        """檢查器本身會抓到：SaveGame、SaveGameToSlot、FFileHelper::Save、GConfig->Set、SaveConfig、IFileManager、IPlatformFile、FArchive、CreateFileWriter、ofstream、fopen"""
        bad_samples = [
            "UGameplayStatics::SaveGameToSlot(Save, TEXT(\"A\"), 0);",
            "class UAchievementSave : public USaveGame {};",
            "USaveGame* S = UGameplayStatics::CreateSaveGameObject(UAchievementSave::StaticClass());",
            "FFileHelper::SaveStringToFile(Text, *Path);",
            "FFileHelper::SaveArrayToFile(Bytes, *Path);",
            "GConfig->SetString(TEXT(\"A\"), TEXT(\"B\"), TEXT(\"C\"), GGameIni);",
            "GConfig->Flush(false, GGameIni);",
            "UAchievementSettings::StaticClass()->GetDefaultObject()->SaveConfig();",
            "IFileManager::Get().Delete(*Path);",
            "IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();",
            "FArchive* Ar = IFileManager::Get().CreateFileWriter(*Path);",
            "TUniquePtr<FArchive> Ar(Manager.CreateFileWriter(*Path));",
            "FArchive& Ar,",
            "std::ofstream Out(\"achievements.txt\");",
            "FILE* F = fopen(\"a.txt\", \"w\");",
            "int X = 1; GConfig  ->  SetInt(A, B, C, D);",
        ]
        for sample in bad_samples:
            self.assertNotEqual(find_persistence(sample), [], sample)

    def test_checker_allows_reading_and_mentions(self):
        """檢查器不誤抓：讀檔（LoadFileToString）、註解、字串裡提到的名字、名字相近的變數"""
        good_samples = [
            "FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT(\"Data\") / Name));",
            "// 成就不用 SaveGame、不寫 GConfig->SetString",
            "/* USaveGame\n   FArchive */ int X;",
            "const TCHAR* Msg = TEXT(\"SaveGame FFileHelper::SaveStringToFile\");",
            "bool bSavedGameOk = true; int32 SaveCount = 0;",
            "TSet<FString> UnlockedAchievements; Team.AddUnlockedAchievement(Id);",
            "FString Path = FPaths::ProjectContentDir() / TEXT(\"Data\");",
        ]
        for sample in good_samples:
            self.assertEqual(find_persistence(sample), [], sample)

    def test_checker_strip_keeps_line_numbers(self):
        """拿掉註解與字串後行號不變，違規回報的行號是對的"""
        text = "int A; // c\n/* x\ny */ int B;\nconst TCHAR* S = TEXT(\"//SaveGame\");\nUGameplayStatics::SaveGameToSlot(S, N, 0);\n"
        stripped = strip_comments_and_strings(text)
        self.assertEqual(stripped.count("\n"), text.count("\n"))
        found = find_persistence(text)
        self.assertEqual([(no, name) for no, name, _ in found], [(5, "SaveGame")])


if __name__ == "__main__":
    unittest.main()
