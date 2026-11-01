# AGENTS.md — 專案規則

## 必須遵守
1. 依 BRIEF.md 與 docs/design/ 實作，不擴大遊戲範圍。
2. 一項功能一個清楚的 commit；commit subject 用精簡英文。
3. 每次變更後跑 `node --test`。不得為了通過而放寬測試。
4. 核心規則放在 `src/core.mjs`，不得依賴 Three.js、DOM、網路或儲存裝置。
5. 靜態遊戲不得要求建置、帳號、伺服器或網路連線；字型、程式和素材都由本機提供。
6. 遊戲文字使用 `textContent`，不把使用者輸入插入 HTML。
7. 不使用 pnpm、套件快取或會產生大量重複檔案的指令。
8. `docs/design/` 是玩法規格來源；除非人明確要求，勿改寫遊戲規則。

## 專案指令
- 全部測試：`node --test`
- 本機預覽：`python -m http.server 8000`，開啟 `http://localhost:8000`
- 無執行階段套件安裝；Three.js 版本固定為 0.180.0 並隨專案提供。
- `src/core.mjs`：單機遊戲規則、狀態和互動解析。
- `src/scene.mjs`、`src/main.mjs`：Three.js 場景、輸入、畫面和 DOM。
- `tests/*.test.mjs`：使用 Node.js 內建測試工具。
