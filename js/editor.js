/**
 * editor.js — CodeMirror editor instances and live preview renderer
 * Attached to window.LiveCSS.editor
 *
 * Call LiveCSS.editor.init(defaultHtml, defaultCss, defaultJs) once the DOM is ready.
 * Provides four editors: JS, HTML, CSS, and a live preview iframe.
 * All editors have code folding (Ctrl/Cmd+Q to fold at cursor) and fold gutters.
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editor = (function () {

    var htmlEditor, cssEditor, jsEditor;

    /**
     * Build a right-click context-menu inspector script.
     * Always injected into the preview iframe.
     * Right-clicking any element shows selector, dimensions, and key styles.
     */
    function buildContextMenuScript() {
        function ctxMenuInit() {
            var menu = document.createElement('div');
            menu.setAttribute('data-ctx-menu', '1');
            menu.style.position = 'fixed';
            menu.style.zIndex = '999999';
            menu.style.background = '#0c071c';
            menu.style.color = '#eceaf6';
            menu.style.font = '12px/1.6 Consolas,Monaco,monospace';
            menu.style.padding = '6px 0';
            menu.style.border = '1px solid #4d31bf';
            menu.style.display = 'none';
            menu.style.minWidth = '220px';
            menu.style.maxWidth = '360px';
            menu.style.boxShadow = '0 4px 16px rgba(0,0,0,.5)';
            menu.style.overflow = 'hidden';
            document.body.appendChild(menu);

            var lastEl = null;

            function hideMenu() {
                menu.style.display = 'none';
                if (lastEl) { lastEl.style.outline = ''; lastEl = null; }
            }

            function esc(str) {
                var d = document.createElement('span');
                d.textContent = str;
                return d.innerHTML;
            }

            document.addEventListener('contextmenu', function (e) {
                var el = e.target;
                if (!el || !el.tagName) return;
                if (el.getAttribute('data-ctx-menu') === '1') return;
                var p = el;
                while (p) { if (p === menu) return; p = p.parentNode; }
                e.preventDefault();

                if (lastEl) lastEl.style.outline = '';
                lastEl = el;
                el.style.outline = '2px solid #4d31bf';

                var tag = el.tagName.toLowerCase();
                var idStr = el.id ? '#' + el.id : '';
                var clsStr = '';
                if (el.className && typeof el.className === 'string' && el.className.trim()) {
                    clsStr = '.' + el.className.trim().replace(/\s+/g, '.');
                }
                var selector = tag + idStr + clsStr;

                var rect = el.getBoundingClientRect();
                var dims = Math.round(rect.width) + ' \u00D7 ' + Math.round(rect.height);

                var cs = window.getComputedStyle(el);
                var props = [
                    ['display', cs.display],
                    ['position', cs.position],
                    ['color', cs.color],
                    ['background', cs.backgroundColor],
                    ['font-size', cs.fontSize],
                    ['margin', cs.margin],
                    ['padding', cs.padding]
                ];

                var h = '';
                h += '<div style="padding:5px 10px;color:#b39ddb;font-weight:bold;border-bottom:1px solid #2a1f4e;margin-bottom:2px;word-break:break-all;">' + esc(selector) + '</div>';
                h += '<div style="padding:2px 10px;color:#888;">' + esc(dims) + '</div>';
                for (var i = 0; i < props.length; i++) {
                    h += '<div style="padding:2px 10px;"><span style="color:#7c6fb0;">' + esc(props[i][0]) + '</span><span style="color:#555;"> : </span><span style="color:#eceaf6;">' + esc(props[i][1]) + '</span></div>';
                }
                h += '<div data-copy="1" style="padding:5px 10px;margin-top:2px;border-top:1px solid #2a1f4e;cursor:pointer;color:#4d31bf;">Copy selector</div>';
                menu.innerHTML = h;

                menu.style.display = 'block';
                var x = e.clientX, y = e.clientY;
                var mw = menu.offsetWidth, mh = menu.offsetHeight;
                if (x + mw > window.innerWidth)  x = window.innerWidth - mw - 4;
                if (y + mh > window.innerHeight) y = window.innerHeight - mh - 4;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                menu.style.left = x + 'px';
                menu.style.top  = y + 'px';
            }, true);

            menu.addEventListener('mouseover', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && t.getAttribute('data-copy') === '1') { t.style.background = '#1a1130'; return; }
                    t = t.parentNode;
                }
            });
            menu.addEventListener('mouseout', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && t.getAttribute('data-copy') === '1') { t.style.background = 'transparent'; return; }
                    t = t.parentNode;
                }
            });

            menu.addEventListener('click', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && t.getAttribute('data-copy') === '1') {
                        var selDiv = menu.firstChild;
                        if (selDiv) {
                            var txt = selDiv.textContent;
                            var ta = document.createElement('textarea');
                            ta.value = txt;
                            ta.style.position = 'fixed';
                            ta.style.left = '-9999px';
                            document.body.appendChild(ta);
                            ta.select();
                            document.execCommand('copy');
                            document.body.removeChild(ta);
                            t.textContent = 'Copied!';
                            setTimeout(function () { t.textContent = 'Copy selector'; }, 1200);
                        }
                        return;
                    }
                    t = t.parentNode;
                }
            });

            document.addEventListener('click', function (e) {
                if (menu.style.display !== 'none') {
                    var p = e.target;
                    while (p) { if (p === menu) return; p = p.parentNode; }
                    hideMenu();
                }
            }, true);

            document.addEventListener('scroll', hideMenu, true);
        }

        return '<script>(' + ctxMenuInit.toString() + ')();<\/script>';
    }

    var FOLD_KEYS = {
        'Ctrl-Q': function (cm) { cm.foldCode(cm.getCursor()); },
        'Cmd-Q':  function (cm) { cm.foldCode(cm.getCursor()); },
        'Ctrl-F': function (cm) {
            if (window.LiveCSS && LiveCSS.editorSearch) { LiveCSS.editorSearch.open(cm); }
            return false;
        },
        'Cmd-F': function (cm) {
            if (window.LiveCSS && LiveCSS.editorSearch) { LiveCSS.editorSearch.open(cm); }
            return false;
        }
    };

    var FOLD_GUTTERS = ['CodeMirror-linenumbers', 'CodeMirror-foldgutter', 'CodeMirror-lint-markers'];

    /** JSHint options -- relaxed for quick prototyping */
    var JSHINT_OPTS = {
        esversion: 11,
        asi:       true,
        boss:      true,
        evil:      true,
        laxbreak:  true,
        laxcomma:  true,
        loopfunc:  true,
        sub:       true,
        supernew:  true,
        undef:     false,
        unused:    false,
        browser:   true,
        devel:     true
    };

    /** CSSLint rules to disable (too noisy or incompatible with modern CSS) */
    var CSSLINT_OPTS = {
        'known-properties':        false,
        'vendor-prefix':           false,
        'compatible-vendor-prefixes-and-properties': false,
        'star-property-hack':      false,
        'underscore-property-hack': false,
        'important':               false,
        'box-sizing':              false,
        'adjoining-classes':       false,
        'qualified-headings':      false,
        'unique-headings':         false,
        'universal-selector':      false,
        'unqualified-attributes':  false,
        'overqualified-elements':  false,
        'floats':                  false,
        'font-sizes':              false,
        'ids':                     false,
        'regex-selectors':         false,
        'outline-none':            false,
        'shorthand':               false,
        'display-property-grouping': false,
        'fallback-colors':         false,
        'duplicate-properties':    false,
        'order-alphabetical':      false,
        'zero-units':              false,
        'bulletproof-font-face':   false,
        'font-faces':              false,
        'gradients':               false
    };

    /**
     * Custom CSS lint function. Wraps CSSLint but pre-processes
     * modern CSS features that CSSLint cannot parse (custom props, var()).
     */
    function cssLintValidator(text) {
        // 1. Strip custom property declarations (--foo: value;) entirely,
        //    replacing with a harmless placeholder that keeps line structure
        var cleaned = text.replace(/(--[\w-]+)\s*:\s*([^;}{]*)/g, function (m, prop, val) {
            // Replace with a dummy property of the same line count
            var lines = m.split('\n');
            var out = 'color: red';
            for (var i = 1; i < lines.length; i++) { out += '\n'; }
            return out;
        });

        // 2. Replace var(...) references with a valid CSS value placeholder
        //    Handle nested var() like var(--a, var(--b, #fff))
        var maxIter = 10;
        while (maxIter-- > 0 && cleaned.indexOf('var(') !== -1) {
            cleaned = cleaned.replace(/var\s*\([^()]*\)/g, '#000');
        }

        // 3. Replace other modern CSS functions CSSLint can't handle
        cleaned = cleaned.replace(/env\s*\([^)]*\)/g, '0px');
        cleaned = cleaned.replace(/clamp\s*\([^)]*\)/g, '16px');
        cleaned = cleaned.replace(/min\s*\([^)]*\)/g, '0px');
        cleaned = cleaned.replace(/max\s*\([^)]*\)/g, '0px');

        // 4. Replace modern CSS units CSSLint doesn't know
        //    fr (grid), dvh/dvw/svh/svw/lvh/lvw, cqi/cqb, etc.
        cleaned = cleaned.replace(/(\d+(?:\.\d+)?)\s*fr\b/g, '$1px');
        cleaned = cleaned.replace(/(\d+(?:\.\d+)?)\s*(dvh|dvw|svh|svw|lvh|lvw|cqi|cqb|cqw|cqh|cqmin|cqmax)\b/g, '$1px');

        // 5. Replace modern CSS keywords/functions CSSLint can't parse
        cleaned = cleaned.replace(/repeat\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/minmax\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/fit-content\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/\bauto-fill\b/g, '1');
        cleaned = cleaned.replace(/\bauto-fit\b/g, '1');

        var errors = CSSLint.verify(cleaned, CSSLINT_OPTS);
        var found = [];
        var messages = errors.messages || [];
        for (var i = 0; i < messages.length; i++) {
            var msg = messages[i];
            // Skip remaining false positives
            if (msg.message && msg.message.indexOf('Unknown property') !== -1) continue;
            if (msg.message && msg.message.indexOf('Expected') !== -1 && msg.message.indexOf('--') !== -1) continue;
            // Skip "fallback" and "empty" warnings from our replacements
            if (msg.message && msg.message.indexOf('fallback') !== -1) continue;
            found.push({
                from:     CodeMirror.Pos((msg.line || 1) - 1, (msg.col || 1) - 1),
                to:       CodeMirror.Pos((msg.line || 1) - 1, (msg.col || 1)),
                message:  msg.message,
                severity: msg.type === 'error' ? 'error' : 'warning'
            });
        }
        return found;
    }

    /** Check whether lint addons loaded successfully */
    function lintAvailable(mode) {
        if (!CodeMirror.lint) return false;
        if (mode === 'javascript' && typeof JSHINT === 'undefined') return false;
        if (mode === 'css' && typeof CSSLint === 'undefined') return false;
        if (mode === 'htmlmixed' && typeof HTMLHint === 'undefined') return false;
        return true;
    }

    function init(defaultHtml, defaultCss, defaultJs) {

        jsEditor = CodeMirror.fromTextArea(document.getElementById('jsEditor'), {
            mode:              'javascript',
            theme:             'material-darker',
            lineNumbers:       true,
            autoCloseBrackets: true,
            matchBrackets:     true,
            tabSize:           2,
            indentWithTabs:    false,
            lineWrapping:      true,
            gutters:           FOLD_GUTTERS,
            foldGutter:        true,
            lint:              lintAvailable('javascript') ? { options: JSHINT_OPTS } : false,
            extraKeys:         FOLD_KEYS
        });
        jsEditor.setValue(defaultJs || '');

        htmlEditor = CodeMirror.fromTextArea(document.getElementById('htmlEditor'), {
            mode:              'htmlmixed',
            theme:             'material-darker',
            lineNumbers:       true,
            autoCloseTags:     true,
            autoCloseBrackets: true,
            matchBrackets:     true,
            tabSize:           2,
            indentWithTabs:    false,
            lineWrapping:      true,
            gutters:           FOLD_GUTTERS,
            foldGutter:        true,
            lint:              lintAvailable('htmlmixed'),
            extraKeys:         FOLD_KEYS
        });

        cssEditor = CodeMirror.fromTextArea(document.getElementById('cssEditor'), {
            mode:              'css',
            theme:             'material-darker',
            lineNumbers:       true,
            autoCloseBrackets: true,
            matchBrackets:     true,
            tabSize:           2,
            indentWithTabs:    false,
            lineWrapping:      true,
            gutters:           FOLD_GUTTERS,
            foldGutter:        true,
            lint:              lintAvailable('css') ? { getAnnotations: cssLintValidator, async: false } : false,
            extraKeys:         FOLD_KEYS
        });

        var debouncedUpdate = LiveCSS.utils.debounce(updatePreview, 150);
        htmlEditor.on('change', debouncedUpdate);
        cssEditor.on('change',  debouncedUpdate);
        jsEditor.on('change',   debouncedUpdate);

        // Ensure layout is correct before the first render
        setTimeout(function () {
            htmlEditor.refresh();
            cssEditor.refresh();
            jsEditor.refresh();
            updatePreview();
        }, 200);
    }

    /** Rebuild the iframe contents from all three editor values */
    function updatePreview() {
        if (!htmlEditor || !cssEditor || !jsEditor) { return; }
        var frame  = document.getElementById('previewFrame');
        var jsCode = jsEditor.getValue();
        var safeJs = jsCode
            ? '<script>\ntry {\n' + jsCode + '\n} catch (e) { console.error("[preview]", e); }\n<\/script>'
            : '';
        frame.srcdoc =
            '<!DOCTYPE html><html><head><meta charset="UTF-8"><style>' +
            cssEditor.getValue() +
            '<\/style></head><body>' +
            htmlEditor.getValue() +
            safeJs +
            buildContextMenuScript() +
            '</body></html>';
    }



    function getHtmlEditor() { return htmlEditor; }
    function getCssEditor()  { return cssEditor;  }
    function getJsEditor()   { return jsEditor;   }

    function setHtmlValue(val) { if (htmlEditor) { htmlEditor.setValue(val); updatePreview(); } }
    function setCssValue(val)  { if (cssEditor)  { cssEditor.setValue(val);  updatePreview(); } }
    function setJsValue(val)   { if (jsEditor)   { jsEditor.setValue(val);   updatePreview(); } }

    return {
        init:          init,
        updatePreview: updatePreview,
        getHtmlEditor: getHtmlEditor,
        getCssEditor:  getCssEditor,
        getJsEditor:   getJsEditor,
        setHtmlValue:  setHtmlValue,
        setCssValue:   setCssValue,
        setJsValue:    setJsValue
    };

}());
