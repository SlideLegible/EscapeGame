import { Game, FORMS, ACHIEVEMENTS, achievementEntries } from './core.mjs';
import { AcademyScene } from './scene.mjs';
import { OPENING_STORY, ENDING_STORIES } from './story.mjs';

const game = new Game();
const $ = (id) => document.getElementById(id);
const canvas = $('view');
const startPanel = $('start');
const dialog = $('dialog');
const endingPanel = $('ending');
const messages = $('toast');
const prompt = $('prompt');
const achievementPanel = $('achievements');
const achievementBook = $('achievement-book');
const storyPanel = $('story');
const storyOpen = () => !storyPanel.classList.contains('hidden');
const overlayOpen = () => !dialog.classList.contains('hidden') || !achievementBook.classList.contains('hidden') || storyOpen();
const movementCodes = new Set(['KeyW', 'KeyA', 'KeyS', 'KeyD', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight']);
function movementCode(event) {
  if (movementCodes.has(event.code)) return event.code;
  const key = event.key?.toLowerCase();
  return ({ w: 'KeyW', a: 'KeyA', s: 'KeyS', d: 'KeyD' })[key] ?? null;
}

let scene;
let started = false;
let lastFrame = 0;
let lastHud = 0;
let toastTimer = 0;
let password = '';
let lastAchievementIds = new Set();
let lastShownEnding = null;
let storyQueue = [];
let storyDone = null;

try {
  scene = new AcademyScene(canvas, game);
} catch {
  $('start-copy').textContent = '這個瀏覽器無法啟用 WebGL。請使用支援 WebGL 的新版瀏覽器。';
}

function updateHud() {
  $('health').textContent = `♥ ${game.state.health}`;
  const seconds = game.state.remainingSeconds;
  $('timer').textContent = `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
  $('form-name').textContent = FORMS[game.player().form];
  $('room-name').textContent = scene?.getRoomName() ?? '';
  const target = scene?.nearestTarget();
  const form = game.player().form;
  const selfHint = form === 'F08' ? 'E　讓骰子滾一圈' : form === 'F09' ? 'E　把時鐘指針撥快' : '';
  if (target && started && overlayOpen()) prompt.classList.remove('visible');
  else if (target && started) {
    prompt.textContent = `E　${target.name}`;
    prompt.classList.add('visible');
  } else if (selfHint && started) {
    prompt.textContent = selfHint;
    prompt.classList.add('visible');
  } else prompt.classList.remove('visible');
}

function showMessage(text) {
  if (!text) return;
  messages.textContent = text;
  messages.classList.add('visible');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => messages.classList.remove('visible'), 3600);
}

function updateAchievements() {
  const current = game.player().achievementIds;
  for (const id of current) {
    if (lastAchievementIds.has(id)) continue;
    lastAchievementIds.add(id);
    const item = document.createElement('div');
    item.className = 'achievement';
    const code = document.createElement('small');
    code.textContent = '解鎖成就';
    const name = document.createElement('strong');
    name.textContent = ACHIEVEMENTS[id]?.name ?? id;
    item.append(code, name);
    achievementPanel.append(item);
    setTimeout(() => item.remove(), 4200);
  }
  $('achievement-count').textContent = `${current.size} / ${Object.keys(ACHIEVEMENTS).length}`;
  if (!achievementBook.classList.contains('hidden')) renderAchievementBook();
}

function renderAchievementBook() {
  const entries = achievementEntries(game.player().achievementIds);
  $('achievement-summary').textContent = `已解鎖 ${entries.filter((entry) => entry.unlocked).length} / ${entries.length}。重新整理網頁後會清空。`;
  const list = $('achievement-entries');
  list.replaceChildren();
  for (const entry of entries) {
    const item = document.createElement('div');
    item.className = entry.unlocked ? 'book-entry unlocked' : 'book-entry';
    const name = document.createElement('strong');
    name.textContent = entry.name;
    const condition = document.createElement('span');
    condition.textContent = entry.condition;
    item.append(name, condition);
    list.append(item);
  }
}

function toggleAchievementBook(open = achievementBook.classList.contains('hidden')) {
  if (open && !dialog.classList.contains('hidden')) return;
  if (open) {
    renderAchievementBook();
    scene?.keys.clear();
  }
  achievementBook.classList.toggle('hidden', !open);
  updateHud();
}

function playStory({ title, lines }, onDone) {
  $('story-title').textContent = title;
  $('story-lines').replaceChildren();
  storyQueue = [...lines];
  storyDone = onDone;
  storyPanel.classList.remove('hidden');
  prompt.classList.remove('visible');
  scene?.keys.clear();
  advanceStory();
}

function advanceStory() {
  if (!storyQueue.length) { finishStory(); return; }
  const line = document.createElement('p');
  line.textContent = storyQueue.shift();
  $('story-lines').append(line);
  $('story-hint').textContent = storyQueue.length ? '點一下繼續' : '點一下結束';
}

function finishStory() {
  storyPanel.classList.add('hidden');
  const done = storyDone;
  storyQueue = [];
  storyDone = null;
  done?.();
}

function showEnding(ending) {
  if (!ending || !ENDING_STORIES[ending] || ending !== 'E05' && lastShownEnding === ending) return;
  lastShownEnding = ending;
  playStory(ENDING_STORIES[ending], () => {
    $('ending-title').textContent = ENDING_STORIES[ending].title;
    $('ending-button').textContent = '重新開始 →';
    endingPanel.classList.remove('hidden');
  });
}

function sync(previousForm, result) {
  scene?.syncAfterAction(previousForm, result);
  showMessage(result.text);
  updateAchievements();
  if (result.ending) showEnding(result.ending);
  updateHud();
}

function performInteraction(targetId) {
  if (!targetId || !started || overlayOpen()) return;
  if (targetId === 'S01' && game.player().form === 'F00' && !game.state.powerCut) {
    openKeypad();
    return;
  }
  if (targetId === 'EXIT') {
    const result = scene.exitSpecialLocation();
    showMessage(result.text);
    updateHud();
    return;
  }
  const previousForm = game.player().form;
  const result = targetId === 'SELF' ? game.interact('local', 'SELF') : game.interact('local', targetId);
  sync(previousForm, result);
}

function openKeypad() {
  password = '';
  renderPassword();
  dialog.classList.remove('hidden');
  prompt.classList.remove('visible');
}

function renderPassword() {
  $('password-display').textContent = password.padEnd(4, '_').split('').join(' ');
}

function closeKeypad() {
  dialog.classList.add('hidden');
  password = '';
  updateHud();
}

function useKey(key) {
  if (key === 'clear') password = '';
  else if (key === 'delete') password = password.slice(0, -1);
  else if (/^\d$/.test(key) && password.length < 4) password += key;
  renderPassword();
}

function submitPassword() {
  if (password.length !== 4) { showMessage('請輸入四位數密碼。'); return; }
  const result = game.submitPassword('local', password);
  password = '';
  closeKeypad();
  sync(game.player().form, result);
}

function startGame() {
  if (!scene) {
    $('start-copy').textContent = '這個瀏覽器無法啟用 WebGL。請更新瀏覽器或使用支援 WebGL 的裝置。';
    return;
  }
  started = true;
  scene.started = true;
  startPanel.classList.add('hidden');
  canvas.focus();
  updateHud();
}

function frame(now) {
  requestAnimationFrame(frame);
  const dt = Math.min(0.05, Math.max(0, (now - lastFrame) / 1000 || 0));
  lastFrame = now;
  if (started) {
    if (!overlayOpen() && !game.player().ending) scene?.move(dt);
    if (!game.player().ending && !storyOpen()) game.tick(dt);
    if (now - lastHud > 150) {
      lastHud = now;
      scene?.refresh();
      updateHud();
      updateAchievements();
      showEnding(game.player().ending);
    }
  }
  scene?.render();
}

$('start-button').addEventListener('click', () => {
  if (scene) playStory(OPENING_STORY, startGame);
  else startGame();
});
storyPanel.addEventListener('click', (event) => {
  if (!event.target.closest('#skip-story')) advanceStory();
});
$('skip-story').addEventListener('click', finishStory);
$('close-dialog').addEventListener('click', closeKeypad);
$('achievement-button').addEventListener('click', () => toggleAchievementBook());
$('close-achievements').addEventListener('click', () => toggleAchievementBook(false));
$('start-achievements').addEventListener('click', () => toggleAchievementBook(true));
$('ending-achievements').addEventListener('click', () => toggleAchievementBook(true));
$('submit-password').addEventListener('click', submitPassword);
$('ending-button').addEventListener('click', () => {
  if (game.player().ending && game.player().ending !== 'E05') {
    game.newRound();
    scene?.resetRound();
    lastShownEnding = null;
  }
  endingPanel.classList.add('hidden');
  updateAchievements();
  updateHud();
});
$('keypad').addEventListener('click', (event) => {
  const button = event.target.closest('button[data-key]');
  if (button) useKey(button.dataset.key);
});

window.addEventListener('keydown', (event) => {
  if (storyOpen()) {
    if (['Space', 'Enter', 'KeyE'].includes(event.code) && !event.repeat) advanceStory();
    if (event.code === 'Escape') finishStory();
    if (['Space', 'Enter', 'Tab'].includes(event.code)) event.preventDefault();
    return;
  }
  if (event.code === 'Escape' && !dialog.classList.contains('hidden')) { closeKeypad(); return; }
  if (event.code === 'Escape' && !achievementBook.classList.contains('hidden')) { toggleAchievementBook(false); return; }
  if (event.code === 'Tab') {
    event.preventDefault();
    if (!event.repeat) toggleAchievementBook();
    return;
  }
  if (!started || overlayOpen()) return;
  if (event.code === 'Space') {
    event.preventDefault();
    if (!event.repeat) scene?.jump();
    return;
  }
  const code = movementCode(event);
  if (code) {
    scene?.keys.add(code);
    event.preventDefault();
  }
  if (event.code === 'KeyE' && !event.repeat) {
    const target = scene?.nearestTarget();
    if (target) performInteraction(target.id);
    else if (['F08', 'F09'].includes(game.player().form)) performInteraction('SELF');
  }
  if (event.code === 'KeyQ' && !event.repeat) {
    const previousForm = game.player().form;
    sync(previousForm, game.returnToHuman('local'));
  }
});
window.addEventListener('keyup', (event) => {
  const code = movementCode(event);
  if (code) scene?.keys.delete(code);
});
window.addEventListener('blur', () => scene?.keys.clear());
document.addEventListener('contextmenu', (event) => event.preventDefault(), true);

canvas.addEventListener('pointerdown', (event) => {
  if (!started || overlayOpen()) return;
  scene.pointerDown = { x: event.clientX, y: event.clientY, originX: event.clientX, originY: event.clientY, id: event.pointerId };
  canvas.setPointerCapture(event.pointerId);
});
canvas.addEventListener('pointermove', (event) => {
  if (!scene?.pointerDown || scene.pointerDown.id !== event.pointerId || !started) return;
  const dx = event.clientX - scene.pointerDown.x;
  const dy = event.clientY - scene.pointerDown.y;
  scene.pointerDown.x = event.clientX; scene.pointerDown.y = event.clientY;
  scene.yaw -= dx * 0.005;
  scene.pitch = Math.max(0.05, Math.min(1.25, scene.pitch + dy * 0.004));
});
canvas.addEventListener('pointerup', (event) => {
  if (!scene?.pointerDown || scene.pointerDown.id !== event.pointerId) return;
  const moved = Math.hypot(event.clientX - scene.pointerDown.originX, event.clientY - scene.pointerDown.originY);
  scene.pointerDown = null;
  if (moved < 8) performInteraction(scene.pickTarget(event.clientX, event.clientY));
});

updateHud();
requestAnimationFrame(frame);
