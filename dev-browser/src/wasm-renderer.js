/**
 * wasm-renderer.js  --  Web Worker: load render_core.wasm, expose render API
 *
 * Runs inside a dedicated Worker.  The main thread communicates via postMessage.
 *
 * Inbound messages  (from main thread):
 *   { id, cmd: 'render', html, css, width, height }
 *      -> renders HTML+CSS at given viewport size
 *      -> replies { id, ok:true, width, height, pixels:ArrayBuffer, metrics:{} }
 *
 *   { id, cmd: 'resize', width, height }
 *      -> adjusts the internal pixel buffer size (destroys and recreates context)
 *      -> replies { id, ok:true }
 *
 *   { id, cmd: 'status' }
 *      -> replies { id, ok:true, ready:bool, wasmSize }
 *
 *   { id, cmd: 'ping' }
 *      -> replies { id, pong:true }
 *
 * Outbound messages:
 *   { type: 'ready' }           -- emitted once after WASM is loaded
 *   { type: 'error', message }  -- emitted on fatal init failure
 *
 * The pixel buffer is transferred (zero-copy) to the main thread via
 * ArrayBuffer transfer.  The main thread should upload it to a WebGL texture
 * or draw it onto a Canvas2D context.
 *
 * Usage (main thread):
 *   const worker = new Worker('./wasm-renderer.js');
 *   worker.onmessage = ({data}) => {
 *     if (data.type === 'ready') {
 *       worker.postMessage({ id:1, cmd:'render', html:'...', css:'...', width:800, height:600 });
 *     } else if (data.id === 1 && data.ok) {
 *       // data.pixels: ArrayBuffer (RGBA8, data.width * data.height * 4 bytes)
 *       const imageData = new ImageData(new Uint8ClampedArray(data.pixels), data.width, data.height);
 *       ctx.putImageData(imageData, 0, 0);
 *     }
 *   };
 */

'use strict';

// -------------------------------------------------------------------------
// State
// -------------------------------------------------------------------------
let Module   = null;   // Emscripten module instance
let api      = null;   // Wrapped C API
let ready    = false;
let ctx_ptr  = 0;      // current xcm_ctx*
let ctx_w    = 0;
let ctx_h    = 0;

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
    render:       mod.cwrap('xcm_render',        'number', ['number','string','number','string','number']),
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
// -------------------------------------------------------------------------
function render(html, css, width, height) {
  ensureCtx(width, height);

  const htmlBuf = allocStr(html);
  const cssBuf  = allocStr(css);

  const rc = Module._xcm_render(ctx_ptr,
    htmlBuf.ptr, htmlBuf.len,
    cssBuf.ptr,  cssBuf.len);

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
// Message handler
// -------------------------------------------------------------------------
self.onmessage = function ({ data }) {
  const { id, cmd } = data;
  try {
    if (cmd === 'ping') {
      self.postMessage({ id, pong: true });
      return;
    }
    if (cmd === 'status') {
      self.postMessage({ id, ok: true, ready, wasmLoaded: !!Module });
      return;
    }
    if (!ready) {
      self.postMessage({ id, ok: false, error: 'WASM not ready yet' });
      return;
    }
    if (cmd === 'render') {
      const { html = '', css = '', width = 800, height = 600 } = data;
      const result = render(html, css, width, height);
      // Transfer pixel ArrayBuffer (zero-copy).
      self.postMessage({ id, ok: true, ...result }, [result.pixels]);
      return;
    }
    if (cmd === 'resize') {
      const { width = 800, height = 600 } = data;
      ensureCtx(width, height);
      self.postMessage({ id, ok: true });
      return;
    }
    self.postMessage({ id, ok: false, error: `Unknown command: ${cmd}` });
  } catch (err) {
    self.postMessage({ id, ok: false, error: err.message || String(err) });
  }
};

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
