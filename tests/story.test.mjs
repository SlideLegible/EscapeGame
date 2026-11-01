// Click-through opening and ending stories
import test from 'node:test';
import assert from 'node:assert/strict';
import { OPENING_STORY, ENDING_STORIES } from '../src/story.mjs';

test('the opening story follows the design premise in the second person', () => {
  const text = OPENING_STORY.lines.join('');
  for (const phrase of ['密碼學課上睡著', '晚上', '鎖', '學生證', '密碼']) assert.match(text, new RegExp(phrase));
  assert.doesNotMatch(text, /我|夢/);
  assert.ok(OPENING_STORY.lines.length >= 3);
});

const WAKE_UP = ['眼前一片白光。', '你睜開眼睛，回到了原本的課堂。', '下課鐘響了，老師說：「好，那我們今天就上到這邊，下課。」', '原來剛剛的一切都是夢。'];

test('ending stories use the agreed titles and lines', () => {
  const expected = {
    E01: ['夢醒', WAKE_UP],
    E02: ['密室逃脫大神', ['四位數密碼剛好被你猜中了。', ...WAKE_UP]],
    E03: ['誰說一定要走正門', ['你用長竿頂開高窗，成功離開了學院。', ...WAKE_UP]],
    E04: ['天亮了', ['窗外透進清晨的陽光。', '突然眼前一片白光。', '你睜開眼睛，回到了原本的課堂。', '下課鐘響了。', '有人拍了拍你的肩膀：「同學，下課了喔，睡得很熟嘛。」']],
    E05: ['遊戲結束', ['貪玩的後果是可怕的死亡']],
  };
  assert.deepEqual(Object.keys(ENDING_STORIES), Object.keys(expected));
  for (const [id, [title, lines]] of Object.entries(expected)) {
    assert.equal(ENDING_STORIES[id].title, title, id);
    assert.deepEqual(ENDING_STORIES[id].lines, lines, id);
  }
});

test('the health-zero ending looks like an ordinary game over and never mentions a dream or sleep', () => {
  const { title, lines } = ENDING_STORIES.E05;
  assert.doesNotMatch(`${title}${lines.join('')}`, /夢|睡/);
});
