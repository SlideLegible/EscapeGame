import test from 'node:test';
import assert from 'node:assert/strict';
import { areaAt, areaCameraRect, dropPoint, entryPoint, nearestCandidate, OcclusionGrid, rayRectLimit, SPECIAL_EXITS, OBJECT_POSITIONS, LECTERN_SIZE } from '../src/scene.mjs';

test('walkable room areas touch across the classroom, corridor, office, restroom, and storage thresholds', () => {
  assert.equal(areaAt(7, -6.2), 'Corridor');
  assert.equal(areaAt(11, -15.9), 'Office');
  assert.equal(areaAt(-11, -15.9), 'Restroom');
  assert.equal(areaAt(-16.3, -21), 'Storage');
  assert.equal(areaAt(22, -15.9), 'GuardRoom');
});

test('area lookup still keeps the classroom and corridor interiors distinct', () => {
  assert.equal(areaAt(0, -3), 'Classroom');
  assert.equal(areaAt(0, -11), 'Corridor');
  assert.equal(areaAt(11, -21), 'Office');
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

test('walking from the restroom into the storage room never steps outside every room', () => {
  for (let x = -15.5; x >= -16.5; x -= 0.05) {
    assert.ok(['Restroom', 'Storage'].includes(areaAt(x, -21)), `x=${x.toFixed(2)}`);
  }
});

test('the storage door sits on the wall between the restroom and the storage room', () => {
  const [x, z] = OBJECT_POSITIONS.S09;
  assert.equal(areaAt(x + 0.1, z), 'Restroom');
  assert.equal(areaAt(x - 0.1, z), 'Storage');
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
  assert.ok(office.minZ >= -26 && office.maxZ <= -15.8 && office.minX >= 6 && office.maxX <= 16);
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
