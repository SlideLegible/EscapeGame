#!/usr/bin/env python3
"""設計文件 §7（結局）、§8（成就）→ Content/Data/DT_Endings.csv、DT_Achievements.csv（步驟 10）。只用標準函式庫。

用法：python3 -I tools/design_to_endings.py [--script MD] [--out DIR]
  DT_Endings.csv       Name,Title,Kind,Scope,Story    E01～E05；Title＝§8 結局成就名稱，Kind／Scope＝§7 表格原文，
                       Story＝§7 劇情段落（E01～E03 夢醒、E04 被叫醒、E05 壞結局），非空行去掉開頭 `> `，用 \\n 接
  DT_Achievements.csv  Name,Title,Condition           E01～E05、A01～A04、A06、A07（沒有 A05）；Condition＝§8 解鎖條件原文
格式錯誤 → 結束碼 1，stderr 寫檔名（和行號）與原因；錯誤時不留下任何輸出檔。
"""
import argparse
import csv
import io
import os
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENDINGS_NAME = "DT_Endings.csv"
ACHIEVEMENTS_NAME = "DT_Achievements.csv"
ENDING_IDS = ["E01", "E02", "E03", "E04", "E05"]
ACHIEVEMENT_IDS = ENDING_IDS + ["A01", "A02", "A03", "A04", "A06", "A07"]
# 結局 → 劇情段落標題（以此開頭的 `###` 標題）
STORY_HEADINGS = {
    "E01": "### 劇情：夢醒", "E02": "### 劇情：夢醒", "E03": "### 劇情：夢醒",
    "E04": "### 劇情：被叫醒", "E05": "### 壞結局",
}


class DesignError(Exception):
    pass


def section_range(lines, prefix, label, name):
    """以 prefix 開頭的 `## ` 標題之後到下一個 `## ` 之前的行號範圍（0 起算）"""
    start = next((n for n, l in enumerate(lines) if l.startswith(prefix)), None)
    if start is None:
        raise DesignError(f"{name}: 找不到「{label}」章節")
    end = next((n for n in range(start + 1, len(lines)) if lines[n].startswith("## ")), len(lines))
    return start + 1, end


def table_rows(lines, rng, id_re, min_cells, name):
    """章節裡第一欄符合 id_re 的表格列：{ID: (行號, [儲存格…])}；欄位不夠 → 錯誤（含行號）"""
    rows = {}
    for n in range(*rng):
        if not lines[n].startswith("|"):
            continue
        cells = [c.strip() for c in lines[n].strip().strip("|").split("|")]
        if not re.fullmatch(id_re, cells[0]):
            continue
        if len(cells) < min_cells:
            raise DesignError(f"{name}:{n + 1}: 表格列 {cells[0]} 只有 {len(cells)} 欄，需要至少 {min_cells} 欄")
        rows[cells[0]] = (n + 1, cells)
    return rows


def story_text(lines, rng, heading, name):
    """章節裡以 heading 開頭的標題之後，到下一個 `#` 標題或 `---` 之前的非空行；去掉 `> `；用 \\n 接"""
    start = next((n for n in range(*rng) if lines[n].startswith(heading)), None)
    if start is None:
        raise DesignError(f"{name}: §7 找不到劇情段落「{heading}」")
    body = []
    for n in range(start + 1, rng[1]):
        l = lines[n]
        if l.startswith("#") or l.strip() == "---":
            break
        if l.startswith(">"):
            l = l[1:]
        l = l.strip()
        if l:
            body.append(l)
    if not body:
        raise DesignError(f"{name}:{start + 1}: 劇情段落「{heading}」是空的")
    return "\n".join(body)


def parse(path):
    path = Path(path)
    name = path.name
    if not path.is_file():
        raise DesignError(f"找不到檔案：{path}")
    lines = path.read_text(encoding="utf-8").split("\n")
    r7 = section_range(lines, "## 7.", "## 7. 結局", name)
    r8 = section_range(lines, "## 8.", "## 8. 成就", name)

    sec7 = table_rows(lines, r7, r"E0\d", 4, name)
    sec8 = table_rows(lines, r8, r"[EA]\d\d", 3, name)

    endings = []
    for e in ENDING_IDS:
        if e not in sec7:
            raise DesignError(f"{name}: §7 結局表找不到 {e}")
        if e not in sec8:
            raise DesignError(f"{name}: §8 成就表找不到 {e}")
        cells = sec7[e][1]
        endings.append([e, sec8[e][1][1], cells[2], cells[3], story_text(lines, r7, STORY_HEADINGS[e], name)])

    achievements = []
    for a in ACHIEVEMENT_IDS:
        if a not in sec8:
            raise DesignError(f"{name}: §8 成就表找不到 {a}")
        cells = sec8[a][1]
        achievements.append([a, cells[1], cells[2]])
    return endings, achievements


def render(header, rows):
    buf = io.StringIO(newline="")
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(header)
    w.writerows(rows)
    return buf.getvalue()


def write_atomic(directory, name, content):
    fd, tmp = tempfile.mkstemp(prefix=name + ".", suffix=".tmp", dir=directory)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
            f.write(content)
        os.chmod(tmp, 0o644)
        return tmp
    except BaseException:
        os.unlink(tmp)
        raise


def main(argv=None):
    ap = argparse.ArgumentParser(description="設計文件 §7、§8 → DT_Endings.csv、DT_Achievements.csv")
    ap.add_argument("--script", type=Path, default=ROOT / "docs/design/escape_room_script.md")
    ap.add_argument("--out", type=Path, default=ROOT / "Content/Data")
    args = ap.parse_args(argv)
    try:
        endings, achievements = parse(args.script)
    except DesignError as e:
        print(f"design_to_endings 錯誤：{e}", file=sys.stderr)
        return 1
    contents = {
        ENDINGS_NAME: render(["Name", "Title", "Kind", "Scope", "Story"], endings),
        ACHIEVEMENTS_NAME: render(["Name", "Title", "Condition"], achievements),
    }
    args.out.mkdir(parents=True, exist_ok=True)
    tmps = {}
    try:
        for name, content in contents.items():
            tmps[name] = write_atomic(args.out, name, content)
        for name, tmp in tmps.items():
            os.replace(tmp, args.out / name)
    finally:
        for tmp in tmps.values():
            if os.path.exists(tmp):
                os.unlink(tmp)
    print(f"{ENDINGS_NAME}: {len(endings)} 列；{ACHIEVEMENTS_NAME}: {len(achievements)} 列")
    return 0


if __name__ == "__main__":
    sys.exit(main())
