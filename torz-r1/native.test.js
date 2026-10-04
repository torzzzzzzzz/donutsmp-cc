'use strict';
// Checks the Windows-only code path against a fake koffi + fake platform (the real DLLs cannot load on Linux).
const assert = require('assert');
const Module = require('module');

function load({ failMask } = {}) {
  const calls = [];
  const koffi = {
    load: lib => ({
      func: sig => {
        const name = /(\w+)\(/.exec(sig)[1];
        return (...a) => {
          calls.push([name, ...a]);
          if (name === 'SetProcessInformation' && a[2].ControlMask === failMask) throw new Error('rejected');
          return name === 'GetCurrentProcess' ? -1 : 0;
        };
      }
    }),
    struct: () => {}
  };
  const origLoad = Module._load, origPlat = Object.getOwnPropertyDescriptor(process, 'platform');
  Module._load = function (req, ...r) { return req === 'koffi' ? koffi : origLoad.call(this, req, ...r); };
  Object.defineProperty(process, 'platform', { value: 'win32' });
  delete require.cache[require.resolve('./recoil')];
  try { return { native: require('./recoil').createNative(), calls }; }
  finally { Module._load = origLoad; Object.defineProperty(process, 'platform', origPlat); }
}

// boost asks for a 1 ms timer and sends the two throttling opt-outs as SEPARATE calls (1 then 4)
{
  const { native, calls } = load();
  assert.ok(native && native.boost && native.unboost);
  calls.length = 0;
  native.boost();
  const names = calls.map(c => c[0]);
  assert.deepStrictEqual(names, ['timeBeginPeriod', 'GetCurrentProcess', 'SetProcessInformation', 'GetCurrentProcess', 'SetProcessInformation']);
  assert.deepStrictEqual(calls[0], ['timeBeginPeriod', 1]);
  const sets = calls.filter(c => c[0] === 'SetProcessInformation');
  assert.deepStrictEqual(sets.map(c => c[3].ControlMask), [1, 4]);
  assert.ok(sets.every(c => c[1] === -1 && c[2] === 4 && c[3].Version === 1 && c[3].StateMask === 0 && c[4] === 12));
  calls.length = 0; native.unboost();
  assert.deepStrictEqual(calls, [['timeEndPeriod', 1]]);
}

// Windows 10 rejects the Windows-11-only bit: the supported opt-out must still have been sent and nothing may throw
{
  const { native, calls } = load({ failMask: 4 });
  calls.length = 0;
  assert.doesNotThrow(() => native.boost());
  assert.deepStrictEqual(calls.filter(c => c[0] === 'SetProcessInformation').map(c => c[3].ControlMask), [1, 4]);
}
// ...and the reverse
{
  const { native, calls } = load({ failMask: 1 });
  calls.length = 0;
  assert.doesNotThrow(() => native.boost());
  assert.strictEqual(calls.filter(c => c[0] === 'SetProcessInformation').length, 2);
}
console.log('all native (fake koffi) tests passed');
