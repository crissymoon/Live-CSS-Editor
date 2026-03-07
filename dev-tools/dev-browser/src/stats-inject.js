/**
 * stats-inject.js -- Frame-budget and rendering statistics monitor.
 *
 * Measures real rAF frame intervals, JS task cost (via PerformanceObserver),
 * and long-task counts.  Reports to window.__xcm.stats.  Optionally renders
 * a miniature HUD overlay (toggled by window.__xcm.stats.showHud()).
 *
 * All work happens inside the rAF loop or PerformanceObserver callback --
 * no timers, no synchronous DOM reads during scroll.
 *
 * Injected at DocumentEnd, main frame only.
 */
(function (global) {
    'use strict';

    // Guard against double-injection.
    if (global.__xcmStatsLoaded__) return;
    global.__xcmStatsLoaded__ = true;

    // ── Ring-buffer helpers ──────────────────────────────────────────────────
    var RING = 120;  // store 120 samples (~2 s at 60 Hz)

    function Ring(n) {
        this._buf = new Float32Array(n);
        this._n   = n;
        this._i   = 0;
        this._len = 0;
    }
    Ring.prototype.push = function (v) {
        this._buf[this._i] = v;
        this._i = (this._i + 1) % this._n;
        if (this._len < this._n) this._len++;
    };
    Ring.prototype.last = function () {
        return this._buf[(this._i - 1 + this._n) % this._n];
    };
    Ring.prototype.min = function () {
        var m = Infinity;
        for (var i = 0; i < this._len; i++) if (this._buf[i] < m) m = this._buf[i];
        return m;
    };
    Ring.prototype.max = function () {
        var m = -Infinity;
        for (var i = 0; i < this._len; i++) if (this._buf[i] > m) m = this._buf[i];
        return m;
    };
    Ring.prototype.avg = function () {
        if (!this._len) return 0;
        var s = 0;
        for (var i = 0; i < this._len; i++) s += this._buf[i];
        return s / this._len;
    };

    // ── State ────────────────────────────────────────────────────────────────
    var _fpsRing     = new Ring(RING);
    var _ftRing      = new Ring(RING);  // frame time ms
    var _ltRing      = new Ring(RING);  // long-task ms
    var _last        = 0;
    var _ltTotal     = 0;
    var _ltCount     = 0;
    var _dropped     = 0;   // frames where ft > budget * 1.5
    var _rafId       = null;
    var _running     = false;
    var _unsubTick   = null;  // unsubscribe handle from __xcmTick
    var _budget      = 16;  // updated from ticker

    // ── rAF measurement loop ─────────────────────────────────────────────────
    // Plugs into the shared __xcmTick bus when available so this module does
    // not spin up a competing requestAnimationFrame listener.  Falls back to
    // its own rAF if the ticker has not initialised yet.
    function _tick(ts) {
        if (_last) {
            var ft  = ts - _last;
            // Sync budget from the ticker on every frame.
            if (global.__xcmFrameMs) _budget = global.__xcmFrameMs;

            _ftRing.push(ft);
            _fpsRing.push(1000 / ft);
            if (ft > _budget * 1.5) _dropped++;
        }
        _last = ts;
    }

    function _startLoop() {
        if (_running) return;
        _running = true;
        if (global.__xcmTick) {
            // Use shared rAF bus -- no extra requestAnimationFrame needed.
            _unsubTick = global.__xcmTick(_tick);
        } else {
            // Ticker not ready yet (sampling phase still running); retry once
            // the document is interactive and the ticker loop has started.
            function _rafFallback(ts) {
                _tick(ts);
                if (!_running) return;
                if (global.__xcmTick) {
                    // Ticker bus now available -- hand off, stop our own rAF.
                    _unsubTick = global.__xcmTick(_tick);
                    _rafId = null;
                } else {
                    _rafId = requestAnimationFrame(_rafFallback);
                }
            }
            _rafId = requestAnimationFrame(_rafFallback);
        }
    }

    function _stopLoop() {
        _running = false;
        if (_unsubTick) { _unsubTick(); _unsubTick = null; }
        if (_rafId)     { cancelAnimationFrame(_rafId); _rafId = null; }
    }

    _startLoop();

    // ── PerformanceObserver: long tasks ──────────────────────────────────────
    if (global.PerformanceObserver) {
        try {
            var _po = new PerformanceObserver(function (list) {
                var entries = list.getEntries();
                for (var i = 0; i < entries.length; i++) {
                    var d = entries[i].duration;
                    _ltTotal += d;
                    _ltCount++;
                    _ltRing.push(d);
                }
            });
            _po.observe({ type: 'longtask', buffered: true });
        } catch (_) {}
    }

    // ── HUD overlay ──────────────────────────────────────────────────────────
    var _hudEl    = null;
    var _hudTimer = null;
    var _hudVis   = false;

    // ── Bottom FPS sync bar ──────────────────────────────────────────────────
    // Always-visible bar at the bottom of the viewport.
    // Shows device FPS (rAF rate from the Hz sampler) vs engine FPS
    // (WASM render completions/s from window.__xcmWasmComp.engineFps())
    // so rendering sync is immediately visible.
    var _barEl    = null;
    var _barTimer = null;

    function _createBar() {
        var el = document.createElement('div');
        el.id = '__xcmFpsBar__';
        Object.assign(el.style, {
            position:       'fixed',
            bottom:         '0',
            left:           '0',
            right:          '0',
            zIndex:         '2147483647',
            background:     'rgba(0,0,0,0.72)',
            color:          '#e0e0e0',
            fontFamily:     'monospace',
            fontSize:       '11px',
            lineHeight:     '1',
            padding:        '4px 12px',
            display:        'flex',
            alignItems:     'center',
            gap:            '20px',
            pointerEvents:  'none',
            userSelect:     'none',
            letterSpacing:  '0.04em',
            borderTop:      '1px solid rgba(255,255,255,0.08)',
        });
        document.body.appendChild(el);
        return el;
    }

    // Returns a <span> coloured by the sync delta magnitude.
    function _syncSpan(devFps, engFps) {
        var diff   = Math.round(devFps - engFps);
        var sign   = diff >= 0 ? '+' : '';
        var color  = '#4caf50'; // green -- within 3 fps
        if (Math.abs(diff) >= 8)       color = '#f44336'; // red -- large gap
        else if (Math.abs(diff) >= 3)  color = '#ff9800'; // amber -- moderate gap
        return '<span style="color:' + color + '">SYNC ' + sign + diff + '</span>';
    }

    // ── Bar: throttled DOM update (500 ms interval, NOT per-frame rAF) ─────
    // Previous version set innerHTML on EVERY rAF frame, which was the single
    // biggest performance drain (~15-20 fps cost due to HTML parse + style
    // recalc + layout on every 16 ms frame).
    // Now uses pre-built spans with textContent updates, on a 500 ms timer.

    var _barSpans = null;   // {dev, avg, eng, sync, hz, drop}

    function _buildBarSpans() {
        var mkSpan = function (color, text) {
            var s = document.createElement('span');
            s.style.color = color;
            s.textContent = text;
            return s;
        };
        var sp = document.createDocumentFragment();
        var dev  = mkSpan('#90caf9', 'DEV -- fps');
        var avg  = mkSpan('#aaa',    'avg --');
        var sep1 = mkSpan('#aaa',    '|');
        var eng  = mkSpan('#ce93d8', '');
        var sync = mkSpan('#4caf50', '');
        var sep2 = mkSpan('#aaa',    '');
        var hz   = mkSpan('#aaa',    'DISPLAY -- Hz');
        var drop = mkSpan('#555',    'DROP 0');
        sp.appendChild(dev);  sp.appendChild(avg);  sp.appendChild(sep1);
        sp.appendChild(eng);  sp.appendChild(sync); sp.appendChild(sep2);
        sp.appendChild(hz);   sp.appendChild(drop);
        _barEl.appendChild(sp);
        _barSpans = {dev: dev, avg: avg, eng: eng, sync: sync, sep2: sep2, hz: hz, drop: drop};
    }

    function _updateBar() {
        if (!_barEl || !_barSpans) return;
        var devFps = Math.round(_fpsRing.last()  || 0);
        var devAvg = Math.round(_fpsRing.avg()   || 0);
        var comp   = window.__xcmWasmComp;
        var engFps = (comp && typeof comp.engineFps === 'function') ? Math.round(comp.engineFps()) : null;
        var devHz  = window.__xcmHz || 0;

        _barSpans.dev.textContent  = 'DEV ' + devFps + ' fps';
        _barSpans.avg.textContent  = 'avg ' + devAvg;

        if (engFps !== null) {
            _barSpans.eng.textContent  = 'ENG ' + engFps + ' fps';
            _barSpans.eng.style.display = '';
            var diff  = Math.round(devFps - engFps);
            var sign  = diff >= 0 ? '+' : '';
            var scol  = '#4caf50';
            if (Math.abs(diff) >= 8)       scol = '#f44336';
            else if (Math.abs(diff) >= 3)  scol = '#ff9800';
            _barSpans.sync.textContent   = 'SYNC ' + sign + diff;
            _barSpans.sync.style.color   = scol;
            _barSpans.sync.style.display = '';
            _barSpans.sep2.textContent   = '|';
        } else {
            _barSpans.eng.style.display  = 'none';
            _barSpans.sync.style.display = 'none';
            _barSpans.sep2.textContent   = '';
        }

        _barSpans.hz.textContent   = 'DISPLAY ' + devHz + ' Hz';
        _barSpans.drop.textContent = 'DROP ' + _dropped;
        _barSpans.drop.style.color = _dropped > 0 ? '#f44336' : '#555';
    }

    function _showBar() {
        if (!_barEl) _barEl = _createBar();
        if (!_barSpans) _buildBarSpans();
        _barEl.style.display = 'flex';
        if (!_barTimer) _barTimer = setInterval(_updateBar, 500);
    }

    function _hideBar() {
        if (_barTimer) { clearInterval(_barTimer); _barTimer = null; }
        if (_barEl) _barEl.style.display = 'none';
    }

    function _createHud() {
        var el = document.createElement('div');
        el.id = '__xcmStatsHud__';
        Object.assign(el.style, {
            position:       'fixed',
            top:            '8px',
            right:          '8px',
            zIndex:         '2147483647',
            background:     'rgba(0,0,0,0.75)',
            color:          '#0f0',
            fontFamily:     'monospace',
            fontSize:       '11px',
            lineHeight:     '1.5',
            padding:        '6px 10px',
            borderRadius:   '4px',
            pointerEvents:  'none',
            whiteSpace:     'pre',
            userSelect:     'none',
            backdropFilter: 'blur(2px)',
        });
        document.body.appendChild(el);
        return el;
    }

    function _updateHud() {
        if (!_hudVis) return;
        if (!_hudEl) _hudEl = _createHud();
        var fps    = _fpsRing.last().toFixed(0);
        var fpsAvg = _fpsRing.avg().toFixed(0);
        var fpsMin = _fpsRing.min().toFixed(0);
        var ft     = _ftRing.last().toFixed(2);
        var ftMax  = _ftRing.max().toFixed(2);
        var ltAvg  = _ltRing._len ? _ltRing.avg().toFixed(1) : '0';
        var ltMax  = _ltRing._len ? _ltRing.max().toFixed(1) : '0';
        _hudEl.textContent = [
            'FPS  ' + fps + '  avg ' + fpsAvg + '  min ' + fpsMin,
            'FT   ' + ft + 'ms  max ' + ftMax + 'ms',
            'LT   avg ' + ltAvg + 'ms  max ' + ltMax + 'ms  x' + _ltCount,
            'DROP ' + _dropped,
            'BDG  ' + _budget.toFixed(2) + 'ms',
        ].join('\n');
    }

    // ── Public API ───────────────────────────────────────────────────────────
    var stats = {
        fps:          function () { return _fpsRing.last();  },
        fpsAvg:       function () { return _fpsRing.avg();   },
        fpsMin:       function () { return _fpsRing.min();   },
        frameTime:    function () { return _ftRing.last();   },
        frameTimeMax: function () { return _ftRing.max();    },
        longTaskAvg:  function () { return _ltRing.avg();    },
        longTaskMax:  function () { return _ltRing.max();    },
        longTaskCount: function () { return _ltCount;        },
        dropped:      function () { return _dropped;         },
        budget:       function () { return _budget;          },

        snapshot: function () {
            return {
                fps:          stats.fps(),
                fpsAvg:       stats.fpsAvg(),
                fpsMin:       stats.fpsMin(),
                frameTime:    stats.frameTime(),
                frameTimeMax: stats.frameTimeMax(),
                longTaskAvg:  stats.longTaskAvg(),
                longTaskMax:  stats.longTaskMax(),
                longTaskCount: stats.longTaskCount(),
                dropped:      stats.dropped(),
                budget:       stats.budget(),
            };
        },

        showHud: function () {
            _hudVis = true;
            if (!_hudEl) _hudEl = _createHud();
            _hudEl.style.display = 'block';
            if (!_hudTimer) _hudTimer = setInterval(_updateHud, 500);
        },

        hideHud: function () {
            _hudVis = false;
            if (_hudTimer) { clearInterval(_hudTimer); _hudTimer = null; }
            if (_hudEl) _hudEl.style.display = 'none';
        },

        // Start / stop the measurement loop.
        start: function () { _startLoop(); },
        stop:  function () { _stopLoop(); },

        // Bottom FPS sync bar (device vs engine, always-visible).
        showBar: function () { _showBar(); },
        hideBar: function () { _hideBar(); },

        // Spector-style: capture one frame's PerformanceEntry list.
        captureFrame: function (cb) {
            var _before = performance.now();
            requestAnimationFrame(function () {
                var dt = performance.now() - _before;
                var entries = [];
                if (performance.getEntriesByType) {
                    // Grab resource timing entries created in the last ~2 frames.
                    var all = performance.getEntriesByType('resource');
                    var cutoff = performance.now() - 200;
                    for (var i = 0; i < all.length; i++) {
                        if (all[i].startTime > cutoff) entries.push({
                            name:     all[i].name,
                            type:     all[i].initiatorType,
                            start:    all[i].startTime.toFixed(2),
                            duration: all[i].duration.toFixed(2),
                            size:     all[i].transferSize || 0,
                        });
                    }
                }
                if (cb) cb({
                    frameTime: dt.toFixed(2),
                    resources: entries,
                    snapshot:  stats.snapshot(),
                });
            });
        },
    };

    // Attach to __xcm if the CDM SDK is present; otherwise expose standalone.
    if (global.__xcm) {
        global.__xcm.stats = stats;
    } else {
        global.__xcmStats = stats;
    }

    // Auto-start.
    stats.start();

    // Auto-show the bottom FPS sync bar.
    // Deferred to DOMContentLoaded so document.body is available for appendChild.
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function () { _showBar(); });
    } else {
        _showBar();
    }

})(window);
