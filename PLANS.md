# PLANS — 逃離學院（Three.js）

> `threejs-dev` 遷移計畫。程式修改以規則模組、畫面整合、離線檢查、清理為原子單位。

## 步驟
- [ ] **1 文件與驗收條件**：將專案文件改為 Three.js 靜態網頁方案 → VERIFY STEP 0、5。
- [ ] **2 單機規則核心**：移植形態、互動、生命、時間、密碼、場景狀態、結局與成就；Node 內建測試 → STEP 1–3。
- [ ] **3 靜態 Three.js 遊戲**：場景、移動、鏡頭、互動、HUD、對話框、密碼鍵盤、結局及成就 → STEP 4。
- [ ] **4 無網路檢查與收尾**：本機瀏覽器完整路線驗收、離線資源檢查、移除 Unreal 舊檔 → STEP 0、4、5。

## 檔案責任
- `src/core.mjs`：不依賴 DOM 或 Three.js 的規則與遊戲狀態。
- `src/scene.mjs`：Three.js 場景、物件佈置、玩家網格、命中測試與場景更新。
- `src/main.mjs`：DOM UI、鍵盤與滑鼠輸入、連接規則和場景。
- `index.html`、`styles.css`：入口與介面樣式。
- `vendor/three.module.js`：固定版本 Three.js，供離線靜態部署。
- `tests/*.test.mjs`：使用 Node 內建測試工具驗證規則與入口文件。
- `docs/design/`：遊戲設計規格，維持原樣。

## 做法
- Three.js 固定 v0.180.0，本機 ESM；不加入執行階段網路依賴或建置工具。
- 核心規則使用注入的亂數來源和 `tick(dt)`，不讀取系統時間之外的非確定狀態，便於測試。
- 靜態網頁用原生模組，由任一本機或靜態 HTTP 主機供應；不要求伺服器 API。
- 幾何圖形以 Three.js primitive 程式化建立，不增加大型素材包。
- UI 將使用者密碼顯示在輸入框中；遊戲內其他文字只透過 `textContent` 顯示。
- WebGL 是渲染需求；不支援 WebGL 的瀏覽器顯示明確錯誤，規則測試仍可在 Node 執行。

## 調整紀錄
- 2026-11-01：由 Unreal Engine + Pixel Streaming 改為瀏覽器內 Three.js；靜態單人可玩是必要條件，多人伺服器不是必要條件。
