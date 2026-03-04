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
 *   { "cmd": "mouseEvent",  "type": "click|mousedown|mouseup|mousemove",
 *                           "x": N, "y": N, "button": "left|right|middle" }
 *   { "cmd": "wheelEvent",  "x": N, "y": N, "dX": N, "dY": N }
 *   { "cmd": "keyEvent",    "type": "keydown|keyup|char", "key": "...",
 *                           "code": "...", "modifiers": 0 }
 *
 * Server -> Python events:
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

const os      = require('os');
const fs      = require('fs');
const path    = require('path');
const http    = require('http');
const net     = require('net');
const WS      = require('ws');
const puppeteer = require('puppeteer-core');

// ── Configuration ─────────────────────────────────────────────────────────────
const WS_PORT         = 9922;
const CDP_PORT        = 9223;   // remote debugging port for Chrome
const SCRIPTS_DIR     = __dirname;
const COMPOSITOR_JS   = path.join(SCRIPTS_DIR, 'chrome-gl-compositor.js');
const INPUT_WATCHER_JS = path.join(SCRIPTS_DIR, 'input-watcher.js');

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
        'VaapiVideoDecodeLinuxGL,BackForwardCache,LazyImageLoading,' +
        'SpeculationRules,Prerender2,PrefetchProxy',

    // Disable features that cause visual glitches
    '--disable-features=TranslateUI,MediaRouter,PaintHolding,ElasticOverscroll',

    // Smooth scrolling: keeps trackpad momentum as fluid interpolation
    '--enable-smooth-scrolling',

    // Memory and process model
    '--process-per-site',
    '--memory-model=high',
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

// ── Emit to all connected Python clients ─────────────────────────────────────
function emit(obj) {
    const msg = JSON.stringify(obj);
    for (const ws of _clients) {
        if (ws.readyState === WS.OPEN) {
            ws.send(msg);
        }
    }
}

// ── Attach CDP event listeners to a page ─────────────────────────────────────
async function _attachPageListeners(page) {
    page.on('framenavigated', async (frame) => {
        if (frame !== page.mainFrame()) return;
        const url   = frame.url();
        const title = await page.title().catch(() => '');
        emit({ event: 'url',   url });
        emit({ event: 'title', title });
    });

    page.on('load', async () => {
        const title = await page.title().catch(() => '');
        emit({ event: 'load'  });
        emit({ event: 'title', title });
    });

    page.on('domcontentloaded', () => {
        emit({ event: 'loadProgress', percent: 50 });
    });

    page.on('requestfailed', (req) => {
        const reason = req.failure() ? req.failure().errorText : 'unknown';
        emit({ event: 'error', msg: `Request failed: ${req.url()} (${reason})` });
    });

    page.on('console', (msg) => {
        const level = msg.type();
        if (['log', 'warn', 'error', 'info'].includes(level)) {
            emit({ event: 'console', level, text: msg.text() });
        }
    });

    page.on('pageerror', (err) => {
        emit({ event: 'error', msg: err.message });
    });

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
    const inputWatcherSrc = loadScript(INPUT_WATCHER_JS);
    const glCompositorSrc = loadScript(COMPOSITOR_JS);

    // Inject in order: input-watcher first (atom must exist before cdm-sdk reads it).
    if (inputWatcherSrc) {
        await page.evaluateOnNewDocument(inputWatcherSrc).catch(e =>
            console.error('[puppeteer-server] input-watcher inject error:', e.message)
        );
    }
    if (glCompositorSrc) {
        await page.evaluateOnNewDocument(glCompositorSrc).catch(e =>
            console.error('[puppeteer-server] gl-compositor inject error:', e.message)
        );
    }
}

// ── Command handler ───────────────────────────────────────────────────────────
async function handleCommand(cmd) {
    if (!_page) return;

    switch (cmd.cmd) {

        case 'navigate':
            await _page.goto(cmd.url, {
                waitUntil: 'domcontentloaded',
                timeout:   30000,
            }).catch(e => emit({ event: 'error', msg: e.message }));
            break;

        case 'back':
            await _page.goBack({ waitUntil: 'domcontentloaded', timeout: 10000 })
                .catch(() => {});
            break;

        case 'forward':
            await _page.goForward({ waitUntil: 'domcontentloaded', timeout: 10000 })
                .catch(() => {});
            break;

        case 'reload':
            await _page.reload({ waitUntil: 'domcontentloaded', timeout: 15000 })
                .catch(e => emit({ event: 'error', msg: e.message }));
            break;

        case 'stop':
            await _page.evaluate(() => window.stop()).catch(() => {});
            break;

        case 'evaluate': {
            let result = null, error = null;
            try {
                result = await _page.evaluate(new Function(`return (${cmd.js})`));
            } catch (e) {
                error = e.message;
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
            const cdp = await _page.createCDPSession().catch(() => null);
            if (cdp) {
                await cdp.send('Network.clearBrowserCache').catch(() => {});
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

        default:
            console.warn('[puppeteer-server] unknown command:', cmd.cmd);
    }
}

// ── Launch ────────────────────────────────────────────────────────────────────
async function launch() {
    const chromePath = process.env.CHROME_BIN || findChromeBinary();
    console.log(`[puppeteer-server] Chrome: ${chromePath}`);

    _browser = await puppeteer.launch({
        executablePath: chromePath,
        headless: false,      // visible window -- Chrome handles rendering natively
        devtools: false,
        args: [
            ...CHROME_LAUNCH_FLAGS,
            `--remote-debugging-port=${CDP_PORT}`,
            // Start with a blank page; Python sends "navigate" immediately.
            '--app=about:blank',
            // No default browser check dialog.
            '--no-default-browser-check',
            '--no-first-run',
            '--disable-infobars',
            '--hide-crash-restore-bubble',
        ],
        defaultViewport: null,  // use the window's actual size
        ignoreDefaultArgs: ['--enable-automation'],  // remove the automation banner
    });

    const pages = await _browser.pages();
    _page = pages[0] || await _browser.newPage();

    await _setupScriptInjection(_page);
    await _attachPageListeners(_page);

    // Graceful shutdown on browser close.
    _browser.on('disconnected', () => {
        console.log('[puppeteer-server] Chrome disconnected -- shutting down');
        process.exit(0);
    });

    console.log(`[puppeteer-server] Chrome ready`);
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
