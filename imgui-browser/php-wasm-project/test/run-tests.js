#!/usr/bin/env node
/**
 * test/run-tests.js -- Headless integration test for the PHP WASM build.
 *
 * Tests that PHP actually runs in the WASM module and produces correct output
 * for several scenarios, with no PHP installed on the host.
 *
 * Requirements:
 *   - wasm/php.js and wasm/php.wasm must already be built (make build)
 *   - Node.js 18+ (for WebAssembly.instantiateStreaming + fetch compatibility)
 *
 * Usage:
 *   node test/run-tests.js
 *   make test
 */

'use strict';

const path = require('path');
const fs   = require('fs');

// ── WASM module bootstrap ─────────────────────────────────────────────────────

const WASM_JS   = path.join(__dirname, '..', 'wasm', 'php.js');
const WASM_BIN  = path.join(__dirname, '..', 'wasm', 'php.wasm');

if (!fs.existsSync(WASM_JS)) {
    console.error('ERROR: wasm/php.js not found. Run `make build` first.');
    process.exit(1);
}

// Emscripten MODULARIZE=1 output: require() returns a factory function.
const PHPFactory = require(WASM_JS);

async function loadPHP() {
    // The module was built with ENVIRONMENT=web (uses fetch to load .wasm).
    // In Node.js there is no URL context, so we pre-read the binary and pass it
    // via wasmBinary which bypasses the fetch path entirely.
    const wasmBinary = fs.readFileSync(WASM_BIN);
    const module = await PHPFactory({ wasmBinary });
    const rc = module.ccall('php_wasm_init', 'number', [], []);
    if (rc !== 0) throw new Error(`php_wasm_init failed (code ${rc})`);
    return module;
}

// ── Test runner ───────────────────────────────────────────────────────────────

let passed = 0;
let failed = 0;

function assert(label, actual, expected) {
    if (actual === expected || (typeof expected === 'string' && String(actual).includes(expected))) {
        console.log(`  PASS  ${label}`);
        passed++;
    } else {
        console.error(`  FAIL  ${label}`);
        console.error(`        expected: ${JSON.stringify(expected)}`);
        console.error(`        actual:   ${JSON.stringify(String(actual).slice(0, 200))}`);
        failed++;
    }
}

function runPHP(PHP, serverVars, body) {
    // Inject $_SERVER vars
    const defaults = {
        REQUEST_METHOD: 'GET',
        REQUEST_URI:    '/',
        QUERY_STRING:   '',
        PATH_INFO:      '/',
        SERVER_NAME:    'localhost',
        SERVER_PORT:    '80',
        HTTP_HOST:      'localhost',
        ...serverVars,
    };
    for (const [k, v] of Object.entries(defaults)) {
        PHP.ccall('php_set_server_var', null, ['string', 'string'], [k, String(v)]);
    }
    if (body) {
        PHP.ccall('php_set_request_body', null, ['string', 'number'], [body, body.length]);
    }
    PHP.ccall('wasm_exec', 'number', ['string'], ['/src/index.php']);
    const output  = PHP.ccall('php_get_output',  'string', [], []);
    const headers = PHP.ccall('php_get_headers', 'string', [], []);
    const status  = PHP.ccall('php_get_status',  'number', [], []);
    PHP.ccall('php_reset', null, [], []);
    return { output, headers, status };
}

// ── Tests ─────────────────────────────────────────────────────────────────────

async function main() {
    console.log('Loading PHP WASM module...');
    const PHP = await loadPHP();
    console.log('Module loaded. Running tests...\n');

    // 1. Home route renders PHP_VERSION
    {
        const { output, status } = runPHP(PHP, { REQUEST_URI: '/', REQUEST_METHOD: 'GET' });
        assert('GET /  -- status 200',              status, 200);
        assert('GET /  -- contains PHP WASM Demo',  output, 'PHP WASM Demo');
        assert('GET /  -- contains PHP version',    output, '8.');
        assert('GET /  -- contains WebAssembly',    output, 'WebAssembly');
    }

    // 2. JSON route
    {
        const { output, status, headers } = runPHP(PHP, { REQUEST_URI: '/json', REQUEST_METHOD: 'GET' });
        assert('GET /json  -- status 200',           status, 200);
        assert('GET /json  -- Content-Type header',  headers, 'application/json');
        let parsed;
        try { parsed = JSON.parse(output); } catch { parsed = null; }
        assert('GET /json  -- valid JSON',           parsed !== null, true);
        assert('GET /json  -- status:ok field',      parsed && parsed.status, 'ok');
        assert('GET /json  -- runtime:WebAssembly',  parsed && parsed.runtime, 'WebAssembly');
    }

    // 3. phpinfo route
    {
        const { output, status } = runPHP(PHP, { REQUEST_URI: '/info', REQUEST_METHOD: 'GET' });
        assert('GET /info  -- status 200',         status, 200);
        assert('GET /info  -- contains phpinfo',   output, 'PHP Version');
    }

    // 4. 404 for unknown route
    {
        const { output, status } = runPHP(PHP, { REQUEST_URI: '/no-such-route', REQUEST_METHOD: 'GET' });
        assert('GET /no-such-route  -- status 404',  status, 404);
        assert('GET /no-such-route  -- JSON error',  output, 'Not Found');
    }

    // 5. POST route with body (exercises php_set_request_body)
    {
        const body = 'test=1&key=value';
        const { status } = runPHP(PHP, { REQUEST_URI: '/', REQUEST_METHOD: 'POST' }, body);
        // The home route is GET-only so we expect a 404, confirming method dispatch works
        assert('POST /  -- method dispatch (404)',  status, 404);
    }

    // 6. State isolation: a second GET / must return the same output (no leaking state)
    {
        const r1 = runPHP(PHP, { REQUEST_URI: '/json', REQUEST_METHOD: 'GET' });
        const r2 = runPHP(PHP, { REQUEST_URI: '/json', REQUEST_METHOD: 'GET' });
        assert('State isolation -- two /json calls identical',  r1.output, r2.output);
    }

    // 7. Math in PHP (bcmath extension)
    {
        // Write a minimal inline PHP file to the VFS and exec it
        const testScript = '<?php echo bcadd("1.5", "2.5", 1); ?>';
        PHP.FS.writeFile('/tmp/math_test.php', testScript);
        PHP.ccall('php_set_server_var', null, ['string','string'], ['REQUEST_METHOD','GET']);
        PHP.ccall('wasm_exec', 'number', ['string'], ['/tmp/math_test.php']);
        const out = PHP.ccall('php_get_output', 'string', [], []);
        PHP.ccall('php_reset', null, [], []);
        assert('bcmath bcadd("1.5","2.5") = 4.0',  out.trim(), '4.0');
    }

    // 8. JSON encode / decode round-trip
    {
        const testScript = '<?php $d=["a"=>1,"b"=>[1,2,3]]; echo json_decode(json_encode($d),true)["a"]; ?>';
        PHP.FS.writeFile('/tmp/json_test.php', testScript);
        PHP.ccall('php_set_server_var', null, ['string','string'], ['REQUEST_METHOD','GET']);
        PHP.ccall('wasm_exec', 'number', ['string'], ['/tmp/json_test.php']);
        const out = PHP.ccall('php_get_output', 'string', [], []);
        PHP.ccall('php_reset', null, [], []);
        assert('json encode/decode round-trip',  out.trim(), '1');
    }

    // 9. mbstring mb_strlen (does not need mbregex/oniguruma)
    {
        const testScript = '<?php echo mb_strlen("hello world"); ?>';
        PHP.FS.writeFile('/tmp/mb_test.php', testScript);
        PHP.ccall('php_set_server_var', null, ['string','string'], ['REQUEST_METHOD','GET']);
        PHP.ccall('wasm_exec', 'number', ['string'], ['/tmp/mb_test.php']);
        const out = PHP.ccall('php_get_output', 'string', [], []);
        PHP.ccall('php_reset', null, [], []);
        assert('mbstring mb_strlen("hello world") = 11',  out.trim(), '11');
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    console.log('');
    console.log(`Results: ${passed} passed, ${failed} failed`);
    if (failed > 0) {
        console.error('SOME TESTS FAILED');
        process.exit(1);
    } else {
        console.log('All tests passed -- PHP runs in WASM with no host PHP required.');
    }
}

main().catch(err => {
    console.error('Fatal:', err);
    process.exit(1);
});
