#!/usr/bin/env node
/**
 * server.js -- Dev HTTP server for the PHP WASM project.
 *
 * Serves the php-wasm-project directory tree. my_project is available via a
 * symlink at php-wasm-project/my_project, so all project files are reachable
 * at /my_project/... without any cross-origin restrictions.
 *
 * PHP files are executed via php-cgi so that PHP code is actually run and
 * the correct HTML is returned (not raw PHP source). This fixes wireframe.php
 * and any other PHP page served in dev mode.
 *
 * Native build mode: COEP/COOP/CSP headers are intentionally omitted.
 * The host WKWebView enforces its own security model.
 *
 * All requests and errors are logged to server.log in this directory.
 *
 * Usage:
 *   node server.js [port]        (default port 8080)
 *   make serve                   (calls this via Makefile)
 */

'use strict';

const http   = require('http');
const fs     = require('fs');
const path   = require('path');
const url    = require('url');
const spawn  = require('child_process').spawn;

const PORT    = parseInt(process.argv[2] || process.env.PORT || '8080', 10);
const ROOT    = __dirname;
const LOGFILE = path.join(ROOT, 'server.log');

/* ── Logger ─────────────────────────────────────────────────────────────── */

const logStream = fs.createWriteStream(LOGFILE, { flags: 'a' });

function log(level, msg) {
    const line = `[${new Date().toISOString()}] [${level}] ${msg}\n`;
    process.stdout.write(line);
    logStream.write(line);
}

log('INFO', `Server starting. ROOT=${ROOT}  LOGFILE=${LOGFILE}`);

/* ── MIME map ────────────────────────────────────────────────────────────── */

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

/* ── PHP-CGI execution ───────────────────────────────────────────────────── */

/**
 * Execute a PHP file via php-cgi. Passes a minimal CGI environment so that
 * $_SERVER['SCRIPT_NAME'], $_SERVER['DOCUMENT_ROOT'] etc. are correct.
 * CGI output format: headers, blank line, body.
 * We parse the headers, forward them to the Node response, then send the body.
 */
function servePHP(req, res, filepath, pathname, queryString, hostHeader) {
    const env = Object.assign({}, process.env, {
        REDIRECT_STATUS  : '200',
        SERVER_SOFTWARE  : 'node-dev-server/1.0',
        SERVER_NAME      : 'localhost',
        SERVER_PORT      : String(PORT),
        SERVER_PROTOCOL  : 'HTTP/1.1',
        REQUEST_METHOD   : req.method || 'GET',
        REQUEST_URI      : req.url,
        SCRIPT_FILENAME  : filepath,
        SCRIPT_NAME      : pathname,
        PATH_INFO        : '',
        DOCUMENT_ROOT    : ROOT,
        QUERY_STRING     : queryString || '',
        HTTP_HOST        : hostHeader || `localhost:${PORT}`,
        HTTP_USER_AGENT  : req.headers['user-agent'] || '',
        HTTP_ACCEPT      : req.headers['accept'] || '',
        HTTP_ACCEPT_LANG : req.headers['accept-language'] || '',
        CONTENT_TYPE     : req.headers['content-type'] || '',
        CONTENT_LENGTH   : req.headers['content-length'] || '0',
    });

    log('PHP', `EXEC  ${filepath}`);

    const proc = spawn('php-cgi', [filepath], { env });

    const chunks = [];
    proc.stdout.on('data', (d) => chunks.push(d));

    proc.stderr.on('data', (d) => {
        log('PHP-ERR', d.toString().trim());
    });

    proc.on('close', (code) => {
        if (code !== 0) {
            log('PHP', `Exit code ${code} for ${filepath}`);
        }

        const raw    = Buffer.concat(chunks);
        const rawStr = raw.toString('binary');

        // CGI output: headers terminated by \r\n\r\n or \n\n then body follows
        const sep   = rawStr.indexOf('\r\n\r\n');
        const sep2  = rawStr.indexOf('\n\n');
        const hdrEnd = sep !== -1
            ? sep + 4
            : (sep2 !== -1 ? sep2 + 2 : -1);

        if (hdrEnd === -1) {
            // No header separator; treat entire output as body
            res.setHeader('Access-Control-Allow-Origin', '*');
            res.setHeader('Cache-Control', 'no-store');
            res.setHeader('Content-Type', 'text/html; charset=utf-8');
            res.writeHead(200);
            res.end(raw);
            return;
        }

        const hdrStr = rawStr.slice(0, hdrEnd - (sep !== -1 ? 4 : 2));
        const body   = raw.slice(hdrEnd); // preserve binary (images etc.)

        // Parse CGI response headers
        let statusCode = 200;
        const hdrs = {};
        hdrStr.split('\n').forEach((line) => {
            line = line.replace(/\r$/, '').trim();
            if (!line) return;
            const colon = line.indexOf(':');
            if (colon === -1) return;
            const name  = line.slice(0, colon).trim();
            const value = line.slice(colon + 1).trim();
            const nameLc = name.toLowerCase();
            if (nameLc === 'status') {
                statusCode = parseInt(value, 10) || 200;
            } else {
                hdrs[name] = value;
            }
        });

        // Enforce CORS and cache headers for native build
        hdrs['Access-Control-Allow-Origin'] = '*';
        hdrs['Cache-Control']               = 'no-store';

        // Remove any CSP or COEP headers PHP may have emitted
        delete hdrs['Content-Security-Policy'];
        delete hdrs['Cross-Origin-Embedder-Policy'];
        delete hdrs['Cross-Origin-Opener-Policy'];

        Object.entries(hdrs).forEach(([k, v]) => res.setHeader(k, v));
        res.writeHead(statusCode);
        res.end(body);

        log('REQ', `${req.method} ${req.url} -> PHP ${statusCode} (${body.length}B)`);
    });

    proc.on('error', (err) => {
        log('ERR', `Failed to spawn php-cgi: ${err.message}`);
        res.writeHead(500, { 'Content-Type': 'text/plain' });
        res.end(`php-cgi spawn error: ${err.message}`);
    });

    // Pipe request body for POST requests
    req.pipe(proc.stdin);
}

/* ── Static file serving ─────────────────────────────────────────────────── */

function serveStatic(req, res, filepath) {
    const contentType = mime(filepath);
    res.setHeader('Content-Type', contentType);
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Cache-Control', 'no-store');

    const stream = fs.createReadStream(filepath);
    stream.on('error', (err) => {
        log('ERR', `Static read error ${filepath}: ${err.message}`);
        res.writeHead(500);
        res.end('Internal server error');
    });
    res.writeHead(200);
    stream.pipe(res);

    log('REQ', `${req.method} ${req.url} -> 200 static`);
}

/* ── Request router ──────────────────────────────────────────────────────── */

const server = http.createServer((req, res) => {
    const parsedUrl  = url.parse(req.url);
    let   pathname   = decodeURIComponent(parsedUrl.pathname);
    const queryStr   = parsedUrl.query || '';

    // Client-side JS event logging endpoint
    // Browser posts JSON { level, msg } and it gets written into server.log
    if (pathname === '/dev-log' && req.method === 'POST') {
        let body = '';
        req.on('data', (c) => { body += c; });
        req.on('end', () => {
            try {
                const ev = JSON.parse(body);
                log(ev.level || 'BROWSER', String(ev.msg || '').slice(0, 2000));
            } catch (e) {
                log('BROWSER', body.slice(0, 2000));
            }
            res.setHeader('Access-Control-Allow-Origin', '*');
            res.writeHead(204);
            res.end();
        });
        return;
    }

    // CORS preflight for /dev-log
    if (req.method === 'OPTIONS') {
        res.setHeader('Access-Control-Allow-Origin', '*');
        res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
        res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
        res.writeHead(204);
        res.end();
        return;
    }

    // Default route
    if (pathname === '/') pathname = '/my_project/index.php';

    // Share viewer: /view/{token} -> view.php?token={token}
    const viewMatch = pathname.match(/^\/view\/([a-f0-9]{8})$/i);
    if (viewMatch) {
        const token    = viewMatch[1];
        const viewPhp  = path.join(ROOT, 'my_project/vscode-bridge/api/view.php');
        servePHP(req, res, viewPhp, '/my_project/vscode-bridge/api/view.php', 'token=' + token, req.headers.host);
        return;
    }

    const filepath = path.join(ROOT, pathname);

    // Prevent directory traversal outside ROOT
    if (!filepath.startsWith(ROOT + path.sep) && filepath !== ROOT) {
        log('WARN', `Blocked traversal attempt: ${pathname}`);
        res.writeHead(403);
        res.end('Forbidden');
        return;
    }

    fs.stat(filepath, (err, stat) => {
        if (err || !stat || !stat.isFile()) {
            // Directory: try index.php then index.html
            const tryPhp  = path.join(filepath, 'index.php');
            const tryHtml = path.join(filepath, 'index.html');
            fs.stat(tryPhp, (e2, s2) => {
                if (!e2 && s2.isFile()) {
                    servePHP(req, res, tryPhp, pathname + '/index.php', queryStr, req.headers.host);
                    return;
                }
                fs.stat(tryHtml, (e3, s3) => {
                    if (!e3 && s3.isFile()) {
                        serveStatic(req, res, tryHtml);
                    } else {
                        log('WARN', `404 ${pathname}`);
                        res.writeHead(404, { 'Content-Type': 'text/plain' });
                        res.end(`404 Not found: ${pathname}`);
                    }
                });
            });
            return;
        }

        const ext = path.extname(filepath).toLowerCase();
        if (ext === '.php') {
            servePHP(req, res, filepath, pathname, queryStr, req.headers.host);
        } else {
            serveStatic(req, res, filepath);
        }
    });
});

server.on('error', (err) => {
    log('FATAL', `Server error: ${err.message}`);
});

server.listen(PORT, '127.0.0.1', () => {
    log('INFO', `Dev server running at http://localhost:${PORT}/`);
    log('INFO', `CSP/COEP/COOP: off (native build mode)`);
    log('INFO', `PHP execution: php-cgi (processes .php files)`);
    log('INFO', `ES modules:    allowed`);
    log('INFO', `Log file:      ${LOGFILE}`);
    console.log(`\nDev server running at http://localhost:${PORT}/`);
    console.log(`All requests logged to: ${LOGFILE}`);
});
