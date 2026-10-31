# 步驟 1 設計文件轉 CSV → VERIFY STEP 1
import csv
import hashlib
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "tools" / "design_to_csv.py"
DESIGN = ROOT / "docs" / "design"
INTERACTIONS_MD = DESIGN / "item_interactions.md"
SCRIPT_MD = DESIGN / "escape_room_script.md"
DATA_DIR = ROOT / "Content" / "Data"
CSV_NAMES = ["DT_Forms.csv", "DT_SceneObjects.csv", "DT_Locations.csv", "DT_Interactions.csv"]

EXPECTED_IDS = {
    "DT_Forms.csv": {f"F{i:02d}" for i in range(0, 13)},
    "DT_SceneObjects.csv": {f"S{i:02d}" for i in range(1, 21)},
    "DT_Locations.csv": {"L01", "L02"},
    "DT_Interactions.csv": {f"X{i:03d}" for i in range(1, 38)} - {"X019"},
}
ROW_ID_RE = re.compile(r"^\|\s*([FSLX]\d+)\s*\|", re.M)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_csv(path):
    with open(path, encoding="utf-8", newline="") as f:
        return list(csv.reader(f))


def run_script(*args, cwd=None):
    return subprocess.run(
        [sys.executable, "-I", str(SCRIPT), *map(str, args)],
        cwd=cwd, capture_output=True, text=True, timeout=60,
    )


class Base(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.tmp = Path(self._tmp.name)
        self.out = self.tmp / "out"

    def generate(self):
        r = run_script("--interactions", INTERACTIONS_MD, "--script", SCRIPT_MD, "--out", self.out)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def copy_docs(self):
        d = self.tmp / "docs"
        d.mkdir()
        i = d / "item_interactions.md"
        s = d / "escape_room_script.md"
        shutil.copy(INTERACTIONS_MD, i)
        shutil.copy(SCRIPT_MD, s)
        return i, s


class TestStep1DesignToCsv(Base):
    def test_run_script_produces_four_csv_files(self):
        """執行 `python3 -I tools/design_to_csv.py` → 在 `Content/Data/` 產生 `DT_Forms.csv`、`DT_SceneObjects.csv`、`DT_Locations.csv`、`DT_Interactions.csv`"""
        self.assertTrue(SCRIPT.is_file(), "tools/design_to_csv.py 不存在")
        self.generate()
        for name in CSV_NAMES:
            self.assertTrue((self.out / name).is_file(), f"{name} 沒有產生")

    def test_script_works_from_another_directory_with_default_inputs(self):
        """從別的資料夾執行 → 一樣讀 repo 裡的設計文件（輸出指到暫存資料夾）"""
        r = run_script("--out", self.out, cwd="/")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for name in CSV_NAMES:
            self.assertTrue((self.out / name).is_file(), f"{name} 沒有產生")

    def test_each_id_appears_exactly_once_in_matching_csv(self):
        """13 個形態（F00～F12）、20 個場景物件（S01～S20）、2 個特殊地點（L01、L02）、36 個互動（X001～X037，沒有 X019）→ 每個 ID 在對應的 CSV 裡剛好出現 1 次"""
        self.generate()
        counts = {"DT_Forms.csv": 13, "DT_SceneObjects.csv": 20, "DT_Locations.csv": 2, "DT_Interactions.csv": 36}
        for name in CSV_NAMES:
            rows = read_csv(self.out / name)
            self.assertEqual(rows[0][0], "Name", f"{name} 第一欄要叫 Name")
            ids = [r[0] for r in rows[1:]]
            self.assertEqual(len(ids), counts[name], f"{name} 列數")
            self.assertEqual(len(set(ids)), len(ids), f"{name} 有重複 ID")
            self.assertEqual(set(ids), EXPECTED_IDS[name], f"{name} 的 ID 集合不對")
            for r in rows[1:]:
                self.assertEqual(len(r), len(rows[0]), f"{name} {r[0]} 欄位數和表頭不同")
        self.assertNotIn("X019", [r[0] for r in read_csv(self.out / "DT_Interactions.csv")])

    def test_mixed_x_rows_in_form_table_go_to_interactions(self):
        """形態表裡混進的 X027、X032、X035 → 出現在 `DT_Interactions.csv`，**不在** `DT_Forms.csv`"""
        self.generate()
        forms = {r[0] for r in read_csv(self.out / "DT_Forms.csv")}
        inter = {r[0] for r in read_csv(self.out / "DT_Interactions.csv")}
        for xid in ("X027", "X032", "X035"):
            self.assertIn(xid, inter)
            self.assertNotIn(xid, forms)

    def test_chinese_fullwidth_and_cake_emoji_preserved_in_utf8(self):
        """文字欄位裡的中文、全形標點、`🎂` → 原樣保留，CSV 用 UTF-8"""
        self.generate()
        md_text = INTERACTIONS_MD.read_text(encoding="utf-8")
        cells = []
        for line in md_text.splitlines():
            if line.startswith("|") and "🎂" in line:
                cells += [c.strip() for c in line.strip().strip("|").split("|") if "🎂" in c]
        self.assertTrue(cells, "設計文件裡找不到含 🎂 的儲存格")
        raw = (self.out / "DT_Interactions.csv").read_bytes()
        text = raw.decode("utf-8")  # 不是 UTF-8 會在這裡丟例外
        csv_cells = [c for row in read_csv(self.out / "DT_Interactions.csv") for c in row]
        for cell in cells:
            self.assertIn(cell, csv_cells, "含 🎂 的儲存格沒有原樣保留")
            self.assertIn(cell.encode("utf-8"), raw)
        self.assertIn("「", text)

    def test_broken_row_fails_with_id_and_line_number(self):
        """把設計文件的某一列改壞（欄位數不對）→ 腳本以非 0 結束碼結束，錯誤訊息寫出是哪一個 ID、哪一行"""
        i, s = self.copy_docs()
        lines = i.read_text(encoding="utf-8").split("\n")
        idx = next(n for n, l in enumerate(lines) if l.startswith("| X013 |"))
        lines[idx] = lines[idx].replace(" | 地點、物品 |", " |", 1)  # 少一格
        i.write_text("\n".join(lines), encoding="utf-8")
        r = run_script("--interactions", i, "--script", s, "--out", self.out)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("X013", r.stderr)
        self.assertIn(str(idx + 1), r.stderr)
        self.assertIn("item_interactions.md", r.stderr)

    def test_broken_mixed_row_in_form_table_also_detected(self):
        """把設計文件的某一列改壞（欄位數不對）→ 形態表裡的 X 列也以互動表的欄數檢查"""
        i, s = self.copy_docs()
        lines = i.read_text(encoding="utf-8").split("\n")
        idx = next(n for n, l in enumerate(lines) if l.startswith("| X032 |"))
        lines[idx] = lines[idx].rstrip().rstrip("|").rstrip() + " | 多一格 |"
        i.write_text("\n".join(lines), encoding="utf-8")
        r = run_script("--interactions", i, "--script", s, "--out", self.out)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("X032", r.stderr)
        self.assertIn(str(idx + 1), r.stderr)

    def test_failure_leaves_no_partial_csv_and_keeps_old_ones(self):
        """出錯時不可以留下寫到一半的 CSV"""
        self.generate()
        before = {n: (self.out / n).read_bytes() for n in CSV_NAMES}
        i, s = self.copy_docs()
        lines = i.read_text(encoding="utf-8").split("\n")
        idx = next(n for n, l in enumerate(lines) if l.startswith("| S03 |"))
        lines[idx] = "| S03 | 飲料機 |"
        i.write_text("\n".join(lines), encoding="utf-8")
        r = run_script("--interactions", i, "--script", s, "--out", self.out)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("S03", r.stderr)
        self.assertEqual(sorted(p.name for p in self.out.iterdir()), sorted(CSV_NAMES))
        for n in CSV_NAMES:
            self.assertEqual((self.out / n).read_bytes(), before[n])
        # 全新資料夾也不會留下任何東西
        fresh = self.tmp / "fresh"
        r = run_script("--interactions", i, "--script", s, "--out", fresh)
        self.assertNotEqual(r.returncode, 0)
        self.assertEqual(list(fresh.iterdir()) if fresh.exists() else [], [])

    def test_duplicate_id_fails(self):
        """**不可以** 遺漏或重複任何 ID → 同一個 ID 出現兩次時以非 0 結束碼結束並指出 ID"""
        i, s = self.copy_docs()
        lines = i.read_text(encoding="utf-8").split("\n")
        idx = next(n for n, l in enumerate(lines) if l.startswith("| X001 |"))
        lines.insert(idx + 1, lines[idx])
        i.write_text("\n".join(lines), encoding="utf-8")
        r = run_script("--interactions", i, "--script", s, "--out", self.out)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("X001", r.stderr)

    def test_no_id_missing_or_duplicated_versus_markdown_regex_count(self):
        """**不可以** 遺漏或重複任何 ID → 從 md 原文用正規表示式數出的所有表格列 ID，和 CSV 的 ID 完全相同"""
        self.generate()
        md_ids = []
        for p in (INTERACTIONS_MD, SCRIPT_MD):
            md_ids += ROW_ID_RE.findall(p.read_text(encoding="utf-8"))
        csv_ids = []
        for name in CSV_NAMES:
            csv_ids += [r[0] for r in read_csv(self.out / name)[1:]]
        self.assertEqual(sorted(md_ids), sorted(csv_ids))
        self.assertEqual(len(md_ids), 13 + 20 + 2 + 36)

    def test_design_documents_not_modified(self):
        """**不可以** 修改 `docs/design/` 底下的檔案 → 跑腳本前後兩份 md 的 hash 相同"""
        before = (sha(INTERACTIONS_MD), sha(SCRIPT_MD))
        self.generate()
        run_script("--out", self.out)
        self.assertEqual(before, (sha(INTERACTIONS_MD), sha(SCRIPT_MD)))

    def test_committed_csv_matches_regenerated(self):
        """`Content/Data/` 裡 commit 的 CSV 和重新產生的一模一樣（資料和設計文件同步）"""
        self.generate()
        for name in CSV_NAMES:
            committed = DATA_DIR / name
            self.assertTrue(committed.is_file(), f"Content/Data/{name} 不存在")
            self.assertEqual(committed.read_bytes(), (self.out / name).read_bytes(), f"{name} 和設計文件不同步，請重跑 tools/design_to_csv.py")


if __name__ == "__main__":
    unittest.main()
