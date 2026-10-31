# 步驟 0c UE 專案與 UE 測試環境 → VERIFY STEP 0（第 3、4 條）、STEP 0b（第 1 條）
import json
import os
import re
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NESTED = "ESCAPEGAME_NESTED_TEST_RUN"
UE_ROOT = Path(os.environ.get("UE_ROOT", str(Path.home() / "UnrealEngine")))
REDIRECT_MARK = "TP_ThirdPerson"


def read_text(path):
    return Path(path).read_text(encoding="utf-8", errors="replace")


class TestUProject(unittest.TestCase):
    def test_uproject_is_valid_json_with_runtime_module_named_escapegame(self):
        """EscapeGame.uproject 是合法 JSON，有一個 Runtime 類型、名稱為 EscapeGame 的 module"""
        path = ROOT / "EscapeGame.uproject"
        self.assertTrue(path.is_file(), "EscapeGame.uproject 不存在")
        data = json.loads(read_text(path))
        mods = [m for m in data.get("Modules", []) if m.get("Name") == "EscapeGame"]
        self.assertEqual(len(mods), 1, "找不到名稱為 EscapeGame 的 module")
        self.assertEqual(mods[0].get("Type"), "Runtime")

    def test_mcp_plugins_enabled_for_editor_only(self):
        """ModelContextProtocol、AllToolsets 兩個 plugin 啟用，而且只限 Editor target"""
        path = ROOT / "EscapeGame.uproject"
        self.assertTrue(path.is_file(), "EscapeGame.uproject 不存在")
        plugins = {p["Name"]: p for p in json.loads(read_text(path)).get("Plugins", [])}
        for name in ("ModelContextProtocol", "AllToolsets"):
            self.assertIn(name, plugins, f"{name} 沒有列在 plugins")
            self.assertTrue(plugins[name].get("Enabled"), f"{name} 沒有啟用")
            self.assertEqual(plugins[name].get("TargetAllowList"), ["Editor"], f"{name} 不是只限 Editor")

    def test_target_and_build_files_exist(self):
        """Source 底下有 EscapeGame.Target.cs、EscapeGameEditor.Target.cs、EscapeGame/EscapeGame.Build.cs"""
        for rel in (
            "Source/EscapeGame.Target.cs",
            "Source/EscapeGameEditor.Target.cs",
            "Source/EscapeGame/EscapeGame.Build.cs",
        ):
            self.assertTrue((ROOT / rel).is_file(), f"{rel} 不存在")

    def test_no_leftover_template_name_in_source_or_config(self):
        """Source、Config 裡不可以 殘留 TP_ThirdPerson 字樣（redirect 那幾行除外）"""
        bad = []
        for folder in ("Source", "Config"):
            base = ROOT / folder
            self.assertTrue(base.is_dir(), f"{folder}/ 不存在")
            for path in base.rglob("*"):
                if not path.is_file():
                    continue
                if REDIRECT_MARK.lower() in path.name.lower():
                    bad.append(f"檔名 {path.relative_to(ROOT)}")
                    continue
                for n, line in enumerate(read_text(path).splitlines(), 1):
                    if REDIRECT_MARK.lower() in line.lower() and "redirect" not in line.lower():
                        bad.append(f"{path.relative_to(ROOT)}:{n}: {line.strip()}")
        self.assertEqual(bad, [], "殘留範本名稱：\n" + "\n".join(bad))

    def test_default_engine_ini_redirects_old_module_to_escapegame(self):
        """Config/DefaultEngine.ini 有把 /Script/TP_ThirdPerson 導向 /Script/EscapeGame 的 redirect"""
        path = ROOT / "Config" / "DefaultEngine.ini"
        self.assertTrue(path.is_file(), "Config/DefaultEngine.ini 不存在")
        text = read_text(path)
        self.assertRegex(
            text,
            r'(?i)PackageRedirects=\(.*OldName="/Script/TP_ThirdPerson".*NewName="/Script/EscapeGame".*\)',
        )

    def test_variants_were_removed(self):
        """只留基本第三人稱骨架：Source、Content 底下沒有 Variant_* 資料夾"""
        found = [str(p.relative_to(ROOT)) for p in ROOT.glob("**/Variant_*")
                 if ".git" not in p.parts and "Intermediate" not in p.parts and "Saved" not in p.parts]
        self.assertEqual(found, [])


class TestMcpConfig(unittest.TestCase):
    def test_mcp_json_points_only_at_127_0_0_1(self):
        """.mcp.json 只指向 127.0.0.1:8000，不可以 出現 0.0.0.0 或其他主機"""
        path = ROOT / ".mcp.json"
        self.assertTrue(path.is_file(), ".mcp.json 不存在")
        text = read_text(path)
        data = json.loads(text)
        self.assertNotIn("0.0.0.0", text)
        urls = re.findall(r'https?://[^"\s]+', text)
        self.assertTrue(urls, ".mcp.json 裡沒有網址")
        for url in urls:
            self.assertTrue(url.startswith("http://127.0.0.1:8000/mcp"), f"網址不是本機：{url}")
        self.assertTrue(data.get("mcpServers"), "沒有 mcpServers")


class TestUeAutomation(unittest.TestCase):
    def test_ue_installed(self):
        """裝好 UE 5.8 → 找得到 UnrealEditor-Cmd（沒裝就是失敗，不是跳過）"""
        cmd = UE_ROOT / "Engine" / "Binaries" / "Linux" / "UnrealEditor-Cmd"
        self.assertTrue(
            cmd.is_file(),
            f"找不到 {cmd}：UE 5.8 沒有安裝在 {UE_ROOT}（可用環境變數 UE_ROOT 指定）",
        )

    def test_run_ue_tests_script_forces_nullrhi(self):
        """不可以 跑測試時需要螢幕或 GPU → tools/run_ue_tests.py 一定帶 -nullrhi"""
        script = ROOT / "tools" / "run_ue_tests.py"
        self.assertTrue(script.is_file(), "tools/run_ue_tests.py 不存在")
        self.assertIn("-nullrhi", read_text(script))

    def test_run_ue_tests_runs_headless_and_all_escapegame_tests_pass(self):
        """tools/run_tests.sh 也跑 UE 自動化測試（headless）→ 列出 EscapeGame. 開頭的測試，全部通過"""
        if os.environ.get(NESTED):
            self.skipTest("在 run_tests.sh 的巢狀執行裡，UE 測試由外層那次跑")
        cmd = UE_ROOT / "Engine" / "Binaries" / "Linux" / "UnrealEditor-Cmd"
        self.assertTrue(cmd.is_file(), f"UE 5.8 沒有安裝在 {UE_ROOT}，無法跑 UE 自動化測試")
        script = ROOT / "tools" / "run_ue_tests.py"
        self.assertTrue(script.is_file(), "tools/run_ue_tests.py 不存在")
        result = subprocess.run(
            ["python3", "-I", str(script)], cwd="/", capture_output=True, text=True, timeout=900
        )
        output = result.stdout + result.stderr
        print("\n" + result.stdout)
        self.assertEqual(result.returncode, 0, f"run_ue_tests.py 結束碼不是 0：\n{output[-3000:]}")
        listed = [l for l in result.stdout.splitlines() if "EscapeGame." in l]
        self.assertGreaterEqual(len(listed), 1, "輸出沒有列出任何 EscapeGame. 開頭的測試")
        self.assertIn("全部通過", result.stdout)


if __name__ == "__main__":
    unittest.main()
