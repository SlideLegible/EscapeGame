const FORMS = {
  F00: '人類', F01: '飲料罐垃圾', F02: '飲料', F03: '學生證', F04: '辦公室鑰匙',
  F05: '不知道用來做甚麼的鑰匙', F06: '期中考考卷', F07: '長竿子', F08: '骰子',
  F09: '時鐘', F10: '監視器', F11: '一大串鑰匙圈', F12: '椅子',
};

const OBJECTS = {
  S01: '學院大門', S02: '垃圾桶', S03: '飲料機', S04: '飲料機底下', S05: '辦公室門',
  S06: '辦公桌抽屜', S07: '電腦', S08: '牛皮紙包', S09: '儲物間門', S10: '花盆',
  S11: '馬桶', S12: '廁所垃圾桶', S13: '高窗', S14: '黑板', S15: '配電箱',
  S16: '講桌', S17: '公布欄', S18: '月曆', S19: '教室門', S20: '講桌抽屜',
};

const OBJECT_AREAS = {
  S01: 'Corridor', S02: 'Corridor', S03: 'Corridor', S04: 'Corridor', S05: 'Corridor',
  S06: 'Office', S07: 'Office', S08: 'Office', S09: 'Restroom', S10: 'Corridor',
  S11: 'Restroom', S12: 'Restroom', S13: 'Restroom', S14: 'Classroom', S15: 'GuardRoom',
  S16: 'Classroom', S17: 'Corridor', S18: 'Office', S19: 'Classroom', S20: 'Classroom',
};

const INITIAL_ITEMS = ['F08', 'F09', 'F10', 'F11', 'F12'];
const DOORS = ['S01', 'S05', 'S09', 'S19'];
const DEFAULT_TEXT = (name) => `${name}紋絲不動，好像在嘲笑你：「就這？你是認真的嗎？」`;

function seededRandom(seed) {
  let value = seed >>> 0;
  return () => {
    value += 0x6D2B79F5;
    let t = value;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function freshItems() {
  return Object.fromEntries(Object.keys(FORMS).filter((id) => id !== 'F00').map((id) => [id, {
    exists: INITIAL_ITEMS.includes(id), occupiedBy: null, area: id === 'F08' || id === 'F09' || id === 'F12' ? 'Classroom' : id === 'F10' || id === 'F11' ? 'GuardRoom' : null,
  }]));
}

export class Game {
  #password;
  #random;
  #elapsed = 0;
  #drinkClaimed = false;
  #tried1031 = false;
  #doorTouches = new Map();

  constructor({ seed = Date.now(), random } = {}) {
    this.#random = random ?? seededRandom(seed);
    this.players = new Map();
    this.addPlayer('local');
    this.state = { health: 5, remainingSeconds: 1800, powerCut: false, doors: new Set(), items: freshItems(), achievements: new Set(), endedRound: false };
    this.#password = this.#newPassword();
  }

  #newPassword() {
    const n = Math.floor(this.#random() * 9999);
    return String(n >= 1031 ? n + 1 : n).padStart(4, '0');
  }

  addPlayer(id) {
    if (this.players.has(id)) return this.players.get(id);
    const player = { id, form: 'F00', area: 'Classroom', humanArea: 'Classroom', ending: null, achievementIds: new Set(), transformCount: 0, defaultCount: 0, nonHumanSeconds: 0, canSeeWholeMap: false };
    this.players.set(id, player);
    this.#doorTouches.set(id, new Set());
    return player;
  }

  player(id = 'local') { return this.players.get(id); }
  item(id) { return this.state.items[id]; }
  formName(id) { return FORMS[id]; }
  objectName(id) { return OBJECTS[id]; }
  isDoorOpen(id) { return this.state.doors.has(id); }
  hasAchievement(playerId, id) { return this.player(playerId)?.achievementIds.has(id) ?? false; }

  unlock(playerId, id) {
    const player = this.player(playerId);
    if (!player || player.achievementIds.has(id)) return false;
    player.achievementIds.add(id);
    this.state.achievements.add(id);
    return true;
  }

  transform(playerId, formId) {
    const player = this.player(playerId);
    const target = this.item(formId);
    if (!player || player.ending || !target?.exists) return { rejected: true, text: '' };
    if (target.occupiedBy && target.occupiedBy !== playerId) return { blocked: true, text: '' };
    if (player.form === formId) return { rejected: true, text: '' };
    if (target.area && target.area !== player.area) return { rejected: true, text: '' };

    if (player.form === 'F00') player.humanArea = player.area;
    else {
      const old = this.item(player.form);
      old.occupiedBy = null;
      old.area = player.area === 'L01' || player.area === 'L02' ? 'Corridor' : player.area;
    }
    target.occupiedBy = playerId;
    target.area = null;
    player.form = formId;
    player.transformCount += 1;
    if (player.transformCount >= 20) this.unlock(playerId, 'A02');
    if (formId === 'F10') player.canSeeWholeMap = true;
    let interactionId = null;
    let text = '';
    if (formId === 'F06') { interactionId = 'X027'; text = '你看到了考題，受到精神傷害'; this.damage(1, text); }
    if (formId === 'F02') {
      if (!this.#drinkClaimed) { this.#drinkClaimed = true; this.state.health += 1; interactionId = 'X004'; }
      else { interactionId = 'X026'; text = '喝太多，肚子痛'; this.damage(1, text); }
    }
    return { transformed: true, form: formId, interactionId, text };
  }

  returnToHuman(playerId = 'local') {
    const player = this.player(playerId);
    if (!player || player.ending) return { rejected: true };
    if (player.form !== 'F00') {
      const item = this.item(player.form);
      item.occupiedBy = null;
      item.area = player.area === 'L01' || player.area === 'L02' ? 'Corridor' : player.area;
      player.form = 'F00';
    }
    player.area = player.humanArea;
    player.nonHumanSeconds = 0;
    return { transformed: true, form: 'F00' };
  }

  moveToArea(playerId, area) {
    const player = this.player(playerId);
    if (!player || player.ending) return false;
    const from = player.area;
    const allowed = from === area ||
      (from === 'Classroom' && area === 'Corridor' && this.isDoorOpen('S19')) ||
      (from === 'Corridor' && area === 'Classroom') ||
      (from === 'Corridor' && area === 'Office' && this.isDoorOpen('S05')) ||
      (from === 'Office' && area === 'Corridor') ||
      (from === 'Corridor' && ['Restroom', 'GuardRoom'].includes(area)) ||
      (['Restroom', 'GuardRoom'].includes(from) && area === 'Corridor') ||
      (from === 'Restroom' && area === 'Storage' && this.isDoorOpen('S09')) ||
      (from === 'Storage' && area === 'Restroom') ||
      (['L01', 'L02'].includes(from) && area === 'Corridor');
    if (!allowed) return false;
    player.area = area;
    if (player.form !== 'F00') this.item(player.form).area = area;
    return true;
  }

  #end(playerId, ending) {
    const affected = ending === 'E04' || ending === 'E05'
      ? [...this.players.values()].filter((p) => !p.ending)
      : [this.player(playerId)].filter((p) => p && !p.ending);
    for (const player of affected) {
      player.ending = ending;
      this.unlock(player.id, ending);
      if (player.form !== 'F00') {
        const item = this.item(player.form);
        item.occupiedBy = null;
        item.area = player.area;
        player.form = 'F00';
      }
    }
    return affected.map((p) => p.id);
  }

  damage(amount = 1, text = '') {
    this.state.health = Math.max(0, this.state.health - amount);
    if (this.state.health === 0) {
      this.#end('local', 'E05');
      this.#restartRound();
      return { ending: 'E05', text };
    }
    return { text };
  }

  #restartRound() {
    this.state.health = 5;
    this.state.remainingSeconds = 1800;
    this.state.powerCut = false;
    this.state.doors.clear();
    this.state.endedRound = false;
    this.#elapsed = 0;
    this.#drinkClaimed = false;
    this.#tried1031 = false;
    this.state.items = freshItems();
    this.#password = this.#newPassword();
    for (const player of this.players.values()) {
      if (player.ending !== 'E05') continue;
      player.ending = null;
      player.form = 'F00';
      player.area = 'Classroom';
      player.humanArea = 'Classroom';
      player.nonHumanSeconds = 0;
    }
  }

  #resolve(playerId, interactionId, text, extra = {}) {
    const result = { interactionId, text: text ?? '', ending: null, spawned: null, location: null, ...extra };
    if (extra.damage) {
      const damage = this.damage(extra.damage, result.text);
      result.ending = damage.ending ?? null;
    }
    if (extra.ending) {
      result.ending = extra.ending;
      result.affectedPlayers = this.#end(playerId, extra.ending);
    }
    return result;
  }

  interact(playerId = 'local', targetId) {
    const player = this.player(playerId);
    if (!player || player.ending) return { rejected: true, text: '' };
    if (targetId === 'SELF') {
      if (player.form === 'F08') return this.#resolve(playerId, 'X020', `你滾了一圈，停在了 ${1 + Math.floor(this.#random() * 6)} 點`);
      if (player.form === 'F09') {
        this.unlock(playerId, 'A06');
        this.state.endedRound = true;
        return this.#resolve(playerId, 'X021', '你把指針撥快……窗外天亮了', { ending: 'E04' });
      }
      return this.#default(playerId, '自己');
    }

    if (FORMS[targetId]) return this.transform(playerId, targetId);
    const name = OBJECTS[targetId];
    if (!name || OBJECT_AREAS[targetId] !== player.area) return { rejected: true, text: '' };

    if (player.form === 'F11' && DOORS.includes(targetId)) {
      this.#doorTouches.get(playerId).add(targetId);
      if (DOORS.every((id) => this.#doorTouches.get(playerId).has(id))) this.unlock(playerId, 'A07');
      return this.#resolve(playerId, 'X033', '你試了每一支鑰匙……沒有一支插得進去');
    }

    const form = player.form;
    if (targetId === 'S19') {
      if (!this.isDoorOpen('S19') && form === 'F12') { this.state.doors.add('S19'); return this.#resolve(playerId, 'X036', '「砰！」你用盡全力撞了上去，教室門被撞開了'); }
      if (!this.isDoorOpen('S19')) return this.#resolve(playerId, 'X037', '門被鎖住了，也許可以找個夠重的東西把它撞開');
    }
    if (targetId === 'S14' && form === 'F00') return this.#resolve(playerId, 'X018', '黑板上寫著一串奇怪的字：WKH SDVVZRUG LV QRW KHUH');
    if ((targetId === 'S14' || targetId === 'S16') && form === 'F08') return this.#resolve(playerId, targetId === 'S14' ? 'X028' : 'X029', '你從講桌滾下去，摔得頭暈眼花', { damage: 1 });
    if (targetId === 'S20') return this.#resolve(playerId, 'X030', '抽屜鎖得死死的，好像在說：「別白費力氣了，你永遠打不開我。」');
    if (targetId === 'S02' && form === 'F00') return this.#spawn(playerId, 'X001', 'F01', '從垃圾桶裡翻出一個飲料罐垃圾');
    if (targetId === 'S03' && form === 'F01') return this.#enterSpecial(playerId, 'X002', 'L01', 'F02', '鑽進飲料機裡，發現一罐飲料');
    if (targetId === 'S04' && form === 'F01') return this.#enterSpecial(playerId, 'X003', 'L02', 'F03', '滾進飲料機底下，發現一張學生證');
    if (targetId === 'S01' && form === 'F03' && !this.state.powerCut) return this.#resolve(playerId, 'X010', '刷學生證，大門打開了', { ending: 'E01' });
    if (targetId === 'S10' && form === 'F00') {
      this.#spawn(playerId, 'X009', 'F04', '花盆砸到了你的腳，土裡掉出一支鑰匙');
      return this.#resolve(playerId, 'X009', '花盆砸到了你的腳，土裡掉出一支鑰匙', { damage: 1, spawned: 'F04' });
    }
    if (targetId === 'S05' && form === 'F04') { this.state.doors.add('S05'); return this.#resolve(playerId, 'X005', '打開辦公室門'); }
    if (targetId === 'S06' && form === 'F00') return this.#spawn(playerId, 'X006', 'F05', '打開抽屜，裡面有一支不知道用來做甚麼的鑰匙');
    if (targetId === 'S07' && form === 'F00') return this.#resolve(playerId, 'X007', '電腦需要密碼才能登入，打不開');
    if (targetId === 'S08' && form === 'F00') { this.unlock(playerId, 'A04'); return this.#spawn(playerId, 'X008', 'F06', '拆開牛皮紙包，裡面竟然是期中考考卷'); }
    if (targetId === 'S18' && form === 'F00') return this.#resolve(playerId, 'X035', this.#tried1031 ? '10 月 31 日被紅筆圈了起來，旁邊多了一行小字：老師生日🎂' : '10 月 31 日被紅筆圈了起來');
    if (targetId === 'S17' && form === 'F00') return this.#resolve(playerId, 'X031', '門禁密碼更改公告：即日起，新密碼為——後面被撕掉了，只剩一角');
    if (targetId === 'S09' && form === 'F05') { this.state.doors.add('S09'); this.#spawn(playerId, 'X013', 'F07', '成功打開儲物間的門，裡面有一支很長的竿子'); return this.#resolve(playerId, 'X013', '成功打開儲物間的門，裡面有一支很長的竿子', { spawned: 'F07' }); }
    if (targetId === 'S09') return this.#resolve(playerId, 'X014', '上鎖的門，應該是儲物間吧？');
    if (targetId === 'S13' && form === 'F07') return this.#resolve(playerId, 'X015', '用竿子頂開高窗，成功逃出', { ending: 'E03' });
    if (targetId === 'S11') return form === 'F00' ? this.#resolve(playerId, 'X011', '你舒服的上了個廁所') : this.#resolve(playerId, 'X024', '你被沖進了下水道……又爬了回來', { damage: 1 });
    if (targetId === 'S12' && form === 'F00') return this.#resolve(playerId, 'X012', '你為什麼會想在這裡翻到東西');
    if (targetId === 'S15') return form === 'F00' ? this.#cutPower(playerId) : this.#resolve(playerId, 'X025', '滋滋滋——你觸電了', { damage: 1 });
    if (targetId === 'S10' && form !== 'F00') return this.#default(playerId, name);
    return this.#default(playerId, name);
  }

  #default(playerId, name) {
    const player = this.player(playerId);
    player.defaultCount += 1;
    if (player.defaultCount >= 10) this.unlock(playerId, 'A01');
    return this.#resolve(playerId, null, DEFAULT_TEXT(name));
  }

  #spawn(playerId, interactionId, formId, text) {
    const item = this.item(formId);
    if (!item.exists) { item.exists = true; item.area = this.player(playerId).area; }
    return this.#resolve(playerId, interactionId, text, { spawned: formId });
  }

  #enterSpecial(playerId, interactionId, location, formId, text) {
    const item = this.item(formId);
    if (!item.exists) { item.exists = true; item.area = location; }
    this.player(playerId).area = location;
    return this.#resolve(playerId, interactionId, text, { location, spawned: formId });
  }

  #cutPower(playerId) {
    this.state.powerCut = true;
    return this.#resolve(playerId, 'X022', '喀——整棟樓瞬間安靜了下來，門禁機的燈也熄了');
  }

  submitPassword(playerId = 'local', code = '') {
    const player = this.player(playerId);
    if (!player || player.ending || player.form !== 'F00' || player.area !== 'Corridor') return { rejected: true, text: '' };
    if (code === '1031') this.#tried1031 = true;
    if (!this.state.powerCut && code === this.#password) return this.#resolve(playerId, 'X016', '輸入正確，大門打開了', { ending: 'E02' });
    if (this.#random() < 0.001) return this.#resolve(playerId, 'X017', '門紋絲不動，好像有點死了。「手指抽筋了」', { damage: 1 });
    return this.#resolve(playerId, 'X017', '門紋絲不動，好像有點死了');
  }

  tick(dt) {
    if (!(dt > 0) || this.state.endedRound) return;
    this.#elapsed = Math.min(1800, this.#elapsed + dt);
    this.state.remainingSeconds = Math.ceil(Math.max(0, 1800 - this.#elapsed - 1e-9));
    for (const player of this.players.values()) {
      if (!player.ending && player.form !== 'F00') {
        player.nonHumanSeconds += dt;
        if (player.nonHumanSeconds > 600) this.unlock(player.id, 'A03');
      }
    }
    if (this.#elapsed >= 1800) {
      this.state.endedRound = true;
      this.#resolve('local', 'E04', '天亮了', { ending: 'E04' });
    }
  }
}

export { FORMS, OBJECTS, OBJECT_AREAS, DEFAULT_TEXT };
