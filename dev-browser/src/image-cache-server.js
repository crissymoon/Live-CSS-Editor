/**
 * image-cache-server.js
 *
 * Local HTTP image proxy with Redis caching, sharp compression, and a
 * Piscina worker-thread pool for all CPU-bound image processing.
 *
 * Architecture:
 *   Main thread  -- HTTP server, Redis cache, fetch, request routing
 *   Worker pool  -- libvips decode + resize + WebP encode (via piscina)
 *
 * The main thread never blocks on image work.  Each incoming /img request
 * is served immediately from cache, or dispatched to the pool and awaited
 * while the event loop remains free for the next request.
 *
 * Route:
 *   GET /img?url=<encoded>&w=<px>&h=<px>
 *   GET /ping        -- health check (returns 'pong')
 *   GET /stats       -- JSON pool + cache statistics
 *
 * Pool config (env overrides):
 *   POOL_MIN_THREADS   default: 1
 *   POOL_MAX_THREADS   default: min(cpuCount, 4)
 *   POOL_IDLE_TIMEOUT  default: 10000 ms
 *   POOL_MAX_QUEUE     default: maxThreads * 8
 *
 * TTL rules (seconds):
 *   Social CDNs (pbs.twimg, redd.it, cdninstagram, etc.)  3600   (1h)
 *   YouTube thumbnails (i.ytimg)                           7200   (2h)
 *   Generic CDNs / unknown                               86400   (24h)
 *   Static assets (fonts, logos, icons by suffix)       604800   (7d)
 *
 * Redis:
 *   REDIS_URL env var (default: redis://127.0.0.1:6379).
 *   Falls back to an in-memory LRU Map when Redis is unavailable.
 *
 * Usage:
 *   node src/image-cache-server.js [--port=7779]
 */

'use strict';

const http     = require('http');
const https    = require('https');
const crypto   = require('crypto');
const os       = require('os');
const path     = require('path');
const { URL }  = require('url');
const Piscina  = require('piscina');

// ── Config ──────────────────────────────────────────────────────────────────
const _portArg      = process.argv.find(a => a.startsWith('--port='));
const PORT          = parseInt(process.env.IMG_CACHE_PORT || (_portArg && _portArg.split('=')[1]) || '7779', 10);
const REDIS_URL     = process.env.REDIS_URL || 'redis://127.0.0.1:6379';
const MEM_MAX       = parseInt(process.env.NODE_MEM_CACHE_MAX || '200', 10);
const WEBP_QUALITY  = parseInt(process.env.IMG_WEBP_QUALITY  || '82',  10);
const MAX_SRC_BYTES = parseInt(process.env.IMG_MAX_SRC_BYTES || String(10 * 1024 * 1024), 10);
// SWR: after SWR_FACTOR of the TTL has elapsed the entry is "stale" -- served
// immediately with X-Cache: STALE while a background revalidation runs.
const SWR_FACTOR    = parseFloat(process.env.IMG_SWR_FACTOR   || '0.75');
// Optional byte cap for the in-memory LRU store (0 = disabled).
const MEM_MAX_BYTES = parseInt(process.env.NODE_MEM_CACHE_BYTES || '0', 10);

// ── Piscina worker pool ──────────────────────────────────────────────────────
// sharp (libvips) is CPU-bound.  Running it inside worker threads completely
// frees the main event loop during decode + resize + encode.
const CPU_COUNT     = os.cpus().length;
const POOL_MIN      = parseInt(process.env.POOL_MIN_THREADS  || '1',   10);
const POOL_MAX      = parseInt(process.env.POOL_MAX_THREADS  || String(Math.min(CPU_COUNT, 4)), 10);
const POOL_IDLE     = parseInt(process.env.POOL_IDLE_TIMEOUT || '10000', 10);
const POOL_Q_MAX    = parseInt(process.env.POOL_MAX_QUEUE    || String(POOL_MAX * 8), 10);

const pool = new Piscina({
    filename:    path.resolve(__dirname, 'image-worker.js'),
    minThreads:  POOL_MIN,
    maxThreads:  POOL_MAX,
    idleTimeout: POOL_IDLE,
    maxQueue:    POOL_Q_MAX,
});

pool.on('error', function (err) {
    console.error('[img-cache] pool error:', err.message);
});

console.log('[img-cache] pool: min=' + POOL_MIN + ' max=' + POOL_MAX
    + ' idleMs=' + POOL_IDLE + ' queueMax=' + POOL_Q_MAX
    + ' cpus=' + CPU_COUNT);

// ── LRU maintenance pool ─────────────────────────────────────────────────────
// Single-thread pool dedicated to LRU eviction-decision sweeps.
// The main thread passes a lightweight metadata snapshot (no buffers) so the
// worker can sort + score entries and return a list of keys to evict.
// This keeps all LRU bookkeeping CPU off the main event loop.
const lruPool = new Piscina({
    filename:    path.resolve(__dirname, 'lru-worker.js'),
    minThreads:  1,
    maxThreads:  1,
    idleTimeout: 30000,  // LRU thread may sit idle between sweeps
    maxQueue:    2,      // discard extra sweep requests if one is already queued
});

lruPool.on('error', function (err) {
    console.warn('[img-cache] lruPool error:', err.message);
});

// ── TTL determination ────────────────────────────────────────────────────────
const TTL_STATIC  = 604800;  // 7 days
const TTL_GENERIC = 86400;   // 24 hours
const TTL_CDN     = 7200;    // 2 hours
const TTL_SOCIAL  = 3600;    // 1 hour

const SOCIAL_HOSTS = [
    'pbs.twimg.com', 'video.twimg.com',
    'preview.redd.it', 'i.redd.it', 'external-preview.redd.it',
    'cdninstagram.com', 'fbcdn.net',
    'media.licdn.com', 'media-exp',
    'yt3.ggpht.com',
];
const STATIC_EXTS = ['.woff', '.woff2', '.ttf', '.otf', '.eot', '.ico', '.svg'];

function _ttl(urlStr) {
    try {
        const u = new URL(urlStr);
        const host = u.hostname.toLowerCase();
        const ext  = u.pathname.slice(u.pathname.lastIndexOf('.')).toLowerCase();
        if (STATIC_EXTS.includes(ext)) return TTL_STATIC;
        if (host === 'i.ytimg.com') return TTL_CDN;
        if (SOCIAL_HOSTS.some(h => host.includes(h))) return TTL_SOCIAL;
    } catch (_) {}
    return TTL_GENERIC;
}

// ── Cache key ────────────────────────────────────────────────────────────────
function _key(url, w, h) {
    return 'xcm:img:' + crypto
        .createHash('sha256')
        .update(url + '|' + (w || '0') + '|' + (h || '0'))
        .digest('hex')
        .slice(0, 32);
}

// ── LruStore ─── true LRU in-memory cache with SWR metadata ─────────────────
// Each entry: { buf, storedAt, ttlMs, staleAtMs, lastAccess, size }
//   buf        -- processed WebP bytes (Buffer)
//   storedAt   -- Date.now() at write time
//   ttlMs      -- full TTL in milliseconds
//   staleAtMs  -- timestamp after which SWR revalidation should trigger
//   lastAccess -- Date.now() at last cache hit (drives LRU eviction order)
//   size       -- buf.length in bytes
//
// LRU semantics: on every get() the key is moved to the tail of the Map so
// the oldest-access entry is always at the head (Map preserves insertion order).
class LruStore {
    constructor(max) {
        this.max = max;
        this._m  = new Map();
    }

    get(k) {
        const entry = this._m.get(k);
        if (!entry) return null;
        // Promote to tail (most recently used).
        this._m.delete(k);
        entry.lastAccess = Date.now();
        this._m.set(k, entry);
        return entry;
    }

    set(k, buf, ttlMs) {
        if (this._m.has(k)) {
            this._m.delete(k);  // re-insert at tail with fresh data
        } else if (this._m.size >= this.max) {
            // Synchronous fast-path: evict the LRU entry (head of Map).
            this._m.delete(this._m.keys().next().value);
        }
        const now = Date.now();
        this._m.set(k, {
            buf,
            storedAt:   now,
            ttlMs,
            staleAtMs:  now + Math.floor(ttlMs * SWR_FACTOR),
            lastAccess: now,
            size:       buf.length,
        });
    }

    delete(k) { this._m.delete(k); }

    get size() { return this._m.size; }

    // Keys iterator (for external use).
    keys() { return this._m.keys(); }

    // Lightweight metadata snapshot for the LRU worker (no buffers).
    snapshot() {
        const out = [];
        for (const [key, e] of this._m) {
            out.push({ key, lastAccess: e.lastAccess, storedAt: e.storedAt, ttlMs: e.ttlMs, size: e.size });
        }
        return out;
    }
}

// ── Redis client (ioredis) with graceful fallback ────────────────────────────
let redis = null;
const _memCache = new LruStore(MEM_MAX);

(function _connectRedis() {
    try {
        const Redis = require('ioredis');
        redis = new Redis(REDIS_URL, {
            lazyConnect:          true,
            connectTimeout:       2000,
            maxRetriesPerRequest: 1,
            retryStrategy:        function (times) {
                // Back off, max 10s, stop retrying after 5 attempts.
                if (times > 5) return null;
                return Math.min(times * 500, 10000);
            },
        });
        redis.on('error', function (e) {
            if (!_redisLogged) { console.warn('[img-cache] Redis unavailable, using memory cache:', e.message); _redisLogged = true; }
            redis = null;
        });
        redis.on('connect', function () {
            _redisLogged = false;
            console.log('[img-cache] Redis connected:', REDIS_URL);
        });
    } catch (e) {
        console.warn('[img-cache] ioredis not loadable, using memory cache:', e.message);
    }
})();
var _redisLogged = false;

// cacheGet returns { buf, staleAtMs } or null.
//   staleAtMs > 0 means the entry is within the SWR revalidation window;
//   staleAtMs === 0 means Redis served it (Redis TTL handles expiry, no SWR).
async function cacheGet(k) {
    if (redis) {
        try {
            const buf = await redis.getBuffer(k);
            if (buf) return { buf, staleAtMs: 0 };
        } catch (_) { redis = null; }
    }
    const entry = _memCache.get(k);
    if (!entry) return null;
    return { buf: entry.buf, staleAtMs: entry.staleAtMs };
}

async function cacheSet(k, buf, ttl) {
    if (redis) {
        try {
            await redis.set(k, buf, 'EX', ttl);
        } catch (_) { redis = null; }
    }
    // Always populate LRU so a Redis failure mid-session has in-memory fallback.
    _memCache.set(k, buf, ttl * 1000);
}

async function cacheTtl(k) {
    if (redis) {
        try {
            const t = await redis.ttl(k);
            return t > 0 ? t : null;
        } catch (_) { redis = null; }
    }
    const entry = _memCache.get(k);
    if (!entry) return null;
    const remaining = Math.floor((entry.storedAt + entry.ttlMs - Date.now()) / 1000);
    return remaining > 0 ? remaining : null;
}

// ── Fetch helper ─────────────────────────────────────────────────────────────
function _fetch(urlStr) {
    return new Promise(function (resolve, reject) {
        let u;
        try { u = new URL(urlStr); } catch (e) { return reject(new Error('bad url')); }
        const mod = u.protocol === 'https:' ? https : http;
        const opts = {
            hostname: u.hostname,
            port:     u.port || (u.protocol === 'https:' ? 443 : 80),
            path:     u.pathname + u.search,
            headers:  {
                'User-Agent': 'Mozilla/5.0 (compatible; XCMImageCache/1.0)',
                'Accept':     'image/webp,image/avif,image/*,*/*',
            },
            timeout: 10000,
        };
        const req = mod.get(opts, function (res) {
            if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                return resolve(_fetch(res.headers.location));
            }
            if (res.statusCode !== 200) return reject(new Error('status ' + res.statusCode));
            const chunks = [];
            let total = 0;
            res.on('data', function (c) {
                total += c.length;
                if (total > MAX_SRC_BYTES) { req.destroy(); return reject(new Error('too large')); }
                chunks.push(c);
            });
            res.on('end', function () { resolve(Buffer.concat(chunks)); });
            res.on('error', reject);
        });
        req.on('error', reject);
        req.on('timeout', function () { req.destroy(); reject(new Error('timeout')); });
    });
}

// ── Dispatch to pool ─────────────────────────────────────────────────────────
// Transfers the raw buffer to the worker thread (zero-copy ArrayBuffer
// transfer).  The worker returns an ArrayBuffer which we wrap into a Buffer.
async function _process(rawBuf, w, h) {
    // Transfer rawBuf.buffer to the worker thread.  After transfer the
    // original Buffer's backing store is detached -- we must not read it again.
    const inputAB = rawBuf.buffer.slice(
        rawBuf.byteOffset,
        rawBuf.byteOffset + rawBuf.byteLength
    );

    const resultAB = await pool.run(
        { buffer: inputAB, w: w || null, h: h || null, quality: WEBP_QUALITY },
        { transferList: [inputAB] }
    );

    return Buffer.from(resultAB);
}

// ── SWR revalidation ─────────────────────────────────────────────────────────
// _swrInFlight prevents duplicate background fetches for the same key when
// multiple requests arrive during the stale window simultaneously.
const _swrInFlight = new Set();

async function _swrRevalidate(srcUrl, k, w, h, ttl) {
    if (_swrInFlight.has(k)) return;
    _swrInFlight.add(k);
    try {
        const raw       = await _fetch(srcUrl);
        const processed = await _process(raw, w, h);
        await cacheSet(k, processed, ttl);
        console.log('[img-cache] SWR refreshed:', k);
    } catch (err) {
        console.warn('[img-cache] SWR failed (' + k + '):', err.message);
    } finally {
        _swrInFlight.delete(k);
    }
}

// ── Periodic LRU maintenance sweep ───────────────────────────────────────────
// Every LRU_SWEEP_MS milliseconds the main thread snapshots cache metadata
// (no buffers) and dispatches to the lruPool worker.  The worker identifies
// expired + excess entries and returns a list of keys to evict.
const LRU_SWEEP_MS = parseInt(process.env.LRU_SWEEP_MS || '300000', 10);  // 5 min
let _lruSweepTimer = null;

async function _lruSweep() {
    if (_memCache.size === 0) return;
    try {
        const result = await lruPool.run({
            entries:    _memCache.snapshot(),
            maxEntries: MEM_MAX,
            maxBytes:   MEM_MAX_BYTES,
            nowMs:      Date.now(),
        });
        let evicted = 0;
        for (const key of result.evict) { _memCache.delete(key); evicted++; }
        if (evicted > 0) {
            console.log('[img-cache] LRU sweep evicted ' + evicted
                + ' / ' + _memCache.size + ' remain');
        }
    } catch (err) {
        console.warn('[img-cache] LRU sweep error:', err.message);
    }
}

_lruSweepTimer = setInterval(_lruSweep, LRU_SWEEP_MS);
_lruSweepTimer.unref();

// ── Request handler ──────────────────────────────────────────────────────────
async function _handle(req, res) {
    // CORS -- allow any origin since this is a local proxy.
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET');
    res.setHeader('X-Pool-Threads',  String(pool.threads.length));
    res.setHeader('X-Pool-Pending',  String(pool.queueSize));

    if (req.method !== 'GET') { res.writeHead(405); return res.end(); }

    // Parse URL upfront so all route checks can use parsedUrl.pathname.
    let parsedUrl;
    try { parsedUrl = new URL('http://localhost' + req.url); } catch (_) {
        res.writeHead(400); return res.end('bad request');
    }
    const parsedPath = parsedUrl.pathname;

    // Health ping.
    if (parsedPath === '/ping') {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        return res.end('pong');
    }

    // Pool + cache stats.
    if (parsedPath === '/stats') {
        const stats = {
            pool: {
                threads:     pool.threads.length,
                pending:     pool.queueSize,
                completed:   pool.completed,
                utilization: pool.utilization,
            },
            lruPool: {
                threads: lruPool.threads.length,
                pending: lruPool.queueSize,
            },
            cache: {
                memEntries: _memCache.size,
                memMax:     MEM_MAX,
                redis:      redis ? 'connected' : 'fallback',
            },
            swr: {
                inFlight: _swrInFlight.size,
                factor:   SWR_FACTOR,
            },
        };
        res.writeHead(200, { 'Content-Type': 'application/json' });
        return res.end(JSON.stringify(stats, null, 2));
    }

    if (parsedPath !== '/img') { res.writeHead(404); return res.end(); }

    const srcUrl = parsedUrl.searchParams.get('url');
    if (!srcUrl) { res.writeHead(400); return res.end('missing url'); }

    const w = parseInt(parsedUrl.searchParams.get('w') || '0', 10) || null;
    const h = parseInt(parsedUrl.searchParams.get('h') || '0', 10) || null;

    const k   = _key(srcUrl, w, h);
    const ttl = _ttl(srcUrl);

    try {
        // ── Cache hit path ────────────────────────────────────────────────
        const cacheResult = await cacheGet(k);
        if (cacheResult) {
            const { buf: cachedBuf, staleAtMs } = cacheResult;
            const remaining = await cacheTtl(k);
            const age       = remaining !== null ? (ttl - remaining) : 0;
            // staleAtMs > 0 means entry is tracked in LruStore.
            // isStale = the SWR threshold has been crossed but entry still exists.
            const isStale   = staleAtMs > 0 && Date.now() >= staleAtMs;

            // Serve immediately -- never block on a stale entry.
            // The browser gets the image without any blur or placeholder delay.
            res.writeHead(200, {
                'Content-Type':  'image/webp',
                'Cache-Control': 'public, max-age=' + ttl + ', immutable',
                'Age':           String(Math.max(0, age)),
                'X-Cache':       isStale ? 'STALE' : 'HIT',
                'Vary':          'Accept',
            });
            res.end(cachedBuf);

            // Fire-and-forget revalidation after the response is sent.
            if (isStale) {
                _swrRevalidate(srcUrl, k, w, h, ttl);
            }
            return;
        }

        // ── Cache miss: fetch + process + store ───────────────────────────
        const raw       = await _fetch(srcUrl);
        const processed = await _process(raw, w, h);

        await cacheSet(k, processed, ttl);

        res.writeHead(200, {
            'Content-Type':  'image/webp',
            'Cache-Control': 'public, max-age=' + ttl + ', immutable',
            'Age':           '0',
            'X-Cache':       'MISS',
            'Vary':          'Accept',
        });
        res.end(processed);

    } catch (err) {
        // Proxy error: tell the browser to use the original src.
        res.writeHead(502, { 'Content-Type': 'text/plain', 'X-Cache': 'ERROR' });
        res.end(err.message || 'upstream error');
    }
}

// ── Server ───────────────────────────────────────────────────────────────────
const server = http.createServer(function (req, res) {
    _handle(req, res).catch(function (err) {
        if (!res.headersSent) { res.writeHead(500); res.end(); }
        console.error('[img-cache] unhandled:', err.message);
    });
});

server.listen(PORT, '127.0.0.1', function () {
    console.log('[img-cache] listening on http://127.0.0.1:' + PORT);
});

server.on('error', function (e) {
    if (e.code === 'EADDRINUSE') {
        console.warn('[img-cache] port ' + PORT + ' in use -- already running?');
    } else {
        console.error('[img-cache] server error:', e.message);
    }
});

// ── Graceful shutdown ────────────────────────────────────────────────────────
// On SIGTERM/SIGINT: stop accepting new connections, wait for in-flight
// requests to finish, drain the pool, then exit.
async function _shutdown(signal) {
    console.log('[img-cache] ' + signal + ' -- shutting down');
    if (_lruSweepTimer) clearInterval(_lruSweepTimer);
    server.close(async function () {
        try { await pool.destroy(); } catch (_) {}
        try { await lruPool.destroy(); } catch (_) {}
        try { if (redis) redis.disconnect(); } catch (_) {}
        console.log('[img-cache] clean exit');
        process.exit(0);
    });
    // Force exit after 8 seconds if draining stalls.
    setTimeout(function () { process.exit(1); }, 8000).unref();
}

process.on('SIGTERM', function () { _shutdown('SIGTERM'); });
process.on('SIGINT',  function () { _shutdown('SIGINT');  });

module.exports = { server, pool, lruPool, cacheGet, cacheSet };
