/**
 * ticker-inject.js
 *
 * Injected at DocumentCreation, runs before all other xcm scripts.
 *
 * Responsibilities:
 *
 *   1. Hz detection -- measures the actual display refresh rate by sampling
 *      rAF timestamp deltas over 30 frames (~250ms at 60Hz, ~125ms at 120Hz).
 *      Sets window.__xcmHz and window.__xcmFrameMs immediately after detection.
 *      A safe provisional value of 16ms (60Hz) is assumed until then so other
 *      scripts that start earlier do not divide by zero.
 *
 *   2. GSAP ticker setup -- imports GSAP, configures lag smoothing so a tab
 *      that was backgrounded does not spike with a giant delta on resume, and
 *      exposes it as window.__xcmGSAP.  If the page already has window.gsap
 *      loaded we reuse it instead of injecting a second copy.
 *
 *   3. Rate cap -- on high-Hz displays (>75Hz) the utility ticker is capped
 *      at 60fps via gsap.ticker.fps(60).  Full native Hz is only needed for
 *      genuine animation interpolation, not for DOM housekeeping work.
 *
 *   4. window.__xcmTick(fn) -- register a per-frame callback on the shared
 *      GSAP ticker at the capped rate.  Returns an unregister function.
 *
 *   5. window.__xcmIdleThreshold -- the idle-callback stop threshold in ms,
 *      derived from the detected frame time (frameMs * 0.3).  The lazy,
 *      virtualizer, and compress injectors read this instead of hard-coded
 *      values, so they never over-run a short 120Hz frame budget.
 *
 * Naming notes:
 *   window.__xcmGSAP    -- the GSAP instance (may be the same as window.gsap)
 *   window.__xcmHz      -- detected refresh rate (30 | 60 | 90 | 120 | 144)
 *   window.__xcmFrameMs -- milliseconds per frame at detected Hz
 *   window.__xcmIsHighHz -- true when Hz > 75
 *   window.__xcmTick    -- (fn) => unregisterFn
 *   window.__xcmIdleThreshold -- ms remaining cutoff for idle-chunk loops
 */

import { gsap } from 'gsap';

(function () {
    'use strict';

    // ── Provisional values until Hz detection completes ──────────────────────
    window.__xcmHz             = 60;
    window.__xcmFrameMs        = 16;
    window.__xcmIsHighHz       = false;
    window.__xcmIdleThreshold  = 4;   // will be updated after detection

    // ── 1. Hz detection ──────────────────────────────────────────────────────
    // Sample SAMPLE_COUNT consecutive rAF deltas.  Median of the sorted list
    // is more robust than mean because a few slow frames (GC, cold start) can
    // skew the mean but leave the median untouched.
    var SAMPLE_COUNT = 30;
    var _samples = [];
    var _last    = 0;

    function _snap(hz) {
        // Snap to the nearest standard refresh rate.
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
        var hz      = _snap(raw);
        var frameMs = 1000 / hz;

        window.__xcmHz            = hz;
        window.__xcmFrameMs       = frameMs;
        window.__xcmIsHighHz      = hz > 75;
        // Allow approx 30% of a frame for idle work before stopping the chunk.
        window.__xcmIdleThreshold = Math.max(1, frameMs * 0.3);

        // Inform GSAP ticker of the actual cap.
        var g = window.__xcmGSAP;
        if (g && g.ticker) {
            // At high Hz, cap utility ticker at 60fps.
            // At normal Hz, just set it to the actual Hz.
            var cap = window.__xcmIsHighHz ? 60 : hz;
            g.ticker.fps(cap);
        }
    }

    function _sampleRaf(ts) {
        if (_last) _samples.push(ts - _last);
        _last = ts;
        if (_samples.length < SAMPLE_COUNT) {
            requestAnimationFrame(_sampleRaf);
        } else {
            _onDetected();
        }
    }
    requestAnimationFrame(_sampleRaf);

    // ── 2. GSAP ticker setup ─────────────────────────────────────────────────
    // If the page already loaded gsap, reuse it to avoid two tick loops and
    // two copies of the tween engine in the V8 heap.
    var _gsap = (typeof window !== 'undefined' && window.gsap) ? window.gsap : gsap;
    window.__xcmGSAP = _gsap;

    // lagSmoothing: if the tab was backgrounded, rAF resumes with a huge
    // timestamp delta, which makes GSAP think it needs to jump all tweens
    // forward by hundreds of ms.  lagSmoothing caps the simulated elapsed
    // time to maxLag (500ms threshold, 33ms cap = one dropped frame).
    _gsap.ticker.lagSmoothing(500, 33);

    // Set an initial cap.  Will be updated in _onDetected once Hz is known.
    // 60fps is a safe default -- harmless at 60Hz, conservative at 120Hz.
    _gsap.ticker.fps(60);

    // ── 3. window.__xcmTick ──────────────────────────────────────────────────
    // Thin wrapper so other injectors can register per-frame callbacks without
    // importing GSAP directly.  Returns an unregister function.
    window.__xcmTick = function (fn) {
        _gsap.ticker.add(fn);
        return function () { _gsap.ticker.remove(fn); };
    };

    // ── 4. Per-frame Hz readout diagnostics (dev only) ───────────────────────
    // Uncomment to log detected Hz to the console.
    // _gsap.ticker.add(function () {
    //     console.log('xcm tick', window.__xcmHz + 'Hz', window.__xcmFrameMs.toFixed(2) + 'ms');
    // }, false, 0);

})();
