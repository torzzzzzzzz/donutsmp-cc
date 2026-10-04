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
  const both = () => { keys[VK_LBUTTON] = keys[VK_RBUTTON] = true; };
  return { keys, moves, eng, both, at(ms) { t = ms; eng.tick(); } };
}
const sum = m => m.reduce(([a, b], [c, d]) => [a + c, b + d], [0, 0]);
const base = { power: true, aim: 'both', rpm: 600, acc: 1, smooth: false, pattern: [[0, -4], [1, -2], [0, -1]] };

// ---- step mode (smooth:false): whole shot lands at the start of its interval ----
{ const r = rig(base); r.at(0); r.keys[VK_LBUTTON] = true; r.at(10);           // only left held -> nothing
  assert.deepStrictEqual(r.moves, []); }
{ const r = rig(base); r.both();                                                // 100 ms/shot @ 600 rpm
  [0, 50, 100, 150, 200, 300, 400].forEach(t => r.at(t));
  assert.deepStrictEqual(r.moves, [[0, 4], [-1, 2], [0, 1]]); }                // +dy = pull down
{ const r = rig(base); r.both(); r.at(0); r.keys[VK_RBUTTON] = false; r.at(100); r.keys[VK_RBUTTON] = true; r.at(500);
  assert.deepStrictEqual(r.moves, [[0, 4], [0, 4]]); }                          // release + press restarts
{ const r = rig({ ...base, aim: 'lmb' }); r.keys[VK_LBUTTON] = true; r.at(0);
  assert.deepStrictEqual(r.moves, [[0, 4]]); }
{ const r = rig({ ...base, power: false }); r.both(); r.at(0);
  assert.deepStrictEqual(r.moves, []); }
{ const r = rig({ ...base, acc: 0.5, pattern: [[0, -1], [0, -1], [0, -1], [0, -1]] }); r.both();
  for (let t = 0; t <= 400; t += 100) r.at(t);
  assert.deepStrictEqual(r.moves, [[0, 1], [0, 1]]); }                          // sub-pixel remainder carried
{ const r = rig(base); r.both(); r.at(0); r.at(5000);                           // throttled timer: no burst
  assert.strictEqual(r.moves.length, 2); }

// ---- smooth mode: each shot's pull is spread across its interval ----
{ const r = rig({ ...base, smooth: true, smoothWindow: 1, pattern: [[0, -10]] }); r.both();
  r.at(0); assert.deepStrictEqual(r.moves, []);                                 // nothing yet at t0
  r.at(50); assert.deepStrictEqual(r.moves, [[0, 5]]);                          // halfway through the shot
  r.at(100); assert.deepStrictEqual(r.moves, [[0, 5], [0, 5]]);
  r.at(150); assert.strictEqual(r.moves.length, 2); }                           // pattern over
{ const pat = [[0, -6], [2, -4], [-1, -5], [0, -3]];                            // 1 kHz ticks, whole pattern
  const r = rig({ ...base, smooth: true, smoothWindow: 1, rpm: 600, pattern: pat }); r.both();
  for (let t = 0; t <= 500; t++) r.at(t);
  const [sx, sy] = sum(r.moves);
  assert.ok(Math.abs(sx - -1) < 1 && Math.abs(sy - 18) < 1, `total ${sx},${sy}`);   // = -(sum of kicks)
  assert.ok(r.moves.length >= 15, 'many 1px moves (' + r.moves.length + '), not 4 big jumps');
  assert.ok(Math.max(...r.moves.map(m => Math.abs(m[1]))) <= 1, 'no single move bigger than 1px at 1 kHz'); }

// ---- default smoothing finishes within half the shot interval (less lag than spreading over all of it) ----
{ const r = rig({ ...base, smooth: true, pattern: [[0, -10]] }); r.both();      // smoothWindow defaults to 0.5
  r.at(0); r.at(25); assert.deepStrictEqual(r.moves, [[0, 5]]);                  // half done a quarter of the way in
  r.at(50); assert.deepStrictEqual(r.moves, [[0, 5], [0, 5]]);                   // complete by the half-way point
  r.at(99); assert.strictEqual(r.moves.length, 2); }

// ---- fire rate: a faster gun gets its pull sooner ----
{ const run = rpm => { const r = rig({ ...base, smooth: false, rpm, pattern: [[0, -4], [0, -4], [0, -4]] }); r.both();
    const at = []; for (let t = 0; t <= 400; t++) { const n = r.moves.length; r.at(t); if (r.moves.length > n) at.push(t); } return at; };
  assert.deepStrictEqual(run(450).slice(0, 3), [0, 134, 267]);                   // 133 ms/shot
  assert.deepStrictEqual(run(900).slice(0, 3), [0, 67, 134]);                    // 67 ms/shot
  assert.deepStrictEqual(run(1e9).slice(0, 1), [0]); }                           // absurd rpm is clamped, not infinite

// ---- timer boost is requested once on start and released on stop ----
{ let on = 0, off = 0;
  const native = { held: () => false, move() {}, boost: () => on++, unboost: () => off++ };
  const eng = createEngine(native, { tickMs: 1000 });
  eng.start(); eng.start(); assert.strictEqual(on, 1);
  eng.stop(); eng.stop(); assert.strictEqual(off, 1); }

// ---- per-axis strength ----
{ const r = rig({ ...base, pattern: [[4, -4]], sx: 0.5, sy: 2 }); r.both(); r.at(0);
  assert.deepStrictEqual(r.moves, [[-2, 8]]); }
{ const r = rig({ ...base, pattern: [[4, -4]], sx: 0, sy: 1 }); r.both(); r.at(0);
  assert.deepStrictEqual(r.moves, [[0, 4]]); }

// ---- bad input never reaches the mouse ----
{ const r = rig({ power: true, pattern: 'nope', rpm: 'x', acc: 99 }); r.both(); r.at(0);
  assert.deepStrictEqual(r.moves, []); }
{ const r = rig({ ...base, pattern: [[1e9, -1e9]] }); r.both(); r.at(0);       // clamped to ±200
  assert.deepStrictEqual(r.moves, [[-200, 200]]); }
{ const r = rig({ ...base, pattern: [[NaN, 1], [0, -2]] }); r.both(); r.at(0);  // NaN shot dropped
  assert.deepStrictEqual(r.moves, [[0, 2]]); }

console.log('all recoil engine tests passed');
