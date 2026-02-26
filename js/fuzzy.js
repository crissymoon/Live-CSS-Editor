/**
 * fuzzy.js — Fuzzy autocomplete dropdown for CSS, JS, and HTML editors
 * Attached to window.LiveCSS.fuzzy
 *
 * Call LiveCSS.fuzzy.init(allCssProperties) after editor.init().
 * Call LiveCSS.fuzzy.initJs()  to enable autocomplete in the JS editor.
 * Call LiveCSS.fuzzy.initHtml() to enable autocomplete in the HTML editor.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.fuzzy = (function () {

    var dropdown;

    // ── Shared match helpers ─────────────────────────────────────

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

    function fuzzyScore(query, target) {
        query  = query.toLowerCase();
        target = target.toLowerCase();
        if (target.indexOf(query) === 0)  return 0;
        var idx = target.indexOf(query);
        if (idx !== -1)                   return 1 + idx;
        return 100 + target.length;
    }

    // ── Shared driver — attaches fuzzy dropdown to any CodeMirror ─

    /**
     * @param {CodeMirror} cm          CodeMirror instance to attach to
     * @param {string[]}   wordList    Candidate words to match against
     * @param {RegExp}     charRe      Character class for word chars (e.g. /[a-zA-Z\-]/)
     * @param {Function}   [extraGuard] Optional fn(cm) → bool; return false to suppress
     */
    function attachFuzzy(cm, wordList, charRe, extraGuard) {
        var selIdx   = -1;
        var matchArr = [];
        var active   = false;
        var wStart   = null;

        function getWord(editor) {
            var cursor = editor.getCursor();
            var line   = editor.getLine(cursor.line);
            var end    = cursor.ch;
            var start  = end;
            while (start > 0 && charRe.test(line[start - 1])) { start--; }
            var word = line.substring(start, end);
            if (!word.length) { return null; }
            return { word: word, start: start, end: end, line: cursor.line };
        }

        function hide() {
            if (dropdown) {
                dropdown.classList.add('hidden');
                dropdown.innerHTML = '';
            }
            active   = false;
            selIdx   = -1;
            matchArr = [];
            wStart   = null;
        }

        function accept(editor, idx) {
            if (idx < 0 || idx >= matchArr.length || !wStart) { return; }
            editor.replaceRange(
                matchArr[idx],
                { line: wStart.line, ch: wStart.start },
                { line: wStart.line, ch: wStart.end }
            );
            hide();
            editor.focus();
        }

        function render(editor) {
            var escapeHtml = LiveCSS.utils.escapeHtml;
            var html = '';
            for (var i = 0; i < matchArr.length; i++) {
                var cls = 'fuzzy-item' + (i === selIdx ? ' fuzzy-item-active' : '');
                html += '<div class="' + cls + '" data-index="' + i + '">' +
                        escapeHtml(matchArr[i]) + '</div>';
            }
            dropdown.innerHTML = html;

            var coords = editor.cursorCoords(true, 'page');
            dropdown.style.left = coords.left + 'px';
            dropdown.style.top  = (coords.bottom + 2) + 'px';
            dropdown.classList.remove('hidden');

            var items = dropdown.querySelectorAll('.fuzzy-item');
            for (var j = 0; j < items.length; j++) {
                items[j].addEventListener('mousedown', (function (idx2) {
                    return function (e) { e.preventDefault(); accept(editor, idx2); };
                })(j));
            }
        }

        function show(editor) {
            if (extraGuard && !extraGuard(editor)) { hide(); return; }

            var info = getWord(editor);
            if (!info) { hide(); return; }

            wStart   = info;
            matchArr = wordList
                .filter(function (p) { return fuzzyMatch(info.word, p); })
                .sort(function (a, b) {
                    return fuzzyScore(info.word, a) - fuzzyScore(info.word, b);
                })
                .slice(0, 14);

            if (!matchArr.length) { hide(); return; }
            if (matchArr.length === 1 && matchArr[0] === info.word) { hide(); return; }

            selIdx = 0;
            render(editor);
            active = true;
        }

        cm.on('inputRead', function (editor, change) {
            if (change.origin === '+input' || change.origin === '+delete') {
                show(editor);
            }
        });

        cm.on('cursorActivity', function (editor) {
            if (!active) { return; }
            var info = getWord(editor);
            if (!info) { hide(); } else { show(editor); }
        });

        cm.on('keydown', function (editor, e) {
            if (!active) { return; }
            if (e.keyCode === 40) {                          // Arrow Down
                e.preventDefault();
                selIdx = Math.min(selIdx + 1, matchArr.length - 1);
                render(editor);
            } else if (e.keyCode === 38) {                   // Arrow Up
                e.preventDefault();
                selIdx = Math.max(selIdx - 1, 0);
                render(editor);
            } else if (e.keyCode === 9 || e.keyCode === 13) { // Tab / Enter
                e.preventDefault();
                if (selIdx >= 0) { accept(editor, selIdx); }
            } else if (e.keyCode === 27) {                   // Escape
                hide();
            }
        });

        cm.on('blur', function () { setTimeout(hide, 200); });
    }

    // ── CSS — guard: do not trigger when cursor is in a value position ──

    function cssPosGuard(editor) {
        var cursor = editor.getCursor();
        var line   = editor.getLine(cursor.line);
        var end    = cursor.ch;
        var start  = end;
        while (start > 0 && /[a-zA-Z\-]/.test(line[start - 1])) { start--; }
        var before = line.substring(0, start).trim();
        if (before.length && before[before.length - 1] === ':') { return false; }
        return true;
    }

    // ── JS words ──────────────────────────────────────────────────

    var JS_WORDS = [
        'console.log', 'console.error', 'console.warn', 'console.table',
        'document.getElementById', 'document.querySelector', 'document.querySelectorAll',
        'document.createElement', 'document.createTextNode',
        'document.body', 'document.head', 'document.title', 'document.cookie',
        'addEventListener', 'removeEventListener', 'dispatchEvent',
        'setTimeout', 'clearTimeout', 'setInterval', 'clearInterval',
        'requestAnimationFrame', 'cancelAnimationFrame',
        'fetch', 'Promise', 'Promise.all', 'Promise.race',
        'async', 'await', 'return', 'typeof', 'instanceof',
        'Array.from', 'Array.isArray', 'Array.of',
        'Object.keys', 'Object.values', 'Object.entries', 'Object.assign',
        'Object.freeze', 'Object.create', 'Object.defineProperty',
        'JSON.parse', 'JSON.stringify',
        'localStorage.getItem', 'localStorage.setItem', 'localStorage.removeItem',
        'sessionStorage.getItem', 'sessionStorage.setItem',
        'window.location', 'window.location.href', 'window.location.reload',
        'window.history.pushState', 'window.history.back',
        'window.innerWidth', 'window.innerHeight',
        'window.scrollX', 'window.scrollY',
        'classList.add', 'classList.remove', 'classList.toggle', 'classList.contains',
        'setAttribute', 'getAttribute', 'removeAttribute',
        'textContent', 'innerHTML', 'outerHTML',
        'appendChild', 'removeChild', 'insertBefore', 'replaceChild',
        'closest', 'matches', 'getBoundingClientRect',
        'offsetWidth', 'offsetHeight', 'offsetTop', 'offsetLeft',
        'scrollTop', 'scrollLeft',
        'preventDefault', 'stopPropagation',
        'event.target', 'event.currentTarget',
        'event.clientX', 'event.clientY', 'event.key', 'event.keyCode',
        'Math.floor', 'Math.ceil', 'Math.round', 'Math.abs',
        'Math.max', 'Math.min', 'Math.random', 'Math.sqrt', 'Math.pow', 'Math.PI',
        'parseInt', 'parseFloat', 'isNaN', 'isFinite', 'encodeURIComponent',
        'decodeURIComponent', 'encodeURI', 'decodeURI',
        'hasOwnProperty', 'prototype', 'constructor',
        'map', 'filter', 'reduce', 'forEach', 'find', 'findIndex',
        'some', 'every', 'includes', 'push', 'pop', 'shift', 'unshift',
        'splice', 'slice', 'sort', 'reverse', 'concat', 'join',
        'flat', 'flatMap', 'fill', 'copyWithin', 'indexOf', 'lastIndexOf',
        'split', 'trim', 'trimStart', 'trimEnd', 'replace', 'replaceAll',
        'startsWith', 'endsWith', 'substring', 'padStart', 'padEnd',
        'toUpperCase', 'toLowerCase', 'charAt', 'charCodeAt',
        'function', 'const', 'let', 'var', 'class', 'extends', 'new',
        'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'break',
        'continue', 'try', 'catch', 'finally', 'throw', 'import', 'export',
        'default', 'null', 'undefined', 'true', 'false', 'NaN', 'Infinity'
    ];

    // ── HTML attribute / tag words ────────────────────────────────

    var HTML_WORDS = [
        // Common tags
        'div', 'span', 'section', 'article', 'aside', 'header', 'footer',
        'nav', 'main', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
        'p', 'a', 'img', 'button', 'input', 'textarea', 'select', 'option',
        'form', 'label', 'ul', 'ol', 'li', 'table', 'thead', 'tbody',
        'tr', 'td', 'th', 'script', 'style', 'link', 'meta',
        'canvas', 'video', 'audio', 'source', 'iframe', 'figure', 'figcaption',
        'blockquote', 'pre', 'code', 'em', 'strong', 'small', 'sup', 'sub',
        // Attributes
        'id', 'class', 'style', 'title', 'lang', 'dir', 'tabindex', 'hidden',
        'href', 'src', 'alt', 'width', 'height', 'target', 'rel', 'download',
        'type', 'name', 'value', 'placeholder', 'disabled', 'readonly', 'required',
        'checked', 'selected', 'multiple', 'min', 'max', 'step', 'maxlength',
        'pattern', 'autofocus', 'autocomplete', 'form', 'for', 'action', 'method',
        'enctype', 'novalidate', 'accept', 'capture',
        'colspan', 'rowspan', 'scope', 'headers',
        'data-*', 'role', 'aria-label', 'aria-hidden', 'aria-expanded',
        'aria-controls', 'aria-live', 'aria-atomic', 'aria-describedby',
        'aria-labelledby', 'aria-selected', 'aria-disabled', 'aria-checked',
        'draggable', 'contenteditable', 'spellcheck', 'translate',
        'crossorigin', 'loading', 'decoding', 'fetchpriority', 'sizes', 'srcset',
        'media', 'async', 'defer', 'integrity', 'nonce',
        'http-equiv', 'charset', 'content', 'viewport'
    ];

    // ── Public init functions ─────────────────────────────────────

    function init(cssProperties) {
        dropdown = document.getElementById('fuzzyDropdown');
        var cm   = LiveCSS.editor.getCssEditor();
        attachFuzzy(cm, cssProperties, /[a-zA-Z\-]/, cssPosGuard);
    }

    function initJs() {
        if (!dropdown) { dropdown = document.getElementById('fuzzyDropdown'); }
        var cm = LiveCSS.editor.getJsEditor && LiveCSS.editor.getJsEditor();
        if (!cm) { return; }
        // JS: match identifier chars including dot for method names
        attachFuzzy(cm, JS_WORDS, /[a-zA-Z0-9_.$]/);
    }

    function initHtml() {
        if (!dropdown) { dropdown = document.getElementById('fuzzyDropdown'); }
        var cm = LiveCSS.editor.getHtmlEditor && LiveCSS.editor.getHtmlEditor();
        if (!cm) { return; }
        // HTML: match tag/attribute chars including dash
        attachFuzzy(cm, HTML_WORDS, /[a-zA-Z0-9\-]/);
    }

    return { init: init, initJs: initJs, initHtml: initHtml };

}());
