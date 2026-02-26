/**
 * fuzzy.js — Fuzzy autocomplete dropdown for CSS, JS, and HTML editors
 * Attached to window.LiveCSS.fuzzy
 *
 * CSS editor: property names before ":" and value keywords after ":"
 * JS editor:  common APIs / keywords
 * HTML editor: tags / attributes
 *
 * Call LiveCSS.fuzzy.init(allCssProperties, propertyValues) after editor.init().
 * Call LiveCSS.fuzzy.initJs()   for JS editor autocomplete.
 * Call LiveCSS.fuzzy.initHtml() for HTML editor autocomplete.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.fuzzy = (function () {

    var dropdown;

    /* ================================================================
       Match helpers
       ================================================================ */

    function fuzzyMatch(query, target) {
        var q = query.toLowerCase();
        var t = target.toLowerCase();
        if (!q.length) return false;
        if (t.indexOf(q) !== -1) return true;
        var qi = 0;
        for (var ti = 0; ti < t.length && qi < q.length; ti++) {
            if (t[ti] === q[qi]) qi++;
        }
        return qi === q.length;
    }

    function fuzzyScore(query, target) {
        var q = query.toLowerCase();
        var t = target.toLowerCase();
        if (t.indexOf(q) === 0)  return 0;            /* prefix match — best */
        var idx = t.indexOf(q);
        if (idx !== -1)          return 1 + idx;       /* substring match */
        return 100 + t.length;                         /* fuzzy fallback */
    }

    /* ================================================================
       Generic driver — attaches fuzzy dropdown to any CodeMirror
       ================================================================ */

    /**
     * @param {CodeMirror}  cm
     * @param {Function}    getWordList  (cm) => string[] — returns candidates for current context
     * @param {RegExp}      charRe       word-character pattern
     */
    function attachFuzzy(cm, getWordList, charRe) {
        var selIdx   = -1;
        var matchArr = [];
        var active   = false;
        var wStart   = null;

        function getWord(editor) {
            var cursor = editor.getCursor();
            var line   = editor.getLine(cursor.line);
            var end    = cursor.ch;
            var start  = end;
            while (start > 0 && charRe.test(line[start - 1])) start--;
            var word = line.substring(start, end);
            if (!word.length) return null;
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
            if (idx < 0 || idx >= matchArr.length || !wStart) return;
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

            /* ── Position: below cursor, but flip above if near bottom ── */
            var coords = editor.cursorCoords(true, 'page');
            var spaceBelow = window.innerHeight - coords.bottom - 8;
            var spaceAbove = coords.top - 8;
            var dHeight = dropdown.offsetHeight || 120;

            if (spaceBelow >= dHeight || spaceBelow >= spaceAbove) {
                /* show below */
                dropdown.style.top  = (coords.bottom + 2) + 'px';
            } else {
                /* flip above */
                dropdown.style.top  = Math.max(4, (coords.top - dHeight - 2)) + 'px';
            }
            dropdown.style.left = coords.left + 'px';

            /* clamp right edge */
            var dRight = coords.left + (dropdown.offsetWidth || 200);
            if (dRight > window.innerWidth - 8) {
                dropdown.style.left = Math.max(4, window.innerWidth - (dropdown.offsetWidth || 200) - 8) + 'px';
            }

            dropdown.classList.remove('hidden');

            /* click handlers */
            var items = dropdown.querySelectorAll('.fuzzy-item');
            for (var j = 0; j < items.length; j++) {
                items[j].addEventListener('mousedown', (function (idx2) {
                    return function (e) { e.preventDefault(); accept(editor, idx2); };
                })(j));
            }
        }

        function show(editor) {
            var info = getWord(editor);
            if (!info) { hide(); return; }

            var candidates = getWordList(editor);
            if (!candidates || !candidates.length) { hide(); return; }

            wStart   = info;
            matchArr = candidates
                .filter(function (p) { return fuzzyMatch(info.word, p); })
                .sort(function (a, b) {
                    return fuzzyScore(info.word, a) - fuzzyScore(info.word, b);
                })
                .slice(0, 18);

            if (!matchArr.length) { hide(); return; }
            if (matchArr.length === 1 && matchArr[0] === info.word) { hide(); return; }

            selIdx = 0;
            render(editor);
            active = true;
        }

        /* ── Events ── */
        cm.on('inputRead', function (editor, change) {
            if (change.origin === '+input' || change.origin === '+delete') {
                show(editor);
            }
        });

        cm.on('cursorActivity', function (editor) {
            if (!active) return;
            var info = getWord(editor);
            if (!info) hide(); else show(editor);
        });

        cm.on('keydown', function (editor, e) {
            if (!active) return;
            if (e.keyCode === 40) {                           /* Down */
                e.preventDefault();
                selIdx = Math.min(selIdx + 1, matchArr.length - 1);
                render(editor);
            } else if (e.keyCode === 38) {                    /* Up */
                e.preventDefault();
                selIdx = Math.max(selIdx - 1, 0);
                render(editor);
            } else if (e.keyCode === 9 || e.keyCode === 13) { /* Tab / Enter */
                e.preventDefault();
                if (selIdx >= 0) accept(editor, selIdx);
            } else if (e.keyCode === 27) {                    /* Escape */
                hide();
            }
        });

        cm.on('blur', function () { setTimeout(hide, 200); });
    }

    /* ================================================================
       CSS — context-aware: properties OR values
       ================================================================ */

    var cssPropertyList = [];
    var cssValueMap     = {};    /* property → string[] of value keywords */
    var cssAllValues    = [];    /* merged flat list of every value keyword */

    /**
     * Parses the propertyValues file structure:
     *  { "text-align": "left | right | center | justify | start | end", ... }
     * into cssValueMap: { "text-align": ["left","right","center",...], ... }
     * and cssAllValues: deduplicated flat array.
     */
    function buildValueMap(pvObj) {
        var seen = {};
        for (var prop in pvObj) {
            if (!pvObj.hasOwnProperty(prop)) continue;
            var raw = pvObj[prop];
            var vals = raw.split('|').map(function (s) { return s.trim(); })
                          .filter(function (s) { return s.length && s[0] !== '<'; });
            cssValueMap[prop] = vals;
            for (var i = 0; i < vals.length; i++) {
                if (!seen[vals[i]]) {
                    seen[vals[i]] = true;
                    cssAllValues.push(vals[i]);
                }
            }
        }
    }

    /**
     * Determine which property the cursor is inside the value of.
     * Returns the property name or null if cursor is in property position.
     */
    function getPropertyForValue(editor) {
        var cursor = editor.getCursor();
        var line   = editor.getLine(cursor.line);
        var before = line.substring(0, cursor.ch);

        /* Walk backward on the same line for the last colon not inside parens */
        var colonIdx = -1;
        var depth = 0;
        for (var i = before.length - 1; i >= 0; i--) {
            var c = before[i];
            if (c === ')') depth++;
            else if (c === '(') depth--;
            else if (c === ':' && depth <= 0) { colonIdx = i; break; }
            else if (c === ';' || c === '{' || c === '}') break;
        }
        if (colonIdx === -1) return null;

        /* Extract the property name before the colon */
        var propSlice = before.substring(0, colonIdx).replace(/^\s+/, '');
        /* get last word (e.g. "  text-align" → "text-align") */
        var propMatch = propSlice.match(/([a-zA-Z\-]+)\s*$/);
        return propMatch ? propMatch[1].toLowerCase() : null;
    }

    function cssGetWordList(editor) {
        var prop = getPropertyForValue(editor);
        if (prop) {
            /* cursor is after ":" — return value candidates */
            var specific = cssValueMap[prop];
            if (specific && specific.length) return specific;
            return cssAllValues;
        }
        /* cursor is before ":" — return property names */
        return cssPropertyList;
    }

    /* ================================================================
       JS words
       ================================================================ */
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

    /* ================================================================
       HTML words
       ================================================================ */
    var HTML_WORDS = [
        'div', 'span', 'section', 'article', 'aside', 'header', 'footer',
        'nav', 'main', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
        'p', 'a', 'img', 'button', 'input', 'textarea', 'select', 'option',
        'form', 'label', 'ul', 'ol', 'li', 'table', 'thead', 'tbody',
        'tr', 'td', 'th', 'script', 'style', 'link', 'meta',
        'canvas', 'video', 'audio', 'source', 'iframe', 'figure', 'figcaption',
        'blockquote', 'pre', 'code', 'em', 'strong', 'small', 'sup', 'sub',
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

    /* ================================================================
       Public init functions
       ================================================================ */

    function init(cssProperties, propertyValuesObj) {
        dropdown = document.getElementById('fuzzyDropdown');
        cssPropertyList = cssProperties || [];
        if (propertyValuesObj) buildValueMap(propertyValuesObj);

        var cm = LiveCSS.editor.getCssEditor();
        /* CSS: allow hyphens in word chars, and value chars too */
        attachFuzzy(cm, cssGetWordList, /[a-zA-Z\-]/);
    }

    function initJs() {
        if (!dropdown) dropdown = document.getElementById('fuzzyDropdown');
        var cm = LiveCSS.editor.getJsEditor && LiveCSS.editor.getJsEditor();
        if (!cm) return;
        attachFuzzy(cm, function () { return JS_WORDS; }, /[a-zA-Z0-9_.$]/);
    }

    function initHtml() {
        if (!dropdown) dropdown = document.getElementById('fuzzyDropdown');
        var cm = LiveCSS.editor.getHtmlEditor && LiveCSS.editor.getHtmlEditor();
        if (!cm) return;
        attachFuzzy(cm, function () { return HTML_WORDS; }, /[a-zA-Z0-9\-]/);
    }

    return { init: init, initJs: initJs, initHtml: initHtml };

}());
