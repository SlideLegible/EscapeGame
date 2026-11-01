import { Game, FORMS } from './core.mjs';
import { AcademyScene } from './scene.mjs';

const game = new Game();
const $ = (id) => document.getElementById(id);
const canvas = $('view');
const startPanel = $('start');
const dialog = $('dialog');
const endingPanel = $('ending');
const messages = $('toast');
const prompt = $('prompt');
const achievementPanel = $('achievements');

const achievementNames = {
  E01: '撿到就是我的', E02: '密室逃脫大神', E03: '誰說一定要走正門',
  E04: '又看到了清晨的太陽', E05: '上課睡覺的代價', A01: '被嘲笑專家',
  A02: '我是誰', A03: '魂不附體', A04: '這是夢，對吧？', A06: '時間管理大師', A07: '鑰匙收藏家',
};
const movementCodes = new Set(['KeyW', 'KeyA', 'KeyS', 'KeyD', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight']);
function movementCode(event) {
  if (movementCodes.has(event.code)) return event.code;
  const key = event.key?.toLowerCase();
  return ({ w: 'KeyW', a: 'KeyA', s: 'KeyS', d: 'KeyD' })[key] ?? null;
}
const endingCopy = {
  E01: ['夢醒', '眼前一片白光。你睜開眼睛，回到了原本的課堂。下課鐘響了，老師說：「好，那我們今天就上到這邊，下課。」原來剛剛的一切都是夢。'],
  E02: ['密室逃脫大神', '四位數密碼剛好被你猜中了。眼前一片白光，你回到了原本的課堂。'],
  E03: ['誰說一定要走正門', '你用長竿頂開高窗，成功離開了學院。眼前一片白光，原來剛剛的一切都是夢。'],
  E04: ['天亮了', '窗外透進清晨的陽光。有人拍了拍你的肩膀：「同學，下課了喔，睡得很熟嘛。」原來你整堂課都在睡。'],
  E05: ['上課睡覺的代價', '夢境崩塌了。你又回到密碼學教室，生命值恢復，重新開始這場夢。'],
};

let scene;
let started = false;
let lastFrame = 0;
let lastHud = 0;
let toastTimer = 0;
let password = '';
let lastAchievementIds = new Set();
let lastShownEnding = null;

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
  if (target && started && !dialog.classList.contains('hidden')) prompt.classList.remove('visible');
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
    code.textContent = `ACHIEVEMENT · ${id}`;
    const name = document.createElement('strong');
    name.textContent = achievementNames[id] ?? id;
    item.append(code, name);
    achievementPanel.append(item);
    setTimeout(() => item.remove(), 4200);
  }
}

function showEnding(ending) {
  if (!ending || !endingCopy[ending] || ending !== 'E05' && lastShownEnding === ending) return;
  lastShownEnding = ending;
  $('ending-id').textContent = `ENDING · ${ending}`;
  $('ending-title').textContent = endingCopy[ending][0];
  $('ending-copy').textContent = endingCopy[ending][1];
  $('ending-button').textContent = ending === 'E05' ? '回到夢境 →' : '重新開始 →';
  endingPanel.classList.remove('hidden');
}

function sync(previousForm, result) {
  scene?.syncAfterAction(previousForm, result);
  showMessage(result.text);
  updateAchievements();
  if (result.ending) showEnding(result.ending);
  updateHud();
}

function performInteraction(targetId) {
  if (!targetId || !started || !dialog.classList.contains('hidden')) return;
  if (targetId === 'S01' && game.player().form === 'F00' && !game.state.powerCut) {
    openKeypad();
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
    if (dialog.classList.contains('hidden') && !game.player().ending) scene?.move(dt);
    if (!game.player().ending) game.tick(dt);
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

$('start-button').addEventListener('click', startGame);
$('close-dialog').addEventListener('click', closeKeypad);
$('submit-password').addEventListener('click', submitPassword);
$('ending-button').addEventListener('click', () => {
  if (game.player().ending && game.player().ending !== 'E05') window.location.reload();
  else endingPanel.classList.add('hidden');
});
$('keypad').addEventListener('click', (event) => {
  const button = event.target.closest('button[data-key]');
  if (button) useKey(button.dataset.key);
});

window.addEventListener('keydown', (event) => {
  if (event.code === 'Escape' && !dialog.classList.contains('hidden')) { closeKeypad(); return; }
  if (!started || !dialog.classList.contains('hidden')) return;
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

canvas.addEventListener('pointerdown', (event) => {
  if (!started || !dialog.classList.contains('hidden')) return;
  scene.pointerDown = { x: event.clientX, y: event.clientY, originX: event.clientX, originY: event.clientY, id: event.pointerId };
  canvas.setPointerCapture(event.pointerId);
});
canvas.addEventListener('pointermove', (event) => {
  if (!scene?.pointerDown || scene.pointerDown.id !== event.pointerId || !started) return;
  const dx = event.clientX - scene.pointerDown.x;
  const dy = event.clientY - scene.pointerDown.y;
  scene.pointerDown.x = event.clientX; scene.pointerDown.y = event.clientY;
  scene.yaw -= dx * 0.005;
  scene.pitch = Math.max(-0.1, Math.min(0.75, scene.pitch + dy * 0.002));
});
canvas.addEventListener('pointerup', (event) => {
  if (!scene?.pointerDown || scene.pointerDown.id !== event.pointerId) return;
  const moved = Math.hypot(event.clientX - scene.pointerDown.originX, event.clientY - scene.pointerDown.originY);
  scene.pointerDown = null;
  if (moved < 8) performInteraction(scene.pickTarget(event.clientX, event.clientY));
});

updateHud();
requestAnimationFrame(frame);
