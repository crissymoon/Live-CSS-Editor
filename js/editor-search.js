/**
 * editor-search.js
 * Inline per-panel search bar for CSS / HTML / JS editors.
 * Attached to window.LiveCSS.editorSearch
 *
 * Each editor panel has its own .panel-search strip that slides open
 * below the panel header. Opened via Cmd+F / Ctrl+F (extraKeys in editor.js)
 * or by clicking inside the search input directly.
 *
 * Public API:  init()   - call once after editors exist
 *              open(cm) - show and focus the search bar for a specific editor
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editorSearch = (function () {
    'use strict';

    // Map: CodeMirror instance -> SearchInstance
    var instances = [];

    /* ---- Overlay highlight ----------------------------------------- */

    function buildOverlay(term) {
        var lc  = term.toLowerCase();
        var len = term.length;
        return {
            token: function (stream) {
                var rest = stream.string.slice(stream.pos).toLowerCase();
                var idx  = rest.indexOf(lc);
                if (idx === 0) { stream.pos += len; return 'search-match'; }
                if (idx > 0)  { stream.pos += idx; }
                else          { stream.skipToEnd(); }
                return null;
            }
        };
    }

    /* ---- Single panel search instance ------------------------------ */

    function SearchInstance(cm, barEl) {
        var self      = this;
        self.cm       = cm;
        self.bar      = barEl;
        self.input    = barEl.querySelector('.ps-input');
        self.countEl  = barEl.querySelector('.ps-count');
        self.prevBtn  = barEl.querySelector('.ps-prev');
        self.nextBtn  = barEl.querySelector('.ps-next');
        self.closeBtn = barEl.querySelector('.ps-close');
        self.matches  = [];
        self.idx      = -1;
        self.overlay  = null;

        function clearOverlay() {
            if (self.overlay) {
                try { cm.removeOverlay(self.overlay); } catch (e) {}
                self.overlay = null;
            }
        }

        function applyOverlay(term) {
            clearOverlay();
            if (!term) { return; }
            self.overlay = buildOverlay(term);
            cm.addOverlay(self.overlay);
        }

        function findAll(term) {
            self.matches = [];
            self.idx     = -1;
            if (!term) { return; }
            var lc        = term.toLowerCase();
            var lineCount = cm.lineCount();
            for (var i = 0; i < lineCount; i++) {
                var line = cm.getLine(i);
                if (!line) { continue; }
                var ll  = line.toLowerCase();
                var pos = 0;
                var hit;
                while ((hit = ll.indexOf(lc, pos)) !== -1) {
                    self.matches.push({
                        from: { line: i, ch: hit },
                        to:   { line: i, ch: hit + term.length }
                    });
                    pos = hit + term.length;
                }
            }
        }

        function updateCount() {
            if (!self.input.value) { self.countEl.textContent = ''; return; }
            self.countEl.textContent = self.matches.length === 0
                ? '0 / 0'
                : (self.idx + 1) + ' / ' + self.matches.length;
        }

        function jumpTo(i) {
            if (self.matches.length === 0) { return; }
            self.idx = ((i % self.matches.length) + self.matches.length) % self.matches.length;
            var m = self.matches[self.idx];
            cm.setSelection(m.from, m.to);
            cm.scrollIntoView({ from: m.from, to: m.to }, 60);
            updateCount();
        }

        function runSearch(term) {
            applyOverlay(term);
            findAll(term);
            if (self.matches.length === 0) { updateCount(); return; }
            // Jump to match closest to current cursor
            var cur  = cm.getCursor();
            var best = 0;
            for (var i = 0; i < self.matches.length; i++) {
                var m = self.matches[i];
                if (m.from.line > cur.line ||
                   (m.from.line === cur.line && m.from.ch >= cur.ch)) {
                    best = i; break;
                }
                best = (i + 1) % self.matches.length;
            }
            jumpTo(best);
        }

        // Public: open this instance's bar
        self.open = function () {
            self.bar.classList.add('ps-visible');
            self.input.focus();
            self.input.select();
            if (self.input.value) { runSearch(self.input.value); }
        };

        // Public: close
        self.close = function () {
            self.bar.classList.remove('ps-visible');
            clearOverlay();
            self.matches = [];
            self.idx     = -1;
            self.countEl.textContent = '';
            cm.focus();
        };

        // Wire events
        self.input.addEventListener('input', function () {
            runSearch(self.input.value);
        });

        self.input.addEventListener('keydown', function (e) {
            if (e.key === 'Enter') {
                e.preventDefault();
                jumpTo(e.shiftKey ? self.idx - 1 : self.idx + 1);
            } else if (e.key === 'Escape') {
                self.close();
            }
        });

        self.prevBtn.addEventListener('mousedown',  function (e) { e.preventDefault(); jumpTo(self.idx - 1); });
        self.nextBtn.addEventListener('mousedown',  function (e) { e.preventDefault(); jumpTo(self.idx + 1); });
        self.closeBtn.addEventListener('mousedown', function (e) { e.preventDefault(); self.close(); });
    }

    /* ---- Public open(cm) ------------------------------------------- */

    function open(cm) {
        for (var i = 0; i < instances.length; i++) {
            if (instances[i].cm === cm) {
                instances[i].open();
                return;
            }
        }
    }

    /* ---- Init -------------------------------------------------------- */

    function init() {
        var map = [
            { getter: 'getCssEditor',  barId: 'cssSearch'  },
            { getter: 'getHtmlEditor', barId: 'htmlSearch' },
            { getter: 'getJsEditor',   barId: 'jsSearch'   }
        ];

        map.forEach(function (entry) {
            var cm  = LiveCSS.editor[entry.getter] && LiveCSS.editor[entry.getter]();
            var bar = document.getElementById(entry.barId);
            if (!cm || !bar) { return; }
            instances.push(new SearchInstance(cm, bar));
        });
    }

    return { init: init, open: open };

}());
