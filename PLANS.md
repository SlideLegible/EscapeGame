# PLANS — 逃離學院

> AI 起草，人審方向。第一版留在 `PLANS.v1.md` 對照。
> 每一步：先寫測試 → 跑一次確認失敗 → commit → 寫程式 → 全部通過 → commit → 在這裡勾掉。

## 步驟

### 第 0 批 環境
- [x] **0a Python 測試環境**：`tools/run_tests.sh` 一行跑全部 Python 測試；確認設計文件存在 → STEP 0（第 1、2、4、5 條）
- [ ] **0b 工具可行性（人工）**：使用者安裝 UE 5.8；Epic MCP 連線並列出 Actor；Flopperam plugin 在 Linux 編譯；PixelStreaming2 範本在 Firefox 可玩；實測單一實例資源；確認 MCP 只聽 localhost → STEP 0b
  - **Go/No-Go**：Epic MCP 在 Linux 不能用 → 改用 Flopperam 為主，加 `UnrealEditor-Cmd -run=pythonscript` 跑官方 Python 編輯器腳本當後備，寫進調整紀錄
  - **2026-10-30 結果：Go**（細節見調整紀錄）。AI 能驗的都驗完了；**剩人工：在 Firefox 用真的鍵盤滑鼠操控角色**，確認後人在 VERIFY 打勾、在這裡勾掉。
- [ ] **0c UE 專案與 UE 測試環境**：repo 根目錄建 C++ ThirdPerson 專案 `EscapeGame.uproject`；`tools/run_tests.sh` 加上 headless UE 自動化測試 → STEP 0（第 3、4 條）

### 第 1 批 資料管線
- [ ] **1 設計文件轉 CSV**：`tools/design_to_csv.py` 依 ID 前綴分類，輸出 `Content/Data/DT_*.csv`；格式錯誤時指出 ID 與行號 → STEP 1

### 第 2 批 核心邏輯（`Source/EscapeGame/Logic/`，UE 自動化測試，固定種子、固定 dt）
- [ ] **2 互動判定**：Resolver 優先序（X033 鑰匙圈碰門 → 精確 → 其他／非人類形態 → 任一形態 → 預設嘲諷）；CSV 每列 X 一個 test case → STEP 2
- [ ] **3 生命值**：初始 5、無上限、不為負、歸零 → E05 重啟但保留成就 → STEP 3
- [ ] **4 時間**：30:00 倒數、時鐘自身互動 → E04 + A06；已逃出者不受影響 → STEP 4
- [ ] **5 大門密碼**：可注入種子的亂數、排除 1031、輸錯 1/1000 扣血、試過 1031 的旗標 → STEP 5
- [ ] **6 變身**：身體留原地、變回人類、物品互換、物品唯一、L01／L02 進入規則、監視器形態 → STEP 6
- [ ] **7 場景狀態**：電閘不可逆、教室門、辦公室門、儲物間門 → STEP 7
- [ ] **8 成就**：A01／A02／A03 計數、A04／A06／A07、E01～E05；不重複、不寫磁碟 → STEP 8

### 第 3 批 場景與單人通關
- [ ] **9 灰盒關卡**：MCP 搭 8 個區域，擺放所有 F、S 物件；自動化測試掃描關卡 → STEP 9
- [ ] **10 介面**：HUD（生命值、mm:ss、形態）、彈出文字框、四位數鍵盤、結局畫面、成就提示、Noto Sans TC → STEP 10
- [ ] **11 操控**：Enhanced Input（WASD、滑鼠、E、Q）；各形態移動模式（Hop／Roll／Static）與鏡頭臂長度 → STEP 11
- [ ] **12 單人全路線**：Functional Test 跑 E01、E03 路線；開發版測試模式讀密碼 → STEP 12

### 第 4 批 多人
- [ ] **13 連線與伺服器權威**：Listen Server、上限 3 人、Server RPC 驗證（ID 存在、距離）、密碼不複製 → STEP 13
- [ ] **14 多人規則**：共用血量、各自逃出、逃生方法可重複、E04／E05 全隊、監視器看到其他玩家、玩家間不能互動 → STEP 14

### 第 5 批 美術與手感
- [ ] **15 美術、音效、效能**：Fab 免費／Megascans 替換灰盒（來源記在下方「素材來源」）、音效、fps 實測 → STEP 15

### 第 6 批 網頁遊玩
- [ ] **16 打包**：Linux Shipping；Shipping 排除測試模式與 MCP plugin；串流用畫質設定 → STEP 16
- [ ] **17 串流與實例管理**：PixelStreaming2 + Wilbur、配對頁、斷線／重新整理回收重置、訊息 64 KB 上限、只開放必要連接埠 → STEP 17
- [ ] **18 容量**：3 人同房實測，資源數字寫進調整紀錄 → STEP 18

## 檔案怎麼放

```
BRIEF.md VERIFY.md PLANS.md PLANS.v1.md AGENTS.md CLAUDE.md dev_rule.md README.md
docs/design/                 設計文件（素材，不修改）
tools/
  run_tests.sh               一行跑全部測試（Python；0c 之後加 UE）
  design_to_csv.py           設計文件 → Content/Data/DT_*.csv（步驟 1）
tests/                       Python 測試，test_stepN_名稱.py
EscapeGame.uproject          （0c）
Source/EscapeGame/
  Logic/                     純邏輯，只依賴 Core；不可 include Engine 的 Actor／畫面標頭
    InteractionResolver.*    （形態, 對象, 旗標）→ 互動結果
    TeamState.*              生命值、旗標、倒數、密碼、結局判定
    AchievementTracker.*     成就計數與解鎖
  Core/                      組合層：GameMode、GameState、PlayerState（複製、伺服器權威）
  Forms/                     畫面層：HumanCharacter、FormPawn 與各形態
  UI/                        畫面層：HUD、文字框、密碼鍵盤、結局、成就提示
  Tests/                     UE 自動化測試，StepN_名稱.spec.cpp
Content/Data/                DT_*.csv 與匯入後的 DataTable
Content/Maps/                主關卡
deploy/                      Pixel Streaming：Wilbur 設定、配對頁、實例管理腳本（第 6 批）
```

## 做法

### 硬體（2026-10-30 實測）

| 項目 | 規格 | 對策 |
|---|---|---|
| CPU | Ryzen 7 7700 8C/16T | `BuildConfiguration.xml` 限制平行數 |
| RAM | 30 GiB + 16 GiB swap，平常已用 18 GiB（一個 python 行程約 8 GB） | **最大瓶頸**。開 UE 時關大型行程；`MaxParallelActions=10`；不從原始碼編譯引擎 → v1 不做 Dedicated Server |
| GPU | RTX 5060 Ti 16 GB，驅動 580 | Lumen、NVENC 可用 |
| 顯示輸出 | ~~HDMI 接在 AMD 610M 內顯~~ 已改插 5060 Ti（2026-10-30 確認） | — |
| 硬碟 | NVMe，`/home` 剩 246 GB | 引擎實際裝在 `~/UnrealEngine`（5.8.3，73 GB），DDC 也在 `/home` |
| OS | EndeavourOS、Hyprland／Wayland | UnrealEditor 設 windowrule float；必要時走 XWayland |
| Windows | 沒有 | 只打包 Linux；沒有 Live Coding：改完 C++ 關編輯器，用 `Engine/Build/BatchFiles/Linux/Build.sh EscapeGameEditor Linux Development -project=...` 編，再開 |
| Python | 3.14 | ~~Flopperam 要 `<3.14`~~ Flopperam 已放棄，不需要 |

**Pixel Streaming 容量**：0b 實測單一實例（未打包、用 `UnrealEditor -game` 跑 ThirdPerson 範本、720p、畫質預設、FPS 不限）約 **3.6 GB RAM、3.9 GB VRAM、1.6 核心、GPU 89%、NVENC 8%**，比原本估的高。
照這個數字本機只撐 **2～3 位玩家**。**人數上限：3 人**（2026-10-31 人決定）。步驟 16 仍要打包成 Shipping（比 editor 執行檔省 RAM）、`t.MaxFPS 30`、串流專用低畫質，讓 3 人穩定在 30 fps 以上。

### 工具分工
- **Epic UE Skills plugin + UE 5.8 內建 MCP（唯一的 MCP）**：擺放 Actor、BP 子類別、UMG、DataTable、PIE、跑測試。0b 已確認在 Linux 可用。灰盒房間也用它（`SceneTools.add_to_scene_from_asset` 放基本形狀）。
- ~~Flopperam MCP（選用）~~：**0b 放棄**，在 UE 5.8.3 編譯失敗（見調整紀錄）。
- **gamedev skills**：只裝 router + unreal，加 level-design、game-ui-ux、game-feel、camera、puzzle。
- 原則：
  - 邏輯寫在 C++ 和 CSV，Claude Code 直接改文字檔（可 diff、可測、省 token）；MCP 只做編輯器端的資產操作。
  - Blueprint 保持薄，只做 C++ 類別的子類別與參數。
  - 角色操控以官方 ThirdPerson C++ 範本為骨架，不從零生成。
  - 每次 MCP 工作前先 commit。

### 架構（dev_rule §6 邏輯和畫面分開）
- `Logic/` 是純邏輯，對外只給 `Interact(玩家, 對象)`、`Tick(dt)`、查詢狀態；亂數用可注入種子的 `FRandomStream`，時間用傳入的 dt → 測試結果固定。
- `Core/` 把 Logic 包進可複製的 GameState／PlayerState，只透過 Server RPC 推進。
- 只有 `Forms/`、`UI/` 可以 include 畫面相關標頭。
- 資料唯一來源是設計文件 → `design_to_csv.py` → CSV → DataTable。
- 多人：密碼只在伺服器、不複製；`FormPawn.OccupiedBy` 伺服器鎖定；互動射線忽略其他玩家的 Pawn；伺服器驗證每個 RPC（ID 存在、距離內）。
- Pixel Streaming：PixelStreaming2 + Wilbur；實例 `-RenderOffscreen -ResX=1280 -ResY=720 -PixelStreamingConnectionURL=ws://127.0.0.1:8888`；多人用 Listen Server（開房者的實例當 host）；公網要 coturn，先在 LAN 展示。
  - Wilbur 用 `get_ps_servers.sh` 抓（目前是 `UE5.7-0.1.2`），**不用它的 `start.sh`**（會呼叫 sudo、apt）；改成 `npm install --ignore-scripts -w …` 只裝需要的 workspace，再 `npm run build:all:cjs`。
  - Wilbur 沒有 bind address 選項，會聽所有介面 → 步驟 17 用防火牆只開放配對頁和 player port。
  - UE 的 Trace 控制埠 1985 也聽 `0.0.0.0` → 步驟 17 一併擋掉，Shipping 版確認有沒有開。

### 素材來源
（步驟 15 開始填：素材名稱、來源網址、授權。）

## 審查紀錄（對照 PLANS.v1.md，問三件事）

1. **VERIFY 每一條都排進去了嗎？**
   - v1 的 0a 寫「→ STEP 0」，但 STEP 0 第 3 條（UE 自動化測試）在 0a 做不到 → 0a 對到第 1、2、4、5 條，第 3 條移到 0c。
   - STEP 10「成就提示」v1 步驟 10 沒列 → 補上。
   - STEP 13「上限 4 人、第 5 位被拒絕」與 STEP 18「不可以擠掉前 4 人」v1 沒排 → 步驟 13 補上人數上限。
   - STEP 16「Shipping 排除測試模式與 MCP plugin」v1 沒排 → 補進步驟 16。
   - STEP 17「64 KB 訊息上限」「只開放必要連接埠」v1 沒排 → 補進步驟 17。
   - STEP 0b「MCP 只聽 localhost」v1 沒排 → 補進 0b。
   - STEP 6 有監視器形態（X023），v1 把監視器放在步驟 14 → 形態本身在步驟 6，看到其他玩家留在 14。
2. **每一步都對到測試嗎？**
   - v1 的 0c 寫「（沒寫）」→ 對到 STEP 0 第 3、4 條。
   - 0b 是人工驗證，沒有自動測試 → 結果寫進調整紀錄，人確認後打勾。
3. **有沒有偷加 BRIEF 沒要的？**
   - Flopperam、gamedev skills 是工具不是功能，保留但標為選用。
   - 「測試模式讀密碼」只存在開發版，VERIFY STEP 12、16 都有寫，保留。
   - 「配對頁」是 BRIEF 第 13 條網頁遊玩的必要部分，保留。
   - 沒有發現超出 BRIEF 的功能。

### 待人確認的設計問題（不改設計文件）
- 形態表（`item_interactions.md` §1）裡混進了 X027、X032、X035 三列 → `design_to_csv.py` 依 ID 前綴歸類即可，不需改文件。
- ~~成就沒有 A05~~ → **刻意跳過**（2026-10-31 人確認）。
- ~~互動沒有 X019~~ → **刻意跳過**（2026-10-31 人確認）。
- ~~人數上限~~ → **3 人**（2026-10-31 人決定），已寫進 BRIEF。⚠️ VERIFY STEP 18 還寫 4 人（「4 個瀏覽器」「第 5 個瀏覽器」），VERIFY 是人的文件，待人改成 3。

## 調整紀錄

（邊做邊調整時寫在這裡，說明改了什麼、為什麼。）

- **2026-10-30 步驟 0a**：`tools/run_tests.sh` 用 `python3 -I -m unittest discover -s tests -t .`；`tests/` 也做成套件（`tests/__init__.py`），`-t .` 才能 import `tools`。
  測試裡會巢狀執行 `run_tests.sh`，用環境變數 `ESCAPEGAME_NESTED_TEST_RUN` 讓巢狀那次跳過自己，避免無限遞迴。
  「不可以需要網路／螢幕／GPU」目前只有標準函式庫檢查守著（沒有網路呼叫），0c 加 UE 測試時用 `-nullrhi` 守住。

- **2026-10-30 步驟 0b 工具可行性（Go）**：UE 5.8.3（CL 58210709）裝在 `~/UnrealEngine`；HDMI 已改插 5060 Ti。用 scratchpad 的拋棄式專案驗證，沒有動 repo。
  - **Epic MCP ✅**：`ModelContextProtocol`、`AllToolsets` 都有 Linux 預編譯檔。啟動參數 `-ExecCmds="ModelContextProtocol.StartServer"` → `127.0.0.1:8000/mcp`（Streamable HTTP），`ss` 確認**只聽 127.0.0.1**（HTTPServer 預設 bind `localhost`）。
    用 JSON-RPC 直接呼叫：`list_toolsets` 列出 57 個 toolset；`SceneTools.find_actors` 讀到關卡 Actor；`add_to_scene_from_asset` 放一個 Cube、`find_actors` 找得到、`remove_from_scene` 刪掉後找不到 → **讀寫都可以**。
    呼叫格式：`call_tool` 帶 `toolset_name`、`tool_name`、`arguments`；`find_actors` 要給 `name`、`tag`、`collision_channels`（可空）。編輯器空專案約 3 GB RAM。
  - **Flopperam ❌ 放棄**：`BuildPlugin` 什麼都沒編（Editor 模組被略過，`.uplugin` 還用舊的 `WhitelistPlatforms`）；放進 C++ 宿主專案編 `Editor` target → 2 個錯誤（`BPVariables.cpp` 用 `FindRef("…")`，5.8 的 JSON map key 改成 `TSharedString`），另有 61 個「下一版就會編不過」的 deprecation 警告。照計畫放棄，不修第三方程式碼。Epic MCP 能放基本形狀，灰盒改用它。
  - **Pixel Streaming ✅（操控待人確認）**：`PixelStreaming2` 支援 Linux。ThirdPerson 範本（含 LevelPrototyping、Characters、Input 共用素材）用 `UnrealEditor -game -RenderOffscreen` 跑，首次啟動編 shader 約 2 分鐘後連上 Wilbur；Chrome 開 `http://localhost:8080` 按一下就看到畫面，log 顯示 video、audio、data channel 都是 Active。
    自動化工具只能送「瞬間按放」的按鍵，角色看不出移動 → **鍵盤滑鼠操控要人在 Firefox 實際試**。
  - **資源（單一實例，有一位觀看者）**：RSS 3.6 GB、VRAM 3.9 GB、CPU 160%、GPU 89%（FPS 未限制）、NVENC 8%。沒有觀看者時 RSS 3.5 GB、VRAM 3.4 GB。→ 已更新「Pixel Streaming 容量」。
  - **連接埠**：MCP 8000 只聽 127.0.0.1 ✅；另外發現 UE Trace 控制埠 1985、Wilbur 的 8080／8888 聽所有介面 → 寫進「做法」，步驟 17 處理。
  - **還沒做、要人做的**：Claude Code 裝 Epic 的 skills plugin 與 gamedev skills（plugin 設定是使用者的，見回報）；`.mcp.json` 在 0c 建好真正的專案後用 `ModelContextProtocol.GenerateClientConfig ClaudeCode` 產生。

- **2026-10-31 人確認**：BRIEF、VERIFY 確認；A05、X019 刻意跳過；人數上限 3 人；授權 MIT（加 `LICENSE`）。
  步驟 13 改「上限 3 人」、步驟 18 改「3 人同房」、「Pixel Streaming 容量」改寫成 3 人。Claude Code 的 Epic skills plugin 與 gamedev skills 人已安裝。
