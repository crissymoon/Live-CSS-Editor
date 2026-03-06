// file.js -- save, load, insert image, insert link

// ── Save as Word ────────────────────────────────────────────────────────────
function saveAsWord() {
    const content = document.getElementById('editor').innerHTML;
    const html = `<html xmlns:o="urn:schemas-microsoft-com:office:office"
          xmlns:w="urn:schemas-microsoft-com:office:word"
          xmlns="http://www.w3.org/TR/REC-html40">
<head><meta charset="UTF-8"><title>Document</title></head>
<body>${content}</body>
</html>`;
    const blob = new Blob(['\ufeff', html], { type: 'application/msword' });
    const url  = URL.createObjectURL(blob);
    const a    = document.createElement('a');
    a.href = url;
    a.download = 'document.doc';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// ── Insert Link ─────────────────────────────────────────────────────────────
function insertLinkAtSelection() {
    const editableDiv = document.getElementById('editor');
    const selection   = window.getSelection();
    if (selection.rangeCount === 0 || !selection.toString().trim()) {
        alert('Please select/highlight some text to turn it into a link.');
        return;
    }
    const range = selection.getRangeAt(0);
    const url   = prompt('Enter the URL/Webpage Address:', 'https://');
    if (!url) return;

    const link       = document.createElement('a');
    link.href        = url;
    link.textContent = selection.toString();
    range.deleteContents();
    range.insertNode(link);

    const newRange = document.createRange();
    newRange.setStartAfter(link);
    newRange.collapse(true);
    selection.removeAllRanges();
    selection.addRange(newRange);
    editableDiv.focus();
}

// ── Insert Image at Cursor ──────────────────────────────────────────────────
function insertImageAtCursor(imageUrl) {
    const editor    = document.getElementById('editor');
    const selection = window.getSelection();
    const img       = document.createElement('img');
    img.src          = imageUrl;
    img.style.maxWidth  = '100%';
    img.style.height    = 'auto';

    if (selection.rangeCount > 0) {
        const range = selection.getRangeAt(0);
        range.deleteContents();
        range.insertNode(img);
        const newRange = document.createRange();
        newRange.setStartAfter(img);
        newRange.collapse(true);
        selection.removeAllRanges();
        selection.addRange(newRange);
    } else {
        editor.appendChild(img);
    }
    document.getElementById('imageInput').value = '';
}

// ── Load Editz HTML File ─────────────────────────────────────────────────────
function loadFile() {
    const fileInput  = document.getElementById('fileInput');
    const contentDiv = document.getElementById('editor');

    if (fileInput.files.length === 0) {
        contentDiv.innerHTML = '<p style="color:red;">Please select an HTML file first.</p>';
        return;
    }
    const file = fileInput.files[0];
    if (!file.name.endsWith('.html') && !file.name.endsWith('.htm')) {
        contentDiv.innerHTML = '<p style="color:red;">Please select a valid HTML file (.html or .htm).</p>';
        return;
    }
    const reader = new FileReader();
    reader.onload  = e => { contentDiv.innerHTML = e.target.result; };
    reader.onerror = () => { contentDiv.innerHTML = '<p style="color:red;">Error reading the file.</p>'; };
    reader.readAsText(file);
}

// ── DOMContentLoaded wiring ──────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', function() {

    // Save as Editz (HTML)
    document.getElementById('saveButton').addEventListener('click', function() {
        const divContent = document.getElementById('editor').innerHTML;
        const fileName   = prompt('Enter a name for the HTML __EDITZ__ file:', 'editz-file');
        if (!fileName) return;
        const blob = new Blob([divContent], { type: 'text/html' });
        const a    = document.createElement('a');
        a.href     = URL.createObjectURL(blob);
        a.download = fileName.endsWith('.html') ? fileName : fileName + '.html';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(a.href);
    });

    // Insert Link button
    document.getElementById('insertLinkBtn').addEventListener('click', insertLinkAtSelection);

    // Image file input
    document.getElementById('imageInput').addEventListener('change', function(event) {
        const file = event.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = e => insertImageAtCursor(e.target.result);
        reader.readAsDataURL(file);
    });

    // Load HTML file input
    document.getElementById('fileInput').addEventListener('change', loadFile);
});
