import test from 'node:test';
import assert from 'node:assert/strict';
import { areaAt, dropPoint, entryPoint, SPECIAL_EXITS } from '../src/scene.mjs';

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
