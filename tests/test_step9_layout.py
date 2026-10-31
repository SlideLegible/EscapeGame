# 步驟 9 灰盒關卡 → VERIFY STEP 9
#
# 靜態檢查 Content/Data/Level_Layout.csv（關卡資料檔，9b 由它蓋出 Lvl_Academy 地圖）：
#   表頭、8 個區域、F01～F12／S01～S20 的出現次數、每個物件在設計文件寫的區域盒子裡、
#   出生點在密碼學教室、S19／S05／S09 是門、牆與門夠高（跳不過）、Id 不重複（馬桶除外）。
# 區域對照表是這個測試自己的規格，和 UE spec 各寫一份，不共用實作。
# 檔案不存在時，每個測試都是「斷言失敗」，不是 import 錯誤。
import csv
import io
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "Content" / "Data"
LAYOUT = DATA / "Level_Layout.csv"

HEADER = ["Kind", "Id", "Area", "X", "Y", "Z", "SX", "SY", "SZ", "Yaw"]
KINDS = {"Area", "Floor", "Wall", "Door", "Object", "Spawn"}
AREAS = ["Classroom", "Corridor", "Office", "Restroom", "Storage", "GuardRoom", "L01", "L02"]
DOORS = ["S19", "S05", "S09"]
MIN_WALL_HEIGHT = 400

# 設計文件的位置文字 → 區域 ID
LOCATION_TO_AREA = {
    "密碼學教室": "Classroom",
    "走廊": "Corridor",
    "走廊（花盆裡）": "Corridor",
    "辦公室": "Office",
    "廁所": "Restroom",
    "廁所（每間都有）": "Restroom",
    "廁所儲物間內部": "Storage",
    "警衛室": "GuardRoom",
    "飲料機內部": "L01",
    "飲料機底下": "L02",
}

FORM_IDS = [f"F{n:02d}" for n in range(1, 13)]
SCENE_IDS = [f"S{n:02d}" for n in range(1, 21)]


def read_csv_rows(path):
    return list(csv.DictReader(io.StringIO(path.read_text(encoding="utf-8"))))


def design_locations():
    """Id → 設計文件寫的位置文字（F01～F12、S01～S20）"""
    out = {}
    for row in read_csv_rows(DATA / "DT_Forms.csv"):
        if row["Name"] != "F00":
            out[row["Name"]] = row["Location"].strip()
    for row in read_csv_rows(DATA / "DT_SceneObjects.csv"):
        out[row["Name"]] = row["Location"].strip()
    return out


class LayoutCase(unittest.TestCase):
    def load(self):
        """讀 Level_Layout.csv；不存在或格式壞掉 → 斷言失敗"""
        self.assertTrue(LAYOUT.is_file(), "Content/Data/Level_Layout.csv 不存在")
        raw = LAYOUT.read_bytes()
        self.assertFalse(raw.startswith(b"\xef\xbb\xbf"), "Level_Layout.csv 不可以有 BOM")
        self.assertNotIn(b"\r", raw, "Level_Layout.csv 要用 \\n 換行")
        text = raw.decode("utf-8")
        reader = csv.reader(io.StringIO(text))
        rows = [r for r in reader if r]
        self.assertTrue(rows, "Level_Layout.csv 是空的")
        self.assertEqual(rows[0], HEADER, "表頭要剛好是 " + ",".join(HEADER))
        out = []
        for n, r in enumerate(rows[1:], 2):
            self.assertEqual(len(r), len(HEADER), f"第 {n} 列欄位數不是 {len(HEADER)}：{r}")
            d = dict(zip(HEADER, r))
            self.assertIn(d["Kind"], KINDS, f"第 {n} 列 Kind 不認得：{d['Kind']}")
            for col in HEADER[3:]:
                try:
                    d[col] = float(d[col])
                except ValueError:
                    self.fail(f"第 {n} 列 {col} 不是數字：{d[col]!r}")
            for col in ("SX", "SY", "SZ"):
                self.assertGreater(d[col], 0, f"第 {n} 列（{d['Id']}）{col} 要大於 0")
            out.append(d)
        return out

    def areas(self, rows):
        return {r["Id"]: r for r in rows if r["Kind"] == "Area"}

    @staticmethod
    def inside(row, box):
        return all(
            abs(row[c] - box[c]) <= box[s] / 2 + 1e-6
            for c, s in (("X", "SX"), ("Y", "SY"), ("Z", "SZ"))
        )


class TestLevelLayout(LayoutCase):
    def test_layout_file_has_exact_header_and_known_kinds(self):
        """Level_Layout.csv 表頭剛好 Kind,Id,Area,X,Y,Z,SX,SY,SZ,Yaw，UTF-8 無 BOM、\\n 換行"""
        self.load()

    def test_open_main_level_has_eight_areas(self):
        """開啟主關卡 → 有密碼學教室、走廊、辦公室、廁所、儲物間、警衛室、飲料機內部、飲料機底下 8 個區域"""
        rows = self.load()
        ids = [r["Id"] for r in rows if r["Kind"] == "Area"]
        self.assertEqual(sorted(ids), sorted(AREAS), "Area 列要剛好是 8 個區域各一列")
        for r in rows:
            if r["Kind"] == "Area":
                self.assertEqual(r["Area"], r["Id"], f"Area 列 {r['Id']} 的 Area 欄要等於 Id")

    def test_every_form_and_scene_object_placed_exactly_once_in_design_area(self):
        """自動化測試掃描主關卡 → F01～F12 每個可變身物品、S01～S20 每個場景物件都剛好放了 1 個（馬桶可以多個），而且放在設計文件寫的區域"""
        rows = self.load()
        areas = self.areas(rows)
        self.assertEqual(sorted(areas), sorted(AREAS), "先要有 8 個區域")
        design = design_locations()
        placed = {}
        for r in rows:
            if r["Kind"] in ("Object", "Door"):
                placed.setdefault(r["Id"], []).append(r)

        problems = []
        for oid in FORM_IDS + SCENE_IDS:
            text = design.get(oid)
            if text not in LOCATION_TO_AREA:
                problems.append(f"{oid} 的設計位置文字 {text!r} 不在區域對照表裡")
                continue
            want = LOCATION_TO_AREA[text]
            rs = placed.get(oid, [])
            if oid == "S11":
                if not rs:
                    problems.append("S11 馬桶至少要放 1 個")
            elif len(rs) != 1:
                problems.append(f"{oid} 要剛好放 1 個，實際 {len(rs)} 個")
            for r in rs:
                if r["Area"] != want:
                    problems.append(f"{oid} 的 Area 欄是 {r['Area']}，設計文件（{text}）要 {want}")
                elif not self.inside(r, areas[want]):
                    problems.append(f"{oid} 的中心點不在區域 {want} 的盒子裡")
        extra = sorted(set(placed) - set(FORM_IDS) - set(SCENE_IDS))
        if extra:
            problems.append(f"出現不認得的 Object／Door Id：{extra}")
        self.assertEqual(problems, [], "\n".join(problems))

    def test_three_doors_are_door_rows_not_object_rows(self):
        """S19 教室門、S05 辦公室門、S09 儲物間門是 Door 列（不再有同 Id 的 Object 列），Door 列只有這三個"""
        rows = self.load()
        door_ids = sorted(r["Id"] for r in rows if r["Kind"] == "Door")
        self.assertEqual(door_ids, sorted(DOORS))
        object_ids = {r["Id"] for r in rows if r["Kind"] == "Object"}
        self.assertEqual(sorted(object_ids & set(DOORS)), [], "S19／S05／S09 不可以另外出現 Object 列")

    def test_player_spawns_in_classroom(self):
        """開局玩家出生在密碼學教室"""
        rows = self.load()
        areas = self.areas(rows)
        self.assertIn("Classroom", areas, "沒有 Classroom 區域")
        spawns = [r for r in rows if r["Kind"] == "Spawn"]
        self.assertTrue(spawns, "至少要有一列 Spawn")
        for s in spawns:
            self.assertEqual(s["Area"], "Classroom", f"Spawn {s['Id']} 的 Area 要是 Classroom")
            self.assertTrue(self.inside(s, areas["Classroom"]), f"Spawn {s['Id']} 不在教室盒子裡")

    def test_walls_and_doors_are_too_tall_to_jump_over(self):
        """不可以 牆或門矮到跳得過去：每個 Wall 與 Door 的 SZ 至少 400"""
        rows = self.load()
        short = [f"{r['Kind']} {r['Id']} SZ={r['SZ']:g}" for r in rows
                 if r["Kind"] in ("Wall", "Door") and r["SZ"] < MIN_WALL_HEIGHT]
        self.assertEqual(short, [], "太矮：" + "；".join(short))
        self.assertTrue(any(r["Kind"] == "Wall" for r in rows), "一道牆都沒有")

    def test_ids_are_unique_except_toilets(self):
        """除了 S11 馬桶可以多個，所有 Id 都不重複"""
        rows = self.load()
        seen = {}
        for r in rows:
            if r["Kind"] == "Spawn":
                continue
            seen.setdefault(r["Id"], []).append(r["Kind"])
        dup = {k: v for k, v in seen.items() if len(v) > 1 and k != "S11"}
        self.assertEqual(dup, {}, f"Id 重複：{dup}")
        s11 = [r for r in rows if r["Id"] == "S11"]
        self.assertTrue(all(r["Kind"] == "Object" for r in s11), "S11 只能是 Object 列")

    def test_floor_and_wall_area_column_is_blank_or_known_area(self):
        """Floor、Wall 的 Area 欄不是空白，就是 8 個區域之一"""
        rows = self.load()
        bad = [f"{r['Kind']} {r['Id']} Area={r['Area']!r}" for r in rows
               if r["Kind"] in ("Floor", "Wall") and r["Area"] != "" and r["Area"] not in AREAS]
        self.assertEqual(bad, [], "；".join(bad))
        self.assertTrue(any(r["Kind"] == "Floor" for r in rows), "一塊地板都沒有")

    def test_every_id_is_well_formed(self):
        """Object、Door 的 Id 都是 F01～F12 或 S01～S20 的格式"""
        rows = self.load()
        bad = [r["Id"] for r in rows if r["Kind"] in ("Object", "Door")
               and not re.fullmatch(r"F(0[1-9]|1[0-2])|S(0[1-9]|1[0-9]|20)", r["Id"])]
        self.assertEqual(bad, [])


class TestLocationTable(unittest.TestCase):
    """對照表本身要涵蓋設計文件所有位置文字（不需要 Level_Layout.csv）"""

    def test_location_table_covers_design_documents(self):
        """區域對照表涵蓋 DT_Forms、DT_SceneObjects 的每一個位置文字"""
        design = design_locations()
        missing = {k: v for k, v in design.items()
                   if (k in FORM_IDS or k in SCENE_IDS) and v not in LOCATION_TO_AREA}
        self.assertEqual(missing, {}, f"對照表缺：{missing}")


if __name__ == "__main__":
    unittest.main()
