import test from 'node:test';
import assert from 'node:assert/strict';
import { areaAt, dropPoint } from '../src/scene.mjs';

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

