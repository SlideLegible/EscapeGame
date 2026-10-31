#!/usr/bin/env bash
# 一行跑全部測試：tools/run_tests.sh
# UE 測試由 tests/test_step0c_ue_project.py 呼叫；-u 不緩衝，讓最後一行是 unittest 的 OK。
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 -I -u -m unittest discover -s tests -t . "$@"
