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

    // Run /src/index.php from the virtual FS
    const exitCode = PHP.ccall(
        'wasm_exec',        // C function exported from the build
        'number',
        ['string'],
        ['/src/index.php'],
    );

    const output  = PHP.ccall('php_get_output',  'string', [], []);
    const rawHdrs = PHP.ccall('php_get_headers', 'string', [], []);
    const status  = PHP.ccall('php_get_status',  'number', [], []);

    // Reset runtime state for the next call
    PHP.ccall('php_reset', null, [], []);

    return buildResponse(status || 200, rawHdrs, output, exitCode);
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

// ── Type docs ──────────────────────────────────────────────────────────────

/**
 * @typedef {object} PHPResponse
 * @property {number}                status   - HTTP status code
 * @property {Record<string,string>} headers  - parsed response headers
 * @property {string}                body     - raw PHP output
 * @property {number}                exitCode - WASM process exit code
 */
