/**
 * wasm-compositor.js  --  Main-thread WebGL compositor for the WASM render core
 *
 * Receives RGBA8 pixel buffers from wasm-renderer.js (the Worker) and
 * composites them onto a fullscreen WebGL canvas.
 *
 * Why WebGL (not Canvas2D putImageData)?
 *   - putImageData is synchronous CPU->GPU upload on the main thread.
 *   - texSubImage2D + drawArrays is async: the GPU DMA copies the buffer
 *     while the CPU is free.  For large viewports this saves 5-15 ms/frame.
 *
 * API (injected as window.__xcmWasmComp):
 *   window.__xcmWasmComp.render(html, css, width, height)  -> Promise<metrics>
 *   window.__xcmWasmComp.resize(width, height)
 *   window.__xcmWasmComp.setCanvas(canvasElement)
 *   window.__xcmWasmComp.ready  -- true once WASM worker is initialised
 *
 * Injected by puppeteer-server.js into every new page via evaluateOnNewDocument.
 * Also usable standalone in any web page: <script src="wasm-compositor.js"></script>
 */
(function () {
  'use strict';

  // Prevent double-install.
  if (window.__xcmWasmComp) return;

  // -----------------------------------------------------------------------
  // GLSL shaders
  // -----------------------------------------------------------------------
  const VERT_SRC = `#version 300 es
precision highp float;
const vec2 POSITIONS[4] = vec2[4](
  vec2(-1.0,  1.0),
  vec2(-1.0, -1.0),
  vec2( 1.0,  1.0),
  vec2( 1.0, -1.0)
);
const vec2 UVS[4] = vec2[4](
  vec2(0.0, 0.0),
  vec2(0.0, 1.0),
  vec2(1.0, 0.0),
  vec2(1.0, 1.0)
);
out vec2 v_uv;
void main() {
  gl_Position = vec4(POSITIONS[gl_VertexID], 0.0, 1.0);
  v_uv = UVS[gl_VertexID];
}`;

  const FRAG_SRC = `#version 300 es
precision highp float;

// Per-frame parameters in a UBO -- single bufferSubData write maps
// directly to the GPU uniform block, no per-uniform state changes.
layout(std140) uniform FrameParams {
  vec2  u_res;        // render resolution in pixels
  float u_subpxl_str; // sub-pixel blend strength [0..1]
  float u_pad;        // std140 alignment
};

uniform sampler2D u_frame;
in  vec2 v_uv;
out vec4 fragColor;

void main() {
  vec4 c = texture(u_frame, v_uv);

  // Sub-pixel RGB horizontal reconstruction (ClearType-style).
  // R samples 1/3 px left, B samples 1/3 px right -- amplifies
  // per-channel coverage already written by the C++ rasterizer.
  vec2 px = vec2(1.0) / u_res;
  float r = texture(u_frame, v_uv + vec2(-px.x * 0.333, 0.0)).r;
  float b = texture(u_frame, v_uv + vec2( px.x * 0.333, 0.0)).b;
  c.r = mix(c.r, r, u_subpxl_str);
  c.b = mix(c.b, b, u_subpxl_str);

  fragColor = c;
}`;

  // -----------------------------------------------------------------------
  // WebGL state
  // -----------------------------------------------------------------------
  let canvas  = null;
  let gl      = null;
  let prog    = null;
  let tex     = null;
  let tex_w   = 0;
  let tex_h   = 0;
  let u_frame = null;
  // UBO: pre-allocated Float32Array mapped to a GPU uniform block.
  // Layout (std140): vec2 u_res (8 B) | float u_subpxl_str (4 B) | float u_pad (4 B)
  let uboBuffer = null;
  const uboData = new Float32Array(4); // [res_x, res_y, subpxl_str, pad]
  uboData[2] = 0.55; // default sub-pixel blend strength

  function initGl(cvs) {
    canvas = cvs;
    gl = cvs.getContext('webgl2', {
      alpha: false, antialias: false, depth: false,
      stencil: false, preserveDrawingBuffer: false,
      powerPreference: 'high-performance',
    });
    if (!gl) {
      console.warn('[xcm-wasm-comp] WebGL2 unavailable, falling back to Canvas2D');
      return false;
    }

    // Compile shaders.
    function compile(type, src) {
      const s = gl.createShader(type);
      gl.shaderSource(s, src);
      gl.compileShader(s);
      if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
        throw new Error(gl.getShaderInfoLog(s));
      }
      return s;
    }
    const vs = compile(gl.VERTEX_SHADER,   VERT_SRC);
    const fs = compile(gl.FRAGMENT_SHADER, FRAG_SRC);
    prog = gl.createProgram();
    gl.attachShader(prog, vs); gl.attachShader(prog, fs);
    gl.linkProgram(prog);
    if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
      throw new Error(gl.getProgramInfoLog(prog));
    }
    u_frame = gl.getUniformLocation(prog, 'u_frame');

    // UBO: bind FrameParams block to binding point 0 and allocate a
    // persistent GPU buffer -- updated each frame via bufferSubData
    // (a single DMA write to the mapped block, no per-uniform dispatch).
    const uboBlockIndex = gl.getUniformBlockIndex(prog, 'FrameParams');
    if (uboBlockIndex !== gl.INVALID_INDEX) {
      gl.uniformBlockBinding(prog, uboBlockIndex, 0);
      uboBuffer = gl.createBuffer();
      gl.bindBufferBase(gl.UNIFORM_BUFFER, 0, uboBuffer);
      gl.bufferData(gl.UNIFORM_BUFFER, uboData, gl.DYNAMIC_DRAW);
    }

    // Create texture.
    tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    // LINEAR required so the sub-pixel ±0.333px offsets in the fragment shader
    // actually interpolate between adjacent texels rather than snapping to the same one.
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    return true;
  }

  function uploadAndDraw(pixels, w, h) {
    if (!gl) return;

    // Update UBO data in-place (direct Float32Array view, no allocation).
    uboData[0] = w;
    uboData[1] = h;
    if (uboBuffer) {
      gl.bindBufferBase(gl.UNIFORM_BUFFER, 0, uboBuffer);
      gl.bufferSubData(gl.UNIFORM_BUFFER, 0, uboData);
    }

    // Write pixels into texBack.  texFront is still bound to any pending draw
    // commands in the GPU pipeline -- writing to texBack instead ensures the
    // GPU is never reading from the object being written.
    gl.bindTexture(gl.TEXTURE_2D, texBack);
    // pixels is a transferred ArrayBuffer -- Uint8Array view is zero-copy.
    const view = pixels instanceof ArrayBuffer ? new Uint8Array(pixels) : pixels;
    if (w !== tex_w || h !== tex_h) {
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, view);
      // Also resize texFront to keep both textures the same dimensions so the
      // swap is always valid without a re-allocation on the next frame.
      gl.bindTexture(gl.TEXTURE_2D, texFront);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, null);
      gl.bindTexture(gl.TEXTURE_2D, texBack);
      tex_w = w; tex_h = h;
    } else {
      gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, w, h,
                       gl.RGBA, gl.UNSIGNED_BYTE, view);
    }

    // Swap front and back.  From this point, texFront holds the new frame and
    // texBack holds the previous (now recycled for the next write).
    const tmp = texFront; texFront = texBack; texBack = tmp;

    gl.useProgram(prog);
    gl.uniform1i(u_frame, 0);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, texFront);
    gl.viewport(0, 0, w, h);

    // Scissor to the exact render region before clearing and drawing.
    // This prevents fragment writes -- and any sub-pixel interpolation
    // at the texture border -- from reaching pixels outside the uploaded
    // region when the canvas is momentarily larger than the render target
    // (e.g. first frame after a window resize before the new buffer lands).
    gl.enable(gl.SCISSOR_TEST);
    gl.scissor(0, 0, w, h);
    gl.clearColor(1.0, 1.0, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    gl.disable(gl.SCISSOR_TEST);

    // Check for GL errors after every draw.  getError() is a synchronous
    // GPU barrier so it is kept behind a debug flag to avoid stalling the
    // pipeline in production.  Set window.__xcmGlDebug = true in the
    // console to enable.  Any error is logged once and the flag auto-clears
    // after the first hit so the console is not flooded.
    if (window.__xcmGlDebug) {
      const err = gl.getError();
      if (err !== gl.NO_ERROR) {
        // Map the numeric code to a readable name.
        const _GL_ERR = {
          [gl.INVALID_ENUM]:                  'INVALID_ENUM',
          [gl.INVALID_VALUE]:                 'INVALID_VALUE',
          [gl.INVALID_OPERATION]:             'INVALID_OPERATION',
          [gl.INVALID_FRAMEBUFFER_OPERATION]: 'INVALID_FRAMEBUFFER_OPERATION',
          [gl.OUT_OF_MEMORY]:                 'OUT_OF_MEMORY',
          [gl.CONTEXT_LOST_WEBGL]:            'CONTEXT_LOST_WEBGL',
        };
        console.error('[xcm-gl] getError() =>', _GL_ERR[err] || ('0x' + err.toString(16)),
          '| tex:', tex_w, 'x', tex_h, '| canvas:', w, 'x', h);
        // Auto-disable after first error so the console log is not spammed
        // every frame.  Re-set window.__xcmGlDebug = true to resume.
        window.__xcmGlDebug = false;
      }
    }
  }

  // -----------------------------------------------------------------------
  // Worker bridge -- MessageChannel port for lower-latency messaging.
  //
  // The browser dispatches postMessage() on the Worker's global scope via the
  // task queue, which competes with other tasks (resource loads, microtasks).
  // A dedicated MessageChannel port pair is scheduled on its own port
  // message queue which has higher priority than generic tasks in most
  // browser implementations, reducing round-trip latency by 1-3 ms per frame.
  //
  // Architecture:
  //   Main thread holds _compPort (MessageChannel.port1).
  //   Worker receives _workerPort (MessageChannel.port2) via the init message.
  //   All subsequent render/resize/ping traffic flows through the port pair.
  //   The worker's global postMessage is only used for the initial handshake
  //   (type:'ready' / type:'error') before the port is established.
  // -----------------------------------------------------------------------
  let worker      = null;
  let workerReady = false;
  let _compPort   = null;   // main-thread side of the MessageChannel
  let msgId       = 0;
  const pending   = new Map(); // id -> { resolve, reject }

  function startWorker(workerUrl) {
    worker = new Worker(workerUrl);

    // Handshake listener on the Worker global scope (only used until ready).
    worker.onmessage = ({ data }) => {
      if (data.type === 'ready') {
        // Worker is initialised -- transfer a MessageChannel port to it so all
        // subsequent messages travel through the dedicated port queue.
        const mc = new MessageChannel();
        _compPort = mc.port1;
        _compPort.onmessage = _onPortMessage;
        // Send port2 to the worker.  Transferred (not copied) so it is a true
        // shared endpoint with zero serialisation overhead.
        worker.postMessage({ type: 'port', port: mc.port2 }, [mc.port2]);
        workerReady = true;
        return;
      }
      if (data.type === 'error') {
        console.error('[xcm-wasm-comp] Worker error:', data.message);
        return;
      }
    };
    worker.onerror = (e) => {
      console.error('[xcm-wasm-comp] Worker fatal:', e);
    };
  }

  function _onPortMessage({ data }) {
    const p = pending.get(data.id);
    if (!p) return;
    pending.delete(data.id);
    if (data.ok) p.resolve(data);
    else         p.reject(new Error(data.error || 'render failed'));
  }

  // Send a message through the port (or Worker global if port not yet ready).
  // Builds the outbound object without object spread to avoid a hidden copy.
  function workerCall(cmd, params, transfer) {
    return new Promise((resolve, reject) => {
      const id = ++msgId;
      pending.set(id, { resolve, reject });
      const msg = { id, cmd };
      if (params) {
        // Copy only known keys -- avoids spreading unknown enumerable properties.
        if (params.html     !== undefined) msg.html     = params.html;
        if (params.css      !== undefined) msg.css      = params.css;
        if (params.width    !== undefined) msg.width    = params.width;
        if (params.height   !== undefined) msg.height   = params.height;
        if (params.scroll_y !== undefined) msg.scroll_y = params.scroll_y;
      }
      const port = _compPort || worker;
      port.postMessage(msg, transfer || []);
    });
  }

  // -----------------------------------------------------------------------
  // Wait for worker ready with timeout.
  // -----------------------------------------------------------------------
  function waitReady(timeoutMs = 15000) {
    if (workerReady) return Promise.resolve();
    return new Promise((resolve, reject) => {
      const t = setTimeout(() => reject(new Error('WASM worker init timeout')), timeoutMs);
      const check = setInterval(() => {
        if (workerReady) { clearInterval(check); clearTimeout(t); resolve(); }
      }, 50);
    });
  }

  // -----------------------------------------------------------------------
  // Render mutex with latest-wins queuing.
  //
  // The worker is single-threaded: if the main thread sends N render messages
  // before the worker finishes one, all N queue up and execute sequentially.
  // By the time the 2nd renders, the viewport has moved on -- producing wasted
  // CPU time and a visual stutter as stale frames arrive out of order.
  //
  // The mutex ensures only ONE render message is in-flight at a time.
  // If render() is called while a render is in-flight, the call is stored as
  // _latestRender (replacing any previous queued-but-not-yet-sent request).
  // The moment the in-flight render completes, _latestRender is dispatched.
  // This guarantees the worker always renders the most current state and the
  // message queue never grows beyond depth 1.
  // -----------------------------------------------------------------------
  let _renderInFlight  = false;
  let _latestRender    = null;  // { html, css, width, height, scroll_y, resolve, reject }

  function _dispatchRender(p) {
    _renderInFlight = true;
    workerCall('render', p)
      .then(res => {
        _engRecordFrame();
        _pendingFrame = { pixels: res.pixels, w: res.width, h: res.height, dpr: _dpr };
        p.resolve(res.metrics);
      })
      .catch(err => p.reject(err))
      .finally(() => {
        _renderInFlight = false;
        // If a newer render arrived while we were busy, send it now.  Only the
        // latest request is retained so the queue never accumulates stale frames.
        if (_latestRender) {
          const next   = _latestRender;
          _latestRender = null;
          _dispatchRender(next);
        }
      });
  }
  const _engTimestamps = [];   // circular: stores performance.now() of each render
  const ENG_WINDOW_MS  = 1000;

  function _engRecordFrame() {
    const now = performance.now();
    _engTimestamps.push(now);
    // Evict timestamps older than the measurement window.
    const cutoff = now - ENG_WINDOW_MS;
    while (_engTimestamps.length > 0 && _engTimestamps[0] < cutoff)
      _engTimestamps.shift();
  }

  function _engFps() {
    if (_engTimestamps.length < 2) return 0;
    // frames completed in the last ENG_WINDOW_MS
    return _engTimestamps.length;
  }

  // -----------------------------------------------------------------------
  // Canvas2D fallback.
  // -----------------------------------------------------------------------
  let ctx2d = null;

  function uploadCanvas2d(pixels, w, h) {
    if (!ctx2d) ctx2d = canvas.getContext('2d');
    const id = new ImageData(new Uint8ClampedArray(pixels), w, h);
    ctx2d.putImageData(id, 0, 0);
  }

  // -----------------------------------------------------------------------
  // rAF-synced upload buffer.
  // The Worker's render Promise resolves at an arbitrary micro-task boundary
  // which can be mid-frame.  Buffering the result here and flushing it on
  // the next __xcmTick guarantees texSubImage2D fires inside a display-
  // linked rAF callback, eliminating partial-frame texture tears.
  // -----------------------------------------------------------------------
  let _pendingFrame = null; // { pixels, w, h, dpr } from the last Worker render
  let _dpr = 1;             // devicePixelRatio at the time of the last render call

  function _scheduledUpload() {
    if (!_pendingFrame || !canvas) return;
    const { pixels, w, h, dpr } = _pendingFrame;
    _pendingFrame = null;

    // Physical backing size in device pixels.
    // CSS display size is 1/_dpr of the backing size so the canvas occupies
    // exactly the expected CSS viewport area while rendering at full Retina
    // resolution (2x DPR = 4x pixel count on a MacBook Pro).
    if (canvas.width  !== w) canvas.width  = w;
    if (canvas.height !== h) canvas.height = h;
    // Keep the CSS size fixed at logical (CSS) pixels so the canvas does not
    // inadvertently enlarge the scroll area or overflow its container.
    const cssW = Math.round(w / dpr) + 'px';
    const cssH = Math.round(h / dpr) + 'px';
    if (canvas.style.width  !== cssW) canvas.style.width  = cssW;
    if (canvas.style.height !== cssH) canvas.style.height = cssH;

    if (gl) {
      uploadAndDraw(pixels, w, h);
    } else {
      uploadCanvas2d(pixels, w, h);
    }
  }

  function _registerRafUpload() {
    if (typeof window.__xcmTick === 'function') {
      window.__xcmTick(_scheduledUpload);
    } else {
      // Ticker not ready yet (Hz sampling still in progress) -- retry.
      requestAnimationFrame(_registerRafUpload);
    }
  }
  // Defer until after ticker has had at least one rAF to initialise.
  requestAnimationFrame(_registerRafUpload);

  // -----------------------------------------------------------------------
  // Public API
  // -----------------------------------------------------------------------
  const comp = {
    ready: false,

    setCanvas(cvs) {
      canvas = cvs;
      try { initGl(cvs); } catch (e) { console.warn('[xcm-wasm-comp]', e); }
    },

    async init(workerUrl) {
      workerUrl = workerUrl || (function () {
        // Resolve relative to this script's location.
        const scripts = document.querySelectorAll('script[src]');
        for (const s of scripts) {
          if (s.src.includes('wasm-compositor')) {
            return s.src.replace('wasm-compositor.js', 'wasm-renderer.js');
          }
        }
        return './wasm-renderer.js';
      })();
      startWorker(workerUrl);
      await waitReady();
      comp.ready = true;
    },

    async render(html, css, width, height, scroll_y = 0) {
      if (!worker) throw new Error('Call init() first');
      await waitReady();
      return new Promise((resolve, reject) => {
        const p = { html, css, width, height, scroll_y, resolve, reject };
        if (_renderInFlight) {
          // A render is already in the worker.  Drop the previously queued
          // request (it is stale) and replace with the latest parameters.
          // The dropped request resolves with null metrics -- callers should
          // treat a null return as "superseded, no metrics available".
          if (_latestRender) _latestRender.resolve(null);
          _latestRender = p;
        } else {
          _dispatchRender(p);
        }
      });
    },

    async resize(width, height) {
      if (!worker) return;
      await workerCall('resize', { width, height });
    },

    // Returns current engine renders per second (WASM completions in last 1 s).
    engineFps() { return _engFps(); },

    // Render current page (with live DOM serialisation, current scroll position,
    // and physical Retina pixel resolution via window.devicePixelRatio).
    //
    // MacBook Pro Retina displays have devicePixelRatio = 2, meaning the backing
    // buffer must be 2x2 = 4x larger than the CSS viewport to fill every physical
    // pixel.  Without this, the GPU composites a 1x buffer scaled up 2x, wasting
    // half the display bandwidth and making the render look soft.
    async renderCurrentPage(css_override) {
      const html = document.documentElement.outerHTML;
      const css  = css_override || '';
      // __xcmDprCap lets callers (or the dev console) reduce the effective DPR
      // without changing the OS display setting -- useful for profiling whether
      // the 4x pixel count at DPR=2 is the render bottleneck.
      // Default cap is 1.5: still noticeably sharper than 1x on Retina but
      // reduces the pixel buffer to ~2.25x instead of 4x, cutting render time
      // roughly in half for large viewports.
      const cap  = (typeof window.__xcmDprCap === 'number') ? window.__xcmDprCap : 1.5;
      const dpr  = Math.min(window.devicePixelRatio || 1, cap);
      _dpr       = dpr;
      const w    = Math.round(window.innerWidth  * dpr);
      const h    = Math.round(window.innerHeight * dpr);
      // Pass the live scroll position scaled to physical pixels so the WASM
      // engine's viewport-cull math works in the same coordinate space as the
      // physical pixel buffer.
      const sy   = Math.round((window.scrollY || 0) * dpr);
      return comp.render(html, css, w, h, sy);
    },
  };

  window.__xcmWasmComp = comp;

  // Auto-init if a canvas#xcm-canvas is present.
  document.addEventListener('DOMContentLoaded', () => {
    const cvs = document.getElementById('xcm-canvas');
    if (cvs) {
      comp.setCanvas(cvs);
      comp.init();
    }
  });

})();
