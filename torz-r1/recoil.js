'use strict';
// Real mouse compensation for Torz R1 (Windows only).
// The renderer sends the selected pattern; this plays it back while the aim key is held.

const VK_LBUTTON = 0x01;
const VK_RBUTTON = 0x02;
const MOUSEEVENTF_MOVE = 0x0001;

// Windows throttles timers for background processes (a game in front, this window behind), which makes the
// pull sluggish. Ask for a 1 ms system timer and opt this process out of power throttling. Best effort.
function createBoost(koffi) {
  try {
    const winmm = koffi.load('winmm.dll');
    const kernel32 = koffi.load('kernel32.dll');
    const begin = winmm.func('uint32_t __stdcall timeBeginPeriod(uint32_t uPeriod)');
    const end = winmm.func('uint32_t __stdcall timeEndPeriod(uint32_t uPeriod)');
    let noThrottle = null;
    try {
      koffi.struct('PROCESS_POWER_THROTTLING_STATE', { Version: 'uint32_t', ControlMask: 'uint32_t', StateMask: 'uint32_t' });
      const self = kernel32.func('intptr_t __stdcall GetCurrentProcess()');
      const setInfo = kernel32.func(
        'int __stdcall SetProcessInformation(intptr_t hProcess, int infoClass, _In_ PROCESS_POWER_THROTTLING_STATE *info, uint32_t size)'
      );
      // class 4 = ProcessPowerThrottling; control EXECUTION_SPEED(1)|IGNORE_TIMER_RESOLUTION(4), state 0 = not throttled
      noThrottle = () => setInfo(self(), 4, { Version: 1, ControlMask: 5, StateMask: 0 }, 12);
    } catch (_) {}
    return {
      on() {
        try { begin(1); } catch (_) {}
        try { if (noThrottle) noThrottle(); } catch (_) {}
      },
      off() { try { end(1); } catch (_) {} }
    };
  } catch (_) {
    return null;
  }
}

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
    const boost = createBoost(koffi);
    return {
      held: vk => (getKey(vk) & 0x8000) !== 0,          // high bit = physically down right now
      move: (dx, dy) => mouseEvent(MOUSEEVENTF_MOVE, dx, dy, 0, 0),  // relative move; +dy is down
      boost: () => boost && boost.on(),
      unboost: () => boost && boost.off()
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
  let cfg = { power: false, aim: 'both', rpm: 450, acc: 1, sx: 1, sy: 1, smooth: true, smoothWindow: 0.5, pattern: [] };
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
      smoothWindow: num(s.smoothWindow, 0.5, 0.05, 1),  // ...finishing within this fraction of it (1 = whole interval)
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
    if (cfg.smooth) { k = Math.floor(shotsF); frac = Math.min(1, (shotsF - k) / cfg.smoothWindow); }   // ramp through the current shot
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
    start() { if (!timer) { if (native && native.boost) native.boost(); timer = setInterval(tick, tickMs); } },
    stop() { if (timer && native && native.unboost) native.unboost(); clearInterval(timer); timer = null; }
  };
}

module.exports = { createNative, createEngine, VK_LBUTTON, VK_RBUTTON };
