// templates.js -- load template HTML files from templates/ via fetch

async function loadTemplate(filename) {
    try {
        const resp = await fetch('templates/' + filename);
        if (!resp.ok) throw new Error('HTTP ' + resp.status);
        const html = await resp.text();
        document.getElementById('editor').innerHTML = html;
        saveState();
    } catch (e) {
        alert('Could not load template: ' + e.message);
    }
}

document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('fillButton').addEventListener('click',     () => loadTemplate('business-resume.html'));
    document.getElementById('basicResume').addEventListener('click',    () => loadTemplate('basic-resume.html'));
    document.getElementById('hospitalityBtn').addEventListener('click', () => loadTemplate('hospitality-resume.html'));
    document.getElementById('constructionBTN').addEventListener('click',() => loadTemplate('construction-resume.html'));
    document.getElementById('coverL').addEventListener('click',         () => loadTemplate('basic-cover-letter.html'));
    document.getElementById('fancyCL').addEventListener('click',        () => loadTemplate('fancy-cover-letter.html'));
});
