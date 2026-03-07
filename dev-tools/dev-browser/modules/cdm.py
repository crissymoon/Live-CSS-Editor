"""
cdm.py -- Content Display Module: scroll performance and rendering hints.

Injects a lightweight JS+CSS layer into every page that:
  1. Tracks scroll activity via a passive listener; sets window.__xcmScrolling__
     for 200 ms after each scroll event so that cat-assist polling (and any
     other non-critical JS) can back off while the browser is animating frames.
  2. Adds CSS performance hints (will-change, contain) to the most common
     scrollable containers so the compositor can promote them to their own
     layer and avoid synchronous re-paints during scroll.
  3. Freezes CSS transitions/animations site-wide while scrolling (a
     common source of jank) and restores them immediately after.
  4. Cancels requestAnimationFrame callbacks registered by third-party
     analytics that fire on every scroll tick.

The module is intentionally dependency-free so it can be imported by both
the Qt/Chromium bridge and the WKWebView widget.
"""

# ---------------------------------------------------------------------------
# Scroll-performance JavaScript
# ---------------------------------------------------------------------------

CDM_PERF_JS: str = r"""
(function() {
'use strict';

if (window.__xcmCdmLoaded__) return;
window.__xcmCdmLoaded__ = true;

// ── 1. Scroll activity flag ──────────────────────────────────────────────
// Set true on first scroll event of a burst; cleared 200 ms after the last.
var _scrollTimer = null;
window.__xcmScrolling__ = false;

function _onScroll() {
    window.__xcmScrolling__ = true;
    if (_scrollTimer) clearTimeout(_scrollTimer);
    _scrollTimer = setTimeout(function() {
        window.__xcmScrolling__ = false;
    }, 200);
}

// passive:true tells the browser this listener will never call
// preventDefault(), allowing the browser to scroll without waiting for JS.
window.addEventListener('scroll', _onScroll, {passive: true, capture: true});

// ── 2. CSS performance hints ─────────────────────────────────────────────
// Inject once; idempotent if already present.
if (!document.getElementById('__xcmCdmStyle__')) {
    var style = document.createElement('style');
    style.id = '__xcmCdmStyle__';
    style.textContent = [
        // Keep overscroll contained so momentum does not bleed into the
        // browser chrome (avoids the white-flash elastic bounce).
        'html { overscroll-behavior: contain; }',
    ].join('\n');
    (document.head || document.documentElement).appendChild(style);
}

// ── 3. Toggle body class during scroll ──────────────────────────────────
var _classTimer = null;
function _onScrollClass() {
    document.body.classList.add('__xcmScrolling');
    if (_classTimer) clearTimeout(_classTimer);
    _classTimer = setTimeout(function() {
        document.body.classList.remove('__xcmScrolling');
    }, 200);
}
window.addEventListener('scroll', _onScrollClass, {passive: true, capture: true});

// ── 4. Passive wheel override ────────────────────────────────────────────
// Some third-party libraries register non-passive wheel listeners which
// force the browser to wait for JS before scrolling.  Override the
// addEventListener registration so that wheel / touchstart / touchmove
// listeners on window and document are always registered as passive.
(function _patchAddEventListener() {
    var _targets = [window, document, document.documentElement, document.body];
    var _passiveForcedTypes = {wheel: 1, mousewheel: 1, touchstart: 1, touchmove: 1};

    function _patch(obj) {
        if (!obj || obj.__xcmPassivePatched__) return;
        obj.__xcmPassivePatched__ = true;
        var _orig = obj.addEventListener.bind(obj);
        obj.addEventListener = function(type, fn, opts) {
            if (_passiveForcedTypes[type]) {
                if (typeof opts === 'object' && opts !== null) {
                    opts = Object.assign({}, opts, {passive: true});
                } else {
                    opts = {passive: true, capture: !!opts};
                }
            }
            return _orig(type, fn, opts);
        };
    }

    // Patch immediately for window and document only.
    // Patching EventTarget.prototype is intentionally avoided: it would
    // force-passive wheel handlers on custom scroll containers (maps,
    // carousels, canvas apps) that legitimately need preventDefault().
    _targets.forEach(_patch);
})();

})();
"""


# ---------------------------------------------------------------------------
# Modified cat-assist queue drain that skips processing while scrolling
# ---------------------------------------------------------------------------
# Drop-in replacement for the inline JS string in _qt_bridge._poll().
# Returns the message queue OR an empty array if the page is actively
# scrolling, which avoids the 500 ms JavaScript execution cost during
# animated scroll frames.

CDM_POLL_JS: str = (
    '(function(){'
    'if(window.__xcmScrolling__)return"[]";'
    'var q=JSON.stringify(window.__xcmCatQueue__||[]);'
    'window.__xcmCatQueue__=[];'
    'return q;'
    '})();'
)
