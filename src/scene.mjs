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
    this.camera = new THREE.PerspectiveCamera(48, 1, 0.1, 180);
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: false });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.8));
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
    this.playerPosition = new THREE.Vector3(7, 0, 3.2);
    this.humanPosition = this.playerPosition.clone();
    this.targetMeshes = [];
    this.targetGroups = new Map();
    this.itemGroups = new Map();
    this.playerGroup = null;
    this.playerMaterials = new Map();
    this.cameraTarget = new THREE.Vector3();
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
      case 'S01': this.#box(group, 0, 1.45, 0, 2.6, 2.9, 0.24, this.#material(0x4c665b)); this.#box(group, 0.78, 1.5, 0.15, 0.08, 0.2, 0.08, this.#material(0xc4ef80)); break;
      case 'S02': this.#box(group, 0, 0.48, 0, 0.9, 1.0, 0.8, this.#material(0x414d47)); this.#box(group, 0, 1.02, 0, 1.0, 0.08, 0.88, metal); break;
      case 'S03': this.#box(group, 0, 1.35, 0, 1.25, 2.7, 0.8, this.#material(0x66847c)); this.#box(group, 0, 1.35, 0.43, 0.85, 0.75, 0.04, dark, false); this.#box(group, 0, 0.4, 0.44, 0.75, 0.55, 0.04, this.#material(0xa3ad91), false); break;
      case 'S04': this.#box(group, 0, 0.1, 0, 1.7, 0.2, 0.9, wood); break;
      case 'S05': case 'S09': case 'S19': this.#box(group, 0, 1.35, 0, 2.0, 2.7, 0.2, this.#material(0x53675b)); break;
      case 'S06': this.#box(group, 0, 0.72, 0, 1.4, 0.12, 0.8, wood); this.#box(group, 0, 0.36, 0, 1.2, 0.62, 0.65, wood); break;
      case 'S07': this.#box(group, 0, 0.58, 0, 1.3, 0.95, 0.1, dark); this.#box(group, 0, 0.52, 0.08, 1.12, 0.7, 0.03, this.#material(0x466b63), false); break;
      case 'S08': this.#box(group, 0, 0.3, 0, 1.0, 0.55, 0.7, this.#material(0x97866a)); break;
      case 'S10': this.#box(group, 0, 0.4, 0, 0.85, 0.8, 0.85, this.#material(0x796e5a)); this.#box(group, 0, 0.96, 0, 1.05, 0.16, 1.05, this.#material(0x465947)); break;
      case 'S11': this.#box(group, 0, 0.25, 0, 0.8, 0.5, 1.15, pale); this.#box(group, 0, 0.75, -0.2, 0.75, 0.8, 0.35, pale); break;
      case 'S12': this.#box(group, 0, 0.42, 0, 0.65, 0.82, 0.62, this.#material(0x68766d)); break;
      case 'S13': this.#box(group, 0, 2.7, 0, 2.5, 1.0, 0.12, metal); break;
      case 'S14': this.#box(group, 0, 1.65, 0, 4.6, 2.3, 0.16, this.#material(0x304b43)); break;
      case 'S15': this.#box(group, 0, 1.2, 0, 1.2, 2.2, 0.35, this.#material(0x62685b)); this.#box(group, 0, 1.3, 0.2, 0.18, 0.85, 0.12, this.#material(0x222a25)); break;
      case 'S16': this.#box(group, 0, 0.56, 0, 1.8, 1.1, 0.8, wood); this.#box(group, 0, 1.14, 0, 2.0, 0.12, 0.9, wood); break;
      case 'S17': this.#box(group, 0, 1.55, 0, 2.4, 2.2, 0.13, this.#material(0x79745f)); break;
      case 'S18': this.#box(group, 0, 1.35, 0, 1.3, 1.65, 0.12, pale); break;
      case 'S20': this.#box(group, 0, 0.7, 0, 1.1, 0.85, 0.8, wood); break;
    }
    group.add(this.#label(OBJECTS[id], 0, id === 'S13' ? 3.55 : 3.0, 0.1, 20));
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
      add(new THREE.Mesh(new THREE.CapsuleGeometry(0.38, 0.78, 4, 8), mat(0x92a889)), 0, 0.85);
      add(new THREE.Mesh(new THREE.SphereGeometry(0.29, 16, 12), mat(0xd1bc9a)), 0, 1.62);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8, 0.14, 0.48), mat(0x374a42)), 0, 0.2);
    } else if (id === 'F01') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.35, 0.38, 0.62, 16), mat(0x8b9b8c, 0.55)), 0, 0.45);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.37, 0.37, 0.06, 16), mat(0x4f5e53, 0.4)), 0, 0.77);
    } else if (id === 'F02') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.28, 0.31, 0.8, 16), mat(0x9e7549)), 0, 0.48);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.13, 0.13, 0.14, 12), mat(0xb9ba90)), 0, 0.96);
    } else if (id === 'F03' || id === 'F06') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.76, 0.5, 0.08), mat(id === 'F03' ? 0xc4d4b3 : 0xd3c19b)), 0, 0.4);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.44, 0.06, 0.04), mat(0x566b5d)), 0, 0.44, 0.06);
    } else if (id === 'F04' || id === 'F05' || id === 'F11') {
      const chain = new THREE.Mesh(new THREE.TorusGeometry(id === 'F11' ? 0.36 : 0.2, 0.06, 8, 18), mat(0xb6a16e, 0.55));
      chain.rotation.x = Math.PI / 2;
      add(chain, 0, 0.5);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.55, 0.08, 0.08), mat(0xb6a16e, 0.45)), 0.16, 0.28);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.08, 0.28, 0.08), mat(0xb6a16e, 0.45)), 0.4, 0.18);
    } else if (id === 'F07') {
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.06, 1.8, 8), mat(0x8d886f)), 0, 0.9);
    } else if (id === 'F08') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.72, 0.72, 0.72), mat(0xe4e0c6)), 0, 0.43);
      for (const [x, z] of [[-.18,.18],[.18,-.18],[0,0]]) add(new THREE.Mesh(new THREE.SphereGeometry(0.055, 8, 8), mat(0x34483c)), x, 0.43, z + 0.38);
    } else if (id === 'F09') {
      const ring = new THREE.Mesh(new THREE.TorusGeometry(0.46, 0.09, 10, 28), mat(0xd6c88c, 0.4)); ring.rotation.x = Math.PI / 2; add(ring, 0, 0.88);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.34,0.34,0.08,24), mat(0x47574b)), 0, 0.88);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.06,0.06,0.33), mat(0xd6c88c)), 0, 0.89, -0.07);
    } else if (id === 'F10') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.88,0.56,0.2), mat(0x4d675d,0.18)), 0, 0.68);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.68,0.36,0.04), mat(0x9eb99b)), 0, 0.7, 0.12);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.07,0.07,0.35,10), mat(0x63766a)), 0, 0.22);
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
    target.add(this.#label(FORMS[formId], 0, 2.2, 0, 19));
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
    else if (result.ending === 'E05') this.playerPosition.set(7, 0, 3.2);
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
    for (const [id, group] of [...this.targetGroups, ...this.itemGroups]) {
      if (!group.visible) continue;
      const near = Math.hypot(group.position.x - this.playerPosition.x, group.position.z - this.playerPosition.z) <= 4.5;
      for (const child of group.children) if (child.isSprite) child.visible = near;
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

  updateCamera() {
    if (this.game.player().canSeeWholeMap) {
      this.camera.position.lerp(new THREE.Vector3(7, 50, -14), 0.06);
      this.camera.lookAt(new THREE.Vector3(7, 0, -11));
      return;
    }
    const distance = this.game.player().form === 'F12' ? 10.2 : this.game.player().form === 'F08' ? 7.8 : 9.2;
    const height = (this.game.player().form === 'F09' || this.game.player().form === 'F10' ? 6.0 : 7.4) + this.pitch * 1.5;
    const dx = Math.sin(this.yaw) * distance;
    const dz = Math.cos(this.yaw) * distance;
    const desired = new THREE.Vector3(this.playerPosition.x - dx, height, this.playerPosition.z + dz);
    this.camera.position.lerp(desired, 0.13);
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
