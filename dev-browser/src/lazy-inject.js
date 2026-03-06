/**
 * lazy-inject.js
 *
 * Injected at DocumentCreation.
 *
 * Changes from v1:
 *   - MutationObserver callback is now fully off the main thread: mutations
 *     are queued and flushed inside requestIdleCallback (MessageChannel
 *     fallback for browsers without rIC). This prevents the observer from
 *     stealing frame time and causing scroll stutter.
 *   - _applyToSubtree uses a chunked loop inside rIC so large DOM subtrees
 *     (infinite-scroll batches of 20+ cards) do not block a frame.
 */

(function () {
    'use strict';

    var NEAR_VIEWPORT = 1.5;

    // ── Off-main-thread scheduler ───────────────────────────────────────────
    // Use requestIdleCallback when available; fall back to MessageChannel
    // (fires after paint, before next frame) then setTimeout(0).
    var _schedule = (function () {
        if (typeof requestIdleCallback === 'function') {
            return function (fn) { requestIdleCallback(fn, { timeout: 500 }); };
        }
        if (typeof MessageChannel !== 'undefined') {
            var _ch = new MessageChannel();
            var _queue = [];
            _ch.port1.onmessage = function () {
                var fn = _queue.shift();
                if (fn) fn();
            };
            return function (fn) { _queue.push(fn); _ch.port2.postMessage(null); };
        }
        return function (fn) { setTimeout(fn, 0); };
    })();

    // ── 1. Patch document.createElement ────────────────────────────────────
    // Sets loading=lazy + decoding=async on every img/iframe before the caller
    // can assign src, so the browser never eagerly fetches off-screen resources.
    // fetchpriority=low tells the browser these images lose the bandwidth race
    // against above-fold content.  We upgrade to 'high' for visible images in
    // the full-DOM pass (section 5).
    var _origCreate = document.createElement.bind(document);
    document.createElement = function (tag) {
        var el = _origCreate.apply(document, arguments);
        var t = (tag || '').toLowerCase();
        if (t === 'img') {
            el.loading       = 'lazy';
            el.decoding      = 'async';
            el.fetchPriority = 'low';
        } else if (t === 'iframe') {
            el.loading  = 'lazy';
            el.decoding = 'async';
        }
        return el;
    };

    // ── 2. IntersectionObserver for <video> ─────────────────────────────────
    var _videoIO = null;
    var _videoCount = 0;

    function _observeVideo(video) {
        if (_videoCount === 0) { _videoCount++; return; }
        _videoCount++;
        var src = video.getAttribute('src') || '';
        if (!src) return;
        // Never touch blob: or data: sources -- these are live MediaSource or
        // inline objects that cannot be detached and restored.  Stripping a
        // blob: src closes the MediaSource state machine permanently (LinkedIn,
        // YouTube, and any HLS/DASH player use blob: for all their video).
        if (src.startsWith('blob:') || src.startsWith('data:')) return;
        if (!video.dataset.xcmSrc) {
            video.dataset.xcmSrc = src;
            // preload=none prevents the browser reading video metadata/bytes
            // even without a src attribute.  Set it before clearing src.
            video.preload = 'none';
            video.removeAttribute('src');
            var sources = video.querySelectorAll('source[src]');
            for (var i = 0; i < sources.length; i++) {
                sources[i].dataset.xcmSrc = sources[i].src;
                sources[i].removeAttribute('src');
            }
        }
        if (!_videoIO) {
            _videoIO = new IntersectionObserver(function (entries) {
                entries.forEach(function (entry) {
                    if (!entry.isIntersecting) return;
                    var v = entry.target;
                    _videoIO.unobserve(v);
                    if (v.dataset.xcmSrc) { v.src = v.dataset.xcmSrc; delete v.dataset.xcmSrc; }
                    v.querySelectorAll('source[data-xcm-src]').forEach(function (s) {
                        s.src = s.dataset.xcmSrc; delete s.dataset.xcmSrc;
                    });
                    v.load();
                });
            }, { rootMargin: (Math.round(window.innerHeight * NEAR_VIEWPORT)) + 'px 0px', threshold: 0 });
        }
        _videoIO.observe(video);
    }

    // ── 3. Attribute helpers ─────────────────────────────────────────────────
    // _applyLazy: sets lazy/async/priority on individual elements.
    // _preserveAspectRatio: if an img has width+height attributes, copies them
    //   into CSS aspect-ratio so the document flow reserves the right space
    //   before the image loads -- prevents Cumulative Layout Shift (CLS) which
    //   manifests as a visible "jump" jank during scroll.
    function _preserveAspectRatio(el) {
        var w = el.getAttribute('width');
        var h = el.getAttribute('height');
        if (w && h && !el.style.aspectRatio) {
            el.style.aspectRatio = w + ' / ' + h;
            // Also ensure the element doesn't collapse to 0 while lazy.
            if (!el.style.minHeight) el.style.minHeight = '1px';
        }
    }

    function _applyLazy(el) {
        if (!el || !el.tagName) return;
        var t = el.tagName.toLowerCase();
        if (t === 'img') {
            if (!el.getAttribute('loading'))       el.setAttribute('loading',       'lazy');
            if (!el.getAttribute('decoding'))      el.setAttribute('decoding',      'async');
            if (!el.getAttribute('fetchpriority')) el.setAttribute('fetchpriority', 'low');
            _preserveAspectRatio(el);
        } else if (t === 'iframe') {
            if (!el.getAttribute('loading'))  el.setAttribute('loading',  'lazy');
            if (!el.getAttribute('decoding')) el.setAttribute('decoding', 'async');
        }
        if (t === 'video') _observeVideo(el);
    }

    // Chunked subtree walk -- yields when the idle deadline runs low.
    // window.__xcmIdleThreshold is set by ticker-inject.js to frameMs*0.3
    // (4.8ms at 60Hz, 2.4ms at 120Hz).  Falls back to 4ms if ticker has
    // not yet run (e.g. script injection order changed).
    function _applyToSubtreeIdle(root, deadline) {
        if (!root || !root.querySelectorAll) return;
        var els       = root.querySelectorAll('img, iframe, video');
        var i         = 0;
        var threshold = window.__xcmIdleThreshold || 4;
        function chunk() {
            while (i < els.length) {
                if (deadline && i % 20 === 0 && deadline.timeRemaining() < threshold) {
                    _schedule(function (d) { deadline = d; chunk(); });
                    return;
                }
                _applyLazy(els[i++]);
            }
        }
        chunk();
        _applyLazy(root);
    }

    // ── 4. MutationObserver -- queued, flushed off main thread ───────────────
    var _pending = [];
    var _flushScheduled = false;

    function _flush() {
        _flushScheduled = false;
        var batch = _pending.splice(0);
        _schedule(function (deadline) {
            for (var i = 0; i < batch.length; i++) {
                _applyToSubtreeIdle(batch[i], deadline);
            }
        });
    }

    var _mo = new MutationObserver(function (mutations) {
        for (var mi = 0; mi < mutations.length; mi++) {
            var added = mutations[mi].addedNodes;
            for (var ni = 0; ni < added.length; ni++) {
                var node = added[ni];
                if (node.nodeType !== 1) continue;
                _pending.push(node);
            }
        }
        if (!_flushScheduled) {
            _flushScheduled = true;
            // Defer until after the current render-commit phase.
            Promise.resolve().then(_flush);
        }
    });

    function _startObserver() {
        var root = document.documentElement || document.body;
        if (root) {
            _mo.observe(root, { childList: true, subtree: true });
        } else {
            document.addEventListener('DOMContentLoaded', function () {
                _mo.observe(document.documentElement, { childList: true, subtree: true });
            }, { once: true });
        }
    }
    _startObserver();

    // ── 5. Full DOM pass at DOMContentLoaded ────────────────────────────────
    // Also upgrades fetchpriority to 'high' for any images already in the
    // viewport at load time (above-fold).  Uses an IntersectionObserver with
    // a zero rootMargin so we only touch what the user can see right now.
    function _upgradePriority() {
        if (!document.body) return;
        var imgs = document.body.querySelectorAll('img[fetchpriority="low"]');
        var io = new IntersectionObserver(function (entries) {
            io.disconnect();
            entries.forEach(function (e) {
                if (e.isIntersecting) e.target.fetchPriority = 'high';
            });
        }, { rootMargin: '0px', threshold: 0 });
        for (var i = 0; i < imgs.length; i++) io.observe(imgs[i]);
    }

    function _fullPass() {
        _schedule(function (d) {
            _applyToSubtreeIdle(document.body || document.documentElement, d);
        });
        _upgradePriority();
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _fullPass, { once: true });
    } else {
        _fullPass();
    }

    // ── 6. SPA navigation rehook ─────────────────────────────────────────────
    (function () {
        var _op = history.pushState, _or = history.replaceState;
        function _onNav() { setTimeout(_fullPass, 250); }
        if (typeof _op === 'function') history.pushState    = function () { _op.apply(this, arguments);    _onNav(); };
        if (typeof _or === 'function') history.replaceState = function () { _or.apply(this, arguments); _onNav(); };
    })();

})();
