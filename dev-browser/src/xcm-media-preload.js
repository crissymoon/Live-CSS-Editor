/**
 * xcm-media-preload.js
 *
 * Eliminates the blank-frame flash on image/video-heavy pages during scroll.
 *
 * Strategy: two IntersectionObserver rings around the viewport.
 *
 *   Ring 1 -- PRELOAD ring (large margin, ~3 viewport heights each direction)
 *     Images:  remove loading=lazy, set fetchPriority=high so the browser
 *              starts the network fetch before the row is visible.
 *     Videos:  set preload=metadata so the browser reads the first keyframe
 *              and dimensions without pulling the full stream.
 *
 *   Ring 2 -- NEAR ring (~1 viewport each direction)
 *     Images that are almost on screen: fetchPriority=high + attempt to
 *              pre-decode via createImageBitmap so they are GPU-ready.
 *     Videos:  preload=auto so buffering starts while the user is still
 *              a scroll-height away.
 *
 * Works alongside lazy-inject.js -- lazy-inject defers fetch, this module
 * pulls it forward on a generous schedule.
 *
 * Injected at document-start by imgui-browser / main.mm.
 */
(function () {
    'use strict';

    if (window.__xcmMediaPreloadInstalled) return;
    window.__xcmMediaPreloadInstalled = true;

    var VH = window.innerHeight || 800;

    // Ring margins as pixel strings (top bottom, no left/right effect needed).
    var PRELOAD_MARGIN = Math.round(VH * 3) + 'px 0px';
    var NEAR_MARGIN    = Math.round(VH * 1) + 'px 0px';

    // ── Helpers ──────────────────────────────────────────────────────────────

    // Return the real src for an img, accounting for lazy-inject stashing it
    // in data-xcm-src or data-src / data-lazy-src conventions.
    function _imgSrc(el) {
        return el.getAttribute('src') ||
               el.dataset.xcmSrc      ||
               el.dataset.src         ||
               el.dataset.lazySrc     ||
               el.dataset.original    || '';
    }

    // Upgrade a lazy image to eager + high priority.
    function _upgradeImg(img) {
        if (img.dataset.xcmPreload) return;
        img.dataset.xcmPreload = '1';

        // If lazy-inject stashed the real src, restore it first.
        var realSrc = img.dataset.xcmSrc || img.dataset.src || '';
        if (realSrc && !img.getAttribute('src')) {
            img.src = realSrc;
            if (img.dataset.xcmSrc) delete img.dataset.xcmSrc;
        }

        img.setAttribute('loading', 'eager');
        img.fetchPriority = 'high';
    }

    // Pre-decode an image that is about to enter the viewport.
    function _nearImg(img) {
        if (img.dataset.xcmNear) return;
        img.dataset.xcmNear = '1';
        _upgradeImg(img);   // ensure eager + high priority first

        var src = _imgSrc(img);
        if (!src || src.startsWith('data:')) return;
        if (typeof createImageBitmap === 'function') {
            var tmp = new Image();
            tmp.crossOrigin = 'anonymous';
            tmp.src = src;
            tmp.decode().catch(function () {});  // fire-and-forget
        }
    }

    // Upgrade a video in the preload ring.
    function _upgradeVideo(video) {
        if (video.dataset.xcmPreload) return;
        video.dataset.xcmPreload = '1';

        // Restore stashed src from lazy-inject.
        if (video.dataset.xcmSrc) {
            video.src = video.dataset.xcmSrc;
            delete video.dataset.xcmSrc;
            video.querySelectorAll('source[data-xcm-src]').forEach(function (s) {
                s.src = s.dataset.xcmSrc;
                delete s.dataset.xcmSrc;
            });
        }
        if (video.preload === 'none' || !video.preload) {
            video.preload = 'metadata';
        }
    }

    // Warm a video that is nearly on screen.
    function _nearVideo(video) {
        if (video.dataset.xcmNear) return;
        video.dataset.xcmNear = '1';
        _upgradeVideo(video);
        // Upgrade from metadata to auto -- buffer a few seconds of content.
        if (video.preload !== 'auto') {
            video.preload = 'auto';
        }
    }

    // ── Observers ────────────────────────────────────────────────────────────

    var _preloadIO = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
            if (!e.isIntersecting) return;
            var el = e.target;
            var tag = el.tagName.toLowerCase();
            _preloadIO.unobserve(el);
            if (tag === 'img')   _upgradeImg(el);
            if (tag === 'video') _upgradeVideo(el);
        });
    }, { rootMargin: PRELOAD_MARGIN, threshold: 0 });

    var _nearIO = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
            if (!e.isIntersecting) return;
            var el = e.target;
            var tag = el.tagName.toLowerCase();
            _nearIO.unobserve(el);
            if (tag === 'img')   _nearImg(el);
            if (tag === 'video') _nearVideo(el);
        });
    }, { rootMargin: NEAR_MARGIN, threshold: 0 });

    // ── DOM scan ─────────────────────────────────────────────────────────────

    function _scan(root) {
        var imgs   = (root || document).querySelectorAll('img');
        var videos = (root || document).querySelectorAll('video');
        for (var i = 0; i < imgs.length; i++) {
            _preloadIO.observe(imgs[i]);
            _nearIO.observe(imgs[i]);
        }
        for (var j = 0; j < videos.length; j++) {
            _preloadIO.observe(videos[j]);
            _nearIO.observe(videos[j]);
        }
    }

    // Initial scan after the DOM is ready.
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function () { _scan(); }, { once: true });
    } else {
        _scan();
    }

    // Watch for dynamically added images/videos (infinite-scroll, SPA routes).
    var _mo = new MutationObserver(function (mutations) {
        mutations.forEach(function (m) {
            m.addedNodes.forEach(function (node) {
                if (node.nodeType !== 1) return;
                var tag = node.tagName ? node.tagName.toLowerCase() : '';
                if (tag === 'img' || tag === 'video') {
                    _preloadIO.observe(node);
                    _nearIO.observe(node);
                } else {
                    // Subtree additions (e.g. a card div containing images).
                    var imgs   = node.querySelectorAll ? node.querySelectorAll('img')   : [];
                    var videos = node.querySelectorAll ? node.querySelectorAll('video') : [];
                    for (var i = 0; i < imgs.length; i++) {
                        _preloadIO.observe(imgs[i]);
                        _nearIO.observe(imgs[i]);
                    }
                    for (var j = 0; j < videos.length; j++) {
                        _preloadIO.observe(videos[j]);
                        _nearIO.observe(videos[j]);
                    }
                }
            });
        });
    });

    // Start observing as soon as body exists.
    function _attach() {
        if (document.body) {
            _mo.observe(document.body, { childList: true, subtree: true });
        } else {
            setTimeout(_attach, 50);
        }
    }
    _attach();

})();
