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
LOG_DIR = ROOT / "Saved" / "Automation"
REPORT_DIR = LOG_DIR / "Report"
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
        "-nullrhi", "-notraceserver", "-unattended", "-nopause", "-nosplash", "-nosound",
        f"-ReportExportPath={REPORT_DIR}",
    ]


def run_to_log(cmd, log_name):
    """輸出寫到 log 檔，不用 PIPE：UnrealTraceServer 之類的子行程會繼承管道，導致 PIPE 永遠等不到 EOF。"""
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    log = LOG_DIR / log_name
    with open(log, "wb") as f:
        code = subprocess.run(cmd, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=f, stderr=subprocess.STDOUT).returncode
    return code, log


def log_tail(log, n=4000):
    return log.read_text(encoding="utf-8", errors="replace")[-n:]


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
    code, log = run_to_log(build_command(), "build.log")
    if code != 0:
        print(log_tail(log))
        print(f"編譯失敗（結束碼 {code}），完整 log：{log}")
        return 3

    if REPORT_DIR.exists():
        shutil.rmtree(REPORT_DIR)
    print("執行 UE 自動化測試（headless、-nullrhi）...", flush=True)
    code, log = run_to_log(test_command(), "ue_tests.log")

    results = read_results()
    if results is None:
        print(log_tail(log))
        print(f"找不到測試報告 {REPORT_DIR / 'index.json'}（UnrealEditor-Cmd 結束碼 {code}），完整 log：{log}")
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
