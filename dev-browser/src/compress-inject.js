/**
 * compress-inject.js  v2
 *
 * Two-path image compression:
 *
 * Path A -- Local proxy (primary)
 *   Sends a GET to http://127.0.0.1:7779/img?url=...&w=...&h=...
 *   The image-cache-server handles fetch + sharp resize/WebP + Redis cache.
 *   Because the request is same-origin (localhost), cross-origin CORS is not
 *   a constraint. Proxy URL is read from window.__xcmImgProxy set by _profile.py.
 *   Browser-side caching: the response carries Cache-Control: max-age=<ttl>.
 *   We also keep a sessionStorage Map (key = original src, value = blob URL)
 *   so the same image is not proxied twice within the same page session.
 *
 * Path B -- OffscreenCanvas fallback
 *   Used when the proxy is unavailable (server not started, 502, timeout).
 *   Works only for same-origin or CORS-enabled cross-origin images.
 *   Silently skipped on SecurityError (cross-origin tainted canvas).
 *
 * In both paths, processing is deferred to requestIdleCallback (or
 * MessageChannel fallback) to stay off the main thread.
 *
 * Injected at DocumentReady.
 */
(function () {
    'use strict';

    var PROXY_BASE   = (window.__xcmImgProxy || 'http://127.0.0.1:7779');
    var MAX_RATIO    = 2.0;
    var WEBP_QUALITY = 0.82;   // fallback OffscreenCanvas quality
    var DPR          = window.devicePixelRatio || 1;
    var FETCH_TIMEOUT_MS = 8000;

    var _schedule;
    if (typeof requestIdleCallback === 'function') {
        _schedule = function (fn) { requestIdleCallback(fn, { timeout: 2000 }); };
    } else {
        _schedule = function (fn) { setTimeout(fn, 200); };
    }

    // ── Session cache: original src -> blob URL ──────────────────────────────
    // Stored in a plain Map (not sessionStorage -- avoids serialization cost).
    var _blobCache = new Map();

    function _applyBlob(img, blobUrl) {
        var old = img.src;
        img.src = blobUrl;
        // Revoke our previous blob URL for this image if we made one.
        if (old && old.startsWith('blob:') && old !== blobUrl) {
            URL.revokeObjectURL(old);
        }
    }

    // ── Path A: proxy ────────────────────────────────────────────────────────
    function _proxyCompress(img, rw, rh) {
        var src = img.src || '';
        if (!src || src.startsWith('data:') || src.startsWith('blob:')) return Promise.reject(new Error('skip'));

        // Return cached blob immediately if we have it.
        if (_blobCache.has(src)) {
            _applyBlob(img, _blobCache.get(src));
            return Promise.resolve('cached');
        }

        var cw = Math.round(rw * DPR);
        var ch = Math.round(rh * DPR);

        var proxyUrl = PROXY_BASE + '/img'
            + '?url=' + encodeURIComponent(src)
            + '&w=' + cw
            + '&h=' + ch;

        var controller = typeof AbortController !== 'undefined' ? new AbortController() : null;
        var timer = controller
            ? setTimeout(function () { controller.abort(); }, FETCH_TIMEOUT_MS)
            : null;

        return fetch(proxyUrl, {
            signal: controller ? controller.signal : undefined,
            // Ask the browser to use its own HTTP cache for the proxy response.
            // The proxy sets Cache-Control: public, max-age=<ttl> and the browser
            // will serve from disk cache on subsequent requests without hitting
            // the proxy at all.
            cache: 'default',
        })
        .then(function (r) {
            if (timer) clearTimeout(timer);
            if (!r.ok) throw new Error('proxy ' + r.status);
            return r.blob();
        })
        .then(function (blob) {
            var blobUrl = URL.createObjectURL(blob);
            _blobCache.set(src, blobUrl);
            _applyBlob(img, blobUrl);
            return 'proxied';
        })
        .catch(function (err) {
            if (timer) clearTimeout(timer);
            throw err;  // bubble to caller so Path B fires
        });
    }

    // ── Path B: OffscreenCanvas fallback ─────────────────────────────────────
    function _canvasCompress(img, rw, rh) {
        return new Promise(function (resolve, reject) {
            var cw = Math.round(rw * DPR);
            var ch = Math.round(rh * DPR);
            try {
                var oc  = new OffscreenCanvas(cw, ch);
                var ctx = oc.getContext('2d');
                ctx.drawImage(img, 0, 0, cw, ch);
                oc.convertToBlob({ type: 'image/webp', quality: WEBP_QUALITY })
                    .then(function (blob) {
                        var src     = img.src;
                        var blobUrl = URL.createObjectURL(blob);
                        _blobCache.set(src, blobUrl);
                        _applyBlob(img, blobUrl);
                        resolve('canvas');
                    })
                    .catch(reject);
            } catch (e) {
                reject(e);
            }
        });
    }

    // ── Main compress entry ───────────────────────────────────────────────────
    function _compress(img) {
        var rw = img.offsetWidth  || 0;
        var rh = img.offsetHeight || 0;
        if (rw < 4 || rh < 4) return;

        var nw = img.naturalWidth  || 0;
        var nh = img.naturalHeight || 0;
        if (nw < 4 || nh < 4) return;

        var displayPx = rw * DPR * rh * DPR;
        var naturalPx = nw * nh;
        if (naturalPx <= displayPx * MAX_RATIO) return;

        var src = img.src || '';
        if (!src || src.startsWith('data:') || src.startsWith('blob:')) return;

        // Return early if already processed this src in this session.
        if (_blobCache.has(src)) {
            if (img.src !== _blobCache.get(src)) _applyBlob(img, _blobCache.get(src));
            return;
        }

        _proxyCompress(img, rw, rh)
            .catch(function () {
                // Proxy unavailable or timed out -- try OffscreenCanvas.
                return _canvasCompress(img, rw, rh);
            })
            .catch(function () {
                // Both paths failed (cross-origin without CORS, no proxy).
                // Leave the image untouched.
            });
    }

    // ── Idle-time drain queue ────────────────────────────────────────────────
    var _queue     = [];
    var _scheduled = false;

    function _drain(deadline) {
        _scheduled = false;
        var threshold = window.__xcmIdleThreshold || 4;
        while (_queue.length) {
            if (deadline && deadline.timeRemaining() < threshold) {
                _scheduled = true;
                _schedule(_drain);
                return;
            }
            _compress(_queue.shift());
        }
    }

    function _enqueue(img) {
        if (_blobCache.has(img.src)) return;  // already done
        _queue.push(img);
        if (!_scheduled) {
            _scheduled = true;
            _schedule(_drain);
        }
    }

    // ── IntersectionObserver: only process visible images ────────────────────
    var _loadIO = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
            if (!e.isIntersecting) return;
            _loadIO.unobserve(e.target);
            var img = e.target;
            if (img.complete && img.naturalWidth > 0) {
                _enqueue(img);
            } else {
                img.addEventListener('load', function () { _enqueue(img); }, { once: true });
            }
        });
    }, { rootMargin: '200px 0px', threshold: 0 });

    function _observe(img) {
        var w = img.getAttribute('width');
        if (w && parseInt(w, 10) < 32) return;
        _loadIO.observe(img);
    }

    // ── DOM scan and MutationObserver ────────────────────────────────────────
    function _scanDOM() {
        if (!document.body) return;
        var imgs = document.body.querySelectorAll('img');
        for (var i = 0; i < imgs.length; i++) _observe(imgs[i]);
    }

    var _mo = new MutationObserver(function (mutations) {
        for (var mi = 0; mi < mutations.length; mi++) {
            var added = mutations[mi].addedNodes;
            for (var ni = 0; ni < added.length; ni++) {
                var node = added[ni];
                if (node.nodeType !== 1) continue;
                var tag = (node.tagName || '').toLowerCase();
                if (tag === 'img') {
                    _observe(node);
                } else if (node.querySelectorAll) {
                    var imgs = node.querySelectorAll('img');
                    for (var ii = 0; ii < imgs.length; ii++) _observe(imgs[ii]);
                }
            }
        }
    });

    document.addEventListener('DOMContentLoaded', function () {
        _scanDOM();
        if (document.body) _mo.observe(document.body, { childList: true, subtree: true });
    }, { once: true });

    /* SPA hook */
    (function () {
        var _op = history.pushState, _or = history.replaceState;
        function _onNav() { setTimeout(_scanDOM, 600); }
        if (typeof _op === 'function') history.pushState    = function () { _op.apply(this, arguments);    _onNav(); };
        if (typeof _or === 'function') history.replaceState = function () { _or.apply(this, arguments); _onNav(); };
    })();

})();
