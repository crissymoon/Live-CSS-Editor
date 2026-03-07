/**
 * xcm-app-helper.js — injected at document-start into every WKWebView tab.
 *
 * Responsibilities:
 *   1. Select normalisation — strips WKWebView's native macOS system appearance
 *      from every <select> so hosted apps can fully style them with CSS.
 *   2. ARIA hygiene — adds missing aria-label / aria-expanded / role attributes
 *      to common interactive elements using a non-destructive, idempotent pass.
 *
 * Pattern: same IIFE + __xcm guard used by other xcm-* scripts.
 * Safe to load on any page; bails instantly if already initialised.
 */
(function (global) {
    'use strict';

    if (global.__xcmAppHelper) return;
    global.__xcmAppHelper = true;

    /* -----------------------------------------------------------------------
     * 1. SELECT NORMALISATION
     * WKWebView renders <select> as a native macOS popup button by default,
     * overriding any custom CSS. Forcing -webkit-appearance:none lets the page
     * own the full look-and-feel.
     * --------------------------------------------------------------------- */

    function normaliseSelect(el) {
        if (el.__xcmNormalised) return;
        el.__xcmNormalised = true;
        el.style.setProperty('-webkit-appearance', 'none', 'important');
        el.style.setProperty('appearance',         'none', 'important');
        // Preserve the element's own border-box sizing so it doesn't collapse.
        if (!el.style.boxSizing) {
            el.style.setProperty('box-sizing', 'border-box');
        }
    }

    function normaliseAllSelects(root) {
        (root || document).querySelectorAll('select').forEach(normaliseSelect);
    }

    /* -----------------------------------------------------------------------
     * 2. ARIA HYGIENE
     * Non-destructive: only adds attributes that are absent; never overwrites.
     * --------------------------------------------------------------------- */

    /**
     * Derive a human-readable label for a form control:
     * priority: aria-label > aria-labelledby text > <label for=> > title > placeholder > name
     */
    function hasLabel(el) {
        return el.hasAttribute('aria-label') ||
               el.hasAttribute('aria-labelledby') ||
               el.hasAttribute('aria-describedby');
    }

    function findLabelText(el) {
        // Explicit <label for="id">
        if (el.id) {
            const lbl = document.querySelector('label[for="' + el.id + '"]');
            if (lbl) return (lbl.textContent || '').trim();
        }
        // Wrapping <label>
        const wrap = el.closest('label');
        if (wrap) {
            // Text without the input's own value mixed in
            const clone = wrap.cloneNode(true);
            clone.querySelectorAll('input,select,textarea,button').forEach(n => n.remove());
            const t = (clone.textContent || '').trim();
            if (t) return t;
        }
        return el.getAttribute('title') ||
               el.getAttribute('placeholder') ||
               el.getAttribute('name') ||
               null;
    }

    function applyAriaLabel(el) {
        if (hasLabel(el)) return;
        const text = findLabelText(el);
        if (text) el.setAttribute('aria-label', text);
    }

    function applyAriaOnElement(el) {
        const tag = el.tagName;

        // --- <select> ---
        if (tag === 'SELECT') {
            applyAriaLabel(el);
            if (!el.hasAttribute('role')) el.setAttribute('role', 'combobox');
        }

        // --- <input> ---
        if (tag === 'INPUT') {
            applyAriaLabel(el);
            const type = (el.getAttribute('type') || 'text').toLowerCase();
            if (!el.hasAttribute('role')) {
                if (type === 'checkbox') el.setAttribute('role', 'checkbox');
                else if (type === 'radio') el.setAttribute('role', 'radio');
                else if (type === 'range') el.setAttribute('role', 'slider');
                else if (type === 'color') el.setAttribute('role', 'button');
            }
        }

        // --- <textarea> ---
        if (tag === 'TEXTAREA') {
            applyAriaLabel(el);
        }

        // --- <button> ---
        if (tag === 'BUTTON') {
            applyAriaLabel(el);
        }

        // --- Toolbar/menubar containers ---
        if (!el.hasAttribute('role')) {
            const cls = el.className || '';
            if (/\btoolbar\b/.test(cls))  el.setAttribute('role', 'toolbar');
            if (/\bmenubar\b/.test(cls))  el.setAttribute('role', 'menubar');
            if (/\bmenu\b/.test(cls) && !/\bmenubar\b/.test(cls))
                                          el.setAttribute('role', 'menu');
        }

        // --- Dropdown toggles: aria-expanded management ---
        // Detect elements that already have aria-expanded set so we can keep
        // it synchronised when their open/close class changes.
        const isToggle =
            el.hasAttribute('aria-expanded') ||
            /\bdropdown[-_]toggle\b|\bmenu[-_]btn\b/.test(el.className || '');

        if (isToggle && !el.__xcmExpandedWired) {
            el.__xcmExpandedWired = true;
            // Initial state
            if (!el.hasAttribute('aria-expanded')) {
                el.setAttribute('aria-expanded', 'false');
            }
            // Keep aria-expanded in sync with click events
            el.addEventListener('click', function () {
                // Allow a microtask for the page's own handler to toggle classes
                Promise.resolve().then(function () {
                    const open =
                        el.classList.contains('open') ||
                        el.classList.contains('active') ||
                        el.getAttribute('aria-expanded') === 'true';
                    el.setAttribute('aria-expanded', open ? 'true' : 'false');
                });
            });
        }
    }

    function applyAriaToAll(root) {
        const scope = root || document;
        scope.querySelectorAll(
            'select, input, textarea, button, [class*="toolbar"], [class*="menubar"], ' +
            '[class*="dropdown-toggle"], [class*="menu-btn"]'
        ).forEach(applyAriaOnElement);
    }

    /* -----------------------------------------------------------------------
     * 3. MUTATION OBSERVER — watches for dynamically added nodes
     * --------------------------------------------------------------------- */

    function processNode(node) {
        if (node.nodeType !== 1) return; // Element nodes only
        const tag = node.tagName;
        if (tag === 'SELECT') {
            normaliseSelect(node);
            applyAriaOnElement(node);
        } else if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'BUTTON') {
            applyAriaOnElement(node);
        } else {
            // Subtree additions (e.g. a toolbar div injected wholesale)
            if (node.querySelectorAll) {
                node.querySelectorAll('select').forEach(function (s) {
                    normaliseSelect(s);
                    applyAriaOnElement(s);
                });
                node.querySelectorAll('input, textarea, button').forEach(applyAriaOnElement);
                applyAriaOnElement(node); // the container itself (toolbar/menubar etc.)
            }
        }
    }

    var observer = new MutationObserver(function (mutations) {
        mutations.forEach(function (m) {
            m.addedNodes.forEach(processNode);
        });
    });

    /* -----------------------------------------------------------------------
     * 4. BOOTSTRAP — run on DOMContentLoaded (or immediately if already ready)
     * --------------------------------------------------------------------- */

    function boot() {
        normaliseAllSelects();
        applyAriaToAll();
        observer.observe(document.body, { childList: true, subtree: true });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', boot);
    } else {
        boot();
    }

}(typeof globalThis !== 'undefined' ? globalThis : window));
