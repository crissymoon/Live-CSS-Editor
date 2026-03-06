// color.js -- color history management and color picker event handlers

const MAX_COLORS = 6;

function loadColorHistory(key) {
    const saved = localStorage.getItem(key);
    return saved ? JSON.parse(saved) : [];
}

function saveColorHistory(key, colors) {
    localStorage.setItem(key, JSON.stringify(colors));
}

function addToColorHistory(color, containerId, pickerType) {
    const key = 'colorHistory_' + pickerType;
    let colors = loadColorHistory(key);
    colors = colors.filter(c => c !== color);
    colors.unshift(color);
    if (colors.length > MAX_COLORS) colors = colors.slice(0, MAX_COLORS);
    saveColorHistory(key, colors);
    renderColorHistory(containerId, colors, pickerType);
}

function renderColorHistory(containerId, colors, pickerType) {
    const container = document.getElementById(containerId);
    if (!container) return;
    container.innerHTML = '';
    colors.forEach(color => {
        const swatch = document.createElement('div');
        swatch.className = 'color-swatch';
        swatch.style.backgroundColor = color;
        swatch.title = color;
        swatch.addEventListener('click', function() {
            const picker = document.getElementById(pickerType);
            if (picker) {
                picker.value = color;
                picker.dispatchEvent(new Event('change', { bubbles: true }));
            }
        });
        container.appendChild(swatch);
    });
}

// ── savedSelection -- saved before color inputs steal focus ──────────────────
let savedSelection = null;

function saveEditorSelection() {
    const sel = window.getSelection();
    if (sel && sel.rangeCount > 0) savedSelection = sel.getRangeAt(0).cloneRange();
}

document.addEventListener('DOMContentLoaded', function() {
    // Render previously saved color histories
    renderColorHistory('textColorHistory',      loadColorHistory('colorHistory_textColor'),  'textColor');
    renderColorHistory('highlightColorHistory', loadColorHistory('colorHistory_colorPicker'), 'colorPicker');

    const textColorInput = document.getElementById('textColor');
    const highlightInput = document.getElementById('colorPicker');
    const bgColorInput   = document.getElementById('bgColor');
    const editor         = document.getElementById('editor');

    // Restore saved page background color
    const savedBg = localStorage.getItem('editorBgColor');
    if (savedBg) {
        editor.style.background = savedBg;
        if (bgColorInput) bgColorInput.value = savedBg;
    }

    // Page background color change
    if (bgColorInput) {
        bgColorInput.addEventListener('change', function () {
            editor.style.background = this.value;
            localStorage.setItem('editorBgColor', this.value);
        });
    }

    // Save selection on mousedown (fires before the input steals focus)
    textColorInput.addEventListener('mousedown', saveEditorSelection);
    highlightInput.addEventListener('mousedown', saveEditorSelection);

    // Fallback: save on editor blur
    editor.addEventListener('blur', function() {
        const sel = window.getSelection();
        if (sel && sel.rangeCount > 0 && !sel.isCollapsed)
            savedSelection = sel.getRangeAt(0).cloneRange();
    });

    // Text color change
    textColorInput.addEventListener('change', function() {
        const color = this.value;
        if (savedSelection) {
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(savedSelection);
            editor.focus();
            saveState();
            document.execCommand('foreColor', false, color);
            setTimeout(saveState, 100);
            addToColorHistory(color, 'textColorHistory', 'textColor');
            savedSelection = null;
        } else {
            editor.focus();
            document.execCommand('foreColor', false, color);
            addToColorHistory(color, 'textColorHistory', 'textColor');
        }
    });

    // Highlight color change
    highlightInput.addEventListener('change', function() {
        const color = this.value;
        if (savedSelection) {
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(savedSelection);
            editor.focus();
            saveState();
            document.execCommand('backColor', false, color);
            setTimeout(saveState, 100);
            addToColorHistory(color, 'highlightColorHistory', 'colorPicker');
            savedSelection = null;
        }
        // No selection -- silently do nothing
    });
});
