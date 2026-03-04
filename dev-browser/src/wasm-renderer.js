/**
 * wasm-renderer.js  --  Web Worker: load render_core.wasm, expose render API
 *
 * Runs inside a dedicated Worker.  The main thread communicates via a
 * dedicated MessageChannel port (see wasm-compositor.js for the handshake).
 *
 * Startup handshake:
 *   1. Worker loads WASM and posts  { type:'ready' }  on its global scope.
 *   2. Main thread sends  { type:'port', port: MessagePort }  on the global scope.
 *   3. Worker stores the port and routes ALL subsequent replies through it.
 *
 * Inbound messages  (received on the transferred MessagePort):
 *   { id, cmd: 'render', html, css, width, height, scroll_y }
 *      -> renders HTML+CSS with scroll offset
 *      -> replies { id, ok:true, width, height, pixels:ArrayBuffer, metrics:{} }
 *
 *   { id, cmd: 'resize', width, height }
 *      -> adjusts the internal pixel buffer size (destroys and recreates context)
 *      -> replies { id, ok:true }
 *
 *   { id, cmd: 'status' }
 *      -> replies { id, ok:true, ready:bool }
 *
 *   { id, cmd: 'ping' }
 *      -> replies { id, pong:true }
 *
 * Outbound messages:
 *   { type: 'ready' }           -- emitted on global scope once after WASM loads
 *   { type: 'error', message }  -- emitted on global scope on fatal init failure
 *
 * After the handshake all traffic travels on the MessagePort, which has lower
 * task-queue latency than the Worker's global scope channel.  Pixel buffers are
 * transferred zero-copy via ArrayBuffer transfer.
 */

'use strict';

// -------------------------------------------------------------------------
// State
// -------------------------------------------------------------------------
let Module      = null;   // Emscripten module instance
let api         = null;   // Wrapped C API
let ready       = false;
let ctx_ptr     = 0;      // current xcm_ctx*
let ctx_w       = 0;
let ctx_h       = 0;
// _replyPort: once the main thread transfers a MessageChannel port we route
// all replies through it instead of the Worker global scope.  Falls back to
// self.postMessage for any messages that arrive before the port handshake.
let _replyPort  = null;

function _reply(msg, transfer) {
  const target = _replyPort || self;
  if (transfer && transfer.length > 0) {
    target.postMessage(msg, transfer);
  } else {
    target.postMessage(msg);
  }
}

// -------------------------------------------------------------------------
// Load the Emscripten-generated module.
// The render_core.js glue script must be in the same directory as this worker.
// -------------------------------------------------------------------------
function loadWasm() {
  const wasmDir = self.location.href.replace(/\/[^/]+$/, '/');
  return new Promise((resolve, reject) => {
    importScripts(wasmDir + 'render_core.js');
    // XcmRenderCore is the factory exported by -sEXPORT_NAME=XcmRenderCore.
    XcmRenderCore({
      locateFile(path) {
        return wasmDir + path;
      },
    }).then(mod => {
      resolve(mod);
    }).catch(reject);
  });
}

// -------------------------------------------------------------------------
// Wrap the raw C API via cwrap.
// -------------------------------------------------------------------------
function wrapApi(mod) {
  return {
    create:       mod.cwrap('xcm_create',       'number', ['number','number']),
    destroy:      mod.cwrap('xcm_destroy',       null,     ['number']),
    render:       mod.cwrap('xcm_render',        'number', ['number','number','number','number','number','number']),
    pixels:       mod.cwrap('xcm_pixels',        'number', ['number']),
    width:        mod.cwrap('xcm_width',         'number', ['number']),
    height:       mod.cwrap('xcm_height',        'number', ['number']),
    doc_height:   mod.cwrap('xcm_doc_height',    'number', ['number']),
    metrics_json: mod.cwrap('xcm_metrics_json',  'string', ['number']),
    alloc:        mod.cwrap('xcm_alloc',         'number', ['number']),
    free:         mod.cwrap('xcm_free',          null,     ['number']),
    HEAPU8:       mod.HEAPU8,  // live view into WASM linear memory
  };
}

// -------------------------------------------------------------------------
// Ensure a render context for the given dimensions.
// -------------------------------------------------------------------------
function ensureCtx(width, height) {
  if (ctx_ptr && ctx_w === width && ctx_h === height) return;
  if (ctx_ptr) {
    api.destroy(ctx_ptr);
    ctx_ptr = 0;
  }
  ctx_ptr = api.create(width, height);
  ctx_w   = width;
  ctx_h   = height;
}

// -------------------------------------------------------------------------
// Write a JS string into WASM heap via xcm_alloc.
// Returns { ptr, len }.  Caller must call xcm_free(ptr) after use.
// -------------------------------------------------------------------------
function allocStr(str) {
  const encoded = new TextEncoder().encode(str);
  const ptr     = api.alloc(encoded.length + 1);
  api.HEAPU8.set(encoded, ptr);
  api.HEAPU8[ptr + encoded.length] = 0; // null terminator
  return { ptr, len: encoded.length };
}

// -------------------------------------------------------------------------
// Perform a full render and return an ArrayBuffer of RGBA pixels.
// scroll_y: document vertical scroll offset in CSS pixels.
// -------------------------------------------------------------------------
function render(html, css, width, height, scroll_y = 0) {
  ensureCtx(width, height);

  const htmlBuf = allocStr(html);
  const cssBuf  = allocStr(css);

  // xcm_render(ctx, html_ptr, html_len, css_ptr, css_len, scroll_y)
  const rc = Module._xcm_render(ctx_ptr,
    htmlBuf.ptr, htmlBuf.len,
    cssBuf.ptr,  cssBuf.len,
    scroll_y);

  api.free(htmlBuf.ptr);
  api.free(cssBuf.ptr);

  if (rc !== 0) throw new Error(`xcm_render returned ${rc}`);

  const pixPtr   = Module._xcm_pixels(ctx_ptr);
  const byteLen  = width * height * 4;
  // Copy pixels out of WASM heap into a new ArrayBuffer (transferred to main thread).
  const src      = api.HEAPU8.subarray(pixPtr, pixPtr + byteLen);
  const out      = new Uint8ClampedArray(byteLen);
  out.set(src);

  let metrics = {};
  try {
    metrics = JSON.parse(Module.UTF8ToString(Module._xcm_metrics_json(ctx_ptr)));
  } catch (_) {}

  return { pixels: out.buffer, width, height, metrics };
}

// -------------------------------------------------------------------------
// Message handler -- shared by both the Worker global scope and the port.
// -------------------------------------------------------------------------
function _handleMessage({ data }) {
  // ---- Port handshake (arrives on global scope only) ----
  if (data.type === 'port') {
    _replyPort = data.port;
    // Attach the same handler to the port so messages sent via the port
    // are processed via this same function.
    _replyPort.onmessage = _handleMessage;
    return;
  }

  const { id, cmd } = data;
  try {
    if (cmd === 'ping') {
      _reply({ id, pong: true });
      return;
    }
    if (cmd === 'status') {
      _reply({ id, ok: true, ready, wasmLoaded: !!Module });
      return;
    }
    if (!ready) {
      _reply({ id, ok: false, error: 'WASM not ready yet' });
      return;
    }
    if (cmd === 'render') {
      const { html = '', css = '', width = 800, height = 600, scroll_y = 0 } = data;
      const result = render(html, css, width, height, scroll_y);
      // Build reply explicitly -- no object spread to avoid a hidden allocation.
      _reply(
        { id, ok: true,
          pixels: result.pixels, width: result.width,
          height: result.height, metrics: result.metrics },
        [result.pixels]   // transfer zero-copy
      );
      return;
    }
    if (cmd === 'resize') {
      const { width = 800, height = 600 } = data;
      ensureCtx(width, height);
      _reply({ id, ok: true });
      return;
    }
    _reply({ id, ok: false, error: `Unknown command: ${cmd}` });
  } catch (err) {
    _reply({ id, ok: false, error: err.message || String(err) });
  }
}

self.onmessage = _handleMessage;

// -------------------------------------------------------------------------
// Init: load WASM on worker startup.
// -------------------------------------------------------------------------
(async () => {
  try {
    Module = await loadWasm();
    // Initialize the cwrap API.
    api = wrapApi(Module);
    // Expose raw _ functions for manual memory operations too.
    ready = true;
    self.postMessage({ type: 'ready' });
  } catch (err) {
    self.postMessage({ type: 'error', message: String(err) });
  }
})();
