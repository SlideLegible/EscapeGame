# AGENTS.md — 給 AI 的長期規則

> ⚠️ AI 草稿，需人確認。這份是人的文件。

## 一定要遵守
1. VERIFY.md 不要改，那是我的。
2. PLANS.md 你可以自己調整，回報時說改了什麼，但不能做 BRIEF 沒要的。
3. 一次只做一步，做完停下來回報。
4. 每一步用 TDD：先把這一步的測試寫好、跑一次確認失敗、commit，再寫程式；
   全部測試通過才 commit，並在 PLANS.md 勾掉這一步。
5. 不要為了通過而修改測試。覺得測試錯了，先問我。
6. 不要修改 `docs/design/` 底下的設計文件；發現問題寫進 PLANS.md 的「待人確認」。
7. 每次透過 MCP 操作 Unreal 編輯器之前，先 commit；一次 MCP 任務只做一件小而具體的事。
8. 不使用 `--dangerously-skip-permissions`。
9. MCP 的連接埠只能聽 `127.0.0.1`，不可以對外開放；部署時也不可以開放。
10. 不讀取、不複製別人的範例原始碼；Unreal 官方範本（ThirdPerson）可以當骨架。

## 這個專案
- 開發流程的細節寫在 `dev_rule.md`。
- 跑全部測試：`tools/run_tests.sh`
  - Python 測試：`tests/test_stepN_名稱.py`，只用內建 `unittest`，用 `python3 -I` 執行。
  - UE 自動化測試（步驟 0c 之後）：`Source/EscapeGame/Tests/StepN_名稱.spec.cpp`，headless（`-nullrhi`）。
  - 測試檔第一行註解寫對到哪裡，例：`# 步驟 1 設計文件轉 CSV → VERIFY STEP 1`；測試名稱用 VERIFY 的句子。
- 本機開起來：（步驟 0c 之後補上 UE 編輯器啟動指令。）
- 哪些檔案可以碰什麼：
  - `Source/EscapeGame/Logic/`：純邏輯，只能依賴 UE 的 `Core`，不可以 include Actor、Engine 畫面、UMG 的標頭。亂數用注入種子的 `FRandomStream`，時間用傳入的 dt。
  - `Source/EscapeGame/Core/`：組合層，可以用 GameFramework（GameMode／GameState／PlayerState）與網路複製。
  - `Source/EscapeGame/Forms/`、`Source/EscapeGame/UI/`：只有這兩個資料夾可以碰畫面、輸入、UMG。
  - `tools/`：Python 只用標準函式庫。
- 平台：Linux（EndeavourOS、Hyprland）。沒有 Live Coding：改完 C++ 要關掉編輯器，用 `Build.sh` 編譯再開。
- 機器 RAM 只有 32 GB：不要同時開多個編輯器實例；編譯平行數照 PLANS.md。
- 限制寫在 BRIEF.md 最後。
- commit 說明範例：「步驟 1 設計文件轉 CSV：測試（紅，還沒有 design_to_csv.py）」「步驟 1 設計文件轉 CSV：…，測試全部通過（n/n）」。
