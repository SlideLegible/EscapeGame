# PLANS — 逃離學院

> AI 起草，人審方向。第一版留在 `PLANS.v1.md` 對照。
> 每一步：先寫測試 → 跑一次確認失敗 → commit → 寫程式 → 全部通過 → commit → 在這裡勾掉。

## 步驟

### 第 0 批 環境
- [x] **0a Python 測試環境**：`tools/run_tests.sh` 一行跑全部 Python 測試；確認設計文件存在 → STEP 0（第 1、2、4、5 條）
- [x] **0b 工具可行性（人工）**：使用者安裝 UE 5.8；Epic MCP 連線並列出 Actor；Flopperam plugin 在 Linux 編譯；PixelStreaming2 範本在 Firefox 可玩；實測單一實例資源；確認 MCP 只聽 localhost → STEP 0b
  - **Go/No-Go**：Epic MCP 在 Linux 不能用 → 改用 Flopperam 為主，加 `UnrealEditor-Cmd -run=pythonscript` 跑官方 Python 編輯器腳本當後備，寫進調整紀錄
  - **2026-10-30 結果：Go**（細節見調整紀錄）。AI 能驗的都驗完了；**剩人工：在 Firefox 用真的鍵盤滑鼠操控角色**，確認後人在 VERIFY 打勾、在這裡勾掉。
- [x] **0c UE 專案與 UE 測試環境**：repo 根目錄建 C++ ThirdPerson 專案 `EscapeGame.uproject`；`tools/run_tests.sh` 加上 headless UE 自動化測試 → STEP 0（第 3、4 條）

### 第 1 批 資料管線
- [x] **1 設計文件轉 CSV**：`tools/design_to_csv.py` 依 ID 前綴分類，輸出 `Content/Data/DT_*.csv`；格式錯誤時指出 ID 與行號 → STEP 1

### 第 2 批 核心邏輯（`Source/EscapeGame/Logic/`，UE 自動化測試，固定種子、固定 dt）
- [x] **2 互動判定**：Resolver 優先序（X033 鑰匙圈碰門 → 精確 → 其他／非人類形態 → 任一形態 → 預設嘲諷）；CSV 每列 X 一個 test case → STEP 2
- [x] **3 生命值**：初始 5、無上限、不為負、歸零 → E05 重啟但保留成就 → STEP 3
- [x] **4 時間**：30:00 倒數、時鐘自身互動 → E04 + A06；已逃出者不受影響 → STEP 4
- [x] **5 大門密碼**：可注入種子的亂數、排除 1031、輸錯 1/1000 扣血、試過 1031 的旗標 → STEP 5
- [x] **6 變身**：身體留原地、變回人類、物品互換、物品唯一、L01／L02 進入規則、監視器形態 → STEP 6
- [x] **7 場景狀態**：電閘不可逆、教室門、辦公室門、儲物間門 → STEP 7
- [x] **8 成就**：A01／A02／A03 計數、A04／A06／A07、E01～E05；不重複、不寫磁碟 → STEP 8

### 第 3 批 場景與單人通關
- [x] **9 灰盒關卡**：MCP 搭 8 個區域，擺放所有 F、S 物件；自動化測試掃描關卡 → STEP 9
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
  run_tests.sh               一行跑全部測試（Python；UE 測試由 test_step0c_ue_project.py 呼叫 run_ue_tests.py）
  run_ue_tests.py            編譯 EscapeGameEditor + headless（-nullrhi）跑 UE 自動化測試（0c）
  design_to_csv.py           設計文件 → Content/Data/DT_*.csv（步驟 1）；可用 --interactions／--script／--out 指定輸入輸出
  design_to_responses.py     設計文件 §5 → Content/Data/DT_Responses.csv（步驟 2）；--interactions／--out
  mcp_build_graybox.py       （步驟 9）讀 Level_Layout.csv，經 MCP（127.0.0.1:8000）在開著的編輯器裡重蓋並存檔 /Game/Maps/Lvl_Academy；--print-script 只印腳本
tests/                       Python 測試，test_stepN_名稱.py（test_step5_password_secrecy.py：掃 Source/ 的 log 與 DevOnly 函式；test_step8_no_persistence.py：掃 Source/ 不可有永久儲存 API）
EscapeGame.uproject          （0c）UE 專案；.mcp.json 是 Claude Code 連 MCP 的設定（只指向 127.0.0.1:8000）
Config/ Content/ThirdPerson/ Content/{LevelPrototyping,Characters,Input}/   ThirdPerson 範本與共用素材（Content 約 130 MB，*.uasset／*.umap 走 LFS）
Source/EscapeGame/
  Logic/                     純邏輯，只依賴 Core；不可 include Engine 的 Actor／畫面標頭
    InteractionResolver.*    （形態, 對象, 狀態快照）→ 互動結果（步驟 2，純函式）
    TeamState.*              全隊狀態：生命值、玩家名單、已觸發、成就、E05 重啟（步驟 3）；倒數與 E04（步驟 4）；密碼、旗標、E01～E03、機率扣血（步驟 5）
    EscapeGameLogic.*        組合層（步驟 5 建立）：Resolver + TeamState，`Interact`／`SubmitPassword`／`ReturnToHuman`／`Tick`、`FInteractOutcome`；形態與位置（步驟 6）；場景狀態（步驟 7：`IsDoorOpen`／`GetOpenDoors`／`IsPowerCut`）；成就（步驟 8：`GetAchievements`／`PopNewAchievements`／`FInteractOutcome::NewAchievements`）
    AchievementTracker.*     成就計數與解鎖（步驟 8，每位玩家各自，只在記憶體）
  Core/                      組合層：GameMode、GameState、PlayerState（複製、伺服器權威）
    EscapeAreaVolume.*       （步驟 9）區域盒子：AreaId、Box（Root，只 Overlap）、ContainsPoint
  Forms/                     畫面層：HumanCharacter、FormPawn 與各形態
    EscapeObjectActor.*      （步驟 9）場景物件／可變身物品的灰盒方塊：ObjectId、bIsDoor、SetDoorOpen／IsDoorOpen
    EscapeFormPawn.*         （步驟 9）人類身體／物品的 Pawn 空殼：FormId、Mesh（移動與鏡頭是步驟 11）
  UI/                        畫面層：HUD、文字框、密碼鍵盤、結局、成就提示
  Tests/                     UE 自動化測試，StepN_名稱.spec.cpp（Step2_InteractionResolver.spec.cpp、Step3_TeamState.spec.cpp、Step4_Clock.spec.cpp、Step5_DoorPassword.spec.cpp、Step6_Transform.spec.cpp、Step7_SceneState.spec.cpp、Step8_Achievements.spec.cpp）
Content/Data/                DT_*.csv 與匯入後的 DataTable；Level_Layout.csv（步驟 9，自己的資料：關卡尺寸與物件位置）
Content/Maps/                主關卡 Lvl_Academy.umap（步驟 9，由 tools/mcp_build_graybox.py 產生，LFS）
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
  - 少了 Flopperam 唯一真正缺的是「一鍵蓋建築」（`create_wall`、`construct_house`、`create_staircase` 這類）；Epic 只有 `PrimitiveTools.add_cube` 等 4 種基本形狀。步驟 9 改成：灰盒尺寸寫成 repo 裡的資料檔，用 `ProgrammaticToolset` 一次批次放牆、地板、門洞，可重跑、可 diff。
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
  - UE 的 Trace 控制埠 1985 也聽 `0.0.0.0`：這是編輯器／遊戲行程**自己**開的（`Runtime/TraceLog/Private/Trace/Control.cpp`，Linux 版 `UnixTrace.cpp` 綁 `s_addr = 0`），寫死、沒有參數能改成只聽 127.0.0.1，`-notraceserver` 也擋不掉。用途是讓別台電腦的 Unreal Insights 遙控開始／停止效能追蹤。Shipping 版不支援完整追蹤（`Config.h` 的 static_assert），預設不會編進這個控制埠 → 部署用 Shipping 就沒有它；開發時靠步驟 17 的防火牆規則擋外部連線，步驟 16 打包後用 `ss` 確認。

### 步驟 4～8 的 Logic 架構（2026-10-31 developer session 規劃，給執行的 subagent 照著做）
全部在 `Source/EscapeGame/Logic/`，純 C++、只依賴 Core（`FRandomStream` 在 `Math/RandomStream.h`，`FVector` 在 Core）。
- **`FTeamState`（TeamState.*）全隊、每局的狀態**：生命值、倒數、玩家名單（是否已結束、結局）、已觸發 X、旗標、全隊已解鎖成就（聯集，步驟 3 測試要用）、大門密碼與亂數。
  - 步驟 4：`Tick(float Dt)`、`GetRemainingSeconds()`（整數秒，無條件進位，加極小容差避免 1/60 累加誤差）、`static FString FormatClock(int32 Seconds)` → `"30:00"`。時間到 0 → 一次 E04 事件（受影響＝還沒結束的玩家），之後時鐘停在 0。
  - `ApplyInteraction(const FInteractionResult&, const FString& PlayerId = FString())`：結果帶 E04 → 同時間到；帶 E01～E03 → 只有 PlayerId 結束（步驟 5）。拿到 E04 的玩家標成「已結束」（沿用 `bEscaped`／`GetEscapeEnding` 回傳 E04），所以不會再收到 E04；全部玩家都結束 → `Tick` 不再倒數。
  - 步驟 5：建構子接種子 `FTeamState(int32 Seed)`（保留無參數版本給舊測試）。密碼：`RandRange(0, 9998)`，≥1031 的加 1（均勻、排除 1031、不用重抽迴圈）。`CheckPassword(const FString& Code)` → `EPasswordInput`（不是 4 位數字也算 Wrong）。X017 這類 `bHasHpChance` 在 `ApplyInteraction` 用同一個亂數擲骰（`RandHelper(Den) < Num`），擲中才套 `HpChanceDelta`。取得密碼的函式只在 `#if !UE_BUILD_SHIPPING` 裡（步驟 12 測試模式用）；任何 log 都不寫密碼。
  - **E05 重啟＝新的一局**：生命值 5、已觸發與旗標清空、倒數回 30:00、重新產生密碼；成就與成就計數、已結束的玩家保留。
- **`FEscapeGameLogic`（EscapeGameLogic.*，步驟 5 建立）組合層，UE 端只跟它說話**：持有 Resolver、`FTeamState`、形態狀態（步驟 6）、`FAchievementTracker`（步驟 8）。
  - `static bool Load(Interactions, Forms, SceneObjects, Responses, Locations 五份 CSV 文字, int32 Seed, FEscapeGameLogic& Out, FString& OutError)`。
  - `FInteractOutcome Interact(const FString& PlayerId, const FString& TargetId)`、`FInteractOutcome SubmitPassword(PlayerId, Code)`、`ReturnToHuman(PlayerId)`、`Tick(float Dt)`、`AddPlayer(PlayerId, FVector Spawn)`、`SetPlayerLocation`／`SetItemLocation`、查詢函式。
  - `FInteractOutcome`：`EKind { Rejected, Blocked, NeedsPassword, Resolved }`、要顯示的文字、`FInteractionResult`、這次的 `FTeamEvent`、新解鎖成就（依玩家）、變身前後形態、進入的特殊地點。
  - 步驟 5：`SubmitPassword` 用 `CheckPassword` 的結果組 query（`State.Password`）再 Resolve；1031 輸錯 → 旗標 `Tried1031`。X017 文字有兩段「」：第一段是輸錯的文字，第二段是擲中扣血時**另外**顯示的文字（載入時檢查剛好兩段，否則報錯）。骰子 X020 文字裡的 `[a～b 隨機]` 用亂數換成點數。
  - 步驟 6 形態：每位玩家有 `FormId`、目前位置、人類身體位置、所在特殊地點（`""`／`L01`／`L02`）；每個物品 F01～F12 有 `bExists`（F08～F12 開局就在，其餘由「出現物品」產生）、位置、`OccupiedBy`、所在特殊地點（DT_Forms 的 Location 等於 DT_Locations 的 LocationName 時就是那個地點）。
    - 變身：Resolver 給 `TransformFormId`，或對象是可變身物品而 Resolver 回預設回應時（設計文件 §1 通用規則）。人類變身 → 身體留在當時位置；物品換物品 → 舊物品留在當時位置、放開。`ReturnToHuman` → 回到身體位置、離開特殊地點。
    - 對象是別人占用中的物品 → `Blocked`（不顯示文字、不改任何狀態、不算嘲諷次數）；物品還沒出現、ID 不存在、特殊地點裡的物品而玩家不在那個地點 → `Rejected`。
    - 特殊地點：形態在 DT_Locations 的 AllowedForms、對象是 Entrance、而且 Resolver 不是預設回應 → 玩家進入該地點。看得到整張地圖：形態的 Ability 含「整張地圖」（F10）。
    - E05 重啟：所有還沒結束的玩家變回人類、回到出生點；物品回到初始狀態（只剩開局就在的）。
  - 步驟 7 場景狀態：門與開門的互動 `{S19: X036, S05: X005, S09: X013}` 用小表（載入時檢查 X 存在且對象相符），`IsDoorOpen(DoorId)` 看該 X 是否已觸發。X022 已觸發＝停電：之後**任何密碼都當作輸錯**（走 X017），不讓「正確密碼的回應和輸錯不一樣」洩漏答案。
  - 步驟 8：`FAchievementTracker`（AchievementTracker.*），**每位玩家各自**的已解鎖與計數；全隊聯集仍寫進 `FTeamState`。A01 預設嘲諷（`bIsDefault` 而且不是變身）第 10 次；A02 變成物品（不含變回人類）第 20 次；A03 連續處於物品形態 > 600 秒（物品換物品不中斷，變回人類或 E05 重啟歸零，用 `Tick` 的 dt 累計）；A04／A06／A07 來自互動資料；E01～E05 結局 → 受影響的玩家解鎖同名成就。計數、碰過的門都跨 E05 保留；同一成就只回報一次。

### 步驟 9～12 的 UE 端架構（2026-10-31 developer session 規劃）
原則：能用文字檔表達的都放 repo 文字檔（可 diff、可重跑）；二進位資產（.umap）只由腳本產生；Blueprint 不新增；UI 與輸入都在 C++ 建，不做 WBP／IA 資產。
- **關卡資料**：`Content/Data/Level_Layout.csv`（自己的資料，不是設計文件產生的）：每列 `Kind,Id,Area,X,Y,Z,SX,SY,SZ,Yaw`，Kind ∈ `Area`（區域盒）、`Floor`、`Wall`、`Door`（S19／S05／S09 的擋門）、`Object`（F01～F12、S01～S20 的位置）、`Spawn`（玩家出生點）。區域 ID：`Classroom`、`Corridor`、`Office`、`Restroom`、`Storage`、`GuardRoom`、`L01`、`L02`（L01／L02 是地圖外的封閉小房間，只能靠進入特殊地點傳送）。牆高 ≥ 400 cm（跳不過）。
- **C++ 類別**（改 C++ 要關編輯器、`Build.sh` 編）：
  - `Core/EscapeAreaVolume`：`AActor` + `UBoxComponent`（只 overlap），`FName AreaId`。
  - `Forms/EscapeObjectActor`：場景物件 S01～S20，`FString ObjectId`、基本形狀 `UStaticMeshComponent`、名稱用文字標籤（`UTextRenderComponent`，繁中字型步驟 10 再換）。門（S19／S05／S09）`bIsDoor`、`SetDoorOpen(bool)` 關掉碰撞與隱藏。
  - `Forms/EscapeFormPawn`：可變身物品 F01～F12，`APawn`，`FString FormId`、網格、碰撞；步驟 11 再加移動與鏡頭。還沒出現的物品（F01～F07）開局隱藏、沒有碰撞。
  - `Core/EscapeGameMode`（取代 BP_ThirdPersonGameMode 當 Lvl_Academy 的 GameMode）：伺服器持有 `FEscapeGameLogic`，從 `Content/Data` 讀 CSV、種子用 `FDateTime::Now().GetTicks()`；Tick 時把狀態寫進 `AEscapeGameState`（生命值、剩餘秒數、門狀態；步驟 13 才處理網路複製）。
- **蓋關卡（步驟 9）**：`tools/mcp_build_graybox.py`（標準函式庫，`urllib` 連 `http://127.0.0.1:8000/mcp`）讀 `Level_Layout.csv`，組成一段 ProgrammaticToolset 腳本（`execute_tool_script`，沙盒只能 import json／math／re 等，用 `execute_tool(工具名, json)` 批次呼叫其他工具），一次 MCP 任務就把整個關卡重蓋一遍：`AssetTools.duplicate` 非 World Partition 的範本地圖（例如 `/Engine/Maps/Templates/Template_Default`）成 `/Game/Maps/Lvl_Academy`（已存在就先刪）→ `SceneTools.load_level` → 移除範本地板 → `SceneTools.add_to_scene_from_asset`（`/Engine/BasicShapes/Cube` 牆、地板）／`add_to_scene_from_class`（自己的 C++ 類別、PlayerStart）→ `ObjectTools.set_properties` 設 ObjectId／AreaId／門 → `ActorTools.set_actor_transform`、`set_label`、`SceneTools.set_actor_folder` → `AssetTools.save_assets`。執行前先 commit；編輯器用 headless 開：`UnrealEditor EscapeGame.uproject -nullrhi -unattended -nosplash -nosound -notraceserver -ExecCmds="ModelContextProtocol.StartServer"`（2026-11-01 實測：約 15 秒後 127.0.0.1:8000 可連，`ss` 確認只聽 127.0.0.1；關掉編輯器後 zenserver 會短暫繼承 0.0.0.0:1985，約 20 秒內自己結束）。MCP 呼叫方式：`initialize` 拿 `Mcp-Session-Id` → `notifications/initialized` → `tools/call` 名稱 `call_tool`，參數 `toolset_name`（完整名稱，例 `editor_toolset.toolsets.scene.SceneTools`）、`tool_name`、`arguments`。MCP 不能用時改 `UnrealEditor-Cmd -run=pythonscript`（0b 的後備），寫進調整紀錄。`DefaultEngine.ini` 的 GameDefaultMap／EditorStartupMap 改成 Lvl_Academy。
- **步驟 9 的測試**：Python `tests/test_step9_layout.py` 檢查 Level_Layout.csv（每個 ID 剛好一次、區域合法、物件在設計文件寫的區域盒裡）；UE `Step9_Level.spec.cpp` 載入 Lvl_Academy（編輯器世界）掃描 Actor：8 個區域、F／S 各一（S11 可多個）且在正確區域、PlayerStart 在教室；**可達性**：從出生點在 2D 格子（25 cm）上用玩家膠囊做 overlap 洪水填充，門關著時走不到走廊；只開 S19 → 走得到走廊、警衛室、廁所，走不到辦公室、儲物間；再開 S05／S09 → 走得到；L01／L02 永遠走不到。
- **組合層（步驟 10 一開始）**：`Core/EscapeGameMode`（伺服器持有 `FEscapeGameLogic`，從 `Content/Data` 讀 CSV，種子用 `FDateTime::Now().GetTicks()`，可用主控台變數或測試覆寫種子）、`Core/EscapeGameState`（給 HUD 的生命值、剩餘秒數、各玩家形態；步驟 13 才處理網路複製）、`Core/EscapePlayerController` 的伺服器函式 `ServerInteract(TargetId)`／`ServerSubmitPassword`／`ServerReturnToHuman`；每次 logic 狀態改變後同步場景 Actor（物品出現／隱藏、門開關、被占用的物品）。
- **介面（步驟 10，UI/）**：`UUserWidget` 子類別在 C++ 用 `WidgetTree` 建：HUD（生命值、`FormatClock`、形態名稱）、彈出文字框（任意鍵或 3 秒關閉；計時邏輯放純 C++ 類別可測）、四位數鍵盤（0～9、刪除、確認，最多 4 位）、結局畫面、成就提示（右上角，3 秒）。字型：系統的 `NotoSansCJK-Regular.ttc`（TC 子字型）複製到 `Content/Fonts/`（`.gitattributes` 加 LFS），執行時用 `FCompositeFont` 從檔案載入；🎂 要查 Slate 能不能顯示彩色表情（`NotoColorEmoji.ttf`），不能就用單色字型或另想辦法，不可以是方塊。結局與成就名稱：新增 `tools/design_to_endings.py` → `Content/Data/DT_Endings.csv`、`DT_Achievements.csv`（從設計文件 §7、§8 讀）。字型涵蓋測試用 Python 標準函式庫解析字型的 cmap。
- **操控（步驟 11）**：Enhanced Input 在 C++ 執行時建立 `UInputAction`／`UInputMappingContext`（WASD、滑鼠、E 互動、Q 變回人類）；各形態的移動方式與鏡頭臂長度放 `Content/Data/DT_FormControls.csv`（自己的資料：FormId、MoveMode=Hop／Roll／Static、ArmLength）。E：準星前方 250 cm 內有可互動物件就互動，沒有而且目前形態有「（自身）」互動時就是自身互動。
- **單人全路線（步驟 12）**：PIE 測試（`LatentIt` + `AutomationOpenMap` 或 `FStartPIECommand`，`-nullrhi`）載入 Lvl_Academy，用伺服器端函式（移到物件旁、呼叫互動）跑 E01、E03、E02（開發版測試模式 `EscapeGame.Dev.Password` 主控台指令讀密碼，`#if !UE_BUILD_SHIPPING`）、E04、E05；「5 分鐘內」用路線各段距離 ÷ 走路速度加每次互動的時間估算。測試明顯拖慢時分組，但 `tools/run_tests.sh` 仍一行跑全部。

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
- **步驟 2 的詮釋 → 全部同意（2026-10-31 人確認）**，包含 subagent 自己定的三項（`SELF` 的預設回應名稱、A／E 拆開、X017 只記機率不擲骰）：
  - 同一層有多列都成立時，條件數多的勝出（X035「已試過 1031」勝過 X032）；同數量取設計文件較前的列。
  - 「其他任一形態」排除「對該對象已有精確列」的形態，不管那一列的前置條件有沒有成立（X014 與 F05、X037 與 F12）。
  - X016 要密碼 `Correct`、X017 要 `Wrong`：設計文件沒有機器可讀欄位，Resolver 用 ID 小表對應，其他「（密碼鎖）」列載入時報錯。建議之後在設計文件加欄位。
  - 密碼結果 `None` 時人類碰大門回傳「需要輸入密碼」（`bNeedsPassword`），不回傳預設回應。
  - X034 不是獨立命中：F11 碰門，把這扇門算進去後四扇門都碰過時，結果是 X033，`ExtraTriggers` 多一筆 X034（帶 A07）。之後每次碰門都會再回傳，去重是步驟 8 的事。
  - X020、X021 的「（自身）」對象 ID 是 `SELF`；預設回應碰上 `SELF` 時 `[對象]` 用玩家目前形態的名稱。
  - 成就欄「A06、E04」：A 開頭放 `AchievementIds`，E 開頭放 `EndingId`（一列最多一個結局）。
  - VERIFY 第 3 條「椅子以外的形態」不含 F11（F11 碰教室門依第 2 條得到 X033）；第 5 條 X007 需要 X005 已觸發才成立，測試這樣設定。
- **步驟 3 的詮釋（待人確認）**：
  - ~~E05 本身要不要算進結局成就~~ → **設計文件已有答案**（`escape_room_script.md` §8：E05「上課睡覺的代價」是結局成就，解鎖條件「血量歸零」，死亡重來後保留）。步驟 3 的 `TeamState` 只回傳事件；**步驟 8 必須在 E05 事件時解鎖 E05，並和其他成就一樣跨重啟保留**。
  - E05 重啟時，本次互動的 X 也會跟著「已觸發」一起清掉（例：扣到 0 的那個 X027 重啟後不算已觸發）。
  - `ExtraTriggers`（X034）的互動 ID 與成就也記進已觸發／成就集合。
  - 結局欄 `EndingId`（E01～E04）這一步不處理，步驟 4、8 做。
  - 已逃出的玩家不算重啟的「受影響玩家」，也不會被重啟帶回遊戲。
- **步驟 4 的詮釋（待人確認，選了最保守的做法）**：
  - 倒數不倒數的條件：玩家名單**至少一位**、而且全部已結束時 `Tick` 不倒數；沒有任何玩家時照常倒數（方便單元測試與開局前）。
  - X021（時鐘自身互動）觸發 E04 時**不改剩餘秒數**（剩幾秒就顯示幾秒），只是把還沒結束的玩家全部標成 E04，並記下「這一局的 E04 已發生」，之後 `Tick` 不倒數、也不會再有第二個 E04。E05 重啟才把這個記號清掉。
  - 同一局裡 E04 已發生後，又有 X021 → 不再回傳 E04 事件（`ApplyInteraction` 仍會記已觸發與成就）。
  - 結局成就（E04 這個成就本身）這一步**沒有**解鎖，步驟 8 做；A06 由成就欄流程加進 `UnlockedAchievements`。
- **步驟 5 的詮釋（待人確認，選了最保守的做法）**：
  - 密碼用一顆 `FRandomStream`（`FTeamState(int32 Seed)`；無參數版本 = 種子 0）。這顆亂數**同時**給密碼與 X017 的機率骰子，所以每次輸錯、每次 E05 重啟都會讓之後的密碼序列不同（固定種子下仍完全可重現）。
  - X017 的 `HpDelta` 先套、再擲機率骰；擲中才套 `HpChanceDelta`（-1），扣到 0 照常 E05 重啟。X017 本身 `HpDelta` 是 0，所以順序沒有影響。
  - 輸入 1031 失敗的旗標 `Tried1031` 在**套用互動之前**加進去，所以這次輸錯若剛好擲中並歸零（E05），新的一局從沒有旗標開始。
  - 密碼 `CheckPassword` 要**完全等於**四位數字字串：「 1031」、空字串、五位數都算輸錯（也會走 X017 並擲骰）。
  - 輸入密碼的對象（S01）從 Resolver 資料找 `PasswordRequirement` 不是 NotRequired 的列，不寫死；載入時要求所有密碼鎖列同一個對象。
  - X017 的 Messages 是「第一段」、擲中再多「第二段」，內文不含「」；其他互動的 Messages 就是 `Result.Text` 原樣（X035 的文字因此仍帶外層「」）。
  - 被拒絕（`Rejected`）的情形：玩家不存在、已結束、`SubmitPassword` 時不是 F00。物件 ID 不存在這一步**不**拒絕（交給 Resolver 的預設回應），步驟 6 再處理。
  - `Interact` 碰大門回傳 `NeedsPassword` 時完全不改狀態（不記已觸發、不擲骰）。
  - X022 停電後密碼一律當輸錯是步驟 7 的事，這一步沒做。
- **步驟 6 的詮釋（待人確認，選了最保守的做法）**：
  - **離開特殊地點（設計文件沒寫清楚）**：`item_interactions.md` §3 的「離開方式：變回人類或變成其他物品（依變身規則）」，加上 §2.2「舊物品留在原地」，照字面會讓飲料罐垃圾卡在飲料機裡、學生證永遠出不了飲料機底下，E01 路線走不通。定案：在特殊地點裡變身成任何物品 → 玩家離開地點，新物品被帶出來放在入口物件（S03／S04）的位置；舊物品（例 F01）也放在入口位置；在地點裡變回人類 → 舊物品放在入口位置。物品不會被留在特殊地點裡（只有還沒被拿走的 F02／F03 在裡面）。入口位置要先用 `RegisterObjectLocation` 註冊，沒註冊就用玩家目前位置。
  - **在特殊地點裡能做的事**：只能碰「目前在該地點裡的物品」（物品的 `LocationId` 等於玩家所在地點；F02 一開始在 L01、F03 一開始在 L02，被帶出去之後就不算）和自己（`SELF`），其他（入口、別的地點的物品、一般場景物件）一律 `Rejected`。不在地點裡的人碰還在地點裡的物品（飲料機裡的飲料）也是 `Rejected`（從外面碰不到）。設計文件沒寫這兩條，是為了讓「看到飲料／看到學生證」只發生在進入之後。
  - **哪個物品屬於哪個特殊地點**：DT_Forms 的 Location 欄等於 DT_Locations 的 LocationName 才算（「飲料機內部」→ F02 屬於 L01、「飲料機底下」→ F03 屬於 L02）；「走廊」「廁所儲物間內部」等不是特殊地點。出現物品（`SpawnedItemId`）第一次出現時才產生，位置是這次互動對象的位置，屬於某地點的就在該地點裡；已存在的物品不重產、不移動（飲料被帶出來之後，別人再進飲料機不會看到新的飲料）。
  - **任何形態碰可變身物品就變成它**（§1 通用規則）：Resolver 回預設回應、對象是 F01～F12 時，視為變身；`Outcome.Result.bIsDefault` 設成 false、文字清空（沒有嘲諷文字）、`bTransformed` 為 true，並帶 `FromForm`／`ToForm`。步驟 8 要數「預設嘲諷」（A01）時看 `Result.bIsDefault`，要數「變成物品」（A02）時看 `bTransformed`（變回人類 `ReturnToHuman` 不算）。
  - **被別人占用的物品 → `Blocked`**：不顯示文字、不改任何狀態、不算任何次數；回應和「碰任何別人正在當的物品」完全一樣。碰自己正在當的物品 → `Rejected`。對象 ID 不存在、F00、還沒出現的物品 → `Rejected`（步驟 5 時「ID 不存在交給 Resolver 預設回應」改成這樣）。`SELF` 永遠可以。
  - **玩家位置**：玩家變成物品時，玩家位置＝物品位置；之後 `SetPlayerLocation` 會讓占用中的物品一起動，人類身體不動。放開物品時物品留在當時位置。逃出（E01～E03）或 E04 的玩家放開物品、物品留在當時位置（學生證留在大門旁）。進入特殊地點時玩家位置不變（UE 端自己決定要不要傳送到裡面）。
  - **E05 重啟**：還沒結束的玩家變回人類、位置與身體都回到 `AddPlayer` 的出生點、離開特殊地點；所有物品回到初始狀態（只有「一開始就在」的存在、位置回到 `RegisterObjectLocation` 的位置、沒有占用、在地點外）。已逃出的玩家不動。
  - **骰子 X020**：文字裡的 `[a～b 隨機]` 用 `FTeamState` 同一顆亂數（`RollRange`）換成整數，所以骰子也會讓之後的密碼序列不同（固定種子下仍可重現）。
- **步驟 7 的詮釋（待人確認，選了最保守的做法）**：
  - **停電後密碼一律當輸錯**：`SubmitPassword` 在 X022 已觸發後，不管輸入什麼都把 `State.Password` 設成 `Wrong`（走 X017，照常擲 1/1000，輸 1031 仍記 `Tried1031`）。理由：若正確密碼在停電後得到和輸錯不同的回應（例如預設嘲諷），等於洩漏「這組是對的」，違反設計文件「不得出現任何指向密碼的線索」。測試用同一個種子兩局，一局輸正確密碼、一局輸錯，Messages、事件、生命值逐項相同。代價：停電後玩家永遠看不到「門禁機沒電了」之類的提示——設計文件沒有這種文字，要加的話請人決定怎麼不洩漏。
  - **E05 重啟＝新的一局，門重新鎖上、電恢復**：門與電都只看「已觸發的 X」，E05 重啟會清掉已觸發集合，所以教室門、辦公室門、儲物間門重新鎖上，電閘可以再拉。這不違反「不可逆」（限制指的是同一局裡不能推回去）；若人認為電閘跨 E05 也要保持，需要另外加規則。
  - **門表只有三扇**：`{S19: X036, S05: X005, S09: X013}`。大門（S01）不在表裡，`IsDoorOpen("S01")` 永遠 false（大門靠學生證／密碼結束該玩家的遊戲，不是共用的開門狀態）。
  - **人類再碰配電箱**：資料如此，拉下後人類再碰仍是 X022（不會變成預設回應也不會恢復）；非人類碰是 X025 扣血，與是否停電無關。
  - **學生證碰大門（停電後）**：Resolver 因為 X010 的前置條件「未觸發 X022」不成立，回預設回應；這一步只確認，沒有改 Resolver。
- **步驟 8 的詮釋（待人確認，選了最保守的做法）**：
  - **A03 的門檻與容差**：連續處於物品形態的秒數用 `double` 累計，`> 600.0 + 1e-3` 才解鎖。`Tick` 的 dt 是 float 的 1/60，每格比 1/60 多約 8e-9 秒，累加 36000 格（600 秒整）約多 3e-4 秒，所以容差用 1e-3（遠小於一格 0.0167 秒）：600 秒整（36000 格）還沒解鎖、再多一格才解鎖，599 秒多一點更沒有。
  - **A03 只算 `Tick` 的時間**：沒有 `Tick` 就不會累計；已結束（逃出或 E04）的玩家不累計；人類形態不累計。變回人類（`ReturnToHuman`）與 E05 重啟歸零；物品換物品、進出特殊地點不歸零。E05 重啟歸零是我的詮釋（設計文件只說「離開超過 10 分鐘」，沒寫重啟怎麼算；重啟時玩家本來就被帶回人類身體）。
  - **A01 只數 `Kind == Resolved` 而且 `Result.bIsDefault`**：Blocked／Rejected／NeedsPassword、碰可變身物品的預設變身（步驟 6 已改成非嘲諷）都不算；`SELF` 的預設回應、`SubmitPassword` 若得到預設回應也算（資料上不會發生）。
  - **A02 數 `bTransformed`**：包含物品換物品、在特殊地點裡變身；`ReturnToHuman` 不算。
  - **A04／A06／A07 給「做這個互動的玩家」**；E04 由 X021 觸發時，A06 只給做互動的玩家，E04 給所有還沒逃出的玩家（A06 與 E04 是兩個不同的成就來源）。
  - **E01～E05 一律從 `FTeamEvent` 的受影響玩家發**：E01～E03 只有逃出的那位；E04、E05 是事件當下所有還沒逃出的玩家（已逃出的不會補發）。E05 也發（設計文件 §8），跨重啟保留。
  - **成就、計數、鑰匙圈碰過的門全部跨 E05 保留**（BRIEF：成就同一次遊玩跨局保留）。因為已觸發集合會被 E05 清掉，所以「碰過的門」放在追蹤器（每位玩家各自），不放在 `FTeamState`；組 query 時才放進 `State.TouchedDoors`。
  - **A07 碰門的記錄**：形態是 F11、對象在 Resolver 的門清單（`GetDoorIds()`）時，Resolve 之後把門記進該玩家的集合（Resolver 判定時已經把這次碰的門算進去，所以第四扇門那次就得到 X034）。X034 在四扇門都碰過之後，每次碰門仍會回傳（步驟 2 的詮釋），但成就只回報一次。碰門的記錄是每位玩家各自的：P1 碰三扇、P2 碰一扇不會合併。
  - **全隊聯集**：每個新解鎖也寫進 `FTeamState::AddUnlockedAchievement`（新增的函式）；Resolver 結果裡的成就欄 `ApplyInteraction` 本來就會加。聯集只是給步驟 3 的測試與之後的 UI 看，真正「誰解鎖了什麼」以追蹤器為準。
  - **`PopNewAchievements` 是單一佇列**：不分玩家，給 UE 端用；`FInteractOutcome::NewAchievements` 只有那次呼叫新解鎖的（`Interact`／`SubmitPassword`），`Tick` 與事件來的只在佇列裡（`Tick` 回傳型別沒改）。沒人取走的佇列會一直累積（一局最多幾十筆，沒有上限處理）。
  - **解鎖要求玩家存在**：`UnlockAchievement` 的玩家必須是 `AddPlayer` 加過的。
  - **不寫永久儲存**：追蹤器只有 `TMap`、`TSet`、`TArray`，沒有 static、沒有全域；`tests/test_step8_no_persistence.py` 掃整個 `Source/EscapeGame/`（含測試）不可以有 `SaveGame`／`FFileHelper::Save*`／`GConfig->Set*`／`SaveConfig`／`IFileManager`／`IPlatformFile`／`FArchive`／`CreateFileWriter`／`ofstream`／`fopen` 等（讀檔 `LoadFileToString` 不算）。這條 Python 測試在紅的 commit 就是綠的（目前程式碼本來就沒有這些 API），它是之後的防護網，不是「還沒實作」的紅。
- **步驟 9 的詮釋與配置（待人確認，選了最保守的做法）**：
  - **哪些門口開著**：警衛室（走廊北牆 X 300～500）與廁所（走廊北牆 X 1300～1500）是沒有門的開口，進得去；只有教室（S19）、辦公室（S05）、儲物間（S09）三扇會擋人。設計文件沒寫警衛室、廁所有沒有門，照約定的建議配置開著。F10 監視器、F11 鑰匙圈、S15 配電箱在警衛室，所以開了教室門就拿得到。
  - **L01／L02 放哪**：地圖外東邊 X 5000～5400（L01 在 Y 0～400，L02 在 Y 600～1000），各自一個封閉小房間（地板＋四面牆＋天花板），沒有門也沒有路；地板之間隔著空地，步行到不了。進入方式是步驟 6 的特殊地點邏輯，UE 端怎麼傳送進去留給步驟 10～12。天花板用 `Floor` 列表示（`Ceiling_L01`、`Ceiling_L02`），因為 `Wall` 列規定 SZ ≥ 400。
  - **門擋在哪一側**：門方塊是兼 S 物件的擋路盒子，中心點必須在設計文件指定的區域盒裡（S19 在 Classroom、S05 在 Corridor、S09 在 Restroom，因為 DT_SceneObjects 的位置欄就是這樣寫），所以 S19 往教室內凸出 40 cm、S05 往走廊內凸出 60 cm、S09 往廁所內凸出 40 cm；開啟時隱藏且沒有碰撞，門洞寬 200 cm。
  - **物件都是方塊**：F／S 全部是 `AEscapeObjectActor`（BlockAll 方塊，大小是估的），沒有 `AEscapeFormPawn`（F00 人類身體在步驟 11 才放，spec 查詢本來就忽略它）。F01～F07 目前一開始就放在關卡裡，「還沒出現、隱藏、沒有碰撞」的規則（PLANS 架構）是步驟 10 組合層同步場景時做，不在這一步。
  - **配置是我估的**：尺寸不是設計文件給的；人想改就改 `Level_Layout.csv`、重跑 `tools/mcp_build_graybox.py`。
- 形態表（`item_interactions.md` §1）裡混進了 X027、X032、X035 三列 → `design_to_csv.py` 依 ID 前綴歸類即可，不需改文件。
- ~~成就沒有 A05~~ → **刻意跳過**（2026-10-31 人確認）。
- ~~互動沒有 X019~~ → **刻意跳過**（2026-10-31 人確認）。
- ~~人數上限~~ → **3 人**（2026-10-31 人決定），已寫進 BRIEF；VERIFY STEP 13、18 已改成 3 人（`ba140d4`）。

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

- **2026-10-31 步驟 0b 完成**：人在 Firefox 實測 Pixel Streaming，鍵盤滑鼠可以操控角色 → 0b 勾掉。
  另外列出 Epic MCP 全部 toolset，和 Flopperam 開源版的功能逐項比對：Blueprint（53 個工具，含圖的 DSL 讀寫）、Actor、材質、UMG（23 個）、DataTable、PIE、截圖、自動化測試都有；只少了蓋牆／房子／樓梯的巨集 → 已寫進「工具分工」。

- **2026-10-31 步驟 0c UE 專案與 UE 測試環境**：
  - **怎麼建專案**：照 `Templates/TP_ThirdPerson/Config/TemplateDefs.ini` 手動做出編輯器「新增專案」會做的事（一次性，沒有留腳本）：複製範本、跳過 FoldersToIgnore／FilesToIgnore；cpp／h／ini／cs 裡 `TP_ThirdPerson` 改 `EscapeGame`（大小寫都處理），資料夾與檔名跟著改；三個 SharedContentPacks（LevelPrototyping、Characters、Input）從 `Templates/TemplateResources/High/*/Content` 複製到 `Content/<MountName>/`（`CopyAdditionalFilesToFolder` 的規則）；`DefaultGame.ini` 換新的 ProjectID、ProjectName。
    - **拿掉三個 Variant**：`Source/EscapeGame/Variant_{Combat,Platforming,SideScrolling}`、`Content/Variant_*`、`Content/__ExternalActors__/Variant_*`、`Content/__ExternalObjects__/Variant_*`，以及 Build.cs 的 Variant include 路徑。
    - `.uproject` 保留範本的 ModelingToolsEditorMode、StateTree、GameplayStateTree，加 `ModelContextProtocol`、`AllToolsets`（都 `TargetAllowList: ["Editor"]`）。PixelStreaming2 留到步驟 17。預設地圖仍是 `Lvl_ThirdPerson`。
    - **redirect**：`GameProjectUtils` 只加 `+ActiveClassRedirects`（類別名）。藍圖父類別存的是 `/Script/TP_ThirdPerson.TP_ThirdPersonCharacter` 這種路徑，所以我改用 `[CoreRedirects]` 加 `PackageRedirects`（`/Script/TP_ThirdPerson` → `/Script/EscapeGame`）和三條 `ClassRedirects`（Character、GameMode、PlayerController）。用 `-run=pythonscript` 實際載入三個 BP_ThirdPerson* 藍圖，確認父類別解析成 `EscapeGameCharacter`／`EscapeGameGameMode`／`EscapeGamePlayerController`；載入 `Lvl_ThirdPerson` 沒有「找不到父類別」或載入失敗的錯誤。
    - 範本留下的 `Content/ThirdPerson/Blueprints/BP_ThirdPerson*` 名稱還沒改（藍圖改名會動 uasset，之後做步驟 9、11 時再決定）。
  - **UE 測試怎麼接進 `run_tests.sh`**：沒有改 `run_tests.sh`。由 `tests/test_step0c_ue_project.py` 的一個測試呼叫 `tools/run_ue_tests.py`（增量編譯 → `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests EscapeGame;Quit" -nullrhi -unattended -nopause -nosplash -nosound -ReportExportPath=Saved/Automation/Report` → 讀 `index.json` → 印出每條 `EscapeGame.` 測試與「n/n 全部通過」）。原因：`test_step0_env.py` 會巢狀再跑一次 `run_tests.sh`，逾時只有 120 秒、而且要求最後一行是 `OK`；所以這個測試在 `ESCAPEGAME_NESTED_TEST_RUN` 有值時 skip，UE 只在最外層跑一次。UE 沒裝時測試**失敗**（不是 skip）。
  - **時間**：首次編譯（引擎是預編譯版，只編專案模組 + 測試）約 26 秒（`-MaxParallelActions=10`，log 有「Executing up to 10 actions based on MaxProcessorCount override」，確認有效）；之後增量編譯幾秒。一次 UE 測試（含增量編譯與開關編輯器）約 13～17 秒；整個 `tools/run_tests.sh` 約 16 秒。`-run=pythonscript` 載入藍圖的 commandlet 尖峰 RAM 約 2.2 GB。
  - **遇到的問題**：
    1. 自己的 spec 測試名稱含「.」（`UE 5.8`、`EscapeGame.`），UE 會當成路徑分隔，名稱被截斷；`GetTestName()` 只回傳最後一段。在寫程式前另開一個 commit 修正名稱與改用 `GetBeautifiedTestName()`，斷言內容沒有放寬。**之後的 spec 名稱不要含「.」。**
    2. 非 Automation 的 `-ExecCmds="...;Quit"`：`;` 不會分隔（要用逗號），而且 `UnrealEditor-Cmd` 收到 `Quit` 後在這台機器上會卡住不結束（不是 Automation 路徑）。`run_ue_tests.py` 用 Automation 自己的 `;Quit`，會正常結束；其他用法要加 `timeout` 並用 `kill` 收尾。
    3. 從引擎複製來的檔案是唯讀，改名取代前要 `chmod -R u+w`。
    4. 每次編輯器 log 都有 `Asset manager settings do not include a rule for assets of type GameFeatureData` 的 Error，是範本專案的固有訊息，不影響載入；先不處理。
    5. UBT 編譯時會短暫開 `UbaServer - Listening on 0.0.0.0:1345`（Unreal Build Accelerator，只有編譯期間）。MCP 仍只聽 127.0.0.1；UBA 這個埠步驟 17 部署時要一起擋（部署機不會編譯，應該不會有）。
  - `.mcp.json` 由 `ModelContextProtocol.GenerateClientConfig ClaudeCode` 產生，伺服器名稱 `unreal-mcp`、`http://127.0.0.1:8000/mcp`。
  - **0c 修正（驗收時發現的兩個問題）**：
    1. `run_ue_tests.py` 會卡住：UE 啟動的 `UnrealTraceServer fork` 繼承了 stdout／stderr 管道，`subprocess.run(stdout=PIPE)` 等不到 EOF（UE 本身早已結束）。修法：編譯與 UE 的輸出都改寫到 `Saved/Automation/build.log`、`ue_tests.log`，不用 PIPE，stdin 接 DEVNULL；失敗時印 log 最後 4000 字。另外查引擎原始碼（`TraceAuxiliary.cpp`）確認 `-notraceserver` 會跳過自動啟動 Trace 伺服器，已加進測試命令列；實測跑完沒有 UnrealTraceServer 留下。（2026-10-31 更正：1985 不是 Trace 伺服器開的，`-notraceserver` 擋不掉它，見「做法」。）
    2. `run_tests.sh` 最後一行不是 `OK`：stdout 非 tty 時有緩衝，UE 清單在 unittest 總結（stderr）之後才出現。修法：`python3 -I -u`（不緩衝），沒有改測試。

- **2026-10-31 人接受 5d60e51**（0c 修改 spec 測試名稱）；之後 UE spec 的 Describe/It 名稱不可以含「.」。

- **2026-10-31 步驟 1 設計文件轉 CSV**：`tools/design_to_csv.py`（標準函式庫），輸出 4 個 `Content/Data/DT_*.csv` 並 commit（`.gitattributes` 只有 uasset／umap 走 LFS，csv 不是）。列數：Forms 13、SceneObjects 20、Locations 2、Interactions 36。
  - **歸類**：依每列第一欄 ID 前綴（F／S／L／X），不看它在哪一節。X027、X032、X035 因此進 DT_Interactions。其他表（結局 E、成就 A、基本資料……）不處理；§5 預設回應、成就、結局這一步都**沒做**。
  - **欄數**：以前綴自己的表頭為準，所以形態表裡的 X 列要 11 欄（不是形態表的 6 欄）。
  - **CSV 格式**：UTF-8、無 BOM、`\n` 換行、`csv` 模組處理引號；第一欄 `Name` 放 ID；儲存格只去前後空白，其餘原樣（中文、全形標點、`🎂`、反引號）。列順序＝設計文件出現順序（S10 在 S09 前面、X027／X032／X035 在 X018 前面）。沒有加「所屬區域小節」欄（步驟 2 用不到）。
  - **欄名對照**（步驟 2 的 USTRUCT 要一致；也寫在腳本最上面）：
    - Forms：形態 ID→Name、形態名稱→FormName、取得方式→Acquisition、位置→Location、形態能力／限制→Ability、說明→Description
    - SceneObjects：物件 ID→Name、名稱→ObjectName、位置→Location、說明→Description
    - Locations：地點 ID→Name、地點名稱→LocationName、入口→Entrance、可進入形態→AllowedForms、進入後內容→Contents、離開方式→ExitMethod
    - Interactions：互動 ID→Name、玩家形態→PlayerForm、互動對象→Target、事件類型→EventType、事件描述→EventText、變身結果→TransformResult、出現物品→SpawnedItem、生命值變化→HpChange、成就→Achievement、前置條件→Precondition、備註→Notes
  - **錯誤處理**：欄位數不對、ID 重複、表頭和預期不同、我們的表裡第一欄不是 ID → 結束碼 1，stderr 寫「檔名:行號: ID …」。全部在記憶體解析成功後才寫暫存檔再一起替換，出錯不動既有 CSV、也不建立新檔。
  - **給步驟 2 的建議**：所有欄位目前都是字串（含 HpChange 如「-1」、「每次輸錯有 1/1000 機率 -1」）；結構化（機率、前置條件、形態 ID 與對象 ID 拆開）要在步驟 2 的 Resolver 或另一層做，或之後再請人決定是否在設計文件加機器可讀欄位。步驟 2 若要「預設嘲諷」文字、成就名稱、結局，需要另外的 CSV（DT_Achievements／DT_Endings），到時再加進腳本。
  - **遇到的問題**：形態表混入的 X 列欄數和形態表表頭不同，所以不能用「同一個表的表頭」檢查欄數。

- **2026-10-31 步驟 2 互動判定**：`Source/EscapeGame/Logic/InteractionResolver.h/.cpp`（只用 Core：`FCsvParser`、FString、TArray、TSet、TMap，沒有 USTRUCT）。
  - **預設回應的資料來源**：新增 `tools/design_to_responses.py` → `Content/Data/DT_Responses.csv`（`Name,Text`，一列 `Default`，`[對象]` 保留）。原本要擴充 `design_to_csv.py`，但步驟 1 的測試斷言輸出資料夾恰好 4 個檔，改它要動步驟 1 的測試，人決定改用獨立腳本；`design_to_csv.py` 與步驟 1 測試沒動。
  - **資料結構**：`FInteractionRow`（CSV 原始文字加解析後欄位：形態規則 Exact／AnyForm／OtherForm／NonHuman、對象種類 Id／Self／AnyDoor／AllDoorsTouched、條件清單、密碼需求、結果欄位）、`FInteractionQuery`（形態、對象、`FInteractionState`：已觸發 X、旗標、已碰過的門、密碼輸入結果）、`FInteractionResult`（互動 ID、`bIsDefault`、`bNeedsPassword`、文字、HpDelta、機率扣血 numerator／denominator／delta、物品、變身、成就、結局、`ExtraTriggers`）。`FInteractionResolver::Load(四份 CSV 文字, …)` 失敗回傳 false 並寫錯誤；`Resolve` 是 const 純函式；`GetDisplayName` 查 ID → 名稱。
  - **優先序實作**：(1) 資料裡 `AnyDoor` 的列（X033）：形態符合、對象在門清單、條件成立；門清單與形態都從 CSV 解析，沒寫死 → (2) 精確 → (3) 其他任一形態／非人類形態 → (4) 任一形態 → (5) 預設回應。每層只看前置條件成立的列。
  - **前置條件文法**：`條件 := 子句 { "，且" 子句 }`；`子句 := [已觸發過｜未觸發｜尚未觸發] Xnnn｜Xnnn 碰過全部的門｜曾在大門輸入 dddd 且失敗（旗標 Tried<dddd>）`。對象欄括號：（第一次）＝這一列自己未觸發；（第二次以後）＝同對象「第一次」列已觸發；（已試過 dddd）＝旗標；（密碼鎖）＝密碼小表。引用不存在的 X／形態／對象、無法解析的文字，載入時一律報錯並指出互動 ID。
  - **測試**：spec 每列 X 一個 `It`（36）、VERIFY 第 2～7 條各一個、再加「需要輸入密碼」「無法解析的前置條件載入報錯」，共 44 條；X034 的測試檢查 `ExtraTriggers`。測試端自己從原始 CSV 文字解讀形態與狀態（不用 Resolver 的解析結果）。「任一形態」等規則挑形態：依形態表順序取第一個「該列接受、沒有更高層的列接受、不是鑰匙圈（碰門時）」的形態。Python：`test_step2_default_response.py`（5 條）、`test_step2_logic_isolation.py`（3 條，白名單寫在測試裡，`FCsvParser` 在 `Serialization/Csv/CsvParser.h`）。
  - **時間**：整個 `tools/run_tests.sh` 約 21 秒（UE 46/46，含增量編譯）。
  - **沒有遇到需要改測試的情況**；第一次實作就全綠。

- **2026-10-31 步驟 3 生命值**：`Logic/TeamState.h/.cpp`，純 C++（`FTeamState`），只 include `CoreMinimal.h` 與 `Logic/InteractionResolver.h`，沒有改 Resolver。
  - **資料**：`Hp`（開局 5、無上限、最低 0）、`RestartCount`、玩家名單（ID、是否逃出、結局 ID，依加入順序）、`TriggeredIds`（已觸發 X）、`UnlockedAchievements`。
  - **介面**：`ApplyHpDelta(int32)`、`ApplyInteraction(FInteractionResult)`（記已觸發含 ExtraTriggers → 加成就 → 套 HpDelta，回傳 `TArray<FTeamEvent>`）、`AddPlayer`／`MarkEscaped`／`HasEscaped`／`GetEscapeEnding`／`GetRemainingPlayers`、`MakeSnapshot()`（目前只帶 `TriggeredIds`）。`FTeamEvent` = `EndingId` + `AffectedPlayers`。
  - **E05 重啟**：生命值從 >0 變成 0 時回傳 E05（受影響＝當下還沒逃出的玩家），然後 **清掉**：已觸發集合（生命值回 5）；**保留**：已解鎖成就、玩家名單與已逃出狀態及其結局；`RestartCount` +1。1 扣 2 只到 0、只觸發一次。
  - `bHasHpChance`（X017）不擲骰，步驟 5 處理。
  - X004／X026／X027 的測試走真流程：CSV → Resolver → `MakeSnapshot()` 組 query → `ApplyInteraction` 回 TeamState。
  - **時間**：一次完整 `tools/run_tests.sh`（含增量編譯與 UE 53 條）約 20 秒（Python 35 條）。測試當時沒有別的 UnrealEditor 行程，可用記憶體約 12 GB。

- **2026-10-31 步驟 4 時間**：只改 `Logic/TeamState.h/.cpp`（沒有建 EscapeGameLogic，那是步驟 5），新增 `Tests/Step4_Clock.spec.cpp`（`EscapeGame.Step4`，7 條 It）。
  - **介面**：`static constexpr int32 RoundSeconds = 1800`、`TArray<FTeamEvent> Tick(float Dt)`、`int32 GetRemainingSeconds() const`、`static FString FormatClock(int32)`（1800 → `30:00`、59 → `00:59`、0 → `00:00`）、`ApplyInteraction(Result, PlayerId = FString())`（舊呼叫方式不變，步驟 3 測試沒動）。`PlayerId` 這一步沒用到，步驟 5 的 E01～E03 才用。
  - **倒數**：用 `double` 累計已過秒數；`GetRemainingSeconds` = `ceil(1800 - 已過 - 1e-4)`，夾在 0～1800。固定 dt 1/60 跑 60 格剛好 1799；整局 108000 格（float 的 1/60）在第 108000 格（±1）觸發 E04，且只有一次。
  - **E04**：時間到 0 或 `ApplyInteraction` 的 `Result.EndingId == "E04"`（X021）→ `EndRoundWithDawn()`：受影響＝`GetRemainingPlayers()`，全部 `MarkEscaped(…, "E04")`（沿用 `bEscaped`／`EndingId`），已逃出的玩家（例 P2 結局 E01）不受影響。時間到與 X021 兩條路徑都有測「不可以已逃出再收到 E04」。A06 由原本的成就欄流程進 `UnlockedAchievements`（確認即可，沒改）。
  - **E05 重啟**：倒數回到 30:00、E04 記號清掉（另有一條 It）；密碼與旗標是之後的步驟。
  - **X021 測試**走真流程：CSV → `Load` → query（F09、`SELF`、`MakeSnapshot()`）→ `ApplyInteraction(R, "P1")`。
  - **沒有碰** E01～E03（步驟 5）、結局成就（步驟 8）。詮釋與取捨見「待人確認」的步驟 4 一項。
  - **時間／測試**：整個 `tools/run_tests.sh` 約 20 秒（含增量編譯）。紅的時候 Step4 的 7 條都失敗、舊的全綠（UE 53/60）。最後 Python 35/35；UE 60/60。第一次實作就全綠，沒有改過測試。

- **2026-10-31 步驟 5 大門密碼**：改 `Logic/TeamState.h/.cpp`，新建 `Logic/EscapeGameLogic.h/.cpp`，新增 `Tests/Step5_DoorPassword.spec.cpp`（`EscapeGame.Step5`，8 條 VERIFY + 5 條 PLANS 詮釋，共 13 條 It）與 `tests/test_step5_password_secrecy.py`（6 條）。
  - **FTeamState**：`explicit FTeamState(int32 Seed)`（無參數 = 種子 0，舊測試沒動）；內部 `FRandomStream`；密碼 `RandRange(0, 9998)`，≥1031 加 1，`%04d`。`CheckPassword(Code)` → `EPasswordInput`；`AddFlag`／`HasFlag`／`GetFlags`，`MakeSnapshot()` 帶旗標；E05 重啟＝`StartNewRound()`：清旗標、重抽密碼（和倒數、已觸發一起重置）。`ApplyInteraction(Result, PlayerId = {}, bool* OutChanceHit = nullptr)`：E01～E03 → 只 `MarkEscaped(PlayerId)` 並回傳 `FTeamEvent{結局, [PlayerId]}`（PlayerId 空／不存在／已結束 → 沒事件）；`bHasHpChance` → `RandHelper(Den) < Num` 才套 `HpChanceDelta`。
  - **密碼不外洩**：`GetPasswordForDevOnly()` 與 `StartNewRoundForTesting()` 只在 `#if !UE_BUILD_SHIPPING` 裡；`Password` 是 private，沒有 getter、沒有任何 log。Step5 spec 整個檔案包在 `#if !UE_BUILD_SHIPPING`（因為它要呼叫 DevOnly 函式）。Python 檢查器（`strip_comments` 處理字串裡的 `//`、輸出類呼叫的整個陳述句、`#if` 堆疊追蹤 `#else`／`#elif`）掃整個 `Source/EscapeGame/`，另有 3 條測檢查器本身會抓到故意寫壞的範例。
  - **FEscapeGameLogic**：`Load(五份 CSV, Seed, Out, OutError)`（Locations 先只存起來；X017 文字不是剛好兩段「…」→ 失敗並寫出 `X017`；所有密碼鎖列要同一個對象）、`AddPlayer`、`Interact`、`SubmitPassword`、`Tick`、`GetTeam`、`GetResolver`；`FInteractOutcome{Kind, Result, Messages, Events, bHpChanceHit}`、`EInteractKind{Rejected, Blocked, NeedsPassword, Resolved}`（`Blocked` 這一步還用不到）。旗標名稱 `Tried1031` 與 Resolver 解析「曾在大門輸入 1031 且失敗」/「已試過 1031」產生的相同（X035 測試走真流程確認）。
  - **測試**：10,000 局用兩種做法（10,000 個種子各一局；同一個種子 `StartNewRoundForTesting` 10,000 次），都沒有 1031，並確認 1031 兩邊的數字都出得來。100,000 次輸錯用種子 20261031（第一個選的種子，沒有挑），擲中 117 次（70～130 內）、E05 重啟 23 次（= 117 / 5）、每一次都是 `Resolved` + X017。密碼不外洩：30 個種子 × 每個 S 物件 + 各種錯碼 + 正確碼，收集所有 `Messages` 與 `Result.Text`，沒有一則含當局密碼。
  - **偏離 PLANS 架構的地方**：沒有。`FInteractOutcome` 的欄位照派工單（`Events` 一個陣列、`bHpChanceHit`），「新解鎖成就、變身前後形態、進入的特殊地點」留給步驟 6、8。
  - **時間／測試**：每輪 `tools/run_tests.sh` 約 20～40 秒（含增量編譯）。紅的時候 Step5 的 13 條都失敗（空殼）、舊的 UE 測試全綠、Python 41 條裡只有 UE 那條失敗；第一次紅跑時我的測試輔助函式拿空密碼取 `Out[0]` 讓測試行程當掉（segfault），改成密碼不是四位時回傳一定錯的碼，再重跑才是乾淨的「斷言失敗」。最後 Python 41/41；UE 73/73。綠的階段第一次實作就全綠，沒有改過測試。詮釋與取捨見「待人確認」的步驟 5 一項。

- **2026-10-31 步驟 6 變身**：改 `Logic/EscapeGameLogic.h/.cpp`（組合層加形態與位置）、`Logic/TeamState.h/.cpp`（加 `RollRange`），新增 `Tests/Step6_Transform.spec.cpp`（`EscapeGame.Step6`，7 條 VERIFY + 8 條 PLANS 詮釋，共 15 條 It）。`InteractionResolver` 沒動。
  - **資料**：`Load` 現在解析 DT_Forms／DT_SceneObjects／DT_Locations：F01～F12 是物品（F00 不是），`bStartsPresent`＝Acquisition 是「一開始就在」，`HomeLocationId`＝Location 欄等於某個 LocationName，`bSeesWholeMap`＝Ability 含「整張地圖」；地點的入口與可進入形態用「Prefix 加數字」掃描（"F01 飲料罐垃圾"→F01）。載入時檢查入口是 DT_SceneObjects 裡的一個 S、可進入形態與所有出現物品在 DT_Forms，否則報錯並寫出地點／互動 ID。
  - **狀態**：玩家 `FormId`／`Location`／`BodyLocation`／`SpawnLocation`／`LocationId`；物品 `bExists`／`Location`／`InitialLocation`／`OccupiedBy`／`LocationId`。`FInteractOutcome` 加 `bTransformed`、`FromForm`、`ToForm`、`EnteredLocationId`。介面：`RegisterObjectLocation`、`SetObjectLocation`（PLANS 架構寫 `SetItemLocation`，改名因為 S 物件也要登記位置）、`SetPlayerLocation`、`ReturnToHuman`、`GetPlayerForm`／`GetPlayerLocation`／`GetBodyLocation`／`GetPlayerLocationId`／`CanSeeWholeMap`／`GetItemLocation`／`IsItemPresent`／`GetItemOccupant`／`GetItemLocationId`。
  - **Interact 檢查順序**照派工單：玩家 → 對象 ID（SELF／F／S，F00 不行）→ 物品已出現／被占用（別人 `Blocked`、自己 `Rejected`）→ 特殊地點規則；都通過才 Resolve。出現物品 → 進入地點 → 變身 → `Apply`（TeamState）；`Apply` 之後處理事件（結束的玩家放開物品、E05 重啟）。`Tick` 的 E04 也走同一條。
  - **偏離 PLANS 架構**：`SetItemLocation` 改叫 `SetObjectLocation`；其餘照做。詮釋與取捨見「待人確認」的步驟 6 一項（特殊地點的離開方式是最大的一項）。
  - **時間／測試**：寫測試約 1 輪、實作約 1 輪；整個 `tools/run_tests.sh` 約 25 秒（含增量編譯）。紅的時候 Step6 的 15 條都失敗（空殼）、舊的 UE 測試 73 條全綠（第一次紅跑因為 `FindPlayer` 的非 const 多載只有宣告沒有定義，連結失敗，先把查找函式補上再重跑）。最後 Python 41/41；UE 88/88。實作第一次就全綠，沒有改過測試。

- **2026-10-31 步驟 7 場景狀態**：改 `Logic/EscapeGameLogic.h/.cpp`，新增 `Tests/Step7_SceneState.spec.cpp`（`EscapeGame.Step7`，6 條 VERIFY + 5 條 PLANS 詮釋，共 11 條 It）。`TeamState`、`InteractionResolver` 沒動。
  - **做法**：場景狀態不另外存，全部由 `FTeamState` 的已觸發 X 推出來，所以全隊共用、E05 重啟自動清掉。`IsDoorOpen(DoorId)`＝門表裡該門的 X 已觸發（`{S19: X036, S05: X005, S09: X013}`，cpp 裡的小常數表）；`GetOpenDoors()` 依門表順序回傳；`IsPowerCut()`＝X022 已觸發（X022／S15 也是常數）。不在門表的 ID（含 S01）一律 false。
  - **載入檢查**：`Load` 檢查門表三列與 X022 都存在、`TargetKind` 是單一 ID 而且等於那扇門（S15），否則回傳 false 並寫出互動 ID 與實際的對象文字。
  - **停電後密碼**：`SubmitPassword` 在 `IsPowerCut()` 時把 `Password` 設成 `Wrong`（見待人確認）。學生證碰大門不用改：Resolver 已因 X010 的「未觸發 X022」給預設回應。
  - **測試**：每條 VERIFY 一個 It；測試用 `BecomeForm` 輔助函式走真的互動路線變成 F00～F12 每一種形態（F05 要先 X005／X006、F06 要先 X005／X008、F07 要先 F05 開儲物間門 X013），「不可以推回去」13 種形態各跑一局。「停電後正確密碼和輸錯一樣」用同一個種子的兩局對照，並連續 200 次。E05 測試用椅子連碰配電箱 5 次（X025 每次 -1）扣到 0。載入檢查測試用把 CSV 的對象換掉來弄壞資料（X036／X005／X013／X022 各一次）再加刪掉 X022 那一列。
  - **紅的時候**：header 與 cpp 先放空殼（`IsDoorOpen` 回 false 等），spec 編得過。Step7 11 條裡 10 條失敗，原因都是「還沒實作」的斷言（停電／門開沒有、停電後密碼不走 X017、載入不報錯）；第 3 條 VERIFY（非人類碰配電箱 X025 扣血）在紅的時候就通過，因為那是 Resolver 資料已有的行為（步驟 2），這條只是確認組合層沒有擋掉它。舊的 88 條全綠。
  - **偏離 PLANS 架構的地方**：沒有。
  - **時間／測試**：寫測試約 1 輪、實作約 1 輪；整個 `tools/run_tests.sh` 約 25 秒（含增量編譯）。最後 Python 41/41；UE 99/99。實作第一次就全綠，沒有改過測試。

- **2026-10-31 步驟 8 成就**：新增 `Logic/AchievementTracker.h/.cpp`（`FAchievementTracker`，只用 Core），改 `Logic/EscapeGameLogic.h/.cpp`（接上追蹤器）、`Logic/TeamState.h/.cpp`（加 `AddUnlockedAchievement`），新增 `Tests/Step8_Achievements.spec.cpp`（`EscapeGame.Step8`，13 條 VERIFY 相關 + 9 條 PLANS 詮釋，共 22 條 It）與 `tests/test_step8_no_persistence.py`（5 條）。`InteractionResolver` 沒動。
  - **追蹤器**：每位玩家一筆紀錄（已解鎖清單依解鎖順序、嘲諷次數、變身次數、連續物品形態秒數 `double`、鑰匙圈碰過的門）。`Unlock` 第一次 true 之後 false；門檻是具名常數（`TauntThreshold = 10`、`TransformThreshold = 20`、`ItemFormSecondsThreshold = 600.0`、`SecondsTolerance = 1e-3`）。
  - **組合層**：`FAchievementUnlock { PlayerId, AchievementId }`；`FInteractOutcome::NewAchievements`；`PopNewAchievements()` 取出並清空佇列。解鎖統一走 `UnlockAchievement`（追蹤器 → 全隊聯集 → 佇列）。A01 在 `Apply`（預設嘲諷）、A02 在 `Interact` 變身之後、A03 在 `Tick`、A04／A06／A07 在 `Apply` 讀 `Result.AchievementIds` 與 `ExtraTriggers`、E01～E05 讀事件（`Interact`／`SubmitPassword`／`Tick` 三條路徑都有）。
  - **補上 X034 永遠不會觸發的缺口**：`Interact` 組 query 時把該玩家碰過的門放進 `State.TouchedDoors`，F11 碰門後記進集合。步驟 2 的 Resolver 與測試沒動。
  - **偏離 PLANS 架構的地方**：沒有。詮釋與取捨見「待人確認」的步驟 8 一項。
  - **測試**：每條 VERIFY 一個 It（結局成就 E01、E02、E03、E04（時鐘）、E04（時間到）、E05 各一條，所以 VERIFY 第 6 條共 6 個 It）；A01 用「人類碰講桌 S16」（互動表沒有人類碰講桌，一定是預設嘲諷，測試裡每次都檢查 `bIsDefault`）；A02 用椅子 F12 與骰子 F08 來回、每 4 次中間變回人類；A03 跑 599 秒、600 秒整、再多一格；A07 先碰 S01 兩次、S05、S09 確認同一扇門不重複計、只有三扇不算。E03 路線走真的互動（X009 → F04 → X005 → X006 → F05 → X013 → F07 → X015）。額外：跨 E05（嘲諷 6 次 → E05 → 再 4 次；鑰匙圈碰三扇 → E05 → 只碰第四扇就 A07）、每位玩家各自、`PopNewAchievements`（Interact／Tick／事件三種來源）、`NewAchievements` 與佇列一致、A03 物品換物品不歸零／變回人類歸零／E05 歸零、Blocked／Rejected／NeedsPassword／變身不算嘲諷、追蹤器單元。不可以寫進永久儲存：C++ 端兩個實例先後建立、第一個結束後第二個沒有成就；Python 端靜態掃描與檢查器自測。
  - **時間／測試**：寫測試約 1 輪、實作約 1 輪；整個 `tools/run_tests.sh` 約 25 秒（含增量編譯）。紅的時候 Step8 的 22 條都失敗（空殼）、舊的 99 條 UE 測試全綠，Python 5 條新測試是綠的（防護網，見待人確認）。最後 Python 46/46；UE 121/121。實作第一次就全綠，沒有改過測試。
- **2026-11-01 步驟 6 修正：特殊地點檢查看物品目前的位置**：
  - **誰發現的**：另一個唯讀檢查 session。`Interact` 的特殊地點檢查看的是物品的 `HomeLocationId`（原本屬於哪個地點），不是 `LocationId`（目前在哪）。情境一：P1 把飲料 F02 帶出 L01、在別處變回人類（F02 留在外面），P2 再進 L01 碰 F02 仍通過檢查，得到 X026 變身，F02 還被瞬移回 S03 入口。情境二：P1 用學生證 F03 逃出（F03 留在大門旁），P2 進 L02 能隔空把大門旁的學生證拿走。
  - **做法**：照 TDD。新增 `Source/EscapeGame/Tests/Step6_TransformFix.spec.cpp`（`EscapeGame.Step6Fix`，3 條 It：兩個情境的 Rejected 並斷言物品位置／LocationId／占用者、玩家形態與地點與位置、生命值、已觸發集合都不變；一條正向對照：飲料還在 L01 裡時從 L01 碰得到、是 X004 變身）；既有的 Step6 spec 沒動。紅的時候只有兩個情境失敗。修正只改一行：`Interact` 特殊地點檢查改成 `TargetItem->LocationId == Player->LocationId`。規則本意不變：在特殊地點裡只能碰「目前在該地點裡的物品」和 `SELF`；「待人確認」步驟 6 那項的說法已同步改成「目前在該地點裡的物品」。
  - **測試**：Python 46/46；UE 124/124（原本 121 + 新增 3）。

- **2026-11-01 步驟 9 灰盒關卡**：9a 紅 commit `3d62bd0`（測試與空殼）、9b 實作到全綠。
  - **C++ 類別**：`Core/EscapeAreaVolume`（Box 是 Root，單位方塊 extent 50，`SetCollisionEnabled(QueryOnly)` + 所有通道 Overlap；`ContainsPoint` 把點轉到盒子本地座標（`InverseTransformPositionNoScale`）再和 `GetScaledBoxExtent()` 比，所以 Actor 縮放與旋轉都算進去）。`Forms/EscapeObjectActor`（Mesh 是 Root，`ConstructorHelpers` 載入 `/Engine/BasicShapes/Cube`，`BlockAll`；`SetDoorOpen(true)` = `SetActorHiddenInGame(true)` + `NoCollision`，`false` = 顯示 + `QueryAndPhysics`）。`Forms/EscapeFormPawn`（Mesh Root，同樣預設 Cube、`Pawn` 碰撞）。尺寸都用 Actor 縮放：`Scale = SX／SY／SZ ÷ 100`，所以 CSV 的尺寸直接對應。
  - **Level_Layout.csv（83 列）**：Area 8、Floor 10（含 2 個 L01／L02 天花板）、Wall 30、Door 3、Object 31（F01～F12、S01～S20 去掉三扇門、S11 三個）、Spawn 1。地板頂面 Z=0（地板方塊 Z=-10、厚 20，連牆下一起鋪，門洞底下也有地板），區域盒 Z 中心 200、SZ 400，牆厚 20、高 400。平面（X, Y，單位 cm）：Corridor 0～3000 × 0～400（東西向長廊）；Classroom 0～1000 × -1000～-20；Office 1020～1720 × -800～-20；GuardRoom 0～800 × 420～1020；Restroom 1000～1800 × 420～1220；Storage 1000～1400 × 1240～1640（廁所北側）；L01 5000～5400 × 0～400；L02 5000～5400 × 600～1000。門洞：S19 在走廊南牆 X 400～600、S05 X 1280～1480、S09 在廁所／儲物間之間的牆 X 1100～1300；警衛室與廁所的開口在走廊北牆 X 300～500、X 1300～1500。出生點 (500, -600, 98)。
  - **蓋地圖用到的 MCP 工具與參數格式**（都經 `call_tool`，`toolset_name` 前綴 `editor_toolset.toolsets.`；腳本裡用 `execute_tool(前綴 + "scene.SceneTools.xxx", json.dumps(參數))`，回傳 dict，值在 `["returnValue"]`）：`programmatic.ProgrammaticToolset.execute_tool_script(script)`（回傳的 `returnValue` 是 JSON 字串）；`asset.AssetTools.exists(path)`、`duplicate(path, new_path)`、`save_assets(asset_paths=[...])`；`scene.SceneTools.load_level(level_path)`、`get_current_level()`、`find_actors(name, tag, collision_channels, actor_type?)`（`name`、`tag`、`collision_channels` 都要給，空字串／空陣列即可；`actor_type={"refPath":"/Script/Engine.PlayerStart"}`）、`get_folders()`、`get_actors_in_folder(folder_path, recursive)`、`remove_from_scene(actor)`、`add_to_scene_from_asset(asset_path="/Engine/BasicShapes/Cube", name, xform)`、`add_to_scene_from_class(actor_type={"refPath":"/Script/EscapeGame.EscapeObjectActor"}, name, xform)`、`set_actor_folder(actor, folder_path)`；`actor.ActorTools.set_label(actor, label)`、`set_actor_transform(actor, xform, worldspace)`、`get_label`；`object.ObjectTools.set_properties(instance, values=<JSON 字串>)`（屬性名稱就是 C++ 的名稱：`AreaId`、`ObjectId`、`bIsDoor`）、`get_properties(instance, properties=[...])`。Actor 參數是 `{"refPath": "..."}`（工具回傳的就是這個格式，直接傳回去）；`xform = {"location":{x,y,z}, "rotation":{pitch,yaw,roll}, "scale":{x,y,z}}`。
  - **`tools/mcp_build_graybox.py`**（標準函式庫，`python3 -I` 可跑，只能連 `127.0.0.1`，其他主機直接結束）：自己實作 MCP 的 `initialize` → `notifications/initialized` → `tools/call`，把 CSV 變成 `ROWS` 接在腳本字串前面，整段送 `execute_tool_script`。流程：地圖不存在 → 複製 `/Engine/Maps/Templates/Template_Default` 並先存檔；`load_level`；刪掉資料夾 `Graybox` 底下所有 Actor 與範本的 `Floor`；照 CSV 放（Area → `Area_<Id>`、Floor／Wall → CSV 的 Id、Object／Door → Id、S11 → `S11_1`～`S11_3`；資料夾 `Graybox/{Areas,Floors,Walls,Doors,Objects/<區域>,Spawn}`）；Spawn 列用範本原有的 PlayerStart（沒有才新增、多的刪掉）；`save_assets`。重跑驗證：第二次 `removed: 83`、`placed` 與第一次相同、PlayerStart 仍然 1 個。腳本印出 JSON 報告，`saved` 與 `player_starts == 1` 才回傳 0。`--print-script` 只印要送的腳本。
  - **偏離 PLANS 架構的地方（原因都是 MCP 的實際行為）**：(1) 「已存在就先刪」做不到：`AssetTools.delete` 回傳 true、記憶體裡的 World 被 Force Delete，但磁碟上的 `.umap` 還在、`exists` 仍是 true（連試兩次）。改成「已存在就載入，清掉 `Graybox` 資料夾再重蓋」，結果一樣。(2) `duplicate` 出來的地圖只在記憶體裡、是 dirty，馬上 `load_level` 會報 `the level has unsaved changes`；要先 `save_assets` 再載入。(3) 範本自己的 PlayerStart 直接拿來用，不另外刪除再新增（`PlayerStart` 剛好一個）。(4) 沒有用到 `ActorTools.set_actor_transform` 以外的搬動工具；`add_to_scene_*` 的 `xform` 直接帶縮放。
  - **遇到的問題**：(1) 第一次跑時 `get_actors_in_folder("Graybox")` 對不存在的資料夾會丟錯 → 先查 `get_folders()`（為這個多一個存檔點 commit）。(2) 探索時用複製出來的 `Lvl_Probe` 試過一次存檔，確認 `.umap` 單檔約 190 KB、沒有產生 `__ExternalActors__`（Actor 存在地圖裡，符合「不是 World Partition」）；探索用的 `Lvl_Probe.umap` 已手動刪掉，沒有進 repo。(3) 載入引擎範本地圖 `load_level("/Engine/Maps/Templates/Template_Default")` 可行，但最後沒用到。
  - **MCP**：實際跑了 3 次 `mcp_build_graybox.py`（第 1 次因 Graybox 資料夾不存在失敗，沒有放任何 Actor；第 2 次蓋成功；第 3 次重跑驗證相同）＋ 約 10 次探索用的小呼叫（`describe_toolset`、複製／載入／放 Actor／`set_properties` 試用）。headless 開編輯器（`-nullrhi`），約 15 秒後 `127.0.0.1:8000` 可連；`ss -ltn` 只看到 `127.0.0.1:8000`（另有 `0.0.0.0:1985` 是 UE Trace 控制埠，已知，編輯器關掉後約 20 秒消失；不是 MCP）。編輯器 RSS 約 3.1 GB。每次 MCP 任務前都先 commit（`970efb2`、`3380df8`、`a3478c5` 三個存檔點）。一次完整重蓋約 2.6 秒。
  - **`DefaultEngine.ini`**：`GameDefaultMap`、`EditorStartupMap` 改成 `/Game/Maps/Lvl_Academy.Lvl_Academy`；`GlobalDefaultGameMode` 仍是 `BP_ThirdPersonGameMode`。
  - **時間／測試**：實作＋MCP 約 40 分鐘。整個 `tools/run_tests.sh` 約 22 秒（含增量編譯與 UE 測試）。最後 Python 56/56（新增 10 條）；UE 132/132（新增 8 條：VERIFY 5 + PLANS 詮釋 3）。紅的時候 `Lvl_Academy` 不存在，spec 8 條都因「找不到地圖」失敗；實作第一次就全綠，沒有改過測試與 spec。
