import test from 'node:test';
import assert from 'node:assert/strict';
import { areaAt } from '../src/scene.mjs';

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
