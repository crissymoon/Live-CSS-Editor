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
     * Jump the CSS editor to the first rule that references part of `selector`.
     * Search order: each class name, then the id, then the tag name.
     * Also ensures the CSS panel is visible (restores it from minimized state if needed)
     * and calls CodeMirror refresh so the selection is visible after a panel show.
     */
    function jumpToCssRule(selector) {
        if (!cssEditor) {
            console.warn('[GoToCSS] cssEditor not available');
            return;
        }
        if (!selector || !selector.trim()) {
            console.warn('[GoToCSS] empty selector -- nothing to search for');
            return;
        }

        // Build an ordered list of search terms from most to least specific
        var terms = [];
        var classes = selector.match(/\.[^.#\s[\]()]+/g) || [];
        for (var i = 0; i < classes.length; i++) { terms.push(classes[i]); }
        var idMatch = selector.match(/#[^.#\s[\]()]+/);
        if (idMatch) { terms.push(idMatch[0]); }
        var tagMatch = selector.match(/^[a-zA-Z][a-zA-Z0-9]*/);
        if (tagMatch && tagMatch[0] !== 'html' && tagMatch[0] !== 'body') {
            terms.push(tagMatch[0]);
        }

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
                var col   = lower.indexOf(needle);
                if (col === -1) { continue; }
                // Only match in selector portion (before any '{' on this line)
                var bracePos = lower.indexOf('{');
                if (bracePos !== -1 && col > bracePos) { continue; }
                foundLn  = ln;
                foundCol = col;
                foundLen = terms[j].length;
                console.log('[GoToCSS] found "' + terms[j] + '" at line ' + (ln + 1) + ' col ' + col);
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
                console.log('[GoToCSS] scrolled to line ' + (foundLn + 1));
            } catch (e) {
                console.error('[GoToCSS] setSelection/scrollIntoView failed:', e);
            }
        }

        // Ensure the CSS panel is visible. If it has been minimized the panel's
        // display is set to 'none' by gutter.js. We need to show it and remove
        // its taskbar chip before CodeMirror can render the selection.
        var cssPanel  = document.getElementById('cssPanel');
        var isHidden  = cssPanel && cssPanel.style.display === 'none';

        if (isHidden) {
            console.log('[GoToCSS] CSS panel is minimized -- restoring it');
            // Restore geometry that gutter.js saved before minimising
            if (cssPanel.dataset.savedLeft)   { cssPanel.style.left   = cssPanel.dataset.savedLeft; }
            if (cssPanel.dataset.savedTop)    { cssPanel.style.top    = cssPanel.dataset.savedTop; }
            if (cssPanel.dataset.savedWidth)  { cssPanel.style.width  = cssPanel.dataset.savedWidth; }
            if (cssPanel.dataset.savedHeight) { cssPanel.style.height = cssPanel.dataset.savedHeight; }
            cssPanel.style.display = '';

            // Remove the matching taskbar chip
            try {
                var taskbar = document.getElementById('panel-taskbar');
                if (taskbar) {
                    var chip = taskbar.querySelector('[data-panel-id="cssPanel"]');
                    if (chip && chip.parentNode) { chip.parentNode.removeChild(chip); }
                }
            } catch (e) {
                console.warn('[GoToCSS] could not remove taskbar chip:', e);
            }

            // Give CodeMirror time to re-render after the panel becomes visible
            setTimeout(function () {
                try { cssEditor.refresh(); } catch (e) { console.warn('[GoToCSS] refresh failed:', e); }
                setTimeout(doScroll, 60);
            }, 40);
        } else {
            doScroll();
        }
    }

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
            var lastSelector = '';

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
                lastSelector = selector;

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
                h += '<div data-goto-css="1" style="padding:5px 10px;cursor:pointer;color:#7c6fb0;">Go to in CSS</div>';
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
                    if (t.getAttribute && (t.getAttribute('data-copy') === '1' || t.getAttribute('data-goto-css') === '1')) { t.style.background = '#1a1130'; return; }
                    t = t.parentNode;
                }
            });
            menu.addEventListener('mouseout', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && (t.getAttribute('data-copy') === '1' || t.getAttribute('data-goto-css') === '1')) { t.style.background = 'transparent'; return; }
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
                    if (t.getAttribute && t.getAttribute('data-goto-css') === '1') {
                        console.log('[GoToCSS] sending postMessage for selector:', lastSelector);
                        window.parent.postMessage({ type: 'livecss-goto-css', selector: lastSelector }, '*');
                        hideMenu();
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

        // Listen for "Go to in CSS" messages from the preview iframe context menu
        window.addEventListener('message', function (e) {
            if (!e.data || e.data.type !== 'livecss-goto-css') { return; }
            console.log('[GoToCSS] postMessage received, selector:', e.data.selector);
            if (!cssEditor) {
                console.error('[GoToCSS] message received but cssEditor is not initialised');
                return;
            }
            jumpToCssRule(e.data.selector);
        });

        // Ensure layout is correct before the first render
        setTimeout(function () {
            htmlEditor.refresh();
            cssEditor.refresh();
            jsEditor.refresh();
            updatePreview();
        }, 200);
    }

    /**
     * Intercept anchor clicks inside the preview iframe so that:
     *  - hash-only links (#section) smooth-scroll within the preview instead
     *    of navigating the iframe (which would load the parent app URL + hash)
     *  - external / full-URL links are suppressed entirely to keep the preview
     *    self-contained
     */
    function buildNavFixScript() {
        var fn = function () {
            document.addEventListener('click', function (e) {
                var a = e.target.closest ? e.target.closest('a[href]') : null;
                if (!a) {
                    // fallback for browsers without closest
                    var el = e.target;
                    while (el && el.tagName !== 'A') { el = el.parentElement; }
                    if (el && el.hasAttribute('href')) { a = el; }
                }
                if (!a) { return; }
                var href = a.getAttribute('href') || '';
                // Always prevent default – keep preview contained
                e.preventDefault();
                if (href.charAt(0) === '#' && href.length > 1) {
                    var target = document.querySelector(href);
                    if (target) { target.scrollIntoView({ behavior: 'smooth' }); }
                }
                // All other links (external, relative) are intentionally blocked
            });
        };
        return '<script>(' + fn.toString() + ')();<\/script>';
    }

    /**
     * Base CSS reset injected before user CSS.
     * Strips native OS form-control appearance (macOS forces its own styling
     * on select/input/button/textarea without this).
     */
    var PREVIEW_BASE_CSS =
        'select,input,button,textarea{' +
            '-webkit-appearance:none;' +
            '-moz-appearance:none;' +
            'appearance:none;' +
        '}' +
        'select{background-image:none;}';

    /** Rebuild the iframe contents from all three editor values */
    function updatePreview() {
        if (!htmlEditor || !cssEditor || !jsEditor) { return; }
        var frame   = document.getElementById('previewFrame');
        var htmlVal = htmlEditor.getValue();
        var jsCode  = jsEditor.getValue();
        var safeJs  = jsCode
            ? '<script>\ntry {\n' + jsCode + '\n} catch (e) { console.error("[preview]", e); }\n<\/script>'
            : '';
        var userCss = cssEditor.getValue();
        var styleBlocks =
            '<style>' + PREVIEW_BASE_CSS + '<\/style>' +
            '<style data-livecss-user="1">' + (userCss || '') + '<\/style>';
        var scriptBlocks = buildNavFixScript() + buildContextMenuScript() + safeJs;

        // If the HTML editor contains a full document, inject our extras into its <head>
        // rather than double-wrapping it in another HTML shell.
        var isFullDoc = /^\s*<!doctype\s|^\s*<html[\s>]/i.test(htmlVal);
        if (isFullDoc) {
            var injected = htmlVal;
            // Inject styles into <head>, scripts before </body>
            if (/<\/head>/i.test(injected)) {
                injected = injected.replace(/<\/head>/i, styleBlocks + '<\/head>');
            } else {
                injected = styleBlocks + injected;
            }
            if (/<\/body>/i.test(injected)) {
                injected = injected.replace(/<\/body>/i, scriptBlocks + '<\/body>');
            } else {
                injected = injected + scriptBlocks;
            }
            frame.srcdoc = injected;
        } else {
            frame.srcdoc =
                '<!DOCTYPE html><html><head><meta charset="UTF-8">' +
                styleBlocks +
                '<\/head><body>' +
                htmlVal +
                scriptBlocks +
                '</body></html>';
        }
    }



    function getHtmlEditor() { return htmlEditor; }
    function getCssEditor()  { return cssEditor;  }
    function getJsEditor()   { return jsEditor;   }

    function setHtmlValue(val) { if (htmlEditor) { htmlEditor.setValue(val); updatePreview(); } }
    function setCssValue(val)  { if (cssEditor)  { cssEditor.setValue(val);  updatePreview(); } }
    function setJsValue(val)   { if (jsEditor)   { jsEditor.setValue(val);   updatePreview(); } }

    /**
     * Update only the user CSS in the live preview without rebuilding the
     * entire srcdoc. Used by color-swatch and size-slider for real-time drag
     * feedback. Rebuilding srcdoc on every drag is too slow and resets scroll.
     */
    function setPreviewCss(fullCss) {
        try {
            var frame = document.getElementById('previewFrame');
            if (!frame) { console.warn('[editor] setPreviewCss: previewFrame not found'); return; }
            var fdoc = frame.contentDocument || (frame.contentWindow && frame.contentWindow.document);
            if (!fdoc) { console.warn('[editor] setPreviewCss: cannot access iframe document'); return; }
            var styleEl = fdoc.querySelector('style[data-livecss-user]');
            if (!styleEl) {
                var all = fdoc.querySelectorAll('style');
                styleEl = all.length > 1 ? all[all.length - 1] : (all[0] || null);
                if (!styleEl) { console.warn('[editor] setPreviewCss: no style tag found in preview'); return; }
                console.warn('[editor] setPreviewCss: falling back to last style tag -- preview may need refresh');
            }
            styleEl.textContent = fullCss;
        } catch (e) {
            console.error('[editor] setPreviewCss failed:', e);
        }
    }

    return {
        init:          init,
        updatePreview: updatePreview,
        setPreviewCss: setPreviewCss,
        getHtmlEditor: getHtmlEditor,
        getCssEditor:  getCssEditor,
        getJsEditor:   getJsEditor,
        setHtmlValue:  setHtmlValue,
        setCssValue:   setCssValue,
        setJsValue:    setJsValue
    };

}());
