#!/usr/bin/env python3
"""設計文件 §5「預設回應」→ Content/Data/DT_Responses.csv（步驟 2）。只用標準函式庫。

用法：python3 -I tools/design_to_responses.py [--interactions MD] [--out DIR]
輸出欄位：Name,Text。目前只有一列 Default；文字裡的 `[對象]` 原樣保留成占位符。
找不到 §5 或找不到引用區塊 → 結束碼 1，stderr 寫檔名和原因；不留下半成品。
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
OUT_NAME = "DT_Responses.csv"
SECTION_RE = re.compile(r"^##\s*5[.\s、]")


class DesignError(Exception):
    pass


def extract_default(path):
    lines = Path(path).read_text(encoding="utf-8").split("\n")
    start = next((n for n, l in enumerate(lines) if SECTION_RE.match(l)), None)
    if start is None:
        raise DesignError(f"{path.name}: 找不到「## 5.」預設回應章節")
    quote = []
    for n in range(start + 1, len(lines)):
        if lines[n].startswith("## "):
            break
        if lines[n].startswith(">"):
            quote.append(lines[n][1:].strip())
        elif quote:
            break
    text = " ".join(q for q in quote if q)
    if not text:
        raise DesignError(f"{path.name}:{start + 1}: §5 裡找不到引用區塊（> 開頭的預設回應文字）")
    if "[對象]" not in text:
        raise DesignError(f"{path.name}:{start + 1}: 預設回應文字沒有 [對象] 占位符")
    return text


def render(text):
    buf = io.StringIO(newline="")
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(["Name", "Text"])
    w.writerow(["Default", text])
    return buf.getvalue()


def main(argv=None):
    ap = argparse.ArgumentParser(description="設計文件 §5 → DT_Responses.csv")
    ap.add_argument("--interactions", type=Path, default=ROOT / "docs/design/item_interactions.md")
    ap.add_argument("--out", type=Path, default=ROOT / "Content/Data")
    args = ap.parse_args(argv)
    try:
        if not args.interactions.is_file():
            raise DesignError(f"找不到檔案：{args.interactions}")
        content = render(extract_default(args.interactions))
    except DesignError as e:
        print(f"design_to_responses 錯誤：{e}", file=sys.stderr)
        return 1
    args.out.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=OUT_NAME + ".", suffix=".tmp", dir=args.out)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
            f.write(content)
        os.replace(tmp, args.out / OUT_NAME)
        os.chmod(args.out / OUT_NAME, 0o644)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)
    print(f"{OUT_NAME}: 1 列")
    return 0


if __name__ == "__main__":
    sys.exit(main())
