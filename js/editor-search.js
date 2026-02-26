/**
 * editor-search.js — Floating draggable Cmd+F / Ctrl+F search bar
 * Attached to window.LiveCSS.editorSearch
 *
 * Searches whichever of the three editors last had focus.
 * Call LiveCSS.editorSearch.init() after editors are created.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editorSearch = (function () {
    'use strict';

    var bar        = null;
    var input      = null;
    var countEl    = null;
    var activeEd   = null;   // currently focused CodeMirror instance
    var matches    = [];
    var matchIdx   = -1;
    var overlayActive = false;

    // ── Overlay to highlight all matches ─────────────────────────
    function buildOverlay(term) {
        var re = new RegExp(term.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi');
        return {
            token: function (stream) {
                re.lastIndex = stream.pos;
                var m = re.exec(stream.string);
                if (m && m.index === stream.pos) {
                    stream.pos += m[0].length;
                    return 'search-match';
                }
                stream.next();
                return null;
            }
        };
    }

    var _overlay = null;

    function clearOverlay() {
        if (_overlay && activeEd) {
            try { activeEd.removeOverlay(_overlay); } catch (e) {}
        }
        _overlay = null;
        overlayActive = false;
    }

    function applyOverlay(term) {
        clearOverlay();
        if (!activeEd || !term) { return; }
        _overlay = buildOverlay(term);
        activeEd.addOverlay(_overlay);
        overlayActive = true;
    }

    // ── Find all match positions ──────────────────────────────────
    function findAll(term) {
        matches  = [];
        matchIdx = -1;
        if (!activeEd || !term) { return; }
        var cursor = activeEd.getSearchCursor(term, { line: 0, ch: 0 }, { caseFold: true });
        while (cursor.findNext()) {
            matches.push({ from: cursor.from(), to: cursor.to() });
        }
    }

    function updateCount() {
        if (!countEl) { return; }
        if (matches.length === 0) {
            countEl.textContent = input.value ? '0 / 0' : '';
        } else {
            countEl.textContent = (matchIdx + 1) + ' / ' + matches.length;
        }
    }

    function jumpTo(idx) {
        if (!activeEd || matches.length === 0) { return; }
        matchIdx = ((idx % matches.length) + matches.length) % matches.length;
        var m = matches[matchIdx];
        activeEd.setSelection(m.from, m.to);
        activeEd.scrollIntoView({ from: m.from, to: m.to }, 80);
        updateCount();
    }

    function search(term) {
        applyOverlay(term);
        findAll(term);
        if (matches.length > 0) {
            // Start from cursor position
            var cur = activeEd ? activeEd.getCursor() : { line: 0, ch: 0 };
            var start = 0;
            for (var i = 0; i < matches.length; i++) {
                var m = matches[i];
                if (m.from.line > cur.line ||
                    (m.from.line === cur.line && m.from.ch >= cur.ch)) {
                    start = i;
                    break;
                }
                start = (i + 1) % matches.length;
            }
            jumpTo(start);
        } else {
            updateCount();
        }
    }

    // ── Open / close ──────────────────────────────────────────────
    function open() {
        if (!bar) { return; }
        bar.classList.remove('search-bar-hidden');
        input.focus();
        input.select();
        if (input.value) { search(input.value); }
    }

    function close() {
        if (!bar) { return; }
        bar.classList.add('search-bar-hidden');
        clearOverlay();
        matches  = [];
        matchIdx = -1;
        updateCount();
        if (activeEd) { activeEd.focus(); }
    }

    // ── Drag ──────────────────────────────────────────────────────
    function initDrag(handle) {
        handle.addEventListener('mousedown', function (e) {
            if (e.button !== 0) { return; }
            e.preventDefault();
            var startX = e.clientX - bar.offsetLeft;
            var startY = e.clientY - bar.offsetTop;
            function onMove(ev) {
                bar.style.left = Math.max(0, ev.clientX - startX) + 'px';
                bar.style.top  = Math.max(0, ev.clientY - startY) + 'px';
            }
            function onUp() {
                document.removeEventListener('mousemove', onMove);
                document.removeEventListener('mouseup', onUp);
            }
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
        });
    }

    // ── Init ──────────────────────────────────────────────────────
    function init() {
        bar = document.getElementById('editorSearchBar');
        if (!bar) { return; }

        input   = bar.querySelector('.esb-input');
        countEl = bar.querySelector('.esb-count');
        var prevBtn  = bar.querySelector('.esb-prev');
        var nextBtn  = bar.querySelector('.esb-next');
        var closeBtn = bar.querySelector('.esb-close');
        var handle   = bar.querySelector('.esb-handle');

        initDrag(handle);

        // Track which editor has focus
        var editors = {
            css:  LiveCSS.editor.getCssEditor(),
            html: LiveCSS.editor.getHtmlEditor(),
            js:   LiveCSS.editor.getJsEditor()
        };

        Object.keys(editors).forEach(function (key) {
            var cm = editors[key];
            if (!cm) { return; }
            cm.on('focus', function () {
                if (activeEd !== cm) {
                    clearOverlay();
                    activeEd = cm;
                    if (!bar.classList.contains('search-bar-hidden') && input.value) {
                        search(input.value);
                    }
                }
            });
            // Capture Cmd+F / Ctrl+F inside each editor
            cm.on('keydown', function (instance, e) {
                if ((e.metaKey || e.ctrlKey) && e.key === 'f') {
                    e.preventDefault();
                    open();
                }
            });
        });

        // Default active editor
        activeEd = editors.css || editors.html || editors.js;

        // Search input events
        input.addEventListener('input', function () {
            search(input.value);
        });

        input.addEventListener('keydown', function (e) {
            if (e.key === 'Enter') {
                e.preventDefault();
                if (e.shiftKey) {
                    jumpTo(matchIdx - 1);
                } else {
                    jumpTo(matchIdx + 1);
                }
            } else if (e.key === 'Escape') {
                close();
            }
        });

        prevBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            jumpTo(matchIdx - 1);
        });

        nextBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            jumpTo(matchIdx + 1);
        });

        closeBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            close();
        });

        // Global Cmd+F / Ctrl+F when not inside an editor
        document.addEventListener('keydown', function (e) {
            if ((e.metaKey || e.ctrlKey) && e.key === 'f') {
                var tag = document.activeElement && document.activeElement.tagName;
                // Let editor handler fire first; only intercept for non-CM focus
                if (tag === 'INPUT' || tag === 'TEXTAREA') { return; }
                if (document.activeElement &&
                    document.activeElement.closest('.CodeMirror')) { return; }
                e.preventDefault();
                open();
            }
            if (e.key === 'Escape') {
                if (!bar.classList.contains('search-bar-hidden')) { close(); }
            }
        });
    }

    return { init: init };

}());
