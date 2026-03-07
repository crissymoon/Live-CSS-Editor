/**
 * xcm-focus-tracker.js
 *
 * Injected at document-start into every page tab.
 *
 * Tracks the last focused editable element (input, textarea, contenteditable)
 * and exposes it as window.__xcmLastFocused.  The paste handler in
 * webview.mm reads this instead of document.activeElement because clicking
 * the toolbar paste button causes the page to lose focus before the paste
 * JS runs, leaving activeElement as document.body.
 */
(function () {
    'use strict';

    window.__xcmLastFocused = null;

    function _track(e) {
        var el = e.target;
        if (!el) return;
        var tag = (el.tagName || '').toLowerCase();
        if (tag === 'input' || tag === 'textarea' || el.isContentEditable) {
            window.__xcmLastFocused = el;
        }
    }

    // useCapture=true so we see focus events before the page does.
    document.addEventListener('focusin',  _track, true);

    // On blur we keep the reference -- do not clear it.  The paste handler
    // needs it even after the element loses focus due to a toolbar click.
    // We do clear it on navigation within SPAs so stale references to
    // detached DOM nodes do not persist.
    (function () {
        var _op = history.pushState, _or = history.replaceState;
        function _onNav() { window.__xcmLastFocused = null; }
        if (typeof _op === 'function')
            history.pushState    = function () { _op.apply(this, arguments);    _onNav(); };
        if (typeof _or === 'function')
            history.replaceState = function () { _or.apply(this, arguments); _onNav(); };
    })();
})();
