#!/usr/bin/env python3
"""設計文件 → Content/Data/DT_*.csv（步驟 1）。只用標準函式庫。

用法：python3 -I tools/design_to_csv.py [--interactions MD] [--script MD] [--out DIR]
不帶參數時讀 repo 裡的 docs/design/，輸出到 Content/Data/。

規則：
- 依每列「第一欄 ID 的前綴」（F、S、L、X）決定進哪個 CSV，不看它在哪一節。
  （形態表裡混進的 X027、X032、X035 因此會進 DT_Interactions。）
- 每個前綴的欄數與表頭，以該前綴自己的表頭為準（X 列一律 11 欄）。
- 第一欄名稱固定 `Name`（UE DataTable 匯入的 row name），值是 ID。
- 欄位數不對、ID 重複、表頭和預期不同 → 非 0 結束，stderr 寫出 ID、檔名、行號。
- 先全部在記憶體解析成功，再寫暫存檔，最後一起替換；出錯不留半成品。

中文表頭 → 英文欄名（步驟 2 的 USTRUCT 欄位名要和這裡一致）：
  形態表 (F, DT_Forms)：
    形態 ID→Name  形態名稱→FormName  取得方式→Acquisition  位置→Location
    形態能力／限制→Ability  說明→Description
  場景物件表 (S, DT_SceneObjects)：
    物件 ID→Name  名稱→ObjectName  位置→Location  說明→Description
  特殊地點表 (L, DT_Locations)：
    地點 ID→Name  地點名稱→LocationName  入口→Entrance  可進入形態→AllowedForms
    進入後內容→Contents  離開方式→ExitMethod
  互動表 (X, DT_Interactions)：
    互動 ID→Name  玩家形態→PlayerForm  互動對象→Target  事件類型→EventType
    事件描述→EventText  變身結果→TransformResult  出現物品→SpawnedItem
    生命值變化→HpChange  成就→Achievement  前置條件→Precondition  備註→Notes
"""
import argparse
import csv
import os
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# 前綴 → (輸出檔名, [(中文表頭, 英文欄名), ...])
SCHEMAS = {
    "F": ("DT_Forms.csv", [
        ("形態 ID", "Name"), ("形態名稱", "FormName"), ("取得方式", "Acquisition"),
        ("位置", "Location"), ("形態能力／限制", "Ability"), ("說明", "Description"),
    ]),
    "S": ("DT_SceneObjects.csv", [
        ("物件 ID", "Name"), ("名稱", "ObjectName"), ("位置", "Location"), ("說明", "Description"),
    ]),
    "L": ("DT_Locations.csv", [
        ("地點 ID", "Name"), ("地點名稱", "LocationName"), ("入口", "Entrance"),
        ("可進入形態", "AllowedForms"), ("進入後內容", "Contents"), ("離開方式", "ExitMethod"),
    ]),
    "X": ("DT_Interactions.csv", [
        ("互動 ID", "Name"), ("玩家形態", "PlayerForm"), ("互動對象", "Target"),
        ("事件類型", "EventType"), ("事件描述", "EventText"), ("變身結果", "TransformResult"),
        ("出現物品", "SpawnedItem"), ("生命值變化", "HpChange"), ("成就", "Achievement"),
        ("前置條件", "Precondition"), ("備註", "Notes"),
    ]),
}
HEADER_TO_PREFIX = {cols[0][0]: p for p, (_, cols) in SCHEMAS.items()}
ID_RE = re.compile(r"^([FSLX])\d+$")
SEP_RE = re.compile(r"^\|[\s:|-]+\|$")


class DesignError(Exception):
    pass


def split_row(line):
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [c.strip() for c in s.split("|")]


def read_tables(path):
    """回傳 [(行號, [儲存格])]，只含表格列（含表頭，不含分隔線）。"""
    rows = []
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if line.startswith("|") and not SEP_RE.match(line.strip()):
                rows.append((n, split_row(line)))
    return rows


def parse(paths):
    """paths: [Path]。回傳 {前綴: [(ID, 儲存格, 檔名, 行號)]}"""
    tables = [(p, read_tables(p)) for p in paths]
    # 第一輪：確認各前綴的表頭和預期一致
    for p, rows in tables:
        for n, cells in rows:
            prefix = HEADER_TO_PREFIX.get(cells[0])
            if prefix is None:
                continue
            expected = [c for c, _ in SCHEMAS[prefix][1]]
            if cells != expected:
                raise DesignError(f"{p.name}:{n}: 表頭 {cells[0]!r} 和預期不同，預期 {expected}，實際 {cells}")
    # 第二輪：資料列
    result = {p: [] for p in SCHEMAS}
    seen = {}
    for p, rows in tables:
        for n, cells in rows:
            m = ID_RE.match(cells[0])
            if not m:
                continue  # 其他表（結局、成就、基本資料……）或表頭
            prefix, rid = m.group(1), cells[0]
            want = len(SCHEMAS[prefix][1])
            if len(cells) != want:
                raise DesignError(f"{p.name}:{n}: {rid} 欄位數 {len(cells)}，應為 {want}")
            if rid in seen:
                raise DesignError(f"{p.name}:{n}: {rid} 重複，第一次出現在 {seen[rid]}")
            seen[rid] = f"{p.name}:{n}"
            result[prefix].append((rid, cells, p.name, n))
    return result


def check_native_tables(paths):
    """表頭是我們的表，但資料列的第一欄不是 ID 格式 → 視為壞列（避免悄悄漏掉）。"""
    for p in paths:
        in_table = None
        with open(p, encoding="utf-8") as f:
            for n, line in enumerate(f, 1):
                s = line.rstrip("\n")
                if not s.startswith("|"):
                    in_table = None
                    continue
                if SEP_RE.match(s.strip()):
                    continue
                cells = split_row(s)
                if cells[0] in HEADER_TO_PREFIX:
                    in_table = cells[0]
                elif in_table and not ID_RE.match(cells[0]):
                    raise DesignError(f"{p.name}:{n}: 在「{in_table}」表裡，第一欄 {cells[0]!r} 不是 ID")


def render(rows, prefix):
    import io
    buf = io.StringIO(newline="")
    w = csv.writer(buf, lineterminator="\n")
    w.writerow([en for _, en in SCHEMAS[prefix][1]])
    for _, cells, _, _ in rows:
        w.writerow(cells)
    return buf.getvalue()


def write_all(outputs, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    tmps = []
    try:
        for name, text in outputs.items():
            fd, tmp = tempfile.mkstemp(prefix=name + ".", suffix=".tmp", dir=out_dir)
            tmps.append((tmp, out_dir / name))
            with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
                f.write(text)
        for tmp, dst in tmps:
            os.replace(tmp, dst)
            os.chmod(dst, 0o644)
        tmps = []
    finally:
        for tmp, _ in tmps:
            try:
                os.unlink(tmp)
            except FileNotFoundError:
                pass


def main(argv=None):
    ap = argparse.ArgumentParser(description="設計文件 → Content/Data/DT_*.csv")
    ap.add_argument("--interactions", type=Path, default=ROOT / "docs/design/item_interactions.md")
    ap.add_argument("--script", type=Path, default=ROOT / "docs/design/escape_room_script.md",
                    help="主劇本；一起掃描 F/S/L/X 列（目前沒有）")
    ap.add_argument("--out", type=Path, default=ROOT / "Content/Data")
    args = ap.parse_args(argv)
    paths = [args.interactions] + ([args.script] if args.script else [])
    try:
        for p in paths:
            if not p.is_file():
                raise DesignError(f"找不到檔案：{p}")
        check_native_tables(paths)
        parsed = parse(paths)
        outputs = {SCHEMAS[p][0]: render(parsed[p], p) for p in SCHEMAS}
        write_all(outputs, args.out)
    except DesignError as e:
        print(f"design_to_csv 錯誤：{e}", file=sys.stderr)
        return 1
    for p in SCHEMAS:
        print(f"{SCHEMAS[p][0]}: {len(parsed[p])} 列")
    return 0


if __name__ == "__main__":
    sys.exit(main())
