import * as THREE from '../vendor/three.module.js';
import { FORMS, OBJECTS, OBJECT_AREAS } from './core.mjs';
import { loadGlb } from './glb.mjs';

const ROOM = {
  Classroom: { label: '密碼學教室', x: 0, z: 0, w: 18, d: 12, color: 0x8b7859 },
  Corridor: { label: '走廊', x: 0, z: -11, w: 50, d: 8, color: 0x756b58 },
  Office: { label: '辦公室', x: 11, z: -21, w: 10, d: 10, color: 0x766c58 },
  Restroom: { label: '廁所', x: -11, z: -21, w: 10, d: 10, color: 0x626b66 },
  Storage: { label: '儲物間', x: -21.5, z: -21, w: 11, d: 10, color: 0x5b5244 },
  GuardRoom: { label: '警衛室', x: 23, z: -21, w: 10, d: 10, color: 0x696553 },
  L01: { label: '飲料機內部', x: 38, z: -9, w: 9, d: 8, color: 0x515f58 },
  L02: { label: '飲料機底下', x: 38, z: -21, w: 9, d: 8, color: 0x50564e },
};

export const OBJECT_POSITIONS = {
  S01: [0, -14], S02: [-6, -10], S03: [-14, -10], S04: [-14, -10.7], S05: [11, -15],
  S06: [13, -20], S07: [9, -21], S08: [13, -24], S09: [-16, -21], S10: [4, -10],
  S11: [-12, -19], S12: [-8, -23], S13: [-9, -17], S14: [-7.65, 0], S15: [25, -22],
  S16: [-6.32, 0], S17: [-3, -10], S18: [9, -24], S19: [-5.55, -5.45], S20: [-5.65, 0.35],
  S21: [22.5, -22],
};

const ITEM_POSITIONS = {
  F01: [-4.8, -11], F02: [38, -9], F03: [38, -21], F04: [4.8, -9.3], F05: [12, -20.8],
  F06: [11.8, -24], F07: [-22, -21], F08: [-5.2, 0.9], F09: [-5, 2.2],
  F11: [20.5, -22], F12: [-5, -2.8],
};

// Footprint (x, z) of the teacher's lectern in the bundled classroom, parallel to the blackboard.
export const LECTERN_SIZE = [0.9, 1.48];

// Walkable collision rows measured from the bundled classroom's desk and chair layout.
const CLASSROOM_DESK_COLUMNS = [-3.77, -1.67, 0.41, 2.43, 4.45, 6.5];
const CLASSROOM_DESK_ROWS = [-3.96, -1.96, -0.01, 1.93, 3.89];
const CLASSROOM_EXIT_DOOR_PARTS = ['PUERTA_1_StingrayPBS9_0', 'PUERTA_3_StingrayPBS9_0', 'GRIPPUERTA_StingrayPBS1_0', 'GRIPPUERTA2_StingrayPBS1_0'];
const MODEL_TARGETS = new Set(['S04', 'S14', 'S16', 'S19', 'S20']);

const ROOM_NAMES = Object.fromEntries(Object.entries(ROOM).map(([id, room]) => [id, room.label]));
const palette = { wall: 0xb3a789, trim: 0x51483b, wood: 0x8d704a, light: 0xe8d8b6, dark: 0x292720, lime: 0xc4ef80 };

export function areaAt(x, z) {
  if (Math.abs(x) <= 7.75 && z >= -5.75 && z <= 5.75) return 'Classroom';
  if (x >= 6 && x <= 16 && z >= -26 && z <= -15.8) return 'Office';
  if (x >= -16 && x <= -6 && z >= -26 && z <= -15.8) return 'Restroom';
  if (x >= -27 && x <= -16 && z >= -26 && z <= -16) return 'Storage';
  if (x >= 18 && x <= 28 && z >= -26 && z <= -15.8) return 'GuardRoom';
  if (x >= -25 && x <= 25 && z >= -16.2 && z <= -5.75) return 'Corridor';
  if (x >= 33 && x <= 43 && z >= -14 && z <= -5) return 'L01';
  if (x >= 33 && x <= 43 && z >= -25 && z <= -16) return 'L02';
  return null;
}

export const SPECIAL_EXITS = {
  L01: { at: [34.1, -11.8], to: [-14, -6.9], text: '你從飲料機的出貨口鑽了出來' },
  L02: { at: [34.1, -23.8], to: [-14, -12.2], text: '你從飲料機底下滾了出來' },
};

// Targets behind or beside the camera direction cost extra, so the one the player faces wins.
export function nearestCandidate(x, z, yaw, candidates, maxDistance = 3.7) {
  const fx = Math.sin(yaw), fz = -Math.cos(yaw);
  let best = null;
  let bestScore = Infinity;
  for (const candidate of candidates) {
    const { bounds } = candidate;
    const nx = bounds ? Math.max(candidate.x + bounds.min.x, Math.min(x, candidate.x + bounds.max.x)) : candidate.x;
    const nz = bounds ? Math.max(candidate.z + bounds.min.z, Math.min(z, candidate.z + bounds.max.z)) : candidate.z;
    const distance = Math.hypot(nx - x, nz - z);
    if (distance >= maxDistance) continue;
    const [dx, dz] = distance > 1e-6 ? [nx - x, nz - z] : [candidate.x - x, candidate.z - z];
    const length = Math.hypot(dx, dz);
    const facing = length > 1e-6 ? (dx * fx + dz * fz) / length : 1;
    const score = distance + 0.4 * (1 - facing);
    if (score < bestScore) { bestScore = score; best = { ...candidate, distance }; }
  }
  return best;
}

export function entryPoint(area) {
  return area === 'L01' ? [35.5, -9] : [35.5, -21];
}

export function dropPoint(x, z) {
  if (areaAt(x, z) === 'L01') return [-14 + (x - 38) / 3, -8];
  if (areaAt(x, z) === 'L02') return [-14 + (x - 38) / 3, -11.2];
  return [x, z];
}

export class AcademyScene {
  #onResize;

  constructor(canvas, game) {
    this.canvas = canvas;
    this.game = game;
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x101d20);
    this.scene.fog = new THREE.FogExp2(0x101d20, 0.012);
    this.camera = new THREE.PerspectiveCamera(52, 1, 0.1, 180);
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: false });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.5));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.1;
    this.keys = new Set();
    this.yaw = 0;
    this.pitch = 0.28;
    this.pointerDown = null;
    this.started = false;
    this.playerPosition = new THREE.Vector3(-5.2, 0, -4.5);
    this.humanPosition = this.playerPosition.clone();
    this.itemDrops = new Map();
    this.targetMeshes = [];
    this.targetGroups = new Map();
    this.itemGroups = new Map();
    this.exitGroups = new Map();
    this.roomGroups = new Map();
    this.playerGroup = null;
    this.playerRig = null;
    this.humanModel = null;
    this.playerAnimationTime = 0;
    this.jumpHeight = 0;
    this.jumpVelocity = 0;
    this.walking = false;
    this.classroomExitDoors = [];
    this.playerMaterials = new Map();
    this.cameraTarget = new THREE.Vector3();
    this.cameraDesired = new THREE.Vector3();
    this.mapTarget = new THREE.Vector3(7, 0, -11);
    this.lastCameraUpdate = 0;
    this.#buildLights();
    this.#buildRooms();
    this.#buildObjects();
    this.#buildExits();
    this.#buildPlayer();
    this.playerModelLoad = this.#loadPlayerModel();
    this.classroomLoad = this.#loadClassroomModel();
    this.#onResize = () => this.resize();
    window.addEventListener('resize', this.#onResize);
    this.resize();
    this.refresh();
  }

  #buildLights() {
    this.scene.add(new THREE.HemisphereLight(0xf0dfc0, 0x39352e, 2.25));
    const key = new THREE.DirectionalLight(0xffedcf, 2.2);
    key.position.set(-12, 24, 15);
    key.castShadow = true;
    key.shadow.mapSize.set(1024, 1024);
    key.shadow.camera.left = -45; key.shadow.camera.right = 45;
    key.shadow.camera.top = 30; key.shadow.camera.bottom = -42;
    this.scene.add(key);
    const pool = new THREE.PointLight(0xc4ef80, 26, 25, 2);
    pool.position.set(0, 7, -9);
    this.scene.add(pool);
  }

  #material(color, roughness = 0.85) {
    return new THREE.MeshStandardMaterial({ color, roughness, metalness: 0.03 });
  }

  #box(parent, x, y, z, w, h, d, material, castShadow = true) {
    const mesh = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), material);
    mesh.position.set(x, y, z);
    mesh.castShadow = castShadow;
    mesh.receiveShadow = true;
    parent.add(mesh);
    return mesh;
  }

  #localBounds(group) {
    group.updateMatrixWorld(true);
    const bounds = new THREE.Box3().setFromObject(group);
    if (bounds.isEmpty()) return null;
    bounds.translate(new THREE.Vector3(-group.position.x, -group.position.y, -group.position.z));
    return bounds;
  }

  #label(text, x, y, z, size = 26) {
    const canvas = document.createElement('canvas');
    canvas.width = 512; canvas.height = 128;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = 'rgba(9, 20, 22, .78)';
    ctx.fillRect(12, 18, 488, 92);
    ctx.strokeStyle = 'rgba(196, 239, 128, .42)';
    ctx.strokeRect(12, 18, 488, 92);
    ctx.fillStyle = '#e6eddf';
    ctx.font = `500 ${size}px "Microsoft JhengHei", sans-serif`;
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(text, 256, 64, 460);
    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    const sprite = new THREE.Sprite(new THREE.SpriteMaterial({ map: texture, transparent: true, depthWrite: false }));
    sprite.position.set(x, y, z);
    sprite.scale.set(2.9, 0.72, 1);
    return sprite;
  }

  #buildRooms() {
    const concrete = this.#material(0x66736a);
    const wallMaterial = this.#material(palette.wall);
    for (const [id, room] of Object.entries(ROOM)) {
      const group = new THREE.Group();
      group.name = id;
      this.scene.add(group);
      this.roomGroups.set(id, group);
      const floorMat = this.#material(room.color);
      const floor = new THREE.Mesh(new THREE.BoxGeometry(room.w, 0.28, room.d), floorMat);
      floor.position.set(room.x, -0.17, room.z);
      floor.receiveShadow = true;
      group.add(floor);
      const border = this.#material(palette.trim);
      this.#box(group, room.x, -0.02, room.z - room.d / 2, room.w, 0.24, 0.18, border, false);
      this.#box(group, room.x, -0.02, room.z + room.d / 2, room.w, 0.24, 0.18, border, false);
      this.#box(group, room.x - room.w / 2, -0.02, room.z, 0.18, 0.24, room.d, border, false);
      this.#box(group, room.x + room.w / 2, -0.02, room.z, 0.18, 0.24, room.d, border, false);
      group.add(this.#label(room.label, room.x, 0.08, room.z + room.d / 2 - 1.0, 22));
      const wall = (x, z, width, depth) => this.#box(group, x, 1.42, z, width, 2.84, depth, wallMaterial, false);
      const left = room.x - room.w / 2, right = room.x + room.w / 2;
      const top = room.z + room.d / 2, bottom = room.z - room.d / 2;
      if (id === 'Classroom') {
        wall(room.x, top, room.w, 0.22); wall(left, room.z, 0.22, room.d); wall(right, room.z, 0.22, room.d);
        wall((left - 2) / 2, bottom, Math.abs(left + 2), 0.22); wall((right + 2) / 2, bottom, right - 2, 0.22);
      } else if (id === 'Office' || id === 'GuardRoom') {
        wall(room.x, bottom, room.w, 0.22); wall(left, room.z, 0.22, room.d); wall(right, room.z, 0.22, room.d);
        wall((left + room.x - 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
        wall((right + room.x + 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
      } else if (id === 'Restroom') {
        wall(room.x, bottom, room.w, 0.22);
        wall(left, (top + room.z + 1.1) / 2, 0.22, top - room.z - 1.1);
        wall(left, (bottom + room.z - 1.1) / 2, 0.22, room.z - 1.1 - bottom);
        wall(right, room.z, 0.22, room.d);
        wall((left + room.x - 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
        wall((right + room.x + 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
      } else if (id === 'Storage') {
        wall(room.x, top, room.w, 0.22); wall(left, room.z, 0.22, room.d); wall(room.x, bottom, room.w, 0.22);
      } else if (id === 'L01' || id === 'L02') {
        wall(room.x, top, room.w, 0.22); wall(room.x, bottom, room.w, 0.22);
        wall(left, room.z, 0.22, room.d); wall(right, room.z, 0.22, room.d);
      }
      if (id === 'Classroom') {
        for (let row = 0; row < 2; row++) for (let col = 0; col < 3; col++) {
          const x = -5 + col * 5, z = -1 + row * 3;
          const desk = this.#box(group, x, 0.75, z, 2.2, 0.16, 1.2, this.#material(0x756d58));
          desk.userData.decor = true;
          this.#box(group, x, 0.38, z, 0.16, 0.75, 0.16, concrete, false);
          this.#box(group, x + 0.15, 0.42, z + 1.35, 0.8, 0.8, 0.16, this.#material(0x5a655c));
        }
        this.#box(group, 0, 1.85, 5.72, 8.5, 2.2, 0.22, this.#material(0x213b37));
        this.#box(group, 0, 1.85, 5.58, 8.1, 1.85, 0.04, this.#material(0x36514b), false);
      }
    }
    const hallwayMat = this.#material(0x8a7c62);
    this.#box(this.scene, 0, -0.26, -11, 49, 0.08, 0.45, hallwayMat, false);
    const lampMat = new THREE.MeshStandardMaterial({ color: 0xf4dfae, emissive: 0xa57e43, emissiveIntensity: 1.35 });
    for (const x of [-18, -8, 2, 12, 22]) {
      const lamp = new THREE.Mesh(new THREE.BoxGeometry(2.2, 0.08, 0.35), lampMat);
      lamp.position.set(x, 3.15, -11);
      this.scene.add(lamp);
    }
  }

  async #loadClassroomModel() {
    try {
      const model = await loadGlb(new URL('../assets/models/japanese_classroom.glb', import.meta.url));
      const room = ROOM.Classroom;
      model.rotation.y = Math.PI;
      const bounds = new THREE.Box3().setFromObject(model);
      const size = bounds.getSize(new THREE.Vector3());
      const scale = Math.min((room.w - 0.4) / size.x, (room.d - 0.4) / size.z);
      model.scale.setScalar(scale);
      model.updateMatrixWorld(true);
      bounds.setFromObject(model);
      const center = bounds.getCenter(new THREE.Vector3());
      model.position.set(room.x - center.x, -bounds.min.y, room.z - center.z);
      model.name = 'Japanese classroom (T I A N, CC BY)';
      this.scene.add(model);
      this.classroomExitDoors = CLASSROOM_EXIT_DOOR_PARTS
        .map((name) => model.getObjectByName(name))
        .filter(Boolean);
      this.#addClassroomClue();
      this.roomGroups.get('Classroom').visible = false;
      this.classroomModel = model;
      this.refresh();
    } catch (error) {
      console.error('教室模型載入失敗，使用簡化場景。', error);
    }
  }

  #addClassroomClue() {
    const canvas = document.createElement('canvas');
    canvas.width = 1024; canvas.height = 256;
    const context = canvas.getContext('2d');
    context.clearRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = '#eee0bd';
    context.textAlign = 'center';
    context.font = 'bold 42px Georgia, serif';
    context.fillText('WKH SDVVZRUG LV QRW KHUH', 512, 112);
    context.font = '26px sans-serif';
    context.fillStyle = '#c5b994';
    context.fillText('Caesar cipher  ·  shift 3', 512, 172);
    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    const clue = new THREE.Mesh(
      new THREE.PlaneGeometry(6.0, 1.38),
      new THREE.MeshBasicMaterial({ map: texture, transparent: true, depthWrite: false, side: THREE.DoubleSide }),
    );
    clue.name = 'S14 blackboard clue';
    clue.position.set(-7.72, 2.05, 0);
    clue.rotation.y = Math.PI / 2;
    this.scene.add(clue);
  }

  #objectShape(id) {
    const group = new THREE.Group();
    const wood = this.#material(palette.wood);
    const dark = this.#material(palette.dark);
    const pale = this.#material(0xb5b99d);
    const metal = new THREE.MeshStandardMaterial({ color: 0x899b8e, roughness: 0.35, metalness: 0.42 });
    switch (id) {
      case 'S01': {
        const leaf = this.#material(0x526b5e);
        this.#box(group, 0, 1.48, 0, 2.78, 2.96, 0.18, this.#material(0xb0b99b));
        this.#box(group, 0, 1.46, 0.12, 2.48, 2.72, 0.16, leaf);
        this.#box(group, 0, 1.46, 0.22, 2.08, 2.34, 0.06, this.#material(0x40584e));
        this.#box(group, 0, 1.46, 0.27, 1.88, 2.14, 0.04, leaf, false);
        this.#box(group, 0.93, 1.48, 0.32, 0.08, 0.24, 0.08, this.#material(0xd8be79));
        break;
      }
      case 'S02': {
        const bin = new THREE.Mesh(new THREE.CylinderGeometry(0.48, 0.38, 0.98, 20), this.#material(0x46584f));
        bin.position.y = 0.52; bin.castShadow = true; group.add(bin);
        const rim = new THREE.Mesh(new THREE.CylinderGeometry(0.5, 0.5, 0.09, 20), metal);
        rim.position.y = 1.03; group.add(rim);
        this.#box(group, 0, 0.5, 0.43, 0.3, 0.25, 0.035, dark, false);
        break;
      }
      case 'S03': {
        this.#box(group, 0, 1.38, 0, 1.36, 2.76, 0.78, this.#material(0x637f72));
        this.#box(group, 0, 1.62, 0.41, 0.96, 1.05, 0.035, dark, false);
        this.#box(group, 0, 0.7, 0.42, 0.88, 0.42, 0.04, this.#material(0x273e38), false);
        this.#box(group, 0, 0.42, 0.45, 0.84, 0.07, metal, false);
        for (let row = 0; row < 3; row++) for (let col = 0; col < 3; col++) {
          this.#box(group, -0.36 + col * 0.36, 2.3 - row * 0.3, 0.42, 0.25, 0.18, 0.035, this.#material([0xb57859, 0xd2b774, 0x799a82][(row + col) % 3]), false);
        }
        break;
      }
      case 'S21': {
        this.#box(group, 0, 0.36, 0, 1.1, 0.72, 0.6, wood);
        this.#box(group, 0, 0.98, -0.05, 0.9, 0.56, 0.08, this.#material(0x374c45, 0.3));
        this.#box(group, 0, 0.98, 0, 0.78, 0.44, 0.02, this.#material(0x789b91, 0.2), false);
        this.#box(group, 0, 0.75, -0.08, 0.12, 0.1, 0.12, this.#material(0x63766a));
        break;
      }
      case 'S04': {
        this.#box(group, 0, 0.5, 0.15, 1.36, 1.0, 0.3, this.#material(0x637f72));
        break;
      }
      case 'S05': case 'S09': case 'S19': {
        const door = this.#material(id === 'S09' ? 0x806a4d : 0x725a3d);
        const frameWidth = id === 'S19' ? 3.8 : 2.18;
        const leafWidth = id === 'S19' ? 3.5 : 1.92;
        const panelWidth = id === 'S19' ? 3.0 : 1.56;
        this.#box(group, 0, 1.4, 0, frameWidth, 2.82, 0.2, this.#material(palette.wall));
        this.#box(group, 0, 1.36, 0.12, leafWidth, 2.62, 0.16, door);
        this.#box(group, 0, 1.56, 0.22, panelWidth, 1.94, 0.055, this.#material(palette.trim));
        this.#box(group, 0, 1.56, 0.255, panelWidth - 0.2, 1.74, 0.035, door, false);
        const knob = new THREE.Mesh(new THREE.SphereGeometry(0.075, 12, 10), this.#material(0xd2b776, 0.35));
        knob.position.set(leafWidth / 2 - 0.28, 1.28, 0.31); group.add(knob);
        break;
      }
      case 'S06': {
        for (const x of [-0.55, 0.55]) for (const z of [-0.26, 0.26]) this.#box(group, x, 0.34, z, 0.12, 0.65, 0.12, this.#material(0x655740));
        this.#box(group, 0, 0.72, 0, 1.55, 0.15, 0.9, wood);
        this.#box(group, 0, 0.52, 0.47, 1.28, 0.32, 0.08, this.#material(0x6c5b43));
        this.#box(group, 0, 0.53, 0.53, 0.24, 0.04, 0.04, metal, false);
        break;
      }
      case 'S07': {
        this.#box(group, 0, 0.24, 0, 1.12, 0.08, 0.74, this.#material(0x323d39));
        this.#box(group, 0, 0.31, -0.08, 0.22, 0.45, 0.16, metal);
        this.#box(group, 0, 0.73, -0.12, 1.34, 0.96, 0.14, dark);
        this.#box(group, 0, 0.73, -0.035, 1.19, 0.78, 0.035, this.#material(0x365950), false);
        this.#box(group, 0, 0.32, 0.42, 0.92, 0.035, 0.25, this.#material(0x677369), false);
        for (let i = 0; i < 5; i++) this.#box(group, -0.32 + i * 0.16, 0.345, 0.42, 0.08, 0.012, 0.04, pale, false);
        break;
      }
      case 'S08': {
        const paper = this.#material(0xb69a70);
        this.#box(group, 0, 0.25, 0, 1.12, 0.48, 0.76, paper);
        this.#box(group, 0, 0.5, 0, 1.14, 0.06, 0.78, this.#material(0xc5ad81));
        this.#box(group, 0, 0.52, 0.4, 0.18, 0.045, 0.03, this.#material(0x8d7654), false);
        break;
      }
      case 'S10': this.#box(group, 0, 0.4, 0, 0.85, 0.8, 0.85, this.#material(0x796e5a)); this.#box(group, 0, 0.96, 0, 1.05, 0.16, 1.05, this.#material(0x465947)); break;
      case 'S11': {
        const porcelain = this.#material(0xe2e5d4, 0.28);
        const base = new THREE.Mesh(new THREE.CylinderGeometry(0.34, 0.46, 0.56, 18), porcelain);
        base.position.set(0, 0.34, 0.1); base.scale.z = 1.25; base.castShadow = true; group.add(base);
        const bowl = new THREE.Mesh(new THREE.SphereGeometry(0.52, 20, 14), porcelain);
        bowl.position.set(0, 0.68, 0.05); bowl.scale.set(0.92, 0.48, 1.12); bowl.castShadow = true; group.add(bowl);
        const seat = new THREE.Mesh(new THREE.TorusGeometry(0.38, 0.065, 8, 24), this.#material(0xf0f0e5, 0.25));
        seat.position.set(0, 0.89, 0.05); seat.rotation.x = Math.PI / 2; seat.scale.set(1, 1.25, 1); group.add(seat);
        this.#box(group, 0, 0.94, -0.43, 0.76, 0.84, 0.25, porcelain);
        break;
      }
      case 'S12': {
        const bin = new THREE.Mesh(new THREE.CylinderGeometry(0.34, 0.28, 0.72, 16), this.#material(0x65746c));
        bin.position.y = 0.43; bin.castShadow = true; group.add(bin);
        const rim = new THREE.Mesh(new THREE.CylinderGeometry(0.37, 0.37, 0.08, 16), metal);
        rim.position.y = 0.82; group.add(rim);
        this.#box(group, 0, 0.44, 0.29, 0.2, 0.18, 0.035, dark, false);
        break;
      }
      case 'S13': {
        this.#box(group, 0, 2.7, 0, 2.7, 1.2, 0.16, metal);
        this.#box(group, 0, 2.7, 0.11, 2.38, 0.9, 0.05, this.#material(0x263f40), false);
        this.#box(group, 0, 2.7, 0.16, 0.08, 0.92, 0.05, this.#material(0xc6c9a7), false);
        this.#box(group, 0, 2.7, 0.16, 2.38, 0.07, 0.05, this.#material(0xc6c9a7), false);
        break;
      }
      case 'S14': {
        this.#box(group, 0, 1.8, 0, 0.16, 1.7, 6.0, wood, false);
        break;
      }
      case 'S15': {
        this.#box(group, 0, 1.25, 0, 1.36, 2.42, 0.42, this.#material(0x74796a));
        this.#box(group, 0, 1.25, 0.22, 1.12, 2.18, 0.08, this.#material(0x434d43));
        this.#box(group, 0, 1.9, 0.28, 0.7, 0.32, 0.06, this.#material(0xb95c4d), false);
        this.#box(group, 0, 1.2, 0.29, 0.18, 0.72, 0.08, this.#material(0x202b27));
        this.#box(group, 0, 0.78, 0.34, 0.42, 0.06, 0.04, metal, false);
        for (let i = 0; i < 3; i++) {
          const light = new THREE.Mesh(new THREE.SphereGeometry(0.06, 10, 8), this.#material([0xb95c4d, 0xc4ef80, 0x8eb4ab][i], 0.3));
          light.position.set(-0.28 + i * 0.28, 0.44, 0.3); group.add(light);
        }
        break;
      }
      case 'S16': {
        this.#box(group, 0, 0.56, 0, LECTERN_SIZE[0], 1.12, LECTERN_SIZE[1], wood);
        break;
      }
      case 'S17': {
        this.#box(group, 0, 1.55, 0, 2.55, 2.35, 0.15, wood);
        this.#box(group, 0, 1.55, 0.1, 2.3, 2.1, 0.04, this.#material(0x8e8068), false);
        for (const [x, y, color] of [[-0.55,2.05,0xb68069],[0.48,1.9,0xc3b881],[-0.15,1.1,0x819b85]]) {
          this.#box(group, x, y, 0.14, 0.65, 0.72, 0.045, this.#material(color), false);
          this.#box(group, x, y + 0.12, 0.17, 0.42, 0.04, pale, false);
          this.#box(group, x, y - 0.03, 0.17, 0.34, 0.025, pale, false);
        }
        break;
      }
      case 'S18': {
        this.#box(group, 0, 1.35, 0, 1.42, 1.78, 0.14, this.#material(0x9c9a82));
        this.#box(group, 0, 1.35, 0.1, 1.26, 1.62, 0.035, pale, false);
        this.#box(group, 0, 2.02, 0.14, 1.1, 0.22, 0.035, this.#material(0x9c624f), false);
        for (let row = 0; row < 4; row++) for (let col = 0; col < 5; col++) {
          this.#box(group, -0.42 + col * 0.21, 1.65 - row * 0.25, 0.14, 0.12, 0.13, 0.025, this.#material(row === 2 && col === 4 ? 0xb95c4d : 0xd9d6bd), false);
        }
        break;
      }
      case 'S20': {
        this.#box(group, 0, 0.76, 0, 0.9, 0.28, 0.06, this.#material(0x8d7a5a));
        this.#box(group, 0, 0.76, 0.04, 0.22, 0.04, 0.035, metal, false);
        break;
      }
    }
    return group;
  }

  #buildObjects() {
    for (const [id, label] of Object.entries(OBJECTS)) {
      const group = this.#objectShape(id);
      const [x, z] = OBJECT_POSITIONS[id];
      group.position.set(x, 0, z);
      if (id === 'S09') group.rotation.y = Math.PI / 2;
      group.userData.targetId = id;
      group.userData.interactionOnly = MODEL_TARGETS.has(id);
      group.name = `${id} ${label}`;
      if (group.userData.interactionOnly) {
        group.traverse((node) => {
          if (!node.isMesh) return;
          node.castShadow = false;
          node.receiveShadow = false;
          const wasArray = Array.isArray(node.material);
          const materials = wasArray ? node.material : [node.material];
          const proxyMaterials = materials.map((material) => {
            const proxyMaterial = material.clone();
            proxyMaterial.colorWrite = false;
            proxyMaterial.depthWrite = false;
            return proxyMaterial;
          });
          node.material = wasArray ? proxyMaterials : proxyMaterials[0];
        });
      }
      this.scene.add(group);
      this.targetGroups.set(id, group);
      this.targetMeshes.push(group);
      group.userData.localBounds = this.#localBounds(group);
    }
    for (const id of Object.keys(FORMS).filter((formId) => formId !== 'F00')) {
      const group = new THREE.Group();
      group.userData.targetId = id;
      group.name = `${id} ${FORMS[id]}`;
      this.scene.add(group);
      this.itemGroups.set(id, group);
      this.targetMeshes.push(group);
      this.#rebuildFormMesh(group, id);
      group.userData.localBounds = this.#localBounds(group);
    }
  }

  #buildExits() {
    for (const [area, exit] of Object.entries(SPECIAL_EXITS)) {
      const group = new THREE.Group();
      const [x, z] = exit.at;
      group.position.set(x, 0, z);
      group.name = `${area} 出口`;
      group.userData.targetId = 'EXIT';
      this.#box(group, -0.5, 0.55, 0, 0.08, 1.1, 1.3, this.#material(0x0b1214), false);
      this.#box(group, -0.45, 1.13, 0, 0.12, 0.08, 1.46, this.#material(palette.lime, 0.4), false);
      for (const side of [-1, 1]) this.#box(group, -0.45, 0.55, side * 0.69, 0.12, 1.1, 0.08, this.#material(palette.lime, 0.4), false);
      const ring = new THREE.Mesh(new THREE.RingGeometry(0.45, 0.58, 32), new THREE.MeshBasicMaterial({ color: palette.lime, transparent: true, opacity: 0.55 }));
      ring.rotation.x = -Math.PI / 2; ring.position.y = 0.02; group.add(ring);
      this.scene.add(group);
      this.exitGroups.set(area, group);
      this.targetMeshes.push(group);
    }
  }

  exitSpecialLocation() {
    const area = this.game.player().area;
    const exit = SPECIAL_EXITS[area];
    if (!exit || !this.game.moveToArea('local', 'Corridor')) return { rejected: true, text: '' };
    const [x, z] = exit.to;
    this.playerPosition.set(x, 0, z);
    this.playerGroup.position.copy(this.playerPosition);
    this.refresh();
    return { text: exit.text, location: 'Corridor' };
  }

  #formShape(id) {
    const group = new THREE.Group();
    const mat = (color, metalness = 0.04) => new THREE.MeshStandardMaterial({ color, roughness: 0.78, metalness });
    const add = (mesh, x = 0, y = 0, z = 0) => { mesh.position.set(x, y, z); mesh.castShadow = true; group.add(mesh); };
    if (id === 'F00') {
      const coat = mat(0x68766b), trim = mat(0x455349), shirt = mat(0xd6cbb1), skin = mat(0xc9a987), hair = mat(0x382f29), trousers = mat(0x51483d), shoes = mat(0x40372f);
      const ellipsoid = (parent, material, size, position) => {
        const mesh = new THREE.Mesh(new THREE.SphereGeometry(1, 20, 16), material);
        mesh.scale.set(...size); mesh.position.set(...position); mesh.castShadow = true; parent.add(mesh); return mesh;
      };
      // Compact, softly rounded student silhouette with joint pivots for continuous motion.
      const torso = new THREE.Group(); torso.position.y = 0.68; group.add(torso);
      ellipsoid(torso, coat, [0.205, 0.29, 0.13], [0, 0.04, 0]);
      ellipsoid(torso, shirt, [0.105, 0.22, 0.035], [0, 0.055, 0.116]);
      ellipsoid(torso, trim, [0.215, 0.09, 0.14], [0, -0.18, 0]);
      ellipsoid(torso, mat(0xb6a16e, 0.25), [0.018, 0.018, 0.014], [0.12, 0.03, 0.128]);
      const head = new THREE.Group(); head.position.set(0, 1.02, 0); group.add(head);
      ellipsoid(head, skin, [0.145, 0.16, 0.14], [0, 0, 0]);
      ellipsoid(head, hair, [0.15, 0.105, 0.148], [0, 0.09, -0.018]);
      ellipsoid(head, hair, [0.145, 0.065, 0.045], [0, 0.035, 0.11]);
      const eye = mat(0x322d28);
      ellipsoid(head, eye, [0.012, 0.012, 0.009], [-0.047, 0.008, 0.137]);
      ellipsoid(head, eye, [0.012, 0.012, 0.009], [0.047, 0.008, 0.137]);
      ellipsoid(head, skin, [0.017, 0.02, 0.018], [0, -0.035, 0.142]);
      for (const side of [-1, 1]) {
        ellipsoid(head, skin, [0.035, 0.05, 0.025], [side * 0.145, -0.02, 0]);
        const shoulder = new THREE.Group(); shoulder.position.set(side * 0.21, 0.86, 0); group.add(shoulder);
        ellipsoid(shoulder, coat, [0.09, 0.105, 0.095], [0, -0.035, 0]);
        const upperArm = new THREE.Group(); shoulder.add(upperArm);
        const upper = new THREE.Mesh(new THREE.CapsuleGeometry(0.058, 0.16, 4, 10), coat); upper.position.y = -0.13; upper.castShadow = true; upperArm.add(upper);
        const elbow = new THREE.Group(); elbow.position.y = -0.26; upperArm.add(elbow);
        ellipsoid(elbow, trim, [0.056, 0.058, 0.057], [0, 0, 0]);
        const forearm = new THREE.Mesh(new THREE.CapsuleGeometry(0.05, 0.13, 4, 10), coat); forearm.position.y = -0.11; forearm.castShadow = true; elbow.add(forearm);
        const hand = ellipsoid(elbow, skin, [0.048, 0.052, 0.045], [0, -0.22, 0.005]);
        const hip = new THREE.Group(); hip.position.set(side * 0.105, 0.48, 0); group.add(hip);
        const thigh = new THREE.Mesh(new THREE.CapsuleGeometry(0.073, 0.13, 4, 10), trousers); thigh.position.y = -0.12; thigh.castShadow = true; hip.add(thigh);
        const knee = new THREE.Group(); knee.position.y = -0.24; hip.add(knee);
        ellipsoid(knee, trousers, [0.07, 0.065, 0.07], [0, 0, 0]);
        const shin = new THREE.Mesh(new THREE.CapsuleGeometry(0.059, 0.13, 4, 10), trousers); shin.position.y = -0.12; shin.castShadow = true; knee.add(shin);
        const foot = new THREE.Mesh(new THREE.CapsuleGeometry(0.065, 0.085, 3, 10), shoes); foot.rotation.x = Math.PI / 2; foot.position.set(0, -0.23, 0.045); foot.castShadow = true; knee.add(foot);
        (group.userData.rig ??= { torso, head, arms: [], hips: [] }).arms.push({ upper: upperArm, elbow, side });
        group.userData.rig.hips.push({ hip, knee, side });
      }
    } else if (id === 'F01') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.35, 0.38, 0.62, 24), mat(0x9eaaa0, 0.58)), 0, 0.45);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.37, 0.37, 0.06, 24), mat(0x4f5e53, 0.48)), 0, 0.77);
      add(new THREE.Mesh(new THREE.TorusGeometry(0.35, 0.025, 8, 24), mat(0xc4c8b2, 0.5)), 0, 0.77);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.46, 0.25, 0.025), mat(0x6d8c76)), 0, 0.48, 0.36);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.27, 0.04, 0.03), mat(0xd1c783)), 0, 0.48, 0.38);
    } else if (id === 'F02') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.28, 0.31, 0.8, 20), mat(0xb3a16e)), 0, 0.48);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.13, 0.13, 0.14, 16), mat(0xb9ba90, 0.38)), 0, 0.96);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.285, 0.285, 0.22, 20), mat(0x718c75)), 0, 0.48);
      add(new THREE.Mesh(new THREE.TorusGeometry(0.14, 0.035, 8, 16), mat(0xe1d2a0)), 0, 0.56);
    } else if (id === 'F03' || id === 'F06') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.76, 0.5, 0.08), mat(id === 'F03' ? 0xc4d4b3 : 0xd3c19b)), 0, 0.4);
      if (id === 'F03') {
        add(new THREE.Mesh(new THREE.BoxGeometry(0.48, 0.08, 0.025), mat(0x526b5d)), 0, 0.56, 0.055);
        add(new THREE.Mesh(new THREE.BoxGeometry(0.18, 0.22, 0.025), mat(0x668274)), -0.2, 0.33, 0.055);
        add(new THREE.Mesh(new THREE.BoxGeometry(0.27, 0.035, 0.025), mat(0x718477)), 0.13, 0.38, 0.055);
        add(new THREE.Mesh(new THREE.BoxGeometry(0.25, 0.035, 0.025), mat(0x718477)), 0.13, 0.29, 0.055);
      } else {
        add(new THREE.Mesh(new THREE.BoxGeometry(0.44, 0.055, 0.04), mat(0x566b5d)), 0, 0.44, 0.06);
        for (let i = 0; i < 4; i++) add(new THREE.Mesh(new THREE.BoxGeometry(0.08, 0.035, 0.025), mat(0x86795f)), -0.12 + i * 0.08, 0.31, 0.06);
      }
    } else if (id === 'F04' || id === 'F05') {
      const gold = mat(id === 'F04' ? 0xb6a16e : 0xa0a39a, 0.55);
      const bow = new THREE.Mesh(new THREE.TorusGeometry(0.2, 0.055, 10, 22), gold);
      bow.rotation.x = Math.PI / 2; add(bow, 0, 0.62);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.045, 0.045, 0.58, 12), gold), 0, 0.27);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.19, 0.07, 0.08), gold), 0.06, 0.08);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.1, 0.12, 0.08), gold), -0.03, 0.19);
    } else if (id === 'F11') {
      const ring = new THREE.Mesh(new THREE.TorusGeometry(0.36, 0.055, 10, 24), mat(0xb6a16e, 0.6));
      ring.rotation.x = Math.PI / 2; add(ring, 0, 0.72);
      for (let i = 0; i < 3; i++) {
        const x = -0.28 + i * 0.28;
        const keyMat = mat([0xb6a16e, 0xc3b88f, 0x9eaa9b][i], 0.5);
        add(new THREE.Mesh(new THREE.TorusGeometry(0.11, 0.028, 8, 14), keyMat), x, 0.5);
        add(new THREE.Mesh(new THREE.CylinderGeometry(0.028, 0.028, 0.56, 8), keyMat), x, 0.19);
        add(new THREE.Mesh(new THREE.BoxGeometry(0.14, 0.06, 0.055), keyMat), x + 0.05, -0.1);
      }
    } else if (id === 'F07') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.06, 1.8, 8), mat(0x8d886f)), 0, 0.9);
    } else if (id === 'F08') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.72, 0.72, 0.72), mat(0xe4e0c6)), 0, 0.43);
      const pip = mat(0x34483c);
      const dot = (x, y, z) => add(new THREE.Mesh(new THREE.SphereGeometry(0.052, 10, 8), pip), x, y, z);
      for (const [x,y] of [[-.18,.61],[.18,.25],[0,.43]]) dot(x,y,0.37);
      for (const [x,y] of [[-.17,.6],[.17,.6],[-.17,.26],[.17,.26]]) dot(x,y,-0.37);
      for (const [z,y] of [[-.17,.6],[.17,.26]]) dot(0.37,y,z);
      for (const [z,y] of [[-.18,.61],[.18,.61],[-.18,.25],[.18,.25],[0,.43]]) dot(-0.37,y,z);
      for (const [x,z] of [[-.18,-.18],[.18,-.18],[0,0],[-.18,.18],[.18,.18]]) dot(x,0.8,z);
      for (const [x,z] of [[-.2,-.2],[.2,-.2],[-.2,.2],[.2,.2],[0,0],[-.2,0]]) dot(x,0.06,z);
    } else if (id === 'F09') {
      const ring = new THREE.Mesh(new THREE.TorusGeometry(0.46, 0.09, 10, 28), mat(0xd6c88c, 0.4)); ring.rotation.x = Math.PI / 2; add(ring, 0, 0.88);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.34,0.34,0.08,24), mat(0xddd6b7)), 0, 0.88);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.3,0.3,0.085,24), mat(0x47574b)), 0, 0.88);
      for (let i = 0; i < 12; i++) {
        const angle = i * Math.PI / 6;
        add(new THREE.Mesh(new THREE.BoxGeometry(0.035,0.025,0.085), mat(0xd9d3b2)), Math.sin(angle) * 0.24, 0.94, Math.cos(angle) * 0.24);
      }
      add(new THREE.Mesh(new THREE.BoxGeometry(0.045,0.025,0.23), mat(0xd6c88c)), 0, 0.95, -0.04);
      const hand = new THREE.Mesh(new THREE.BoxGeometry(0.04,0.025,0.16), mat(0xe2b877)); hand.rotation.y = -0.75; add(hand, 0.04, 0.96, 0.02);
      add(new THREE.Mesh(new THREE.SphereGeometry(0.045,10,8), mat(0xd6c88c)), 0, 0.96, 0);
    } else if (id === 'F12') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8,0.13,0.75), mat(palette.wood)), 0, 0.58);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8,0.75,0.14), mat(palette.wood)), 0, 1.0, -0.3);
      for (const x of [-0.3,0.3]) for (const z of [-0.27,0.27]) add(new THREE.Mesh(new THREE.BoxGeometry(0.08,0.55,0.08), mat(0x6c5840)), x, 0.27, z);
    }
    return group;
  }

  #rebuildFormMesh(target, formId) {
    for (const child of [...target.children]) {
      target.remove(child);
      if (child === this.humanModel) continue;
      child.traverse((node) => {
        node.geometry?.dispose();
        const materials = Array.isArray(node.material) ? node.material : [node.material];
        for (const material of materials) {
          material?.map?.dispose();
          material?.dispose();
        }
      });
    }
    if (formId === 'F00' && this.humanModel) {
      target.add(this.humanModel);
      target.userData.rig = this.playerRig;
      if (target === this.playerGroup) this.playerRig = target.userData.rig;
      return;
    }
    const shape = this.#formShape(formId);
    target.userData.rig = shape.userData.rig ?? null;
    while (shape.children.length) target.add(shape.children[0]);
    if (target === this.playerGroup) this.playerRig = target.userData.rig;
  }

  #buildPlayer() {
    this.playerGroup = this.#formShape('F00');
    this.playerGroup.name = '玩家';
    this.playerGroup.position.copy(this.playerPosition);
    this.scene.add(this.playerGroup);
    this.#rebuildFormMesh(this.playerGroup, 'F00');
    this.humanGroup = new THREE.Group();
    this.humanGroup.name = '人類身體';
    this.humanGroup.visible = false;
    this.scene.add(this.humanGroup);
    this.#rebuildFormMesh(this.humanGroup, 'F00');
  }

  #itemPosition(id) {
    return this.itemDrops.get(id) ?? ITEM_POSITIONS[id];
  }

  async #loadPlayerModel() {
    try {
      const source = await loadGlb(new URL('../assets/models/vivi_manga_student_girl_style.glb', import.meta.url));
      source.updateMatrixWorld(true);
      source.traverse((node) => { if (node.isSkinnedMesh) node.skeleton.update(); });
      const bounds = new THREE.Box3().setFromObject(source);
      const size = bounds.getSize(new THREE.Vector3());
      if (!Number.isFinite(size.y) || size.y <= 0) throw new Error('玩家模型沒有有效高度');
      source.scale.setScalar(1.3 / size.y);
      source.updateMatrixWorld(true);
      bounds.setFromObject(source);
      const center = bounds.getCenter(new THREE.Vector3());
      source.position.set(-center.x, -bounds.min.y, -center.z);
      source.name = 'Vivi manga student';
      source.updateMatrixWorld(true);
      source.traverse((node) => {
        if (!node.isMesh) return;
        node.castShadow = true;
        node.receiveShadow = true;
        node.frustumCulled = false;
      });
      this.humanModel = source;
      this.playerRig = this.#createSkinnedPlayerRig(source);
      if (this.game.player().form === 'F00') this.#rebuildFormMesh(this.playerGroup, 'F00');
    } catch (error) {
      console.error('玩家模型載入失敗，使用簡化角色。', error);
    }
  }

  #createSkinnedPlayerRig(model) {
    const find = (part) => model.getObjectByName(`mixamorig:${part}_Armature`);
    const parts = {
      leftArm: find('LeftArm'), rightArm: find('RightArm'),
      leftForearm: find('LeftForeArm'), rightForearm: find('RightForeArm'),
      leftHand: find('LeftHand'), rightHand: find('RightHand'),
      leftLeg: find('LeftUpLeg'), rightLeg: find('RightUpLeg'),
      leftKnee: find('LeftLeg'), rightKnee: find('RightLeg'),
      leftFoot: find('LeftFoot'), rightFoot: find('RightFoot'),
    };
    const restRotations = new Map(Object.values(parts).filter(Boolean).map((bone) => [bone, bone.quaternion.clone()]));
    return {
      kind: 'skinned', parts, restRotations,
    };
  }

  #updatePlayerSkin() {
    const rig = this.playerRig;
    if (!rig || rig.kind !== 'skinned') return;
    const gait = this.walking ? Math.sin(this.playerAnimationTime * 8) : 0;
    const { parts } = rig;
    const jump = this.jumpHeight > 0.02;
    const model = this.humanModel;
    const modelRotation = model.getWorldQuaternion(new THREE.Quaternion());
    const parentRotation = new THREE.Quaternion();
    const desiredParentDirection = new THREE.Vector3();
    const restDirection = new THREE.Vector3();
    const deltaRotation = new THREE.Quaternion();
    const setBoneDirection = (bone, child, direction) => {
      if (!bone || !child || !bone.parent) return;
      bone.parent.getWorldQuaternion(parentRotation).invert();
      desiredParentDirection.copy(direction).applyQuaternion(modelRotation).applyQuaternion(parentRotation).normalize();
      restDirection.copy(child.position).applyQuaternion(rig.restRotations.get(bone)).normalize();
      deltaRotation.setFromUnitVectors(restDirection, desiredParentDirection);
      bone.quaternion.copy(deltaRotation).multiply(rig.restRotations.get(bone));
      model.updateMatrixWorld(true);
    };
    const armSwing = jump ? 0.42 : gait * 0.16;
    const forearmSwing = jump ? 0.24 : gait * 0.07 + 0.08;
    for (const [side, arm, forearm, hand] of [
      [-1, parts.leftArm, parts.leftForearm, parts.leftHand],
      [1, parts.rightArm, parts.rightForearm, parts.rightHand],
    ]) {
      setBoneDirection(arm, forearm, new THREE.Vector3(-side * 0.1, -Math.cos(armSwing), Math.sin(armSwing)));
      setBoneDirection(forearm, hand, new THREE.Vector3(-side * 0.06, -Math.cos(forearmSwing), Math.sin(forearmSwing)));
    }
    for (const [side, leg, knee] of [
      [-1, parts.leftLeg, parts.leftKnee],
      [1, parts.rightLeg, parts.rightKnee],
    ]) {
      const stride = jump ? side * 0.16 : -gait * side * 0.24;
      setBoneDirection(leg, knee, new THREE.Vector3(0, -Math.cos(stride), Math.sin(stride)));
      const kneeFlex = jump ? 0.2 : Math.max(0, -stride) * 0.45;
      const foot = side < 0 ? parts.leftFoot : parts.rightFoot;
      setBoneDirection(knee, foot, new THREE.Vector3(0, -Math.cos(kneeFlex), Math.sin(kneeFlex)));
    }
  }

  setForm(formId) {
    // The single character model moves to the body left behind while the player is an item.
    if (formId !== 'F00' && this.humanModel?.parent === this.playerGroup) {
      this.humanGroup.rotation.y = this.playerGroup.rotation.y;
      this.#rebuildFormMesh(this.humanGroup, 'F00');
    }
    this.#rebuildFormMesh(this.playerGroup, formId);
    this.jumpHeight = 0;
    this.jumpVelocity = 0;
    this.playerGroup.position.y = 0;
  }

  syncAfterAction(previousForm, result) {
    const player = this.game.player();
    const changedForm = player.form !== previousForm;
    if (result.ending === 'E05') this.itemDrops.clear();
    else if (changedForm && previousForm !== 'F00') this.itemDrops.set(previousForm, [this.playerPosition.x, this.playerPosition.z]);
    if (previousForm === 'F00' && player.form !== 'F00') this.humanPosition.copy(this.playerPosition);
    if (result.location) {
      const [x, z] = entryPoint(result.location);
      this.playerPosition.set(x, 0, z);
    } else if (result.ending === 'E05') this.playerPosition.set(-5.2, 0, -4.5);
    else if (changedForm && player.form !== 'F00') {
      const [x, z] = this.#itemPosition(player.form);
      this.playerPosition.set(x, 0, z);
    } else if (!result.ending && player.form === 'F00' && previousForm !== 'F00') this.playerPosition.copy(this.humanPosition);
    this.playerGroup.position.copy(this.playerPosition);
    this.setForm(player.form);
    this.refresh();
  }

  resetRound() {
    this.itemDrops.clear();
    this.playerPosition.set(-5.2, 0, -4.5);
    this.humanPosition.copy(this.playerPosition);
    this.playerGroup.position.copy(this.playerPosition);
    this.setForm(this.game.player().form);
    this.refresh();
  }

  refresh() {
    for (const [id, group] of this.itemGroups) {
      const item = this.game.item(id);
      const isPlayer = this.game.player().form === id;
      group.visible = Boolean(item?.exists && !item.occupiedBy && !isPlayer);
      if (group.visible && item.area) {
        let [x, z] = this.#itemPosition(id);
        if (areaAt(x, z) !== item.area) {
          [x, z] = dropPoint(x, z);
          this.itemDrops.set(id, [x, z]);
        }
        group.position.set(x, 0, z);
      }
    }
    this.humanGroup.visible = this.game.player().form !== 'F00';
    this.humanGroup.position.copy(this.humanPosition);
    for (const doorId of ['S05', 'S09', 'S19']) {
      const door = this.targetGroups.get(doorId);
      if (door) door.visible = !this.game.isDoorOpen(doorId);
    }
    const exitOpen = this.game.isDoorOpen('S19');
    for (const part of this.classroomExitDoors) part.visible = !exitOpen;
    const nearest = this.nearestTarget();
    for (const [id, group] of [...this.targetGroups, ...this.itemGroups]) {
      if (!group.visible) continue;
      for (const child of group.children) if (child.isSprite) child.visible = nearest?.id === id;
    }
    this.playerGroup.visible = true;
  }

  nearestTarget(maxDistance = 3.7) {
    const candidates = [];
    const exit = this.exitGroups.get(this.game.player().area);
    if (exit) candidates.push({ id: 'EXIT', name: '離開這裡', x: exit.position.x, z: exit.position.z });
    for (const [id, group] of this.targetGroups) {
      if (!group.visible) continue;
      if (OBJECT_AREAS[id] !== this.game.player().area) continue;
      candidates.push({ id, name: OBJECTS[id], x: group.position.x, z: group.position.z, bounds: group.userData.localBounds });
    }
    for (const [id, group] of this.itemGroups) {
      if (!group.visible) continue;
      candidates.push({ id, name: FORMS[id], x: group.position.x, z: group.position.z });
    }
    const best = nearestCandidate(this.playerPosition.x, this.playerPosition.z, this.yaw, candidates, maxDistance);
    return best && { id: best.id, name: best.name, distance: best.distance };
  }

  pickTarget(clientX, clientY) {
    const rect = this.canvas.getBoundingClientRect();
    const pointer = new THREE.Vector2((clientX - rect.left) / rect.width * 2 - 1, -((clientY - rect.top) / rect.height) * 2 + 1);
    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(pointer, this.camera);
    const hits = raycaster.intersectObjects(this.targetMeshes, true);
    for (const hit of hits) {
      let node = hit.object;
      while (node && !node.userData.targetId) node = node.parent;
      if (!node) continue;
      const dist = Math.hypot(node.position.x + (node.parent === this.scene ? 0 : node.parent.position.x) - this.playerPosition.x, node.position.z - this.playerPosition.z);
      if (dist <= 4.2) return node.userData.targetId;
    }
    return null;
  }

  #blockedAt(x, z, radius) {
    const area = areaAt(x, z);
    if (area === 'Classroom') {
      if (Math.abs(x) > 7.45 || z > 5.45) return true;
      if (z < -5.35 && !(this.game.isDoorOpen('S19') && x >= -6.8 && x <= -4.4)) return true;
      for (const row of CLASSROOM_DESK_ROWS) for (const column of CLASSROOM_DESK_COLUMNS) {
        const desk = { minX: column - 0.39, maxX: column + 0.39, minZ: row - 0.54, maxZ: row + 0.54 };
        if (this.#circleHitsBox(x, z, radius, desk)) return true;
        const chair = { minX: column + 0.1, maxX: column + 0.68, minZ: row - 0.26, maxZ: row + 0.26 };
        if (this.#circleHitsBox(x, z, radius, chair)) return true;
      }
    }

    for (const group of [...this.targetGroups.values(), ...this.itemGroups.values()]) {
      if (!group.visible || group.userData.interactionOnly || !group.userData.localBounds) continue;
      const bounds = group.userData.localBounds;
      const box = {
        minX: bounds.min.x + group.position.x,
        maxX: bounds.max.x + group.position.x,
        minZ: bounds.min.z + group.position.z,
        maxZ: bounds.max.z + group.position.z,
      };
      if (this.#circleHitsBox(x, z, radius, box)) return true;
    }

    if (!this.game.isDoorOpen('S19')) {
      for (const part of this.classroomExitDoors) {
        if (!part.visible) continue;
        const bounds = new THREE.Box3().setFromObject(part);
        if (this.#circleHitsBox(x, z, radius, { minX: bounds.min.x, maxX: bounds.max.x, minZ: bounds.min.z, maxZ: bounds.max.z })) return true;
      }
    }
    return false;
  }

  #circleHitsBox(x, z, radius, box) {
    const nearestX = Math.max(box.minX, Math.min(x, box.maxX));
    const nearestZ = Math.max(box.minZ, Math.min(z, box.maxZ));
    return (x - nearestX) ** 2 + (z - nearestZ) ** 2 < radius ** 2;
  }

  #canCrossBoundary(from, to, x, z) {
    if ((from === 'Classroom' && to === 'Corridor') || (from === 'Corridor' && to === 'Classroom')) {
      return this.game.isDoorOpen('S19') && x >= -6.8 && x <= -4.4;
    }
    if ((from === 'Corridor' && to === 'Office') || (from === 'Office' && to === 'Corridor')) {
      return this.game.isDoorOpen('S05') && x >= 9.7 && x <= 12.3;
    }
    if ((from === 'Corridor' && to === 'Restroom') || (from === 'Restroom' && to === 'Corridor')) {
      return x >= -12.3 && x <= -9.7;
    }
    if ((from === 'Corridor' && to === 'GuardRoom') || (from === 'GuardRoom' && to === 'Corridor')) {
      return x >= 21.7 && x <= 24.3;
    }
    if ((from === 'Restroom' && to === 'Storage') || (from === 'Storage' && to === 'Restroom')) {
      return this.game.isDoorOpen('S09') && z >= -21.9 && z <= -20.1;
    }
    return true;
  }

  jump() {
    if (!this.started || this.game.player().ending || this.game.player().form !== 'F00' || this.jumpHeight > 0) return false;
    this.jumpVelocity = 4.3;
    return true;
  }

  move(dt) {
    if (!this.started || this.game.player().ending) return;
    this.playerAnimationTime += dt;
    if (this.jumpHeight > 0 || this.jumpVelocity > 0) {
      this.jumpVelocity -= 11.5 * dt;
      this.jumpHeight += this.jumpVelocity * dt;
      if (this.jumpHeight <= 0) { this.jumpHeight = 0; this.jumpVelocity = 0; }
    }
    this.playerGroup.position.y = this.game.player().form === 'F00' ? this.jumpHeight : 0;
    const moving = this.keys.has('KeyW') || this.keys.has('KeyA') || this.keys.has('KeyS') || this.keys.has('KeyD') || this.keys.has('ArrowUp') || this.keys.has('ArrowDown') || this.keys.has('ArrowLeft') || this.keys.has('ArrowRight');
    this.walking = Boolean(moving);
    if (this.game.player().form === 'F00') {
      const gait = moving ? Math.sin(this.playerAnimationTime * 10) : 0;
      this.playerGroup.rotation.y = this.yaw + Math.PI;
      if (this.playerRig) {
        if (this.playerRig.kind === 'skinned') this.#updatePlayerSkin();
        else {
          this.playerRig.torso.position.y = 0.68 + (moving ? Math.abs(gait) * 0.018 : Math.sin(this.playerAnimationTime * 2) * 0.006);
          this.playerRig.head.rotation.z = moving ? Math.sin(this.playerAnimationTime * 5) * 0.025 : 0;
          for (const { upper, elbow, side } of this.playerRig.arms) {
            upper.rotation.x = this.jumpHeight > 0.02 ? -0.7 : gait * 0.48;
            upper.rotation.z = side * (this.jumpHeight > 0.02 ? 0.18 : 0.04);
            elbow.rotation.x = this.jumpHeight > 0.02 ? -0.35 : -0.08 - Math.max(0, gait) * 0.13;
          }
          for (const { hip, knee, side } of this.playerRig.hips) {
            hip.rotation.x = this.jumpHeight > 0.02 ? (side < 0 ? -0.32 : 0.22) : gait * (side < 0 ? 0.42 : -0.42);
            knee.rotation.x = this.jumpHeight > 0.02 ? 0.4 : Math.max(0, -gait * (side < 0 ? 1 : -1)) * 0.4;
          }
        }
      }
    }
    let forward = Number(this.keys.has('KeyW') || this.keys.has('ArrowUp')) - Number(this.keys.has('KeyS') || this.keys.has('ArrowDown'));
    let right = Number(this.keys.has('KeyD') || this.keys.has('ArrowRight')) - Number(this.keys.has('KeyA') || this.keys.has('ArrowLeft'));
    if (!forward && !right) return;
    const length = Math.hypot(forward, right); forward /= length; right /= length;
    const speed = this.game.player().form === 'F08' ? 2.6 : this.game.player().form === 'F09' ? 1.8 : 4.4;
    const fx = Math.sin(this.yaw), fz = -Math.cos(this.yaw);
    const rx = Math.cos(this.yaw), rz = Math.sin(this.yaw);
    const oldX = this.playerPosition.x, oldZ = this.playerPosition.z;
    let nx = oldX + (fx * forward + rx * right) * speed * dt;
    let nz = oldZ + (fz * forward + rz * right) * speed * dt;
    const radius = this.game.player().form === 'F12' ? 0.38 : this.game.player().form === 'F00' ? 0.22 : 0.3;
    if (this.#blockedAt(nx, nz, radius)) {
      if (!this.#blockedAt(nx, oldZ, radius)) nz = oldZ;
      else if (!this.#blockedAt(oldX, nz, radius)) nx = oldX;
      else return;
    }
    const area = areaAt(nx, nz);
    const from = this.game.player().area;
    if (!area || area !== from && (!this.#canCrossBoundary(from, area, nx, nz) || !this.game.moveToArea('local', area))) return;
    if (area !== this.game.player().area) this.game.player().area = area;
    this.playerPosition.x = nx; this.playerPosition.z = nz;
    this.playerGroup.position.set(this.playerPosition.x, this.game.player().form === 'F00' ? this.jumpHeight : 0, this.playerPosition.z);
    if (this.game.player().form !== 'F00') this.game.item(this.game.player().form).area = area;
  }

  updateCamera(now = performance.now()) {
    const dt = this.lastCameraUpdate ? Math.min(0.1, (now - this.lastCameraUpdate) / 1000) : 1 / 60;
    this.lastCameraUpdate = now;
    const smooth = 1 - Math.exp(-10 * dt);
    if (this.game.player().canSeeWholeMap) {
      this.cameraDesired.set(7, 50, -14);
      this.camera.position.lerp(this.cameraDesired, smooth);
      this.camera.lookAt(this.mapTarget);
      return;
    }
    const distance = this.game.player().form === 'F12' ? 8.5 : this.game.player().form === 'F08' ? 6.2 : this.game.player().form === 'F00' ? 6.5 : 7.2;
    const height = (this.game.player().form === 'F09' ? 3.4 : 3.8) + this.pitch * 1.5;
    const dx = Math.sin(this.yaw) * distance;
    const dz = Math.cos(this.yaw) * distance;
    this.cameraDesired.set(this.playerPosition.x - dx, height, this.playerPosition.z + dz);
    this.camera.position.lerp(this.cameraDesired, smooth);
    this.cameraTarget.set(this.playerPosition.x, 0.78 + this.jumpHeight * 0.35, this.playerPosition.z);
    this.camera.lookAt(this.cameraTarget);
  }

  render() {
    this.updateCamera();
    this.renderer.render(this.scene, this.camera);
  }

  resize() {
    const width = this.canvas.clientWidth || window.innerWidth;
    const height = this.canvas.clientHeight || window.innerHeight;
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height, false);
  }

  getRoomName(id = this.game.player().area) { return ROOM_NAMES[id] ?? id; }
  get roomAt() { return areaAt; }
  get targetPositions() { return OBJECT_POSITIONS; }
  get itemPositions() { return ITEM_POSITIONS; }

  dispose() {
    window.removeEventListener('resize', this.#onResize);
    this.renderer.dispose();
  }
}
