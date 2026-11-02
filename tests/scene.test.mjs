import test from 'node:test';
import assert from 'node:assert/strict';
import { advancePlayerFacing, areaAt, areaCameraRect, bodyPathIntersectsGrid, bodyPathStepHeight, ITEM_POSITIONS, dropPoint, entryPoint, nearestCandidate, OcclusionGrid, rayRectLimit, removeRootTranslationTrack, roomClipRect, DOORWAYS, CORRIDOR_HALF_LENGTH, SPECIAL_EXITS, OBJECT_POSITIONS, LECTERN_SIZE } from '../src/scene.mjs';

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

test('approaching the drink machine from the front reaches the machine, and from the back reaches its underside', () => {
  const [mx, mz] = OBJECT_POSITIONS.S03;
  const nearer = (x, z) => {
    const machine = Math.hypot(x - mx, z - mz);
    const underside = Math.hypot(x - OBJECT_POSITIONS.S04[0], z - OBJECT_POSITIONS.S04[1]);
    return machine < underside ? 'S03' : 'S04';
  };
  for (const dx of [-0.5, 0, 0.5]) {
    assert.equal(nearer(mx + dx, mz + 1.2), 'S03');
    assert.equal(nearer(mx + dx, mz - 1.2), 'S04');
  }
  assert.equal(areaAt(...OBJECT_POSITIONS.S04), 'Corridor');
});

test('leaving each special location puts the player on the side of the machine it was entered from', () => {
  const [, mz] = OBJECT_POSITIONS.S03;
  assert.ok(SPECIAL_EXITS.L01.to[1] > mz);
  assert.ok(SPECIAL_EXITS.L02.to[1] < mz);
  assert.ok(dropPoint(38, -9)[1] > mz);
  assert.ok(dropPoint(38, -21)[1] < mz);
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
  const machine = () => [
    { id: 'S03', x: OBJECT_POSITIONS.S03[0], z: OBJECT_POSITIONS.S03[1], bounds: box(1.36, 0.78) },
    { id: 'S04', x: OBJECT_POSITIONS.S04[0], z: OBJECT_POSITIONS.S04[1], bounds: { min: { x: -0.68, z: 0 }, max: { x: 0.68, z: 0.3 } } },
  ];
  assert.equal(nearestCandidate(-14, -9, 0, machine()).id, 'S03');
  assert.equal(nearestCandidate(-14, -11.3, Math.PI, machine()).id, 'S04');
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
