# 步驟 2 互動判定 → VERIFY STEP 2（第 6 條：預設回應的資料來源 DT_Responses.csv）
import csv
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "tools" / "design_to_responses.py"
MD = ROOT / "docs" / "design" / "item_interactions.md"
CSV_PATH = ROOT / "Content" / "Data" / "DT_Responses.csv"


def run_script(*args):
    return subprocess.run([sys.executable, "-I", str(SCRIPT), *map(str, args)],
                          capture_output=True, text=True, timeout=60, cwd="/")


def md_default_text():
    """獨立從設計文件 §5 抓出引用區塊那一行。"""
    text = MD.read_text(encoding="utf-8")
    sec = text.split("## 5. 預設回應", 1)[1]
    m = re.search(r"^> (.+)$", sec, re.M)
    return m.group(1).strip()


def read_csv(p):
    with open(p, encoding="utf-8", newline="") as f:
        return list(csv.reader(f))


class DefaultResponseTest(unittest.TestCase):
    def test_csv_exists_with_default_row(self):
        """預設回應有資料來源：Content/Data/DT_Responses.csv 存在，表頭 Name,Text，有 Default 列"""
        self.assertTrue(CSV_PATH.is_file())
        rows = read_csv(CSV_PATH)
        self.assertEqual(rows[0], ["Name", "Text"])
        self.assertIn("Default", [r[0] for r in rows[1:]])

    def test_text_matches_design_and_keeps_placeholder(self):
        """文字和設計文件 §5 一致，[對象] 占位符原樣保留"""
        rows = {r[0]: r[1] for r in read_csv(CSV_PATH)[1:]}
        self.assertEqual(rows["Default"], md_default_text())
        self.assertIn("[對象]", rows["Default"])
        self.assertEqual(rows["Default"], "[對象]紋絲不動，好像在嘲笑你：「就這？你是認真的嗎？」")

    def test_repo_version_equals_regenerated(self):
        """repo 內的 DT_Responses.csv 和重新產生的版本逐位元相同"""
        with tempfile.TemporaryDirectory() as d:
            r = run_script("--interactions", MD, "--out", d)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertEqual((Path(d) / "DT_Responses.csv").read_bytes(), CSV_PATH.read_bytes())

    def test_missing_section_fails_and_leaves_nothing(self):
        """§5 不見時腳本以非 0 結束，訊息有檔名，不留下檔案"""
        with tempfile.TemporaryDirectory() as d:
            bad = Path(d) / "item_interactions.md"
            bad.write_text(MD.read_text(encoding="utf-8").split("## 5. 預設回應")[0], encoding="utf-8")
            out = Path(d) / "out"
            r = run_script("--interactions", bad, "--out", out)
            self.assertNotEqual(r.returncode, 0)
            self.assertIn("item_interactions.md", r.stderr)
            self.assertEqual(list(out.iterdir()) if out.exists() else [], [])

    def test_missing_quote_fails(self):
        """§5 沒有引用區塊時以非 0 結束"""
        with tempfile.TemporaryDirectory() as d:
            bad = Path(d) / "item_interactions.md"
            bad.write_text("## 5. 預設回應\n\n沒有引用。\n", encoding="utf-8")
            r = run_script("--interactions", bad, "--out", Path(d) / "out")
            self.assertNotEqual(r.returncode, 0)
            self.assertIn("引用區塊", r.stderr)


if __name__ == "__main__":
    unittest.main()
