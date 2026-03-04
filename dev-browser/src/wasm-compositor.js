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
uniform sampler2D u_frame;
in vec2 v_uv;
out vec4 fragColor;
void main() {
  fragColor = texture(u_frame, v_uv);
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

    // Create texture.
    tex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    return true;
  }

  function uploadAndDraw(pixels, w, h) {
    if (!gl) return;
    gl.bindTexture(gl.TEXTURE_2D, tex);
    if (w !== tex_w || h !== tex_h) {
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, new Uint8ClampedArray(pixels));
      tex_w = w; tex_h = h;
    } else {
      gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, w, h,
                       gl.RGBA, gl.UNSIGNED_BYTE, new Uint8ClampedArray(pixels));
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
      // Composite result onto canvas.
      if (canvas) {
        canvas.width  = res.width;
        canvas.height = res.height;
        if (gl) {
          uploadAndDraw(res.pixels, res.width, res.height);
        } else {
          uploadCanvas2d(res.pixels, res.width, res.height);
        }
      }
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
