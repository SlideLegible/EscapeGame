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

test('the bundled Three.js entry module resolves its required core file locally', async () => {
  const module = await readFile(new URL('../vendor/three.module.js', import.meta.url), 'utf8');
  await readFile(new URL('../vendor/three.core.js', import.meta.url), 'utf8');
  assert.match(module, /from '\.\/three\.core\.js'/);
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

test('the page has an achievement book that can be opened from the HUD or with Tab', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  for (const id of ['achievement-button', 'start-achievements', 'ending-achievements', 'achievement-book', 'achievement-entries', 'achievement-count', 'close-achievements']) {
    assert.match(html, new RegExp(`id=["']${id}["']`), `missing ${id}`);
  }
  assert.match(html, /<kbd>Tab<\/kbd>/);
  assert.match(main, /achievementEntries/);
  assert.match(main, /'Tab'/);
  for (const id of ['start-achievements', 'ending-achievements']) assert.match(main, new RegExp(`\\$\\('${id}'\\)\\.addEventListener`));
});

test('players never see developer IDs in achievement or ending screens', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  assert.doesNotMatch(html, />[^<]*\b[EASFX]\d{2}\b[^<]*</);
  assert.doesNotMatch(main, /textContent = [^;]*(\$\{(id|ending)\}|entry\.id)/);
});

test('restarting after an ending starts a new round in the page instead of reloading it', async () => {
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  assert.doesNotMatch(main, /location\.reload/);
  assert.match(main, /game\.newRound\(\)/);
  assert.match(main, /resetRound\(\)/);
});

test('the opening and endings play as click-through stories', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  for (const id of ['story', 'story-title', 'story-lines', 'story-hint', 'skip-story']) {
    assert.match(html, new RegExp(`id=["']${id}["']`), `missing ${id}`);
  }
  assert.match(main, /from '\.\/story\.mjs'/);
  assert.match(main, /OPENING_STORY/);
  assert.match(main, /ENDING_STORIES/);
});

test('nothing the player sees before an ending hints that the game is a dream', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  assert.doesNotMatch(html, /夢|dream/i);
  assert.doesNotMatch(main, /夢/);
});

test('walking through a doorway fades to black and back while the player changes rooms', async () => {
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  const main = await readFile(new URL('../src/main.mjs', import.meta.url), 'utf8');
  assert.match(html, /id=["']fade["']/);
  assert.match(main, /pendingDoorway/);
  assert.match(main, /completeDoorway\(\)/);
});
