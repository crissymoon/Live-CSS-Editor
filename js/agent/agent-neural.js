/**
 * agent-neural.js -- Neural-network canvas animation overlay for the agent.
 * Shows while the agent is working (busy=true). Cycles through conversational
 * phrases fetched from ai/phrases.php which calls GPT-4o-mini on first use and
 * stores results in SQLite so the phrase pool grows over time.
 *
 * Exposed on C:
 *   C.neuralStart()   -- fade in, begin animation + phrase cycling
 *   C.neuralStop()    -- fade out, clean up canvas
 *   C.neuralPreload() -- warm up phrase cache in the background
 */
'use strict';

(function (LiveCSS) {

    var C = LiveCSS._agentCore;

    // -----------------------------------------------------------------------
    // Phrase cache
    // -----------------------------------------------------------------------

    var PHRASES    = [
        'Analyzing your design structure...',
        'Mapping the component hierarchy...',
        'Weaving the CSS threads together...',
        'Exploring layout possibilities...',
        'Crafting pixel-perfect adjustments...',
        'Running pattern recognition...',
        'Synthesizing style rules...',
        'Tracing the cascade hierarchy...',
        'Building the response...',
        'Inspecting component boundaries...',
        'Calculating visual weight...',
        'Refining the color harmony...',
        'Indexing style properties...',
        'Discovering optimization paths...',
        'Assembling the solution...',
        'Threading through selectors...',
        'Connecting design tokens...',
        'Resolving inherited values...',
        'Checking responsive breakpoints...',
        'Painting the final output...'
    ];
    var LOADED     = false;
    var LOADING    = false;
    var PHRASE_URL = 'ai/phrases.php?action=get';

    function loadPhrases(cb) {
        if (LOADED && PHRASES.length > 20) { if (cb) { cb(); } return; }
        if (LOADING) { if (cb) { cb(); } return; }
        LOADING = true;
        fetch(PHRASE_URL)
            .then(function (r) { return r.json(); })
            .then(function (data) {
                var raw = data.phrases || [];
                if (raw.length > 0) { PHRASES = raw; LOADED = true; }
                LOADING = false;
                if (cb) { cb(); }
            })
            .catch(function () { LOADING = false; if (cb) { cb(); } });
    }

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    var canvas      = null;
    var ctx         = null;
    var overlay     = null;
    var rafId       = null;
    var running     = false;
    var nodes       = [];
    var signals     = [];
    var NODE_COUNT  = 20;
    var CONNECT_D   = 145;
    var SIG_INT     = 300;
    var sigTimer    = 0;
    var lastTime    = 0;

    // phrase cycling
    var pIdx        = 0;
    var pAlpha      = 0;
    var pState      = 'in';   // 'in' | 'hold' | 'out'
    var pTimer      = 0;
    var P_HOLD      = 2400;
    var P_FADE      = 400;

    // Reference to the source container so the overlay can track its position
    var anchorEl = null;

    // -----------------------------------------------------------------------
    // DOM helpers
    // -----------------------------------------------------------------------

    function createOverlay(container) {
        anchorEl = container;
        overlay = document.createElement('div');
        overlay.className = 'agent-neural-overlay';
        canvas = document.createElement('canvas');
        canvas.className = 'agent-neural-canvas';
        overlay.appendChild(canvas);
        // Append to document.body so scrolling the responseArea never moves it
        document.body.appendChild(overlay);
        ctx = canvas.getContext('2d');
        alignOverlay();
    }

    function alignOverlay() {
        if (!overlay || !anchorEl) { return; }
        var r = anchorEl.getBoundingClientRect();
        overlay.style.left   = r.left   + 'px';
        overlay.style.top    = r.top    + 'px';
        overlay.style.width  = r.width  + 'px';
        overlay.style.height = r.height + 'px';
        canvas.width  = r.width  || 400;
        canvas.height = r.height || 300;
    }

    function resize() {
        alignOverlay();
    }

    function dropOverlay() {
        if (overlay && overlay.parentNode) { overlay.parentNode.removeChild(overlay); }
        overlay = null; canvas = null; ctx = null; anchorEl = null;
    }

    // -----------------------------------------------------------------------
    // Node init
    // -----------------------------------------------------------------------

    function initNodes() {
        var w = canvas.width; var h = canvas.height;
        nodes = []; signals = [];
        for (var i = 0; i < NODE_COUNT; i++) {
            nodes.push({
                x:     14 + Math.random() * (w - 28),
                y:     14 + Math.random() * (h - 28),
                vx:    (Math.random() - 0.5) * 0.30,
                vy:    (Math.random() - 0.5) * 0.30,
                r:     3 + Math.random() * 4,
                pulse: Math.random() * Math.PI * 2,
                spd:   0.012 + Math.random() * 0.018,
                glow:  0.45 + Math.random() * 0.55
            });
        }
    }

    // -----------------------------------------------------------------------
    // Edge computation (cached per frame)
    // -----------------------------------------------------------------------

    function getEdges() {
        var e = [];
        for (var i = 0; i < nodes.length; i++) {
            for (var j = i + 1; j < nodes.length; j++) {
                var dx = nodes[j].x - nodes[i].x;
                var dy = nodes[j].y - nodes[i].y;
                var d  = Math.sqrt(dx * dx + dy * dy);
                if (d < CONNECT_D) { e.push({ a: i, b: j, d: d }); }
            }
        }
        return e;
    }

    // -----------------------------------------------------------------------
    // Spawn a signal pulse along a random edge
    // -----------------------------------------------------------------------

    function spawnSig(edges) {
        if (!edges.length) { return; }
        var e   = edges[Math.floor(Math.random() * edges.length)];
        var fwd = Math.random() < 0.5;
        signals.push({
            a:    fwd ? e.a : e.b,
            b:    fwd ? e.b : e.a,
            t:    0,
            spd:  0.005 + Math.random() * 0.009,
            alp:  0.95,
            sz:   2.5 + Math.random() * 2.5
        });
    }

    // -----------------------------------------------------------------------
    // Draw one frame
    // -----------------------------------------------------------------------

    function draw(edges) {
        var w = canvas.width; var h = canvas.height;
        ctx.clearRect(0, 0, w, h);

        // edges
        edges.forEach(function (e) {
            var a   = nodes[e.a]; var b = nodes[e.b];
            var opa = (1 - e.d / CONNECT_D) * 0.38;
            ctx.beginPath();
            ctx.moveTo(a.x, a.y);
            ctx.lineTo(b.x, b.y);
            ctx.strokeStyle = 'rgba(108,72,210,' + opa + ')';
            ctx.lineWidth   = 0.85;
            ctx.stroke();
        });

        // nodes
        nodes.forEach(function (n) {
            var p  = (Math.sin(n.pulse) + 1) / 2;
            var gr = n.r + p * n.r * 1.2;
            var grd = ctx.createRadialGradient(n.x, n.y, 0, n.x, n.y, gr * 3.5);
            grd.addColorStop(0,   'rgba(130,90,240,' +  (n.glow * 0.48) + ')');
            grd.addColorStop(0.5, 'rgba(90,50,190,'  +  (n.glow * 0.18) + ')');
            grd.addColorStop(1,   'rgba(60,30,140,0)');
            ctx.beginPath();
            ctx.arc(n.x, n.y, gr * 3.5, 0, Math.PI * 2);
            ctx.fillStyle = grd; ctx.fill();

            ctx.beginPath();
            ctx.arc(n.x, n.y, n.r * 0.75 + p * 1.6, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(168,130,255,' + (0.72 + p * 0.28) + ')';
            ctx.fill();
        });

        // signals
        signals.forEach(function (s) {
            var a  = nodes[s.a]; var b = nodes[s.b];
            var sx = a.x + (b.x - a.x) * s.t;
            var sy = a.y + (b.y - a.y) * s.t;
            var sg = ctx.createRadialGradient(sx, sy, 0, sx, sy, s.sz * 2.5);
            sg.addColorStop(0,   'rgba(215,180,255,' + s.alp + ')');
            sg.addColorStop(0.5, 'rgba(160,100,255,' + (s.alp * 0.55) + ')');
            sg.addColorStop(1,   'rgba(120,60,220,0)');
            ctx.beginPath();
            ctx.arc(sx, sy, s.sz * 2.5, 0, Math.PI * 2);
            ctx.fillStyle = sg; ctx.fill();
            ctx.beginPath();
            ctx.arc(sx, sy, s.sz * 0.5, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(245,230,255,' + s.alp + ')';
            ctx.fill();
        });

        // phrase
        if (pAlpha > 0.015) {
            var phrase = PHRASES.length ? PHRASES[pIdx % PHRASES.length] : 'Processing...';
            var fs     = Math.max(11, Math.min(14, w / 32));
            ctx.font   = '300 ' + fs + 'px -apple-system, system-ui, sans-serif';
            ctx.textAlign    = 'center';
            ctx.textBaseline = 'middle';
            var tw  = ctx.measureText(phrase).width;
            var px  = w / 2; var py = h / 2;
            var pad = 12;
            var rx  = px - tw / 2 - pad;
            var ry  = py - fs * 0.8;
            var rw  = tw + pad * 2;
            var rh  = fs * 1.6;
            var r6  = 6;
            ctx.fillStyle = 'rgba(10,5,24,' + (pAlpha * 0.78) + ')';
            ctx.beginPath();
            if (ctx.roundRect) {
                ctx.roundRect(rx, ry, rw, rh, r6);
            } else {
                ctx.moveTo(rx + r6, ry);
                ctx.lineTo(rx + rw - r6, ry);
                ctx.arcTo(rx + rw, ry, rx + rw, ry + r6, r6);
                ctx.lineTo(rx + rw, ry + rh - r6);
                ctx.arcTo(rx + rw, ry + rh, rx + rw - r6, ry + rh, r6);
                ctx.lineTo(rx + r6, ry + rh);
                ctx.arcTo(rx, ry + rh, rx, ry + rh - r6, r6);
                ctx.lineTo(rx, ry + r6);
                ctx.arcTo(rx, ry, rx + r6, ry, r6);
                ctx.closePath();
            }
            ctx.fill();
            ctx.fillStyle = 'rgba(215,195,255,' + (pAlpha * 0.94) + ')';
            ctx.fillText(phrase, px, py);
        }
    }

    // -----------------------------------------------------------------------
    // Animation loop
    // -----------------------------------------------------------------------

    function tick(ts) {
        if (!running) { return; }
        var dt = ts - (lastTime || ts);
        lastTime = ts;
        if (dt > 80) { dt = 80; }

        var w = canvas.width; var h = canvas.height;
        nodes.forEach(function (n) {
            n.x += n.vx; n.y += n.vy; n.pulse += n.spd;
            if (n.x < 10 || n.x > w - 10) { n.vx *= -1; }
            if (n.y < 10 || n.y > h - 10) { n.vy *= -1; }
        });

        var edges = getEdges();

        sigTimer += dt;
        if (sigTimer >= SIG_INT) {
            sigTimer = 0; spawnSig(edges);
            if (Math.random() < 0.40) { spawnSig(edges); }
        }

        signals = signals.filter(function (s) {
            s.t += s.spd;
            if (s.t > 0.82) { s.alp = Math.max(0, s.alp - 0.05); }
            return s.t < 1.0 && s.alp > 0.01;
        });

        // phrase state machine
        pTimer += dt;
        if (pState === 'in') {
            pAlpha = Math.min(1, pAlpha + dt / P_FADE);
            if (pAlpha >= 1) { pState = 'hold'; pTimer = 0; }
        } else if (pState === 'hold') {
            if (pTimer >= P_HOLD) { pState = 'out'; pTimer = 0; }
        } else {
            pAlpha = Math.max(0, pAlpha - dt / P_FADE);
            if (pAlpha <= 0) {
                pIdx = (pIdx + 1) % Math.max(1, PHRASES.length);
                pState = 'in'; pTimer = 0;
            }
        }

        draw(edges);
        rafId = requestAnimationFrame(tick);
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    C.neuralPreload = function () { loadPhrases(null); };

    // Keep overlay aligned if the window resizes while busy
    function onWindowResize() { alignOverlay(); }

    C.neuralStart = function () {
        if (running) { return; }
        var container = C.dom.responseArea;
        if (!container) { return; }
        loadPhrases(function () {}); // ensure cache warms

        pIdx = PHRASES.length ? Math.floor(Math.random() * PHRASES.length) : 0;
        pAlpha = 0; pState = 'in'; pTimer = 0; sigTimer = 0;

        createOverlay(container);
        initNodes();
        running = true; lastTime = 0;
        window.addEventListener('resize', onWindowResize);
        setTimeout(function () {
            alignOverlay(); // re-align after layout settles
            overlay && overlay.classList.add('neural-visible');
        }, 30);
        rafId = requestAnimationFrame(tick);
    };

    C.neuralStop = function () {
        if (!running) { return; }
        running = false;
        if (rafId) { cancelAnimationFrame(rafId); rafId = null; }
        window.removeEventListener('resize', onWindowResize);
        var o = overlay;
        if (o) {
            o.classList.remove('neural-visible');
            setTimeout(function () {
                if (o && o.parentNode) { o.parentNode.removeChild(o); }
                if (overlay === o) { overlay = null; canvas = null; ctx = null; anchorEl = null; }
            }, 450);
        }
    };

}(window.LiveCSS = window.LiveCSS || {}));