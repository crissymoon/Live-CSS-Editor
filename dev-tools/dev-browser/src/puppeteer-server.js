/**
 * puppeteer-server.js  --  Chrome CDP bridge for the xcm dev-browser
 *
 * Architecture
 * ------------
 * This server replaces the QWebEngineView / WKWebView rendering backend with
 * a native Chrome window fully controlled via the Chrome DevTools Protocol.
 *
 * Why this removes jank
 * ---------------------
 * QWebEngineView routes input events through Qt's event loop before they
 * reach the Chromium renderer.  WKWebView does the same through the AppKit
 * responder chain.  Both add a round-trip on the main thread between the
 * trackpad hardware interrupt and the GPU compositor.
 *
 * With this backend Chrome runs as a native subprocess.  Trackpad events
 * go directly from the macOS/Linux/Windows input stack to Chrome's own
 * compositor thread.  There is NO intermediate toolkit.
 *
 * What this server does
 * ---------------------
 *   1.  Launches Chrome with compositor-optimised flags.
 *   2.  Opens a persistent WebSocket server on :9922 for the Python bridge.
 *   3.  Injects xcm performance scripts into every page via CDP
 *       Page.addScriptToEvaluateOnNewDocument (input-watcher, cdm-sdk,
 *       ticker, chrome-gl-compositor).
 *   4.  Forwards page events to Python (url, title, load, console, error).
 *   5.  Accepts commands from Python (navigate, back, forward, reload,
 *       evaluate, screenshot, resize, input events, find-in-page).
 *
 * Chrome window management
 * ------------------------
 * Chrome is launched in --app mode (frameless) so the Python shell can
 * position it to fill the content area of the Qt window.  The Python bridge
 * sends a "setBounds" command whenever the Qt window is resized or moved.
 *
 * Protocol
 * --------
 * All messages are newline-delimited JSON over the WebSocket.
 *
 * Python -> Server commands:
 *   { "cmd": "navigate",    "url": "https://..." }
 *   { "cmd": "back" }
 *   { "cmd": "forward" }
 *   { "cmd": "reload"   }
 *   { "cmd": "stop"     }
 *   { "cmd": "evaluate", "js": "...", "id": "optional-correlation-id" }
 *   { "cmd": "screenshot" }         -> emits { event:"screenshot", data: base64 }
 *   { "cmd": "resize", "w": N, "h": N }
 *   { "cmd": "setBounds", "x": N, "y": N, "w": N, "h": N }
 *   { "cmd": "findInPage",   "text": "...", "forward": true }
 *   { "cmd": "stopFind"  }
 *   { "cmd": "zoomSet",  "factor": 1.25 }
 *   { "cmd": "setUserAgent", "ua": "..." }
 *   { "cmd": "clearCache"  }
 *   { "cmd": "getCookies" }           -> emits { event:"cookies", cookies:[...], cookieHeader:"..." }
 *   { "cmd": "waitForCfClearance", "timeout": 30000 }  -> waits until __cf_clearance cookie appears
 *   { "cmd": "showWindow", "x": N, "y": N, "w": N, "h": N }  -- restore hidden window
 *   { "cmd": "hideWindow" }                                    -- re-minimize window
 *   { "cmd": "mouseEvent",  "type": "click|mousedown|mouseup|mousemove",
 *                           "x": N, "y": N, "button": "left|right|middle" }
 *   { "cmd": "wheelEvent",  "x": N, "y": N, "dX": N, "dY": N }
 *   { "cmd": "keyEvent",    "type": "keydown|keyup|char", "key": "...",
 *                           "code": "...", "modifiers": 0 }
 *
 * Server -> Python events:
 *   { "event": "cfChallenge", "url": "...", "status": 403 }   -- Cloudflare challenge page detected
 *   { "event": "cfClearance"  }                               -- CF challenge solved, __cf_clearance present
 *   { "event": "cookies",  "cookies": [...], "cookieHeader": "k=v; ..." }  -- CF+Stripe cookies after load
 *   { "event": "ready"    }          -- emitted once Chrome is fully started
 *   { "event": "url",     "url":   "..." }
 *   { "event": "title",   "title": "..." }
 *   { "event": "load"     }
 *   { "event": "loadStart" }
 *   { "event": "loadProgress", "percent": 0-100 }
 *   { "event": "console", "level": "log|warn|error", "text": "..." }
 *   { "event": "error",   "msg":  "..." }
 *   { "event": "screenshot", "data": "<base64 png>" }
 *   { "event": "evalResult", "id": "...", "result": <any>, "error": null|"..." }
 */

'use strict';

const os        = require('os');
const fs        = require('fs');
const path      = require('path');
const http      = require('http');
const net       = require('net');
const WS        = require('ws');
const puppeteer = require('puppeteer-core');

// ── Real Chrome user-data directory ──────────────────────────────────────────
// Using the real profile means Chrome already has the user's logged-in Google
// session, saved passwords, and cookies -- which eliminates nearly all OAuth
// friction because Google sees a known, trusted device instead of a fresh
// anonymous session.
function defaultUserDataDir() {
    const p = os.platform();
    if (p === 'darwin') {
        return path.join(os.homedir(), 'Library', 'Application Support', 'Google', 'Chrome');
    }
    if (p === 'linux') {
        return path.join(os.homedir(), '.config', 'google-chrome');
    }
    // win32
    return path.join(process.env.LOCALAPPDATA || '', 'Google', 'Chrome', 'User Data');
}

// CDP session kept for window show/hide commands.
let _cdpSession  = null;
let _windowId    = null;

// ── Configuration ─────────────────────────────────────────────────────────────
const WS_PORT         = 9922;
const CDP_PORT        = 9223;   // remote debugging port for Chrome
const SCRIPTS_DIR     = __dirname;
const COMPOSITOR_JS   = path.join(SCRIPTS_DIR, 'chrome-gl-compositor.js');
const INPUT_WATCHER_JS = path.join(SCRIPTS_DIR, 'input-watcher.js');
const AUTH_MASK_JS    = path.join(SCRIPTS_DIR, 'auth-mask.js');

// ── Chrome flags: compositor-optimised for smooth trackpad scrolling ──────────
const CHROME_LAUNCH_FLAGS = [
    // GPU acceleration
    '--enable-gpu-rasterization',
    '--enable-zero-copy',
    '--ignore-gpu-blocklist',
    '--enable-accelerated-video-decode',
    '--num-raster-threads=4',

    // Compositor thread routing: ScrollUnification makes ALL scroll events
    // go through the compositor thread, eliminating the main-thread round-trip.
    '--enable-features=ScrollUnification,UseSkiaRenderer,CanvasOopRasterization,' +
        'VaapiVideoDecodeLinuxGL,LazyImageLoading,SpeculationRules',

    // Disable features that cause visual glitches OR break Google OAuth.
    //
    // BackForwardCache: when the OAuth redirect lands on openai.com/anthropic.com
    // Chrome restores a cache entry instead of navigating; the OAuth state/nonce
    // in that entry no longer matches what Google issued, triggering the
    // "Route Error (400): Unknown error" page.
    //
    // Prerender2 / PrefetchProxy: speculatively fetch pages in the redirect
    // chain using wrong parameters, consuming the one-time OAuth nonce before
    // the real navigation uses it.
    //
    // StoragePartitioningForNetworkState: Chrome 115+ partitions the cookie jar
    // by top-level origin. This puts Google cookies for the OAuth popup into a
    // different partition from the openerTab, so the auth handoff silently fails.
    '--disable-features=TranslateUI,MediaRouter,PaintHolding,ElasticOverscroll,' +
        'BackForwardCache,Prerender2,PrefetchProxy,StoragePartitioningForNetworkState',

    // Smooth scrolling: keeps trackpad momentum as fluid interpolation
    '--enable-smooth-scrolling',

    // Memory and process model
    // NOTE: --process-per-site is intentionally omitted here. It causes OAuth
    // cookie-sharing to break between the originating tab and the Google auth
    // popup because each origin runs in a separate renderer process with an
    // isolated session context, preventing the popup from reading back the
    // session cookies set by the original tab after the redirect.
    '--memory-model=high',
    '--lang=en-US',
    '--disable-background-timer-throttling',
    '--disable-renderer-backgrounding',
    '--disable-backgrounding-occluded-windows',

    // Partial raster causes checkerboard during fast scroll
    '--disable-partial-raster',
    '--disable-checker-imaging',
    '--enable-prefer-compositing-to-lcd-text',

    // V8 caching
    '--v8-cache-options=bypassHeatCheck',

    // Connection
    '--enable-quic',
    '--enable-tcp-fast-open',

    // Rendering
    '--force-device-scale-factor=1',
    '--high-dpi-support=1',

    // Suppress automation-related logging that can leak paths via Error.stack
    // or performance entries.
    '--disable-logging',
    '--log-level=3',
    // Silence the DevTools listening message on stderr.
    '--silent-debugger-extension-api',
];

// ── Chrome binary detection ───────────────────────────────────────────────────
function findChromeBinary() {
    const platform = os.platform();
    const candidates = [];

    if (platform === 'darwin') {
        candidates.push(
            '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
            '/Applications/Chromium.app/Contents/MacOS/Chromium',
            '/Applications/Google Chrome Canary.app/Contents/MacOS/Google Chrome Canary',
        );
    } else if (platform === 'linux') {
        candidates.push(
            '/usr/bin/google-chrome',
            '/usr/bin/google-chrome-stable',
            '/usr/bin/chromium-browser',
            '/usr/bin/chromium',
            '/snap/bin/chromium',
        );
    } else if (platform === 'win32') {
        const local = process.env.LOCALAPPDATA || '';
        const prog  = process.env.PROGRAMFILES || 'C:\\Program Files';
        const prog86 = process.env['PROGRAMFILES(X86)'] || 'C:\\Program Files (x86)';
        candidates.push(
            path.join(local, 'Google\\Chrome\\Application\\chrome.exe'),
            path.join(prog,  'Google\\Chrome\\Application\\chrome.exe'),
            path.join(prog86,'Google\\Chrome\\Application\\chrome.exe'),
        );
    }

    for (const c of candidates) {
        if (fs.existsSync(c)) return c;
    }
    throw new Error(
        `Chrome not found on ${platform}.\n` +
        `Install Google Chrome or set CHROME_BIN environment variable.\n` +
        `Checked: ${candidates.join(', ')}`
    );
}

// ── Script loader ─────────────────────────────────────────────────────────────
function loadScript(filePath) {
    try {
        return fs.readFileSync(filePath, 'utf8');
    } catch (e) {
        console.error(`[puppeteer-server] cannot read ${filePath}: ${e.message}`);
        return '';
    }
}

// ── State ─────────────────────────────────────────────────────────────────────
let _browser   = null;
let _page      = null;
let _wss       = null;
let _clients   = new Set();

// ── Cloudflare / Stripe domain and cookie helpers ─────────────────────────────
//
// Both OpenAI and Anthropic sit behind Cloudflare Bot Management and use
// Stripe Link for billing. Cloudflare issues JS challenges that real Chrome
// solves automatically, but only when:
//   a) navigation waits for networkidle2 (not just domcontentloaded) so the
//      CF JS challenge script runs to completion and sets __cf_clearance.
//   b) Stripe payment iframes finish loading before cookies are harvested.
//
// The __cf_clearance cookie is the Cloudflare challenge clearance token.
// __cf_bm is the Bot Management behavioral token.
// _cfuvid is the Cloudflare Unique Visitor ID that ties sessions together.
// __stripe_mid / __stripe_sid are set inside the Stripe iframe -- they must
// be present in any C++ HTTP client request targeting billing endpoints.

const _CF_DOMAINS = [
    'openai.com', 'chat.openai.com', 'auth.openai.com', 'platform.openai.com',
    'anthropic.com', 'claude.ai', 'console.anthropic.com',
    'stripe.com', 'js.stripe.com', 'pay.stripe.com', 'checkout.stripe.com',
    'link.stripe.com',
];

// Cookie name fragments to extract for the C++ client. Any cookie whose
// name contains one of these strings is included in the cookies event.
const _CF_COOKIE_FRAGMENTS = [
    '__cf_bm', '_cfuvid', '__cf_clearance',
    '__stripe_mid', '__stripe_sid',
    '__Secure-next-auth', 'intercom', 'sess', '__Host-',
];

function _isCfDomain(url) {
    try {
        const h = new URL(url).hostname.replace(/^www\./, '');
        return _CF_DOMAINS.some(d => h === d || h.endsWith('.' + d));
    } catch (_) { return false; }
}

async function _emitCookies(page) {
    try {
        const all = await page.cookies();
        const filtered = all.filter(c =>
            _CF_COOKIE_FRAGMENTS.some(f => c.name.includes(f))
        );
        if (filtered.length === 0) {
            _log('cookies', `no CF/Stripe/auth cookies yet on ${page.url().slice(0, 80)}`);
            return;
        }
        const names  = filtered.map(c => c.name);
        const header = filtered.map(c => `${c.name}=${c.value}`).join('; ');
        _log('cookies', `emitting ${filtered.length} cookies: ${names.join(', ')}`);
        _log('cookies', `header (${header.length} chars): ${header.slice(0, 200)}`);
        emit({ event: 'cookies', cookies: filtered, cookieHeader: header });
    } catch (e) {
        _log('cookies', `error: ${e.message}`);
    }
}

// ── Emit to all connected Python clients ───────────────────────────────
function emit(obj) {
    const msg = JSON.stringify(obj);
    for (const ws of _clients) {
        if (ws.readyState === WS.OPEN) {
            ws.send(msg);
        }
    }
}

// ── Server-side timestamped logger ────────────────────────────────────
function _log(tag, ...args) {
    const t = new Date().toISOString().slice(11, 23); // HH:MM:SS.mmm
    console.log(`[${t}][${tag}]`, ...args);
}

// ── Attach CDP event listeners to a page ─────────────────────────────────────
async function _attachPageListeners(page) {
    // Catch popups as early as possible -- page.on('popup') fires on the
    // opener page synchronously when window.open() is called, before the new
    // target starts loading. This is earlier than browser.on('targetcreated')
    // and guarantees auth-mask.js is registered before the Google sign-in URL
    // issues its first request.
    page.on('popup', async (popupPage) => {
        if (!popupPage) return;
        _log('popup', `new popup opened from ${page.url().slice(0, 80)}`);
        try {
            await _setupScriptInjection(popupPage);
            await _attachPageListeners(popupPage);
            await popupPage.setExtraHTTPHeaders({
                'Accept-Language':    'en-US,en;q=0.9',
                'sec-ch-ua':          '"Google Chrome";v="131", "Chromium";v="131", "Not_A Brand";v="24"',
                'sec-ch-ua-mobile':   '?0',
                'sec-ch-ua-platform': '"macOS"',
            }).catch(() => {});
            _log('popup', `setup complete for popup: ${popupPage.url().slice(0, 80)}`);
        } catch (e) {
            _log('popup', `setup error: ${e.message}`);
        }
    });

    page.on('framenavigated', async (frame) => {
        if (frame !== page.mainFrame()) return;
        const url   = frame.url();
        const title = await page.title().catch(() => '');
        _log('nav', `framenavigated  url=${url.slice(0, 100)}  title="${title}"`);
        emit({ event: 'url',   url });
        emit({ event: 'title', title });
    });

    // Unified load handler: emit load event, check CF challenge title,
    // log cookies. Only one handler registered to avoid double-firing.
    page.on('load', async () => {
        const title = await page.title().catch(() => '');
        const url   = page.url();
        _log('load', `title="${title}" url=${url.slice(0, 100)}`);
        emit({ event: 'load'  });
        emit({ event: 'title', title });
        if (title === 'Just a moment...' || title.startsWith('Checking if the site')) {
            _log('CF-CHALLENGE', `Cloudflare interstitial: "${title}" | url=${url.slice(0, 80)}`);
            emit({ event: 'cfChallenge', url, status: 0 });
        }
        await _emitCookies(page);
    });

    page.on('domcontentloaded', () => {
        emit({ event: 'loadProgress', percent: 50 });
    });

    page.on('requestfailed', (req) => {
        const reason = req.failure() ? req.failure().errorText : 'unknown';
        const url    = req.url();
        // Only log failures on CF/auth domains or main-frame resources to
        // avoid flooding logs with third-party tracker failures.
        if (_isCfDomain(url) || req.resourceType() === 'document') {
            _log('req-fail', `${reason} | ${url.slice(0, 120)}`);
        }
        emit({ event: 'error', msg: `Request failed: ${req.url()} (${reason})` });
    });

    page.on('console', (msg) => {
        const level = msg.type();
        if (['log', 'warn', 'error', 'info'].includes(level)) {
            emit({ event: 'console', level, text: msg.text() });
        }
    });

    page.on('pageerror', (err) => {
        _log('page-error', err.message.slice(0, 200));
        emit({ event: 'error', msg: err.message });
    });

    // Cloudflare Bot Management challenge detection.
    // CF returns HTTP 403 or 503 with a JavaScript challenge page when it
    // detects automation. The cf-ray response header is always present on
    // Cloudflare-proxied responses. A 403/503 + cf-ray on the main document
    // URL means we hit a challenge. Real headful Chrome will solve the JS
    // challenge automatically, but only after networkidle2 completes.
    page.on('response', async (response) => {
        const status  = response.status();
        const headers = response.headers();
        const url     = response.url();
        const rtype   = response.request().resourceType();

        // Log every main-document response for CF/auth domains.
        if ((rtype === 'document' || rtype === 'xhr' || rtype === 'fetch') && _isCfDomain(url)) {
            const cfRay    = headers['cf-ray']    || '';
            const cfCache  = headers['cf-cache-status'] || '';
            const cacheCtl = headers['cache-control'] || '';
            const age      = headers['age'] || '';
            const xCache   = headers['x-cache'] || '';
            const location = headers['location'] || '';
            _log('response',
                `[${status}] ${rtype.padEnd(8)} ${url.slice(0, 120)}` +
                (cfRay   ? ` | cf-ray=${cfRay}`         : '') +
                (cfCache ? ` | cf-cache=${cfCache}`     : '') +
                (cacheCtl? ` | cache-control=${cacheCtl}` : '') +
                (age     ? ` | age=${age}s`             : '') +
                (xCache  ? ` | x-cache=${xCache}`       : '') +
                (location? ` | location=${location}`    : '')
            );
            // Warn explicitly if serving from cache -- this is often the root
            // cause of stale CF challenge pages or stale OAuth state.
            if (cfCache === 'HIT' || xCache === 'HIT' || (age && parseInt(age) > 0)) {
                _log('CACHE-HIT', `WARN stale cached response for ${url.slice(0, 100)} age=${age}s cf-cache=${cfCache}`);
            }
        }

        if ((status === 403 || status === 503) && headers['cf-ray']) {
            if (rtype === 'document' || rtype === 'xhr' || rtype === 'fetch') {
                _log('CF-BLOCK', `${status} blocked by CF | cf-ray=${headers['cf-ray']} | url=${url.slice(0, 100)}`);
                emit({ event: 'cfChallenge', url, status });
            }
        }
    });

    // After every full page load: check for CF challenge title and emit cookies.
    // NOTE: this second load handler is intentionally REMOVED -- it was a duplicate.
    // The unified load handler above handles CF challenge detection and cookie emission.

    // Track navigation progress via the CDP Network domain.
    const cdp = await page.createCDPSession();
    await cdp.send('Page.enable').catch(() => {});

    // Fired by Chrome for each lifecycle state transition.
    cdp.on('Page.lifecycleEvent', ({ name }) => {
        if (name === 'init')              emit({ event: 'loadStart' });
        if (name === 'DOMContentLoaded')  emit({ event: 'loadProgress', percent: 70 });
        if (name === 'load')              emit({ event: 'loadProgress', percent: 100 });
    });
}

// ── Inject performance scripts on every new document ─────────────────────────
async function _setupScriptInjection(page) {
    const authMaskSrc     = loadScript(AUTH_MASK_JS);
    const inputWatcherSrc = loadScript(INPUT_WATCHER_JS);
    const glCompositorSrc = loadScript(COMPOSITOR_JS);

    // Auth mask is injected FIRST so navigator.webdriver and $cdc_* globals
    // are already false/removed before any page scripts execute.
    if (authMaskSrc) {
        await page.evaluateOnNewDocument(authMaskSrc).then(() => {
            _log('auth-mask', `injected OK (${authMaskSrc.length} bytes)`);
        }).catch(e =>
            _log('auth-mask', `INJECT FAILED: ${e.message}`)
        );
    } else {
        _log('auth-mask', 'WARN: auth-mask.js is empty or failed to load -- bot detection NOT active');
    }
    // Inject in order: input-watcher first (atom must exist before cdm-sdk reads it).
    if (inputWatcherSrc) {
        await page.evaluateOnNewDocument(inputWatcherSrc).catch(e =>
            _log('input-watcher', `inject error: ${e.message}`)
        );
    }
    if (glCompositorSrc) {
        await page.evaluateOnNewDocument(glCompositorSrc).catch(e =>
            _log('gl-compositor', `inject error: ${e.message}`)
        );
    }
}

// ── Command handler ───────────────────────────────────────────────────────────
async function handleCommand(cmd) {
    if (!_page) return;

    switch (cmd.cmd) {

        case 'navigate': {
            const navUrl = cmd.url;
            // Cloudflare-protected domains need networkidle2 so the CF JS
            // challenge script runs fully and sets __cf_clearance before we
            // consider the navigation done.
            const waitFor = _isCfDomain(navUrl) ? 'networkidle2' : 'domcontentloaded';
            _log('navigate', `-> ${navUrl} | waitUntil=${waitFor} | isCfDomain=${_isCfDomain(navUrl)}`);
            // Clear the browser cache before navigating to CF-protected domains.
            // Stale cached CF challenge responses are a common cause of persistent
            // 400 / "Just a moment..." loops even in fully headful Chrome.
            if (_isCfDomain(navUrl)) {
                const cdpCache = await _page.createCDPSession().catch(() => null);
                if (cdpCache) {
                    await cdpCache.send('Network.clearBrowserCache').catch(e =>
                        _log('cache-clear', `warn: ${e.message}`)
                    );
                    await cdpCache.send('Network.clearBrowserCookies').catch(() => {});
                    _log('cache-clear', `cleared browser cache + cookies before CF navigation`);
                    await cdpCache.detach().catch(() => {});
                }
            }
            const t0 = Date.now();
            await _page.goto(navUrl, {
                waitUntil: waitFor,
                timeout:   45000,
            }).then(() => {
                _log('navigate', `done in ${Date.now() - t0}ms | url=${_page.url().slice(0, 80)}`);
            }).catch(e => {
                _log('navigate', `ERROR after ${Date.now() - t0}ms: ${e.message}`);
                emit({ event: 'error', msg: e.message });
            });
            // Emit cookies after navigation in case CF set __cf_clearance.
            await _emitCookies(_page);
            // Stealth self-check: confirm auth-mask.js overrides are active on CF domains.
            if (_isCfDomain(navUrl)) {
                try {
                    const stealthInfo = await _page.evaluate(() => ({
                        webdriver:        navigator.webdriver,
                        hasFocus:         document.hasFocus(),
                        visibilityState:  document.visibilityState,
                        outerWidth:       window.outerWidth,
                        outerHeight:      window.outerHeight,
                        screenX:          window.screenX,
                        permissionsQuery: Function.prototype.toString.call(navigator.permissions.query),
                        languages:        navigator.languages ? navigator.languages.join(',') : '',
                        platform:         navigator.platform,
                    }));
                    _log('stealth-check', JSON.stringify(stealthInfo));
                    if (stealthInfo.webdriver) {
                        _log('STEALTH-WARN', 'navigator.webdriver is STILL true! auth-mask.js may not be injected.');
                    }
                    if (!stealthInfo.hasFocus) {
                        _log('STEALTH-WARN', 'document.hasFocus() is false -- CF will detect hidden window.');
                    }
                    if (stealthInfo.visibilityState !== 'visible') {
                        _log('STEALTH-WARN', `visibilityState="${stealthInfo.visibilityState}" -- CF detects hidden tab.`);
                    }
                    if (stealthInfo.outerWidth === 0) {
                        _log('STEALTH-WARN', 'outerWidth=0 -- window dimensions leak minimized state.');
                    }
                    if (!stealthInfo.permissionsQuery.includes('[native code]')) {
                        _log('STEALTH-WARN', `permissions.query toString not native: ${stealthInfo.permissionsQuery.slice(0, 80)}`);
                    }
                } catch (e) {
                    _log('stealth-check', `evaluate failed: ${e.message}`);
                }
            }
            break;
        }

        case 'back':
            _log('cmd', `back from ${_page.url().slice(0, 80)}`);
            await _page.goBack({ waitUntil: 'domcontentloaded', timeout: 10000 })
                .catch(e => _log('cmd', `back ERROR: ${e.message}`));
            break;

        case 'forward':
            _log('cmd', `forward from ${_page.url().slice(0, 80)}`);
            await _page.goForward({ waitUntil: 'domcontentloaded', timeout: 10000 })
                .catch(e => _log('cmd', `forward ERROR: ${e.message}`));
            break;

        case 'reload': {
            const reloadUrl  = _page.url();
            const reloadWait = _isCfDomain(reloadUrl) ? 'networkidle2' : 'domcontentloaded';
            _log('reload', `${reloadUrl.slice(0, 80)} waitUntil=${reloadWait}`);
            const t0 = Date.now();
            await _page.reload({ waitUntil: reloadWait, timeout: 20000 })
                .then(() => _log('reload', `done in ${Date.now() - t0}ms`))
                .catch(e => {
                    _log('reload', `ERROR: ${e.message}`);
                    emit({ event: 'error', msg: e.message });
                });
            await _emitCookies(_page);
            break;
        }

        case 'stop':
            _log('cmd', `stop | url=${_page.url().slice(0, 80)}`);
            await _page.evaluate(() => window.stop()).catch(() => {});
            break;

        case 'evaluate': {
            const jsPreview = (cmd.js || '').slice(0, 100).replace(/\n/g, ' ');
            _log('eval', `id=${cmd.id || 'none'}  js=${jsPreview}`);
            let result = null, error = null;
            try {
                result = await _page.evaluate(new Function(`return (${cmd.js})`));
                _log('eval', `id=${cmd.id || 'none'}  result=${JSON.stringify(result).slice(0, 200)}`);
            } catch (e) {
                error = e.message;
                _log('eval', `id=${cmd.id || 'none'}  ERROR: ${e.message.slice(0, 200)}`);
            }
            if (cmd.id) emit({ event: 'evalResult', id: cmd.id, result, error });
            break;
        }

        case 'screenshot': {
            try {
                const data = await _page.screenshot({ encoding: 'base64', type: 'png' });
                emit({ event: 'screenshot', data });
            } catch (e) {
                emit({ event: 'error', msg: 'screenshot: ' + e.message });
            }
            break;
        }

        case 'resize':
            await _page.setViewport({ width: cmd.w, height: cmd.h, deviceScaleFactor: 1 })
                .catch(() => {});
            break;

        case 'setBounds': {
            // Position the Chrome window relative to the Qt shell content area.
            // Uses Chrome's chrome.windows API via the extension or CDP window placement.
            const cdp = await _page.createCDPSession().catch(() => null);
            if (cdp) {
                await cdp.send('Browser.setWindowBounds', {
                    windowId: (await cdp.send('Browser.getWindowForTarget').catch(() => ({windowId: null}))).windowId,
                    bounds: { left: cmd.x, top: cmd.y, width: cmd.w, height: cmd.h, windowState: 'normal' },
                }).catch(() => {});
            }
            break;
        }

        case 'findInPage': {
            const cdp = await _page.createCDPSession().catch(() => null);
            if (cdp) {
                await cdp.send('Page.enable').catch(() => {});
                await cdp.send('Overlay.enable').catch(() => {});
                await _page.evaluate(
                    (text, fwd) => window.find(text, false, !fwd, true),
                    cmd.text,
                    cmd.forward !== false,
                ).catch(() => {});
            }
            break;
        }

        case 'stopFind':
            await _page.evaluate(() => window.getSelection().removeAllRanges()).catch(() => {});
            break;

        case 'zoomSet':
            await _page.evaluate((f) => {
                document.body.style.zoom = f;
            }, cmd.factor || 1.0).catch(() => {});
            break;

        case 'setUserAgent':
            await _page.setUserAgent(cmd.ua).catch(() => {});
            break;

        case 'clearCache': {
            _log('cache', `manual clearCache command received | url=${_page.url().slice(0, 80)}`);
            const cdp = await _page.createCDPSession().catch(() => null);
            if (cdp) {
                await cdp.send('Network.clearBrowserCache').catch(e => _log('cache', `clearCache ERROR: ${e.message}`));
                await cdp.send('Network.clearBrowserCookies').catch(() => {});
                _log('cache', 'cache + cookies cleared');
            } else {
                _log('cache', 'WARN: could not create CDP session for clearCache');
            }
            break;
        }

        case 'mouseEvent': {
            const cdp = await _page.createCDPSession().catch(() => null);
            if (!cdp) break;
            const typeMap = {
                click:     'mousePressed',
                mousedown: 'mousePressed',
                mouseup:   'mouseReleased',
                mousemove: 'mouseMoved',
            };
            const btnMap = { left: 'left', right: 'right', middle: 'middle' };
            await cdp.send('Input.dispatchMouseEvent', {
                type:       typeMap[cmd.type] || 'mouseMoved',
                x:          cmd.x || 0,
                y:          cmd.y || 0,
                button:     btnMap[cmd.button] || 'left',
                clickCount: cmd.clickCount || (cmd.type === 'click' ? 1 : 0),
            }).catch(() => {});
            break;
        }

        case 'wheelEvent': {
            const cdp = await _page.createCDPSession().catch(() => null);
            if (!cdp) break;
            await cdp.send('Input.dispatchMouseEvent', {
                type:       'mouseWheel',
                x:          cmd.x || 0,
                y:          cmd.y || 0,
                deltaX:     cmd.dX || 0,
                deltaY:     cmd.dY || 0,
            }).catch(() => {});
            break;
        }

        case 'keyEvent': {
            const cdp = await _page.createCDPSession().catch(() => null);
            if (!cdp) break;
            const typeMap = {
                keydown: 'keyDown',
                keyup:   'keyUp',
                char:    'char',
            };
            await cdp.send('Input.dispatchKeyEvent', {
                type:                  typeMap[cmd.type] || 'keyDown',
                key:                   cmd.key   || '',
                code:                  cmd.code  || '',
                modifiers:             cmd.modifiers || 0,
                unmodifiedText:        cmd.key || '',
                text:                  cmd.text || '',
                nativeVirtualKeyCode:  cmd.nativeKeyCode || 0,
                windowsVirtualKeyCode: cmd.windowsKeyCode || 0,
            }).catch(() => {});
            break;
        }

        case 'getCookies': {
            // Return all CF, Stripe, and session cookies. The Python side or
            // the C++ client uses these to authenticate requests directly
            // without requiring another browser-driven navigation.
            try {
                const all = await _page.cookies();
                const filtered = all.filter(c =>
                    _CF_COOKIE_FRAGMENTS.some(f => c.name.includes(f))
                );
                const header = filtered.map(c => `${c.name}=${c.value}`).join('; ');
                emit({ event: 'cookies', cookies: filtered, cookieHeader: header });
            } catch (e) {
                emit({ event: 'error', msg: 'getCookies: ' + e.message });
            }
            break;
        }

        case 'waitForCfClearance': {
            // Poll until __cf_clearance appears in the page's cookie jar.
            // This is needed when the CF challenge is a slow JS proof-of-work
            // that takes several seconds even in real Chrome.
            const timeout = cmd.timeout || 30000;
            const interval = 500;
            const deadline = Date.now() + timeout;
            let cleared = false;
            while (Date.now() < deadline) {
                try {
                    const cookies = await _page.cookies();
                    const ok = cookies.some(c => c.name === '__cf_clearance');
                    if (ok) {
                        cleared = true;
                        await _emitCookies(_page);
                        emit({ event: 'cfClearance' });
                        break;
                    }
                } catch (_) {}
                await new Promise(r => setTimeout(r, interval));
            }
            if (!cleared) {
                emit({ event: 'error', msg: 'waitForCfClearance: timed out after ' + timeout + 'ms' });
            }
            break;
        }

        case 'showWindow': {
            // Restore the Chrome window from its hidden/minimized state.
            // Called by Python after OAuth auth is confirmed complete.
            if (_cdpSession && _windowId !== null) {
                await _cdpSession.send('Browser.setWindowBounds', {
                    windowId: _windowId,
                    bounds: {
                        windowState: 'normal',
                        left:   cmd.x || 0,
                        top:    cmd.y || 0,
                        width:  cmd.w || 1280,
                        height: cmd.h || 900,
                    },
                }).catch(() => {});
            }
            break;
        }

        case 'hideWindow': {
            // Minimise Chrome back into the background.
            if (_cdpSession && _windowId !== null) {
                await _cdpSession.send('Browser.setWindowBounds', {
                    windowId: _windowId,
                    bounds: { windowState: 'minimized' },
                }).catch(() => {});
            }
            break;
        }

        default:
            console.warn('[puppeteer-server] unknown command:', cmd.cmd);
    }
}

// ── Launch ────────────────────────────────────────────────────────────────────
async function launch() {
    const chromePath  = process.env.CHROME_BIN || findChromeBinary();
    const userDataDir = process.env.CHROME_USER_DATA_DIR || defaultUserDataDir();
    console.log(`[puppeteer-server] Chrome:      ${chromePath}`);
    console.log(`[puppeteer-server] User data:   ${userDataDir}`);
    _log('launch', `headless=false  userDataDir=${userDataDir}`);
    _log('launch', `flags: ${CHROME_LAUNCH_FLAGS.filter(f => f.startsWith('--disable-features') || f.startsWith('--enable-features')).join(' | ')}`);
    _log('launch', `CDP port: ${CDP_PORT} (bound to 127.0.0.1 only)`);
    _log('launch', `AUTH_MASK_JS exists: ${require('fs').existsSync(AUTH_MASK_JS)} (${AUTH_MASK_JS})`);
    _log('launch', `INPUT_WATCHER_JS exists: ${require('fs').existsSync(INPUT_WATCHER_JS)} (${INPUT_WATCHER_JS})`);

    _browser = await puppeteer.launch({
        executablePath: chromePath,

        // headless: false keeps Chrome in full headful mode so pages see a real
        // GPU-composited viewport and navigator.webdriver can be patched at the
        // JS level.  We suppress visibility via CDP setWindowBounds (minimized)
        // immediately after launch instead of using headless mode -- macOS
        // headless Chrome still exposes automation signals that Google reads.
        headless: false,
        devtools: false,

        // Point at the real Chrome profile so Google, OpenAI, and Anthropic see
        // an established session with known cookies instead of a fresh anonymous
        // browser -- the single most effective way to avoid the 400 OAuth block.
        userDataDir,

        args: [
            ...CHROME_LAUNCH_FLAGS,
            `--remote-debugging-port=${CDP_PORT}`,
            // Bind the CDP port to localhost only so page scripts cannot
            // probe it via fetch('http://<LAN-IP>:9223/json').
            '--remote-debugging-address=127.0.0.1',
            // Open to a blank page.  Python sends a "navigate" command immediately
            // after the bridge emits "ready".  Using about:blank instead of --app
            // mode so normal Chrome window chrome (toolbar, tabs) is present for
            // the OAuth popup flow.
            'about:blank',
            '--no-default-browser-check',
            '--no-first-run',
            '--disable-infobars',
            '--hide-crash-restore-bubble',
            // Remove the Blink automation flag that sets navigator.webdriver=true
            // at the engine level before any evaluateOnNewDocument override runs.
            '--disable-blink-features=AutomationControlled',
            // Start the window off-screen so there is no visible flash before the
            // CDP minimize command fires.  Python sends showWindow when it wants
            // Chrome to become visible (e.g. after OAuth completes).
            '--window-position=-32000,0',
            '--window-size=1280,900',
        ],

        defaultViewport: null,

        // Must exclude --enable-automation or Chrome adds the info bar
        // "Chrome is being controlled by automated software" which Google
        // reads as an automation signal.
        ignoreDefaultArgs: ['--enable-automation'],
    });

    const pages = await _browser.pages();
    _page = pages[0] || await _browser.newPage();

    await _setupScriptInjection(_page);
    await _attachPageListeners(_page);

    // Set real Chrome client-hint and language headers on every request.
    //
    // Cloudflare Bot Management checks sec-ch-ua to verify the browser brand
    // matches the User-Agent string and the TLS JA3 fingerprint. If sec-ch-ua
    // is absent (which it is when Puppeteer does not set it explicitly because
    // it is only auto-sent for same-origin requests), CF treats the client as
    // a non-browser bot and issues a challenge or a silent 400.
    //
    // The version string here must stay in sync with the Chrome binary in use.
    // Chrome 131 ships with Puppeteer-core ^21; update if the Chrome version changes.
    const CH_UA = '"Google Chrome";v="131", "Chromium";v="131", "Not_A Brand";v="24"';
    await _page.setExtraHTTPHeaders({
        'Accept-Language':     'en-US,en;q=0.9',
        'sec-ch-ua':           CH_UA,
        'sec-ch-ua-mobile':    '?0',
        'sec-ch-ua-platform':  '"macOS"',
    }).catch(() => {});

    // Store a persistent CDP session for window management commands.
    // We use this to minimize / restore the Chrome window without requiring
    // a new session per command, which avoids a CDP target-not-found race
    // when the page is navigating.
    _cdpSession = await _page.createCDPSession().catch(() => null);
    if (_cdpSession) {
        const winInfo = await _cdpSession.send('Browser.getWindowForTarget')
            .catch(() => null);
        if (winInfo) {
            _windowId = winInfo.windowId;
            // Minimize immediately -- the window is off-screen from --window-position
            // but minimizing also removes it from Expose / Mission Control so it is
            // fully hidden as a background helper process until Python calls showWindow.
            await _cdpSession.send('Browser.setWindowBounds', {
                windowId: _windowId,
                bounds: { windowState: 'minimized' },
            }).catch(() => {});
            console.log(`[puppeteer-server] window ${_windowId} hidden (minimized)`);
        }
    }

    // Handle popup windows opened by OAuth flows (window.open).
    // When OpenAI / Anthropic trigger Google sign-in they open a popup.
    // Without this handler the popup page never receives auth-mask.js, so
    // Google immediately detects automation and returns a 400 error.
    _browser.on('targetcreated', async (target) => {
        if (target.type() !== 'page') return;
        _log('target', `targetcreated type=page url=${target.url().slice(0, 80)}`);
        try {
            const popup = await target.page();
            if (!popup) return;
            await _setupScriptInjection(popup);
            await _attachPageListeners(popup);
            await popup.setExtraHTTPHeaders({
                'Accept-Language':    'en-US,en;q=0.9',
                'sec-ch-ua':          '"Google Chrome";v="131", "Chromium";v="131", "Not_A Brand";v="24"',
                'sec-ch-ua-mobile':   '?0',
                'sec-ch-ua-platform': '"macOS"',
            }).catch(() => {});
            _log('target', `popup setup complete`);
        } catch (e) {
            _log('target', `popup setup error: ${e.message}`);
        }
    });

    // Graceful shutdown on browser close.
    _browser.on('disconnected', () => {
        _log('browser', 'Chrome disconnected -- shutting down');
        process.exit(0);
    });

    _log('launch', 'Chrome ready');
    return _page;
}

// ── WebSocket server ──────────────────────────────────────────────────────────
function startWsServer() {
    _wss = new WS.Server({ port: WS_PORT });

    _wss.on('connection', (ws) => {
        _clients.add(ws);
        console.log(`[puppeteer-server] Python client connected (${_clients.size} total)`);

        // Send current state to newly connected client.
        if (_page) {
            _page.url()   .then(url   => ws.send(JSON.stringify({ event: 'url',   url   }))).catch(() => {});
            _page.title() .then(title => ws.send(JSON.stringify({ event: 'title', title }))).catch(() => {});
            ws.send(JSON.stringify({ event: 'ready' }));
        }

        ws.on('message', (raw) => {
            let cmd;
            try { cmd = JSON.parse(raw); }
            catch (e) { return; }
            handleCommand(cmd).catch(e =>
                emit({ event: 'error', msg: 'cmd error: ' + e.message })
            );
        });

        ws.on('close', () => {
            _clients.delete(ws);
            console.log(`[puppeteer-server] client disconnected (${_clients.size} remaining)`);
        });

        ws.on('error', (e) => {
            console.error('[puppeteer-server] ws client error:', e.message);
            _clients.delete(ws);
        });
    });

    _wss.on('error', (e) => {
        console.error('[puppeteer-server] WebSocket server error:', e.message);
    });

    console.log(`[puppeteer-server] WebSocket bridge listening on :${WS_PORT}`);
}

// ── Port check ────────────────────────────────────────────────────────────────
function isPortFree(port) {
    return new Promise((resolve) => {
        const srv = net.createServer();
        srv.once('error', () => resolve(false));
        srv.once('listening', () => { srv.close(); resolve(true); });
        srv.listen(port, '127.0.0.1');
    });
}

// ── Main ──────────────────────────────────────────────────────────────────────
(async () => {
    // Check port availability.
    if (!(await isPortFree(WS_PORT))) {
        console.log(`[puppeteer-server] port ${WS_PORT} already in use -- exiting`);
        process.exit(0);
    }

    startWsServer();

    try {
        await launch();
        emit({ event: 'ready' });
    } catch (e) {
        console.error('[puppeteer-server] launch error:', e.message);
        emit({ event: 'error', msg: 'launch: ' + e.message });
        process.exit(1);
    }
})();

// ── Graceful shutdown ─────────────────────────────────────────────────────────
async function shutdown() {
    console.log('[puppeteer-server] shutting down...');
    if (_browser) await _browser.close().catch(() => {});
    if (_wss)    _wss.close();
    process.exit(0);
}

process.on('SIGTERM', shutdown);
process.on('SIGINT',  shutdown);
