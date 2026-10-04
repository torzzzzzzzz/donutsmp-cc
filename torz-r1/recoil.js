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

function createEngine(native, { now = () => performance.now(), tickMs = 1 } = {}) {
  let cfg = { power: false, aim: 'both', rpm: 450, acc: 1, pattern: [] };
  let timer = null, firing = false, shot = 0, nextAt = 0, remX = 0, remY = 0;

  function configure(s = {}) {
    cfg = {
      power: !!s.power,
      aim: s.aim === 'lmb' ? 'lmb' : 'both',
      rpm: num(s.rpm, 450, 30, 3000),
      acc: num(s.acc, 1, 0, 1),
      pattern: Array.isArray(s.pattern)
        ? s.pattern.filter(p => Array.isArray(p) && p.length === 2 && p.every(Number.isFinite))
        : []
    };
  }

  const aimHeld = () =>
    cfg.aim === 'lmb'
      ? native.held(VK_LBUTTON)
      : native.held(VK_LBUTTON) && native.held(VK_RBUTTON);   // hold left + right

  function tick() {
    if (!native || !cfg.power || !cfg.pattern.length || !aimHeld()) { firing = false; return; }
    const t = now(), gap = 60000 / cfg.rpm;
    if (!firing) { firing = true; shot = 0; nextAt = t; remX = remY = 0; }
    if (t - nextAt > gap * 2) nextAt = t;                      // timer was throttled: don't fire a burst
    while (shot < cfg.pattern.length && t >= nextAt) {
      const [dx, dy] = cfg.pattern[shot++];
      remX -= dx * cfg.acc;                                    // pull against the recoil kick
      remY -= dy * cfg.acc;
      nextAt += gap;
    }
    const mx = Math.trunc(remX) || 0, my = Math.trunc(remY) || 0;   // keep sub-pixel remainder
    if (mx || my) { native.move(mx, my); remX -= mx; remY -= my; }
  }

  return {
    available: !!native,
    configure,
    tick,
    start() { if (!timer) timer = setInterval(tick, tickMs); },
    stop() { clearInterval(timer); timer = null; }
  };
}

module.exports = { createNative, createEngine, VK_LBUTTON, VK_RBUTTON };
