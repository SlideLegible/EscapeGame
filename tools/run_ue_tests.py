#!/usr/bin/env python3
"""編譯 EscapeGameEditor，再用 headless（-nullrhi）跑 UE 自動化測試，列出每條 EscapeGame. 測試的結果。

結束碼 0：全部通過，而且至少有 1 條 EscapeGame. 開頭的測試。其他情況非 0。
引擎路徑預設 ~/UnrealEngine，可用環境變數 UE_ROOT 覆蓋。只用標準函式庫。
"""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UE_ROOT = Path(os.environ.get("UE_ROOT", str(Path.home() / "UnrealEngine")))
UPROJECT = ROOT / "EscapeGame.uproject"
REPORT_DIR = ROOT / "Saved" / "Automation" / "Report"
TEST_FILTER = "EscapeGame"
MAX_PARALLEL_ACTIONS = 10  # 這台機器 RAM 只有 30 GiB


def build_command():
    build_sh = UE_ROOT / "Engine" / "Build" / "BatchFiles" / "Linux" / "Build.sh"
    return [
        str(build_sh), "EscapeGameEditor", "Linux", "Development",
        f"-project={UPROJECT}", f"-MaxParallelActions={MAX_PARALLEL_ACTIONS}", "-NoHotReload",
    ]


def test_command():
    editor_cmd = UE_ROOT / "Engine" / "Binaries" / "Linux" / "UnrealEditor-Cmd"
    # -nullrhi：不建立 GPU／視窗，headless 跑測試。
    return [
        str(editor_cmd), str(UPROJECT),
        f"-ExecCmds=Automation RunTests {TEST_FILTER};Quit",
        "-nullrhi", "-unattended", "-nopause", "-nosplash", "-nosound",
        f"-ReportExportPath={REPORT_DIR}",
    ]


def read_results():
    index = REPORT_DIR / "index.json"
    if not index.is_file():
        return None
    # index.json 可能帶 BOM
    data = json.loads(index.read_text(encoding="utf-8-sig"))
    return [(t.get("fullTestPath") or t.get("testDisplayName", "?"), t.get("state", "?"))
            for t in data.get("tests", [])]


def main():
    editor_cmd = UE_ROOT / "Engine" / "Binaries" / "Linux" / "UnrealEditor-Cmd"
    if not editor_cmd.is_file():
        print(f"錯誤：找不到 {editor_cmd}。UE 5.8 沒有安裝在 {UE_ROOT}（可用環境變數 UE_ROOT 指定）。")
        return 2
    if not UPROJECT.is_file():
        print(f"錯誤：找不到 {UPROJECT}")
        return 2

    print("編譯 EscapeGameEditor ...", flush=True)
    build = subprocess.run(build_command(), cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if build.returncode != 0:
        print(build.stdout[-4000:])
        print(f"編譯失敗（結束碼 {build.returncode}）")
        return 3

    if REPORT_DIR.exists():
        shutil.rmtree(REPORT_DIR)
    print("執行 UE 自動化測試（headless、-nullrhi）...", flush=True)
    run = subprocess.run(test_command(), cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    results = read_results()
    if results is None:
        print(run.stdout[-4000:])
        print(f"找不到測試報告 {REPORT_DIR / 'index.json'}（UnrealEditor-Cmd 結束碼 {run.returncode}）")
        return 4

    mine = [(p, s) for p, s in results if p.startswith("EscapeGame.")]
    for path, state in mine:
        print(f"  [{'通過' if state == 'Success' else '失敗：' + state}] {path}")
    passed = sum(1 for _, s in mine if s == "Success")
    if not mine:
        print("UE 自動化測試：沒有找到任何 EscapeGame. 開頭的測試")
        return 5
    if passed == len(mine):
        print(f"UE 自動化測試：{passed}/{len(mine)} 全部通過")
        return 0
    print(f"UE 自動化測試：{passed}/{len(mine)} 通過，有失敗")
    return 1


if __name__ == "__main__":
    sys.exit(main())
