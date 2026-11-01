#!/usr/bin/env node
// 把 Sketchfab 原始下載壓成遊戲用的單一 GLB：assets/models/items/fNN.glb。
// 來源優先用 assets/models/raw/fNN.glb，沒有才用 assets/models/fNN/scene.gltf。只保留位置、法線、UV0、索引和底色貼圖；
// 貼圖縮到 512px。需要 ImageMagick 的 `magick`。開發用工具，不屬於遊戲執行階段。
// 用法：node scripts/pack-item-models.mjs [F01 F05 ...]

import { readFile, writeFile, mkdir, mkdtemp, rm, access } from 'node:fs/promises';
import { execFileSync } from 'node:child_process';
import { tmpdir } from 'node:os';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', 'assets', 'models');
const OUT = path.join(ROOT, 'items');
const MAX_TEXTURE = 512;
const COMPONENTS = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4 };
const TYPED = { 5120: Int8Array, 5121: Uint8Array, 5122: Int16Array, 5123: Uint16Array, 5125: Uint32Array, 5126: Float32Array };

const pad4 = (n) => (n + 3) & ~3;

function readAccessor(gltf, bin, index) {
  const acc = gltf.accessors[index];
  const view = gltf.bufferViews[acc.bufferView];
  const Type = TYPED[acc.componentType];
  const size = COMPONENTS[acc.type];
  const stride = view.byteStride ?? size * Type.BYTES_PER_ELEMENT;
  const start = (view.byteOffset ?? 0) + (acc.byteOffset ?? 0);
  const out = new Type(acc.count * size);
  const dv = new DataView(bin.buffer, bin.byteOffset, bin.byteLength);
  const getter = { 5120: 'getInt8', 5121: 'getUint8', 5122: 'getInt16', 5123: 'getUint16', 5125: 'getUint32', 5126: 'getFloat32' }[acc.componentType];
  for (let i = 0; i < acc.count; i++) {
    for (let c = 0; c < size; c++) {
      out[i * size + c] = dv[getter](start + i * stride + c * Type.BYTES_PER_ELEMENT, true);
    }
  }
  return { array: out, acc, size };
}

// 讀取原始模型，回傳 { gltf, bin, imageFile(index, tmp) }；imageFile 會回傳可給 magick 的檔案路徑。
async function loadSource(id) {
  const name = id.toLowerCase();
  const rawGlb = path.join(ROOT, 'raw', `${name}.glb`);
  if (await access(rawGlb).then(() => true, () => false)) {
    const buf = await readFile(rawGlb);
    const view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
    if (view.getUint32(0, true) !== 0x46546c67 || view.getUint32(4, true) !== 2) throw new Error(`${id}：不是有效的 GLB 2.0`);
    let offset = 12, gltf = null, bin = null;
    while (offset < buf.byteLength) {
      const len = view.getUint32(offset, true), type = view.getUint32(offset + 4, true);
      offset += 8;
      if (type === 0x4e4f534a) gltf = JSON.parse(buf.subarray(offset, offset + len).toString('utf8'));
      if (type === 0x004e4942) bin = new Uint8Array(buf.buffer, buf.byteOffset + offset, len);
      offset += len;
    }
    if (!gltf || !bin) throw new Error(`${id}：GLB 缺少資料區塊`);
    const imageFile = async (index, tmp) => {
      const image = gltf.images[index];
      if (image.bufferView === undefined) throw new Error(`${id}：GLB 含外部貼圖`);
      const v = gltf.bufferViews[image.bufferView];
      const file = path.join(tmp, `src${index}${image.mimeType === 'image/png' ? '.png' : '.jpg'}`);
      await writeFile(file, bin.subarray(v.byteOffset ?? 0, (v.byteOffset ?? 0) + v.byteLength));
      return file;
    };
    return { gltf, bin, imageFile };
  }
  const dir = path.join(ROOT, name);
  const gltf = JSON.parse(await readFile(path.join(dir, 'scene.gltf'), 'utf8'));
  const bin = new Uint8Array(await readFile(path.join(dir, gltf.buffers[0].uri)));
  const imageFile = async (index) => path.join(dir, decodeURIComponent(gltf.images[index].uri));
  return { gltf, bin, imageFile };
}

function shrinkImage(src, dest, keepAlpha) {
  execFileSync('magick', [src, '-resize', `${MAX_TEXTURE}x${MAX_TEXTURE}>`, ...(keepAlpha ? [] : ['-background', 'white', '-alpha', 'remove', '-quality', '82']), dest]);
}

async function pack(id) {
  const { gltf, bin, imageFile } = await loadSource(id);
  const tmp = await mkdtemp(path.join(tmpdir(), 'pack-'));

  const chunks = [];
  let length = 0;
  const bufferViews = [];
  const accessors = [];
  const push = (bytes, target) => {
    const offset = length;
    chunks.push(bytes);
    const padded = pad4(bytes.byteLength);
    if (padded > bytes.byteLength) chunks.push(new Uint8Array(padded - bytes.byteLength));
    length += padded;
    bufferViews.push({ buffer: 0, byteOffset: offset, byteLength: bytes.byteLength, ...(target ? { target } : {}) });
    return bufferViews.length - 1;
  };
  const addAccessor = ({ array, acc, size }, type, componentType, target, withBounds) => {
    const view = push(new Uint8Array(array.buffer, array.byteOffset, array.byteLength), target);
    const entry = { bufferView: view, componentType, count: acc.count, type };
    if (withBounds) { entry.min = acc.min; entry.max = acc.max; }
    accessors.push(entry);
    return accessors.length - 1;
  };

  // 材質：只留底色（含 spec-gloss 的 diffuse）。
  const imageOf = new Map(); // gltf texture index -> 新 images index
  const images = [];
  const textures = [];
  const samplers = gltf.samplers ?? [];
  const textureFor = async (texIndex, opaque) => {
    if (imageOf.has(texIndex)) return imageOf.get(texIndex);
    const tex = gltf.textures[texIndex];
    const srcFile = await imageFile(tex.source, tmp);
    const out = path.join(tmp, `t${texIndex}.${opaque ? 'jpg' : 'png'}`);
    shrinkImage(srcFile, out, !opaque);
    const view = push(new Uint8Array(await readFile(out)));
    images.push({ bufferView: view, mimeType: opaque ? 'image/jpeg' : 'image/png' });
    textures.push({ source: images.length - 1, ...(tex.sampler !== undefined ? { sampler: tex.sampler } : {}) });
    imageOf.set(texIndex, textures.length - 1);
    return textures.length - 1;
  };
  const materials = [];
  for (const m of gltf.materials ?? []) {
    const sg = m.extensions?.KHR_materials_pbrSpecularGlossiness;
    const base = sg ? { baseColorFactor: sg.diffuseFactor, baseColorTexture: sg.diffuseTexture } : (m.pbrMetallicRoughness ?? {});
    const opaque = (m.alphaMode ?? 'OPAQUE') === 'OPAQUE';
    const pbr = { metallicFactor: 0, roughnessFactor: 0.8 };
    if (base.baseColorFactor) pbr.baseColorFactor = base.baseColorFactor;
    if (base.baseColorTexture) pbr.baseColorTexture = { index: await textureFor(base.baseColorTexture.index, opaque), texCoord: base.baseColorTexture.texCoord ?? 0 };
    materials.push({
      name: m.name, pbrMetallicRoughness: pbr, doubleSided: m.doubleSided ?? false,
      ...(m.alphaMode && m.alphaMode !== 'OPAQUE' ? { alphaMode: m.alphaMode, alphaCutoff: m.alphaCutoff } : {}),
    });
  }

  const meshes = gltf.meshes.map((mesh) => ({
    name: mesh.name,
    primitives: mesh.primitives.filter((p) => (p.mode ?? 4) === 4).map((p) => {
      const attributes = {};
      attributes.POSITION = addAccessor(readAccessor(gltf, bin, p.attributes.POSITION), 'VEC3', 5126, 34962, true);
      if (p.attributes.NORMAL !== undefined) attributes.NORMAL = addAccessor(readAccessor(gltf, bin, p.attributes.NORMAL), 'VEC3', 5126, 34962);
      if (p.attributes.TEXCOORD_0 !== undefined) {
        const uv = readAccessor(gltf, bin, p.attributes.TEXCOORD_0);
        if (uv.acc.componentType !== 5126) throw new Error(`${id}：UV 不是 float，未支援`);
        attributes.TEXCOORD_0 = addAccessor(uv, 'VEC2', 5126, 34962);
      }
      const out = { attributes, ...(p.material !== undefined ? { material: p.material } : {}) };
      if (p.indices !== undefined) {
        const idx = readAccessor(gltf, bin, p.indices);
        const array = Uint32Array.from(idx.array);
        out.indices = addAccessor({ array, acc: idx.acc, size: 1 }, 'SCALAR', 5125, 34963);
      }
      return out;
    }),
  }));

  const nodes = gltf.nodes.map((n) => {
    const { name, children, matrix, translation, rotation, scale, mesh } = n;
    return Object.fromEntries(Object.entries({ name, children, matrix, translation, rotation, scale, mesh }).filter(([, v]) => v !== undefined));
  });

  const json = {
    asset: { version: '2.0', generator: 'EscapeGame pack-item-models' },
    scene: 0, scenes: [{ nodes: gltf.scenes[gltf.scene ?? 0].nodes }], nodes, meshes, materials,
    ...(textures.length ? { textures, images, samplers } : {}),
    accessors, bufferViews, buffers: [{ byteLength: length }],
  };
  const jsonBytes = new TextEncoder().encode(JSON.stringify(json));
  const jsonPadded = new Uint8Array(pad4(jsonBytes.length)).fill(0x20);
  jsonPadded.set(jsonBytes);
  const binBytes = new Uint8Array(length);
  let at = 0;
  for (const c of chunks) { binBytes.set(c, at); at += c.byteLength; }
  const total = 12 + 8 + jsonPadded.length + 8 + binBytes.length;
  const glb = new DataView(new ArrayBuffer(total));
  glb.setUint32(0, 0x46546c67, true); glb.setUint32(4, 2, true); glb.setUint32(8, total, true);
  glb.setUint32(12, jsonPadded.length, true); glb.setUint32(16, 0x4e4f534a, true);
  new Uint8Array(glb.buffer).set(jsonPadded, 20);
  const binAt = 20 + jsonPadded.length;
  glb.setUint32(binAt, binBytes.length, true); glb.setUint32(binAt + 4, 0x004e4942, true);
  new Uint8Array(glb.buffer).set(binBytes, binAt + 8);

  await mkdir(OUT, { recursive: true });
  const file = path.join(OUT, `${id.toLowerCase()}.glb`);
  await writeFile(file, new Uint8Array(glb.buffer));
  await rm(tmp, { recursive: true, force: true });
  console.log(`${id}：${(total / 1024).toFixed(0)} KB → ${path.relative(process.cwd(), file)}`);
}

const ids = process.argv.slice(2).map((s) => s.toUpperCase());
for (const id of ids.length ? ids : Array.from({ length: 12 }, (_, i) => `F${String(i + 1).padStart(2, '0')}`)) {
  try { await pack(id); } catch (err) { console.error(`${id}：${err.message}`); process.exitCode = 1; }
}
