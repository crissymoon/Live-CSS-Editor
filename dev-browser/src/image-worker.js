/**
 * image-worker.js
 *
 * Piscina worker task -- runs inside a Node.js worker_thread.
 * Each call receives a task object, runs sharp on the raw image buffer,
 * and returns the WebP output buffer.
 *
 * All CPU-bound libvips work (decode + resize + encode) happens here,
 * off the main event loop, so the HTTP server stays fully responsive
 * to incoming requests while images are processing.
 *
 * Task shape:
 *   {
 *     buffer:  ArrayBuffer   -- raw image bytes (transferred, zero-copy)
 *     w:       number|null   -- target width  in physical pixels
 *     h:       number|null   -- target height in physical pixels
 *     quality: number        -- WebP quality 1-100 (default 82)
 *   }
 *
 * Return value:
 *   ArrayBuffer -- WebP-encoded bytes (transferred back, zero-copy)
 *
 * Error:
 *   Any thrown Error propagates via the Piscina promise rejection.
 */

'use strict';

const sharp = require('sharp');

module.exports = async function imageWorkerTask(task) {
    const { buffer, w, h, quality } = task;

    // Reconstruct a Node Buffer from the transferred ArrayBuffer.
    // The main thread transferred ownership, so the ArrayBuffer here
    // is a detached copy -- Buffer.from() wraps it without an extra alloc.
    const rawBuf = Buffer.from(buffer);

    let pipe = sharp(rawBuf)
        .rotate()                      // honour EXIF orientation
        .withMetadata({ exif: {} });   // strip all metadata (EXIF, IPTC, XMP)

    if (w && h) {
        pipe = pipe.resize(w, h, { fit: 'inside', withoutEnlargement: true });
    } else if (w) {
        pipe = pipe.resize(w, null, { fit: 'inside', withoutEnlargement: true });
    } else if (h) {
        pipe = pipe.resize(null, h, { fit: 'inside', withoutEnlargement: true });
    }

    const outBuf = await pipe
        .webp({ quality: quality || 82, effort: 4 })
        .toBuffer();

    // Transfer the ArrayBuffer back to the main thread (zero-copy).
    // Piscina detects the transferList and moves the backing memory
    // instead of copying it.
    return outBuf.buffer.slice(
        outBuf.byteOffset,
        outBuf.byteOffset + outBuf.byteLength
    );
};
