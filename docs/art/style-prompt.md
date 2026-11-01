# 固定風格提示詞：日式教室

所有場景、道具和概念圖都用這份文件定下的風格。新增素材（AI 生圖、AI 生 3D、手工建模、貼圖）時，先貼上「固定風格提示詞」，再接要做的物件。

## 參考模型

- 名稱：Japanese Classroom
- 作者：T I A N
- 來源：https://sketchfab.com/3d-models/japanese-classroom-2a1e3b294c1e4e91bed794bfa520c4f4
- 授權：CC BY（Attribution）。遊戲使用這個模型或它的衍生素材時，製作人員名單必須寫出作者、模型名稱和來源連結。
- 本機檔案：`/home/lifestone/creative/assets/japanese-classroom/japanese_classroom.blend`（貼圖已打包；不放進 git）

## 用 Blender MCP 讀出來的數據

分析時間 2026-11-01，Blender 5.2.2，透過 Blender MCP 外掛下載和匯入。

| 項目 | 數值 |
| --- | --- |
| 物件 | 891 個，網格 421 個 |
| 面數 | 325,960 |
| 教室尺寸 | 約 12.8 × 9.3 公尺，天花板高 3.25 公尺 |
| 材質 | 11 個，Stingray PBS（Maya 匯出），金屬度／粗糙度工作流 |
| 貼圖 | 底色 1024²（少數 512²），金屬粗糙 512² |
| 燈光 | 模型本身沒有燈光 |
| 主要物件 | 25 組課桌椅（5 × 5）、講桌、黑板、軟木公告板、後方木櫃、置物櫃、暖氣片、吊掛式日光燈、大面窗 |

貼圖的顏色變化很小（每個通道的標準差大約 0.03～0.10）。物件靠清楚的形狀和大色塊區分，不靠髒污或細節貼圖。

### 色票

以下是從貼圖實際使用區域取平均值得到的 sRGB 色碼。

| 用途 | 色碼 | 粗糙度 | 金屬度 | 說明 |
| --- | --- | --- | --- | --- |
| 課桌桌面、椅面、椅背 | `#F5D8AA` | 低（0.2 左右） | 0 | 淺色松木，看得到柔和木紋 |
| 後方木櫃 | `#F0D3A8` | 0.8 左右 | 0 | 與課桌同色系，較霧面 |
| 講桌、矮櫃 | `#C1A789` | 0.35 左右 | 0 | 略深的木色 |
| 地板 | `#CEAF8E` | 0.4 左右 | 0 | 淺褐色木地板，拼花紋路很淡 |
| 牆面 | `#B2A696` | 0.3 左右 | 0 | 帶暖灰的米褐色 |
| 天花板 | `#DBD4CB` | 中 | 0 | 接近白的暖灰 |
| 門 | `#FEE9C6` | 低 | 0 | 奶油色 |
| 桌腳、椅腳、置物櫃、暖氣片 | `#D4D4D4` | 低（0.2 左右） | 半金屬 | 淺灰鋼管，略有光澤 |
| 窗框、燈具、門把 | `#B4B4B4` | 低（0.2 左右） | 偏金屬 | 中淺灰鋁框 |
| 窗玻璃 | `#C9C9C9` | 0.9 | 0 | 不透明的平塗淺灰，看不到窗外 |
| 黑板 | `#547657` | 霧面 | 0 | 灰綠色，粉筆擦痕很淡 |
| 公告板 | 軟木色，接近 `#E8C99A` | 霧面 | 0 | 細顆粒軟木 |

### 風格歸納

1. **日本動畫背景感**：低對比、偏亮、粉彩化的寫實。像校園動畫或視覺小說的教室背景，不是照片寫實。
2. **暖色主調、冷灰點綴**：木頭、牆面、地板都是米色到淺褐色；金屬和窗框是中性淺灰；唯一的飽和色是黑板的灰綠。
3. **乾淨、整齊、沒有使用痕跡**：沒有刮痕、灰塵、塗鴉、污漬，也沒有掉在地上的雜物。課桌椅排列整齊。
4. **簡潔的形狀**：鋼管家具是單純的圓管，邊角有小倒角；木板是乾淨的平面加木紋貼圖。細節集中在家具和燈具，牆面和天花板平坦。
5. **材質單純**：每種材質只有一張底色貼圖加一張粗糙度貼圖，沒有法線細節、沒有髒污層。
6. **光線柔和平均**：整體明亮，陰影淡而柔，沒有強烈的明暗對比。窗戶是發亮的淺色面，不畫窗外景色。

## 固定風格提示詞

### 英文版（給生圖或生 3D 工具）

```text
Japanese high school classroom style, anime background art look, clean stylized realism,
soft pastel palette, warm beige walls (#B2A696), pale pine wood (#F5D8AA) with soft visible grain,
light tan wooden floor (#CEAF8E), light grey tubular steel frames (#D4D4D4), grey aluminium window frames (#B4B4B4),
muted green chalkboard (#547657) as the only saturated accent,
low contrast, bright even lighting, soft diffuse shadows, overcast daylight through frosted windows,
tidy and spotless, no wear, no dirt, simple clean shapes with small bevels,
mid-poly game asset, simple PBR materials (base color + roughness), 1024px textures
```

### 負面提示詞

```text
photorealistic grime, dirt, rust, scratches, graffiti, clutter, horror, dark, moody, high contrast,
neon, saturated colors, heavy bloom, lens flare, dramatic god rays, film grain,
ornate details, high-frequency normal maps, cartoon outlines, cel shading with black ink lines,
low-poly flat colors, western classroom, carpet, visible outdoor scenery through windows
```

### 中文版（給人看的設計說明）

> 日本高中教室，動畫背景風的乾淨寫實。整體色調是暖米色牆面、淺松木課桌椅、淺褐色木地板，搭配淺灰鋼管和鋁窗框；唯一較飽和的顏色是灰綠色黑板。低對比、明亮、光線均勻、陰影柔和，像陰天從霧面窗戶照進來的光。物件乾淨整齊、沒有使用痕跡，形狀簡潔、邊角有小倒角。中等面數的遊戲素材，材質只用底色和粗糙度，貼圖 1024。

## 套用到其他場景

BRIEF.md 列出的其他場景（走廊、辦公室、廁所、儲物間、警衛室、飲料機內部與底下）沿用同一套規則：固定風格提示詞不改，只在後面加上場景或物件描述。

| 場景 | 接在固定提示詞後面的描述 | 注意 |
| --- | --- | --- |
| 走廊 | `school hallway, shoe lockers, long row of windows, linoleum-like pale floor` | 地板可以改成淺灰米色，但仍要低對比 |
| 辦公室 | `teachers' office, grey steel desks, stacked documents neatly arranged, filing cabinets` | 文件要整齊，不要雜亂 |
| 廁所 | `school restroom, pale tiles, white porcelain, steel partitions` | 磁磚用米白或淺灰，不能髒 |
| 儲物間 | `storage room, steel shelves, cardboard boxes, cleaning tools` | 允許比教室多一點物件，但仍乾淨 |
| 警衛室 | `security booth, small desk, monitor, key cabinet` | 螢幕不要發強光 |
| 飲料機 | `Japanese vending machine, cream and grey body, rows of drink cans` | 飲料罐可以有較多顏色，但飽和度要壓低 |

### 檢查清單

新素材放進場景前，對照這幾點：

- [ ] 主色在上面的色票範圍內，飽和色只出現在小面積的重點物件
- [ ] 沒有髒污、刮痕、塗鴉
- [ ] 對比低、陰影柔和，放在教室裡不會跳出來
- [ ] 材質只用底色與粗糙度；金屬只用在鋼管、框架、燈具
- [ ] 貼圖 1024² 以內
