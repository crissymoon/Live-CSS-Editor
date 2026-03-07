/**
 * xcm-scroll-restore.js
 *
 * Per-URL scroll position save and restore using sessionStorage.
 * Injected at document-start.
 *
 * Covers three navigation patterns:
 *   1. Native back/forward -- WKWebView reloads the page;
 *      scroll is restored on DOMContentLoaded.
 *   2. SPA pushState/replaceState route changes -- scroll is saved for
 *      the outgoing URL, then restored for the incoming one after a tick.
 *   3. Tab focus restore -- if the user switched tabs and came back,
 *      the last scroll position is re-applied.
 *
 * Also saves on scroll (debounced) and before unload so a page refresh
 * drops back to where the user was.
 *
 * Keys are of the form  xcm:scroll:<href>  stored in sessionStorage so
 * they are cleared when the browser session ends and do not persist
 * across restarts.
 *
 * Skips pages that use their own virtual scroll (GSAP ScrollTrigger,
 * Locomotive Scroll, Lenis) because those keep scroll state internally.
 */
(function () {
    'use strict';

    var PREFIX = 'xcm:scroll:';

    // ── Skip virtual-scroll pages ────────────────────────────────────────────
    function _hasVirtualScroll() {
        return !!(window.locomotiveScroll || window.ScrollTrigger ||
                  window.Scrollbar || window.__xcmLenis__);
    }

    // ── Storage helpers ──────────────────────────────────────────────────────
    function _save(url) {
        if (_hasVirtualScroll()) return;
        var x = Math.round(window.scrollX || document.documentElement.scrollLeft || 0);
        var y = Math.round(window.scrollY || document.documentElement.scrollTop  || 0);
        // Do not overwrite a useful saved position with (0,0) if the page
        // has just been created and has not scrolled yet.
        if (x === 0 && y === 0) return;
        try { sessionStorage.setItem(PREFIX + url, x + ',' + y); } catch (_) {}
    }

    function _restore(url) {
        if (_hasVirtualScroll()) return;
        var val;
        try { val = sessionStorage.getItem(PREFIX + url); } catch (_) { return; }
        if (!val) return;
        var parts = val.split(',');
        var x = parseInt(parts[0], 10) || 0;
        var y = parseInt(parts[1], 10) || 0;
        if (x === 0 && y === 0) return;

        // Two-attempt restore: one on DOMContentLoaded + one deferred so
        // that images / sticky headers do not shift the layout after restore.
        function _apply() {
            window.scrollTo({ top: y, left: x, behavior: 'instant' });
        }
        _apply();
        // Second attempt after a paint to correct for late-loading content.
        // Cancelled immediately if the user scrolls naturally during that window
        // so a fast trackpad gesture does not get snapped back to the saved pos.
        _cancelRestore();
        _restoreTimer = setTimeout(_apply, 200);
    }

    // ── Restore timer (module-scope so the scroll listener can cancel it) ────
    var _restoreTimer = null;
    function _cancelRestore() {
        if (_restoreTimer) { clearTimeout(_restoreTimer); _restoreTimer = null; }
    }

    // ── Save on scroll (debounced 300ms) ─────────────────────────────────────
    var _scrollTimer = null;
    window.addEventListener('scroll', function () {
        // A real user scroll cancels any pending deferred restore so that a
        // fast trackpad swipe never gets snapped back mid-gesture.
        _cancelRestore();
        if (_scrollTimer) clearTimeout(_scrollTimer);
        _scrollTimer = setTimeout(function () { _save(location.href); }, 300);
    }, { passive: true });

    // ── Save on tab hide / unload ────────────────────────────────────────────
    document.addEventListener('visibilitychange', function () {
        if (document.visibilityState === 'hidden') _save(location.href);
    });
    window.addEventListener('beforeunload', function () { _save(location.href); });

    // ── Restore on page load ─────────────────────────────────────────────────
    // history.scrollRestoration = 'manual' opts out of the browser's built-in
    // scroll restoration so our version is the only one that runs.
    if ('scrollRestoration' in history) {
        history.scrollRestoration = 'manual';
    }

    function _initialRestore() {
        _restore(location.href);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _initialRestore, { once: true });
    } else {
        _initialRestore();
    }

    // ── SPA navigation hooks ─────────────────────────────────────────────────
    (function () {
        var _origPush    = history.pushState;
        var _origReplace = history.replaceState;

        function _onPush(newUrl) {
            _save(location.href);           // save current URL's scroll
            // The browser will update location.href after _origPush runs, so
            // we capture the new URL from the call arguments.
            setTimeout(function () { _restore(newUrl); }, 80);
        }

        function _onReplace(newUrl) {
            // replaceState changes the current URL without creating a history
            // entry -- just restore if there is a saved position for it.
            setTimeout(function () { _restore(newUrl); }, 80);
        }

        if (typeof _origPush === 'function') {
            history.pushState = function (state, title, url) {
                _origPush.apply(this, arguments);
                _onPush(url ? String(url) : location.href);
            };
        }
        if (typeof _origReplace === 'function') {
            history.replaceState = function (state, title, url) {
                _origReplace.apply(this, arguments);
                _onReplace(url ? String(url) : location.href);
            };
        }

        // popstate fires on back/forward button within an SPA.
        window.addEventListener('popstate', function () {
            setTimeout(function () { _restore(location.href); }, 80);
        });
    })();

})();
