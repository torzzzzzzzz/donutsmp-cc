'use strict';
// Real mouse compensation for Torz R1 (Windows only).
// The renderer sends the selected pattern; this plays it back while the aim key is held.

const VK_LBUTTON = 0x01;
const VK_RBUTTON = 0x02;
const MOUSEEVENTF_MOVE = 0x0001;

// Thin wrapper over user32. Returns null when unavailable, so the app still runs as a simulator.
function createNative() {
  if (process.platform !== 'win32') return null;
  try {
    const koffi = require('koffi');
    const user32 = koffi.load('user32.dll');
    const getKey = user32.func('int16_t __stdcall GetAsyncKeyState(int vKey)');
    const mouseEvent = user32.func(
      'void __stdcall mouse_event(uint32_t dwFlags, int32_t dx, int32_t dy, uint32_t dwData, uintptr_t dwExtraInfo)'
    );
    return {
      held: vk => (getKey(vk) & 0x8000) !== 0,          // high bit = physically down right now
      move: (dx, dy) => mouseEvent(MOUSEEVENTF_MOVE, dx, dy, 0, 0)   // relative move; +dy is down
    };
  } catch (_) {
    return null;
  }
}

const num = (v, def, lo, hi) => {
  v = Number(v);
  return Number.isFinite(v) ? Math.min(hi, Math.max(lo, v)) : def;
};
const MAX_STEP = 200;                       // same per-shot limit as pattern.js (defence in depth)

function createEngine(native, { now = () => performance.now(), tickMs = 1 } = {}) {
  let cfg = { power: false, aim: 'both', rpm: 450, acc: 1, sx: 1, sy: 1, smooth: true, pattern: [] };
  let cumX = [0], cumY = [0];               // cum*[i] = total compensation owed after i shots
  let timer = null, firing = false, t0 = 0, lastT = 0, emitX = 0, emitY = 0;

  function configure(s = {}) {
    cfg = {
      power: !!s.power,
      aim: s.aim === 'lmb' ? 'lmb' : 'both',
      rpm: num(s.rpm, 450, 30, 3000),
      acc: num(s.acc, 1, 0, 1),                       // overall accuracy
      sx: num(s.sx, 1, 0, 2),                         // horizontal strength
      sy: num(s.sy, 1, 0, 2),                         // vertical strength
      smooth: s.smooth !== false,                     // spread each shot's pull over its interval
      pattern: Array.isArray(s.pattern)
        ? s.pattern
            .filter(p => Array.isArray(p) && p.length === 2 && p.every(Number.isFinite))
            .slice(0, 1000)
            .map(([dx, dy]) => [num(dx, 0, -MAX_STEP, MAX_STEP), num(dy, 0, -MAX_STEP, MAX_STEP)])
        : []
    };
    cumX = [0]; cumY = [0];
    for (const [dx, dy] of cfg.pattern) {              // pull against each recoil kick
      cumX.push(cumX[cumX.length - 1] - dx * cfg.acc * cfg.sx);
      cumY.push(cumY[cumY.length - 1] - dy * cfg.acc * cfg.sy);
    }
    firing = false;                                    // restart cleanly under the new settings
  }

  const aimHeld = () =>
    cfg.aim === 'lmb'
      ? native.held(VK_LBUTTON)
      : native.held(VK_LBUTTON) && native.held(VK_RBUTTON);   // hold left + right

  function tick() {
    if (!native || !cfg.power || !cfg.pattern.length || !aimHeld()) { firing = false; return; }
    const t = now(), gap = 60000 / cfg.rpm, n = cfg.pattern.length;
    if (!firing) { firing = true; t0 = lastT = t; emitX = emitY = 0; }
    if (t - lastT > gap * 2) t0 += (t - lastT) - gap;  // timer was throttled: slide the timeline, don't leap
    lastT = t;

    // Where the cumulative pull should be by now.
    const shotsF = (t - t0) / gap;                     // fractional shots elapsed
    let k, frac;
    if (cfg.smooth) { k = Math.floor(shotsF); frac = shotsF - k; }          // ramp through the current shot
    else { k = Math.floor(shotsF) + 1; frac = 0; }                           // whole shot lands at its start
    let tx, ty;
    if (k >= n) { tx = cumX[n]; ty = cumY[n]; }
    else { tx = cumX[k] + (cumX[k + 1] - cumX[k]) * frac; ty = cumY[k] + (cumY[k + 1] - cumY[k]) * frac; }

    const mx = Math.trunc(tx - emitX) || 0, my = Math.trunc(ty - emitY) || 0;   // whole pixels only; remainder carries
    if (mx || my) { native.move(mx, my); emitX += mx; emitY += my; }
  }

  return {
    available: !!native,
    configure,
    tick,
    isOn: () => cfg.power,
    start() { if (!timer) timer = setInterval(tick, tickMs); },
    stop() { clearInterval(timer); timer = null; }
  };
}

module.exports = { createNative, createEngine, VK_LBUTTON, VK_RBUTTON };
