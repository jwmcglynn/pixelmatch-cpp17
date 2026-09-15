// Usage: node tests/generate_golden.mjs /path/to/pngjs/lib/png.js
// pngjs 7.0.0 is needed only to regenerate PNG fixtures, never to run the C++ tests.
import fs from 'node:fs';
import {pathToFileURL} from 'node:url';
import match from '../third_party/pixelmatch/pixelmatch.mjs';
const {PNG} = await import(pathToFileURL(process.argv[2]));
const cases = [
    ['1a', '1b', '1diff', {threshold: 0.05}],
    ['1a', '1b', '1diffdefaultthreshold', {}],
    ['1a', '1b', '1diffmask', {threshold: 0.05, diffMask: true}],
    ['1a', '1a', '1emptydiffmask', {threshold: 0, diffMask: true}],
    ['2a', '2b', '2diff', {threshold: 0.05, alpha: 0.5, aaColor: [0, 192, 0], diffColor: [255, 0, 255]}],
    ['3a', '3b', '3diff', {threshold: 0.05}],
    ['4a', '4b', '4diff', {threshold: 0.05}],
    ['5a', '5b', '5diff', {threshold: 0.05}],
    ['6a', '6b', '6diff', {threshold: 0.05}],
    ['6a', '6a', '6empty', {threshold: 0}],
    ['6a', '6b', '6diffaa', {threshold: 0.05, includeAA: true}],
    ['7a', '7b', '7diff', {diffColorAlt: [0, 255, 0]}],
    ['8a', '5b', '8diff', {threshold: 0.05}]
];
const path = name => new URL(`testdata/${name}.png`, import.meta.url);
for (const [first, second, name, options] of cases) {
    // The public C++ API has always used float for threshold and alpha.
    options.threshold = Math.fround(options.threshold ?? 0.1);
    options.alpha = Math.fround(options.alpha ?? 0.1);
    const a = PNG.sync.read(fs.readFileSync(path(first)));
    const b = PNG.sync.read(fs.readFileSync(path(second)));
    const output = new PNG({width: a.width, height: a.height});
    const count = match(a.data, b.data, output.data, a.width, a.height, options);
    fs.writeFileSync(path(name), PNG.sync.write(output));
    console.log(`${name}: ${count}`);
}
