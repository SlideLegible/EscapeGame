// STEP 1–3 rules coverage → VERIFY STEP 1–3
import test from 'node:test';
import assert from 'node:assert/strict';
import { Game } from '../src/core.mjs';

function enterCorridor(game, playerId = 'local') {
  game.transform(playerId, 'F12');
  game.interact(playerId, 'S19');
  game.moveToArea(playerId, 'Corridor');
}

function unlockOffice(game, playerId = 'local') {
  enterCorridor(game, playerId);
  game.returnToHuman(playerId);
  game.moveToArea(playerId, 'Corridor');
  game.interact(playerId, 'S10');
  game.transform(playerId, 'F04');
  game.interact(playerId, 'S05');
  game.moveToArea(playerId, 'Office');
}

function obtainCan(game, playerId = 'local') {
  enterCorridor(game, playerId);
  game.returnToHuman(playerId);
  game.moveToArea(playerId, 'Corridor');
  game.interact(playerId, 'S02');
  game.transform(playerId, 'F01');
}

test('a new round starts with five health, 30 minutes, a human form, and a private four digit password', () => {
  const game = new Game({ random: () => 0.2 });
  assert.equal(game.state.health, 5);
  assert.equal(game.state.remainingSeconds, 1800);
  assert.equal(game.player().form, 'F00');
  assert.equal(Object.hasOwn(game, 'password'), false);
  enterCorridor(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.submitPassword('local', '1031').ending, null);
});

test('classroom chair opens the exit, and unmatched interactions use the default response', () => {
  const game = new Game();
  assert.match(game.interact('local', 'S19').text, /夠重的東西/);
  game.transform('local', 'F12');
  assert.equal(game.interact('local', 'S19').interactionId, 'X036');
  assert.equal(game.isDoorOpen('S19'), true);
  const before = game.state.health;
  const result = game.interact('local', 'S16');
  assert.match(result.text, /講桌紋絲不動/);
  assert.equal(game.state.health, before);
});

test('transformation leaves the human body behind and Q returns to that location', () => {
  const game = new Game();
  game.transform('local', 'F12');
  assert.equal(game.player().form, 'F12');
  assert.equal(game.player().humanArea, 'Classroom');
  game.moveToArea('local', 'Corridor');
  game.returnToHuman('local');
  assert.equal(game.player().form, 'F00');
  assert.equal(game.player().area, 'Classroom');
});

test('item transformations can be swapped and another player cannot occupy the same item', () => {
  const game = new Game();
  game.addPlayer('second');
  game.transform('local', 'F12');
  assert.equal(game.transform('local', 'F08').transformed, true);
  assert.equal(game.item('F12').occupiedBy, null);
  game.transform('second', 'F12');
  assert.equal(game.transform('local', 'F12').blocked, true);
  assert.equal(game.player('local').form, 'F08');
});

test('the drink machine and its underside are special locations for the can form', () => {
  const game = new Game();
  obtainCan(game);
  assert.equal(game.interact('local', 'S03').location, 'L01');
  assert.equal(game.interact('local', 'F02').interactionId, 'X004');
  assert.equal(game.state.health, 6);
  game.transform('local', 'F01');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.transform('local', 'F01');
  game.interact('local', 'S04');
  assert.equal(game.player().area, 'L02');
  assert.equal(game.interact('local', 'F03').transformed, true);
});

test('the first drink heals once, later drinks hurt, and damage restarts the round without losing achievements', () => {
  const game = new Game({ random: () => 0.2 });
  obtainCan(game);
  game.interact('local', 'S03');
  assert.equal(game.transform('local', 'F02').interactionId, 'X004');
  assert.equal(game.state.health, 6);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.transform('local', 'F01');
  assert.equal(game.transform('local', 'F02').interactionId, 'X026');
  assert.equal(game.state.health, 5);
  game.unlock('local', 'A04');
  game.state.health = 1;
  game.damage(1);
  assert.equal(game.state.health, 5);
  assert.equal(game.player().form, 'F00');
  assert.equal(game.hasAchievement('local', 'A04'), true);
});

test('E05 from a repeated exam transformation is shown, restarts the round, and preserves achievements', () => {
  const game = new Game();
  unlockOffice(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Office');
  game.interact('local', 'S08');
  game.unlock('local', 'A04');
  game.state.health = 1;
  const result = game.transform('local', 'F06');
  assert.equal(result.interactionId, 'X027');
  assert.equal(result.ending, 'E05');
  assert.equal(game.state.health, 5);
  assert.equal(game.player().form, 'F00');
  assert.equal(game.hasAchievement('local', 'A04'), true);
  assert.equal(game.hasAchievement('local', 'E05'), true);
});

test('office and storage keys open their doors and spawn the exam and pole', () => {
  const game = new Game();
  unlockOffice(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Office');
  assert.equal(game.interact('local', 'S06').spawned, 'F05');
  assert.equal(game.interact('local', 'S08').spawned, 'F06');
  game.transform('local', 'F05');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Restroom');
  assert.equal(game.interact('local', 'S09').interactionId, 'X013');
  assert.equal(game.item('F07').exists, true);
});

test('the password route accepts the generated code without exposing it in the game state', () => {
  const game = new Game({ random: () => 0.2 });
  enterCorridor(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.submitPassword('local', '0000').ending, null);
  assert.equal(game.submitPassword('local', '2000').ending, 'E02');
  assert.equal(game.player().ending, 'E02');
});

test('1031 is rejected and unlocks the teacher birthday detail on the calendar', () => {
  const game = new Game({ random: () => 0.2 });
  unlockOffice(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Office');
  game.interact('local', 'S08');
  game.moveToArea('local', 'Corridor');
  game.submitPassword('local', '1031');
  game.moveToArea('local', 'Office');
  assert.match(game.interact('local', 'S18').text, /老師生日/);
});

test('power cut permanently disables both door escape methods for the round', () => {
  const game = new Game({ random: () => 0.2 });
  enterCorridor(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'GuardRoom');
  game.interact('local', 'S15');
  game.moveToArea('local', 'Corridor');
  game.interact('local', 'S02');
  game.transform('local', 'F01');
  game.interact('local', 'S04');
  game.transform('local', 'F03');
  game.moveToArea('local', 'Corridor');
  assert.equal(game.interact('local', 'S01').ending, null);
  assert.equal(game.state.powerCut, true);
  game.returnToHuman('local');
  assert.equal(game.submitPassword('local', '1999').ending, null);
});

test('the clock ending applies to active players, while an escaped player is unaffected', () => {
  const game = new Game();
  game.addPlayer('second');
  obtainCan(game);
  game.interact('local', 'S04');
  game.interact('local', 'F03');
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.transform('local', 'F03');
  game.interact('local', 'S01');
  game.transform('second', 'F09');
  const result = game.interact('second', 'SELF');
  assert.equal(result.ending, 'E04');
  assert.equal(game.player('local').ending, 'E01');
  assert.equal(game.player('second').ending, 'E04');
  game.tick(1800);
  assert.equal(game.player('local').ending, 'E01');
});

test('the timer counts down and expires into E04', () => {
  const game = new Game();
  game.tick(1799.4);
  assert.equal(game.state.remainingSeconds, 1);
  game.tick(0.6);
  assert.equal(game.state.remainingSeconds, 0);
  assert.equal(game.player().ending, 'E04');
});

test('the window route triggers E03 and the ending achievement', () => {
  const game = new Game();
  unlockOffice(game);
  game.returnToHuman('local');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Office');
  game.interact('local', 'S06');
  game.transform('local', 'F05');
  game.moveToArea('local', 'Corridor');
  game.moveToArea('local', 'Restroom');
  game.interact('local', 'S09');
  game.transform('local', 'F07');
  assert.equal(game.interact('local', 'S13').ending, 'E03');
  assert.equal(game.hasAchievement('local', 'E03'), true);
});

test('the camera form unlocks whole-map view and tracked achievements stay in memory only', () => {
  const game = new Game();
  enterCorridor(game);
  game.moveToArea('local', 'GuardRoom');
  game.transform('local', 'F10');
  assert.equal(game.player().canSeeWholeMap, true);
  game.unlock('local', 'A04');
  assert.equal(game.hasAchievement('local', 'A04'), true);
  assert.equal(Object.hasOwn(globalThis, 'localStorage'), false);
});

test('default responses, repeated transformations, and time in an item unlock A01, A02, and A03', () => {
  const game = new Game();
  for (let i = 0; i < 10; i++) game.interact('local', 'S16');
  assert.equal(game.hasAchievement('local', 'A01'), true);
  for (let i = 0; i < 20; i++) game.transform('local', i % 2 ? 'F08' : 'F12');
  assert.equal(game.hasAchievement('local', 'A02'), true);
  game.tick(600);
  assert.equal(game.hasAchievement('local', 'A03'), false);
  game.tick(0.02);
  assert.equal(game.hasAchievement('local', 'A03'), true);
});

test('all object IDs and form IDs from the design table are represented', () => {
  const game = new Game();
  for (let i = 1; i <= 20; i++) assert.ok(game.objectName(`S${String(i).padStart(2, '0')}`));
  for (let i = 0; i <= 12; i++) assert.ok(game.formName(`F${String(i).padStart(2, '0')}`));
});

test('switching forms inside the drink machine leaves the can inside, even after the player leaves', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S03');
  game.transform('local', 'F02');
  assert.equal(game.item('F01').area, 'L01');
  assert.equal(game.transform('local', 'F01').transformed, true);
  game.transform('local', 'F02');
  game.returnToHuman('local');
  assert.equal(game.item('F01').area, 'L01');
  assert.equal(game.item('F02').area, 'Corridor');
});

test('the drink can go back into the drink machine and become the can left inside', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S03');
  game.transform('local', 'F02');
  game.moveToArea('local', 'Corridor');
  const result = game.interact('local', 'S03');
  assert.equal(result.interactionId, 'X038');
  assert.equal(result.location, 'L01');
  assert.equal(game.player().area, 'L01');
  assert.equal(game.transform('local', 'F01').transformed, true);
  assert.equal(game.interact('local', 'S04').rejected, true);
});

test('the student ID can slide back under the drink machine and become the can left there', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S04');
  game.transform('local', 'F03');
  assert.equal(game.moveToArea('local', 'Corridor'), true);
  assert.equal(game.item('F01').area, 'L02');
  const result = game.interact('local', 'S04');
  assert.equal(result.interactionId, 'X039');
  assert.equal(result.location, 'L02');
  assert.equal(game.transform('local', 'F01').transformed, true);
  game.transform('local', 'F03');
  game.returnToHuman('local');
  assert.equal(game.item('F01').area, 'L02');
  assert.equal(game.item('F03').area, 'Corridor');
});

test('the student ID cannot get into the drink machine itself', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S04');
  game.transform('local', 'F03');
  game.moveToArea('local', 'Corridor');
  assert.match(game.interact('local', 'S03').text, /飲料機紋絲不動/);
  assert.equal(game.player().area, 'Corridor');
});

test('the drink cannot squeeze under the drink machine', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S03');
  game.transform('local', 'F02');
  game.moveToArea('local', 'Corridor');
  assert.match(game.interact('local', 'S04').text, /飲料機底下紋絲不動/);
  assert.equal(game.player().area, 'Corridor');
});

test('leaving the machine underside as the can leaves an untouched student ID inside', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S04');
  game.returnToHuman('local');
  assert.equal(game.item('F01').area, 'Corridor');
  assert.equal(game.item('F03').area, 'L02');
});

test('an item form can walk out of the drink machine into the corridor while the can stays inside', () => {
  const game = new Game();
  obtainCan(game);
  game.interact('local', 'S03');
  game.transform('local', 'F02');
  assert.equal(game.moveToArea('local', 'Corridor'), true);
  assert.equal(game.player().area, 'Corridor');
  assert.equal(game.player().form, 'F02');
  assert.equal(game.item('F01').area, 'L01');
  assert.equal(game.interact('local', 'S01').interactionId, null);
});
