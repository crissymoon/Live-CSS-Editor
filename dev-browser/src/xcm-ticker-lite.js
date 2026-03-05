/**
 * xcm-ticker-lite.js
 *
 * Injected at document-start before all other xcm performance scripts.
 * Lightweight alternative to ticker-inject.js that works without GSAP.
 *
 * Responsibilities:
 *   1. Hz detection via rAF delta sampling (30 frames).
 *      Sets window.__xcmHz and window.__xcmFrameMs.
 *      A safe 60Hz provisional value is used until detection completes.
 *
 *   2. window.__xcmIdleThreshold -- idle-callback stop threshold (ms),
 *      set to frameMs * 0.3 after detection.  The lazy, virtualizer, and
 *      compress injectors read this to know when to yield in their chunked
 *      loops.  Falls back to 4 in those scripts if we have not yet run.
 *
 *   3. window.__xcmTick(fn) -- registers a per-frame rAF callback shared
 *      across all xcm scripts.  Returns an unregister function.
 *      Capped at 60fps on high-Hz displays (>75Hz) so housekeeping work
 *      does not run more than needed.
 *
 *   4. GSAP compatibility shim -- if ticker-inject.js also runs (bundled
 *      version) this script defers to window.__xcmGSAP and skips its own
 *      rAF loop to avoid duplicate ticking.
 */
(function () {
    'use strict';

    // Provisional values -- overwritten after detection.
    window.__xcmHz            = 60;
    window.__xcmFrameMs       = 16.667;
    window.__xcmIsHighHz      = false;
    window.__xcmIdleThreshold = 4;

    // ── 1. Hz detection ──────────────────────────────────────────────────────
    var SAMPLE_COUNT = 30;
    var _samples     = [];
    var _lastTs      = 0;

    function _snapHz(hz) {
        if (hz >= 130) return 144;
        if (hz >= 105) return 120;
        if (hz >=  82) return 90;
        if (hz >=  50) return 60;
        return 30;
    }

    function _onDetected() {
        _samples.sort(function (a, b) { return a - b; });
        var median  = _samples[Math.floor(_samples.length / 2)];
        var raw     = Math.round(1000 / median);
        var hz      = _snapHz(raw);
        var frameMs = 1000 / hz;

        window.__xcmHz            = hz;
        window.__xcmFrameMs       = frameMs;
        window.__xcmIsHighHz      = hz > 75;
        window.__xcmIdleThreshold = Math.max(1, frameMs * 0.3);

        // If GSAP ticker is present, update its fps cap too.
        var g = window.__xcmGSAP;
        if (g && g.ticker && typeof g.ticker.fps === 'function') {
            g.ticker.fps(window.__xcmIsHighHz ? 60 : hz);
        }
    }

    // ── 2. Shared rAF tick loop ───────────────────────────────────────────────
    // All per-frame work is multiplexed through one rAF loop so we never have
    // two competing requestAnimationFrame chains running at the same time.
    var _listeners = [];
    var _rafRunning = false;
    var _lastTickTs = 0;
    // Cap interval in ms: 60fps ceiling on high-Hz displays.
    var _capMs = 1000 / 60;  // updated after Hz detection

    function _raf(ts) {
        // Hz detection sampling.
        if (_samples.length < SAMPLE_COUNT) {
            if (_lastTs) _samples.push(ts - _lastTs);
            _lastTs = ts;
            if (_samples.length >= SAMPLE_COUNT) _onDetected();
        }

        // Apply fps cap on high-Hz displays.
        var elapsed = ts - _lastTickTs;
        if (elapsed >= _capMs) {
            _lastTickTs = ts;
            for (var i = 0; i < _listeners.length; i++) {
                try { _listeners[i](ts, elapsed); } catch (_) {}
            }
        }

        if (_rafRunning) requestAnimationFrame(_raf);
    }

    function _startRaf() {
        if (_rafRunning) return;
        _rafRunning = true;
        requestAnimationFrame(_raf);
    }

    function _stopRaf() {
        _rafRunning = false;
    }

    // ── 3. window.__xcmTick ──────────────────────────────────────────────────
    // Returns an unregister function.  Starts the loop on first registration.
    window.__xcmTick = function (fn) {
        // Defer to GSAP ticker if available from ticker-inject.js.
        var g = window.__xcmGSAP;
        if (g && g.ticker && typeof g.ticker.add === 'function') {
            g.ticker.add(fn);
            return function () { g.ticker.remove(fn); };
        }

        _listeners.push(fn);
        _startRaf();

        return function () {
            var idx = _listeners.indexOf(fn);
            if (idx !== -1) _listeners.splice(idx, 1);
            if (_listeners.length === 0) _stopRaf();
        };
    };

    // Start the shared loop immediately so Hz detection begins right away.
    // It will stay alive as long as there are registered listeners; Hz
    // detection finishes after ~500ms then the loop stops if no listeners
    // have been added yet.
    var _detectionUnregister = window.__xcmTick(function () {});
    // Unregister the placeholder once detection is done.
    var _detWatcher = setInterval(function () {
        if (_samples.length >= SAMPLE_COUNT) {
            clearInterval(_detWatcher);
            _detectionUnregister();
        }
    }, 100);

})();
