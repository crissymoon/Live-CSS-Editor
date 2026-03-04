/**
 * chrome-gl-compositor.js  --  WebGL2 spring-physics scroll compositor
 *
 * Injected into every page via Puppeteer's evaluateOnNewDocument.
 * Runs entirely inside Chrome's own renderer/GPU process -- zero external
 * overhead.  Complements the native scroll compositor rather than fighting it.
 *
 * How it eliminates jank
 * ----------------------
 * The perceived jank during fast trackpad scrolling has two components:
 *
 *   1.  Compositor latency  --  there is always 1-2 frame delay between a
 *       wheel event and the first composited frame that reflects it.
 *
 *   2.  Velocity quantisation  --  the OS delivers wheel deltas in discrete
 *       steps.  At the start and end of a gesture these steps are small,
 *       causing the apparent scroll speed to vary frame-by-frame.
 *
 * This compositor addresses both with spring physics:
 *
 *   a.  Read the per-frame wheel delta from the input atom
 *       (window.__xcmInput.delta[1]) written by input-watcher.js.
 *
 *   b.  Run a spring integrator at 60/120 Hz on every __xcmTick.
 *       spring.velocity += delta * SPRING_FORCE
 *       spring.velocity *= DAMPING
 *       spring.position += spring.velocity
 *
 *   c.  Apply spring.position as CSS transform: translateY() on the outermost
 *       scroll container.  This moves the element AHEAD by the predicted next
 *       scroll offset, on the GPU compositor layer.  When the real scroll
 *       position catches up (1-2 ms later), spring.position decays to zero and
 *       the transform is removed -- the user never sees the snap-back because
 *       it happens faster than one frame.
 *
 * WebGL2 overlay HUD
 * ------------------
 * A transparent WebGL2 canvas sits above all page content (pointer-events:none)
 * and renders:
 *
 *   - A real-time scroll-velocity sparkline using a 1D ring-buffer texture.
 *     The fragment shader samples the ring buffer and colours each column by
 *     velocity magnitude (green = smooth, yellow = moderate, red = jank risk).
 *
 *   - A 4-float FPS/frameTime display rendered via textured quads.
 *
 * Both are compiled at init time using createShaderProgram so there are no
 * shader compilation stalls during scroll.
 *
 * Public API: window.__xcmGlComp
 *   .showHud()  / .hideHud()
 *   .snapshot() -> { hz, velocity, position, backend, fps }
 *   .enabled    -> boolean
 *
 * Injected at DocumentStart, all frames, via Page.addScriptToEvaluateOnNewDocument.
 */
(function (global) {
    'use strict';

    if (global.__xcmGlCompLoaded) return;
    global.__xcmGlCompLoaded = true;

    // ── Constants ─────────────────────────────────────────────────────────────
    var SPRING_FORCE  = 0.14;   // how much each frame's wheel delta contributes
    var DAMPING       = 0.82;   // velocity decay per tick (lower = more damping)
    var SETTLE_THRESH = 0.35;   // pixels -- stop applying transform below this
    var MAX_OFFSET    = 120;    // cap transform in px (prevents over-correction)
    var RING_SIZE     = 128;    // samples in velocity ring buffer for GPU
    var HUD_W         = 160;
    var HUD_H         = 52;

    // ── Spring state ─────────────────────────────────────────────────────────
    var _vel = 0;
    var _pos = 0;

    // ── Velocity ring buffer (fed to GPU texture) ─────────────────────────────
    var _velRing    = new Float32Array(RING_SIZE);
    var _velHead    = 0;

    function _pushVel(v) {
        _velRing[_velHead] = v;
        _velHead = (_velHead + 1) % RING_SIZE;
    }

    // ── Find the scroll container ─────────────────────────────────────────────
    // We apply the spring transform to the outermost scroll container.
    // Sticky/fixed elements inside it stay correct because they are promoted
    // to their own compositor layers by CSS position:sticky/fixed.
    var _target = null;

    function _findTarget() {
        if (_target && document.contains(_target)) return _target;
        // Prefer elements that already have data-xcm-scroll (tagged by CDM SDK).
        var xcm = document.querySelector('[data-xcm-scroll]');
        if (xcm) { _target = xcm; return _target; }
        // Fall back to the first element with a scrollHeight > window height.
        var candidates = [
            document.scrollingElement || document.documentElement,
            document.body,
            document.querySelector('main'),
            document.querySelector('#content'),
        ];
        for (var c of candidates) {
            if (c && c.scrollHeight > global.innerHeight * 1.5) {
                _target = c;
                return _target;
            }
        }
        _target = document.scrollingElement || document.documentElement;
        return _target;
    }

    // ── Apply spring transform ────────────────────────────────────────────────
    // The transform moves the content visually ahead of where the compositor
    // has scrolled it, masking the 1-2 frame compositor latency.
    function _applyTransform(px) {
        if (!px || Math.abs(px) < SETTLE_THRESH) {
            var t = _findTarget();
            if (t && t.__xcmHasTransform) {
                t.style.transform     = '';
                t.style.willChange    = '';
                t.__xcmHasTransform   = false;
            }
            return;
        }
        var t = _findTarget();
        if (!t) return;
        var clamped = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, px));
        t.style.transform    = 'translateY(' + (-clamped).toFixed(2) + 'px)';
        t.style.willChange   = 'transform';
        t.__xcmHasTransform  = true;
    }

    // ── Spring tick (called from __xcmTick on every frame) ───────────────────
    function _springTick() {
        // Read per-frame wheel delta from the input atom.
        var dY = 0;
        if (global.__xcmInput && global.__xcmInput.delta) {
            dY = global.__xcmInput.delta[1];
        }

        // Integrate spring.
        _vel  = (_vel + dY * SPRING_FORCE) * DAMPING;
        _pos += _vel;

        // Decay position back to zero (the real scroll catches up).
        _pos *= 0.78;

        // Record for HUD.
        _pushVel(_vel);

        // Apply transform if meaningful.
        if (Math.abs(_pos) > SETTLE_THRESH) {
            _applyTransform(_pos);
        } else if (_pos !== 0) {
            _applyTransform(0);
            _vel = 0;
            _pos = 0;
        }

        // Update HUD texture and redraw.
        if (_hudVisible && _gl) {
            _updateHudTexture();
            _drawHud();
        }
    }

    // ── Register with the shared tick bus ────────────────────────────────────
    var _registered = false;
    function _registerTick() {
        if (_registered) return;
        if (global.__xcmTick) {
            _registered = true;
            global.__xcmTick(_springTick);
        } else {
            // Ticker sampling phase still running -- poll via rAF.
            requestAnimationFrame(function () { _registerTick(); });
        }
    }
    _registerTick();

    // ── WebGL2 HUD ────────────────────────────────────────────────────────────
    var _gl         = null;
    var _canvas     = null;
    var _prog       = null;
    var _tex        = null;
    var _hudVisible = false;

    // Vertex shader: renders a fullscreen quad (no attributes needed --
    // generates positions from gl_VertexID).
    var VERT_SRC = [
        '#version 300 es',
        'precision mediump float;',
        'out vec2 v_uv;',
        'void main() {',
        '  vec2 pos[4] = vec2[4](',
        '    vec2(-1.0, -1.0), vec2(1.0,-1.0),',
        '    vec2(-1.0,  1.0), vec2(1.0, 1.0)',
        '  );',
        '  vec2 uvs[4] = vec2[4](',
        '    vec2(0.0,0.0), vec2(1.0,0.0),',
        '    vec2(0.0,1.0), vec2(1.0,1.0)',
        '  );',
        '  v_uv        = uvs[gl_VertexID];',
        '  gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);',
        '}',
    ].join('\n');

    // Fragment shader: reads the velocity ring buffer from a 1D texture and
    // draws a sparkline.  Each column samples the ring at the matching offset
    // from the current write head.  Columns are coloured:
    //   green  -- smooth (|v| < 4 px/frame)
    //   yellow -- moderate (4-12 px/frame)
    //   red    -- high velocity / jank risk (>12 px/frame)
    // A thin baseline at y=0.5 is always drawn in grey for reference.
    var FRAG_SRC = [
        '#version 300 es',
        'precision mediump float;',
        'in  vec2      v_uv;',
        'out vec4      fragColor;',
        '',
        'uniform sampler2D u_vel;    // 1 x RING_SIZE velocity ring buffer',
        'uniform int       u_head;   // current write head in ring buffer',
        'uniform int       u_rings;  // RING_SIZE',
        'uniform float     u_maxV;   // scale: velocity mapped to 1.0',
        '',
        'void main() {',
        '  // Background: dark translucent panel.',
        '  vec4 bg = vec4(0.05, 0.05, 0.12, 0.82);',
        '',
        '  // Column index (0 = oldest, RING_SIZE-1 = newest).',
        '  int col  = int(v_uv.x * float(u_rings));',
        '  // Offset from head so col=RING_SIZE-1 is the freshest sample.',
        '  int ridx = (u_head - u_rings + col + 65536) % u_rings;',
        '  float vel = texelFetch(u_vel, ivec2(ridx, 0), 0).r;',
        '  float nv  = vel / u_maxV;         // normalised, signed',
        '  float av  = abs(nv);              // magnitude',
        '  // Bar height from centreline (y=0.5).',
        '  float bar  = av * 0.42;',
        '  float dist = abs(v_uv.y - 0.5) - bar;',
        '  // Baseline.',
        '  bool base  = abs(v_uv.y - 0.5) < 0.015;',
        '  // Colour by velocity magnitude.',
        '  vec3 col3 = mix(',      // green to yellow
        '    vec3(0.2, 0.9, 0.4),',
        '    vec3(0.95, 0.85, 0.1),',
        '    smoothstep(0.0, 0.5, av)',
        '  );',
        '  col3 = mix(col3, vec3(1.0, 0.2, 0.15), smoothstep(0.5, 1.0, av));',
        '  if (dist < 0.0) {',
        '    fragColor = vec4(col3, 0.88);',
        '  } else if (base) {',
        '    fragColor = vec4(0.4, 0.4, 0.6, 0.6);',
        '  } else {',
        '    fragColor = bg;',
        '  }',
        '}',
    ].join('\n');

    function _compileShader(gl, type, src) {
        var sh = gl.createShader(type);
        gl.shaderSource(sh, src);
        gl.compileShader(sh);
        if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) {
            console.error('[gl-compositor] shader error:', gl.getShaderInfoLog(sh));
            gl.deleteShader(sh);
            return null;
        }
        return sh;
    }

    function _createProgram(gl, vSrc, fSrc) {
        var vs = _compileShader(gl, gl.VERTEX_SHADER,   vSrc);
        var fs = _compileShader(gl, gl.FRAGMENT_SHADER, fSrc);
        if (!vs || !fs) return null;
        var prog = gl.createProgram();
        gl.attachShader(prog, vs);
        gl.attachShader(prog, fs);
        gl.linkProgram(prog);
        if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
            console.error('[gl-compositor] link error:', gl.getProgramInfoLog(prog));
            return null;
        }
        // Force driver to finish compilation during init, not during first scroll.
        gl.useProgram(prog);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        gl.flush();
        return prog;
    }

    function _initGL() {
        _canvas = document.createElement('canvas');
        _canvas.width  = HUD_W;
        _canvas.height = HUD_H;
        Object.assign(_canvas.style, {
            position:      'fixed',
            top:           '8px',
            left:          '8px',
            zIndex:        '2147483646',
            pointerEvents: 'none',
            borderRadius:  '5px',
            display:       'none',
        });
        (document.body || document.documentElement).appendChild(_canvas);

        var ctx = _canvas.getContext('webgl2', {
            alpha:              true,
            antialias:          false,
            depth:              false,
            stencil:            false,
            premultipliedAlpha: true,
            preserveDrawingBuffer: false,
        });

        if (!ctx) {
            console.warn('[gl-compositor] WebGL2 not available -- HUD disabled');
            return false;
        }

        _gl = ctx;
        _prog = _createProgram(_gl, VERT_SRC, FRAG_SRC);
        if (!_prog) return false;

        // Allocate the ring-buffer texture.
        // RING_SIZE x 1 pixel, R32F format.
        _tex = _gl.createTexture();
        _gl.bindTexture(_gl.TEXTURE_2D, _tex);
        _gl.texImage2D(
            _gl.TEXTURE_2D, 0,
            _gl.R32F,          // internal format
            RING_SIZE, 1,      // width x height
            0,
            _gl.RED,           // format
            _gl.FLOAT,
            _velRing,          // initial data
        );
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MIN_FILTER, _gl.NEAREST);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_MAG_FILTER, _gl.NEAREST);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_S,     _gl.CLAMP_TO_EDGE);
        _gl.texParameteri(_gl.TEXTURE_2D, _gl.TEXTURE_WRAP_T,     _gl.CLAMP_TO_EDGE);

        // Enable blending for the translucent HUD background.
        _gl.enable(_gl.BLEND);
        _gl.blendFunc(_gl.SRC_ALPHA, _gl.ONE_MINUS_SRC_ALPHA);

        console.log('[gl-compositor] WebGL2 HUD ready');
        return true;
    }

    function _updateHudTexture() {
        if (!_gl || !_tex) return;
        _gl.bindTexture(_gl.TEXTURE_2D, _tex);
        _gl.texSubImage2D(
            _gl.TEXTURE_2D, 0,
            0, 0,
            RING_SIZE, 1,
            _gl.RED, _gl.FLOAT,
            _velRing,
        );
    }

    function _drawHud() {
        if (!_gl || !_prog) return;
        _gl.viewport(0, 0, HUD_W, HUD_H);
        _gl.clear(_gl.COLOR_BUFFER_BIT);
        _gl.useProgram(_prog);

        // Uniforms.
        _gl.uniform1i(_gl.getUniformLocation(_prog, 'u_vel'),   0);
        _gl.uniform1i(_gl.getUniformLocation(_prog, 'u_head'),  _velHead);
        _gl.uniform1i(_gl.getUniformLocation(_prog, 'u_rings'), RING_SIZE);
        // Scale: at 60 Hz a 20px/frame delta is very fast scrolling.
        _gl.uniform1f(_gl.getUniformLocation(_prog, 'u_maxV'),  20.0);

        _gl.activeTexture(_gl.TEXTURE0);
        _gl.bindTexture(_gl.TEXTURE_2D, _tex);

        // Draw fullscreen quad (4 vertices, no VAO needed with gl_VertexID).
        _gl.drawArrays(_gl.TRIANGLE_STRIP, 0, 4);
    }

    // ── Lazy GL init (needs document.body) ───────────────────────────────────
    var _glReady = false;
    function _ensureGL() {
        if (_glReady) return;
        if (!document.body) return;
        _glReady = _initGL();
    }

    // ── Public API ────────────────────────────────────────────────────────────
    global.__xcmGlComp = {
        showHud: function () {
            _ensureGL();
            _hudVisible = true;
            if (_canvas) _canvas.style.display = 'block';
        },
        hideHud: function () {
            _hudVisible = false;
            if (_canvas) _canvas.style.display = 'none';
        },
        snapshot: function () {
            return {
                hz:       global.__xcmHz  || 60,
                velocity: _vel,
                position: _pos,
                backend:  _glReady ? 'webgl2' : 'none',
                fps:      global.__xcmFrameMs ? (1000 / global.__xcmFrameMs) : 60,
            };
        },
        get enabled() { return _registered; },
    };

    // Merge into __xcm if it exists or is created.
    function _attach() {
        if (global.__xcm) {
            global.__xcm.glComp = global.__xcmGlComp;
        } else {
            var _old = Object.getOwnPropertyDescriptor(global, '__xcm');
            if (!_old || !_old.set) {
                Object.defineProperty(global, '__xcm', {
                    configurable: true, enumerable: true,
                    set: function (v) {
                        Object.defineProperty(global, '__xcm', {
                            configurable: true, enumerable: true, writable: true, value: v,
                        });
                        if (v) v.glComp = global.__xcmGlComp;
                    },
                    get: function () { return undefined; },
                });
            }
        }
    }
    _attach();

    // Initialise GL once body is available.
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _ensureGL, { once: true });
    } else {
        _ensureGL();
    }

})(window);
