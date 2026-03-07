/**
 * virt-chrome-bridge.js
 *
 * Launches a native Chrome window (frameless --app mode) and positions it
 * over the imgui browser content area when specific URLs are requested.
 * No navigation chrome, no tab bar, no grab bar.  Chrome is forced to cover
 * the content rect sent by imgui.
 *
 * HTTP API  (port 9928, 127.0.0.1 only)
 * --------------------------------------
 * POST /show   body: { url, x, y, w, h, cookies?: [{name,value,domain,...}] }
 *              Navigate Chrome to url, inject cookies, show at rect.
 * POST /hide   Hide Chrome (move off-screen).
 * GET  /health { ok: true, ready: bool }
 * GET  /loading.html  Animated security loading page for WKWebView.
 *
 * Chrome is started once and kept alive.  The first /show takes ~1-2s;
 * subsequent shows are instant (reuse the existing browser process).
 */

'use strict';

const http       = require('http');
const puppeteer  = require('puppeteer-core');
const path       = require('path');
const os         = require('os');
const { execSync } = require('child_process');

// ── Config ────────────────────────────────────────────────────────────────────

const HTTP_PORT      = 9928;
const OFF_X          = -32000;   // off-screen holding position
const CDP_DEBUG_PORT = 9224;     // separate from puppeteer-server's 9223

// Use a dedicated virt profile so it never conflicts with the user's Chrome.
const VIRT_PROFILE = path.join(
    os.homedir(), 'Library', 'Application Support', 'xcm-virt-profile'
);

// ── Chrome binary detection ───────────────────────────────────────────────────

function findChrome() {
    const candidates = [
        '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
        '/Applications/Chromium.app/Contents/MacOS/Chromium',
        '/Applications/Brave Browser.app/Contents/MacOS/Brave Browser',
    ];
    for (const c of candidates) {
        try { execSync(`test -x "${c}"`); return c; } catch (_) {}
    }
    throw new Error('No Chrome/Chromium found. Install Google Chrome.');
}

// ── Browser state ─────────────────────────────────────────────────────────────

let _browser  = null;
let _page     = null;
let _ready    = false;
let _visible  = false;

async function ensureBrowser() {
    if (_browser && _page) return;
    console.log('[virt-bridge] launching Chrome...');
    _browser = await puppeteer.launch({
        executablePath: findChrome(),
        headless: false,
        userDataDir: VIRT_PROFILE,
        defaultViewport: null,
        args: [
            // Start off-screen so the window is hidden until /show is called.
            `--window-position=${OFF_X},0`,
            '--window-size=1400,820',
            // App mode: no navigation bar, no tab bar, pure content window.
            '--app=about:blank',
            `--remote-debugging-port=${CDP_DEBUG_PORT}`,
            '--remote-debugging-address=127.0.0.1',
            // Performance flags.
            '--enable-gpu-rasterization',
            '--enable-zero-copy',
            '--ignore-gpu-blocklist',
            '--disable-background-timer-throttling',
            '--disable-renderer-backgrounding',
            '--no-first-run',
            '--no-default-browser-check',
            '--disable-infobars',
            // Silence webdriver detection.
            '--disable-blink-features=AutomationControlled',
        ],
    });
    const pages = await _browser.pages();
    _page = pages[0] || await _browser.newPage();
    _ready = true;
    console.log('[virt-bridge] Chrome ready');
}

// ── Cookie injection ──────────────────────────────────────────────────────────

async function injectCookies(cookies) {
    if (!cookies || !cookies.length || !_page) return;
    const cdp = await _page.createCDPSession().catch(() => null);
    if (!cdp) return;
    let n = 0;
    for (const c of cookies) {
        if (!c.name || !c.domain) continue;
        const cookie = {
            name:     c.name,
            value:    c.value || '',
            domain:   c.domain,
            path:     c.path || '/',
            secure:   !!c.secure,
            httpOnly: !!c.httpOnly,
        };
        if (c.expiresAt) cookie.expires = Number(c.expiresAt);
        await cdp.send('Network.setCookie', cookie).catch(() => {});
        n++;
    }
    console.log(`[virt-bridge] injected ${n} cookies`);
    await cdp.detach().catch(() => {});
}

// ── Window positioning ────────────────────────────────────────────────────────

async function setWindowBounds(state, x, y, w, h) {
    const cdp = await _page.createCDPSession().catch(() => null);
    if (!cdp) return;
    try {
        const { windowId } = await cdp.send('Browser.getWindowForTarget')
            .catch(() => ({ windowId: null }));
        if (windowId == null) return;
        await cdp.send('Browser.setWindowBounds', {
            windowId,
            bounds: { left: x, top: y, width: w, height: h, windowState: state },
        }).catch(() => {});
    } finally {
        await cdp.detach().catch(() => {});
    }
}

// ── Show ──────────────────────────────────────────────────────────────────────

async function show(url, x, y, w, h, cookies) {
    await ensureBrowser();
    // 1. Inject cookies before navigation so they are present on first load.
    await injectCookies(cookies);
    // 2. Set viewport to match content area.
    await _page.setViewport({ width: w, height: h, deviceScaleFactor: 1 })
        .catch(() => {});
    // 3. Navigate.
    console.log(`[virt-bridge] navigate -> ${url}`);
    await _page.goto(url, { waitUntil: 'domcontentloaded', timeout: 30000 })
        .catch(e => console.warn('[virt-bridge] goto error:', e.message));
    // 4. Position and show.
    await setWindowBounds('normal', x, y, w, h);
    _visible = true;
    console.log(`[virt-bridge] shown at ${w}x${h}+${x}+${y}`);
}

// ── Hide ──────────────────────────────────────────────────────────────────────

async function hide() {
    if (!_page) return;
    await setWindowBounds('normal', OFF_X, 0, 1400, 820);
    _visible = false;
    console.log('[virt-bridge] hidden (moved off-screen)');
}

// ── Loading page HTML ─────────────────────────────────────────────────────────

const LOADING_HTML = `<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Connecting</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{width:100%;height:100%;background:#0a0f1a;display:flex;
  align-items:center;justify-content:center;font-family:-apple-system,sans-serif}
.card{display:flex;flex-direction:column;align-items:center;gap:22px}
.shield{width:64px;height:64px;position:relative}
.shield svg{width:64px;height:64px}
.shield-ring{
  position:absolute;top:-12px;left:-12px;width:88px;height:88px;
  border-radius:50%;
  border:2px solid rgba(56,189,248,.18);
  border-top-color:rgba(56,189,248,.8);
  animation:spin 1.1s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.label{color:#94a3b8;font-size:13px;letter-spacing:.08em;text-transform:uppercase}
.url{color:#cbd5e1;font-size:12px;max-width:340px;
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;opacity:.7}
.dots span{
  display:inline-block;width:6px;height:6px;border-radius:50%;
  background:#38bdf8;margin:0 3px;
  animation:pulse 1.4s ease-in-out infinite}
.dots span:nth-child(2){animation-delay:.2s}
.dots span:nth-child(3){animation-delay:.4s}
@keyframes pulse{0%,80%,100%{transform:scale(.6);opacity:.4}40%{transform:scale(1);opacity:1}}
</style>
</head>
<body>
<div class="card">
  <div class="shield">
    <div class="shield-ring"></div>
    <svg viewBox="0 0 24 24" fill="none" stroke="#38bdf8" stroke-width="1.5"
         stroke-linecap="round" stroke-linejoin="round">
      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
      <polyline points="9 12 11 14 15 10"/>
    </svg>
  </div>
  <div class="label">Establishing secure connection</div>
  <div class="url" id="u"></div>
  <div class="dots"><span></span><span></span><span></span></div>
</div>
<script>
var u=new URLSearchParams(location.search).get('url');
if(u)document.getElementById('u').textContent=u;
</script>
</body>
</html>`;

// ── HTTP server ───────────────────────────────────────────────────────────────

function readBody(req) {
    return new Promise((resolve) => {
        const chunks = [];
        req.on('data', c => chunks.push(c));
        req.on('end', () => {
            try { resolve(JSON.parse(Buffer.concat(chunks).toString())); }
            catch (_) { resolve({}); }
        });
    });
}

function reply(res, status, body) {
    const data = typeof body === 'string' ? body : JSON.stringify(body);
    const type = typeof body === 'string' ? 'text/html; charset=utf-8' : 'application/json';
    res.writeHead(status, { 'Content-Type': type, 'Content-Length': Buffer.byteLength(data) });
    res.end(data);
}

const srv = http.createServer(async (req, res) => {
    const url = req.url.split('?')[0];

    if (req.method === 'GET' && url === '/health') {
        return reply(res, 200, { ok: true, ready: _ready, visible: _visible });
    }

    if (req.method === 'GET' && url === '/loading.html') {
        return reply(res, 200, LOADING_HTML);
    }

    if (req.method === 'POST' && url === '/show') {
        const data = await readBody(req);
        reply(res, 200, { ok: true });  // respond immediately; show is async
        show(
            data.url || 'about:blank',
            parseInt(data.x) || 0,
            parseInt(data.y) || 0,
            parseInt(data.w) || 1400,
            parseInt(data.h) || 820,
            data.cookies || [],
        ).catch(e => console.error('[virt-bridge] show error:', e.message));
        return;
    }

    if (req.method === 'POST' && url === '/hide') {
        reply(res, 200, { ok: true });
        hide().catch(e => console.error('[virt-bridge] hide error:', e.message));
        return;
    }

    reply(res, 404, { error: 'not found' });
});

srv.listen(HTTP_PORT, '127.0.0.1', () => {
    console.log(`[virt-bridge] HTTP control server on 127.0.0.1:${HTTP_PORT}`);
});

// Warm up Chrome immediately so first /show is fast.
ensureBrowser().catch(e => console.error('[virt-bridge] startup error:', e.message));

process.on('SIGTERM', () => { if (_browser) _browser.close(); process.exit(0); });
process.on('SIGINT',  () => { if (_browser) _browser.close(); process.exit(0); });
