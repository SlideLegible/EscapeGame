import * as THREE from '../vendor/three.module.js';
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { loadGlb } from '../src/glb.mjs';
import { orbitCameraView, ROOM_MODELS, ITEM_MODELS, normalizeItemModel, placeRoomModel, mirrorCorridorWall, advancePlayerFacing, areaAt, areaCameraRect, bodyPathIntersectsGrid, bodyPathStepHeight, ITEM_POSITIONS, dropPoint, entryPoint, nearestCandidate, OcclusionGrid, rayRectLimit, removeRootTranslationTrack, pickPlayerClips, jumpClipTime, JUMP_CLIP_TAKEOFF_TIME, JUMP_CLIP_LAND_TIME, roomClipRect, DOORWAYS, CORRIDOR_HALF_LENGTH, SPECIAL_EXITS, OBJECT_POSITIONS, LECTERN_SIZE } from '../src/scene.mjs';

test('walkable room areas touch across the classroom, corridor, office, restroom, and storage thresholds', () => {
  assert.equal(areaAt(7, -6.2), 'Corridor');
  assert.equal(areaAt(4.1, -15.9), 'Office');
  assert.equal(areaAt(-11, -15.9), 'Restroom');
  assert.equal(areaAt(14.4, -15.9), 'GuardRoom');
});

test('area lookup still keeps the classroom and corridor interiors distinct', () => {
  assert.equal(areaAt(0, -3), 'Classroom');
  assert.equal(areaAt(0, -11), 'Corridor');
  assert.equal(areaAt(4.1, -21), 'Office');
  assert.equal(areaAt(-11, -21), 'Restroom');
});

test('a released item stays where the player let go of it', () => {
  assert.deepEqual(dropPoint(3.2, -11.5), [3.2, -11.5]);
  assert.deepEqual(dropPoint(-7.4, 2.1), [-7.4, 2.1]);
});

test('an item released inside the vending machine lands in the corridor beside it', () => {
  assert.equal(areaAt(...dropPoint(38, -9)), 'Corridor');
  assert.equal(areaAt(...dropPoint(38, -21)), 'Corridor');
});

test('entering the drink machine or its underside does not place the player on top of the item inside', () => {
  for (const area of ['L01', 'L02']) {
    const [x, z] = entryPoint(area);
    assert.equal(areaAt(x, z), area);
    assert.ok(Math.hypot(x - 38, z - (area === 'L01' ? -9 : -21)) >= 1.5);
  }
});

test('each special location has an exit inside it that leads to the corridor beside the machine', () => {
  for (const area of ['L01', 'L02']) {
    const [x, z] = SPECIAL_EXITS[area].at;
    assert.equal(areaAt(x, z), area);
    const [ox, oz] = SPECIAL_EXITS[area].to;
    assert.equal(areaAt(ox, oz), 'Corridor');
  }
});

test('leaving a special location does not land the player on an item pushed out beside the machine', () => {
  for (const area of ['L01', 'L02']) {
    const [ox, oz] = SPECIAL_EXITS[area].to;
    for (const x of [33.5, 38, 42.5]) {
      const [dx, dz] = dropPoint(x, area === 'L01' ? -9 : -21);
      assert.ok(Math.hypot(ox - dx, oz - dz) >= 0.9);
    }
  }
});

// The drink machine's front (+z of its model) turned by its placement, and the direction along its face.
const machineModel = ROOM_MODELS.find(({ file }) => file === 'objects/s03.glb');
const machineFront = [Math.sin(machineModel.rotateY ?? 0), Math.cos(machineModel.rotateY ?? 0)];
const machineSide = [machineFront[1], -machineFront[0]];
const fromMachine = ([x, z]) => {
  const [mx, mz] = OBJECT_POSITIONS.S03;
  return { ahead: (x - mx) * machineFront[0] + (z - mz) * machineFront[1], beside: (x - mx) * machineSide[0] + (z - mz) * machineSide[1] };
};
const besideMachine = (ahead, beside) => [
  OBJECT_POSITIONS.S03[0] + machineFront[0] * ahead + machineSide[0] * beside,
  OBJECT_POSITIONS.S03[1] + machineFront[1] * ahead + machineSide[1] * beside,
];
// Inner faces of the corridor model's walls (the south one is the mirrored north wall), measured on the model.
const CORRIDOR_WALLS = { north: -7.9, south: -13.86 };

test('approaching the drink machine from the front reaches the machine, and from the back reaches its underside', () => {
  const nearer = ([x, z]) => {
    const machine = Math.hypot(x - OBJECT_POSITIONS.S03[0], z - OBJECT_POSITIONS.S03[1]);
    const underside = Math.hypot(x - OBJECT_POSITIONS.S04[0], z - OBJECT_POSITIONS.S04[1]);
    return machine < underside ? 'S03' : 'S04';
  };
  for (const beside of [-0.5, 0, 0.5]) {
    assert.equal(nearer(besideMachine(1.2, beside)), 'S03');
    assert.equal(nearer(besideMachine(-1.2, beside)), 'S04');
  }
  assert.ok(Math.abs(fromMachine(OBJECT_POSITIONS.S04).beside) < 0.05, 'the underside is right behind the machine');
  assert.equal(areaAt(...OBJECT_POSITIONS.S04), 'Corridor');
});

test('leaving each special location puts the player on the side of the machine it was entered from', () => {
  assert.ok(fromMachine(SPECIAL_EXITS.L01.to).ahead > 0);
  assert.ok(fromMachine(SPECIAL_EXITS.L02.to).ahead < 0);
  assert.ok(fromMachine(dropPoint(38, -9)).ahead > 0);
  assert.ok(fromMachine(dropPoint(38, -21)).ahead < 0);
});

test('the drink machine stands with one side against a wall, so its front and back both face along the corridor', () => {
  assert.ok(Math.abs(machineFront[1]) < 1e-9, 'the front faces along the corridor');
  for (const [x, z] of [besideMachine(1.2, 0), besideMachine(-1.2, 0)]) {
    assert.equal(areaAt(x, z), 'Corridor');
    assert.ok(Math.abs(x) < CORRIDOR_HALF_LENGTH - 0.3, 'there is room to stand in front of and behind the machine');
  }
});

test('items left by the drink machine and the spots the player leaves it to are on the corridor floor, clear of the walls', () => {
  const points = [SPECIAL_EXITS.L01.to, SPECIAL_EXITS.L02.to];
  for (const x of [33.5, 38, 42.5]) points.push(dropPoint(x, -9), dropPoint(x, -21));
  for (const [x, z] of points) {
    assert.equal(areaAt(x, z), 'Corridor');
    assert.ok(z < CORRIDOR_WALLS.north - 0.3 && z > CORRIDOR_WALLS.south + 0.3, `${x}, ${z} is in a wall`);
    assert.ok(Math.abs(fromMachine([x, z]).ahead) > 0.9, `${x}, ${z} is inside the machine`);
  }
});

test('there is no storage room behind the restroom', () => {
  assert.equal(areaAt(-21.5, -21), null);
  assert.equal(roomClipRect('Storage'), null);
});

test('the storage door is the door on the right-hand wall when entering the restroom from the corridor', () => {
  const [x, z] = OBJECT_POSITIONS.S09;
  assert.equal(areaAt(x - 0.2, z), 'Restroom');
  assert.ok(x > -7.2 && x < -6.6, 'on the east wall');
  assert.ok(z < -17.2 && z > -18.8, 'at the restroom model door');
  const [px, pz] = ITEM_POSITIONS.F07;
  assert.equal(areaAt(px, pz), 'Restroom');
  assert.ok(Math.hypot(px - x, pz - z) < 1.5, 'the pole appears in front of the door');
});

const box = (w, d) => ({ min: { x: -w / 2, z: -d / 2 }, max: { x: w / 2, z: d / 2 } });
const classroomFront = () => [
  { id: 'S14', x: OBJECT_POSITIONS.S14[0], z: OBJECT_POSITIONS.S14[1], bounds: box(0.16, 6) },
  { id: 'S16', x: OBJECT_POSITIONS.S16[0], z: OBJECT_POSITIONS.S16[1], bounds: box(LECTERN_SIZE[0], LECTERN_SIZE[1]) },
  { id: 'S20', x: OBJECT_POSITIONS.S20[0], z: OBJECT_POSITIONS.S20[1], bounds: box(0.9, 0.1) },
];

test('a dice between the lectern and the blackboard reaches the blackboard when facing it', () => {
  const facingBlackboard = -Math.PI / 2;
  for (const z of [-2.4, -1, 0, 1, 2.4]) {
    assert.equal(nearestCandidate(-7.2, z, facingBlackboard, classroomFront()).id, 'S14', `z=${z}`);
  }
});

test('the lectern target matches the lectern in the classroom model and leaves the walkway in front of the blackboard clear', () => {
  const [x] = OBJECT_POSITIONS.S16;
  assert.ok(Math.abs(x - -6.32) < 0.1);
  assert.ok(LECTERN_SIZE[1] > LECTERN_SIZE[0]);
  assert.ok(x - LECTERN_SIZE[0] / 2 > -7.0);
});

test('a dice pressed against the blackboard reaches it even without turning the camera toward it', () => {
  for (const z of [-2, -1, 0, 1, 2]) {
    assert.equal(nearestCandidate(-7.3, z, 0, classroomFront()).id, 'S14', `z=${z}`);
  }
});

test('the same spot reaches the lectern when the player turns to face it', () => {
  const facingLectern = Math.PI / 2;
  assert.equal(nearestCandidate(-7.2, 0, facingLectern, classroomFront()).id, 'S16');
});

test('facing the drink machine from the front or the back still picks the machine or its underside', () => {
  // The interaction boxes of the machine (1.36 wide, 0.78 deep) and the strip behind it, turned with the machine.
  const turned = (halfWidth, fromAhead, toAhead) => {
    const corners = [[-halfWidth, fromAhead], [halfWidth, fromAhead], [-halfWidth, toAhead], [halfWidth, toAhead]]
      .map(([side, ahead]) => [machineFront[0] * ahead - machineSide[0] * side, machineFront[1] * ahead - machineSide[1] * side]);
    const xs = corners.map(([x]) => x), zs = corners.map(([, z]) => z);
    return { min: { x: Math.min(...xs), z: Math.min(...zs) }, max: { x: Math.max(...xs), z: Math.max(...zs) } };
  };
  const machine = () => [
    { id: 'S03', x: OBJECT_POSITIONS.S03[0], z: OBJECT_POSITIONS.S03[1], bounds: turned(0.68, -0.39, 0.39) },
    { id: 'S04', x: OBJECT_POSITIONS.S04[0], z: OBJECT_POSITIONS.S04[1], bounds: turned(0.68, 0, 0.3) },
  ];
  const facing = ([dx, dz]) => Math.atan2(dx, -dz);
  assert.equal(nearestCandidate(...besideMachine(1, 0), facing([-machineFront[0], -machineFront[1]]), machine()).id, 'S03');
  assert.equal(nearestCandidate(...besideMachine(-1.3, 0), facing(machineFront), machine()).id, 'S04');
});

test('nothing farther than the reach distance is picked', () => {
  assert.equal(nearestCandidate(0, 0, 0, [{ id: 'F08', x: 0, z: -5 }]), null);
  assert.equal(nearestCandidate(0, 0, 0, [{ id: 'F08', x: 0, z: -2 }]).id, 'F08');
});

test('orbit camera rectangle follows the floor of the room the player is in', () => {
  const office = areaCameraRect('Office');
  assert.ok(office.minZ >= -26 && office.maxZ <= -15.8 && office.minX >= -0.9 && office.maxX <= 9.1);
  const classroom = areaCameraRect('Classroom');
  assert.ok(classroom.maxX <= 7.75 && classroom.maxZ <= 5.75);
  const corridor = areaCameraRect('Corridor');
  assert.ok(corridor.maxZ <= -7 && corridor.minZ >= -15);
  assert.equal(areaCameraRect(null), null);
});

test('camera ray is shortened to stay inside the rectangle without changing direction', () => {
  const rect = { minX: 0, maxX: 10, minZ: 0, maxZ: 10 };
  assert.equal(rayRectLimit(5, 5, 2, 0, rect), 1);
  assert.equal(rayRectLimit(5, 5, 10, 0, rect), 0.5);
  assert.equal(rayRectLimit(5, 5, 0, -20, rect), 0.25);
  assert.equal(rayRectLimit(5, 5, 0, 0, rect), 1);
  assert.equal(rayRectLimit(-3, 5, 10, 0, rect), 1);
});

test('occlusion grid stops the camera ray at walls but ignores floors and ceilings', () => {
  const grid = new OcclusionGrid({ minX: 0, minY: 0, minZ: 0, maxX: 10, maxY: 4, maxZ: 10 });
  // A vertical wall at x = 6 spanning z 0..10 and y 0..3.
  grid.addTriangle(6, 0, 0, 6, 3, 0, 6, 0, 10);
  grid.addTriangle(6, 3, 0, 6, 3, 10, 6, 0, 10);
  // A horizontal floor and a ceiling.
  grid.addTriangle(0, 0, 0, 10, 0, 0, 0, 0, 10);
  grid.addTriangle(0, 3.5, 0, 10, 3.5, 0, 0, 3.5, 10);
  assert.equal(grid.isBlocked(6.05, 1.5, 5), true);
  assert.equal(grid.isBlocked(5, 0.1, 5), false);
  assert.equal(grid.isBlocked(5, 3.5, 5), false);
  const hit = grid.firstHit(2, 1, 5, 9, 1, 5);
  assert.ok(hit > 0.5 && hit < 0.65, `hit at ${hit}`);
  assert.equal(grid.firstHit(2, 1, 5, 5, 1, 5), 1);
  assert.equal(grid.firstHit(2, 3.6, 5, 9, 3.6, 5), 1);
  assert.ok(grid.firstHit(5.9, 1, 5, 9, 1, 5, 0.5) === 1, 'ignores the start of the path');
  assert.equal(grid.firstHit(2, 1, 5, Infinity, 1, 5), 1, 'a runaway endpoint cannot hang the search');
  assert.equal(grid.firstHit(2, 1, 5, NaN, 1, 5), 1);
});

const ROOM_IDS = ['Classroom', 'Corridor', 'Office', 'Restroom', 'GuardRoom', 'L01', 'L02'];
const inside = (rect, x, z) => x >= rect.minX && x <= rect.maxX && z >= rect.minZ && z <= rect.maxZ;
const along = (axis, [x, z]) => (axis === 'z' ? z : x);

test('every passage between two rooms is a doorway, matching the room connections in the rules', () => {
  const pairs = DOORWAYS.map(({ sides }) => Object.keys(sides).sort().join('-')).sort();
  assert.deepEqual(pairs, ['Classroom-Corridor', 'Corridor-GuardRoom', 'Corridor-Office', 'Corridor-Restroom']);
  assert.deepEqual(DOORWAYS.filter(({ door }) => door).map(({ door }) => door).sort(), ['S05', 'S19']);
});

test('each room has its own doorway on its own side, between its arrival point and the other room', () => {
  for (const { axis, sides } of DOORWAYS) {
    const [a, b] = Object.keys(sides);
    const order = [along(axis, sides[a].spawn), sides[a].line, sides[b].line, along(axis, sides[b].spawn)];
    const rising = order.every((v, i) => i === 0 || v >= order[i - 1]);
    const falling = order.every((v, i) => i === 0 || v <= order[i - 1]);
    assert.ok(rising || falling, `${a}-${b}: ${order}`);
    for (const area of [a, b]) {
      const { span, spawn: [x, z] } = sides[area];
      const lateral = axis === 'z' ? x : z;
      assert.ok(lateral > span[0] && lateral < span[1], `${area} arrival is not in front of its doorway`);
    }
  }
});

test('arriving through a doorway lands inside the room, clear of its doorway', () => {
  for (const { axis, sides } of DOORWAYS) {
    for (const [area, { line, spawn }] of Object.entries(sides)) {
      assert.equal(areaAt(...spawn), area);
      assert.ok(Math.abs(along(axis, spawn) - line) >= 0.8, area);
    }
  }
});

test('a room draws its own doorways and arrival points', () => {
  for (const { axis, sides } of DOORWAYS) {
    for (const [area, { line, span, spawn }] of Object.entries(sides)) {
      const rect = roomClipRect(area);
      const mid = (span[0] + span[1]) / 2;
      assert.ok(inside(rect, ...(axis === 'z' ? [mid, line] : [line, mid])), `${area} doorway`);
      assert.ok(inside(rect, ...spawn), `${area} arrival`);
    }
  }
});

test('the corridor entrances are the painted doors on its walls, with no black block on the corridor side', () => {
  const corridorSides = DOORWAYS.map(({ sides }) => sides.Corridor).filter(Boolean);
  const centers = corridorSides.map(({ span }) => (span[0] + span[1]) / 2).sort((a, b) => a - b);
  centers.forEach((x, i) => assert.ok(Math.abs(x - [-6.26, -6.26, 4.08, 14.44][i]) < 0.15, `door ${i} at ${x}`));
  for (const side of corridorSides) {
    assert.equal(side.block, false);
    assert.ok(side.span[1] - side.span[0] <= 1.5);
  }
  assert.ok(Math.max(...centers) + 0.7 < 15.5, 'a corridor entrance reaches into the stairwell');
});

test('the office and guard room sit right behind their corridor doors', () => {
  for (const [room, door] of [['Office', 4.08], ['GuardRoom', 14.44]]) {
    const { sides } = DOORWAYS.find((d) => d.sides[room]);
    const own = (sides[room].span[0] + sides[room].span[1]) / 2;
    assert.ok(Math.abs(own - door) < 0.15, room);
    assert.equal(areaAt(door, -21), room);
  }
});

test('the corridor draws nothing beyond its own walls, so its windows show black', () => {
  const rect = roomClipRect('Corridor');
  assert.equal(inside(rect, 0, -7.4), false);
  assert.equal(inside(rect, 0, -14.6), false);
  assert.ok(inside(rect, 0, -11));
});

test('the drawn region of a room never reaches into the middle of another room', () => {
  const centers = { Classroom: [0, 0], Corridor: [0, -11], Office: [4.1, -21], Restroom: [-11, -21], GuardRoom: [14.4, -21], L01: [38, -9], L02: [38, -21] };
  for (const area of ROOM_IDS) {
    const rect = roomClipRect(area);
    for (const [other, [x, z]] of Object.entries(centers)) {
      if (other !== area) assert.equal(inside(rect, x, z), false, `${area} shows the middle of ${other}`);
    }
  }
});

test('the classroom doorway block covers the door opening in front of the door glass and under the door frame', () => {
  const classroom = DOORWAYS.find((d) => d.sides.Classroom).sides.Classroom;
  assert.ok(classroom.height <= 2.5);
  assert.ok(classroom.line > -5.34);
  assert.ok(classroom.span[0] <= -7.0 && classroom.span[1] >= -4.15);
});

test('every corridor entrance and arrival point is in the main hall, not in the stairwells', () => {
  for (const { sides } of DOORWAYS) {
    const corridor = sides.Corridor;
    if (!corridor) continue;
    assert.ok(Math.abs(corridor.spawn[0]) + 0.22 < CORRIDOR_HALF_LENGTH);
    assert.ok(Math.max(...corridor.span.map(Math.abs)) < CORRIDOR_HALF_LENGTH);
  }
});

test('human movement checks the full body path against room geometry', () => {
  const grid = new OcclusionGrid({ minX: 0, minY: 0, minZ: 0, maxX: 10, maxY: 3, maxZ: 10 });
  grid.addTriangle(6, 0, 0, 6, 3, 0, 6, 0, 10);
  grid.addTriangle(6, 3, 0, 6, 3, 10, 6, 0, 10);
  assert.equal(bodyPathIntersectsGrid(grid, 4.5, 5, 6.5, 5), true);
  assert.equal(bodyPathIntersectsGrid(grid, 4.5, 8, 5.5, 8), false);
});

test('human movement can step over a low obstacle but not a wall', () => {
  const low = new OcclusionGrid({ minX: 0, minY: 0, minZ: 0, maxX: 10, maxY: 3, maxZ: 10 });
  low.addTriangle(6, 0, 0, 6, 0.15, 0, 6, 0, 10);
  low.addTriangle(6, 0.15, 0, 6, 0.15, 10, 6, 0, 10);
  const stepHeight = bodyPathStepHeight(low, 4.5, 5, 6.5, 5);
  assert.ok(stepHeight > 0 && stepHeight <= 0.25);
  assert.equal(bodyPathIntersectsGrid(low, 4.5, 5, 6.5, 5, 0.3, 1.3, stepHeight), false);

  const wall = new OcclusionGrid({ minX: 0, minY: 0, minZ: 0, maxX: 10, maxY: 3, maxZ: 10 });
  wall.addTriangle(6, 0, 0, 6, 1, 0, 6, 0, 10);
  wall.addTriangle(6, 1, 0, 6, 1, 10, 6, 0, 10);
  assert.equal(bodyPathStepHeight(wall, 4.5, 5, 6.5, 5), null);
});

test('human facing rotates toward quarter and half turn targets', () => {
  for (const target of [Math.PI / 2, Math.PI]) {
    let yaw = 0;
    let turn = advancePlayerFacing(yaw, target, 0.1);
    assert.equal(turn.turning, true);
    while (turn.turning) {
      yaw = turn.yaw;
      turn = advancePlayerFacing(yaw, target, 0.1);
    }
    assert.ok(Math.abs(turn.yaw - target) < 1e-9);
    assert.equal(turn.turning, false);
  }
});

test('human walk animation drops root translation while keeping skeletal tracks', () => {
  const root = { uuid: 'hips-node' };
  const position = { name: 'hips-node.position' };
  const rotation = { name: 'hips-node.quaternion' };
  const clip = { tracks: [position, rotation] };
  assert.equal(removeRootTranslationTrack(clip, root), true);
  assert.deepEqual(clip.tracks, [rotation]);
  assert.equal(removeRootTranslationTrack(clip, root), false);
});

test('player model GLB carries Idle, Jumping and Walking animations', async () => {
  const { readFileSync } = await import('node:fs');
  const buffer = readFileSync(new URL('../assets/models/vivi_manga_student_girl_style.glb', import.meta.url));
  const json = JSON.parse(buffer.subarray(20, 20 + buffer.readUInt32LE(12)).toString('utf8'));
  assert.deepEqual(json.animations.map((a) => a.name).sort(), ['Idle', 'Jumping', 'Walking']);
  for (const animation of json.animations) assert.ok(animation.channels.length > 100);
});

test('player clips are chosen by name with a first-clip fallback for walking', () => {
  const walking = { name: 'Walking', tracks: [1] };
  const jumping = { name: 'Jumping', tracks: [1] };
  const idle = { name: 'Idle', tracks: [1] };
  assert.deepEqual(pickPlayerClips([jumping, idle, walking]), { walk: walking, jump: jumping, idle });
  assert.deepEqual(pickPlayerClips([jumping, walking]), { walk: walking, jump: jumping, idle: null });
  const legacy = { name: 'Armature|mixamo.com|Layer0', tracks: [1] };
  assert.deepEqual(pickPlayerClips([{ name: 'empty', tracks: [] }, legacy]), { walk: legacy, jump: null, idle: null });
  assert.deepEqual(pickPlayerClips([]), { walk: null, jump: null, idle: null });
});

test('jump clip time maps airtime onto the takeoff-to-touchdown span of the clip', () => {
  assert.equal(jumpClipTime(0, 0.75, 1.07, 0.55), 0.55);
  assert.ok(Math.abs(jumpClipTime(0.375, 0.75, 1.07, 0.55) - 0.81) < 1e-9);
  assert.equal(jumpClipTime(0.75, 0.75, 1.07, 0.55), 1.07);
  assert.equal(jumpClipTime(2, 0.75, 1.07, 0.55), 1.07);
  assert.equal(jumpClipTime(-1, 0.75, 1.07, 0.55), 0.55);
  assert.equal(JUMP_CLIP_TAKEOFF_TIME, 0.55);
  assert.equal(JUMP_CLIP_LAND_TIME, 1.07);
});

// Loads a bundled GLB in Node: the loader's fetch and image decoding are browser APIs, so they read the file instead.
async function loadModel(file) {
  const browser = { fetch: globalThis.fetch, Image: globalThis.Image };
  globalThis.fetch = async (url) => {
    const bytes = await readFile(url);
    return { ok: true, arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) };
  };
  globalThis.Image = class { decode() { return Promise.resolve(); } };
  try {
    return await loadGlb(new URL(`../assets/models/${file}`, import.meta.url));
  } finally {
    globalThis.fetch = browser.fetch;
    if (browser.Image) globalThis.Image = browser.Image; else delete globalThis.Image;
  }
}

test('every corridor object stands against a corridor wall, on the flat wall between its pillars and doors', async () => {
  const corridorConfig = ROOM_MODELS.find(({ file }) => file === 'rooms/corridor.glb');
  const corridor = await loadModel(corridorConfig.file);
  placeRoomModel(corridor, corridorConfig);
  mirrorCorridorWall(corridor);
  corridor.updateMatrixWorld(true);
  const shown = (node) => { for (let n = node; n; n = n.parent) if (!n.visible) return false; return true; };
  const walls = [];
  corridor.traverse((node) => { if (node.isMesh && shown(node)) walls.push(node); });
  const ray = new THREE.Raycaster();

  const objects = ROOM_MODELS.filter(({ room, file }) => room === 'Corridor' && file.startsWith('objects/'));
  assert.deepEqual(objects.flatMap(({ replaces }) => replaces).sort(), ['S01', 'S02', 'S03', 'S04', 'S10', 'S17']);
  for (const config of objects) {
    const model = await loadModel(config.file);
    placeRoomModel(model, config);
    const bounds = new THREE.Box3().setFromObject(model);
    const center = bounds.getCenter(new THREE.Vector3());
    const north = center.z > (CORRIDOR_WALLS.north + CORRIDOR_WALLS.south) / 2;
    const face = north ? bounds.max.z : bounds.min.z;
    assert.ok(Math.abs(face - (north ? CORRIDOR_WALLS.north : CORRIDOR_WALLS.south)) < 0.03, `${config.file} is not against the wall (${face.toFixed(2)})`);
    // Rays from inside the model to the wall behind it meet the wall right at the model's back: no gap, and no pillar in the way.
    for (const x of [bounds.min.x + 0.1, center.x, bounds.max.x - 0.1]) {
      for (const y of [bounds.min.y + 0.15, center.y]) {
        ray.set(new THREE.Vector3(x, y, center.z), new THREE.Vector3(0, 0, north ? 1 : -1));
        const hit = ray.intersectObjects(walls, false)[0];
        assert.ok(hit, `${config.file}: no wall behind x=${x.toFixed(2)}`);
        assert.ok(Math.abs(hit.point.z - face) < 0.03, `${config.file}: wall at ${hit.point.z.toFixed(2)} behind x=${x.toFixed(2)}, back at ${face.toFixed(2)}`);
      }
    }
    for (const { sides } of DOORWAYS) {
      const side = sides.Corridor;
      if (!side || (side.line > (CORRIDOR_WALLS.north + CORRIDOR_WALLS.south) / 2) !== north) continue;
      assert.ok(bounds.max.x < side.span[0] - 0.3 || bounds.min.x > side.span[1] + 0.3, `${config.file} blocks a doorway`);
    }
    for (const id of config.replaces.filter((target) => target !== 'S04')) {
      const [x, z] = OBJECT_POSITIONS[id];
      assert.ok(Math.hypot(x - center.x, z - center.z) < 0.05, `${id} target is not where its model stands`);
    }
  }
});

test('items found in the corridor bin and flowerpot appear in front of them', () => {
  for (const [item, object] of [['F01', 'S02'], ['F04', 'S10']]) {
    const [x, z] = ITEM_POSITIONS[item];
    const [ox, oz] = OBJECT_POSITIONS[object];
    assert.equal(areaAt(x, z), 'Corridor');
    assert.ok(z < CORRIDOR_WALLS.north - 0.3 && z > CORRIDOR_WALLS.south + 0.3, `${item} is in a wall`);
    assert.ok(Math.hypot(x - ox, z - oz) < 1.6, `${item} is far from ${object}`);
    assert.ok(Math.abs(z - (CORRIDOR_WALLS.north + CORRIDOR_WALLS.south) / 2) < Math.abs(oz - (CORRIDOR_WALLS.north + CORRIDOR_WALLS.south) / 2), `${item} is not in front of ${object}`);
  }
});

// World-space triangles of every shown mesh under `root`.
function shownTriangles(root) {
  root.updateMatrixWorld(true);
  const shown = (node) => { for (let n = node; n; n = n.parent) if (!n.visible) return false; return true; };
  const meshes = [];
  root.traverse((node) => { if (node.isMesh && shown(node)) meshes.push(node); });
  return meshes.map((mesh) => ({ mesh, bounds: new THREE.Box3().setFromObject(mesh) }));
}

function boxTouchesMeshes(box, meshes) {
  const triangle = new THREE.Triangle();
  for (const { mesh, bounds } of meshes) {
    if (!bounds.intersectsBox(box)) continue;
    const position = mesh.geometry.attributes.position;
    const index = mesh.geometry.index;
    const count = index ? index.count : position.count;
    for (let i = 0; i + 2 < count; i += 3) {
      triangle.a.fromBufferAttribute(position, index ? index.getX(i) : i).applyMatrix4(mesh.matrixWorld);
      triangle.b.fromBufferAttribute(position, index ? index.getX(i + 1) : i + 1).applyMatrix4(mesh.matrixWorld);
      triangle.c.fromBufferAttribute(position, index ? index.getX(i + 2) : i + 2).applyMatrix4(mesh.matrixWorld);
      if (box.intersectsTriangle(triangle)) return mesh;
    }
  }
  return null;
}

test('items that appear after an interaction, or are pushed out of the drink machine, do not sink into any model', async () => {
  // Where each item can show up in a room that has models: its spawn spot, and for the drink machine's items the drops beside it.
  const spots = { F01: [ITEM_POSITIONS.F01], F04: [ITEM_POSITIONS.F04], F05: [ITEM_POSITIONS.F05], F06: [ITEM_POSITIONS.F06], F07: [ITEM_POSITIONS.F07] };
  for (const item of ['F01', 'F02', 'F03']) {
    for (const x of [33.5, 38, 42.5]) (spots[item] ??= []).push(dropPoint(x, -9), dropPoint(x, -21));
  }
  const roomMeshes = new Map();
  const meshesOf = async (room) => {
    if (!roomMeshes.has(room)) {
      const scene = new THREE.Group();
      for (const config of ROOM_MODELS.filter((entry) => entry.room === room)) {
        const model = await loadModel(config.file);
        placeRoomModel(model, config);
        if (config.mirrorNorthWall) mirrorCorridorWall(model);
        scene.add(model);
      }
      roomMeshes.set(room, shownTriangles(scene));
    }
    return roomMeshes.get(room);
  };
  for (const [item, points] of Object.entries(spots)) {
    const template = normalizeItemModel(await loadModel(`items/${item.toLowerCase()}.glb`), ITEM_MODELS[item]);
    for (const [x, z] of points) {
      const room = areaAt(x, z);
      template.position.set(x, 0, z);
      template.updateMatrixWorld(true);
      // Resting on the floor is fine; anything else the item's box meets is a model it sinks into.
      const box = new THREE.Box3().setFromObject(template);
      box.min.y += 0.03;
      const hit = boxTouchesMeshes(box, await meshesOf(room));
      assert.ok(!hit, `${item} at ${x.toFixed(2)}, ${z.toFixed(2)} sinks into ${hit?.parent?.name || hit?.name} in ${room}`);
    }
  }
});

test('the camera keeps the player in the middle of the view, straight behind along the view direction', () => {
  for (const form of ['F00', 'F01', 'F08', 'F09', 'F12']) {
    for (const yaw of [0, 0.7, Math.PI / 2, -2.4, Math.PI]) {
      const { position: [cx, , cz], target: [tx, , tz] } = orbitCameraView(3, -11, yaw, form, 0.3);
      assert.ok(Math.hypot(tx - 3, tz + 11) < 1e-9, `${form} at yaw ${yaw} looks beside the player`);
      // The camera sits behind the player: the line from it to the player points the way W walks.
      const [fx, fz] = [Math.sin(yaw), -Math.cos(yaw)];
      const [dx, dz] = [tx - cx, tz - cz];
      assert.ok(Math.abs(dx * fz - dz * fx) < 1e-9, `${form} at yaw ${yaw} is off to the side`);
      assert.ok(dx * fx + dz * fz > 0, `${form} at yaw ${yaw} is in front of the player`);
    }
  }
});

test('standing right up against the drink machine picks the machine at its front and its underside at its back', () => {
  // The interaction boxes of the machine (1.36 wide, 0.78 deep) and the strip behind it (0.3 deep from S04), turned with the machine.
  const turned = (halfWidth, fromAhead, toAhead) => {
    const corners = [[-halfWidth, fromAhead], [halfWidth, fromAhead], [-halfWidth, toAhead], [halfWidth, toAhead]]
      .map(([side, ahead]) => [machineFront[0] * ahead - machineSide[0] * side, machineFront[1] * ahead - machineSide[1] * side]);
    const xs = corners.map(([x]) => x), zs = corners.map(([, z]) => z);
    return { min: { x: Math.min(...xs), z: Math.min(...zs) }, max: { x: Math.max(...xs), z: Math.max(...zs) } };
  };
  const machine = [
    { id: 'S03', x: OBJECT_POSITIONS.S03[0], z: OBJECT_POSITIONS.S03[1], bounds: turned(0.68, -0.39, 0.39) },
    { id: 'S04', x: OBJECT_POSITIONS.S04[0], z: OBJECT_POSITIONS.S04[1], bounds: turned(0.68, 0, 0.3) },
  ];
  const facing = ([dx, dz]) => Math.atan2(dx, -dz);
  // The machine model is about 1.01 deep, so the closest the player stands is a little over 0.5 from its middle.
  for (const ahead of [0.6, 0.67, 0.75, 1]) {
    for (const beside of [-0.3, 0, 0.3]) {
      assert.equal(nearestCandidate(...besideMachine(ahead, beside), facing([-machineFront[0], -machineFront[1]]), machine).id, 'S03', `front ${ahead}, ${beside}`);
      assert.equal(nearestCandidate(...besideMachine(-ahead, beside), facing(machineFront), machine).id, 'S04', `back ${ahead}, ${beside}`);
    }
  }
});
