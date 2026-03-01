/**
 * page-builder/js/pb-responsive.js
 *
 * Element-query responsive engine for page-builder previews.
 * Uses ResizeObserver (per-element width, not viewport) so behavior
 * is accurate even when the editor panel is open and shrinking available space.
 *
 * What it does:
 *   - Watches every [data-pb-section] element with ResizeObserver
 *   - Collapses multi-column grids (.pb-grid) when container width falls
 *     below the section's configured collapseAt threshold
 *   - Applies named breakpoint classes (pb-xs / pb-sm / pb-md / pb-lg) to
 *     every section so CSS rules can key off them
 *   - Inserts a hamburger toggle button in the header and collapses the nav
 *     when the header width falls below its navCollapseAt threshold
 *   - Updates the #pb-breakpoint badge in the watcher toolbar
 *   - Logs all decisions to console under [PBResponsive] prefix
 *
 * Config is read from HTML attributes set by build.php:
 *   data-pb-responsive = JSON  e.g. {"collapseAt":640}
 *   data-pb-nav-collapse = number (px)  e.g. "600"
 *
 * Does NOT require PB_CONFIG - safe to load on any page.
 */
;(function () {
    'use strict';

    var PREFIX = '[PBResponsive]';
    var log  = function () { var a = Array.prototype.slice.call(arguments); a.unshift(PREFIX); console.log.apply(console, a); };
    var warn = function () { var a = Array.prototype.slice.call(arguments); a.unshift(PREFIX); console.warn.apply(console, a); };
    var err  = function () { var a = Array.prototype.slice.call(arguments); a.unshift(PREFIX); console.error.apply(console, a); };

    // ---- Named breakpoints applied as classes to every section ----
    var BREAKPOINTS = [
        { name: 'pb-xs', max: 480  },
        { name: 'pb-sm', max: 640  },
        { name: 'pb-md', max: 900  },
        { name: 'pb-lg', max: 1200 },
    ];
    var ALL_BP_CLASSES = BREAKPOINTS.map(function (b) { return b.name; }).concat(['pb-xl']);

    // ---- Named labels for the toolbar badge (keyed off window width) ----
    var BADGE_BREAKPOINTS = [
        { label: 'xs',  max: 480  },
        { label: 'sm',  max: 640  },
        { label: 'md',  max: 900  },
        { label: 'lg',  max: 1200 },
        { label: 'xl',  max: Infinity },
    ];

    // -------------------------------------------------------------------
    //  Parse a data-pb-responsive attribute safely
    // -------------------------------------------------------------------
    function parseResponsiveConfig(el) {
        var raw = el.getAttribute('data-pb-responsive');
        if (!raw) return {};
        try {
            var cfg = JSON.parse(raw);
            return (cfg && typeof cfg === 'object') ? cfg : {};
        } catch (e) {
            warn('parseResponsiveConfig: invalid JSON on', el, '-', e);
            return {};
        }
    }

    // -------------------------------------------------------------------
    //  Apply breakpoint classes to a section based on its current width
    // -------------------------------------------------------------------
    function applySectionClasses(el, width) {
        ALL_BP_CLASSES.forEach(function (cls) { el.classList.remove(cls); });

        var applied = 'pb-xl';
        for (var i = 0; i < BREAKPOINTS.length; i++) {
            if (width <= BREAKPOINTS[i].max) { applied = BREAKPOINTS[i].name; break; }
        }
        el.classList.add(applied);
    }

    // -------------------------------------------------------------------
    //  Collapse / restore grid inside a section
    // -------------------------------------------------------------------
    function updateSectionCollapse(el, width, collapseAt) {
        var grid = el.querySelector('.pb-grid');
        if (!grid) return;

        var collapsed = width <= collapseAt;
        if (collapsed) {
            if (!el.classList.contains('pb-collapsed')) {
                el.classList.add('pb-collapsed');
                log('collapsed grid in #' + (el.id || el.dataset.pbSection), 'at', Math.round(width) + 'px', '(threshold:', collapseAt + 'px)');
            }
        } else {
            if (el.classList.contains('pb-collapsed')) {
                el.classList.remove('pb-collapsed');
                log('restored grid in #' + (el.id || el.dataset.pbSection), 'at', Math.round(width) + 'px');
            }
        }
    }

    // -------------------------------------------------------------------
    //  Observe a single section element
    // -------------------------------------------------------------------
    function observeSection(el) {
        var cfg = parseResponsiveConfig(el);
        var collapseAt = typeof cfg.collapseAt === 'number' ? cfg.collapseAt : null;

        // Run immediately with current size before the ResizeObserver fires
        try {
            var initialWidth = el.getBoundingClientRect().width;
            applySectionClasses(el, initialWidth);
            if (collapseAt !== null) updateSectionCollapse(el, initialWidth, collapseAt);
        } catch (e) {
            err('observeSection initial measure failed for', el, '-', e);
        }

        try {
            var ro = new ResizeObserver(function (entries) {
                try {
                    var width = entries[0].contentRect.width;
                    applySectionClasses(el, width);
                    if (collapseAt !== null) updateSectionCollapse(el, width, collapseAt);
                } catch (e2) {
                    err('ResizeObserver callback failed for section', el, '-', e2);
                }
            });
            ro.observe(el);
        } catch (e) {
            err('observeSection: ResizeObserver setup failed for', el, '-', e);
        }
    }

    // -------------------------------------------------------------------
    //  Hamburger nav for header
    // -------------------------------------------------------------------
    function buildHamburger(header) {
        var existing = header.querySelector('.pb-hamburger');
        if (existing) return existing;

        try {
            var btn = document.createElement('button');
            btn.className = 'pb-hamburger';
            btn.setAttribute('aria-label', 'Toggle navigation');
            btn.setAttribute('aria-expanded', 'false');
            btn.innerHTML = '<span></span><span></span><span></span>';
            btn.addEventListener('click', function () {
                try {
                    var open = header.classList.toggle('pb-nav-open');
                    btn.setAttribute('aria-expanded', open ? 'true' : 'false');
                    log('hamburger toggled:', open ? 'open' : 'closed');
                } catch (e) {
                    err('hamburger click handler failed:', e);
                }
            });

            // Insert hamburger before the closing tag of the inner div
            var inner = header.querySelector('div');
            if (inner) {
                inner.appendChild(btn);
            } else {
                header.appendChild(btn);
                warn('observeHeader: no inner div found in header, appended hamburger directly');
            }
            return btn;
        } catch (e) {
            err('buildHamburger failed:', e);
            return null;
        }
    }

    function observeHeader(header) {
        var raw = header.getAttribute('data-pb-nav-collapse');
        var navCollapseAt = raw ? parseFloat(raw) : 600;
        if (isNaN(navCollapseAt)) {
            warn('observeHeader: data-pb-nav-collapse="' + raw + '" is not a number, using 600');
            navCollapseAt = 600;
        }

        /* Sync --pb-nav-bg CSS custom property from the header's actual computed
           background so the mobile nav panel always matches the header color.
           This handles pages already built before --pb-nav-bg was stamped inline. */
        try {
            if (!header.style.getPropertyValue('--pb-nav-bg')) {
                var computedBg = window.getComputedStyle(header).backgroundColor;
                if (computedBg && computedBg !== 'rgba(0, 0, 0, 0)' && computedBg !== 'transparent') {
                    header.style.setProperty('--pb-nav-bg', computedBg);
                    log('observeHeader: synced --pb-nav-bg from computed background:', computedBg);
                } else {
                    /* Fall back to inline background if computed is transparent */
                    var inlineBg = header.style.background || header.style.backgroundColor;
                    if (inlineBg) {
                        header.style.setProperty('--pb-nav-bg', inlineBg);
                        log('observeHeader: synced --pb-nav-bg from inline style:', inlineBg);
                    } else {
                        warn('observeHeader: could not determine header background for --pb-nav-bg');
                    }
                }
            }
        } catch (e) {
            err('observeHeader: --pb-nav-bg sync failed:', e);
        }

        var hamburger = buildHamburger(header);

        function update(width) {
            try {
                var small = width <= navCollapseAt;
                if (small) {
                    header.classList.add('pb-nav-collapsed');
                } else {
                    header.classList.remove('pb-nav-collapsed');
                    // Always close the menu when going back to wide
                    if (header.classList.contains('pb-nav-open')) {
                        header.classList.remove('pb-nav-open');
                        if (hamburger) hamburger.setAttribute('aria-expanded', 'false');
                        log('observeHeader: nav auto-closed (width restored to', Math.round(width) + 'px)');
                    }
                }
            } catch (e) {
                err('observeHeader update failed:', e);
            }
        }

        try {
            update(header.getBoundingClientRect().width);
        } catch (e) {
            err('observeHeader initial measure failed:', e);
        }

        try {
            var ro = new ResizeObserver(function (entries) {
                try {
                    update(entries[0].contentRect.width);
                } catch (e2) {
                    err('ResizeObserver callback failed for header:', e2);
                }
            });
            ro.observe(header);
        } catch (e) {
            err('observeHeader: ResizeObserver setup failed:', e);
        }
    }

    // -------------------------------------------------------------------
    //  Toolbar breakpoint badge (updates on window resize)
    // -------------------------------------------------------------------
    function initBreakpointBadge() {
        try {
            var target = document.getElementById('pb-breakpoint');
            if (!target) {
                warn('initBreakpointBadge: #pb-breakpoint not found in toolbar - skipping badge');
                return;
            }

            function updateBadge() {
                try {
                    var w = window.innerWidth;
                    var label = 'xl';
                    for (var i = 0; i < BADGE_BREAKPOINTS.length; i++) {
                        if (w <= BADGE_BREAKPOINTS[i].max) { label = BADGE_BREAKPOINTS[i].label; break; }
                    }
                    target.textContent = label + ' (' + w + 'px)';
                    target.setAttribute('data-bp', label);
                } catch (e) {
                    err('updateBadge failed:', e);
                }
            }

            updateBadge();
            window.addEventListener('resize', updateBadge);
        } catch (e) {
            err('initBreakpointBadge failed:', e);
        }
    }

    // -------------------------------------------------------------------
    //  Check ResizeObserver availability
    // -------------------------------------------------------------------
    if (typeof ResizeObserver === 'undefined') {
        err('ResizeObserver is not supported in this browser. Responsive preview engine will not run.');
        return;
    }

    // -------------------------------------------------------------------
    //  Boot
    // -------------------------------------------------------------------
    function init() {
        try {
            var sections = document.querySelectorAll('[data-pb-section]');
            if (!sections.length) {
                warn('init: no [data-pb-section] elements found on this page');
                return;
            }

            var headerEl = null;
            sections.forEach(function (el) {
                try {
                    if (el.tagName === 'HEADER' || el.getAttribute('data-pb-section') === 'header') {
                        headerEl = el;
                        observeHeader(el);
                    } else {
                        observeSection(el);
                    }
                } catch (e) {
                    err('init: failed to set up observer for', el, '-', e);
                }
            });

            initBreakpointBadge();

            log('init complete -', sections.length, 'section(s) observed',
                headerEl ? '(including header with hamburger nav)' : '(no header found)');

        } catch (e) {
            err('init failed:', e);
        }
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    // Public handle for debugging in console
    window.PBResponsive = {
        reinit: init,
        breakpoints: BREAKPOINTS,
    };

}());
