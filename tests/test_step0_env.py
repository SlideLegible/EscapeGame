# 步驟 0a Python 測試環境 → VERIFY STEP 0（第 1、2、5 條）
import ast
import os
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NESTED = "ESCAPEGAME_NESTED_TEST_RUN"


class TestStep0Env(unittest.TestCase):
    def test_run_tests_script_runs_all_python_tests_and_ends_with_ok(self):
        """在 repo 根目錄執行 tools/run_tests.sh → 跑完全部 Python 測試，最後一行顯示 OK，結束碼 0"""
        if os.environ.get(NESTED):
            self.skipTest("在 run_tests.sh 的巢狀執行裡，避免無限遞迴")
        script = ROOT / "tools" / "run_tests.sh"
        self.assertTrue(script.is_file(), "tools/run_tests.sh 不存在")
        self.assertTrue(os.access(script, os.X_OK), "tools/run_tests.sh 不能執行")
        env = dict(os.environ, **{NESTED: "1"})
        result = subprocess.run(
            [str(script)], cwd=ROOT, env=env, capture_output=True, text=True, timeout=120
        )
        output = (result.stdout + result.stderr).strip()
        self.assertEqual(result.returncode, 0, f"結束碼不是 0：\n{output}")
        last_line = output.splitlines()[-1] if output else ""
        self.assertTrue(last_line.startswith("OK"), f"最後一行不是 OK：{last_line!r}")

    def test_run_tests_script_works_from_another_directory(self):
        """從別的資料夾執行 tools/run_tests.sh → 一樣跑 repo 的測試，結束碼 0"""
        if os.environ.get(NESTED):
            self.skipTest("在 run_tests.sh 的巢狀執行裡，避免無限遞迴")
        script = ROOT / "tools" / "run_tests.sh"
        self.assertTrue(script.is_file(), "tools/run_tests.sh 不存在")
        env = dict(os.environ, **{NESTED: "1"})
        result = subprocess.run(
            [str(script)], cwd="/", env=env, capture_output=True, text=True, timeout=120
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_both_design_documents_exist_and_are_not_empty(self):
        """兩份設計文件都在 → 檔案存在且不是空的"""
        for name in ("escape_room_script.md", "item_interactions.md"):
            path = ROOT / "docs" / "design" / name
            self.assertTrue(path.is_file(), f"{path} 不存在")
            self.assertGreater(path.stat().st_size, 0, f"{path} 是空的")

    def test_tools_is_an_importable_package(self):
        """tools/ 是可以 import 的套件（之後的 design_to_csv.py 放在這裡）"""
        self.assertTrue((ROOT / "tools" / "__init__.py").is_file(), "tools/__init__.py 不存在")

    def test_must_not_need_extra_python_packages(self):
        """不可以 需要另外安裝 Python 套件才能跑測試（只用內建 unittest）"""
        local = {"tools", "tests"}
        files = list((ROOT / "tests").rglob("*.py")) + list((ROOT / "tools").rglob("*.py"))
        for path in files:
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module:
                    names = [node.module]
                else:
                    continue
                for name in names:
                    top = name.split(".")[0]
                    self.assertTrue(
                        top in sys.stdlib_module_names or top in local,
                        f"{path.relative_to(ROOT)} import 了非標準函式庫的 {name}",
                    )


if __name__ == "__main__":
    unittest.main()
