#!/usr/bin/env node
// 下載 12 個道具模型到 assets/models/raw/（優先抓原始 .glb，沒有才抓 glTF 壓縮包到 assets/models/fNN/）。
// 用法：SKETCHFAB_TOKEN=你的token node scripts/download-sketchfab-models.mjs [F01 F05 ...]
// token 只從環境變數讀取，不寫進檔案。已存在的模型會跳過。
// 這是開發用素材下載工具，不屬於遊戲執行階段。

import { mkdir, writeFile, access, readdir, rm } from 'node:fs/promises';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const MODELS = {
  F01: '4e523176ab2e42aa80ced9bfe95e5c64', // 飲料罐垃圾
  F02: '4e0be610db9646929d8be84491f1a72f', // 飲料
  F03: '402f58b3cd144662a56b63906875e2c9', // 學生證
  F04: 'd3e520bcda634cbf95c34ca03391d0a0', // 辦公室鑰匙
  F05: '094cd4d0012e467f9e64425c723401da', // 不知用途的鑰匙
  F06: 'f858428c790f43f8ba9a3aa2996b88b0', // 期中考考卷
  F07: 'f40598775b354274929424d33ba9fb59', // 長竿子
  F08: '4d6b7113e5924ee0b75749134204539a', // 骰子
  F09: '2e964ac0242e4b1789adfd9549c653dc', // 時鐘
  F10: '2273be0837bc453b982dfb19d82c9cc0', // 監視器
  F11: 'e8caa78fc4c442cdb671004b716c45cd', // 一大串鑰匙圈
  F12: '4d0e98f0ed7443d7bccc5e075bfeda36', // 椅子
};

const API = 'https://api.sketchfab.com/v3';
const OUT_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', 'assets', 'models');
const ATTRIBUTION_FILE = path.join(OUT_DIR, 'ATTRIBUTION.sketchfab.txt');

const token = process.env.SKETCHFAB_TOKEN;
if (!token) {
  console.error('請設定環境變數 SKETCHFAB_TOKEN。');
  process.exit(1);
}
const headers = { Authorization: `Token ${token}` };

const exists = (p) => access(p).then(() => true, () => false);

async function getJson(url) {
  const res = await fetch(url, { headers });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}：${url}`);
  return res.json();
}

async function download(url, dest) {
  // 預簽名網址，不需再帶 Authorization。
  const res = await fetch(url);
  if (!res.ok) throw new Error(`下載失敗 ${res.status}：${dest}`);
  await writeFile(dest, Buffer.from(await res.arrayBuffer()));
}

async function fetchOne(id, uid) {
  const target = path.join(OUT_DIR, id.toLowerCase());
  const rawFile = path.join(OUT_DIR, 'raw', `${id.toLowerCase()}.glb`);
  if (await exists(target) || await exists(rawFile)) {
    console.log(`${id}：已存在，跳過`);
    return null;
  }
  const [meta, dl] = await Promise.all([
    getJson(`${API}/models/${uid}`),
    getJson(`${API}/models/${uid}/download`),
  ]);
  const glb = dl.glb;
  const gltf = dl.gltf;
  if (glb?.url) {
    console.log(`${id}：${meta.name}（glb，${(glb.size / 1024).toFixed(0)} KB）`);
    await mkdir(path.dirname(rawFile), { recursive: true });
    await download(glb.url, rawFile);
    console.log(`  → assets/models/raw/${id.toLowerCase()}.glb`);
  } else if (gltf?.url) {
    console.log(`${id}：${meta.name}（無 glb，改抓 glTF，${(gltf.size / 1024).toFixed(0)} KB）`);
    const zip = `${target}.zip`;
    await mkdir(target, { recursive: true });
    try {
      await download(gltf.url, zip);
      execFileSync('unzip', ['-q', '-o', zip, '-d', target]);
    } catch (err) {
      await rm(target, { recursive: true, force: true });
      throw err;
    } finally {
      await rm(zip, { force: true });
    }
    const files = await readdir(target);
    console.log(`  → assets/models/${id.toLowerCase()}/（${files.length} 項）`);
  } else {
    throw new Error(`${id}：此模型沒有 glb 或 glTF 下載（可能未開放下載）`);
  }

  const license = meta.license?.label ?? '未知';
  const author = meta.user?.displayName ?? meta.user?.username ?? '未知';
  return `${id} | ${meta.name} | ${author} | ${license} | ${meta.viewerUrl}\n`;
}

const wanted = process.argv.slice(2).map((s) => s.toUpperCase());
const ids = wanted.length ? wanted : Object.keys(MODELS);
for (const id of ids) {
  if (!MODELS[id]) { console.error(`未知道具編號：${id}`); process.exit(1); }
}

await mkdir(OUT_DIR, { recursive: true });
let failed = 0;
for (const id of ids) {
  try {
    const line = await fetchOne(id, MODELS[id]);
    if (line) await writeFile(ATTRIBUTION_FILE, line, { flag: 'a' });
  } catch (err) {
    failed += 1;
    console.error(`${id}：${err.message}`);
  }
}
console.log(failed ? `完成，${failed} 個失敗。` : '全部完成。');
process.exit(failed ? 1 : 0);
