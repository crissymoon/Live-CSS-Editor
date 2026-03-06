// ui.js -- modal, print-scale, dropdowns, AutoSaveManager, copy button, success message

// ── Success Message ──────────────────────────────────────────────────────────
function showSuccessMessage() {
    const message = document.getElementById('successMessage');
    message.style.display = 'inline';
    setTimeout(function() {
        message.style.display = 'none';
    }, 2000);
}

// ── AutoSaveManager ──────────────────────────────────────────────────────────
class AutoSaveManager {
    constructor(options = {}) {
        this.options = {
            storageKey:    'autoSaveData',
            debounceTime:  1000,
            statusElement: null,
            ...options
        };
        this.editableElements = [];
        this.saveTimeout = null;
        this.initialize();
    }

    initialize() {
        this.editableElements = Array.from(
            document.querySelectorAll('[contenteditable][data-save-id]')
        );
        this.loadContent();
        this.setupEventListeners();
    }

    setupEventListeners() {
        this.editableElements.forEach(el =>
            el.addEventListener('input', this.handleInput.bind(this))
        );
    }

    handleInput() {
        if (this.saveTimeout) clearTimeout(this.saveTimeout);
        this.showStatus('Saving...');
        this.saveTimeout = setTimeout(() => {
            this.saveContent();
            this.showStatus('All changes saved', true);
            setTimeout(() => this.showStatus(''), 2000);
        }, this.options.debounceTime);
    }

    saveContent() {
        const saveData = {};
        this.editableElements.forEach(el => {
            saveData[el.getAttribute('data-save-id')] = el.innerHTML;
        });
        localStorage.setItem(this.options.storageKey, JSON.stringify(saveData));
    }

    loadContent() {
        const saved = localStorage.getItem(this.options.storageKey);
        if (!saved) return;
        try {
            const parsedData = JSON.parse(saved);
            this.editableElements.forEach(el => {
                const id = el.getAttribute('data-save-id');
                if (parsedData[id]) el.innerHTML = parsedData[id];
            });
            this.showStatus('Loaded saved content', true);
        } catch (e) {
            console.error('Error loading saved content:', e);
        }
    }

    showStatus(message, success = false) {
        if (!this.options.statusElement) return;
        const el = typeof this.options.statusElement === 'string'
            ? document.querySelector(this.options.statusElement)
            : this.options.statusElement;
        if (el) {
            el.textContent = message;
            el.style.color = success ? '#4CAF50' : '#ff9800';
        }
    }

    clearSavedData() {
        localStorage.removeItem(this.options.storageKey);
        this.showStatus('Cleared saved data', true);
        setTimeout(() => this.showStatus(''), 2000);
    }
}

// ── DOMContentLoaded wiring ──────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', function() {

    // ── AutoSaveManager init ─────────────────────────────────────────────────
    window.autoSaveManager = new AutoSaveManager({ statusElement: '#status' });

    // ── Copy All button ──────────────────────────────────────────────────────
    function fallbackCopy(html) {
        const tmp = document.createElement('div');
        tmp.innerHTML = html;
        document.body.appendChild(tmp);
        const range = document.createRange();
        range.selectNode(tmp);
        window.getSelection().removeAllRanges();
        window.getSelection().addRange(range);
        try {
            if (document.execCommand('copy')) showSuccessMessage();
            else alert('Your browser may not support copying formatted content. Use Ctrl+C instead.');
        } catch (err) {
            alert('Your browser may not support copying formatted content. Use Ctrl+C instead.');
        }
        window.getSelection().removeAllRanges();
        document.body.removeChild(tmp);
    }

    document.getElementById('copyButton').addEventListener('click', function() {
        const contentToCopy = document.getElementById('editor');
        const range = document.createRange();
        range.selectNode(contentToCopy);
        window.getSelection().removeAllRanges();
        window.getSelection().addRange(range);
        try {
            if (document.execCommand('copy')) showSuccessMessage();
            else fallbackCopy(contentToCopy.innerHTML);
        } catch (err) {
            fallbackCopy(contentToCopy.innerHTML);
        }
        window.getSelection().removeAllRanges();
    });

    // ── Modal (Print popup) ──────────────────────────────────────────────────
    const modal = document.getElementById('myModal');
    const btn   = document.getElementById('myBtn');
    const span  = document.getElementsByClassName('close')[0];

    if (btn)  btn.onclick  = () => modal.style.display = 'block';
    if (span) span.onclick = () => modal.style.display = 'none';
    window.addEventListener('click', function(e) {
        if (e.target === modal) modal.style.display = 'none';
    });

    // ── Print Scale ──────────────────────────────────────────────────────────
    let scale = 1;
    const scaleValueEl         = document.getElementById('scale-value');
    const currentScaleDisplay  = document.getElementById('current-scale-display');

    function updateScale() {
        const pct = Math.round(scale * 100);
        if (scaleValueEl)        scaleValueEl.textContent       = pct + '%';
        if (currentScaleDisplay) currentScaleDisplay.textContent = pct + '%';
    }

    document.getElementById('scale-up').addEventListener('click', function() {
        if (scale < 2) { scale = Math.round((scale + 0.1) * 10) / 10; updateScale(); }
    });
    document.getElementById('scale-down').addEventListener('click', function() {
        if (scale > 0.5) { scale = Math.round((scale - 0.1) * 10) / 10; updateScale(); }
    });
    document.getElementById('print-btn').addEventListener('click', function() {
        const printContents = document.getElementById('editor').innerHTML;
        document.body.innerHTML = printContents;
        document.body.style.setProperty('--print-scale', scale);
        window.print();
        window.location.reload();
    });

    // ── Mobile Dropdowns ─────────────────────────────────────────────────────
    const dropdowns = document.querySelectorAll('.dropdown');
    dropdowns.forEach(function(dropdown) {
        const toggle = dropdown.querySelector('.dropdown-toggle');
        if (toggle) {
            toggle.addEventListener('click', function(e) {
                e.stopPropagation();
                dropdowns.forEach(d => { if (d !== dropdown) d.classList.remove('active'); });
                dropdown.classList.toggle('active');
            });
        }
        const content = dropdown.querySelector('.dropdown-content');
        if (content) {
            content.addEventListener('click', function() {
                dropdown.classList.remove('active');
            });
        }
    });
    document.addEventListener('click', function(e) {
        if (!e.target.closest('.dropdown')) {
            dropdowns.forEach(d => d.classList.remove('active'));
        }
    });
});
