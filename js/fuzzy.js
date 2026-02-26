/**
 * fuzzy.js — Fuzzy autocomplete dropdown for CSS property names in the CSS editor
 * Attached to window.LiveCSS.fuzzy
 *
 * Call LiveCSS.fuzzy.init(allCssProperties) after editor.init().
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.fuzzy = (function () {

    var allCssProperties = [];
    var dropdown, selectedIndex, matches, isActive, wordStart;

    function init(cssProperties) {
        allCssProperties = cssProperties;
        dropdown      = document.getElementById('fuzzyDropdown');
        selectedIndex = -1;
        matches       = [];
        isActive      = false;
        wordStart     = null;

        var cm = LiveCSS.editor.getCssEditor();

        cm.on('inputRead', function (editor, change) {
            if (change.origin === '+input' || change.origin === '+delete') {
                showDropdown(editor);
            }
        });

        cm.on('cursorActivity', function (editor) {
            if (!isActive) return;
            var info = getWordBeforeCursor(editor);
            if (!info || info.word.length < 1) {
                hideDropdown();
            } else {
                showDropdown(editor);
            }
        });

        cm.on('keydown', function (editor, e) {
            if (!isActive) return;
            if (e.keyCode === 40) {           // Arrow Down
                e.preventDefault();
                selectedIndex = Math.min(selectedIndex + 1, matches.length - 1);
                renderDropdown(editor);
            } else if (e.keyCode === 38) {    // Arrow Up
                e.preventDefault();
                selectedIndex = Math.max(selectedIndex - 1, 0);
                renderDropdown(editor);
            } else if (e.keyCode === 9 || e.keyCode === 13) {  // Tab / Enter
                e.preventDefault();
                if (selectedIndex >= 0) acceptMatch(editor, selectedIndex);
            } else if (e.keyCode === 27) {    // Escape
                hideDropdown();
            }
        });

        cm.on('blur', function () {
            setTimeout(hideDropdown, 200);
        });
    }

    // ── Matching helpers ─────────────────────────────────────────

    /** True if query is a subsequence of (or substring of) target */
    function fuzzyMatch(query, target) {
        query  = query.toLowerCase();
        target = target.toLowerCase();
        if (!query.length) return false;
        if (target.indexOf(query) !== -1) return true;
        var qi = 0;
        for (var ti = 0; ti < target.length && qi < query.length; ti++) {
            if (target[ti] === query[qi]) qi++;
        }
        return qi === query.length;
    }

    /** Lower score = better match */
    function fuzzyScore(query, target) {
        query  = query.toLowerCase();
        target = target.toLowerCase();
        if (target.indexOf(query) === 0)   return 0;
        var idx = target.indexOf(query);
        if (idx !== -1)                    return 1 + idx;
        return 100 + target.length;
    }

    // ── Cursor word detection ────────────────────────────────────

    /**
     * Walk back from the cursor to extract the word being typed.
     * Returns null when the cursor is in a value position (after a colon).
     */
    function getWordBeforeCursor(cm) {
        var cursor = cm.getCursor();
        var line   = cm.getLine(cursor.line);
        var end    = cursor.ch;
        var start  = end;

        while (start > 0 && /[a-zA-Z\-]/.test(line[start - 1])) start--;

        // If the last non-whitespace character before the word is a colon,
        // the user is typing a value — do not autocomplete a property name.
        var before = line.substring(0, start).trim();
        if (before.length && before[before.length - 1] === ':') return null;

        var word = line.substring(start, end);
        if (!word.length) return null;

        return { word: word, start: start, end: end, line: cursor.line };
    }

    // ── Dropdown rendering ───────────────────────────────────────

    function showDropdown(cm) {
        var info = getWordBeforeCursor(cm);
        if (!info) { hideDropdown(); return; }

        wordStart = info;
        var query = info.word;

        matches = allCssProperties
            .filter(function (p) { return fuzzyMatch(query, p); })
            .sort(function (a, b) { return fuzzyScore(query, a) - fuzzyScore(query, b); })
            .slice(0, 12);

        if (!matches.length) { hideDropdown(); return; }
        // Hide when the only match is already fully typed
        if (matches.length === 1 && matches[0] === query) { hideDropdown(); return; }

        selectedIndex = 0;
        renderDropdown(cm);
        isActive = true;
    }

    function renderDropdown(cm) {
        var escapeHtml = LiveCSS.utils.escapeHtml;
        var html = '';
        for (var i = 0; i < matches.length; i++) {
            var cls = 'fuzzy-item' + (i === selectedIndex ? ' fuzzy-item-active' : '');
            html += '<div class="' + cls + '" data-index="' + i + '">' + escapeHtml(matches[i]) + '</div>';
        }
        dropdown.innerHTML = html;

        var coords = cm.cursorCoords(true, 'page');
        dropdown.style.left = coords.left + 'px';
        dropdown.style.top  = (coords.bottom + 2) + 'px';
        dropdown.classList.remove('hidden');

        // Click to accept
        var items = dropdown.querySelectorAll('.fuzzy-item');
        for (var j = 0; j < items.length; j++) {
            items[j].addEventListener('mousedown', function (e) {
                e.preventDefault();
                acceptMatch(cm, parseInt(this.getAttribute('data-index'), 10));
            });
        }
    }

    function hideDropdown() {
        dropdown.classList.add('hidden');
        dropdown.innerHTML = '';
        isActive      = false;
        selectedIndex = -1;
        matches       = [];
        wordStart     = null;
    }

    function acceptMatch(cm, index) {
        if (index < 0 || index >= matches.length || !wordStart) return;
        cm.replaceRange(
            matches[index],
            { line: wordStart.line, ch: wordStart.start },
            { line: wordStart.line, ch: wordStart.end }
        );
        hideDropdown();
        cm.focus();
    }

    return { init: init };

}());
