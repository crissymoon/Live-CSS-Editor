/**
 * editor.js — CodeMirror editor instances and live preview renderer
 * Attached to window.LiveCSS.editor
 *
 * Call LiveCSS.editor.init(defaultHtml, defaultCss) once the DOM is ready.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editor = (function () {

    var htmlEditor, cssEditor;

    function init(defaultHtml, defaultCss) {
        htmlEditor = CodeMirror.fromTextArea(document.getElementById('htmlEditor'), {
            mode: 'htmlmixed',
            theme: 'material-darker',
            lineNumbers: true,
            autoCloseTags: true,
            autoCloseBrackets: true,
            matchBrackets: true,
            tabSize: 2,
            indentWithTabs: false,
            lineWrapping: true,
        });

        cssEditor = CodeMirror.fromTextArea(document.getElementById('cssEditor'), {
            mode: 'css',
            theme: 'material-darker',
            lineNumbers: true,
            autoCloseBrackets: true,
            matchBrackets: true,
            tabSize: 2,
            indentWithTabs: false,
            lineWrapping: true,
        });

        var debouncedUpdate = LiveCSS.utils.debounce(updatePreview, 150);
        htmlEditor.on('change', debouncedUpdate);
        cssEditor.on('change', debouncedUpdate);

        // Ensure layout is correct before the first render
        setTimeout(function () {
            htmlEditor.refresh();
            cssEditor.refresh();
            updatePreview();
        }, 200);
    }

    /** Rebuild the iframe contents from the current editor values */
    function updatePreview() {
        var frame = document.getElementById('previewFrame');
        frame.srcdoc =
            '<!DOCTYPE html><html><head><meta charset="UTF-8"><style>' +
            cssEditor.getValue() +
            '<\/style></head><body>' +
            htmlEditor.getValue() +
            '</body></html>';
    }

    function getHtmlEditor() { return htmlEditor; }
    function getCssEditor()  { return cssEditor;  }

    return { init: init, updatePreview: updatePreview, getHtmlEditor: getHtmlEditor, getCssEditor: getCssEditor };

}());
