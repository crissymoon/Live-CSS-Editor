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

    // Deep-freeze (content-visibility:hidden) is DISABLED.
    // Setting 'hidden' causes scrollHeight to change when rows unfreeze
    // (their placeholder intrinsic-size differs from their real height),
    // which makes the scrollbar thumb jump during fast trackpad scrolling.
    // content-visibility:auto alone is sufficient -- WebKit culls
    // off-screen layout/paint inside its own compositor without the
    // hidden↔auto toggle, so no scrollHeight discontinuity occurs.
    var _freezeIO = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
            var el = e.target;
            if (e.isIntersecting) {
                _rendered.add(el);
                // Restore auto if it was somehow set to hidden externally.
                if (_frozen.has(el)) {
                    el.style.contentVisibility = 'auto';
                    _frozen.delete(el);
                }
            }
            // Never set 'hidden' -- would change scrollHeight on restore,
            // causing scrollbar thumb to snap/jump.
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

    /* Check whether a row contains expandable/collapsible interactive elements.
     * content-visibility creates a new stacking+layout containment context.
     * When a nested collapsible expands, its content can render underneath
     * surrounding elements or get clipped by the contain box.  Skip rows
     * that host any kind of expand/collapse widget. */
    function _hasExpandable(el) {
        return !!(
            el.querySelector('[aria-expanded],[aria-controls],[aria-haspopup],details,summary') ||
            el.matches('[aria-expanded],[aria-controls],[aria-haspopup],details,summary')
        );
    }

    function _safeObserve(el) {
        if (_hasStickyChild(el) || _hasExpandable(el)) return;
        // Apply content-visibility:auto via JS (not CSS) so only safe rows get
        // the containment context.  Rows with expandable children never get it.
        el.style.contentVisibility = 'auto';
        _freezeIO.observe(el);
    }

    function _observeRows() {
        if (!document.body) return;
        var rows = document.body.querySelectorAll(CHILD_SEL);
        for (var i = 0; i < rows.length; i++) {
            _safeObserve(rows[i]);
        }
    }

    /* ---------- Expandable / collapsible repair ----------
     * content-visibility:auto/hidden establishes layout+stacking containment.
     * When a collapsible inside a virtualized row is clicked or opened, the
     * expanding content can render underneath surrounding elements or be
     * clipped by the contain box.  Clear contentVisibility from the nearest
     * virtualized ancestor on any click or aria-expanded change so the row
     * is no longer contained while the interaction plays out. */
    function _clearAncestorCV(el) {
        var node = el;
        while (node && node !== document.body) {
            if (node.style && node.style.contentVisibility) {
                node.style.contentVisibility = '';
                _frozen.delete(node);
            }
            node = node.parentElement;
        }
    }

    document.addEventListener('click', function (e) {
        _clearAncestorCV(e.target);
    }, true);

    /* Watch for aria-expanded and <details open> attribute changes -- the
     * page's own JS sets these when an accordion or disclosure widget opens. */
    var _ariaObs = new MutationObserver(function (mutations) {
        for (var mi = 0; mi < mutations.length; mi++) {
            var m = mutations[mi];
            if (m.attributeName === 'aria-expanded' || m.attributeName === 'open') {
                _clearAncestorCV(m.target);
            }
        }
    });
    _ariaObs.observe(document.documentElement || document.body, {
        attributes: true,
        attributeFilter: ['aria-expanded', 'open'],
        subtree: true
    });

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
                    _safeObserve(node);
                }
                if (node.querySelectorAll) {
                    var kids = node.querySelectorAll(CHILD_SEL);
                    for (var ki = 0; ki < kids.length; ki++) {
                        _safeObserve(kids[ki]);
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
