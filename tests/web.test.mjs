// STEP 4 static browser shell → VERIFY STEP 0, 4, 5
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

test('the page loads only local Three.js and game modules', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  const scene = await readFile(new URL('../src/scene.mjs', import.meta.url), 'utf8');
  assert.match(html, /src\/main\.mjs/);
  assert.match(scene, /\.\.\/vendor\/three\.module\.js/);
  assert.match(main, /\.\/core\.mjs/);
  assert.doesNotMatch(`${html}\n${main}`, /https?:\/\//);
});

test('the static page exposes the required game HUD and controls', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  for (const id of ['game', 'health', 'timer', 'form-name', 'prompt', 'dialog', 'keypad', 'ending', 'achievements', 'start']) {
    assert.match(html, new RegExp(`id=["']${id}["']`), `missing ${id} UI element`);
  }
  assert.match(html, /WebGL/i);
  assert.match(html, /WASD/);
});

test('game UI uses text nodes for messages and does not persist round state', async () => {
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  assert.match(main, /textContent/);
  assert.doesNotMatch(main, /innerHTML|localStorage|sessionStorage/);
});
