#!/usr/bin/env node
// 開發用素材下載工具（不屬於遊戲執行階段）：把場景物件的 Sketchfab 原始 .glb 下載到指定資料夾。
// 用法：SKETCHFAB_API_TOKEN=你的token node scripts/download-scene-objects.mjs <輸出資料夾> [S01 S02 ...]
// token 只從環境變數讀取，不寫進檔案；已存在的檔案會跳過。授權資訊印在標準輸出，供貼進 ATTRIBUTION.sketchfab.txt。

import { mkdir, writeFile, access } from 'node:fs/promises';
import path from 'node:path';

const MODELS = {
  S01: '3f8d274dfee147aa8fc031f965010692', // 學院大門
  S02: 'f61aeec14ff441abb0f6add5485a2e90', // 垃圾桶
  S03: 'f537e142a592428d85ca7a940be221d3', // 飲料機（S04 沿用）
  S06: '958136efb5bc48c6b1bf20dd444a4b84', // 辦公桌
  S07: 'ea9faf1298d24497b916c27a4ea38636', // 電腦
  S10: 'acce0c2e1e5b4986b4ff18198eb90e3e', // 花盆
  S17: '4e2609119dfa478d8b0337b049050d7c', // 公布欄
  S15: 'b5e233fbe3bf49b2a21fd186b07b190b', // 配電箱
  S18: '03df12d6917a406da7a2b3ee333faba6', // 月曆
};

const API = 'https://api.sketchfab.com/v3';
const token = process.env.SKETCHFAB_API_TOKEN ?? process.env.SKETCHFAB_TOKEN;
if (!token) { console.error('請設定環境變數 SKETCHFAB_API_TOKEN。'); process.exit(1); }
const [outDir, ...wanted] = process.argv.slice(2);
if (!outDir) { console.error('用法：node scripts/download-scene-objects.mjs <輸出資料夾> [S01 ...]'); process.exit(1); }
const headers = { Authorization: `Token ${token}` };
const exists = (p) => access(p).then(() => true, () => false);

async function getJson(url) {
  const res = await fetch(url, { headers });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}：${url}`);
  return res.json();
}

const ids = wanted.length ? wanted.map((s) => s.toUpperCase()) : Object.keys(MODELS);
await mkdir(outDir, { recursive: true });
let failed = 0;
for (const id of ids) {
  try {
    if (!MODELS[id]) throw new Error('未知物件編號');
    const dest = path.join(outDir, `${id.toLowerCase()}.glb`);
    const uid = MODELS[id];
    const meta = await getJson(`${API}/models/${uid}`);
    const line = `${id} | ${meta.name} | ${meta.user?.displayName ?? meta.user?.username ?? '未知'} | ${meta.license?.label ?? '未知'} | ${meta.viewerUrl}`;
    if (await exists(dest)) { console.log(`${id}：已存在，跳過`); console.log(line); continue; }
    const dl = await getJson(`${API}/models/${uid}/download`);
    if (!dl.glb?.url) throw new Error('沒有 glb 下載');
    const res = await fetch(dl.glb.url); // 預簽名網址，不帶 Authorization
    if (!res.ok) throw new Error(`下載失敗 ${res.status}`);
    await writeFile(dest, Buffer.from(await res.arrayBuffer()));
    console.log(`${id}：${meta.name}（${(dl.glb.size / 1024).toFixed(0)} KB，${meta.faceCount} 面）`);
    console.log(line);
  } catch (err) {
    failed += 1;
    console.error(`${id}：${err.message}`);
  }
}
process.exit(failed ? 1 : 0);
