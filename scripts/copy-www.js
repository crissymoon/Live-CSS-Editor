/**
 * scripts/copy-www.js
 *
 * Copies the PHP app files from the project root into src-tauri/www/
 * so Tauri can bundle them as resources.
 *
 * Run via: npm run copy-www
 */

const fs   = require('fs');
const path = require('path');

const ROOT    = path.join(__dirname, '..');
const WWW_OUT = path.join(ROOT, 'src-tauri', 'www');

// Files and directories to include
const INCLUDE = [
    'index.php',
    'style.css',
    'style-context.txt',
    'css',
    'js',
    'data',
    'vendor',
    'style-sheets',
];

function rimraf(dir) {
    if (!fs.existsSync(dir)) return;
    for (const entry of fs.readdirSync(dir)) {
        const full = path.join(dir, entry);
        if (fs.statSync(full).isDirectory()) {
            rimraf(full);
            fs.rmdirSync(full);
        } else {
            fs.unlinkSync(full);
        }
    }
}

function copyDir(src, dest) {
    fs.mkdirSync(dest, { recursive: true });
    for (const entry of fs.readdirSync(src)) {
        const srcPath  = path.join(src, entry);
        const destPath = path.join(dest, entry);
        if (fs.statSync(srcPath).isDirectory()) {
            copyDir(srcPath, destPath);
        } else {
            fs.copyFileSync(srcPath, destPath);
        }
    }
}

// Clean output directory
rimraf(WWW_OUT);
fs.mkdirSync(WWW_OUT, { recursive: true });

for (const item of INCLUDE) {
    const srcPath  = path.join(ROOT, item);
    const destPath = path.join(WWW_OUT, item);
    if (!fs.existsSync(srcPath)) {
        console.warn('  [skip] not found:', item);
        continue;
    }
    const stat = fs.statSync(srcPath);
    if (stat.isDirectory()) {
        copyDir(srcPath, destPath);
    } else {
        fs.copyFileSync(srcPath, destPath);
    }
    console.log('  [copy]', item, '->', path.relative(ROOT, destPath));
}

console.log('copy-www done.');
