// editor.js -- core editor functionality: formatting, undo/redo, insert helpers

// ── Undo / Redo ──────────────────────────────────────────────────────────────
let editorStates = [];
let currentEditorState = 0;
let typingTimer;
let isUndoRedo = false;

function saveState() {
    if (isUndoRedo) return;
    const editor = document.getElementById('editor');
    if (!editor) return;
    const current = editor.innerHTML;
    if (editorStates[currentEditorState] === current) return;
    if (currentEditorState < editorStates.length - 1) {
        editorStates = editorStates.slice(0, currentEditorState + 1);
    }
    editorStates.push(current);
    currentEditorState++;
    if (editorStates.length > 50) {
        editorStates.shift();
        currentEditorState--;
    }
    updateUndoRedoButtons();
}

function updateUndoRedoButtons() {
    const u = document.getElementById('undo');
    const r = document.getElementById('redo');
    if (u) u.disabled = currentEditorState === 0;
    if (r) r.disabled = currentEditorState === editorStates.length - 1;
}

function undo() {
    if (currentEditorState > 0) {
        isUndoRedo = true;
        currentEditorState--;
        document.getElementById('editor').innerHTML = editorStates[currentEditorState];
        updateUndoRedoButtons();
        setTimeout(() => { isUndoRedo = false; }, 100);
    }
}

function redo() {
    if (currentEditorState < editorStates.length - 1) {
        isUndoRedo = true;
        currentEditorState++;
        document.getElementById('editor').innerHTML = editorStates[currentEditorState];
        updateUndoRedoButtons();
        setTimeout(() => { isUndoRedo = false; }, 100);
    }
}

// ── Insert helpers ────────────────────────────────────────────────────────────
function insertHtmlAtSelection(html) {
    const sel = window.getSelection();
    if (!sel.rangeCount) return;
    const range = sel.getRangeAt(0);
    range.deleteContents();
    const div = document.createElement('div');
    div.innerHTML = html;
    const frag = document.createDocumentFragment();
    let node;
    while ((node = div.firstChild)) frag.appendChild(node);
    range.insertNode(frag);
    range.setStartAfter(frag.lastChild || range.startContainer);
    range.collapse(true);
    sel.removeAllRanges();
    sel.addRange(range);
}

function insertHS1() {
    insertHtmlAtSelection(`<section><h2>Heading Special</h2><p>add summary/details. . .</p></section>`);
}

function insertHL() {
    insertHtmlAtSelection(`<div style="background-color:#f0f0f0;padding:5px 10px;border:1px solid #000000;">Highlight background</div>`);
}

function insertHR() {
    const editor = document.getElementById('editor');
    const sel = window.getSelection();
    if (sel.rangeCount > 0) {
        const range = sel.getRangeAt(0);
        const hr = document.createElement('hr');
        range.deleteContents();
        range.insertNode(hr);
        const p = document.createElement('p');
        p.innerHTML = '<br>';
        hr.parentNode.insertBefore(p, hr.nextSibling);
        const nr = document.createRange();
        nr.setStart(p, 0);
        nr.setEnd(p, 0);
        sel.removeAllRanges();
        sel.addRange(nr);
    } else {
        editor.appendChild(document.createElement('hr'));
        editor.appendChild(document.createElement('p')).innerHTML = '<br>';
    }
    editor.focus();
    setTimeout(saveState, 100);
}

function insertList(type) {
    document.execCommand(type === 'ol' ? 'insertOrderedList' : 'insertUnorderedList', false, null);
    document.getElementById('editor').focus();
    setTimeout(saveState, 100);
}

// ── Formatting ────────────────────────────────────────────────────────────────
function clearFormatting() {
    document.execCommand('removeFormat', false, null);
    document.getElementById('editor').focus();
    setTimeout(saveState, 100);
}

function clearFormattingAtSelection() { clearFormatting(); }

function makeUppercase() {
    const sel = window.getSelection();
    if (sel.rangeCount > 0 && !sel.isCollapsed) {
        const range = sel.getRangeAt(0);
        const upper = range.toString().toUpperCase();
        range.deleteContents();
        range.insertNode(document.createTextNode(upper));
        const nr = document.createRange();
        nr.setStart(range.startContainer, range.startOffset);
        nr.setEnd(range.startContainer, range.startOffset + upper.length);
        sel.removeAllRanges();
        sel.addRange(nr);
        setTimeout(saveState, 100);
    } else {
        alert('Please select some text first!');
    }
    document.getElementById('editor').focus();
}

function makeLowercase() {
    const sel = window.getSelection();
    if (sel.rangeCount > 0 && !sel.isCollapsed) {
        const range = sel.getRangeAt(0);
        const lower = range.toString().toLowerCase();
        range.deleteContents();
        range.insertNode(document.createTextNode(lower));
        const nr = document.createRange();
        nr.setStart(range.startContainer, range.startOffset);
        nr.setEnd(range.startContainer, range.startOffset + lower.length);
        sel.removeAllRanges();
        sel.addRange(nr);
        setTimeout(saveState, 100);
    } else {
        alert('Please select some text first!');
    }
    document.getElementById('editor').focus();
}

function setLineSpacing(value) {
    document.getElementById('editor').style.lineHeight = value;
    setTimeout(saveState, 100);
}

function selectText() {
    const div = document.getElementById('editor');
    const range = document.createRange();
    range.selectNodeContents(div);
    const sel = window.getSelection();
    sel.removeAllRanges();
    sel.addRange(range);
}

function toggleSpellCheck() {
    const div = document.getElementById('editor');
    const status = document.getElementById('status');
    div.spellcheck = !div.spellcheck;
    if (status) status.textContent = div.spellcheck ? 'Spell check is ON' : 'Spell check is OFF';
}

// ── Image click-to-resize ─────────────────────────────────────────────────────
document.addEventListener('click', function(e) {
    if (e.target.tagName === 'IMG' && e.target.closest('[contenteditable="true"]')) {
        const w = e.target.style.width || '100px';
        let next;
        if (w === '100px')  next = '200px';
        else if (w === '200px') next = '400px';
        else next = '100px';
        e.target.style.width = next;
        e.target.style.height = 'auto';
    }
});

// ── DOMContentLoaded: wire up all editor controls ─────────────────────────────
document.addEventListener('DOMContentLoaded', function() {
    const editor     = document.getElementById('editor');
    const undoBtn    = document.getElementById('undo');
    const redoBtn    = document.getElementById('redo');
    const fontFamily = document.getElementById('fontFamily');
    const fontSize   = document.getElementById('fontSize');

    // Init undo state
    editorStates = [editor.innerHTML];
    currentEditorState = 0;

    editor.addEventListener('input', function() {
        clearTimeout(typingTimer);
        typingTimer = setTimeout(saveState, 500);
    });
    editor.addEventListener('click', function() { editor.focus(); });

    if (undoBtn) undoBtn.addEventListener('click', undo);
    if (redoBtn) redoBtn.addEventListener('click', redo);

    document.addEventListener('keydown', function(e) {
        if (e.ctrlKey || e.metaKey) {
            if (e.key === 'z' && !e.shiftKey)  { e.preventDefault(); undo(); }
            else if (e.key === 'z' && e.shiftKey) { e.preventDefault(); redo(); }
            else if (e.key === 'y') { e.preventDefault(); redo(); }
        }
    });

    updateUndoRedoButtons();

    // Bold / Italic / Underline
    document.getElementById('boldBtn').addEventListener('click', function() {
        document.execCommand('bold', false, null); editor.focus(); setTimeout(saveState, 100);
    });
    document.getElementById('italicBtn').addEventListener('click', function() {
        document.execCommand('italic', false, null); editor.focus(); setTimeout(saveState, 100);
    });
    document.getElementById('underlineBtn').addEventListener('click', function() {
        document.execCommand('underline', false, null); editor.focus(); setTimeout(saveState, 100);
    });

    // Alignment
    document.getElementById('alignLeft').addEventListener('click', function() {
        document.execCommand('justifyLeft', false, null); editor.focus(); setTimeout(saveState, 100);
    });
    document.getElementById('alignCenter').addEventListener('click', function() {
        document.execCommand('justifyCenter', false, null); editor.focus(); setTimeout(saveState, 100);
    });
    document.getElementById('alignRight').addEventListener('click', function() {
        document.execCommand('justifyRight', false, null); editor.focus(); setTimeout(saveState, 100);
    });

    // Font family
    fontFamily.addEventListener('change', function() {
        const sel = window.getSelection();
        if (sel.rangeCount > 0 && !sel.isCollapsed) {
            document.execCommand('fontName', false, this.value);
        } else {
            editor.style.fontFamily = this.value;
        }
        editor.focus();
        setTimeout(saveState, 100);
    });

    // Font size
    fontSize.addEventListener('change', function() {
        const sel = window.getSelection();
        const size = this.value;
        if (sel.rangeCount > 0 && !sel.isCollapsed) {
            const range = sel.getRangeAt(0);
            const span = document.createElement('span');
            span.style.fontSize = size;
            span.textContent = range.toString();
            range.deleteContents();
            range.insertNode(span);
            const nr = document.createRange();
            nr.selectNodeContents(span);
            sel.removeAllRanges();
            sel.addRange(nr);
        } else {
            editor.style.fontSize = size;
        }
        editor.focus();
        setTimeout(saveState, 100);
    });

    // Clear button
    const selectDeleteBtn = document.getElementById('selectDeleteBtn');
    if (selectDeleteBtn) {
        selectDeleteBtn.addEventListener('click', function() {
            const div = document.getElementById('editor');
            const range = document.createRange();
            range.selectNodeContents(div);
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(range);
            if (confirm('Are you sure you want to clear all text?')) {
                div.textContent = '';
            } else {
                sel.removeAllRanges();
            }
        });
    }
});
