# 步驟 10 介面 → VERIFY STEP 10
#
# 介面的資料面（Python 部分）：
#   1. tools/design_to_endings.py：設計文件 §7、§8 → Content/Data/DT_Endings.csv、DT_Achievements.csv
#      命令列：--script MD（預設 docs/design/escape_room_script.md）、--out DIR（預設 Content/Data）；兩個檔一起寫。
#   2. Content/Data/DT_UiText.csv：畫面上所有固定文字（Name,Text）。
#   3. 字型涵蓋（VERIFY：沒有方塊字）：Content/Fonts 的 Noto Sans CJK（TTC 的第 3 個字型＝TC）加 NotoColorEmoji，
#      兩個字型的 cmap 要涵蓋 Content/Data 所有 CSV 的所有字元。cmap 解析器寫在這個測試裡，並用系統字型先測過解析器本身。
# 工具、CSV、字型還不存在時，每個測試都是「斷言失敗」，不是 import 錯誤。
import csv
import io
import re
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "tools" / "design_to_endings.py"
MD = ROOT / "docs" / "design" / "escape_room_script.md"
DATA = ROOT / "Content" / "Data"
ENDINGS_CSV = DATA / "DT_Endings.csv"
ACHIEVEMENTS_CSV = DATA / "DT_Achievements.csv"
UITEXT_CSV = DATA / "DT_UiText.csv"
FONT_DIR = ROOT / "Content" / "Fonts"
CJK_FONT = FONT_DIR / "NotoSansCJK-Regular.ttc"
EMOJI_FONT = FONT_DIR / "NotoColorEmoji.ttf"
OFL_LICENSE = FONT_DIR / "LICENSE-OFL.txt"
SYSTEM_CJK = Path("/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc")
ENGINE_EMOJI = Path("/home/lifestone/UnrealEngine/Engine/Content/Editor/Slate/Fonts/NotoColorEmoji.ttf")

TC_FACE_INDEX = 3  # NotoSansCJK-Regular.ttc 的第 4 個字型（從 0 數起的 3）是 Noto Sans CJK TC
ENDING_IDS = ["E01", "E02", "E03", "E04", "E05"]
ACHIEVEMENT_IDS = ENDING_IDS + ["A01", "A02", "A03", "A04", "A06", "A07"]  # 沒有 A05
ENDINGS_HEADER = ["Name", "Title", "Kind", "Scope", "Story"]
ACHIEVEMENTS_HEADER = ["Name", "Title", "Condition"]
UITEXT_REQUIRED = ["HudHp", "HudForm", "KeypadTitle", "KeypadDelete", "KeypadConfirm", "EndingContinue", "ToastPrefix"]
# 不會畫出來的字元：變體選擇符（U+FE00～FE0F，例如 ⚠️ 的 FE0F）與零寬連字（U+200D）。
# HarfBuzz 把它們當 default-ignorable，不會變成方塊；它們在 cmap format 14，這個測試不解析。
IGNORABLE = set(range(0xFE00, 0xFE10)) | {0x200D}


# ---------------------------------------------------------------- 工具函式

def run_script(*args):
    return subprocess.run([sys.executable, "-I", str(SCRIPT), *map(str, args)],
                          capture_output=True, text=True, timeout=60, cwd="/")


def read_csv(path):
    with open(path, encoding="utf-8", newline="") as f:
        return list(csv.reader(f))


def md_lines(path=MD):
    return path.read_text(encoding="utf-8").split("\n")


def table_rows(lines, section_prefix, id_re):
    """章節（以 section_prefix 開頭的 `## ` 標題）裡，第一欄符合 id_re 的表格列：[(行號, [儲存格…])]"""
    start = next(n for n, l in enumerate(lines) if l.startswith(section_prefix))
    rows = []
    for n in range(start + 1, len(lines)):
        if lines[n].startswith("## "):
            break
        if lines[n].startswith("|"):
            cells = [c.strip() for c in lines[n].strip().strip("|").split("|")]
            if re.fullmatch(id_re, cells[0]):
                rows.append((n + 1, cells))
    return rows


def story_section(lines, heading_prefix):
    """`### heading_prefix…` 到下一個標題或 `---` 之間的非空行；引用的 `> ` 去掉；用 \\n 接起來。"""
    start = next(n for n, l in enumerate(lines) if l.startswith(heading_prefix))
    body = []
    for n in range(start + 1, len(lines)):
        l = lines[n]
        if l.startswith("#") or l.strip() == "---":
            break
        if l.startswith(">"):
            l = l[1:].strip()
        l = l.strip()
        if l:
            body.append(l)
    return "\n".join(body)


def expected_endings():
    """從設計文件 §7、§8 獨立算出 DT_Endings.csv 該有的列。"""
    lines = md_lines()
    sec7 = {c[0]: c for _, c in table_rows(lines, "## 7.", r"E0\d")}
    sec8 = {c[0]: c for _, c in table_rows(lines, "## 8.", r"E0\d")}
    stories = {}
    for e in ("E01", "E02", "E03"):
        stories[e] = story_section(lines, "### 劇情：夢醒")
    stories["E04"] = story_section(lines, "### 劇情：被叫醒")
    stories["E05"] = story_section(lines, "### 壞結局")
    out = []
    for e in ENDING_IDS:
        out.append([e, sec8[e][1], sec7[e][2], sec7[e][3], stories[e]])
    return out


def expected_achievements():
    lines = md_lines()
    rows = {c[0]: c for _, c in table_rows(lines, "## 8.", r"[EA]\d\d")}
    return [[i, rows[i][1], rows[i][2]] for i in ACHIEVEMENT_IDS]


# ---------------------------------------------------------------- 字型 cmap 解析器（標準函式庫 struct）

def _u16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def _u32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def sfnt_offsets(data):
    """TTC：header 的各字型 offset table 位置；一般 TTF／OTF：[0]。"""
    if data[:4] == b"ttcf":
        n = _u32(data, 8)
        return [_u32(data, 12 + 4 * i) for i in range(n)]
    return [0]


def sfnt_tables(data, base):
    """offset table → {tag: (offset, length)}（table directory 的 offset 是整個檔案的絕對位置）"""
    n = _u16(data, base + 4)
    out = {}
    for i in range(n):
        o = base + 12 + 16 * i
        out[data[o:o + 4].decode("latin1")] = (_u32(data, o + 8), _u32(data, o + 12))
    return out


def cmap_codepoints(data, base):
    """這個字型（offset table 在 base）的 cmap 裡 format 4 與 format 12 子表涵蓋的碼位集合（對應到非 0 的 glyph）。"""
    off, _ = sfnt_tables(data, base)["cmap"]
    count = _u16(data, off + 2)
    cps = set()
    for i in range(count):
        sub = off + _u32(data, off + 4 + 8 * i + 4)
        fmt = _u16(data, sub)
        if fmt == 4:
            segs = _u16(data, sub + 6) // 2
            ends = sub + 14
            starts = ends + 2 * segs + 2
            deltas = starts + 2 * segs
            range_offsets = deltas + 2 * segs
            for s in range(segs):
                end, start = _u16(data, ends + 2 * s), _u16(data, starts + 2 * s)
                delta, ro = _u16(data, deltas + 2 * s), _u16(data, range_offsets + 2 * s)
                for c in range(start, end + 1):
                    if c == 0xFFFF:
                        continue
                    if ro == 0:
                        gid = (c + delta) & 0xFFFF
                    else:
                        gid = _u16(data, range_offsets + 2 * s + ro + 2 * (c - start))
                        if gid:
                            gid = (gid + delta) & 0xFFFF
                    if gid:
                        cps.add(c)
        elif fmt == 12:
            groups = _u32(data, sub + 12)
            for g in range(groups):
                go = sub + 16 + 12 * g
                start, end, gid = _u32(data, go), _u32(data, go + 4), _u32(data, go + 8)
                if gid == 0 and start == end:
                    continue
                cps.update(range(start, end + 1))
        # 其他格式（例如 format 14 變體選擇符）不解析
    return cps


def family_name(data, base):
    """name table 的 nameID 1（family），Windows／Unicode BMP 的 UTF-16BE 字串；找不到回傳 ""。"""
    off, _ = sfnt_tables(data, base)["name"]
    count, str_off = _u16(data, off + 2), _u16(data, off + 4)
    for i in range(count):
        r = off + 6 + 12 * i
        platform, name_id, length, offset = _u16(data, r), _u16(data, r + 6), _u16(data, r + 8), _u16(data, r + 10)
        if platform == 3 and name_id == 1:
            return data[off + str_off + offset: off + str_off + offset + length].decode("utf-16-be")
    return ""


def data_chars():
    """Content/Data 所有 CSV 的所有字元（空白與換行除外）→ {字元: {檔名…}}；另加數字 0～9、':'、'：'（HUD 時間與固定文字）。"""
    chars = {}
    for p in sorted(DATA.glob("*.csv")):
        for row in read_csv(p):
            for cell in row:
                for ch in cell:
                    if not ch.isspace():
                        chars.setdefault(ch, set()).add(p.name)
    for ch in "0123456789:：":
        chars.setdefault(ch, set()).add("(HUD 固定字元)")
    return chars


# ---------------------------------------------------------------- 解析器本身（用系統字型驗證；工具與字型還沒做出來時就能跑）

class FontParserSelfTest(unittest.TestCase):
    @unittest.skipUnless(SYSTEM_CJK.is_file(), "系統沒有 NotoSansCJK-Regular.ttc")
    def test_parser_reads_system_cjk_ttc(self):
        """cmap 解析器：系統的 NotoSansCJK-Regular.ttc 第 3 個字型（TC）有「人」，沒有 🎂（U+1F382）"""
        data = SYSTEM_CJK.read_bytes()
        self.assertEqual(data[:4], b"ttcf")
        offsets = sfnt_offsets(data)
        self.assertGreater(len(offsets), TC_FACE_INDEX)
        self.assertIn("TC", family_name(data, offsets[TC_FACE_INDEX]))
        cps = cmap_codepoints(data, offsets[TC_FACE_INDEX])
        self.assertGreater(len(cps), 30000)
        for ch in "人類生命值：0123456789:":
            self.assertIn(ord(ch), cps, ch)
        self.assertNotIn(0x1F382, cps)

    @unittest.skipUnless(ENGINE_EMOJI.is_file(), "引擎沒有 NotoColorEmoji.ttf")
    def test_parser_reads_engine_emoji_ttf(self):
        """cmap 解析器：引擎的 NotoColorEmoji.ttf（一般 TTF）有 🎂，沒有「人」"""
        data = ENGINE_EMOJI.read_bytes()
        self.assertEqual(sfnt_offsets(data), [0])
        cps = cmap_codepoints(data, 0)
        self.assertIn(0x1F382, cps)
        self.assertNotIn(ord("人"), cps)


# ---------------------------------------------------------------- design_to_endings.py

class EndingsToolTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.tmp = Path(self._tmp.name)

    def generate(self, script=MD, out=None):
        self.assertTrue(SCRIPT.is_file(), f"{SCRIPT} 不存在")
        out = out or (self.tmp / "out")
        r = run_script("--script", script, "--out", out)
        return r, out

    def bad_script(self, mutate):
        text = MD.read_text(encoding="utf-8")
        new = mutate(text)
        self.assertNotEqual(new, text, "測試的竄改沒有生效")
        bad = self.tmp / "escape_room_script.md"
        bad.write_text(new, encoding="utf-8")
        return bad

    def test_tool_exists(self):
        """tools/design_to_endings.py 存在"""
        self.assertTrue(SCRIPT.is_file(), f"{SCRIPT} 不存在")

    def test_endings_csv_matches_design(self):
        """DT_Endings.csv：E01～E05 五列，Title 是 §8 結局成就名稱、Kind／Scope 是 §7 表格、Story 是 §7 劇情"""
        r, out = self.generate()
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        rows = read_csv(out / "DT_Endings.csv")
        self.assertEqual(rows[0], ENDINGS_HEADER)
        self.assertEqual(rows[1:], expected_endings())

    def test_endings_story_content(self):
        """結局劇情：E01～E03 同一段夢醒、E04 被叫醒、E05 遊戲重啟；E01～E03 有「下課」，E04 有「睡得很熟嘛」"""
        r, out = self.generate()
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        story = {row[0]: row[4] for row in read_csv(out / "DT_Endings.csv")[1:]}
        for e in ("E01", "E02", "E03"):
            self.assertIn("好，那我們今天就上到這邊，下課。", story[e])
            self.assertIn("眼前一片白光", story[e])
            self.assertNotIn("睡得很熟嘛", story[e])
            self.assertFalse(story[e].lstrip().startswith(">"), "引用符號 > 要去掉")
        self.assertEqual(story["E01"], story["E02"])
        self.assertEqual(story["E02"], story["E03"])
        self.assertIn("同學，下課了喔，睡得很熟嘛。", story["E04"])
        self.assertIn("窗外透進清晨的陽光", story["E04"])
        self.assertEqual(story["E05"], "遊戲重啟，從第一關重新開始。")

    def test_achievements_csv_ten_rows_without_a05(self):
        """DT_Achievements.csv：E01～E05、A01～A04、A06、A07 共 10 列，沒有 A05；名稱與解鎖條件和 §8 一致"""
        r, out = self.generate()
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        rows = read_csv(out / "DT_Achievements.csv")
        self.assertEqual(rows[0], ACHIEVEMENTS_HEADER)
        self.assertEqual([x[0] for x in rows[1:]], ACHIEVEMENT_IDS)
        self.assertEqual(len(rows) - 1, 10)
        self.assertNotIn("A05", [x[0] for x in rows])
        self.assertEqual(rows[1:], expected_achievements())
        by_id = {x[0]: x[1] for x in rows[1:]}
        self.assertEqual(by_id["A06"], "時間管理大師")
        self.assertEqual(by_id["E05"], "上課睡覺的代價")

    def test_outputs_are_utf8_without_bom(self):
        """輸出是 UTF-8、沒有 BOM、行尾 \\n"""
        r, out = self.generate()
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for name in ("DT_Endings.csv", "DT_Achievements.csv"):
            raw = (out / name).read_bytes()
            self.assertFalse(raw.startswith(b"\xef\xbb\xbf"), name)
            self.assertNotIn(b"\r", raw, name)
            raw.decode("utf-8")

    def test_repo_csvs_equal_regenerated(self):
        """repo 內的 DT_Endings.csv、DT_Achievements.csv 和重新產生的版本逐位元相同"""
        r, out = self.generate()
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for name in ("DT_Endings.csv", "DT_Achievements.csv"):
            repo = DATA / name
            self.assertTrue(repo.is_file(), f"{repo} 不存在")
            self.assertEqual(repo.read_bytes(), (out / name).read_bytes(), name)

    def test_malformed_achievement_row_fails_with_line_number(self):
        """§8 的成就列欄位不夠 → 非 0 結束，stderr 有檔名和該列的行號，不留下檔案"""
        lines = md_lines()
        n = next(n for n, c in table_rows(lines, "## 8.", r"E03"))
        bad = self.bad_script(lambda t: t.replace("| E03 | 誰說一定要走正門 | 跳窗逃出 | |", "| E03 | 誰說一定要走正門 |"))
        r, out = self.generate(bad)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("escape_room_script.md", r.stderr)
        self.assertIn(str(n), r.stderr)
        self.assertEqual(list(out.iterdir()) if out.exists() else [], [])

    def test_missing_story_section_fails(self):
        """§7 少了「劇情：被叫醒」段落 → 非 0 結束，stderr 有檔名，不留下檔案"""
        bad = self.bad_script(lambda t: t.replace("### 劇情：被叫醒（E04）", "### 別的標題"))
        r, out = self.generate(bad)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("escape_room_script.md", r.stderr)
        self.assertEqual(list(out.iterdir()) if out.exists() else [], [])

    def test_missing_ending_row_fails(self):
        """§7 結局表少了 E05 → 非 0 結束，stderr 指出 E05，不留下檔案"""
        bad = self.bad_script(lambda t: re.sub(r"^\| E05 \| 血量歸零 \|.*\n", "", t, flags=re.M))
        r, out = self.generate(bad)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("E05", r.stderr)
        self.assertEqual(list(out.iterdir()) if out.exists() else [], [])

    def test_missing_input_file_fails(self):
        """設計文件不存在 → 非 0 結束，不留下檔案"""
        r, out = self.generate(self.tmp / "nope.md")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("nope.md", r.stderr)
        self.assertEqual(list(out.iterdir()) if out.exists() else [], [])


# ---------------------------------------------------------------- DT_UiText.csv

class UiTextTest(unittest.TestCase):
    def test_required_names_exist(self):
        """DT_UiText.csv：表頭 Name,Text；HudHp、HudForm、KeypadTitle、KeypadDelete、KeypadConfirm、EndingContinue、ToastPrefix 都在、文字不空"""
        self.assertTrue(UITEXT_CSV.is_file(), f"{UITEXT_CSV} 不存在")
        raw = UITEXT_CSV.read_bytes()
        self.assertFalse(raw.startswith(b"\xef\xbb\xbf"), "不可以有 BOM")
        rows = read_csv(UITEXT_CSV)
        self.assertEqual(rows[0], ["Name", "Text"])
        table = {r[0]: r[1] for r in rows[1:]}
        self.assertEqual(len(table), len(rows) - 1, "Name 不可重複")
        for name in UITEXT_REQUIRED:
            self.assertIn(name, table)
            self.assertTrue(table[name].strip(), f"{name} 的文字是空的")

    def test_hud_formats_have_placeholder_and_traditional_chinese(self):
        """HudHp、HudForm 含 {0} 占位符；刪除＝刪除、確認＝確認（繁體中文）"""
        self.assertTrue(UITEXT_CSV.is_file(), f"{UITEXT_CSV} 不存在")
        table = {r[0]: r[1] for r in read_csv(UITEXT_CSV)[1:]}
        self.assertIn("{0}", table.get("HudHp", ""))
        self.assertIn("{0}", table.get("HudForm", ""))
        self.assertEqual(table.get("KeypadDelete"), "刪除")
        self.assertEqual(table.get("KeypadConfirm"), "確認")
        self.assertEqual(table.get("HudHp", "").count("{0}"), 1)
        self.assertTrue(re.search(r"[一-鿿]", table.get("HudHp", "")), "生命值文字要有中文")
        self.assertTrue(re.search(r"[一-鿿]", table.get("HudForm", "")), "形態文字要有中文")


# ---------------------------------------------------------------- 字型涵蓋

class FontCoverageTest(unittest.TestCase):
    def test_font_files_and_license_exist(self):
        """字型檔存在：Content/Fonts 的 NotoSansCJK-Regular.ttc、NotoColorEmoji.ttf、LICENSE-OFL.txt；.gitattributes 讓字型走 LFS"""
        self.assertTrue(CJK_FONT.is_file(), f"{CJK_FONT} 不存在")
        self.assertTrue(EMOJI_FONT.is_file(), f"{EMOJI_FONT} 不存在")
        self.assertTrue(OFL_LICENSE.is_file(), f"{OFL_LICENSE} 不存在")
        self.assertIn("Open Font License", OFL_LICENSE.read_text(encoding="utf-8", errors="replace"))
        attrs = (ROOT / ".gitattributes").read_text(encoding="utf-8")
        for ext in ("ttc", "ttf", "otf"):
            self.assertRegex(attrs, rf"^\*\.{ext}\s+filter=lfs", f"*.{ext} 沒有走 LFS")

    def test_cjk_font_is_tc_subface(self):
        """預設字型是 TTC 的第 3 個字型（Noto Sans CJK TC），不是方塊字重災區的別種字型"""
        self.assertTrue(CJK_FONT.is_file(), f"{CJK_FONT} 不存在")
        data = CJK_FONT.read_bytes()
        self.assertEqual(data[:4], b"ttcf")
        offsets = sfnt_offsets(data)
        self.assertGreater(len(offsets), TC_FACE_INDEX)
        self.assertIn("TC", family_name(data, offsets[TC_FACE_INDEX]))

    def test_every_character_in_data_has_a_glyph(self):
        """Content/Data 所有 CSV 的每個字元（空白與換行除外）加數字 0～9、':'、'：' 都在 CJK 字型（TC）或表情符號字型裡，沒有方塊字"""
        self.assertTrue(CJK_FONT.is_file(), f"{CJK_FONT} 不存在")
        self.assertTrue(EMOJI_FONT.is_file(), f"{EMOJI_FONT} 不存在")
        cjk = CJK_FONT.read_bytes()
        cjk_cps = cmap_codepoints(cjk, sfnt_offsets(cjk)[TC_FACE_INDEX])
        emoji_cps = cmap_codepoints(EMOJI_FONT.read_bytes(), 0)
        missing = {}
        for ch, files in data_chars().items():
            cp = ord(ch)
            if cp in IGNORABLE or cp in cjk_cps or cp in emoji_cps:
                continue
            missing[f"U+{cp:04X} {ch}"] = sorted(files)
        self.assertEqual(missing, {}, "這些字元兩個字型都沒有，畫面會出現方塊")

    def test_emoji_in_event_text_needs_the_emoji_font(self):
        """DT_Interactions 的 🎂（X033 之類事件文字）不在 CJK 字型裡、在表情符號字型裡（所以字型一定要有 fallback）"""
        self.assertTrue(CJK_FONT.is_file(), f"{CJK_FONT} 不存在")
        self.assertTrue(EMOJI_FONT.is_file(), f"{EMOJI_FONT} 不存在")
        self.assertIn("🎂", (DATA / "DT_Interactions.csv").read_text(encoding="utf-8"))
        cjk = CJK_FONT.read_bytes()
        self.assertNotIn(0x1F382, cmap_codepoints(cjk, sfnt_offsets(cjk)[TC_FACE_INDEX]))
        self.assertIn(0x1F382, cmap_codepoints(EMOJI_FONT.read_bytes(), 0))


if __name__ == "__main__":
    unittest.main()
