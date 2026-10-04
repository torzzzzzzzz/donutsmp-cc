// Pattern import/validation, shared by the renderer (<script src>) and the Node tests.
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.TorzPattern = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  const MAX_SHOTS = 1000;          // longest pattern accepted
  const MAX_STEP = 200;            // largest |dx| or |dy| for one shot (pixels)
  const RPM_MIN = 30, RPM_MAX = 3000, DEFAULT_RPM = 450;
  const RESERVED = new Set(['__proto__', 'constructor', 'prototype']);

  const fail = error => ({ ok: false, error });

  // Numbers or numeric strings only. null/true/[]/'' are NOT silently turned into 0.
  const toNum = v =>
    typeof v === 'number' ? v : typeof v === 'string' && v.trim() !== '' ? Number(v) : NaN;

  // Accepts [dx,dy] or {dx,dy} / {x,y}.
  function shot(p) {
    let a, b;
    if (Array.isArray(p)) {
      if (p.length !== 2) return null;
      [a, b] = p;
    } else if (p && typeof p === 'object') {
      a = p.dx !== undefined ? p.dx : p.x;
      b = p.dy !== undefined ? p.dy : p.y;
    } else return null;
    a = toNum(a); b = toNum(b);
    return Number.isFinite(a) && Number.isFinite(b) ? [a, b] : null;
  }

  // Display/storage name: trimmed, single-spaced, no control characters, max 40 chars.
  function cleanName(n) {
    if (typeof n !== 'string') return '';
    n = n.replace(/[\u0000-\u001f\u007f]/g, ' ').replace(/\s+/g, ' ').trim().slice(0, 40);
    return RESERVED.has(n) ? '' : n;
  }

  // text (JSON string) or already-parsed value -> { ok, rpm, pattern, name, notes } | { ok:false, error }
  function parse(input) {
    let j = input;
    if (typeof input === 'string') {
      try { j = JSON.parse(input.replace(/^﻿/, '')); }          // Notepad adds a BOM
      catch (_) { return fail('That file is not valid JSON.'); }
    }
    let raw, rpmRaw, name = '';
    if (Array.isArray(j)) raw = j;
    else if (j && typeof j === 'object') { raw = j.pattern; rpmRaw = j.rpm; name = cleanName(j.name); }
    else return fail('Expected an array of [dx,dy] or an object with a "pattern" array.');

    if (!Array.isArray(raw)) return fail('Missing "pattern" array.');
    if (raw.length === 0) return fail('The pattern is empty.');
    if (raw.length > MAX_SHOTS) return fail(`The pattern has ${raw.length} shots (max ${MAX_SHOTS}).`);

    const notes = [];
    let limited = 0;
    const pattern = [];
    for (let i = 0; i < raw.length; i++) {
      const s = shot(raw[i]);
      if (!s) return fail(`Shot ${i + 1} is not a pair of numbers [dx,dy].`);
      pattern.push(s.map(v => {
        const c = Math.max(-MAX_STEP, Math.min(MAX_STEP, v));
        if (c !== v) limited++;
        return +c.toFixed(3);
      }));
    }
    if (limited) notes.push(`${limited} value(s) were limited to ±${MAX_STEP}.`);

    let rpm = DEFAULT_RPM;
    if (rpmRaw !== undefined && rpmRaw !== null && rpmRaw !== '') {
      const r = toNum(rpmRaw);
      if (Number.isFinite(r) && r >= RPM_MIN && r <= RPM_MAX) rpm = r;
      else notes.push(`rpm "${String(rpmRaw).slice(0, 12)}" is outside ${RPM_MIN}-${RPM_MAX}; using ${DEFAULT_RPM}.`);
    }
    return { ok: true, rpm, pattern, name, notes };
  }

  // Re-validate everything read back from localStorage; drop entries that no longer pass.
  function loadStore(obj) {
    const out = {};
    if (!obj || typeof obj !== 'object' || Array.isArray(obj)) return out;
    for (const key of Object.keys(obj)) {
      const name = cleanName(key);
      if (!name) continue;
      const r = parse(obj[key]);
      if (r.ok) out[name] = { rpm: r.rpm, pattern: r.pattern };
    }
    return out;
  }

  return { parse, cleanName, loadStore, MAX_SHOTS, MAX_STEP };
});
