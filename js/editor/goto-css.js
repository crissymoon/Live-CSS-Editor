/**
 * editor/goto-css.js -- CSS selector navigation
 * Attached to window.LiveCSS.editorGotoCSS
 *
 * Public API
 *   LiveCSS.editorGotoCSS.jumpToCssRule(cssEditor, selector)
 *     Scrolls the CSS CodeMirror instance to the first rule that
 *     references part of `selector`, and restores the CSS panel
 *     from minimized state if needed.
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editorGotoCSS = (function () {

    /**
     * Characters that may appear immediately after a valid selector token
     * (class, id, or tag) inside a CSS rule. Used for word-boundary checks
     * so that e.g. ".btn" does not falsely match ".btn-primary".
     */
    var SELECTOR_BOUNDARY = /[\s{,:.#\[>+~)\]]/;

    /**
     * Search a single line for `needle` with word-boundary awareness.
     * Only matches inside the selector portion (before the first '{').
     * Returns the column index or -1.
     */
    function findSelectorInLine(lineLower, needle) {
        try {
            var bracePos = lineLower.indexOf('{');
            var searchArea = bracePos !== -1 ? lineLower.substring(0, bracePos) : lineLower;
            var start = 0;
            while (true) {
                var col = searchArea.indexOf(needle, start);
                if (col === -1) return -1;
                var afterIdx = col + needle.length;
                if (afterIdx >= searchArea.length || SELECTOR_BOUNDARY.test(searchArea[afterIdx])) {
                    return col;
                }
                start = col + 1;
            }
        } catch (e) {
            console.error('[GoToCSS] findSelectorInLine error:', e);
            return -1;
        }
    }

    /**
     * Jump the CSS editor to the first rule that references part of `selector`.
     * Search order: full selector > ID > compound classes > individual classes > tag.
     * Restores the CSS panel from minimized state if needed.
     */
    function jumpToCssRule(cssEditor, selector) {
        if (!cssEditor) {
            console.warn('[GoToCSS] cssEditor not available');
            return;
        }
        if (!selector || !selector.trim()) {
            console.warn('[GoToCSS] empty selector -- nothing to search for');
            return;
        }

        var terms = [];

        // 1) Full selector (e.g. "div#main.card.active")
        terms.push(selector);

        // 2) ID selector (most specific single token)
        var idMatch = selector.match(/#[^.#\s[\]()]+/);
        if (idMatch) { terms.push(idMatch[0]); }

        // 3) All classes combined (e.g. ".card.active")
        var classes = selector.match(/\.[^.#\s[\]()]+/g) || [];
        if (classes.length > 1) {
            terms.push(classes.join(''));
        }

        // 4) Individual classes, longest first (longer names are more specific)
        var sorted = classes.slice().sort(function (a, b) { return b.length - a.length; });
        for (var i = 0; i < sorted.length; i++) { terms.push(sorted[i]); }

        // 5) Tag name (least specific)
        var tagMatch = selector.match(/^[a-zA-Z][a-zA-Z0-9-]*/);
        if (tagMatch && tagMatch[0] !== 'html' && tagMatch[0] !== 'body') {
            terms.push(tagMatch[0]);
        }

        // De-duplicate while preserving order
        var seen = {};
        var unique = [];
        for (var u = 0; u < terms.length; u++) {
            var key = terms[u].toLowerCase();
            if (!seen[key]) { seen[key] = true; unique.push(terms[u]); }
        }
        terms = unique;

        if (terms.length === 0) {
            console.warn('[GoToCSS] no searchable terms extracted from selector:', selector);
            return;
        }
        console.log('[GoToCSS] searching for selector:', selector, '-- terms:', terms);

        var cssText = cssEditor.getValue();
        var lines   = cssText.split('\n');
        var foundLn = -1, foundCol = -1, foundLen = 0;

        outer: for (var j = 0; j < terms.length; j++) {
            var needle = terms[j].toLowerCase();
            for (var ln = 0; ln < lines.length; ln++) {
                var lower = lines[ln].toLowerCase();
                var col = findSelectorInLine(lower, needle);
                if (col === -1) { continue; }
                foundLn  = ln;
                foundCol = col;
                foundLen = terms[j].length;
                console.log('[GoToCSS] matched "' + terms[j] + '" at line ' + (ln + 1) + ' col ' + col);
                break outer;
            }
        }

        if (foundLn === -1) {
            console.warn('[GoToCSS] selector not found in CSS editor content. Terms tried:', terms);
            return;
        }

        var from = CodeMirror.Pos(foundLn, foundCol);
        var to   = CodeMirror.Pos(foundLn, foundCol + foundLen);

        function doScroll() {
            try {
                cssEditor.setSelection(from, to);
                cssEditor.scrollIntoView({ from: from, to: to }, 80);
                cssEditor.focus();

                // Bright flash mark so the target is unmissable.
                // The CSS animation fades it out over ~1.8s.
                var jumpMark = null;
                try {
                    jumpMark = cssEditor.markText(from, to, {
                        className:    'cm-jump-target',
                        clearOnEnter: false
                    });
                } catch (e) {
                    console.warn('[GoToCSS] markText for jump-target failed:', e);
                }
                if (jumpMark) {
                    setTimeout(function () {
                        try { jumpMark.clear(); }
                        catch (e) { console.warn('[GoToCSS] jumpMark.clear failed:', e); }
                    }, 1900);
                }

                console.log('[GoToCSS] scrolled to line ' + (foundLn + 1));
            } catch (e) {
                console.error('[GoToCSS] setSelection/scrollIntoView failed:', e);
            }
        }

        // Ensure the CSS panel is visible. If it has been minimized the panel's
        // display is set to 'none' by gutter.js. We need to show it and remove
        // its taskbar chip before CodeMirror can render the selection.
        var cssPanel = document.getElementById('cssPanel');
        var isHidden = cssPanel && cssPanel.style.display === 'none';

        if (isHidden) {
            console.log('[GoToCSS] CSS panel is minimized -- restoring it');
            if (cssPanel.dataset.savedLeft)   { cssPanel.style.left   = cssPanel.dataset.savedLeft; }
            if (cssPanel.dataset.savedTop)    { cssPanel.style.top    = cssPanel.dataset.savedTop; }
            if (cssPanel.dataset.savedWidth)  { cssPanel.style.width  = cssPanel.dataset.savedWidth; }
            if (cssPanel.dataset.savedHeight) { cssPanel.style.height = cssPanel.dataset.savedHeight; }
            cssPanel.style.display = '';

            try {
                var taskbar = document.getElementById('panel-taskbar');
                if (taskbar) {
                    var chip = taskbar.querySelector('[data-panel-id="cssPanel"]');
                    if (chip && chip.parentNode) { chip.parentNode.removeChild(chip); }
                }
            } catch (e) {
                console.warn('[GoToCSS] could not remove taskbar chip:', e);
            }

            setTimeout(function () {
                try { cssEditor.refresh(); } catch (e) { console.warn('[GoToCSS] refresh failed:', e); }
                setTimeout(doScroll, 60);
            }, 40);
        } else {
            doScroll();
        }
    }

    return {
        jumpToCssRule: jumpToCssRule
    };

}());
