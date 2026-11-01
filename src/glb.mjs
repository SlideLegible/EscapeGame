import * as THREE from '../vendor/three.module.js';

const COMPONENTS = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4, MAT2: 4, MAT3: 9, MAT4: 16 };
const COMPONENT_TYPES = {
  5120: Int8Array,
  5121: Uint8Array,
  5122: Int16Array,
  5123: Uint16Array,
  5125: Uint32Array,
  5126: Float32Array,
};

function samplerValue(value, fallback, choices) {
  return choices[value] ?? fallback;
}

function parseGlb(buffer) {
  const view = new DataView(buffer);
  if (view.getUint32(0, true) !== 0x46546c67 || view.getUint32(4, true) !== 2) {
    throw new Error('教室模型不是有效的 GLB 2.0 檔案');
  }
  let offset = 12;
  let json = null;
  let binary = null;
  while (offset < buffer.byteLength) {
    const length = view.getUint32(offset, true);
    const type = view.getUint32(offset + 4, true);
    offset += 8;
    if (type === 0x4e4f534a) json = JSON.parse(new TextDecoder().decode(new Uint8Array(buffer, offset, length)));
    if (type === 0x004e4942) binary = new Uint8Array(buffer, offset, length);
    offset += length;
  }
  if (!json || !binary) throw new Error('教室模型缺少 GLB 資料區塊');
  return { json, binary };
}

function createModel(json, binary) {
  const readAccessor = (index) => {
    const accessor = json.accessors[index];
    const bufferView = json.bufferViews[accessor.bufferView];
    const ArrayType = COMPONENT_TYPES[accessor.componentType];
    const itemSize = COMPONENTS[accessor.type];
    if (!ArrayType || !itemSize || bufferView.byteStride) throw new Error('教室模型使用了不支援的頂點格式');
    const start = (bufferView.byteOffset ?? 0) + (accessor.byteOffset ?? 0);
    const array = new ArrayType(binary.buffer, binary.byteOffset + start, accessor.count * itemSize);
    return { array, itemSize, normalized: Boolean(accessor.normalized) };
  };

  const images = (json.images ?? []).map(async (image) => {
    if (image.bufferView === undefined) throw new Error('教室模型包含外部貼圖檔');
    const view = json.bufferViews[image.bufferView];
    const start = binary.byteOffset + (view.byteOffset ?? 0);
    const blob = new Blob([binary.buffer.slice(start, start + view.byteLength)], { type: image.mimeType });
    const url = URL.createObjectURL(blob);
    const bitmap = new Image();
    bitmap.src = url;
    await bitmap.decode();
    URL.revokeObjectURL(url);
    return bitmap;
  });

  const textures = (json.textures ?? []).map((source) => {
    const sampler = json.samplers?.[source.sampler ?? 0] ?? {};
    const texture = new THREE.Texture();
    texture.userData.imageIndex = source.source;
    texture.userData.sampler = sampler;
    texture.flipY = false;
    texture.wrapS = samplerValue(sampler.wrapS, THREE.ClampToEdgeWrapping, {
      10497: THREE.RepeatWrapping, 33648: THREE.MirroredRepeatWrapping, 33071: THREE.ClampToEdgeWrapping,
    });
    texture.wrapT = samplerValue(sampler.wrapT, THREE.ClampToEdgeWrapping, {
      10497: THREE.RepeatWrapping, 33648: THREE.MirroredRepeatWrapping, 33071: THREE.ClampToEdgeWrapping,
    });
    texture.magFilter = samplerValue(sampler.magFilter, THREE.LinearFilter, {
      9728: THREE.NearestFilter, 9729: THREE.LinearFilter,
    });
    texture.minFilter = samplerValue(sampler.minFilter, THREE.LinearMipmapLinearFilter, {
      9728: THREE.NearestFilter, 9729: THREE.LinearFilter, 9984: THREE.NearestMipmapNearestFilter,
      9985: THREE.LinearMipmapLinearFilter, 9986: THREE.NearestMipmapNearestFilter, 9987: THREE.LinearMipmapLinearFilter,
    });
    return texture;
  });

  const materials = (json.materials ?? []).map((entry) => {
    const pbr = entry.pbrMetallicRoughness ?? {};
    const material = new THREE.MeshStandardMaterial({
      name: entry.name ?? '教室材質',
      roughness: pbr.roughnessFactor ?? 1,
      metalness: pbr.metallicFactor ?? 1,
      side: entry.doubleSided ? THREE.DoubleSide : THREE.FrontSide,
      transparent: entry.alphaMode === 'BLEND',
      alphaTest: entry.alphaMode === 'MASK' ? (entry.alphaCutoff ?? 0.5) : 0,
      opacity: pbr.baseColorFactor?.[3] ?? 1,
    });
    if (pbr.baseColorFactor) material.color.setRGB(...pbr.baseColorFactor.slice(0, 3));
    if (entry.emissiveFactor) material.emissive.setRGB(...entry.emissiveFactor);
    const assignTexture = (slot, info, colorSpace = THREE.NoColorSpace) => {
      if (!info) return;
      const texture = textures[info.index];
      texture.channel = info.texCoord ?? 0;
      texture.colorSpace = colorSpace;
      material[slot] = texture;
    };
    assignTexture('map', pbr.baseColorTexture, THREE.SRGBColorSpace);
    assignTexture('metalnessMap', pbr.metallicRoughnessTexture);
    assignTexture('roughnessMap', pbr.metallicRoughnessTexture);
    assignTexture('normalMap', entry.normalTexture);
    assignTexture('aoMap', entry.occlusionTexture);
    assignTexture('emissiveMap', entry.emissiveTexture, THREE.SRGBColorSpace);
    if (entry.normalTexture) material.normalScale.setScalar(entry.normalTexture.scale ?? 1);
    if (entry.occlusionTexture) material.aoMapIntensity = entry.occlusionTexture.strength ?? 1;
    return material;
  });

  const meshes = (json.meshes ?? []).map((entry) => entry.primitives.map((primitive) => {
    if ((primitive.mode ?? 4) !== 4) throw new Error('教室模型使用了不支援的繪圖模式');
    const geometry = new THREE.BufferGeometry();
    for (const [semantic, accessorIndex] of Object.entries(primitive.attributes)) {
      const attributeName = semantic === 'POSITION' ? 'position'
        : semantic === 'NORMAL' ? 'normal'
          : semantic === 'TANGENT' ? 'tangent'
            : semantic.startsWith('TEXCOORD_') ? `uv${semantic.slice(9) === '0' ? '' : semantic.slice(9)}`
              : null;
      if (attributeName) {
        const attribute = readAccessor(accessorIndex);
        geometry.setAttribute(attributeName, new THREE.BufferAttribute(attribute.array, attribute.itemSize, attribute.normalized));
      }
    }
    if (primitive.indices !== undefined) {
      const indices = readAccessor(primitive.indices);
      geometry.setIndex(new THREE.BufferAttribute(indices.array, indices.itemSize, indices.normalized));
    }
    geometry.computeBoundingSphere();
    const material = materials[primitive.material] ?? new THREE.MeshStandardMaterial({ color: 0xffffff });
    const mesh = new THREE.Mesh(geometry, material);
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    return mesh;
  }));

  const makeNode = (index) => {
    const source = json.nodes[index];
    const node = new THREE.Group();
    node.name = source.name ?? `節點 ${index}`;
    if (source.matrix) {
      node.matrix.fromArray(source.matrix);
      node.matrixAutoUpdate = false;
    } else {
      if (source.translation) node.position.fromArray(source.translation);
      if (source.rotation) node.quaternion.fromArray(source.rotation);
      if (source.scale) node.scale.fromArray(source.scale);
    }
    if (source.mesh !== undefined) {
      for (const mesh of meshes[source.mesh]) node.add(mesh);
    }
    for (const child of source.children ?? []) node.add(makeNode(child));
    return node;
  };

  return Promise.all(images).then((decodedImages) => {
    for (const texture of textures) {
      texture.image = decodedImages[texture.userData.imageIndex];
      texture.needsUpdate = true;
      delete texture.userData.imageIndex;
      delete texture.userData.sampler;
    }
    const scene = new THREE.Group();
    scene.name = 'Japanese Classroom';
    for (const index of json.scenes?.[json.scene ?? 0]?.nodes ?? []) scene.add(makeNode(index));
    return scene;
  });
}

export async function loadGlb(url) {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`教室模型載入失敗（${response.status}）`);
  const { json, binary } = parseGlb(await response.arrayBuffer());
  if (json.extensionsRequired?.length) throw new Error('教室模型需要未支援的 GLB 擴充功能');
  return createModel(json, binary);
}
