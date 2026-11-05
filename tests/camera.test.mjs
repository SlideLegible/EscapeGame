import test from 'node:test';
import assert from 'node:assert/strict';
import * as scene from '../src/scene.mjs';

const DT = 1 / 60;
const PLAYER_Y = 0.78;

// A field of waist-high, desk-sized blocks (vertical faces only, like the real occlusion grid is fed).
function deskField() {
  const grid = new scene.OcclusionGrid({ minX: -10, minY: 0, minZ: -5, maxX: 40, maxY: 3, maxZ: 12 });
  const quad = (a, b, c, d) => { grid.addTriangle(...a, ...b, ...c); grid.addTriangle(...a, ...c, ...d); };
  for (let x = 0; x < 40; x += 1.6) for (let z = 1; z < 8; z += 1.6) {
    const w = 0.7, h = 1;
    quad([x, 0, z], [x + w, 0, z], [x + w, h, z], [x, h, z]);
    quad([x, 0, z + w], [x + w, 0, z + w], [x + w, h, z + w], [x, h, z + w]);
    quad([x, 0, z], [x, 0, z + w], [x, h, z + w], [x, h, z]);
    quad([x + w, 0, z], [x + w, 0, z + w], [x + w, h, z + w], [x + w, h, z]);
  }
  return grid;
}

function wall(z) {
  const grid = new scene.OcclusionGrid({ minX: -10, minY: 0, minZ: -5, maxX: 10, maxY: 3, maxZ: 12 });
  grid.addTriangle(-5, 0, z, 5, 0, z, 5, 3, z);
  grid.addTriangle(-5, 0, z, 5, 3, z, -5, 3, z);
  return grid;
}

const distance = (camera, player) => Math.hypot(camera.x - player.x, camera.y - player.y, camera.z - player.z);

test('the form camera does not pop in and out while walking past furniture', () => {
  const grid = deskField();
  const state = { orbit: { x: 0, y: 3, z: 6 } };
  const distances = [];
  for (let i = 0; i < 400; i++) {
    const player = { x: 1 + Math.min(i, 300) * 4.4 * DT, y: PLAYER_Y, z: 0.3 + 0.35 * Math.sin(i / 50) };
    const desired = { x: player.x, y: 3, z: player.z + 6 };
    distances.push(distance(scene.stepFollowCamera(state, player, desired, DT, { grid }), player));
  }
  let variation = 0, jerky = 0;
  for (let i = 61; i < 300; i++) {
    const change = Math.abs(distances[i] - distances[i - 1]);
    variation += change;
    if (change > 0.05) jerky++;
  }
  assert.ok(variation < 20, `camera distance moved ${variation.toFixed(1)} units in total`);
  assert.ok(jerky < 50, `camera distance jumped on ${jerky} frames`);
});

test('the form camera holds still behind an obstacle once the player stops', () => {
  const grid = wall(3);
  const player = { x: 0, y: PLAYER_Y, z: 0 };
  const desired = { x: 0, y: 3, z: 6 };
  const state = { orbit: { ...desired } };
  let previous = null, steadiest = 0;
  for (let i = 0; i < 240; i++) {
    const camera = scene.stepFollowCamera(state, player, desired, DT, { grid });
    if (i >= 120 && previous) steadiest = Math.max(steadiest, Math.hypot(camera.x - previous.x, camera.y - previous.y, camera.z - previous.z));
    previous = { ...camera };
  }
  assert.ok(steadiest < 1e-6, `camera still moved ${steadiest} per frame`);
  assert.ok(previous.z < 3, 'camera stays on the player side of the wall');
});

test('the form camera eases back out slowly after the obstacle is gone', () => {
  const player = { x: 0, y: PLAYER_Y, z: 0 };
  const desired = { x: 0, y: 3, z: 6 };
  const state = { orbit: { ...desired } };
  for (let i = 0; i < 60; i++) scene.stepFollowCamera(state, player, desired, DT, { grid: wall(3) });
  const pulledIn = distance(scene.stepFollowCamera(state, player, desired, DT, { grid: wall(3) }), player);
  const open = new scene.OcclusionGrid({ minX: -10, minY: 0, minZ: -5, maxX: 10, maxY: 3, maxZ: 12 });
  let previous = pulledIn, biggestStep = 0;
  for (let i = 0; i < 400; i++) {
    const now = distance(scene.stepFollowCamera(state, player, desired, DT, { grid: open }), player);
    biggestStep = Math.max(biggestStep, now - previous);
    previous = now;
  }
  assert.ok(biggestStep < 0.12, `camera backed out ${biggestStep.toFixed(2)} units in one frame`);
  assert.ok(Math.abs(previous - Math.hypot(0, 3 - PLAYER_Y, 6)) < 0.05, 'camera ends up at its free distance');
});

test('a snapped form camera lands on the desired point', () => {
  const player = { x: 2, y: PLAYER_Y, z: 1 };
  const desired = { x: 2, y: 3, z: 7 };
  const state = { orbit: { x: 40, y: 3, z: 40 } };
  const camera = scene.stepFollowCamera(state, player, desired, DT, { grid: null, snap: true });
  assert.deepEqual([camera.x, camera.y, camera.z], [2, 3, 7]);
});
