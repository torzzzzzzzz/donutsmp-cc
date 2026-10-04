'use strict';
const assert = require('assert');
const P = require('./pattern');

const ok = (x) => { const r = P.parse(x); assert.ok(r.ok, JSON.stringify(r)); return r; };
const bad = (x, re) => { const r = P.parse(x); assert.ok(!r.ok, 'should fail'); if (re) assert.match(r.error, re); };

// accepted shapes
assert.deepStrictEqual(ok([[0, -4], [1, -2]]).pattern, [[0, -4], [1, -2]]);
assert.strictEqual(ok([[0, -4]]).rpm, 450);
assert.deepStrictEqual(ok({ rpm: 600, pattern: [[1, 2]] }).rpm, 600);
assert.deepStrictEqual(ok([{ x: 1, y: -2 }, { dx: '3', dy: '-4' }]).pattern, [[1, -2], [3, -4]]);
assert.strictEqual(ok({ name: '  My   AK ', pattern: [[0, 1]] }).name, 'My AK');

// UTF-8 BOM (Notepad "UTF-8" save) used to break JSON.parse
assert.deepStrictEqual(ok('﻿[[0,-1]]').pattern, [[0, -1]]);

// invalid JSON / wrong shape
bad('{nope', /valid JSON/);
bad('42', /Expected/);
bad('null', /Expected/);
bad({ rpm: 450 }, /Missing/);
bad({ pattern: 'x' }, /Missing/);
bad([], /empty/);
bad({ pattern: [] }, /empty/);

// bad shots: null / bool / empty string used to coerce to 0 or slip through
bad([[0, -1], [1]], /Shot 2/);
bad([[0, -1], [1, 2, 3]], /Shot 2/);
bad([[null, 1]], /Shot 1/);
bad([[true, 1]], /Shot 1/);
bad([['', 1]], /Shot 1/);
bad([['abc', 1]], /Shot 1/);
bad([[1, Infinity]], /Shot 1/);
bad([null], /Shot 1/);
bad(['12'], /Shot 1/);

// size limit
bad(new Array(P.MAX_SHOTS + 1).fill([0, 1]), /max/);
assert.strictEqual(ok(new Array(P.MAX_SHOTS).fill([0, 1])).pattern.length, P.MAX_SHOTS);

// huge values are limited (and the user is told), not passed on to the mouse
{
  const r = ok([[9999, -9999], [1, 1]]);
  assert.deepStrictEqual(r.pattern[0], [200, -200]);
  assert.match(r.notes.join(' '), /limited/);
}

// rpm handling
assert.strictEqual(ok({ rpm: '300', pattern: [[0, 1]] }).rpm, 300);
for (const rpm of [0, -5, 1e9, 'fast', NaN]) {
  const r = ok({ rpm, pattern: [[0, 1]] });
  assert.strictEqual(r.rpm, 450);
  assert.match(r.notes.join(' '), /rpm/);
}

// names: reserved keys would silently break plain-object storage
for (const n of ['__proto__', 'constructor', 'prototype', '', '   ', 5, null]) assert.strictEqual(P.cleanName(n), '');
assert.strictEqual(P.cleanName('a\nb\tc'), 'a b c');
assert.strictEqual(P.cleanName('x'.repeat(100)).length, 40);

// localStorage round trip drops corrupt entries
{
  const s = P.loadStore({ good: { rpm: 500, pattern: [[0, -1]] }, broken: { pattern: [[0]] }, __proto__x: 5, 'junk': 'x' });
  assert.deepStrictEqual(Object.keys(s), ['good']);
  assert.deepStrictEqual(P.loadStore([1, 2]), {});
  assert.deepStrictEqual(P.loadStore(null), {});
  assert.strictEqual(Object.getPrototypeOf(P.loadStore(JSON.parse('{"__proto__":{"rpm":1,"pattern":[[1,1]]}}'))), Object.prototype);
}

// every shipped pattern file must import cleanly with the app's own importer
{
  const fs = require('fs'), path = require('path');
  const dir = path.join(__dirname, 'patterns');
  const files = fs.existsSync(dir) ? fs.readdirSync(dir).filter(f => f.endsWith('.json')) : [];
  for (const f of files) {
    const r = P.parse(fs.readFileSync(path.join(dir, f), 'utf8'));
    assert.ok(r.ok, f + ': ' + r.error);
    assert.deepStrictEqual(r.notes, [], f + ' needed fixing: ' + r.notes);
  }
}
console.log('all pattern import tests passed');
