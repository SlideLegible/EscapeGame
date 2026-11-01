import * as THREE from '../vendor/three.module.js';
import { FORMS, OBJECTS, OBJECT_AREAS } from './core.mjs';

const ROOM = {
  Classroom: { label: '密碼學教室', x: 0, z: 0, w: 18, d: 12, color: 0x46564f },
  Corridor: { label: '走廊', x: 0, z: -11, w: 50, d: 8, color: 0x394943 },
  Office: { label: '辦公室', x: 11, z: -21, w: 10, d: 10, color: 0x4d5348 },
  Restroom: { label: '廁所', x: -11, z: -21, w: 10, d: 10, color: 0x485958 },
  Storage: { label: '儲物間', x: -22, z: -21, w: 9, d: 10, color: 0x333e3b },
  GuardRoom: { label: '警衛室', x: 23, z: -21, w: 10, d: 10, color: 0x424e47 },
  L01: { label: '飲料機內部', x: 38, z: -9, w: 9, d: 8, color: 0x365854 },
  L02: { label: '飲料機底下', x: 38, z: -21, w: 9, d: 8, color: 0x3c4946 },
};

const OBJECT_POSITIONS = {
  S01: [0, -14], S02: [-6, -10], S03: [-14, -10], S04: [-18, -10], S05: [11, -15],
  S06: [13, -20], S07: [9, -21], S08: [13, -24], S09: [-15, -21], S10: [4, -10],
  S11: [-12, -19], S12: [-8, -23], S13: [-9, -17], S14: [0, 4], S15: [25, -22],
  S16: [-4, 0], S17: [-3, -10], S18: [9, -24], S19: [0, -5], S20: [-4, -1],
};

const ITEM_POSITIONS = {
  F01: [-6, -10], F02: [38, -9], F03: [38, -21], F04: [4, -10], F05: [13, -20],
  F06: [13, -24], F07: [-22, -21], F08: [2, 1], F09: [5, 2], F10: [25, -22],
  F11: [21, -22], F12: [7, 1.5],
};

const ROOM_NAMES = Object.fromEntries(Object.entries(ROOM).map(([id, room]) => [id, room.label]));
const palette = { wall: 0x8c9883, trim: 0x273a36, wood: 0x806e52, light: 0xc8d7bb, dark: 0x1a2928, lime: 0xc4ef80 };

export function areaAt(x, z) {
  if (Math.abs(x) <= 9 && z >= -6 && z <= 6) return 'Classroom';
  if (x >= 6 && x <= 16 && z >= -26 && z <= -15.8) return 'Office';
  if (x >= -16 && x <= -6 && z >= -26 && z <= -15.8) return 'Restroom';
  if (x >= -27 && x <= -16.2 && z >= -26 && z <= -16) return 'Storage';
  if (x >= 18 && x <= 28 && z >= -26 && z <= -15.8) return 'GuardRoom';
  if (x >= -25 && x <= 25 && z >= -16.2 && z <= -5.8) return 'Corridor';
  if (x >= 33 && x <= 43 && z >= -14 && z <= -5) return 'L01';
  if (x >= 33 && x <= 43 && z >= -25 && z <= -16) return 'L02';
  return null;
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
    this.yaw = 0.9;
    this.pitch = 0.28;
    this.pointerDown = null;
    this.started = false;
    this.playerPosition = new THREE.Vector3(3.5, 0, 0);
    this.humanPosition = this.playerPosition.clone();
    this.targetMeshes = [];
    this.targetGroups = new Map();
    this.itemGroups = new Map();
    this.playerGroup = null;
    this.playerMaterials = new Map();
    this.cameraTarget = new THREE.Vector3();
    this.cameraDesired = new THREE.Vector3();
    this.mapTarget = new THREE.Vector3(7, 0, -11);
    this.lastCameraUpdate = 0;
    this.#buildLights();
    this.#buildRooms();
    this.#buildObjects();
    this.#buildPlayer();
    this.#onResize = () => this.resize();
    window.addEventListener('resize', this.#onResize);
    this.resize();
    this.refresh();
  }

  #buildLights() {
    this.scene.add(new THREE.HemisphereLight(0xc4d9cc, 0x202924, 2.25));
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
    const wallMaterial = this.#material(0x7c8978);
    for (const [id, room] of Object.entries(ROOM)) {
      const group = new THREE.Group();
      group.name = id;
      this.scene.add(group);
      const floorMat = this.#material(room.color);
      const floor = new THREE.Mesh(new THREE.BoxGeometry(room.w, 0.28, room.d), floorMat);
      floor.position.set(room.x, -0.17, room.z);
      floor.receiveShadow = true;
      group.add(floor);
      const border = this.#material(0x253431);
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
        wall(left, room.z + 3.25, 0.22, room.d / 2 - 3.25);
        wall(left, room.z - 3.25, 0.22, room.d / 2 - 3.25);
        wall(right, room.z, 0.22, room.d);
        wall((left + room.x - 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
        wall((right + room.x + 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
      } else if (id === 'Storage') {
        wall(room.x, top, room.w, 0.22); wall(left, room.z, 0.22, room.d); wall(room.x, bottom, room.w, 0.22);
        wall(right, room.z - 1.5, 0.22, room.d - 3);
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
    const hallwayMat = this.#material(0x7d836c);
    this.#box(this.scene, 0, -0.26, -11, 49, 0.08, 0.45, hallwayMat, false);
    const lampMat = new THREE.MeshStandardMaterial({ color: 0xd7e2ba, emissive: 0x91a665, emissiveIntensity: 1.5 });
    for (const x of [-18, -8, 2, 12, 22]) {
      const lamp = new THREE.Mesh(new THREE.BoxGeometry(2.2, 0.08, 0.35), lampMat);
      lamp.position.set(x, 3.15, -11);
      this.scene.add(lamp);
    }
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
      case 'S04': {
        const pot = new THREE.Mesh(new THREE.CylinderGeometry(0.58, 0.42, 0.68, 16), this.#material(0x987e60));
        pot.position.y = 0.42; pot.castShadow = true; group.add(pot);
        this.#box(group, 0, 0.79, 0, 1.2, 0.12, 0.85, wood);
        const stem = this.#material(0x587c58);
        for (const [x, y, z, scale] of [[-0.2,1.35,0,0.7],[0.2,1.55,0.08,0.6],[0,1.8,-0.05,0.65],[-0.4,1.65,0.04,0.5]]) {
          const leaf = new THREE.Mesh(new THREE.SphereGeometry(0.34, 10, 8), stem);
          leaf.position.set(x, y, z); leaf.scale.set(1, scale, 0.7); leaf.castShadow = true; group.add(leaf);
        }
        break;
      }
      case 'S05': case 'S09': case 'S19': {
        const door = this.#material(id === 'S09' ? 0x5d6b5e : 0x53675b);
        this.#box(group, 0, 1.4, 0, 2.18, 2.82, 0.2, this.#material(0xb2b69c));
        this.#box(group, 0, 1.36, 0.12, 1.92, 2.62, 0.16, door);
        this.#box(group, 0, 1.56, 0.22, 1.56, 1.94, 0.055, this.#material(0x46584d));
        this.#box(group, 0, 1.56, 0.255, 1.36, 1.74, 0.035, door, false);
        const knob = new THREE.Mesh(new THREE.SphereGeometry(0.075, 12, 10), this.#material(0xd2b776, 0.35));
        knob.position.set(0.68, 1.28, 0.31); group.add(knob);
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
        this.#box(group, 0, 1.65, 0, 4.8, 2.5, 0.16, wood);
        this.#box(group, 0, 1.65, 0.1, 4.48, 2.18, 0.04, this.#material(0x29443c), false);
        this.#box(group, 0, 0.35, 0.1, 4.65, 0.12, 0.24, wood);
        const chalk = document.createElement('canvas'); chalk.width = 1024; chalk.height = 320;
        const ctx = chalk.getContext('2d');
        ctx.fillStyle = '#304b43'; ctx.fillRect(0, 0, chalk.width, chalk.height);
        ctx.fillStyle = '#d8d8bf'; ctx.font = '38px Georgia, serif'; ctx.fillText('WKH SDVVZRUG LV QRW KHUH', 48, 130);
        ctx.font = '21px sans-serif'; ctx.fillStyle = '#9eb5a2'; ctx.fillText('Caesar cipher  ·  shift 3', 50, 195);
        const boardMap = new THREE.CanvasTexture(chalk); boardMap.colorSpace = THREE.SRGBColorSpace;
        this.#box(group, 0, 1.67, 0.14, 4.38, 2.08, 0.025, new THREE.MeshStandardMaterial({ map: boardMap, roughness: 0.95 }), false);
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
        for (const x of [-0.72, 0.72]) for (const z of [-0.25, 0.25]) this.#box(group, x, 0.5, z, 0.14, 0.98, 0.14, this.#material(0x655740));
        this.#box(group, 0, 0.72, 0, 1.95, 0.62, 0.76, wood);
        this.#box(group, 0, 1.06, 0, 2.15, 0.14, 0.94, this.#material(0x918064));
        this.#box(group, 0, 1.08, 0.49, 0.48, 0.08, 0.04, this.#material(0x4d5d53), false);
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
        this.#box(group, 0, 0.46, 0, 1.18, 0.82, 0.82, this.#material(0x75664e));
        this.#box(group, 0, 0.93, 0, 1.3, 0.14, 0.94, wood);
        this.#box(group, 0, 0.63, 0.43, 0.88, 0.3, 0.08, this.#material(0x8d7a5a));
        this.#box(group, 0, 0.65, 0.49, 0.22, 0.04, 0.035, metal, false);
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
      group.userData.targetId = id;
      group.name = `${id} ${label}`;
      this.scene.add(group);
      this.targetGroups.set(id, group);
      this.targetMeshes.push(group);
    }
    for (const id of Object.keys(FORMS).filter((formId) => formId !== 'F00')) {
      const group = new THREE.Group();
      group.userData.targetId = id;
      group.name = `${id} ${FORMS[id]}`;
      this.scene.add(group);
      this.itemGroups.set(id, group);
      this.targetMeshes.push(group);
      this.#rebuildFormMesh(group, id);
    }
  }

  #formShape(id) {
    const group = new THREE.Group();
    const mat = (color, metalness = 0.04) => new THREE.MeshStandardMaterial({ color, roughness: 0.62, metalness });
    const add = (mesh, x = 0, y = 0, z = 0) => { mesh.position.set(x, y, z); mesh.castShadow = true; group.add(mesh); };
    if (id === 'F00') {
      const coat = mat(0x718b76), skin = mat(0xc9ac8d), hair = mat(0x352f2b), trousers = mat(0x394b49), shoes = mat(0x263632);
      add(new THREE.Mesh(new THREE.CapsuleGeometry(0.36, 0.7, 5, 12), coat), 0, 0.92);
      add(new THREE.Mesh(new THREE.SphereGeometry(0.3, 20, 16), skin), 0, 1.66);
      const cap = new THREE.Mesh(new THREE.SphereGeometry(0.305, 18, 8, 0, Math.PI * 2, 0, Math.PI * 0.52), hair);
      add(cap, 0, 1.79);
      for (const side of [-1, 1]) {
        const arm = new THREE.Mesh(new THREE.CapsuleGeometry(0.105, 0.5, 4, 8), coat);
        arm.rotation.z = side * -0.16; add(arm, side * 0.39, 0.91);
        const leg = new THREE.Mesh(new THREE.CapsuleGeometry(0.13, 0.45, 4, 8), trousers);
        add(leg, side * 0.17, 0.3);
        add(new THREE.Mesh(new THREE.BoxGeometry(0.25, 0.12, 0.38), shoes), side * 0.17, 0.08, -0.05);
      }
      add(new THREE.Mesh(new THREE.BoxGeometry(0.12, 0.4, 0.09), mat(0xc4ef80)), 0.12, 1.0, -0.31);
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
    } else if (id === 'F10') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.92,0.52,0.56), mat(0x4d675d,0.18)), 0, 0.77, -0.08);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.2,0.23,0.34,18), mat(0x374c45,0.28)), 0, 0.73, 0.34);
      const lens = new THREE.Mesh(new THREE.CylinderGeometry(0.14,0.14,0.045,18), mat(0x789b91,0.18)); lens.rotation.x = Math.PI / 2; add(lens,0,0.73,0.53);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.5,0.09,0.62), mat(0x9eb99b)), 0, 0.83, -0.08);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.07,0.07,0.3,10), mat(0x63766a)), 0, 0.31, -0.04);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.42,0.08,0.36), mat(0x63766a)), 0, 0.12, -0.04);
    } else if (id === 'F12') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8,0.13,0.75), mat(0x85765d)), 0, 0.58);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8,0.75,0.14), mat(0x85765d)), 0, 1.0, -0.3);
      for (const x of [-0.3,0.3]) for (const z of [-0.27,0.27]) add(new THREE.Mesh(new THREE.BoxGeometry(0.08,0.55,0.08), mat(0x71644f)), x, 0.27, z);
    }
    return group;
  }

  #rebuildFormMesh(target, formId) {
    for (const child of [...target.children]) {
      target.remove(child);
      child.geometry?.dispose();
      const materials = Array.isArray(child.material) ? child.material : [child.material];
      for (const material of materials) {
        material?.map?.dispose();
        material?.dispose();
      }
    }
    const shape = this.#formShape(formId);
    while (shape.children.length) target.add(shape.children[0]);
  }

  #buildPlayer() {
    this.playerGroup = this.#formShape('F00');
    this.playerGroup.name = '玩家';
    this.playerGroup.position.copy(this.playerPosition);
    this.scene.add(this.playerGroup);
    this.#rebuildFormMesh(this.playerGroup, 'F00');
  }

  setForm(formId) {
    this.#rebuildFormMesh(this.playerGroup, formId);
  }

  syncAfterAction(previousForm, result) {
    const player = this.game.player();
    if (previousForm === 'F00' && player.form !== 'F00') this.humanPosition.copy(this.playerPosition);
    if (result.location === 'L01') this.playerPosition.set(38, 0, -9);
    else if (result.location === 'L02') this.playerPosition.set(38, 0, -21);
    else if (result.ending === 'E05') this.playerPosition.set(3.5, 0, 0);
    else if (!result.ending && player.form === 'F00' && previousForm !== 'F00') this.playerPosition.copy(this.humanPosition);
    this.playerGroup.position.copy(this.playerPosition);
    this.setForm(player.form);
    this.refresh();
  }

  refresh() {
    for (const [id, group] of this.itemGroups) {
      const item = this.game.item(id);
      const isPlayer = this.game.player().form === id;
      group.visible = Boolean(item?.exists && !item.occupiedBy && !isPlayer);
      if (group.visible && item.area) {
        const pos = ITEM_POSITIONS[id];
        const room = ROOM[item.area];
        if (room && ['F05', 'F06', 'F07'].includes(id) && item.area !== (id === 'F05' || id === 'F06' ? 'Office' : 'Storage')) {
          group.position.set(room.x, 0, room.z);
        } else if (pos) group.position.set(pos[0], 0, pos[1]);
      }
    }
    for (const doorId of ['S05', 'S09', 'S19']) {
      const door = this.targetGroups.get(doorId);
      if (door) door.visible = !this.game.isDoorOpen(doorId);
    }
    const nearest = this.nearestTarget();
    for (const [id, group] of [...this.targetGroups, ...this.itemGroups]) {
      if (!group.visible) continue;
      for (const child of group.children) if (child.isSprite) child.visible = nearest?.id === id;
    }
    this.playerGroup.visible = true;
  }

  nearestTarget(maxDistance = 3.7) {
    let best = null;
    let bestDistance = maxDistance;
    for (const [id, group] of this.targetGroups) {
      if (!group.visible) continue;
      if (OBJECT_AREAS[id] !== this.game.player().area) continue;
      const distance = Math.hypot(group.position.x - this.playerPosition.x, group.position.z - this.playerPosition.z);
      if (distance < bestDistance) { bestDistance = distance; best = { id, name: OBJECTS[id], distance }; }
    }
    for (const [id, group] of this.itemGroups) {
      if (!group.visible) continue;
      const distance = Math.hypot(group.position.x - this.playerPosition.x, group.position.z - this.playerPosition.z);
      if (distance < bestDistance) { bestDistance = distance; best = { id, name: FORMS[id], distance }; }
    }
    return best;
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

  move(dt) {
    if (!this.started || this.game.player().ending) return;
    let forward = Number(this.keys.has('KeyW') || this.keys.has('ArrowUp')) - Number(this.keys.has('KeyS') || this.keys.has('ArrowDown'));
    let right = Number(this.keys.has('KeyD') || this.keys.has('ArrowRight')) - Number(this.keys.has('KeyA') || this.keys.has('ArrowLeft'));
    if (!forward && !right) return;
    const length = Math.hypot(forward, right); forward /= length; right /= length;
    const speed = this.game.player().form === 'F08' ? 2.6 : this.game.player().form === 'F09' || this.game.player().form === 'F10' ? 1.8 : 4.4;
    const fx = Math.sin(this.yaw), fz = -Math.cos(this.yaw);
    const rx = Math.cos(this.yaw), rz = Math.sin(this.yaw);
    const nx = this.playerPosition.x + (fx * forward + rx * right) * speed * dt;
    const nz = this.playerPosition.z + (fz * forward + rz * right) * speed * dt;
    const area = areaAt(nx, nz);
    if (!area || area !== this.game.player().area && !this.game.moveToArea('local', area)) return;
    if (area !== this.game.player().area) this.game.player().area = area;
    this.playerPosition.x = nx; this.playerPosition.z = nz;
    this.playerGroup.position.copy(this.playerPosition);
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
    const distance = this.game.player().form === 'F12' ? 8.5 : this.game.player().form === 'F08' ? 6.2 : 7.2;
    const height = (this.game.player().form === 'F09' || this.game.player().form === 'F10' ? 3.4 : 3.8) + this.pitch * 1.5;
    const dx = Math.sin(this.yaw) * distance;
    const dz = Math.cos(this.yaw) * distance;
    this.cameraDesired.set(this.playerPosition.x - dx, height, this.playerPosition.z + dz);
    this.camera.position.lerp(this.cameraDesired, smooth);
    this.cameraTarget.set(this.playerPosition.x, 1.15, this.playerPosition.z);
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
