# PLANS v1 — 逃離學院

> 第一版計畫，**不改**，留著和審查後的 `PLANS.md` 對照。
> AI 依 `BRIEF.md`、`VERIFY.md` 起草（2026-10-30）。

## 步驟

- [ ] **0a Python 測試環境**：一行指令跑全部 Python 測試 → STEP 0
- [ ] **0b 工具可行性**：安裝 UE 5.8；Epic MCP、Flopperam plugin、PixelStreaming2 在 Linux 上驗證 → STEP 0b
- [ ] **0c UE 專案與 UE 測試環境**：repo 根目錄建 C++ ThirdPerson 專案 `EscapeGame.uproject`，UE 自動化測試 headless 跑起來 → （沒寫）
- [ ] **1 設計文件轉 CSV**：`tools/design_to_csv.py` → STEP 1
- [ ] **2 互動判定**：InteractionResolver 優先序 → STEP 2
- [ ] **3 生命值** → STEP 3
- [ ] **4 時間** → STEP 4
- [ ] **5 大門密碼** → STEP 5
- [ ] **6 變身** → STEP 6
- [ ] **7 場景狀態** → STEP 7
- [ ] **8 成就** → STEP 8
- [ ] **9 灰盒關卡**：MCP 搭 7 個區域 + L01、L02 → STEP 9
- [ ] **10 UMG**：彈出文字框、HP、倒數、四位數鍵盤、結局畫面、Noto Sans TC → STEP 10
- [ ] **11 第三人稱各形態操控** → STEP 11
- [ ] **12 單人全路線** → STEP 12
- [ ] **13 Listen Server、伺服器權威、RPC 驗證、互動射線忽略其他玩家** → STEP 13
- [ ] **14 多人規則、監視器看全圖** → STEP 14
- [ ] **15 美術、音效、手感** → STEP 15
- [ ] **16 打包 Linux Shipping、串流畫質設定** → STEP 16
- [ ] **17 Wilbur、配對頁、實例回收** → STEP 17
- [ ] **18 4 人容量實測** → STEP 18

## 檔案怎麼放

```
EscapeGame.uproject
Source/EscapeGame/
  Logic/       InteractionResolver, TeamState, AchievementTracker（只依賴 Core）
  Core/        EscapeGameMode, EscapeGameState, EscapePlayerState
  Forms/       HumanCharacter, FormPawn + 各形態
  UI/          HUD、彈出文字框、密碼鍵盤、結局畫面
  Tests/       自動化測試
Content/Data/  DT_*.csv
tools/         design_to_csv.py, run_tests.sh
tests/         Python 測試
```

## 做法

### 硬體（2026-10-30 實測）

| 項目 | 規格 | 對策 |
|---|---|---|
| CPU | Ryzen 7 7700 8C/16T | 限制 UBT 平行數 |
| RAM | 30 GiB + 16 GiB swap，平常已用 18 GiB | 開 UE 時關大型行程；`MaxParallelActions=10`；不從原始碼編譯引擎 |
| GPU | RTX 5060 Ti 16 GB，驅動 580 | Lumen、NVENC 可用 |
| 顯示輸出 | HDMI 接在 AMD 內顯 | 改插 5060 Ti |
| 硬碟 | `/home` 剩 246 GB | 引擎裝在 `~/UnrealEngine/5.8` |
| OS | EndeavourOS、Hyprland／Wayland | UnrealEditor 設 windowrule float；必要時走 XWayland |
| Windows | 沒有 | 只打包 Linux；沒有 Live Coding，改完 C++ 關編輯器用 `Build.sh` 編 |
| Python | 3.14 | Flopperam 要 `<3.14`：`uv python install 3.12` |

Pixel Streaming 估計本機撐 3～4 位玩家（每實例約 2～3 GB VRAM、2～3 GB RAM），瓶頸是 RAM。

### 工具
- **Epic UE Skills plugin + UE 5.8 內建 MCP（主力）**：擺放、BP 子類別、UMG、PIE、測試。
- **Flopperam MCP（選用）**：只在步驟 9 搭灰盒，用完停用；Linux 編不過就放棄。
- **gamedev skills**：router + unreal，加 level-design、game-ui-ux、game-feel、camera、puzzle。
- 邏輯寫 C++ 和 CSV；Blueprint 保持薄；角色操控以 ThirdPerson 範本為骨架；MCP 前先 commit。

### 架構
- Logic 層只依賴 Core，`-nullrhi` 可測。
- GameState／PlayerState 伺服器權威、可複製，只透過 Server RPC 推進。
- 密碼只在伺服器；`FormPawn.OccupiedBy` 伺服器鎖定；互動射線忽略其他玩家的 Pawn。
- Pixel Streaming：PixelStreaming2 + Wilbur；`-RenderOffscreen -ResX=1280 -ResY=720`；Listen Server；公網要 coturn，先 LAN。
