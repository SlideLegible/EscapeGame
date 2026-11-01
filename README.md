# 逃離學院

繁體中文第三人稱密室逃脫網頁遊戲，以 Three.js 0.180.0 執行。規則核心可單獨測試，遊戲本身可由靜態主機提供，無須伺服器 API 或建置流程。

## 開始遊玩

在專案根目錄執行：

```sh
python -m http.server 8000
```

瀏覽器開啟 <http://localhost:8000>。需要支援 WebGL 的現代瀏覽器。

## 測試

```sh
node --test
```

使用 Node.js 內建測試執行器，不需安裝 npm 套件。

## 操作

- `W A S D`：移動
- 滑鼠拖曳：環繞鏡頭
- `E`：與最近的物件互動
- `Q`：變回人類
- `Tab` 或右上角「成就」：查看本次遊玩已解鎖的成就
- 點選提示按鈕：開始遊戲並啟用滑鼠控制

## 文件

- [BRIEF.md](BRIEF.md)：目標與範圍
- [VERIFY.md](VERIFY.md)：可重現的驗收條件
- [PLANS.md](PLANS.md)：遷移計畫與架構
- [docs/design/escape_room_script.md](docs/design/escape_room_script.md)：劇本和關卡
- [docs/design/item_interactions.md](docs/design/item_interactions.md)：形態與互動表
