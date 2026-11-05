import * as THREE from '../vendor/three.module.js';
import { FORMS, OBJECTS, OBJECT_AREAS, LOCKED_DOOR_TEXT } from './core.mjs';
import { loadGlb } from './glb.mjs';

const ROOM = {
  Classroom: { label: '密碼學教室', x: 0, z: 0, w: 18, d: 12, color: 0x8b7859 },
  Corridor: { label: '走廊', x: 0, z: -11, w: 50, d: 8, color: 0x756b58 },
  Office: { label: '辦公室', x: 4.1, z: -21, w: 10, d: 10, color: 0x766c58 },
  Restroom: { label: '廁所', x: -11, z: -21, w: 10, d: 10, color: 0x626b66 },
  GuardRoom: { label: '警衛室', x: 14.4, z: -21, w: 10, d: 10, color: 0x696553 },
  L01: { label: '飲料機內部', x: 38, z: -9, w: 9, d: 8, color: 0x515f58 },
  L02: { label: '飲料機底下', x: 38, z: -21, w: 9, d: 8, color: 0x50564e },
};

export const OBJECT_POSITIONS = {
  S01: [0, -14], S02: [-6, -10], S03: [-14, -10], S04: [-14, -10.7], S05: [4.08, -13.8],
  S06: [6.1, -20], S07: [5.6, -20], S08: [6.1, -24], S09: [-6.85, -18], S10: [4, -10],
  S11: [-12, -19], S12: [-8, -23], S13: [-9, -17], S14: [-7.65, 0], S15: [19.08, -22],
  S16: [-6.32, 0], S17: [-3, -10], S18: [2.1, -24], S19: [-5.55, -5.45], S20: [-5.65, 0.35],
  S21: [13.9, -22],
};

export const ITEM_POSITIONS = {
  F01: [-4.8, -11], F02: [38, -9], F03: [38, -21], F04: [4.8, -9.3], F05: [5.1, -20.8],
  F06: [4.9, -24], F07: [-7.7, -18.2], F08: [-5.2, 0.9], F09: [-5, 2.2],
  F11: [11.9, -22], F12: [-5, -2.8],
};

// Item models packed from Sketchfab downloads (the pack tool lives on the art/classroom-style-prompt branch). `nodes` keeps only the named
// nodes of a multi-object pack; `rotate` stands the model upright; `fit` is the largest
// dimension in world units.
const ITEM_MODELS = {
  F01: { nodes: ['Soda_Trash'], fit: 0.6 },
  F02: { nodes: ['Can'], fit: 0.9 },
  F03: { fit: 0.8 },
  F04: { rotate: [-Math.PI / 2, 0, 0], fit: 0.8 },
  F05: { rotate: [0, 0, Math.PI / 2], fit: 0.8 },
  F06: { nodes: ['SM_PaperStack_13'], fit: 1.0 },
  F07: { fit: 1.9 },
  F08: { fit: 0.72 },
  F09: { fit: 0.95 },
  F11: { fit: 0.8 },
  F12: { fit: 1.4 },
};

// Room models from Sketchfab (downloaded to assets/models/rooms/, see ATTRIBUTION.sketchfab.txt). `fitFootprint` scales to the room's
// floor minus a margin; `scale` is used as-is (the desk and closet files are in centimetres); `fitMax` sets the largest dimension.
// `x`/`z` default to the room centre and `y` to the floor. `replaces` lists the interaction targets whose procedural shapes are hidden; `doorOf` ties the model's visibility to that door's open state; `alignMaxX` slides the model so its largest-x edge touches that x (wall mounting).
export const ROOM_MODELS = [
  { room: 'Corridor', file: 'rooms/corridor.glb', rotateY: Math.PI / 2, fitFootprint: true, mirrorNorthWall: true },
  { room: 'Restroom', file: 'rooms/restroom.glb', fitFootprint: true },
  { room: 'Office', file: 'rooms/computer_desk.glb', scale: 0.01, x: 4.1, z: -21.5 },
  { room: 'Office', file: 'rooms/bulletin_board.glb', rotateY: Math.PI / 2, x: -0.65, y: 1.2, z: -21 },
  { room: 'GuardRoom', file: 'rooms/guard_room.glb', fitFootprint: true },
  { room: 'GuardRoom', file: 'items/f10.glb', rotateY: -Math.PI / 2, fitMax: 1.0, x: 13.9, z: -22, replaces: ['S21'] },
  // Interaction-object models (assets/models/objects/sNN.glb, packed by scripts/pack-scene-objects.py). Sizes follow the old procedural shapes
  // because interaction range and collision still use their bounds. S04 (under the vending machine) reuses the S03 model.
  { room: 'Corridor', file: 'objects/s01.glb', fitMax: 2.9, x: 0, z: -14, replaces: ['S01'] },
  { room: 'Corridor', file: 'objects/s02.glb', fitMax: 1.0, x: -6, z: -10, replaces: ['S02'] },
  { room: 'Corridor', file: 'objects/s03.glb', fitMax: 2.76, x: -14, z: -10, replaces: ['S03', 'S04'] },
  { room: 'Corridor', file: 'objects/s10.glb', fitMax: 1.1, x: 4, z: -10, replaces: ['S10'] },
  { room: 'Corridor', file: 'objects/s17.glb', fitMax: 2.4, y: 0.85, x: -3, z: -10, replaces: ['S17'] },
  { room: 'Office', file: 'objects/s06.glb', fitMax: 1.6, x: 6.1, z: -20, replaces: ['S06'] },
  { room: 'Office', file: 'objects/s07.glb', rotateY: -Math.PI / 2, fitMax: 0.7, y: 0.785, x: 5.6, z: -20, replaces: ['S07'] }, // on the S06 desk top
  { room: 'GuardRoom', file: 'objects/s15.glb', rotateY: -Math.PI / 2, fitMax: 1.2, y: 0.9, z: -22, alignMaxX: 19.29, replaces: ['S15'] },
  // Storage door on the restroom's east wall, right of the entrance (inner face x = -6.64), facing into the room; the model hides while the door is open (`doorOf`).
  { room: 'Restroom', file: 'objects/s09.glb', rotateY: -Math.PI / 2, fitMax: 2.8, z: -18, alignMaxX: -6.7, replaces: ['S09'], doorOf: 'S09' },
  { room: 'Office', file: 'objects/s08.glb', rotateY: Math.PI / 2, fitMax: 1.1, x: 6.1, z: -24, replaces: ['S08'] },
  { room: 'Restroom', file: 'objects/s12.glb', fitMax: 0.9, x: -8, z: -23, replaces: ['S12'] },
  // High window on the restroom's north wall (inner face z = -16.3), facing into the room.
  { room: 'Restroom', file: 'objects/s13.glb', rotateY: Math.PI, fitMax: 1.5, y: 2.1, x: -9, z: -16.4, replaces: ['S13'] },
  { room: 'Office', file: 'objects/s18.glb', fitMax: 1.8, y: 0.5, x: 2.1, z: -24, replaces: ['S18'] },
];

// Footprint (x, z) of the teacher's lectern in the bundled classroom, parallel to the blackboard.
export const LECTERN_SIZE = [0.9, 1.48];

// Walkable collision rows measured from the bundled classroom's desk and chair layout.
const CLASSROOM_DESK_COLUMNS = [-3.77, -1.67, 0.41, 2.43, 4.45, 6.5];
const CLASSROOM_DESK_ROWS = [-3.96, -1.96, -0.01, 1.93, 3.89];
const CLASSROOM_EXIT_DOOR_PARTS = ['PUERTA_1_StingrayPBS9_0', 'PUERTA_3_StingrayPBS9_0', 'GRIPPUERTA_StingrayPBS1_0', 'GRIPPUERTA2_StingrayPBS1_0'];
const MODEL_TARGETS = new Set(['S04', 'S05', 'S09', 'S14', 'S16', 'S19', 'S20']);

const ROOM_NAMES = Object.fromEntries(Object.entries(ROOM).map(([id, room]) => [id, room.label]));
// Orbit camera collision: how far into the ray the player's own body is ignored, how far to stay off a wall, and the closest it may get.
const CAMERA_BODY_CLEARANCE = 0.6, CAMERA_WALL_CLEARANCE = 0.3, CAMERA_MIN_DISTANCE = 0.45;
const PLAYER_TURN_SPEED = 18;
const palette = { wall: 0xb3a789, trim: 0x51483b, wood: 0x8d704a, light: 0xe8d8b6, dark: 0x292720, lime: 0xc4ef80 };

// Walkable rectangles per area, checked in order (the first match wins where they overlap).
const AREA_RECTS = [
  ['Classroom', -7.75, 7.75, -5.75, 5.75],
  ['Office', -0.9, 9.1, -26, -15.8],
  ['Restroom', -16, -6, -26, -15.8],
  ['GuardRoom', 9.4, 19.4, -26, -15.8],
  ['Corridor', -25, 25, -16.2, -5.75],
  ['L01', 33, 43, -14, -5],
  ['L02', 33, 43, -25, -16],
];

// Passages between rooms. Each room's entrance is on its own wall (`line`, measured on the room models) across `span`:
// walking past it moves the player to the other room's arrival point. `axis` is the coordinate the line is on, and
// `height` keeps a black block under a low door frame.
// The corridor's entrances are the doors painted on its walls, so they get no black block.
export const DOORWAYS = [
  {
    door: 'S19', axis: 'z', sides: {
      Classroom: { line: -5.3, span: [-7.05, -4.12], spawn: [-5.6, -4.4], height: 2.45 },
      Corridor: { line: -7.95, span: [-6.96, -5.56], spawn: [-6.26, -9.0], block: false }
    }
  },
  {
    door: 'S05', axis: 'z', sides: {
      Corridor: { line: -13.72, span: [3.38, 4.78], spawn: [4.08, -12.7], block: false },
      Office: { line: -16.05, span: [2.8, 5.4], spawn: [4.1, -17.1] }
    }
  },
  {
    door: null, axis: 'z', sides: {
      Corridor: { line: -13.72, span: [-6.96, -5.56], spawn: [-6.26, -12.7], block: false },
      Restroom: { line: -16.42, span: [-12.3, -9.7], spawn: [-11, -17.4] }
    }
  },
  {
    door: null, axis: 'z', sides: {
      Corridor: { line: -13.72, span: [13.74, 15.14], spawn: [14.44, -12.7], block: false },
      GuardRoom: { line: -16.05, span: [13.1, 15.7], spawn: [14.4, -17.1] }
    }
  },
];

// The corridor model's south wall (windows) is replaced by a mirror of its north wall, so both walls have doors.
const CORRIDOR_MID_Z = -10.88;

// The corridor model's main hall ends here; past it are the stairwells, which the player does not walk into.
export const CORRIDOR_HALF_LENGTH = 15.3;

// How close to a doorway's line the player has to get for it to open; the room's own wall keeps them that far back.
const DOORWAY_REACH = 0.35;

// Rooms whose model walls sit inside their floor rectangle draw only up to those walls, so their windows show black.
const CLIP_OVERRIDES = { Corridor: { minZ: -14.3, maxZ: -7.7 } };

// The part of the world drawn while the player is in `area`: the room and its walkable floor plus its walls.
export function roomClipRect(area, margin = 0.35) {
  const rect = AREA_RECTS.find(([id]) => id === area);
  const room = ROOM[area];
  if (!rect || !room) return null;
  return {
    minX: Math.min(rect[1], room.x - room.w / 2) - margin, maxX: Math.max(rect[2], room.x + room.w / 2) + margin,
    minZ: Math.min(rect[3], room.z - room.d / 2) - margin, maxZ: Math.max(rect[4], room.z + room.d / 2) + margin,
    ...CLIP_OVERRIDES[area],
  };
}

export function areaAt(x, z) {
  for (const [id, minX, maxX, minZ, maxZ] of AREA_RECTS) {
    if (x >= minX && x <= maxX && z >= minZ && z <= maxZ) return id;
  }
  return null;
}

// The floor rectangle the orbit camera may occupy while the player is in `area`: the walkable rectangle cut down to
// the room's real floor, pulled in by `margin` so the camera stays clear of the walls.
export function areaCameraRect(area, margin = 0.3) {
  const rect = AREA_RECTS.find(([id]) => id === area);
  const room = ROOM[area];
  if (!rect || !room) return null;
  return {
    minX: Math.max(rect[1], room.x - room.w / 2) + margin, maxX: Math.min(rect[2], room.x + room.w / 2) - margin,
    minZ: Math.max(rect[3], room.z - room.d / 2) + margin, maxZ: Math.min(rect[4], room.z + room.d / 2) - margin,
  };
}

// Largest t in [0, 1] for which (ox + t*dx, oz + t*dz) is still inside `rect`. An origin outside the rect is not limited.
export function rayRectLimit(ox, oz, dx, dz, rect) {
  if (ox < rect.minX || ox > rect.maxX || oz < rect.minZ || oz > rect.maxZ) return 1;
  let t = 1;
  if (dx > 1e-9) t = Math.min(t, (rect.maxX - ox) / dx); else if (dx < -1e-9) t = Math.min(t, (rect.minX - ox) / dx);
  if (dz > 1e-9) t = Math.min(t, (rect.maxZ - oz) / dz); else if (dz < -1e-9) t = Math.min(t, (rect.minZ - oz) / dz);
  return Math.max(0, t);
}

// Coarse voxel copy of the room geometry (walls, furniture) that the orbit camera must not sink into.
// Floors and ceilings are skipped (the camera always sits above the first and the player never sees the second).
export class OcclusionGrid {
  constructor({ minX, minY, minZ, maxX, maxY, maxZ }, cell = 0.2) {
    this.cell = cell;
    this.minX = minX; this.minY = minY; this.minZ = minZ;
    this.nx = Math.ceil((maxX - minX) / cell); this.ny = Math.ceil((maxY - minY) / cell); this.nz = Math.ceil((maxZ - minZ) / cell);
    this.cells = new Uint8Array(this.nx * this.ny * this.nz);
  }

  #mark(x, y, z) {
    const i = Math.floor((x - this.minX) / this.cell), j = Math.floor((y - this.minY) / this.cell), k = Math.floor((z - this.minZ) / this.cell);
    if (i < 0 || j < 0 || k < 0 || i >= this.nx || j >= this.ny || k >= this.nz) return;
    this.cells[(j * this.nz + k) * this.nx + i] = 1;
  }

  isBlocked(x, y, z) {
    const i = Math.floor((x - this.minX) / this.cell), j = Math.floor((y - this.minY) / this.cell), k = Math.floor((z - this.minZ) / this.cell);
    if (i < 0 || j < 0 || k < 0 || i >= this.nx || j >= this.ny || k >= this.nz) return false;
    return this.cells[(j * this.nz + k) * this.nx + i] === 1;
  }

  addTriangle(ax, ay, az, bx, by, bz, cx, cy, cz) {
    const ux = bx - ax, uy = by - ay, uz = bz - az, vx = cx - ax, vy = cy - ay, vz = cz - az;
    const nxv = uy * vz - uz * vy, nyv = uz * vx - ux * vz, nzv = ux * vy - uy * vx;
    const nLen = Math.hypot(nxv, nyv, nzv);
    if (nLen < 1e-12 || Math.abs(nyv) / nLen > 0.85) return;
    const longest = Math.max(Math.hypot(ux, uy, uz), Math.hypot(vx, vy, vz), Math.hypot(cx - bx, cy - by, cz - bz));
    const n = Math.max(1, Math.ceil(longest / (this.cell * 0.6)));
    for (let i = 0; i <= n; i++) for (let j = 0; j <= n - i; j++) {
      const p = i / n, q = j / n;
      this.#mark(ax + ux * p + vx * q, ay + uy * p + vy * q, az + uz * p + vz * q);
    }
  }

  // Fraction of the way from (ox, oy, oz) to (ex, ey, ez) at which the first blocked voxel is met, or 1 if the path is clear.
  // The first `from` fraction of the path is ignored (the player's own body).
  firstHit(ox, oy, oz, ex, ey, ez, from = 0) {
    const length = Math.hypot(ex - ox, ey - oy, ez - oz);
    if (!(length > 1e-9) || !Number.isFinite(length)) return 1;
    const step = this.cell * 0.5 / length;
    for (let t = from; t < 1; t += step) {
      if (this.isBlocked(ox + (ex - ox) * t, oy + (ey - oy) * t, oz + (ez - oz) * t)) return t;
    }
    return 1;
  }
}

export function bodyPathIntersectsGrid(grid, fromX, fromZ, toX, toZ, radius = 0.3, height = 1.3, baseY = 0) {
  const cell = grid?.cell ?? 0.2;
  const verticalSteps = Math.max(1, Math.ceil(height / cell));
  const pathLength = Math.hypot(toX - fromX, toZ - fromZ);
  const pathSteps = Math.max(1, Math.ceil(pathLength / (cell * 0.4)));
  for (let step = 1; step <= pathSteps; step++) {
    const fraction = step / pathSteps;
    const x = fromX + (toX - fromX) * fraction;
    const z = fromZ + (toZ - fromZ) * fraction;
    for (let level = 0; level <= verticalSteps; level++) {
      const y = baseY + Math.max(0.1, Math.min(height, level * height / verticalSteps));
      if (grid.isBlocked(x, y, z)) return true;
      for (let around = 0; around < 8; around++) {
        const angle = around * Math.PI / 4;
        if (grid.isBlocked(x + Math.cos(angle) * radius, y, z + Math.sin(angle) * radius)) return true;
      }
    }
  }
  return false;
}

export function bodyPathStepHeight(grid, fromX, fromZ, toX, toZ, radius = 0.3, height = 1.3, baseY = 0, maxStep = 0.25) {
  if (!bodyPathIntersectsGrid(grid, fromX, fromZ, toX, toZ, radius, height, baseY)) return baseY;
  const increment = grid.cell * 0.25;
  for (let lift = increment; lift <= maxStep + 1e-6; lift += increment) {
    if (!bodyPathIntersectsGrid(grid, fromX, fromZ, toX, toZ, radius, height, baseY + lift)) return baseY + lift;
  }
  return null;
}

export function advancePlayerFacing(currentYaw, targetYaw, maxTurn) {
  let delta = (targetYaw - currentYaw + Math.PI) % (Math.PI * 2);
  if (delta < 0) delta += Math.PI * 2;
  delta -= Math.PI;
  if (Math.abs(Math.abs(delta) - Math.PI) < 1e-6) delta = -Math.PI;
  if (Math.abs(delta) <= maxTurn) return { yaw: targetYaw, turning: false };
  return { yaw: currentYaw + Math.sign(delta) * maxTurn, turning: true };
}

export function removeRootTranslationTrack(clip, root) {
  if (!clip?.tracks || !root?.uuid) return false;
  const rootPosition = `${root.uuid}.position`;
  const tracks = clip.tracks.filter((track) => track.name !== rootPosition);
  const removed = tracks.length !== clip.tracks.length;
  if (removed) clip.tracks = tracks;
  return removed;
}

// Physics airtime of a jump (2 * jumpVelocity / gravity) and the clip time of the landing pose.
export const JUMP_AIRTIME = (2 * 4.3) / 11.5;
// The clip's on-ground wind-up is skipped: the scrub runs from feet-off-ground to touchdown.
export const JUMP_CLIP_TAKEOFF_TIME = 0.55;
export const JUMP_CLIP_LAND_TIME = 1.07;
// Long enough for the landing crouch (bottom near clip time 1.27) to show before the walk pose returns.
const JUMP_FADE_SECONDS = 0.35;
const STAND_FADE_SECONDS = 0.2;

export function pickPlayerClips(clips = []) {
  const withTracks = clips.filter((clip) => clip.tracks.length > 0);
  const byName = (name) => withTracks.find((clip) => clip.name === name) ?? null;
  return { walk: byName('Walking') ?? withTracks[0] ?? null, jump: byName('Jumping'), idle: byName('Idle') };
}

export function jumpClipTime(airElapsed, airtime = JUMP_AIRTIME, landTime = JUMP_CLIP_LAND_TIME, takeoffTime = JUMP_CLIP_TAKEOFF_TIME) {
  return takeoffTime + Math.min(1, Math.max(0, airElapsed / airtime)) * (landTime - takeoffTime);
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

// Scales and moves a room model as its ROOM_MODELS entry says.
export function placeRoomModel(model, { room, rotateY = 0, scale = 1, fitFootprint, fitMax, x, y = 0, z, alignMaxX }) {
  const area = ROOM[room];
  model.rotation.y = rotateY;
  model.updateMatrixWorld(true);
  const size = new THREE.Box3().setFromObject(model).getSize(new THREE.Vector3());
  let factor = scale;
  if (fitFootprint) factor = Math.min((area.w - 0.6) / size.x, (area.d - 0.6) / size.z);
  if (fitMax) factor = fitMax / Math.max(size.x, size.y, size.z);
  model.scale.setScalar(factor);
  model.updateMatrixWorld(true);
  const bounds = new THREE.Box3().setFromObject(model);
  const center = bounds.getCenter(new THREE.Vector3());
  model.position.set((x ?? area.x) - center.x, y - bounds.min.y, (z ?? area.z) - center.z);
  if (alignMaxX !== undefined) {
    model.updateMatrixWorld(true);
    model.position.x += alignMaxX - new THREE.Box3().setFromObject(model).max.x;
  }
}

// Hides the corridor model's south wall (windows) and puts a mirror copy of its north wall there.
export function mirrorCorridorWall(model) {
  const wall = model.getObjectByName('The Japanese school corridor_Wall_0');
  const windows = model.getObjectByName('The Japanese school corridor_Windows_0');
  if (!wall || !windows) return;
  windows.visible = false;
  model.updateMatrixWorld(true);
  const mirror = new THREE.Matrix4().makeTranslation(0, 0, 2 * CORRIDOR_MID_Z).multiply(new THREE.Matrix4().makeScale(1, 1, -1));
  const copy = wall.clone();
  copy.name = 'Corridor south wall (mirrored north wall)';
  new THREE.Matrix4().copy(model.matrixWorld).invert().multiply(mirror).multiply(wall.matrixWorld)
    .decompose(copy.position, copy.quaternion, copy.scale);
  model.add(copy);
}

export class AcademyScene {
  #onResize;

  constructor(canvas, game) {
    this.canvas = canvas;
    this.game = game;
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x000000);
    this.scene.fog = new THREE.FogExp2(0x000000, 0.012);
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
    this.playerFacingYaw = Math.PI;
    this.pitch = 0.28;
    this.pointerDown = null;
    this.started = false;
    this.playerPosition = new THREE.Vector3(-5.2, 0, -4.5);
    this.stepHeight = 0;
    this.stepHeightTimer = 0;
    this.humanPosition = this.playerPosition.clone();
    this.itemDrops = new Map();
    this.targetMeshes = [];
    this.targetGroups = new Map();
    this.doorModels = new Map();
    this.itemGroups = new Map();
    this.exitGroups = new Map();
    this.doorwayBlocks = [];
    this.pendingDoorway = null;
    this.doorHint = null;
    this.lastDoorHint = -Infinity;
    this.snapCamera = false;
    this.clipPlanes = new Map();
    this.roomGroups = new Map();
    this.playerGroup = null;
    this.playerRig = null;
    this.humanRig = null;
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
    this.occlusion = null;
    this.walls = null;
    this.#buildLights();
    this.#buildRooms();
    this.#buildObjects();
    this.#buildExits();
    this.#buildDoorways();
    this.#buildPlayer();
    this.playerMixer = null;
    this.playerWalkAction = null;
    this.playerJumpAction = null;
    this.playerStandAction = null;
    this.playerStanding = false;
    this.jumpAnimating = false;
    this.jumpElapsed = 0;
    this.jumpBlend = 0;
    this.playerModelLoad = this.#loadPlayerModel();
    this.classroomLoad = this.#loadClassroomModel();
    this.roomModelsLoad = this.#loadRoomModels();
    this.itemTemplates = new Map();
    this.itemModelsLoad = this.#loadItemModels();
    this.#onResize = () => this.resize();
    window.addEventListener('resize', this.#onResize);
    this.resize();
    this.refresh();
    Promise.all([this.classroomLoad, this.roomModelsLoad, this.itemModelsLoad]).then(() => this.#buildOcclusion());
  }

  #buildLights() {
    this.scene.add(new THREE.HemisphereLight(0x8e9b91, 0x111511, 0.42));
    const key = new THREE.DirectionalLight(0xc4c1aa, 0.22);
    key.position.set(-12, 24, 15);
    key.castShadow = true;
    key.shadow.mapSize.set(1024, 1024);
    key.shadow.camera.left = -45; key.shadow.camera.right = 45;
    key.shadow.camera.top = 30; key.shadow.camera.bottom = -42;
    this.scene.add(key);
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
        wall(left, room.z, 0.22, room.d);
        wall(right, room.z, 0.22, room.d);
        wall((left + room.x - 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
        wall((right + room.x + 1.3) / 2, top, room.w / 2 - 1.3, 0.22);
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
    const lampMat = new THREE.MeshStandardMaterial({ color: 0xf4dfae, emissive: 0xe1a354, emissiveIntensity: 2.1 });
    for (const x of [-18, -8, 2, 12, 22]) {
      const lamp = new THREE.Mesh(new THREE.BoxGeometry(2.2, 0.08, 0.35), lampMat);
      lamp.position.set(x, 3.15, -11);
      this.scene.add(lamp);
      const light = new THREE.PointLight(0xffc77b, 8, 11, 2);
      light.position.set(x, 2.75, -11);
      this.scene.add(light);
    }

    const roomFixtures = [
      { x: 0, z: -1.2, color: 0xffd18c, intensity: 9 },
      { x: 4.1, z: -21, color: 0xffc77b, intensity: 8 },
      { x: -11, z: -21, color: 0xd4e6dc, intensity: 7 },
      { x: 14.4, z: -21, color: 0xffc77b, intensity: 8 },
      { x: 38, z: -9, color: 0xc3e7cc, intensity: 6 },
      { x: 38, z: -21, color: 0xb7d8c0, intensity: 6 },
    ];
    for (const fixture of roomFixtures) {
      const emissive = new THREE.Color(fixture.color).multiplyScalar(0.55);
      const material = new THREE.MeshStandardMaterial({
        color: fixture.color,
        emissive,
        emissiveIntensity: 1.8,
        roughness: 0.55,
      });
      const panel = new THREE.Mesh(new THREE.BoxGeometry(1.45, 0.07, 0.42), material);
      panel.position.set(fixture.x, 3.05, fixture.z);
      this.scene.add(panel);
      const light = new THREE.PointLight(fixture.color, fixture.intensity, 10, 2);
      light.position.set(fixture.x, 2.62, fixture.z);
      this.scene.add(light);
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

  async #loadRoomModels() {
    await Promise.all(ROOM_MODELS.map(async (config) => {
      try {
        const model = await loadGlb(new URL(`../assets/models/${config.file}`, import.meta.url));
        placeRoomModel(model, config);
        if (config.mirrorNorthWall) mirrorCorridorWall(model);
        model.name = config.file;
        (this.roomGroups.get(config.room) ?? this.scene).add(model);
        if (config.doorOf) this.doorModels.set(config.doorOf, model);
        for (const id of config.replaces ?? []) {
          for (const child of this.targetGroups.get(id).children) child.visible = false;
        }
      } catch (error) {
        console.error(`${config.file} 房間模型載入失敗，使用簡化場景。`, error);
      }
    }));
    this.refresh();
  }

  async #loadItemModels() {
    await Promise.all(Object.entries(ITEM_MODELS).map(async ([id, config]) => {
      try {
        const model = await loadGlb(new URL(`../assets/models/items/${id.toLowerCase()}.glb`, import.meta.url));
        this.itemTemplates.set(id, this.#normalizeItemModel(model, config));
      } catch (error) {
        console.error(`${id} 道具模型載入失敗，使用簡化形狀。`, error);
      }
    }));
    for (const [id, group] of this.itemGroups) {
      if (!this.itemTemplates.has(id)) continue;
      this.#rebuildFormMesh(group, id);
      group.userData.localBounds = this.#localBounds(group);
    }
    const form = this.game.player().form;
    if (this.itemTemplates.has(form)) this.setForm(form);
    this.refresh();
  }

  #normalizeItemModel(model, { nodes, rotate, fit }) {
    model.updateMatrixWorld(true);
    const root = new THREE.Group();
    if (nodes) {
      for (const name of nodes) {
        const node = model.getObjectByName(name);
        if (node) root.attach(node);
      }
    } else {
      root.add(model);
    }
    if (rotate) root.rotation.set(...rotate);
    root.updateMatrixWorld(true);
    const bounds = new THREE.Box3().setFromObject(root);
    const size = bounds.getSize(new THREE.Vector3());
    const center = bounds.getCenter(new THREE.Vector3());
    const wrapper = new THREE.Group();
    wrapper.add(root);
    const scale = fit / Math.max(size.x, size.y, size.z);
    root.scale.multiplyScalar(scale);
    root.position.set(-center.x * scale, -bounds.min.y * scale, -center.z * scale);
    return wrapper;
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
        for (const [x, y, color] of [[-0.55, 2.05, 0xb68069], [0.48, 1.9, 0xc3b881], [-0.15, 1.1, 0x819b85]]) {
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
      if (id === 'S15') group.rotation.y = -Math.PI / 2; // wall-mounted on the east wall, front toward -x
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

  #buildDoorways() {
    const black = new THREE.MeshBasicMaterial({ color: 0x000000 });
    for (const doorway of DOORWAYS) {
      for (const [area, { line, span: [from, to], block, height = 2.7 }] of Object.entries(doorway.sides)) {
        if (block === false) continue;
        const alongX = doorway.axis === 'z';
        const mesh = new THREE.Mesh(new THREE.BoxGeometry(alongX ? to - from : 0.06, height, alongX ? 0.06 : to - from), black);
        mesh.position.set(alongX ? (from + to) / 2 : line, height / 2, alongX ? line : (from + to) / 2);
        mesh.name = `${area} 轉場`;
        this.scene.add(mesh);
        this.doorwayBlocks.push({ doorway, mesh });
      }
    }
  }

  completeDoorway() {
    const pending = this.pendingDoorway;
    this.pendingDoorway = null;
    if (!pending || !this.game.moveToArea('local', pending.area)) return false;
    const [x, z] = pending.doorway.sides[pending.area].spawn;
    this.playerPosition.set(x, 0, z);
    this.playerGroup.position.set(x, this.game.player().form === 'F00' ? this.jumpHeight : 0, z);
    this.snapCamera = true;
    this.refresh();
    return true;
  }

  #clipPlanesFor(area) {
    if (!this.clipPlanes.has(area)) {
      const rect = roomClipRect(area);
      this.clipPlanes.set(area, rect ? [
        new THREE.Plane(new THREE.Vector3(1, 0, 0), -rect.minX), new THREE.Plane(new THREE.Vector3(-1, 0, 0), rect.maxX),
        new THREE.Plane(new THREE.Vector3(0, 0, 1), -rect.minZ), new THREE.Plane(new THREE.Vector3(0, 0, -1), rect.maxZ),
      ] : []);
    }
    return this.clipPlanes.get(area);
  }

  exitSpecialLocation() {
    const area = this.game.player().area;
    const exit = SPECIAL_EXITS[area];
    if (!exit || !this.game.moveToArea('local', 'Corridor')) return { rejected: true, text: '' };
    const [x, z] = exit.to;
    this.playerPosition.set(x, 0, z);
    this.playerGroup.position.copy(this.playerPosition);
    this.snapCamera = true;
    this.refresh();
    return { text: exit.text, location: 'Corridor' };
  }

  #formShape(id) {
    const group = new THREE.Group();
    const template = this.itemTemplates?.get(id);
    if (template) {
      group.add(template.clone(true));
      return group;
    }
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
      for (const [x, y] of [[-.18, .61], [.18, .25], [0, .43]]) dot(x, y, 0.37);
      for (const [x, y] of [[-.17, .6], [.17, .6], [-.17, .26], [.17, .26]]) dot(x, y, -0.37);
      for (const [z, y] of [[-.17, .6], [.17, .26]]) dot(0.37, y, z);
      for (const [z, y] of [[-.18, .61], [.18, .61], [-.18, .25], [.18, .25], [0, .43]]) dot(-0.37, y, z);
      for (const [x, z] of [[-.18, -.18], [.18, -.18], [0, 0], [-.18, .18], [.18, .18]]) dot(x, 0.8, z);
      for (const [x, z] of [[-.2, -.2], [.2, -.2], [-.2, .2], [.2, .2], [0, 0], [-.2, 0]]) dot(x, 0.06, z);
    } else if (id === 'F09') {
      const ring = new THREE.Mesh(new THREE.TorusGeometry(0.46, 0.09, 10, 28), mat(0xd6c88c, 0.4)); ring.rotation.x = Math.PI / 2; add(ring, 0, 0.88);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.34, 0.34, 0.08, 24), mat(0xddd6b7)), 0, 0.88);
      add(new THREE.Mesh(new THREE.CylinderGeometry(0.3, 0.3, 0.085, 24), mat(0x47574b)), 0, 0.88);
      for (let i = 0; i < 12; i++) {
        const angle = i * Math.PI / 6;
        add(new THREE.Mesh(new THREE.BoxGeometry(0.035, 0.025, 0.085), mat(0xd9d3b2)), Math.sin(angle) * 0.24, 0.94, Math.cos(angle) * 0.24);
      }
      add(new THREE.Mesh(new THREE.BoxGeometry(0.045, 0.025, 0.23), mat(0xd6c88c)), 0, 0.95, -0.04);
      const hand = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.025, 0.16), mat(0xe2b877)); hand.rotation.y = -0.75; add(hand, 0.04, 0.96, 0.02);
      add(new THREE.Mesh(new THREE.SphereGeometry(0.045, 10, 8), mat(0xd6c88c)), 0, 0.96, 0);
    } else if (id === 'F12') {
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8, 0.13, 0.75), mat(palette.wood)), 0, 0.58);
      add(new THREE.Mesh(new THREE.BoxGeometry(0.8, 0.75, 0.14), mat(palette.wood)), 0, 1.0, -0.3);
      for (const x of [-0.3, 0.3]) for (const z of [-0.27, 0.27]) add(new THREE.Mesh(new THREE.BoxGeometry(0.08, 0.55, 0.08), mat(0x6c5840)), x, 0.27, z);
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
      this.playerRig = this.humanRig;
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
    this.playerGroup.rotation.y = this.playerFacingYaw;
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
      source.scale.setScalar((1.3 * 1.3) / size.y);
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
      const { walk: walkClip, jump: jumpClip, idle: idleClip } = pickPlayerClips(source.animations ?? []);
      if (walkClip) {
        const hips = source.getObjectByName('mixamorig:Hips');
        removeRootTranslationTrack(walkClip, hips);
        if (jumpClip) removeRootTranslationTrack(jumpClip, hips);
        if (idleClip) removeRootTranslationTrack(idleClip, hips);
        this.playerMixer = new THREE.AnimationMixer(source);
        this.playerWalkAction = this.playerMixer.clipAction(walkClip);
        this.playerWalkAction.setLoop(THREE.LoopRepeat, Infinity);
        if (jumpClip) {
          this.playerJumpAction = this.playerMixer.clipAction(jumpClip);
          this.playerJumpAction.setLoop(THREE.LoopOnce, 1);
          this.playerJumpAction.clampWhenFinished = true;
        }
        if (idleClip) {
          // Standing: the looping Mixamo idle clip plays whenever the player is not moving.
          this.playerStandAction = this.playerMixer.clipAction(idleClip);
          this.playerStandAction.setLoop(THREE.LoopRepeat, Infinity);
          this.playerStandAction.reset().play();
          this.playerStanding = true;
          this.playerMixer.update(0);
        } else {
          this.playerWalkAction.reset().play();
          this.playerMixer.update(0);
          this.playerWalkAction.paused = true;
        }
        this.humanRig = { kind: 'animated' };
      } else {
        this.humanRig = this.#createSkinnedPlayerRig(source);
      }
      this.playerRig = this.humanRig;
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
      setBoneDirection(arm, forearm, new THREE.Vector3(side * 0.42, -Math.cos(armSwing), Math.sin(armSwing)));
      setBoneDirection(forearm, hand, new THREE.Vector3(side * 0.24, -Math.cos(forearmSwing), Math.sin(forearmSwing)));
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
    if (formId !== 'F00') { this.stepHeight = 0; this.stepHeightTimer = 0; }
    // The single character model moves to the body left behind while the player is an item.
    if (formId !== 'F00' && this.humanModel?.parent === this.playerGroup) {
      this.humanGroup.rotation.y = this.playerGroup.rotation.y;
      this.#rebuildFormMesh(this.humanGroup, 'F00');
    }
    this.#rebuildFormMesh(this.playerGroup, formId);
    this.jumpHeight = 0;
    this.jumpVelocity = 0;
    this.#stopJumpAnimation();
    this.playerGroup.position.y = 0;
  }

  syncAfterAction(previousForm, result) {
    const player = this.game.player();
    const before = this.playerPosition.clone();
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
    if (this.playerPosition.distanceTo(before) > 3) this.snapCamera = true;
    this.refresh();
  }

  resetRound() {
    this.itemDrops.clear();
    this.stepHeight = 0;
    this.stepHeightTimer = 0;
    this.playerPosition.set(-5.2, 0, -4.5);
    this.humanPosition.copy(this.playerPosition);
    this.playerGroup.position.copy(this.playerPosition);
    this.setForm(this.game.player().form);
    this.snapCamera = true;
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
      const doorModel = this.doorModels.get(doorId);
      if (doorModel) doorModel.visible = !this.game.isDoorOpen(doorId);
    }
    for (const { doorway, mesh } of this.doorwayBlocks) mesh.visible = !doorway.door || this.game.isDoorOpen(doorway.door);
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
      if (!group.visible || this.game.item(id)?.area !== this.game.player().area) continue;
      candidates.push({ id, name: FORMS[id], x: group.position.x, z: group.position.z });
    }
    const best = nearestCandidate(this.playerPosition.x, this.playerPosition.z, this.yaw, candidates, maxDistance);
    return best && { id: best.id, name: best.name, distance: best.distance };
  }

  #targetArea(id) {
    if (id === 'EXIT') return this.game.player().area;
    return OBJECT_AREAS[id] ?? this.game.item(id)?.area;
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
      if (!node || this.#targetArea(node.userData.targetId) !== this.game.player().area) continue;
      const dist = Math.hypot(node.position.x + (node.parent === this.scene ? 0 : node.parent.position.x) - this.playerPosition.x, node.position.z - this.playerPosition.z);
      if (dist <= 4.2) return node.userData.targetId;
    }
    return null;
  }

  isWalkable(x, z, radius = 0.3) {
    return areaAt(x, z) === this.game.player().area && !this.#blockedAt(x, z, radius);
  }

  // The room models' walls, partitions, stairs and furniture, sampled around the player's feet and waist.
  #hitsWalls(x, z, radius) {
    if (!this.walls) return false;
    for (const y of [0.35, 0.9]) {
      if (this.walls.isBlocked(x, y, z)) return true;
      for (let i = 0; i < 8; i++) {
        const angle = i * Math.PI / 4;
        if (this.walls.isBlocked(x + Math.cos(angle) * radius, y, z + Math.sin(angle) * radius)) return true;
      }
    }
    return false;
  }

  #blockedAt(x, z, radius) {
    if (this.#hitsWalls(x, z, radius)) return true;
    const area = areaAt(x, z);
    if (area === 'Corridor' && Math.abs(x) + radius > CORRIDOR_HALF_LENGTH) return true;
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

  jump() {
    if (!this.started || this.game.player().ending || this.game.player().form !== 'F00' || this.jumpHeight > 0) return false;
    this.jumpVelocity = 4.3;
    this.#startJumpAnimation();
    return true;
  }

  #startJumpAnimation() {
    const jump = this.playerJumpAction;
    if (!jump) return;
    jump.reset();
    jump.time = JUMP_CLIP_TAKEOFF_TIME;
    jump.paused = true;
    jump.play();
    const from = this.playerStanding && this.playerStandAction ? this.playerStandAction : this.playerWalkAction;
    jump.crossFadeFrom(from, 0.08, false);
    this.playerWalkAction.paused = true;
    this.jumpAnimating = true;
    this.jumpElapsed = 0;
    this.jumpBlend = 0;
    this.playerMixer.update(0);
  }

  #stopJumpAnimation() {
    if (!this.playerJumpAction) return;
    this.jumpAnimating = false;
    this.jumpBlend = 0;
    this.playerJumpAction.stop();
    this.playerStandAction?.stop();
    this.playerStanding = false;
    this.playerWalkAction.reset().play();
    this.playerWalkAction.paused = true;
    this.playerWalkAction.setEffectiveWeight(1);
  }

  // Crossfades between the walk cycle and the standing pose when the player starts or stops moving.
  #updateGaitAnimation(moving) {
    const walk = this.playerWalkAction;
    const stand = this.playerStandAction;
    if (!stand) {
      if (moving && walk.paused) {
        walk.paused = false;
        walk.play();
      } else if (!moving && !walk.paused) walk.paused = true;
      return;
    }
    if (moving && (this.playerStanding || walk.paused)) {
      walk.reset().play();
      walk.crossFadeFrom(stand, STAND_FADE_SECONDS, false);
      this.playerStanding = false;
    } else if (!moving && !this.playerStanding) {
      stand.reset().play();
      stand.crossFadeFrom(walk, STAND_FADE_SECONDS, false);
      this.playerStanding = true;
    }
  }

  // Returns true while the jump clip owns the pose. The clip is scrubbed by airtime so touchdown
  // coincides with the landing pose, then it fades back into the walk pose.
  #updateJumpAnimation(dt, moving) {
    const jump = this.playerJumpAction;
    if (!jump || !this.jumpAnimating) return false;
    if (this.jumpHeight > 0 || this.jumpVelocity > 0) {
      this.jumpElapsed += dt;
      jump.time = jumpClipTime(this.jumpElapsed);
      // The jump action is paused so its time stays where it was scrubbed, but the mixer must
      // still advance so the start crossfade weight can progress.
      this.playerMixer.update(dt);
      return true;
    }
    this.jumpAnimating = false;
    this.jumpBlend = JUMP_FADE_SECONDS;
    jump.time = JUMP_CLIP_LAND_TIME;
    jump.paused = false;
    const next = this.playerStandAction && !moving ? this.playerStandAction : this.playerWalkAction;
    next.reset();
    next.paused = next === this.playerWalkAction && !moving;
    next.play();
    next.crossFadeFrom(jump, JUMP_FADE_SECONDS, false);
    if (this.playerStandAction) this.playerStanding = !moving;
    return false;
  }

  move(dt) {
    if (!this.started || this.game.player().ending || this.pendingDoorway) return;
    this.playerAnimationTime += dt;
    if (this.jumpHeight > 0 || this.jumpVelocity > 0) {
      this.jumpVelocity -= 11.5 * dt;
      this.jumpHeight += this.jumpVelocity * dt;
      if (this.jumpHeight <= 0) { this.jumpHeight = 0; this.jumpVelocity = 0; }
    }
    const human = this.game.player().form === 'F00';
    this.playerGroup.position.y = human ? this.jumpHeight + this.stepHeight : 0;
    let forward = Number(this.keys.has('KeyW') || this.keys.has('ArrowUp')) - Number(this.keys.has('KeyS') || this.keys.has('ArrowDown'));
    let right = Number(this.keys.has('KeyD') || this.keys.has('ArrowRight')) - Number(this.keys.has('KeyA') || this.keys.has('ArrowLeft'));
    const hasMovementInput = Boolean(forward || right);
    if (hasMovementInput) {
      const inputLength = Math.hypot(forward, right);
      forward /= inputLength;
      right /= inputLength;
      const fx = Math.sin(this.yaw), fz = -Math.cos(this.yaw);
      const rx = Math.cos(this.yaw), rz = Math.sin(this.yaw);
      const moveX = fx * forward + rx * right;
      const moveZ = fz * forward + rz * right;
      const targetYaw = Math.atan2(moveX, moveZ);
      if (this.game.player().form === 'F00') {
        const turn = advancePlayerFacing(this.playerFacingYaw, targetYaw, PLAYER_TURN_SPEED * dt);
        this.playerFacingYaw = turn.yaw;
        this.playerGroup.rotation.y = this.playerFacingYaw;
      }
    }
    let moving = false;
    if (hasMovementInput) {
      const speed = this.game.player().form === 'F08' ? 2.6 : this.game.player().form === 'F09' ? 1.8 : 4.4;
      const fx = Math.sin(this.yaw), fz = -Math.cos(this.yaw);
      const rx = Math.cos(this.yaw), rz = Math.sin(this.yaw);
      const oldX = this.playerPosition.x, oldZ = this.playerPosition.z;
      let nx = oldX + (fx * forward + rx * right) * speed * dt;
      let nz = oldZ + (fz * forward + rz * right) * speed * dt;
      // The human model is narrower at the feet than the other forms' generic 0.3-unit collider.
      // A smaller footprint keeps chair and desk clearances usable without changing obstacle geometry.
      const radius = human ? 0 : this.game.player().form === 'F12' ? 0.38 : 0.3;
      const from = this.game.player().area;
      // Rooms never join directly: walking up to a room's doorway starts the transition to the room behind it.
      for (const doorway of DOORWAYS) {
        const side = doorway.sides[from];
        if (!side) continue;
        const [lateral, depth, home] = doorway.axis === 'z' ? [nx, nz, side.spawn[1]] : [nz, nx, side.spawn[0]];
        if (lateral < side.span[0] || lateral > side.span[1] || (depth - side.line) * Math.sign(side.line - home) < -DOORWAY_REACH) continue;
        // A locked door stops the player and says so, at most once every two seconds.
        if (doorway.door && !this.game.isDoorOpen(doorway.door)) {
          const now = performance.now();
          if (now - this.lastDoorHint > 2000) { this.doorHint = LOCKED_DOOR_TEXT[doorway.door]; this.lastDoorHint = now; }
        } else this.pendingDoorway = { doorway, area: Object.keys(doorway.sides).find((id) => id !== from) };
        nx = oldX; nz = oldZ;
        break;
      }
      const assessPath = (x, z) => {
        if (this.#blockedAt(x, z, radius)) return null;
        if (!human || !this.occlusion) return this.stepHeight;
        return bodyPathStepHeight(this.occlusion, oldX, oldZ, x, z, radius, 1.3, this.stepHeight);
      };
      let nextStepHeight = assessPath(nx, nz);
      if (nextStepHeight === null) {
        const xStepHeight = assessPath(nx, oldZ);
        if (xStepHeight !== null) { nz = oldZ; nextStepHeight = xStepHeight; }
        else {
          const zStepHeight = assessPath(oldX, nz);
          if (zStepHeight !== null) { nx = oldX; nextStepHeight = zStepHeight; }
        }
      }
      if (nextStepHeight !== null) {
        const area = areaAt(nx, nz);
        if (area === from) {
          this.playerPosition.x = nx; this.playerPosition.z = nz;
          if (human && nextStepHeight > this.stepHeight + 1e-6) this.stepHeightTimer = 0.35;
          else if (human && this.stepHeight > 0) {
            const lowerPathBlocked = bodyPathIntersectsGrid(this.occlusion, oldX, oldZ, nx, nz, radius, 1.3, 0);
            if (lowerPathBlocked) this.stepHeightTimer = 0.12;
            else if (this.stepHeightTimer > 0) {
              this.stepHeightTimer = Math.max(0, this.stepHeightTimer - dt);
              nextStepHeight = this.stepHeight;
            } else nextStepHeight = Math.max(0, this.stepHeight - Math.min(0.08, dt * 2));
          }
          this.stepHeight = human ? nextStepHeight : 0;
          if (!human) this.stepHeightTimer = 0;
          this.playerGroup.position.set(nx, human ? this.jumpHeight + this.stepHeight : 0, nz);
          if (!human) this.game.item(this.game.player().form).area = area;
          moving = Math.hypot(nx - oldX, nz - oldZ) > 1e-6;
        }
      }
    }
    this.walking = moving;
    if (human) {
      const gait = moving ? Math.sin(this.playerAnimationTime * 10) : 0;
      if (this.playerWalkAction) {
        if (!this.#updateJumpAnimation(dt, moving)) {
          // The jump clip drives the pose while airborne; otherwise blend walk and standing.
          this.#updateGaitAnimation(moving);
          this.playerMixer.update(dt);
          this.jumpBlend = Math.max(0, this.jumpBlend - dt);
        }
      } else if (this.playerRig) {
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
    } else if (this.playerWalkAction && !this.playerWalkAction.paused) {
      this.playerWalkAction.paused = true;
    }
  }

  updateCamera(now = performance.now()) {
    const dt = this.lastCameraUpdate ? Math.min(0.1, (now - this.lastCameraUpdate) / 1000) : 1 / 60;
    this.lastCameraUpdate = now;
    const smooth = this.snapCamera ? 1 : 1 - Math.exp(-10 * dt);
    this.snapCamera = false;
    if (this.game.player().canSeeWholeMap) {
      this.cameraDesired.set(7, 50, -14);
      this.camera.position.lerp(this.cameraDesired, smooth);
      this.camera.lookAt(this.mapTarget);
      return;
    }
    const distance = this.game.player().form === 'F12' ? 8.5 : this.game.player().form === 'F08' ? 6.2 : this.game.player().form === 'F00' ? 3.8 : 7.2;
    // The camera orbits on a sphere around the player: pitch is its elevation angle and only the mouse changes it.
    const targetY = (this.game.player().form === 'F00' ? 1.05 : 0.78) + this.jumpHeight * 0.35;
    const pitch = this.game.player().form === 'F00' ? 0.45 : this.pitch;
    const reach = distance * Math.cos(pitch);
    const height = Math.max(0.5, targetY + distance * Math.sin(pitch) - (this.game.player().form === 'F09' ? 0.4 : 0));
    const shoulder = this.game.player().form === 'F00' ? 1.1 : 0;
    this.cameraDesired.set(this.playerPosition.x - Math.sin(this.yaw) * reach + Math.cos(this.yaw) * shoulder, height,
      this.playerPosition.z + Math.cos(this.yaw) * reach + Math.sin(this.yaw) * shoulder);
    this.camera.position.lerp(this.cameraDesired, smooth);
    this.#pullCameraIn(this.playerPosition.x, targetY, this.playerPosition.z);
    // Aim from the shoulder-offset camera toward the matching shoulder point so the view's
    // horizontal center line stays parallel to the forward direction used by W.
    this.cameraTarget.set(this.playerPosition.x + Math.cos(this.yaw) * shoulder, targetY,
      this.playerPosition.z + Math.sin(this.yaw) * shoulder);
    this.camera.lookAt(this.cameraTarget);
  }

  // Slides the camera along its line to the player until it is clear of the room walls and of any model in the way,
  // so the view angle never changes when something is in the way, only the distance.
  #pullCameraIn(px, py, pz) {
    const cam = this.camera.position;
    const dx = cam.x - px, dy = cam.y - py, dz = cam.z - pz;
    const length = Math.hypot(dx, dy, dz);
    if (length < 1e-6) return;
    let t = 1;
    const rect = areaCameraRect(this.game.player().area);
    if (rect) t = Math.min(t, rayRectLimit(px, pz, dx, dz, rect));
    if (this.occlusion && this.game.player().form !== 'F00') {
      t = Math.min(t, this.occlusion.firstHit(px, py, pz, cam.x, cam.y, cam.z, Math.min(1, CAMERA_BODY_CLEARANCE / length)) - CAMERA_WALL_CLEARANCE / length);
    }
    t = Math.max(t, Math.min(1, CAMERA_MIN_DISTANCE / length));
    cam.set(px + dx * t, py + dy * t, pz + dz * t);
  }

  // Builds the camera occlusion grid and the walls the player bumps into from every static, visible mesh once the
  // room models have loaded.
  #buildOcclusion() {
    const loose = [this.playerGroup, this.humanGroup, ...this.itemGroups.values(), ...this.exitGroups.values(), ...this.doorwayBlocks.map(({ mesh }) => mesh), ...this.classroomExitDoors];
    this.occlusion = this.#gridFrom(new Set([...loose, ...['S05', 'S09', 'S19'].map((id) => this.targetGroups.get(id))]));
    // Targets collide through their own bounds (and interaction-only proxies not at all), so walls leave them out.
    this.walls = this.#gridFrom(new Set([...loose, ...this.targetGroups.values()]));
  }

  #gridFrom(skip) {
    const grid = new OcclusionGrid({ minX: -30, minY: 0, minZ: -28, maxX: 46, maxY: 3, maxZ: 8 });
    const a = new THREE.Vector3(), b = new THREE.Vector3(), c = new THREE.Vector3();
    this.scene.updateMatrixWorld(true);
    const walk = (node) => {
      if (!node.visible || skip.has(node) || node.isSprite || node.isLight || node.isSkinnedMesh) return;
      const position = node.isMesh && node.geometry.attributes.position;
      if (position) {
        const index = node.geometry.index;
        const count = index ? index.count : position.count;
        for (let i = 0; i + 2 < count; i += 3) {
          a.fromBufferAttribute(position, index ? index.getX(i) : i).applyMatrix4(node.matrixWorld);
          b.fromBufferAttribute(position, index ? index.getX(i + 1) : i + 1).applyMatrix4(node.matrixWorld);
          c.fromBufferAttribute(position, index ? index.getX(i + 2) : i + 2).applyMatrix4(node.matrixWorld);
          grid.addTriangle(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
        }
      }
      for (const child of node.children) walk(child);
    };
    walk(this.scene);
    return grid;
  }

  render() {
    this.updateCamera();
    this.renderer.clippingPlanes = this.game.player().canSeeWholeMap ? [] : this.#clipPlanesFor(this.game.player().area);
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
