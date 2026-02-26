/**
 * editor.js — CodeMirror editor instances and live preview renderer
 * Attached to window.LiveCSS.editor
 *
 * Call LiveCSS.editor.init(defaultHtml, defaultCss, defaultJs) once the DOM is ready.
 * Provides four editors: JS, HTML, CSS, and a live preview iframe.
 * All editors have code folding (Ctrl/Cmd+Q to fold at cursor) and fold gutters.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editor = (function () {

    var htmlEditor, cssEditor, jsEditor;

    var FOLD_KEYS = {
        'Ctrl-Q': function (cm) { cm.foldCode(cm.getCursor()); },
        'Cmd-Q':  function (cm) { cm.foldCode(cm.getCursor()); }
    };

    var FOLD_GUTTERS = ['CodeMirror-linenumbers', 'CodeMirror-foldgutter'];

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
