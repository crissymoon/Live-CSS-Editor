/**
 * xcm-clip-watcher.js
 *
 * Injected at document-start into every page tab.
 * Replaces xcm-focus-tracker.js.
 *
 * Responsibilities:
 *   1. Track the last focused editable element by tagging it with the
 *      attribute data-xcm-target="1".  Only one element carries this
 *      attribute at a time.  The attribute survives focus loss so the
 *      paste handler can find the correct target even after the toolbar
 *      button click blurred the element.
 *
 *   2. Expose window.__xcmPaste(text) -- called by the native paste
 *      handler instead of building inline JS in Objective-C.
 *      It finds the tagged element by attribute query, not by
 *      document.activeElement (which is stale after a toolbar click),
 *      re-focuses it, splices the text at the cursor, and fires the
 *      framework events that React/Vue/Angular need.
 *
 *   3. Expose window.__xcmClearTarget() -- called on SPA navigation
 *      to clear the tag from detached nodes.
 */
(function () {
    'use strict';

    var ATTR = 'data-xcm-target';

    // ── Focus tracking ───────────────────────────────────────────────────────
    function _tag(el) {
        // Remove the attribute from the previous target.
        var prev = document.querySelector('[' + ATTR + ']');
        if (prev && prev !== el) prev.removeAttribute(ATTR);
        if (el) el.setAttribute(ATTR, '1');
    }

    function _onFocusIn(e) {
        var el = e.target;
        if (!el) return;
        var tag = (el.tagName || '').toLowerCase();
        if (tag === 'input' || tag === 'textarea' || el.isContentEditable) {
            _tag(el);
        }
    }

    document.addEventListener('focusin', _onFocusIn, true);

    // ── window.__xcmClearTarget ───────────────────────────────────────────────
    window.__xcmClearTarget = function () {
        var prev = document.querySelector('[' + ATTR + ']');
        if (prev) prev.removeAttribute(ATTR);
    };

    // ── window.__xcmPaste(text) ───────────────────────────────────────────────
    // Called directly by the native paste JS:
    //   [wv evaluateJavaScript:@"window.__xcmPaste(<jsonEncodedText>)"]
    //
    // Search order:
    //   1. Element tagged with data-xcm-target (survives blur)
    //   2. document.activeElement if it is an editable type (fallback for
    //      pages that prevent focusin bubbling via stopPropagation)
    window.__xcmPaste = function (text) {
        if (typeof text !== 'string') return;

        var el = document.querySelector('[' + ATTR + ']');

        // Fallback: activeElement may still be correct on pages that block
        // focusin bubbling.
        if (!el || !document.body || !document.body.contains(el)) {
            var ae  = document.activeElement;
            var aet = ae ? (ae.tagName || '').toLowerCase() : '';
            if (aet === 'input' || aet === 'textarea' || (ae && ae.isContentEditable)) {
                el = ae;
            }
        }

        if (!el || !document.body || !document.body.contains(el)) {
            // No known target -- do nothing rather than paste into the wrong place.
            return;
        }

        var tag = (el.tagName || '').toLowerCase();

        if (tag === 'input' || tag === 'textarea') {
            el.focus();
            var s = el.selectionStart != null ? el.selectionStart : el.value.length;
            var e = el.selectionEnd   != null ? el.selectionEnd   : el.value.length;
            var nv = el.value.slice(0, s) + text + el.value.slice(e);
            // Use the native input value setter so frameworks that override the
            // descriptor (React synthetic events) see the mutation.
            var nativeSetter = Object.getOwnPropertyDescriptor(
                tag === 'input' ? window.HTMLInputElement.prototype
                                : window.HTMLTextAreaElement.prototype,
                'value'
            );
            if (nativeSetter && nativeSetter.set) {
                nativeSetter.set.call(el, nv);
            } else {
                el.value = nv;
            }
            el.selectionStart = el.selectionEnd = s + text.length;
            el.dispatchEvent(new Event('input',  { bubbles: true }));
            el.dispatchEvent(new Event('change', { bubbles: true }));
        } else if (el.isContentEditable) {
            el.focus();
            document.execCommand('insertText', false, text);
        }
    };

    // ── SPA navigation: clear stale tag ──────────────────────────────────────
    (function () {
        var _op = history.pushState, _or = history.replaceState;
        function _onNav() { window.__xcmClearTarget(); }
        if (typeof _op === 'function')
            history.pushState    = function () { _op.apply(this, arguments);    _onNav(); };
        if (typeof _or === 'function')
            history.replaceState = function () { _or.apply(this, arguments); _onNav(); };
    })();

})();
