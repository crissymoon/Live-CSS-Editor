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

    gl.bindTexture(gl.TEXTURE_2D, tex);
    // pixels is a transferred ArrayBuffer -- Uint8Array view is zero-copy.
    const view = pixels instanceof ArrayBuffer ? new Uint8Array(pixels) : pixels;
    if (w !== tex_w || h !== tex_h) {
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, view);
      tex_w = w; tex_h = h;
    } else {
      gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, w, h,
                       gl.RGBA, gl.UNSIGNED_BYTE, view);
    }
    gl.useProgram(prog);
    gl.uniform1i(u_frame, 0);
    gl.viewport(0, 0, w, h);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }

  // -----------------------------------------------------------------------
  // Worker bridge
  // -----------------------------------------------------------------------
  let worker    = null;
  let workerReady = false;
  let msgId     = 0;
  const pending = new Map(); // id -> { resolve, reject }

  function startWorker(workerUrl) {
    worker = new Worker(workerUrl);
    worker.onmessage = ({ data }) => {
      if (data.type === 'ready') {
        workerReady = true;
        return;
      }
      if (data.type === 'error') {
        console.error('[xcm-wasm-comp] Worker error:', data.message);
        return;
      }
      const p = pending.get(data.id);
      if (!p) return;
      pending.delete(data.id);
      if (data.ok) p.resolve(data);
      else         p.reject(new Error(data.error || 'render failed'));
    };
    worker.onerror = (e) => {
      console.error('[xcm-wasm-comp] Worker fatal:', e);
    };
  }

  function workerCall(msg, transfer) {
    return new Promise((resolve, reject) => {
      const id = ++msgId;
      pending.set(id, { resolve, reject });
      worker.postMessage({ ...msg, id }, transfer || []);
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
  let _pendingFrame = null; // { pixels, w, h } from the last Worker render

  function _scheduledUpload() {
    if (!_pendingFrame || !canvas) return;
    const { pixels, w, h } = _pendingFrame;
    _pendingFrame = null;
    canvas.width  = w;
    canvas.height = h;
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

    async render(html, css, width, height) {
      if (!worker) throw new Error('Call init() first');
      await waitReady();
      const res = await workerCall({ cmd: 'render', html, css, width, height });
      // Buffer the result -- _scheduledUpload flushes it on the next
      // __xcmTick so the GPU upload always lands inside a rAF callback.
      // Overwrites any previously buffered but not-yet-uploaded frame
      // (only the newest frame is worth drawing).
      _pendingFrame = { pixels: res.pixels, w: res.width, h: res.height };
      return res.metrics;
    },

    async resize(width, height) {
      if (!worker) return;
      await workerCall({ cmd: 'resize', width, height });
    },

    // Render current page (with live DOM serialisation).
    async renderCurrentPage(css_override) {
      const html = document.documentElement.outerHTML;
      const css  = css_override || '';
      const w    = window.innerWidth;
      const h    = window.innerHeight;
      return comp.render(html, css, w, h);
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
