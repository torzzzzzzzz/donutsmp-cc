'use strict';
const assert = require('assert');
const { createEngine, VK_LBUTTON, VK_RBUTTON } = require('./recoil');

function rig(initial) {
  const keys = { [VK_LBUTTON]: false, [VK_RBUTTON]: false };
  const moves = [];
  let t = 0;
  const native = { held: vk => keys[vk], move: (dx, dy) => moves.push([dx, dy]) };
  const eng = createEngine(native, { now: () => t });
  eng.configure(initial);
  return { keys, moves, eng, at(ms) { t = ms; eng.tick(); } };
}
const base = { power: true, aim: 'both', rpm: 600, acc: 1, pattern: [[0, -4], [1, -2], [0, -1]] };

{ // nothing happens until both buttons are down
  const r = rig(base);
  r.at(0); r.keys[VK_LBUTTON] = true; r.at(10);
  assert.deepStrictEqual(r.moves, []);
}
{ // hold left+right: one compensating step per shot (100 ms @ 600 rpm), opposite sign to the pattern
  const r = rig(base);
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true;
  r.at(0); r.at(50); r.at(100); r.at(150); r.at(200); r.at(300); r.at(400);
  assert.deepStrictEqual(r.moves, [[0, 4], [-1, 2], [0, 1]]);   // +dy = pull down
}
{ // releasing then pressing again restarts the pattern
  const r = rig(base);
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true;
  r.at(0); r.keys[VK_RBUTTON] = false; r.at(100); r.keys[VK_RBUTTON] = true; r.at(500);
  assert.deepStrictEqual(r.moves, [[0, 4], [0, 4]]);
}
{ // left-only aim key
  const r = rig({ ...base, aim: 'lmb' });
  r.keys[VK_LBUTTON] = true; r.at(0);
  assert.deepStrictEqual(r.moves, [[0, 4]]);
}
{ // power off
  const r = rig({ ...base, power: false });
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true; r.at(0);
  assert.deepStrictEqual(r.moves, []);
}
{ // accuracy scales strength and sub-pixel remainders are carried, not lost
  const r = rig({ ...base, acc: 0.5, pattern: [[0, -1], [0, -1], [0, -1], [0, -1]] });
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true;
  for (let t = 0; t <= 400; t += 100) r.at(t);
  assert.deepStrictEqual(r.moves, [[0, 1], [0, 1]]);
}
{ // a throttled timer (big gap) does not dump the whole pattern at once
  const r = rig(base);
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true;
  r.at(0); r.at(5000);
  assert.strictEqual(r.moves.length, 2);
}
{ // bad input is ignored instead of crashing
  const r = rig({ power: true, pattern: 'nope', rpm: 'x', acc: 99 });
  r.keys[VK_LBUTTON] = r.keys[VK_RBUTTON] = true; r.at(0);
  assert.deepStrictEqual(r.moves, []);
}
console.log('all recoil engine tests passed');
