#!/usr/bin/env node
/**
 * server.js -- Dev HTTP server for the PHP WASM project.
 *
 * Serves the project root so both /public/ and /wasm/ are reachable.
 *
 * Required browser features this server enables:
 *   - Cross-Origin-Opener-Policy: same-origin
 *   - Cross-Origin-Embedder-Policy: require-corp
 *     These two headers are required for SharedArrayBuffer (used by some
 *     Emscripten builds). Without them Chrome/Firefox block WASM threads.
 *
 *   - Content-Type: application/wasm for .wasm files
 *     Without this browsers fall back to slow synchronous compilation instead
 *     of the fast streaming path (WebAssembly.instantiateStreaming).
 *
 * Usage:
 *   node server.js [port]        (default port 8080)
 *   make serve                   (calls this via Makefile)
 */

'use strict';

const http = require('http');
const fs   = require('fs');
const path = require('path');
const url  = require('url');

const PORT = parseInt(process.argv[2] || process.env.PORT || '8080', 10);
const ROOT = __dirname;

const MIME = {
    '.html' : 'text/html; charset=utf-8',
    '.js'   : 'application/javascript; charset=utf-8',
    '.mjs'  : 'application/javascript; charset=utf-8',
    '.css'  : 'text/css; charset=utf-8',
    '.wasm' : 'application/wasm',
    '.json' : 'application/json; charset=utf-8',
    '.png'  : 'image/png',
    '.svg'  : 'image/svg+xml',
    '.ico'  : 'image/x-icon',
    '.txt'  : 'text/plain; charset=utf-8',
};

function mime(filepath) {
    return MIME[path.extname(filepath).toLowerCase()] || 'application/octet-stream';
}

const server = http.createServer((req, res) => {
    // Required headers for SharedArrayBuffer + WASM streaming compilation
    res.setHeader('Cross-Origin-Opener-Policy',   'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy',  'require-corp');
    res.setHeader('Cross-Origin-Resource-Policy',  'cross-origin');
    res.setHeader('Cache-Control',                 'no-store');

    const parsedUrl = url.parse(req.url);
    let pathname    = decodeURIComponent(parsedUrl.pathname);

    // Default to /public/index.html at root
    if (pathname === '/') pathname = '/public/index.html';

    const filepath = path.join(ROOT, pathname);

    // Prevent directory traversal outside ROOT
    if (!filepath.startsWith(ROOT + path.sep) && filepath !== ROOT) {
        res.writeHead(403);
        res.end('Forbidden');
        return;
    }

    fs.stat(filepath, (err, stat) => {
        if (err || !stat.isFile()) {
            // If a directory was requested, try index.html inside it
            const indexPath = path.join(filepath, 'index.html');
            fs.stat(indexPath, (err2, stat2) => {
                if (!err2 && stat2.isFile()) {
                    serveFile(res, indexPath);
                } else {
                    res.writeHead(404, { 'Content-Type': 'text/plain' });
                    res.end(`404 Not found: ${pathname}`);
                }
            });
            return;
        }
        serveFile(res, filepath);
    });
});

function serveFile(res, filepath) {
    const contentType = mime(filepath);
    res.setHeader('Content-Type', contentType);
    const stream = fs.createReadStream(filepath);
    stream.on('error', () => {
        res.writeHead(500);
        res.end('Internal server error');
    });
    res.writeHead(200);
    stream.pipe(res);
}

server.listen(PORT, '127.0.0.1', () => {
    console.log(`Dev server running at http://localhost:${PORT}/`);
    console.log(`  COOP/COEP headers: enabled`);
    console.log(`  application/wasm:  enabled`);
    console.log(`  Ctrl-C to stop`);
});
