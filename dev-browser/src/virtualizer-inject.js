/**
 * virtualizer-inject.js -- CSS-native row virtualization.  v2.
 *
 * content-visibility:auto  -- Chromium skips layout, paint, raster for
 *   off-screen rows.  Zero JS per scroll frame.
 *
 * Deep-freeze: content-visibility:hidden applied ONLY to rows that have
 *   been rendered at least once and then scrolled > 4 viewport heights away.
 *   v1 bug: the IO fired on first observation, setting 'hidden' on rows that
 *   had never been rendered, causing blank-row flashes on fast scroll.
 *
 * Sticky guard: rows containing position:sticky children are excluded from
 *   content-visibility:auto because the spec forbids sticky from escaping a
 *   content-visibility contain box.
 *
 * contain-intrinsic-size: increased from 280px to 400px -- a better average
 *   for Twitter/Reddit/LinkedIn cards, reducing scrollbar jump on first pass.
 *   'auto' prefix means Chromium caches real height after first render.
 *
 * Injected at DocumentCreation.  Heavy work deferred to DOMContentLoaded.
 */
(function () {
    'use strict';

    var ROW_SEL = [
        'article',
        '[role="article"]',
        '[data-testid="cellInnerDiv"]',
        'shreddit-post',
        '.Post',
        '[data-testid="post-container"]',
        '.feed-shared-update-v2',
        '.occludable-update',
        'ytd-rich-item-renderer',
        'ytd-video-renderer',
        'ytd-compact-video-renderer',
        'ytd-shelf-renderer',
        '.athing',
        '[data-pagelet*="FeedUnit"]'
    ].join(',');

    var CSS = '/* xcm-virtualizer */\n' +
        ROW_SEL + ' {\n' +
        '  content-visibility: auto;\n' +
        '  contain-intrinsic-size: auto 400px;\n' +
        '}\n';

    function _injectCSS() {
        if (document.getElementById('xcm-virtualizer-css')) return;
        var s = document.createElement('style');
        s.id = 'xcm-virtualizer-css';
        s.textContent = CSS;
        (document.head || document.documentElement).appendChild(s);
    }

    /* Deep-freeze: hide rows > 4 viewport heights away -- but ONLY after
     * the row has been rendered at least once.  On first observation every
     * row outside the margin fires as 'not intersecting'.  Without the
     * _rendered guard that would immediately set hidden on unrendered rows,
     * causing blank-box flashes when the user scrolls to them. */
    var _MARGIN   = Math.max(window.innerHeight * 4, 1600);
    var _frozen   = new WeakSet();
    var _rendered = new WeakSet();

    var _freezeIO = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
            var el = e.target;
            if (e.isIntersecting) {
                // Row entered the 4x margin -- mark as rendered, restore if frozen.
                _rendered.add(el);
                if (_frozen.has(el)) {
                    el.style.contentVisibility = 'auto';
                    _frozen.delete(el);
                }
            } else {
                // Row left the 4x margin -- only freeze if it has been rendered before.
                if (_rendered.has(el)) {
                    el.style.contentVisibility = 'hidden';
                    _frozen.add(el);
                }
                // If not yet rendered, content-visibility:auto (from CSS) already
                // skips layout+paint for us -- no need to set 'hidden'.
            }
        });
    }, { rootMargin: _MARGIN + 'px 0px', threshold: 0 });

    var CHILD_SEL = [
        'article',
        'shreddit-post',
        '[data-testid="cellInnerDiv"]',
        'ytd-rich-item-renderer',
        'ytd-video-renderer',
        '.Post',
        '.feed-shared-update-v2',
        '.occludable-update',
        '.athing'
    ].join(',');

    /* Check whether a row contains a position:sticky descendant.
     * content-visibility:auto creates a new stacking/layout context that
     * prevents sticky from escaping the row box -- skip those rows. */
    function _hasStickyChild(el) {
        return !!el.querySelector(
            '[style*="sticky"], [class*="sticky"], [class*="fixed"]'
        );
    }

    function _observeRows() {
        if (!document.body) return;
        var rows = document.body.querySelectorAll(CHILD_SEL);
        for (var i = 0; i < rows.length; i++) {
            if (!_hasStickyChild(rows[i])) _freezeIO.observe(rows[i]);
        }
    }

    /* Watch for rows added by infinite-scroll JS */
    var _mo = new MutationObserver(function (mutations) {
        for (var mi = 0; mi < mutations.length; mi++) {
            var added = mutations[mi].addedNodes;
            for (var ni = 0; ni < added.length; ni++) {
                var node = added[ni];
                if (node.nodeType !== 1) continue;
                var tag = (node.tagName || '').toLowerCase();
                var cl  = node.classList || { contains: function () { return false; } };
                if (tag === 'article' || tag === 'shreddit-post' ||
                    tag === 'ytd-rich-item-renderer' || tag === 'ytd-video-renderer' ||
                    cl.contains('Post') || cl.contains('feed-shared-update-v2') ||
                    cl.contains('occludable-update') || cl.contains('athing')) {
                    if (!_hasStickyChild(node)) _freezeIO.observe(node);
                }
                if (node.querySelectorAll) {
                    var kids = node.querySelectorAll(CHILD_SEL);
                    for (var ki = 0; ki < kids.length; ki++) {
                        if (!_hasStickyChild(kids[ki])) _freezeIO.observe(kids[ki]);
                    }
                }
            }
        }
    });

    function _init() {
        _injectCSS();
        _observeRows();
        var root = document.documentElement || document.body;
        if (root) _mo.observe(root, { childList: true, subtree: true });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _init, { once: true });
    } else {
        _init();
    }

    /* SPA navigation rehook */
    (function () {
        var _op = history.pushState, _or = history.replaceState;
        function _onNav() { setTimeout(_observeRows, 400); }
        if (typeof _op === 'function') {
            history.pushState    = function () { _op.apply(this, arguments);    _onNav(); };
        }
        if (typeof _or === 'function') {
            history.replaceState = function () { _or.apply(this, arguments); _onNav(); };
        }
    })();

})();
