/**
 * scripts/gen-icon.js
 *
 * Generates a minimal 512x512 PNG icon at src-tauri/icons/icon.png
 * using only Node.js built-ins (zlib + Buffer).
 *
 * Color: #2d1c6e (brand primary)
 */

const fs   = require('fs');
const path = require('path');
const zlib = require('zlib');

const OUT  = path.join(__dirname, '..', 'src-tauri', 'icons', 'icon.png');
const SIZE = 512;
const R    = 0x2d, G = 0x1c, B = 0x6e, A = 0xFF;  // #2d1c6e, fully opaque

// ---- Build raw image data (RGBA) ----
// Each row starts with a filter byte (0 = None), then SIZE * 4 color bytes.
const rowSize  = 1 + SIZE * 4;
const rawBytes = Buffer.alloc(SIZE * rowSize);
for (let y = 0; y < SIZE; y++) {
    const offset = y * rowSize;
    rawBytes[offset] = 0; // filter type: None
    for (let x = 0; x < SIZE; x++) {
        rawBytes[offset + 1 + x * 4]     = R;
        rawBytes[offset + 1 + x * 4 + 1] = G;
        rawBytes[offset + 1 + x * 4 + 2] = B;
        rawBytes[offset + 1 + x * 4 + 3] = A;
    }
}

// ---- Helpers ----
function crc32(buf) {
    let c = 0xFFFFFFFF;
    const table = crc32.table || (crc32.table = (function () {
        const t = new Uint32Array(256);
        for (let i = 0; i < 256; i++) {
            let v = i;
            for (let j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320 ^ (v >>> 1) : (v >>> 1);
            t[i] = v;
        }
        return t;
    })());
    for (let i = 0; i < buf.length; i++) c = table[(c ^ buf[i]) & 0xFF] ^ (c >>> 8);
    const result = (c ^ 0xFFFFFFFF) >>> 0;
    const out = Buffer.alloc(4);
    out.writeUInt32BE(result);
    return out;
}

function chunk(type, data) {
    const typeBytes = Buffer.from(type, 'ascii');
    const lenBuf    = Buffer.alloc(4);
    lenBuf.writeUInt32BE(data.length);
    const crc = crc32(Buffer.concat([typeBytes, data]));
    return Buffer.concat([lenBuf, typeBytes, data, crc]);
}

// ---- Build PNG ----
const sig  = Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);

const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(SIZE, 0);  // width
ihdr.writeUInt32BE(SIZE, 4);  // height
ihdr[8]  = 8;  // bit depth
ihdr[9]  = 6;  // color type: RGBA
ihdr[10] = 0;  // compression
ihdr[11] = 0;  // filter
ihdr[12] = 0;  // interlace

const compressed = zlib.deflateSync(rawBytes, { level: 1 });
const idat = chunk('IDAT', compressed);
const png  = Buffer.concat([
    sig,
    chunk('IHDR', ihdr),
    idat,
    chunk('IEND', Buffer.alloc(0)),
]);

fs.mkdirSync(path.dirname(OUT), { recursive: true });
fs.writeFileSync(OUT, png);
console.log(`Icon written: ${OUT} (${png.length} bytes, ${SIZE}x${SIZE} #${R.toString(16)}${G.toString(16)}${B.toString(16)})`);
