// Generate deterministic expected results using the unmodified upstream reference.
import fs from 'node:fs';
import match from './upstream/pixelmatch.mjs';

let state = 0x91e10da5;
function random(n) {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    return Math.floor(state / 0x100000000 * n);
}
const hex = data => Buffer.from(data).toString('hex');
const rows = [];
for (let test = 0; test < 256; test++) {
    const width = 1 + random(9);
    const height = 1 + random(9);
    const size = width * height * 4;
    const a = new Uint8Array(size);
    const b = new Uint8Array(size);
    for (let i = 0; i < size; i++) {
        a[i] = random(256);
        b[i] = random(256);
    }
    if (test % 4 === 0) {
        // Repeated grayscale bands with intermediate shades exercise AA neighbors.
        for (let i = 0; i < size; i += 4) {
            const x = i / 4 % width;
            const shade = x < width / 3 ? 0 : x < width * 2 / 3 ? 128 : 255;
            a.fill(shade, i, i + 3);
            b.fill(Math.min(255, shade + (random(3) - 1) * 64), i, i + 3);
            a[i + 3] = b[i + 3] = 255;
        }
    } else if (test % 4 === 1) {
        b.set(a);
        // Also leave whole images identical to exercise their fast path.
        if (test % 8 === 1) b[random(size)] = random(256);
    } else if (test % 4 === 2) {
        for (let i = 3; i < size; i += 4) a[i] = b[i] = 255;
    }
    const options = {
        threshold: Math.fround([0, 0.01, 0.05, 0.1, 0.5, 1][random(6)]),
        alpha: Math.fround([0, 0.1, 0.5, 1][random(4)]),
        includeAA: Boolean(random(2)),
        diffMask: Boolean(random(2)),
        checkerboard: Boolean(random(2)),
        windowSize: test % 3 ? [0, -3, 1, 2.9, 4, 100, 2147483648][random(7)] : Infinity,
        aaColor: [0, 192, 0],
        diffColor: [255, 0, 255],
        diffColorAlt: test % 2 ? [0, 128, 255] : undefined
    };
    const output = new Uint8Array(size).fill(37);
    const count = match(a, b, output, width, height, options);
    rows.push([width, height, options.threshold, options.alpha, +options.includeAA,
        +options.diffMask, +options.checkerboard, +(options.diffColorAlt !== undefined), Number.isFinite(options.windowSize) ? options.windowSize : -999, count].join(' '));
    rows.push(hex(a), hex(b), hex(output));
}
fs.writeFileSync(new URL('testdata/parity.txt', import.meta.url), rows.join('\n') + '\n');
