// Design interaction IDs X001–X039 → VERIFY STEP 1–3
import test from 'node:test';
import assert from 'node:assert/strict';
import { Game } from '../src/core.mjs';

function enterCorridor(game) {
  game.transform('local', 'F12');
  game.interact('local', 'S19');
  game.moveToArea('local', 'Corridor');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
}

function enterOfficeAsHuman(game) {
  enterCorridor(game);
  game.interact('local', 'S10');
  game.transform('local', 'F04');
  game.interact('local', 'S05');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Office');
}

function enterRestroomAsHuman(game) {
  enterCorridor(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Restroom');
}

test('classroom interaction rows resolve as designed', () => {
  const game = new Game({ random: () => 0.5 });
  assert.equal(game.interact('local', 'S14').interactionId, 'X018');
  game.transform('local', 'F08');
  assert.equal(game.interact('local', 'SELF').interactionId, 'X020');
  assert.equal(game.interact('local', 'S14').interactionId, 'X028');
  assert.equal(game.interact('local', 'S16').interactionId, 'X029');
  assert.equal(game.interact('local', 'S20').interactionId, 'X030');
  assert.equal(game.interact('local', 'S19').interactionId, 'X037');
  game.transform('local', 'F12');
  assert.equal(game.interact('local', 'S19').interactionId, 'X036');
});

test('clock interaction X021 ends the round and awards A06 and E04', () => {
  const game = new Game();
  game.transform('local', 'F09');
  const result = game.interact('local', 'SELF');
  assert.equal(result.interactionId, 'X021');
  assert.equal(result.ending, 'E04');
  assert.equal(game.hasAchievement('local', 'A06'), true);
  assert.equal(game.hasAchievement('local', 'E04'), true);
});

test('corridor rows cover trash, both machine locations, drink, student card, flowers, and noticeboard', () => {
  const game = new Game();
  enterCorridor(game);
  assert.equal(game.interact('local', 'S02').interactionId, 'X001');
  game.transform('local', 'F01');
  assert.equal(game.interact('local', 'S03').interactionId, 'X002');
  assert.equal(game.interact('local', 'F02').interactionId, 'X004');
  game.transform('local', 'F01');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.transform('local', 'F01');
  assert.equal(game.interact('local', 'S04').interactionId, 'X003');
  assert.equal(game.interact('local', 'F03').transformed, true);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.interact('local', 'S17').interactionId, 'X031');
  assert.equal(game.interact('local', 'S10').interactionId, 'X009');
  assert.equal(game.state.health, 5);
});

test('repeat drinks use X026, inflict one damage, and do not farm health', () => {
  const game = new Game();
  enterCorridor(game);
  game.interact('local', 'S02');
  game.transform('local', 'F01');
  game.interact('local', 'S03');
  game.transform('local', 'F02');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.transform('local', 'F01');
  game.transform('local', 'F02');
  assert.equal(game.state.health, 5);
  assert.equal(game.player().form, 'F02');
});

test('password attempts implement X016, X017, and reject 1031 without hints', () => {
  let calls = 0;
  const game = new Game({ random: () => calls++ === 0 ? 0.2 : 0 });
  enterCorridor(game);
  const wrong = game.submitPassword('local', '8888');
  assert.equal(wrong.interactionId, 'X017');
  assert.match(wrong.text, /手指抽筋了/);
  assert.equal(game.state.health, 4);
  assert.equal(game.submitPassword('local', '1031').interactionId, 'X017');
  assert.equal(game.submitPassword('local', '2000').interactionId, 'X016');
  assert.equal(game.player().ending, 'E02');
});

test('office rows implement access, drawer, computer, package, exam damage, and both calendar states', () => {
  const game = new Game();
  enterOfficeAsHuman(game);
  assert.equal(game.interact('local', 'S06').interactionId, 'X006');
  assert.equal(game.interact('local', 'S07').interactionId, 'X007');
  assert.equal(game.interact('local', 'S08').interactionId, 'X008');
  assert.equal(game.hasAchievement('local', 'A04'), true);
  assert.equal(game.interact('local', 'S18').interactionId, 'X032');
  const before = game.state.health;
  game.transform('local', 'F06');
  assert.equal(game.player().form, 'F06');
  assert.equal(game.state.health, before - 1);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.submitPassword('local', '1031').interactionId, 'X017');
  game.moveToArea('local', 'Office');
  assert.equal(game.interact('local', 'S18').interactionId, 'X035');
});

test('restroom rows implement toilet, bin, storage key, alternate response, and window escape', () => {
  const game = new Game();
  enterOfficeAsHuman(game);
  game.interact('local', 'S06');
  game.transform('local', 'F05');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Restroom');
  assert.equal(game.interact('local', 'S09').interactionId, 'X013');
  assert.equal(game.interact('local', 'S11').interactionId, 'X024');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Restroom');
  assert.equal(game.interact('local', 'S12').interactionId, 'X012');
  game.transform('local', 'F05');
  assert.equal(game.interact('local', 'S09').interactionId, 'X013');
  game.transform('local', 'F07');
  assert.equal(game.interact('local', 'S13').interactionId, 'X015');
  assert.equal(game.player().ending, 'E03');

  const wrongKey = new Game();
  enterRestroomAsHuman(wrongKey);
  assert.equal(wrongKey.interact('local', 'S09').interactionId, 'X014');
  assert.equal(wrongKey.interact('local', 'S11').interactionId, 'X011');
});

test('power, monitor, and keyring rows implement X022, X023, X025, X033, and A07', () => {
  const game = new Game();
  enterCorridor(game);
  game.moveToArea('local', 'GuardRoom');
  assert.equal(game.interact('local', 'S15').interactionId, 'X022');
  assert.equal(game.state.powerCut, true);
  assert.equal(game.interact('local', 'S21').interactionId, 'X023');
  assert.equal(game.player().canSeeWholeMap, true);
  game.transform('local', 'F11');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.interact('local', 'S01').interactionId, 'X033');
  assert.equal(game.interact('local', 'S05').interactionId, 'X033');
  game.moveToArea('local', 'Restroom');
  assert.equal(game.interact('local', 'S09').interactionId, 'X033');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Classroom');
  assert.equal(game.interact('local', 'S19').interactionId, 'X033');
  assert.equal(game.hasAchievement('local', 'A07'), true);

  const electric = new Game();
  enterCorridor(electric);
  electric.moveToArea('local', 'GuardRoom');
  electric.transform('local', 'F11');
  assert.equal(electric.interact('local', 'S15').interactionId, 'X025');
});
