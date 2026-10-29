#!/usr/bin/env bash
# 一行跑全部測試：tools/run_tests.sh
# 目前只有 Python 測試；步驟 0c 之後在這裡加上 headless UE 自動化測試。
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 -I -m unittest discover -s tests -t . "$@"
