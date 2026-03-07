/**
 * editor.js -- CodeMirror editor instances and live preview orchestrator
 * Attached to window.LiveCSS.editor
 *
 * Depends on (must be loaded first):
 *   js/editor/goto-css.js  -> window.LiveCSS.editorGotoCSS
 *   js/editor/lint.js      -> window.LiveCSS.editorLint
 *   js/editor/preview.js   -> window.LiveCSS.editorPreview
 *
 * Call LiveCSS.editor.init(defaultHtml, defaultCss, defaultJs) once the DOM is ready.
 * Provides three editors: JS, HTML, CSS and a live preview iframe.
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

    function init(defaultHtml, defaultCss, defaultJs) {
        var lint = LiveCSS.editorLint;

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
            lint:              lint.lintAvailable('javascript') ? { options: lint.JSHINT_OPTS } : false,
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
            lint:              lint.lintAvailable('htmlmixed'),
            extraKeys:         FOLD_KEYS
        });
        htmlEditor.setValue(defaultHtml || '');

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
            lint:              lint.lintAvailable('css')
                                   ? { getAnnotations: lint.cssLintValidator, async: false }
                                   : false,
            extraKeys:         FOLD_KEYS
        });
        cssEditor.setValue(defaultCss || '');

        var debouncedUpdate = LiveCSS.utils.debounce(function () {
            updatePreview();
        }, 150);

        // CSS-only changes: patch the existing style element in-place so the
        // preview scroll position is preserved. Fall back to a full srcdoc
        // rebuild if the iframe is not yet ready (e.g. on first load).
        var debouncedCssUpdate = LiveCSS.utils.debounce(function () {
            try {
                if (cssEditor) {
                    var patched = setPreviewCss(cssEditor.getValue());
                    if (!patched) {
                        console.log('[editor] debouncedCssUpdate: in-place patch failed, running full updatePreview');
                        updatePreview();
                    }
                } else {
                    console.warn('[editor] debouncedCssUpdate: cssEditor not available');
                }
            } catch (e) {
                console.error('[editor] debouncedCssUpdate error:', e);
                updatePreview();
            }
        }, 150);

        htmlEditor.on('change', debouncedUpdate);
        cssEditor.on('change',  debouncedCssUpdate);
        jsEditor.on('change',   debouncedUpdate);

        // Listen for "Go to in CSS" messages sent from the preview iframe context menu
        window.addEventListener('message', function (e) {
            if (!e.data || e.data.type !== 'livecss-goto-css') { return; }
            console.log('[GoToCSS] postMessage received, selector:', e.data.selector);
            if (!cssEditor) {
                console.error('[GoToCSS] message received but cssEditor is not initialised');
                return;
            }
            LiveCSS.editorGotoCSS.jumpToCssRule(cssEditor, e.data.selector);
        });

        // Ensure layout is correct before the first render
        setTimeout(function () {
            htmlEditor.refresh();
            cssEditor.refresh();
            jsEditor.refresh();
            updatePreview();
        }, 200);
    }

    function updatePreview() {
        LiveCSS.editorPreview.updatePreview(htmlEditor, cssEditor, jsEditor);
    }

    function setPreviewCss(fullCss) {
        return LiveCSS.editorPreview.setPreviewCss(fullCss);
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
        setPreviewCss: setPreviewCss,
        getHtmlEditor: getHtmlEditor,
        getCssEditor:  getCssEditor,
        getJsEditor:   getJsEditor,
        setHtmlValue:  setHtmlValue,
        setCssValue:   setCssValue,
        setJsValue:    setJsValue
    };

}());
