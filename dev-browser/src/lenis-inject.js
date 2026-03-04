/**
 * lenis-inject.js
 *
 * Bundled as an IIFE and injected into every page at DocumentReady.
 * Lenis intercepts wheel / touch events, accumulates delta into a damped
 * target, and drives scroll position via rAF.  macOS inertia events are
 * absorbed by the damping curve rather than hitting the compositor directly,
 * which eliminates the "scroll then sudden stop" artifact.
 *
 * Guards:
 *  - Skips pages that already have Lenis running (__xcmLenis__ flag).
 *  - Skips pages with a body transform-scroll setup (GSAP ScrollTrigger / Locomotive).
 *  - Destroys and recreates on SPA navigation via pushState / replaceState.
 */

import Lenis from 'lenis';

(function () {
    'use strict';

    function _initLenis() {
        if (window.__xcmLenis__) return;

        // Do not layer on top of known virtual-scroll frameworks.
        if (window.locomotiveScroll || window.ScrollTrigger || window.Scrollbar) return;

        var lenis = new Lenis({
            duration:           1.2,
            easing:             function (t) { return Math.min(1, 1.001 - Math.pow(2, -10 * t)); },
            orientation:       'vertical',
            gestureOrientation:'vertical',
            smoothWheel:        true,
            syncTouch:          false,    // leave native touch alone
            normalizeWheel:     false,    // let Chromium deliver raw deltas
            infinite:           false,
        });

        window.__xcmLenis__ = lenis;

        var _rafId;
        function raf(time) {
            lenis.raf(time);
            _rafId = requestAnimationFrame(raf);
        }
        _rafId = requestAnimationFrame(raf);

        // Expose a stop/restart for SPA navigations.
        window.__xcmLenisStop__ = function () {
            lenis.destroy();
            cancelAnimationFrame(_rafId);
            delete window.__xcmLenis__;
            delete window.__xcmLenisStop__;
        };
    }

    // Run after DOM is ready so Lenis can find document.body.
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _initLenis, { once: true });
    } else {
        _initLenis();
    }

    // Re-init on SPA route changes.
    (function () {
        var _origPush    = history.pushState;
        var _origReplace = history.replaceState;
        function _onNav() {
            if (window.__xcmLenisStop__) window.__xcmLenisStop__();
            setTimeout(_initLenis, 150);
        }
        if (typeof _origPush === 'function') {
            history.pushState = function () { _origPush.apply(this, arguments); _onNav(); };
        }
        if (typeof _origReplace === 'function') {
            history.replaceState = function () { _origReplace.apply(this, arguments); _onNav(); };
        }
    })();
})();
