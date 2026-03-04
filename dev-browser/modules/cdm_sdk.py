"""
cdm_sdk.py -- Content Display Module SDK

A production-grade cooperative scheduler and DOM safety layer injected into
every page via WKUserScript.  It replaces the lightweight CDM_PERF_JS with a
full SDK that exports a stable public API under window.__xcm.

Architecture
------------
  window.__xcm.schedule(task, priority)
      Queue a unit of work.  The scheduler drains the queue using
      MessageChannel macro-tasks so the browser always gets a chance to
      paint between task slices.  If a frame deadline is approaching (less
      than FRAME_BUDGET_MS of budget left) the drain pauses and resumes on
      the next macro-task.

  window.__xcm.read(fn)   -- batched DOM read  (runs before writes)
  window.__xcm.write(fn)  -- batched DOM write (runs after reads)
      Prevents forced synchronous layouts by guaranteeing the
      read -> write ordering within each frame.  All reads are flushed
      first, then all writes, then the cycle repeats.

  window.__xcm.idle(fn)
      Runs fn in requestIdleCallback (or rAF fallback).  Used for
      non-critical work that should never touch a scroll frame.

  window.__xcm.onScroll(fn)
      Register a post-scroll hook that fires once the scroll burst ends
      (debounced to the Hz-derived idle threshold).

  window.__xcm.hz        -- detected refresh rate (from ticker-inject.js)
  window.__xcm.scrolling -- true while a scroll burst is active

CSS layer
---------
  Injects targeted compositor hints and containment rules for the most
  common scrollable container patterns.  Uses MutationObserver to apply
  containment to feed items as they are added to the DOM.

  The rule set avoids blanket will-change:transform (which causes excessive
  memory use on long pages) and instead promotes only the outermost scroll
  container and leaves child items with contain:layout style.
"""

# ---------------------------------------------------------------------------
# CDM SDK JavaScript
# ---------------------------------------------------------------------------

CDM_SDK_JS: str = r"""
(function (global) {
'use strict';

if (global.__xcm && global.__xcm._v >= 2) return;

// ── Constants ──────────────────────────────────────────────────────────────
var SDK_VERSION    = 2;

// Task-slice budget derived from the detected refresh rate.
// Uses 55% of one frame period so the compositor always has the remaining
// 45% for rasterisation/compositing without contention.
// Falls back to 9 ms until the ticker provides a real frame time.
function _frameBudget() {
    return Math.min(12, (global.__xcmFrameBudget || (global.__xcmFrameMs || 16.67) * 0.55));
}

// How long after the last scroll event before we consider scrolling done.
// Kept for reference; the actual idle timer now lives in input-watcher.js.
// CDM SDK reads scroll state from the input atom -- it does not own a timer.
var _scrollIdle    = (global.__xcmIdleThreshold || 68);  // unused by SDK, atom-driven

// ── MessageChannel yielder ─────────────────────────────────────────────────
// MessageChannel posts are macro-tasks processed after the browser has had
// a chance to paint.  This is faster than setTimeout(fn,0) and is the same
// mechanism used by React's concurrent scheduler.
var _ch    = new MessageChannel();
var _port1 = _ch.port1;
var _port2 = _ch.port2;

// ── Priority queues ─────────────────────────────────────────────────────────
// Three priority levels.  The drain loop processes from high to low.
var PRIORITY = { HIGH: 0, NORMAL: 1, LOW: 2 };
var _queues  = [[], [], []];   // indexed by priority level
var _running = false;

// ── Read / write batch buffers ─────────────────────────────────────────────
var _reads  = [];
var _writes = [];
var _rwScheduled = false;

// ── Scroll state via input atom ───────────────────────────────────────────
// The input-watcher.js module owns the canonical scroll state, written by
// all wheel/scroll/touch events through a single Int32Array atom.
// CDM SDK reads the atom; it never writes to it.
// _scrollHooks are fired here (post-scroll) because CDM SDK owns the
// scheduler API -- but the scroll detection authority is the atom.
var _scrollHooks  = [];
var _prevScrolling = false;  // last seen value, used for edge detection

// Convenience: single 32-bit read of the current input flags.
// Returns 0 if the input watcher has not loaded yet (safe fallback).
function _inputFlags() {
    var iw = global.__xcmInput;
    return iw ? iw.state[0] : 0;
}
function _isScrolling() {
    return (_inputFlags() & 1) !== 0;
}

// ── Internal helpers ───────────────────────────────────────────────────────
function _now() {
    return (global.performance && global.performance.now) ? global.performance.now() : Date.now();
}

// Drain the task queues in priority order, respecting the per-slice budget.
// If the budget is exhausted the remaining tasks re-post via MessageChannel
// so the browser can run a paint / composite pass before we continue.
function _drain() {
    _running = false;
    // Snapshot the input flags ONCE at the top of this task.
    // Using the snapshot for all decisions inside _drain prevents a wheel
    // event that fires between two task calls from invalidating a choice
    // that was already made for this drain pass.
    var flagSnap   = _inputFlags();
    var scrolling  = (flagSnap & 1) !== 0;
    var deadline   = _now() + _frameBudget();
    outer:
    for (var p = 0; p < 3; p++) {
        while (_queues[p].length) {
            // Pause if over budget; let the browser breathe.
            if (_now() >= deadline) {
                _schedule();  // re-post
                break outer;
            }
            // Skip non-critical work while the user is scrolling.
            if (scrolling && p > PRIORITY.HIGH) {
                break outer;
            }
            var fn = _queues[p].shift();
            try { fn(); } catch (e) {}
        }
    }
}

function _schedule() {
    if (_running) return;
    _running = true;
    _port1.postMessage(null);
}

_port2.onmessage = function () { _drain(); };

// Flush read batch then write batch in the same rAF tick.
// This prevents any write queueing a read that forces an immediate relayout.
function _rwFlush() {
    _rwScheduled = false;
    var r = _reads.splice(0);
    var w = _writes.splice(0);
    for (var i = 0; i < r.length; i++) { try { r[i](); } catch (e) {} }
    for (var j = 0; j < w.length; j++) { try { w[j](); } catch (e) {} }
}

// Schedule a read/write flush on the NEXT frame.
// Hooking into __xcmTick (the shared rAF loop from the ticker) means there
// is exactly ONE requestAnimationFrame callback per frame regardless of
// how many read/write operations are queued.  Also skips the flush
// entirely when a scroll burst is active -- defers to the first idle frame.
function _rwTick() {
    if (_rwScheduled) return;
    _rwScheduled = true;

    function _tryFlush(ts, deadline) {
        // Snapshot the flag once at the top of this callback.
        // If the user is scrolling, stay subscribed and try next frame.
        if (_isScrolling()) return;
        // Unsubscribe before flushing so a re-queued write calls _rwTick fresh.
        if (_unsub) { _unsub(); _unsub = null; }
        _rwFlush();
    }

    var _unsub = null;
    if (global.__xcmTick) {
        _unsub = global.__xcmTick(_tryFlush);
    } else {
        // Ticker not ready yet -- plain rAF fallback.
        requestAnimationFrame(function () {
            if (!_isScrolling()) { _rwFlush(); }
            else { _rwScheduled = false; _rwTick(); }
        });
    }
}

// ── Scroll edge detection and body class via __xcmTick ────────────────────
// Subscribe to the shared rAF bus to detect SCROLLING->idle transitions
// and fire the onScroll hooks in the first quiet frame.
// Also maintains the body.__xcmScrolling class which is used by the CSS
// layer to freeze transitions during scroll.
(function () {
    function _scrollTick() {
        var nowScrolling = _isScrolling();

        // Body class: add/remove each frame in sync with the atom.
        if (document.body) {
            if (nowScrolling) {
                document.body.classList.add('__xcmScrolling');
            } else {
                document.body.classList.remove('__xcmScrolling');
            }
        }

        // Scroll-end edge: fire hooks on the first frame where scrolling
        // transitions from true to false.
        if (_prevScrolling && !nowScrolling) {
            for (var i = 0; i < _scrollHooks.length; i++) {
                try { _scrollHooks[i](); } catch (e) {}
            }
        }
        _prevScrolling = nowScrolling;
    }

    function _registerScrollTick() {
        if (global.__xcmTick) {
            global.__xcmTick(_scrollTick);
        } else {
            requestAnimationFrame(function () { _registerScrollTick(); });
        }
    }
    _registerScrollTick();
})();

// ── Passive listener enforcement ───────────────────────────────────────────
// Force wheel / touch listeners on window and document to be passive so the
// browser never has to wait for JS before compositing a scroll frame.
// The input-watcher already registers its own listeners as passive, but
// site JS loaded after injection may still register non-passive listeners.
// Patching addEventListener intercepts those at register time.
(function () {
    var _targets = [global, document, document.documentElement];
    var _forced  = { wheel: 1, mousewheel: 1, touchstart: 1, touchmove: 1 };

    function _patch(obj) {
        if (!obj || obj.__xcmPassivePatched__) return;
        obj.__xcmPassivePatched__ = true;
        var _orig = obj.addEventListener.bind(obj);
        obj.addEventListener = function (type, fn, opts) {
            if (_forced[type]) {
                if (opts && typeof opts === 'object') {
                    opts = Object.assign({}, opts, { passive: true });
                } else {
                    opts = { passive: true, capture: !!opts };
                }
            }
            return _orig(type, fn, opts);
        };
    }

    _targets.forEach(_patch);
})();

// ── CSS layer ──────────────────────────────────────────────────────────────
(function () {
    var STYLE_ID = '__xcmSdkStyle__';
    if (document.getElementById(STYLE_ID)) return;
    var s = document.createElement('style');
    s.id = STYLE_ID;
    // Rules are ordered from broadest to most targeted.
    // - overscroll-behavior:contain stops momentum bleed into chrome.
    // - Scroll containers get their own compositor layer via will-change.
    //   Scope is kept to known patterns so we do not waste GPU memory.
    // - Feed-item containment tells the browser layout inside each card
    //   is independent, enabling partial layout and faster geometry queries.
    // - During __xcmScrolling class, transitions are suspended so the
    //   compositor does not need to interpolate animated values mid-scroll.
    s.textContent = [
        'html { overscroll-behavior: contain; }',

        /* Promote the primary scroll container to its own compositor layer.
           Only the direct scrolling surface -- not every child -- gets
           will-change so GPU memory use stays bounded. */
        '[data-xcm-scroll] { will-change: transform; }',

        /* touch-action: pan-y tells the WKWebView compositor that this
           container handles vertical pans natively.  The compositor can
           then forward momentum-scroll frames directly to the GPU layer
           without pausing to ask the JS thread whether it wants to
           preventDefault -- eliminating the single biggest source of
           main-thread blocking during trackpad and touchpad scrolling. */
        '[data-xcm-scroll] { touch-action: pan-y; }',

        /* Containment for feed-style list items.  layout+style means the
           browser can skip full-page layout when items change internally. */
        '[data-xcm-item] {',
        '  contain: layout style;',
        '  content-visibility: auto;',
        '  contain-intrinsic-size: auto 200px;',
        '}',

        /* Freeze paint-triggering properties while scrolling to keep the
           frame budget available for compositing only. */
        'body.__xcmScrolling [data-xcm-item] {',
        '  transition: none !important;',
        '  animation: none !important;',
        '}',

        /* Prevent fixed/sticky elements from creating unnecessary stacking
           contexts that break GPU layer promotion. */
        '[style*="position: fixed"], [style*="position:fixed"] {',
        '  transform: translateZ(0);',
        '}',
    ].join('\n');
    (document.head || document.documentElement).appendChild(s);
})();

// ── MutationObserver: tag scroll containers and feed items ─────────────────
// Applies data-xcm-scroll / data-xcm-item attributes so the CSS rules above
// take effect without requiring site-specific selectors in CSS.
(function () {
    // Patterns that typically represent a scrollable feed container.
    var SCROLL_SELECTORS = [
        'main', '[role="main"]', '#content', '#main',
        '.feed', '.timeline', '.stream', '.list-view',
        'ytd-page-manager', 'ytd-app',
    ].join(',');

    // Patterns for individual feed items / cards.
    var ITEM_SELECTORS = [
        'article', '[role="article"]',
        '.post', '.tweet', '.card', '.entry',
        'ytd-rich-item-renderer', 'ytd-video-renderer',
        'ytd-compact-video-renderer', 'shreddit-post',
        '[data-testid="tweet"]', '[data-testid="post-container"]',
    ].join(',');

    function _tag(root) {
        var sc = (root || document).querySelectorAll(SCROLL_SELECTORS);
        for (var i = 0; i < sc.length; i++) {
            if (!sc[i].hasAttribute('data-xcm-scroll')) {
                sc[i].setAttribute('data-xcm-scroll', '1');
            }
        }
        var it = (root || document).querySelectorAll(ITEM_SELECTORS);
        for (var j = 0; j < it.length; j++) {
            if (!it[j].hasAttribute('data-xcm-item')) {
                it[j].setAttribute('data-xcm-item', '1');
            }
        }
    }

    // Tag existing DOM.
    if (document.body) {
        _tag(document);
    }

    // Tag new nodes as the page inserts them (infinite scroll, SPAs).
    var _mo = new MutationObserver(function (records) {
        // Batch: collect all added nodes first, then tag in one write pass.
        var roots = [];
        for (var i = 0; i < records.length; i++) {
            var added = records[i].addedNodes;
            for (var j = 0; j < added.length; j++) {
                if (added[j].nodeType === 1) roots.push(added[j]);
            }
        }
        if (roots.length) {
            // Use the write queue so we do not trigger layout during a read.
            __xcm.write(function () {
                for (var k = 0; k < roots.length; k++) { _tag(roots[k]); }
            });
        }
    });

    function _startObserver() {
        if (document.body) {
            _mo.observe(document.body, { childList: true, subtree: true });
            _tag(document);
        } else {
            requestAnimationFrame(_startObserver);
        }
    }
    _startObserver();
})();

// ── Public API ─────────────────────────────────────────────────────────────
var __xcm = {
    _v: SDK_VERSION,

    // Scheduler
    schedule: function (fn, priority) {
        var p = (priority === undefined) ? PRIORITY.NORMAL : priority;
        _queues[p].push(fn);
        _schedule();
    },

    // Priority constants exposed for callers.
    HIGH:   PRIORITY.HIGH,
    NORMAL: PRIORITY.NORMAL,
    LOW:    PRIORITY.LOW,

    // Read/write batcher
    read:  function (fn) { _reads.push(fn);  _rwTick(); },
    write: function (fn) { _writes.push(fn); _rwTick(); },

    // Idle work (requestIdleCallback with rAF fallback)
    idle: function (fn) {
        if (global.requestIdleCallback) {
            global.requestIdleCallback(fn, { timeout: 2000 });
        } else {
            requestAnimationFrame(function () { setTimeout(fn, 0); });
        }
    },

    // Scroll hooks
    onScroll: function (fn) {
        _scrollHooks.push(fn);
        return function () {
            var idx = _scrollHooks.indexOf(fn);
            if (idx !== -1) _scrollHooks.splice(idx, 1);
        };
    },

    // Live state -- reads from the input-watcher atom so there is one truth.
    get scrolling() { return _isScrolling(); },
    get hz()        { return global.__xcmHz || 60; },
};

global.__xcm = __xcm;

})(window);
"""


# ---------------------------------------------------------------------------
# Poll guard that uses the SDK scrolling flag
# ---------------------------------------------------------------------------
# Same contract as CDM_POLL_JS but reads from __xcm.scrolling so there is
# one source of truth.

CDM_SDK_POLL_JS: str = (
    '(function(){'
    'if(window.__xcm&&window.__xcm.scrolling)return"[]";'
    'var q=JSON.stringify(window.__xcmCatQueue__||[]);'
    'window.__xcmCatQueue__=[];'
    'return q;'
    '})();'
)
