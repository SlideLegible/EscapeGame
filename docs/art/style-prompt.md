# 固定風格提示詞：日式教室

所有場景、道具和概念圖都用這份文件定下的風格。新增素材（AI 生圖、AI 生 3D、手工建模、貼圖）時，先貼上「通用風格後綴」，再寫要做的物件。

## 參考模型

- 名稱：Japanese Classroom
- 作者：T I A N
- 來源：https://sketchfab.com/3d-models/japanese-classroom-2a1e3b294c1e4e91bed794bfa520c4f4
- 授權：CC BY（Attribution）。遊戲使用這個模型或它的衍生素材時，製作人員名單必須寫出作者、模型名稱和來源連結。
- 本機檔案：`/home/lifestone/creative/assets/japanese-classroom/japanese_classroom.blend`（貼圖已打包；不放進 git）

## 從模型讀出來的數據

分析時間 2026-11-01，Blender 5.2.2 背景模式讀取 .blend。

| 項目 | 數值 |
| --- | --- |
| 物件 | 891 個，網格 421 個 |
| 面數 | 325,960；單一物件中位數 858 三角面，最多為吊燈約 5.8k、置物櫃約 4.3k、每支桌腳約 1.5k |
| 著色 | 約 80% 的面是平滑著色；沒有標記硬邊，沒有修改器 |
| 教室尺寸 | 約 12.8 × 9.3 公尺，天花板高 3.25 公尺 |
| 材質 | 11 個，Stingray PBS（Maya 匯出）接 Principled BSDF，金屬度／粗糙度工作流 |
| 貼圖 | 底色 1024²（少數 512²），金屬粗糙 512²；沒有法線、AO 或髒污貼圖 |
| 燈光 | 模型本身沒有燈光 |
| 主要物件 | 25 組課桌椅（5 × 5）、講桌、黑板、軟木公告板、後方木櫃、置物櫃、暖氣片、吊掛式日光燈、大面窗 |

貼圖實際使用區域的顏色變化很小（每個通道的標準差大約 0.03～0.10）。物件靠清楚的外形和大色塊區分，不靠髒污或細節貼圖。

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

## 核心風格解析

1. **網格與面數**：中模，平滑著色，邊緣有小圓角倒角，不是 low-poly 的銳利硬邊。鋼管家具是光滑圓管，木板和櫃體是乾淨的方塊。新素材可以比原模型更細，但外形要保持簡潔。
2. **材質與著色**：簡化的 PBR（金屬度／粗糙度），不是卡通渲染，也不是無光照。每個材質只用底色加粗糙度，沒有法線細節。沒有描邊，沒有色階分層。動畫感來自配色和柔和光線，不是著色器。
3. **顏色基調**：暖米色、低飽和、高明度、低對比，像日本動畫的教室背景。木頭、牆面、地板是米色到淺褐色；金屬和窗框是中性淺灰；唯一較明顯的顏色是灰綠黑板。
4. **紋理細節**：接近平塗，只有柔和的木紋和很淡的地板拼花。沒有刮痕、灰塵、污漬、塗鴉或使用痕跡。
5. **光線**：明亮、平均，陰影淡而柔。窗戶是發亮的淺色面，不畫窗外景色。

## 通用風格後綴（Universal Style Modifiers）

接在物件描述後面，給生圖或生 3D 工具使用。

```text
stylized realistic 3D game asset, Japanese anime background art style, clean mid-to-high poly mesh, smooth shading with soft rounded bevels, simple PBR materials, base color and roughness only, no normal map detail, low-saturation warm pastel palette, cream and pale pine wood tones, light grey satin metal accents, bright soft diffuse lighting, low contrast, flat clean textures with subtle grain, brand new, spotless, no wear, no dirt, no scratches, no outlines, no cel shading, game-ready, clean quad-based topology, single isolated object, centered, plain neutral background, real-world scale
```

### 負面提示詞

```text
photorealistic grime, dirt, rust, scratches, stains, worn edges, graffiti, high-frequency normal details, sculpted wrinkles, black ink outlines, toon shading, low-poly faceted, neon, oversaturated colors, high contrast, dark moody lighting, multiple objects, scene background, floor plane, text, logo
```

### 中文版（給人看的設計說明）

> 日本高中教室，動畫背景風的乾淨寫實。整體色調是暖米色牆面、淺松木課桌椅、淺褐色木地板，搭配淺灰鋼管和鋁窗框；唯一較飽和的顏色是灰綠色黑板。低對比、明亮、光線均勻、陰影柔和。物件全新、乾淨、沒有使用痕跡，形狀簡潔、邊角有柔和倒角，平滑著色。材質只用底色和粗糙度，貼圖 1024。

## 範例 Prompt

### 日式雙肩書包（放在走廊或課桌旁）

```text
A Japanese high school student backpack, standing upright as if placed beside a classroom desk, rectangular boxy body with rounded corners, matte muted navy blue fabric (#3E4A5C), soft cream inner trim, neat stitched seams, two padded shoulder straps, top carry handle, single front zip pocket, light grey satin metal zipper pulls and buckles, neatly closed, slightly soft but structured shape, about 30 x 42 x 15 cm, stylized realistic 3D game asset, Japanese anime background art style, clean mid-to-high poly mesh, smooth shading with soft rounded bevels, simple PBR materials, base color and roughness only, no normal map detail, low-saturation warm pastel palette, bright soft diffuse lighting, low contrast, flat clean textures with subtle fabric weave, brand new, spotless, no wear, no dirt, no scratches, no outlines, no cel shading, game-ready, clean quad-based topology, single isolated object, centered, plain neutral background, real-world scale
```

書包的海軍藍不在原模型色票裡，是刻意壓低飽和度的小面積重點色。想更低調可改成深褐色仿皮 `#6B5442`。

### 講義與教鞭組合（放在講台上）

```text
A tidy set of classroom teacher props for a lectern top, one neat stack of white A4 printed handouts with slightly offset sheets and faint grey text lines, a closed attendance book with a pale sage cover, a slim telescoping teacher's pointer stick in pale pine wood (#F5D8AA) with light grey satin metal tip, a small open box of white chalk sticks, arranged together on one compact flat base footprint of about 50 x 35 cm, no lectern, no table, stylized realistic 3D game asset, Japanese anime background art style, clean mid-to-high poly mesh, smooth shading with soft rounded bevels, simple PBR materials, base color and roughness only, no normal map detail, low-saturation warm pastel palette, cream and pale pine wood tones, bright soft diffuse lighting, low contrast, flat clean textures, brand new, spotless, no wear, no dirt, no outlines, no cel shading, game-ready, clean quad-based topology, single isolated prop group, centered, plain neutral background, real-world scale
```

模型已經有講桌，所以寫明 `no lectern, no table`，只生成桌面上的道具。

## 套用到其他場景

BRIEF.md 列出的其他場景沿用同一套規則：通用風格後綴不改，只在前面寫場景或物件描述。

| 場景 | 寫在後綴前面的描述 | 注意 |
| --- | --- | --- |
| 走廊 | `school hallway, shoe lockers, long row of windows, linoleum-like pale floor` | 地板可以改成淺灰米色，但仍要低對比 |
| 辦公室 | `teachers' office, grey steel desks, stacked documents neatly arranged, filing cabinets` | 文件要整齊，不要雜亂 |
| 廁所 | `school restroom, pale tiles, white porcelain, steel partitions` | 磁磚用米白或淺灰，不能髒 |
| 儲物間 | `storage room, steel shelves, cardboard boxes, cleaning tools` | 允許比教室多一點物件，但仍乾淨 |
| 警衛室 | `security booth, small desk, monitor, key cabinet` | 螢幕不要發強光 |
| 飲料機 | `Japanese vending machine, cream and grey body, rows of drink cans` | 飲料罐可以有較多顏色，但飽和度要壓低 |

## 檢查清單

新素材放進場景前，對照這幾點：

- [ ] 主色在上面的色票範圍內，飽和色只出現在小面積的重點物件
- [ ] 沒有髒污、刮痕、塗鴉
- [ ] 對比低、陰影柔和，放在教室裡不會跳出來
- [ ] 平滑著色、邊緣有柔和倒角，沒有描邊或色階
- [ ] 材質只用底色與粗糙度；金屬只用在鋼管、框架、燈具、扣具
- [ ] 貼圖 1024² 以內
- [ ] 面數可以比原模型細；若在瀏覽器裡載入明顯變慢，用 Blender 的 Decimate 減面
