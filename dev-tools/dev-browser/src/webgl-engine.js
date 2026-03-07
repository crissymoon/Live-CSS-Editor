/**
 * webgl-engine.js  v3
 *
 * GPU pre-warmer: WebGPU-first, WebGL2/WebGL1 fallback.
 *
 * Root cause of remaining jank
 * ----------------------------
 * GPU drivers lazily compile shader programs on first draw.  On WebGL the
 * driver defers GLSL compilation until the first drawArrays/drawElements
 * call that uses the program.  This stall blocks the compositor for 4-30 ms
 * on the GPU thread, causing a hard frame drop even when the main thread is
 * completely idle -- the typical "one bad frame every N images" pattern.
 *
 * Three-part fix
 * --------------
 * 1. Compile all shaders at page-load idle time before any image scrolls
 *    into view.  One universal program with all parameters expressed as
 *    uniforms (not #define constants) so the driver never compiles a second
 *    variant.  Issue a dummy draw call immediately after link to force the
 *    driver to finish compilation synchronously during idle time.
 *
 * 2. WebGPU path (Safari / WKWebView macOS 14+):
 *    device.queue.copyExternalImageToTexture() bypasses shader code entirely
 *    for image upload.  createRenderPipelineAsync() pre-compiles the Metal
 *    pipeline on a background thread; the promise only resolves after the
 *    compiled binary is ready, so no lazy compilation ever occurs at runtime.
 *
 * 3. Pre-warm the pipeline immediately after init by submitting one no-op
 *    command encoder.  This forces the Metal command queue to finish any
 *    internal driver state setup before scroll begins.
 *
 * Uniform-over-constants policy
 * ------------------------------
 * Every parameter that could cause a shader variant (format, premultiplied
 * alpha, mipmap level) is expressed as a uniform.  The program is compiled
 * once.  The GPU driver shader cache always hits the same binary.
 *
 * Public API: window.__xcm.gpu
 * Backend detection: window.__xcm.gpu.backend  ('webgpu'|'webgl2'|'webgl1'|'none')
 *
 * Injected at DocumentEnd, main frame only.
 */
(function (global) {
    'use strict';

    if (global.__xcmGpuLoaded__) return;
    global.__xcmGpuLoaded__ = true;

    // ── Shared state ─────────────────────────────────────────────────────────
    var _backend  = 'none';  // 'webgpu' | 'webgl2' | 'webgl1' | 'none'
    var _ready    = false;
    var _initProm = null;

    // Unified cache: url -> { bytes }
    var _cache     = new Map();
    var _lruList   = [];
    var _cacheBytes = 0;
    var MAX_CACHE_BYTES = 96 * 1024 * 1024;  // 96 MB

    var _inFlight = new Set();

    // ── LRU helpers ───────────────────────────────────────────────────────────
    function _lruTouch(url) {
        var i = _lruList.indexOf(url);
        if (i !== -1) _lruList.splice(i, 1);
        _lruList.push(url);
    }

    function _lruEvict(destroyFn) {
        while (_cacheBytes > MAX_CACHE_BYTES && _lruList.length) {
            var url   = _lruList.shift();
            var entry = _cache.get(url);
            if (entry) {
                if (destroyFn) destroyFn(entry);
                _cacheBytes -= entry.bytes || 0;
                _cache.delete(url);
            }
        }
    }

    // ── WGSL shaders (WebGPU) ─────────────────────────────────────────────────
    // Single pipeline.  Premultiplied-alpha handled by a uniform, not a variant.
    var WGSL_VERT = [
        'struct Out { @builtin(position) pos: vec4f, @location(0) uv: vec2f };',
        '@vertex fn vs(@builtin(vertex_index) vi: u32) -> Out {',
        '  var pos = array<vec2f,4>(vec2f(-1.0,1.0),vec2f(1.0,1.0),vec2f(-1.0,-1.0),vec2f(1.0,-1.0));',
        '  var uv  = array<vec2f,4>(vec2f(0.0,0.0),vec2f(1.0,0.0),vec2f(0.0,1.0),vec2f(1.0,1.0));',
        '  return Out(vec4f(pos[vi],0.0,1.0), uv[vi]);',
        '}',
    ].join('\n');

    var WGSL_FRAG = [
        '@group(0) @binding(0) var tex:  texture_2d<f32>;',
        '@group(0) @binding(1) var samp: sampler;',
        'struct P { premul: f32 };',
        '@group(0) @binding(2) var<uniform> p: P;',
        '@fragment fn fs(@location(0) uv: vec2f) -> @location(0) vec4f {',
        '  var c = textureSample(tex, samp, uv);',
        '  if (p.premul > 0.5) { c = vec4f(c.rgb * c.a, c.a); }',
        '  return c;',
        '}',
    ].join('\n');

    // ── GLSL shaders (WebGL) ──────────────────────────────────────────────────
    // One program.  All variants (premul, format) expressed as uniforms.
    var GLSL_VERT = [
        'attribute vec2 a_pos;',
        'attribute vec2 a_uv;',
        'varying   vec2 v_uv;',
        'void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }',
    ].join('\n');

    var GLSL_FRAG = [
        'precision mediump float;',
        'varying   vec2      v_uv;',
        'uniform   sampler2D u_tex;',
        'uniform   float     u_premul;',
        'void main() {',
        '  vec4 c = texture2D(u_tex, v_uv);',
        '  if (u_premul > 0.5) c = vec4(c.rgb * c.a, c.a);',
        '  gl_FragColor = c;',
        '}',
    ].join('\n');

    // ── WebGPU backend ────────────────────────────────────────────────────────
    var _gpuDevice   = null;
    var _gpuPipeline = null;
    var _gpuSampler  = null;
    var _gpuBgl      = null;

    function _initWebGPU() {
        if (!navigator.gpu) return Promise.resolve(false);
        return navigator.gpu.requestAdapter({ powerPreference: 'high-performance' })
            .then(function (adapter) {
                if (!adapter) return false;
                return adapter.requestDevice();
            })
            .then(function (device) {
                if (!device || device === false) return false;
                _gpuDevice = device;

                // Explicit bind group layout prevents the driver from inferring
                // it lazily on first bind -- another common source of first-use stalls.
                _gpuBgl = device.createBindGroupLayout({
                    entries: [
                        { binding: 0, visibility: GPUShaderStage.FRAGMENT,
                          texture:  { sampleType: 'float' } },
                        { binding: 1, visibility: GPUShaderStage.FRAGMENT,
                          sampler:  { type: 'filtering' } },
                        { binding: 2, visibility: GPUShaderStage.FRAGMENT,
                          buffer:   { type: 'uniform' } },
                    ],
                });

                var vsm = device.createShaderModule({ code: WGSL_VERT });
                var fsm = device.createShaderModule({ code: WGSL_FRAG });

                // createRenderPipelineAsync: Metal compiler runs on a bg thread.
                // Resolves only after the compiled binary is ready -- zero lazy
                // compilation in the hot path.
                return device.createRenderPipelineAsync({
                    layout:   device.createPipelineLayout({ bindGroupLayouts: [_gpuBgl] }),
                    vertex:   { module: vsm, entryPoint: 'vs' },
                    fragment: {
                        module: fsm, entryPoint: 'fs',
                        targets: [{ format: 'rgba8unorm' }],
                    },
                    primitive: { topology: 'triangle-strip' },
                });
            })
            .then(function (pipeline) {
                if (!pipeline || pipeline === false) return false;
                _gpuPipeline = pipeline;
                _gpuSampler  = _gpuDevice.createSampler({
                    magFilter: 'linear', minFilter: 'linear',
                });
                _backend = 'webgpu';
                _ready   = true;
                // Pre-warm: submit one empty command encoder so the Metal command
                // queue finishes any internal driver state setup now, not on first scroll.
                var cmd = _gpuDevice.createCommandEncoder();
                _gpuDevice.queue.submit([cmd.finish()]);
                return true;
            })
            .catch(function () { return false; });
    }

    function _gpuUpload(url, bitmap) {
        if (_cache.has(url)) { _lruTouch(url); return; }
        var w = Math.min(bitmap.width  || 1, 4096);
        var h = Math.min(bitmap.height || 1, 4096);
        // copyExternalImageToTexture: DMA path, no shader involved.
        var tex = _gpuDevice.createTexture({
            size:   [w, h, 1],
            format: 'rgba8unorm',
            usage:  GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST
                  | GPUTextureUsage.RENDER_ATTACHMENT,
        });
        _gpuDevice.queue.copyExternalImageToTexture(
            { source: bitmap, flipY: false },
            { texture: tex },
            [w, h],
        );
        // Submit empty encoder to flush the copy to the GPU driver.
        var cmd = _gpuDevice.createCommandEncoder();
        _gpuDevice.queue.submit([cmd.finish()]);
        var bytes = w * h * 4;
        _cache.set(url, { tex: tex, bytes: bytes });
        _lruList.push(url);
        _cacheBytes += bytes;
        _lruEvict(function (e) { if (e.tex) e.tex.destroy(); });
    }

    // ── WebGL backend ─────────────────────────────────────────────────────────
    var _gl      = null;
    var _glver   = 0;
    var _glProg  = null;
    var _glBuf   = null;
    var _glCache = new Map();  // url -> WebGLTexture

    function _compileShader(gl, type, src) {
        var sh = gl.createShader(type);
        gl.shaderSource(sh, src);
        gl.compileShader(sh);
        return sh;
    }

    function _initWebGL() {
        var opts = {
            antialias: false, alpha: true,
            powerPreference: 'high-performance',
            preserveDrawingBuffer: false,
            failIfMajorPerformanceCaveat: false,
        };
        var c;
        try { c = new OffscreenCanvas(2, 2); } catch (_) {
            c = document.createElement('canvas');
            c.width = 2; c.height = 2;
        }
        _gl = c.getContext('webgl2', opts);
        if (_gl) {
            _glver = 2;
        } else {
            _gl = c.getContext('webgl', opts)
               || c.getContext('experimental-webgl', { antialias: false });
            _glver = _gl ? 1 : 0;
        }
        if (!_gl) return false;

        // Compile the universal program once, cover all variants via uniforms.
        var vs = _compileShader(_gl, _gl.VERTEX_SHADER,   GLSL_VERT);
        var fs = _compileShader(_gl, _gl.FRAGMENT_SHADER, GLSL_FRAG);
        _glProg = _gl.createProgram();
        _gl.attachShader(_glProg, vs);
        _gl.attachShader(_glProg, fs);
        _gl.linkProgram(_glProg);
        _gl.useProgram(_glProg);

        // Full-screen quad buffer: positions + UVs interleaved.
        var verts = new Float32Array([
            -1,  1,  0, 0,
             1,  1,  1, 0,
            -1, -1,  0, 1,
             1, -1,  1, 1,
        ]);
        _glBuf = _gl.createBuffer();
        _gl.bindBuffer(_gl.ARRAY_BUFFER, _glBuf);
        _gl.bufferData(_gl.ARRAY_BUFFER, verts, _gl.STATIC_DRAW);

        var aPos = _gl.getAttribLocation(_glProg, 'a_pos');
        var aUv  = _gl.getAttribLocation(_glProg, 'a_uv');
        _gl.enableVertexAttribArray(aPos);
        _gl.enableVertexAttribArray(aUv);
        _gl.vertexAttribPointer(aPos, 2, _gl.FLOAT, false, 16, 0);
        _gl.vertexAttribPointer(aUv,  2, _gl.FLOAT, false, 16, 8);

        // Set uniforms once -- reused for every upload.
        _gl.uniform1i(_gl.getUniformLocation(_glProg, 'u_tex'),    0);
        _gl.uniform1f(_gl.getUniformLocation(_glProg, 'u_premul'), 0);

        // Dummy 1x1 texture + dummy draw: forces the driver to finish any
        // deferred GLSL compilation NOW during idle page-load, not on first scroll.
        var dummyTex = _gl.createTexture();
        _gl.bindTexture(_gl.TEXTURE_2D, dummyTex);
        _gl.texImage2D(_gl.TEXTURE_2D, 0, _gl.RGBA, 1, 1, 0,
                       _gl.RGBA, _gl.UNSIGNED_BYTE, new Uint8Array([0, 0, 0, 0]));
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_S, _gl.CLAMP_TO_EDGE);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_T, _gl.CLAMP_TO_EDGE);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MIN_FILTER, _gl.LINEAR);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MAG_FILTER, _gl.LINEAR);
        _gl.viewport(0, 0, 1, 1);
        _gl.drawArrays(_gl.TRIANGLE_STRIP, 0, 4);  // driver must compile here
        _gl.flush();
        _gl.deleteTexture(dummyTex);

        _backend = 'webgl' + _glver;
        _ready   = true;
        return true;
    }

    function _glUpload(url, bitmap) {
        if (_glCache.has(url)) { _lruTouch(url); return; }
        var w = Math.min(bitmap.width  || 1, 4096);
        var h = Math.min(bitmap.height || 1, 4096);
        var tex = _gl.createTexture();
        _gl.bindTexture(_gl.TEXTURE_2D, tex);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_S, _gl.CLAMP_TO_EDGE);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_T, _gl.CLAMP_TO_EDGE);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MIN_FILTER, _gl.LINEAR);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MAG_FILTER, _gl.LINEAR);
        try {
            _gl.texImage2D(_gl.TEXTURE_2D, 0, _gl.RGBA, _gl.RGBA,
                           _gl.UNSIGNED_BYTE, bitmap);
        } catch (e) {
            _gl.deleteTexture(tex);
            return;
        }
        _gl.flush();  // one flush per upload, not per frame
        var bytes = w * h * 4;
        _glCache.set(url, tex);
        _cache.set(url, { bytes: bytes });
        _lruList.push(url);
        _cacheBytes += bytes;
        _lruEvict(function (e_unused) {
            var oldUrl = _lruList[0];
            var oldTex = _glCache.get(oldUrl);
            if (oldTex) { _gl.deleteTexture(oldTex); _glCache.delete(oldUrl); }
        });
    }

    // ── Unified init (WebGPU first, GL fallback) ──────────────────────────────
    function _init() {
        if (_initProm) return _initProm;
        _initProm = _initWebGPU().then(function (ok) {
            if (!ok) _initWebGL();
        });
        return _initProm;
    }

    // ── Bitmap upload dispatcher ──────────────────────────────────────────────
    function _uploadBitmap(url, bitmap) {
        if (_backend === 'webgpu') { _gpuUpload(url, bitmap); }
        else if (_gl)              { _glUpload(url, bitmap);  }
    }

    // ── createImageBitmap fetch path ──────────────────────────────────────────
    function _preloadUrl(url) {
        if (!url || url.startsWith('data:') ||
            _cache.has(url) || _inFlight.has(url)) return;
        _inFlight.add(url);

        function _doUpload() {
            var img = new Image();
            img.crossOrigin = 'anonymous';
            img.onload = function () {
                if (typeof createImageBitmap === 'function') {
                    createImageBitmap(img, {
                        premultiplyAlpha: 'premultiply',
                        colorSpaceConversion: 'none',
                    }).then(function (bm) {
                        _uploadBitmap(url, bm);
                        bm.close();
                        _inFlight.delete(url);
                    }).catch(function () { _inFlight.delete(url); });
                } else {
                    _uploadBitmap(url, img);
                    _inFlight.delete(url);
                }
            };
            img.onerror = function () { _inFlight.delete(url); };
            img.src = url;
        }

        if (_ready) { _doUpload(); }
        else { _init().then(_doUpload); }
    }

    // ── IntersectionObserver: images 2 viewports ahead ───────────────────────
    var _io = new IntersectionObserver(function (entries) {
        for (var i = 0; i < entries.length; i++) {
            if (!entries[i].isIntersecting) continue;
            var el  = entries[i].target;
            var src = el.currentSrc || el.src || el.getAttribute('src') || '';
            var proxy = el.dataset && el.dataset.xcmProxySrc;
            _preloadUrl(proxy || src);
        }
    }, { rootMargin: '200% 0px', threshold: 0 });

    function _observeImg(img) {
        if (img.__xcmGpuObserved__) return;
        img.__xcmGpuObserved__ = true;
        _io.observe(img);
    }

    function _scanImages(root) {
        var imgs = (root || document).querySelectorAll('img[src],img[data-src]');
        for (var i = 0; i < imgs.length; i++) _observeImg(imgs[i]);
    }

    var _mo = new MutationObserver(function (records) {
        for (var i = 0; i < records.length; i++) {
            var added = records[i].addedNodes;
            for (var j = 0; j < added.length; j++) {
                var n = added[j];
                if (n.nodeType !== 1) continue;
                if (n.tagName === 'IMG') { _observeImg(n); continue; }
                if (n.querySelectorAll) {
                    var imgs = n.querySelectorAll('img');
                    for (var k = 0; k < imgs.length; k++) _observeImg(imgs[k]);
                }
            }
            if (records[i].type === 'attributes'
                && records[i].target.tagName === 'IMG') {
                _observeImg(records[i].target);
            }
        }
    });

    function _startObserving() {
        if (document.body) {
            _scanImages(document);
            _mo.observe(document.body, {
                childList: true, subtree: true,
                attributes: true, attributeFilter: ['src', 'data-src'],
            });
        } else {
            requestAnimationFrame(_startObserving);
        }
    }

    // ── Spector-style frame capture ───────────────────────────────────────────
    function _captureFrame() {
        return new Promise(function (resolve) {
            if (_backend === 'webgpu') {
                var t = performance.now();
                requestAnimationFrame(function () {
                    resolve({ backend: 'webgpu',
                              frameTime: (performance.now() - t).toFixed(2),
                              snapshot: gpu.snapshot() });
                });
                return;
            }
            if (!_gl) { resolve({ error: 'no backend', calls: [] }); return; }
            var _calls = [], _orig = {}, _active = true;
            for (var k in _gl) {
                if (typeof _gl[k] !== 'function') continue;
                (function (name) {
                    _orig[name] = _gl[name].bind(_gl);
                    _gl[name]   = function () {
                        var args = Array.prototype.slice.call(arguments).map(function (a) {
                            if (a === null)                     return null;
                            if (a instanceof WebGLTexture)      return '[Texture]';
                            if (a instanceof WebGLBuffer)       return '[Buffer]';
                            if (a instanceof WebGLProgram)      return '[Program]';
                            if (a instanceof WebGLFramebuffer)  return '[Framebuffer]';
                            if (a instanceof WebGLShader)       return '[Shader]';
                            if (ArrayBuffer.isView && ArrayBuffer.isView(a))
                                return a.constructor.name + '(' + a.length + ')';
                            return a;
                        });
                        if (_active) _calls.push({ fn: name, args: args });
                        return _orig[name].apply(_gl, arguments);
                    };
                })(k);
            }
            requestAnimationFrame(function () {
                _active = false;
                for (var k2 in _orig) { try { _gl[k2] = _orig[k2]; } catch (_) {} }
                resolve({ backend: _backend, callCount: _calls.length, calls: _calls });
            });
        });
    }

    // ── Public API ────────────────────────────────────────────────────────────
    var gpu = {
        init:         function () { return _init(); },
        preload:      function (url) { _preloadUrl(url); },
        has:          function (url) { return _cache.has(url); },
        cacheSize:    function () { return _cache.size; },
        cacheBytes:   function () { return _cacheBytes; },
        captureFrame: _captureFrame,

        flush: function () {
            if (_backend === 'webgpu') {
                _cache.forEach(function (e) { if (e.tex) e.tex.destroy(); });
            } else if (_gl) {
                _glCache.forEach(function (tex) { _gl.deleteTexture(tex); });
                _glCache.clear();
            }
            _cache.clear(); _lruList.length = 0; _cacheBytes = 0;
        },

        snapshot: function () {
            return {
                backend:    _backend,
                ready:      _ready,
                textures:   _cache.size,
                cacheBytes: _cacheBytes,
                maxBytes:   MAX_CACHE_BYTES,
                inFlight:   _inFlight.size,
            };
        },

        get backend()   { return _backend; },
        get glVersion() { return _glver; },
    };

    if (global.__xcm) { global.__xcm.gpu = gpu; }
    else              { global.__xcmGpu  = gpu; }

    // Init eagerly (shader compilation + pipeline pre-warm during page load).
    _init().then(function () { _startObserving(); });

})(window);
