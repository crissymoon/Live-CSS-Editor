/**
 * php-wasm.js – Browser-side glue layer for the PHP WebAssembly runtime.
 *
 * Responsibilities:
 *   - Wait for the Emscripten module (php.js) to finish loading
 *   - Accept simulated HTTP requests from the UI
 *   - Execute PHP via the WASM module
 *   - Return the response body + headers to the caller
 *
 * Module boundary: this file only handles the JS↔WASM bridge.
 * UI event wiring lives in ui.js.
 */

// ── Module bootstrap ───────────────────────────────────────────────────────

/**
 * Resolves once the Emscripten PHP module is fully initialised and the
 * PHP embed runtime has been booted via php_wasm_init().
 *
 * With -s MODULARIZE=1 and -s EXPORT_NAME="PHP", the <script> tag sets
 * window.PHP to an async factory function (not a ready module object).
 * We call the factory, wait for the promise it returns, then boot PHP.
 * @type {Promise<object>}
 */
const phpReady = initPHPModule();

async function initPHPModule() {
    // php.js (defer) runs before this module script, so window.PHP is the factory.
    if (typeof window.PHP !== 'function') {
        throw new Error(
            'php.js did not load. Run `make docker-build` to produce wasm/php.js, ' +
            'then `make serve` to start the dev server.',
        );
    }

    // Calling the factory returns a Promise that resolves to the module object
    // once the WASM binary finishes loading and compiling.
    const module = await window.PHP();

    // Mount IDBFS at /data so the SQLite database persists across page loads.
    // The directory is created if it does not exist in the virtual FS.
    try {
        module.FS.mkdir('/data');
    } catch (_) {
        // Already exists -- ignore.
    }
    module.FS.mount(module.IDBFS, {}, '/data');

    // Sync IndexedDB -> memory before booting PHP so the DB is available.
    await new Promise((resolve, reject) => {
        module.FS.syncfs(true, (err) => {
            if (err) reject(new Error('IDBFS initial sync failed: ' + err));
            else resolve();
        });
    });

    // Boot the PHP embed runtime (calls php_embed_init internally).
    // Must be called exactly once before any wasm_exec() calls.
    const rc = module.ccall('php_wasm_init', 'number', [], []);
    if (rc !== 0) {
        throw new Error(`php_wasm_init() failed with exit code ${rc}`);
    }

    return module;
}

// ── Core execution API ─────────────────────────────────────────────────────

/**
 * Execute a PHP script string inside the WASM runtime.
 *
 * @param {object} request
 * @param {string} request.uri        - e.g. "/json"
 * @param {string} [request.method]   - "GET" | "POST"  (default "GET")
 * @param {string} [request.body]     - raw POST body
 * @param {object} [request.headers]  - extra request headers
 * @returns {Promise<PHPResponse>}
 */
export async function phpExec(request) {
    const PHP = await phpReady;

    const { uri = '/', method = 'GET', body = '', headers = {} } = request;

    // Inject $_SERVER superglobals the router depends on
    const serverVars = buildServerVars(uri, method, headers);
    setServerVars(PHP, serverVars);

    if (method === 'POST' && body) {
        injectPostBody(PHP, body);
    }

    // Run /app/index.php from the virtual FS
    const exitCode = PHP.ccall(
        'wasm_exec',        // C function exported from the build
        'number',
        ['string'],
        ['/app/index.php'],
    );

    const output  = PHP.ccall('php_get_output',  'string', [], []);
    const rawHdrs = PHP.ccall('php_get_headers', 'string', [], []);
    const status  = PHP.ccall('php_get_status',  'number', [], []);

    // Reset runtime state for the next call
    PHP.ccall('php_reset', null, [], []);

    const response = buildResponse(status || 200, rawHdrs, output, exitCode);

    // Flush IDBFS after any write operation so changes survive a page reload.
    if (method === 'POST') {
        PHP.FS.syncfs(false, () => {});
    }

    // If the PHP response includes X-Export-File, forward the content to the
    // C++ native shell via postMessage so it can write to the real file system.
    if (response.headers['x-export-file']) {
        dispatchExport(response);
    }

    return response;
}

// ── Helpers ────────────────────────────────────────────────────────────────

/**
 * Build the $_SERVER array content to inject.
 * @param {string} uri
 * @param {string} method
 * @param {object} headers
 * @returns {Record<string, string>}
 */
function buildServerVars(uri, method, headers) {
    const parsed = new URL(uri, 'http://localhost');
    return {
        REQUEST_URI:    uri,
        REQUEST_METHOD: method,
        QUERY_STRING:   parsed.search.slice(1),
        PATH_INFO:      parsed.pathname,
        SERVER_NAME:    'localhost',
        SERVER_PORT:    '80',
        HTTP_HOST:      'localhost',
        ...headersToServerKeys(headers),
    };
}

/**
 * Convert a plain headers object to HTTP_* keys for $_SERVER.
 * @param {object} headers
 * @returns {Record<string, string>}
 */
function headersToServerKeys(headers) {
    return Object.fromEntries(
        Object.entries(headers).map(([k, v]) => [
            'HTTP_' + k.toUpperCase().replace(/-/g, '_'),
            v,
        ]),
    );
}

/**
 * Write the server vars into the WASM module via the exported C helper.
 * @param {object} PHP - Emscripten module
 * @param {Record<string, string>} vars
 */
function setServerVars(PHP, vars) {
    for (const [key, value] of Object.entries(vars)) {
        PHP.ccall('php_set_server_var', null, ['string', 'string'], [key, value]);
    }
}

/**
 * Inject a raw POST body into the WASM virtual stdin.
 * @param {object} PHP
 * @param {string} body
 */
function injectPostBody(PHP, body) {
    PHP.ccall('php_set_request_body', null, ['string', 'number'], [body, body.length]);
}

/**
 * Parse the raw header string and build a PHPResponse object.
 * @param {number} status
 * @param {string} rawHeaders
 * @param {string} output
 * @param {number} exitCode
 * @returns {PHPResponse}
 */
function buildResponse(status, rawHeaders, output, exitCode) {
    const headers = parseHeaders(rawHeaders);
    return { status, headers, body: output, exitCode };
}

/**
 * Parse a "\r\n"-delimited header string into a plain object.
 * @param {string} raw
 * @returns {Record<string, string>}
 */
function parseHeaders(raw) {
    if (!raw) return {};
    return Object.fromEntries(
        raw.split(/\r?\n/)
            .filter(Boolean)
            .map(line => {
                const idx = line.indexOf(':');
                return idx === -1
                    ? [line.trim(), '']
                    : [line.slice(0, idx).trim(), line.slice(idx + 1).trim()];
            }),
    );
}

// ── Export bridge ─────────────────────────────────────────────────────────

/**
 * Forward a PHP export response to the C++ native shell.
 *
 * The native webview bridge (WKScriptMessageHandler on macOS,
 * add_WebMessageReceived on Windows, webkit script message handler on Linux)
 * listens for messages with type "xcm_export" and writes the file to disk.
 *
 * The body of the response must be a JSON object with `filename` and `content`.
 *
 * @param {PHPResponse} response
 */
function dispatchExport(response) {
    let payload;
    try {
        payload = JSON.parse(response.body);
    } catch {
        console.warn('[php-wasm] export response body is not valid JSON');
        return;
    }

    const message = {
        type:     'xcm_export',
        filename: payload.filename || response.headers['x-export-file'],
        content:  payload.content  || '',
    };

    // window.webkit.messageHandlers is available in WKWebView (macOS / iOS).
    if (window.webkit?.messageHandlers?.xcmBridge) {
        window.webkit.messageHandlers.xcmBridge.postMessage(message);
        return;
    }

    // WebView2 (Windows) exposes window.chrome.webview.postMessage.
    if (window.chrome?.webview) {
        window.chrome.webview.postMessage(JSON.stringify(message));
        return;
    }

    // WebKitGTK (Linux) also uses the webkit message handler path.
    // Fallback: emit a CustomEvent that integrating code can intercept.
    window.dispatchEvent(new CustomEvent('xcm:export', { detail: message }));
}

// ── Type docs ──────────────────────────────────────────────────────────────

/**
 * @typedef {object} PHPResponse
 * @property {number}                status   - HTTP status code
 * @property {Record<string,string>} headers  - parsed response headers
 * @property {string}                body     - raw PHP output
 * @property {number}                exitCode - WASM process exit code
 */
