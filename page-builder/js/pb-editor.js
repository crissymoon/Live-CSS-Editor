/**
 * page-builder/js/pb-editor.js
 * Right-click live DOM editor for the Page Builder.
 * Requires window.PB_CONFIG = { page: string, saveUrl: string } to be set
 * before this script loads (injected by watcher.php).
 */
;(function () {
    'use strict';

    // ------------------------------------------------------------------ //
    //  Logging helpers
    // ------------------------------------------------------------------ //
    const log  = (...a) => console.log('[PBEditor]',   ...a);
    const warn = (...a) => console.warn('[PBEditor]',  ...a);
    const err  = (...a) => console.error('[PBEditor]', ...a);

    // ------------------------------------------------------------------ //
    //  Guard: PB_CONFIG must be defined by watcher.php
    // ------------------------------------------------------------------ //
    if (typeof window.PB_CONFIG === 'undefined') {
        err('PB_CONFIG is not defined. pb-editor.js must be loaded via watcher.php which injects the config. Editor disabled.');
        return;
    }

    const PB_CONFIG = window.PB_CONFIG;

    if (!PB_CONFIG.page) {
        err('PB_CONFIG.page is empty. Editor disabled.');
        return;
    }
    if (!PB_CONFIG.saveUrl) {
        err('PB_CONFIG.saveUrl is empty. Edits will not be persisted.');
    }

    // ------------------------------------------------------------------ //
    //  CSS property map  (pb-prop name -> CSS property name)
    // ------------------------------------------------------------------ //
    const CSS_MAP = {
        color:         'color',
        bg:            'backgroundColor',
        background:    'backgroundColor',
        borderColor:   'borderColor',
        fontSize:      'fontSize',
        padding:       'padding',
        paddingTop:    'paddingTop',
        paddingBottom: 'paddingBottom',
        letterSpacing: 'letterSpacing',
        lineHeight:    'lineHeight',
        borderRadius:  'borderRadius',
        opacity:       'opacity',
        width:         'width',
        height:        'height',
    };

    const COLOR_PROPS = new Set(['color', 'bg', 'background', 'borderColor']);

    // Props that are HTML attributes, not CSS properties
    // applyChange uses setAttribute() for these instead of el.style[]
    const ATTR_PROPS = new Set(['src', 'alt', 'href', 'title']);

    // Props rendered as range sliders
    const RANGE_PROPS = {
        width:        { min: 40,  max: 1400, step: 4,    unit: 'px' },
        height:       { min: 40,  max: 1400, step: 4,    unit: 'px' },
        opacity:      { min: 0,   max: 1,    step: 0.01, unit: '' },
        lineHeight:   { min: 0.8, max: 3,    step: 0.1,  unit: '' },
        borderRadius: { min: 0,   max: 80,   step: 1,    unit: 'px' },
    };

    // ------------------------------------------------------------------ //
    //  Debounce utility
    // ------------------------------------------------------------------ //
    function debounce(fn, ms) {
        let t;
        return function (...args) {
            clearTimeout(t);
            t = setTimeout(() => fn.apply(this, args), ms);
        };
    }

    // ------------------------------------------------------------------ //
    //  Utility: rgb(r,g,b) -> #rrggbb
    // ------------------------------------------------------------------ //
    function rgbToHex(rgb) {
        if (!rgb || rgb === 'transparent') return '#000000';
        const m = rgb.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)/);
        if (!m) return (typeof rgb === 'string' && rgb.startsWith('#')) ? rgb : '#000000';
        return '#' + [m[1], m[2], m[3]]
            .map(n => parseInt(n, 10).toString(16).padStart(2, '0')).join('');
    }

    // ------------------------------------------------------------------ //
    //  Parse fetch response safely (falls back to text when server returns
    //  non-JSON, e.g. a PHP fatal error page)
    // ------------------------------------------------------------------ //
    function parseJsonResponse(res) {
        const ct = res.headers.get('content-type') || '';
        if (ct.includes('application/json')) {
            return res.json();
        }
        return res.text().then(function (text) {
            warn('Expected JSON from server but received non-JSON (HTTP ' + res.status + '):', text.slice(0, 300));
            return { ok: false, error: 'Server returned non-JSON (HTTP ' + res.status + '). Check PHP error logs.' };
        });
    }

    // ------------------------------------------------------------------ //
    //  Panel singleton
    // ------------------------------------------------------------------ //
    let panel      = null;
    let activeEl   = null;
    let saveStatus = null;

    function buildPanel() {
        try {
            const div = document.createElement('div');
            div.id = 'pb-editor-panel';
            div.innerHTML = [
                '<div class="pb-panel-header">',
                '  <span class="pb-panel-id">#</span>',
                '  <button class="pb-panel-close" title="Close">x</button>',
                '</div>',
                '<div class="pb-panel-body"></div>',
                '<div class="pb-panel-status pb-status-ok">ready</div>',
            ].join('');
            document.body.appendChild(div);

            const closeBtn = div.querySelector('.pb-panel-close');
            if (closeBtn) {
                closeBtn.addEventListener('click', closePanel);
            } else {
                warn('buildPanel: .pb-panel-close not found in panel HTML');
            }

            saveStatus = div.querySelector('.pb-panel-status');
            if (!saveStatus) warn('buildPanel: .pb-panel-status not found in panel HTML');

            return div;
        } catch (e) {
            err('buildPanel failed:', e);
            return null;
        }
    }

    function getPanel() {
        if (!panel) panel = buildPanel();
        return panel;
    }

    function closePanel() {
        try {
            if (panel) panel.classList.remove('pb-panel-open');
            if (activeEl) { activeEl.classList.remove('pb-selected'); activeEl = null; }
        } catch (e) {
            err('closePanel failed:', e);
        }
    }

    // ------------------------------------------------------------------ //
    //  Status bar helper
    // ------------------------------------------------------------------ //
    function setStatus(msg, cls) {
        if (!saveStatus) return;
        try {
            saveStatus.textContent = msg;
            saveStatus.className   = 'pb-panel-status ' + (cls || '');
        } catch (e) {
            err('setStatus failed:', e);
        }
    }

    // ------------------------------------------------------------------ //
    //  Persist change to server (debounced 350ms)
    // ------------------------------------------------------------------ //
    const debouncedSave = debounce(function (page, id, prop, value) {
        if (!PB_CONFIG.saveUrl) {
            warn('debouncedSave: saveUrl not set - skipping persist for', id, prop);
            return;
        }

        let body;
        try {
            body = JSON.stringify({ page, id, prop, value });
        } catch (e) {
            err('debouncedSave: could not JSON.stringify value for', id, prop, '-', e);
            return;
        }

        setStatus('saving', 'pb-status-saving');

        fetch(PB_CONFIG.saveUrl, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body,
        })
            .then(function (res) { return parseJsonResponse(res); })
            .then(function (data) {
                if (data && data.ok) {
                    setStatus('saved', 'pb-status-ok');
                    log('saved', id, prop, '=', value);
                } else {
                    const msg = (data && data.error) ? data.error : 'unknown error';
                    setStatus('save error', 'pb-status-error');
                    err('save rejected by server:', msg, '| element:', id, '| prop:', prop);
                }
            })
            .catch(function (e) {
                setStatus('network error', 'pb-status-error');
                err('save fetch failed for', id, prop, '-', e);
            });
    }, 350);

    // ------------------------------------------------------------------ //
    //  Apply live DOM change + schedule persist
    // ------------------------------------------------------------------ //
    function applyChange(el, prop, value) {
        try {
            if (prop === 'text') {
                if (el.childElementCount > 0) {
                    warn('applyChange: setting textContent on an element that has child elements will destroy them.',
                         'Element:', el.dataset.pbId, '| child count:', el.childElementCount);
                }
                el.textContent = value;
            } else if (ATTR_PROPS.has(prop)) {
                // HTML attribute - not a CSS property
                el.setAttribute(prop, value);
                log('applyChange: setAttribute', prop, '=', value, 'on', el.dataset.pbId);
            } else {
                const cssProp = CSS_MAP[prop];
                if (!cssProp) {
                    err('applyChange: unknown prop "' + prop + '". Add it to CSS_MAP to support it. Element:', el.dataset.pbId);
                    return;
                }
                let v = value;
                // Auto-append px for dimensioned props when a bare number is given
                const pxProps = ['padding', 'paddingTop', 'paddingBottom', 'fontSize',
                                 'letterSpacing', 'borderRadius', 'width', 'height'];
                if (pxProps.includes(prop) && v !== '' && v !== '0' && !isNaN(Number(v))) {
                    v = v + 'px';
                }
                el.style[cssProp] = v;
            }
        } catch (e) {
            err('applyChange failed for element', el.dataset && el.dataset.pbId, 'prop', prop, '-', e);
            return;
        }

        debouncedSave(PB_CONFIG.page, el.dataset.pbId, prop, value);
    }

    // ------------------------------------------------------------------ //
    //  Build editable fields from data-pb-props list
    // ------------------------------------------------------------------ //
    function buildFields(el, props) {
        const p    = getPanel();
        const body = p && p.querySelector('.pb-panel-body');
        if (!body) { err('buildFields: .pb-panel-body not found'); return; }

        body.innerHTML = '';

        props.forEach(function (prop, i) {
            try {
                const field = document.createElement('div');
                field.className = 'pb-field';

                const lbl = document.createElement('label');
                lbl.textContent = prop;
                field.appendChild(lbl);

                // ---- text / textarea ----
                if (prop === 'text') {
                    const ta = document.createElement('textarea');
                    ta.rows  = 3;
                    ta.value = el.textContent.trim();
                    ta.addEventListener('input', function () { applyChange(el, 'text', ta.value); });
                    field.appendChild(ta);

                // ---- src: URL text input + file upload ----
                } else if (prop === 'src') {
                    var srcInp = document.createElement('input');
                    srcInp.type        = 'text';
                    srcInp.value       = el.getAttribute('src') || '';
                    srcInp.placeholder = '/path/or/https://...';
                    srcInp.style.marginBottom = '6px';
                    srcInp.addEventListener('input', function () {
                        applyChange(el, 'src', srcInp.value);
                    });
                    field.appendChild(srcInp);

                    // Upload button row
                    var uploadRow = document.createElement('div');
                    uploadRow.style.cssText = 'display:flex;gap:6px;align-items:center;';

                    var fileInput = document.createElement('input');
                    fileInput.type   = 'file';
                    fileInput.accept = 'image/*';
                    fileInput.style.cssText = 'flex:1;font-size:10px;color:#8888a0;background:#13131f;border:1px solid rgba(255,255,255,0.08);padding:4px;';

                    var uploadBtn = document.createElement('button');
                    uploadBtn.textContent = 'upload';
                    uploadBtn.style.cssText = 'background:rgba(99,102,241,0.15);border:1px solid rgba(99,102,241,0.3);color:#a5a5c0;font-family:inherit;font-size:10px;padding:4px 10px;cursor:pointer;';

                    uploadBtn.addEventListener('click', function () {
                        if (!fileInput.files || !fileInput.files[0]) {
                            warn('buildFields [src]: no file selected for upload');
                            setStatus('select a file first', 'pb-status-error');
                            return;
                        }
                        var formData = new FormData();
                        formData.append('file', fileInput.files[0]);
                        formData.append('page', PB_CONFIG.page);

                        setStatus('uploading', 'pb-status-saving');
                        log('uploading file:', fileInput.files[0].name);

                        fetch('/page-builder/upload.php', { method: 'POST', body: formData })
                            .then(function (res) { return parseJsonResponse(res); })
                            .then(function (data) {
                                if (data && data.ok) {
                                    log('upload ok:', data.url);
                                    srcInp.value = data.url;
                                    applyChange(el, 'src', data.url);
                                } else {
                                    var msg = (data && data.error) ? data.error : 'unknown upload error';
                                    setStatus('upload failed', 'pb-status-error');
                                    err('upload failed:', msg);
                                }
                            })
                            .catch(function (e) {
                                setStatus('upload network error', 'pb-status-error');
                                err('upload fetch failed:', e);
                            });
                    });

                    uploadRow.appendChild(fileInput);
                    uploadRow.appendChild(uploadBtn);
                    field.appendChild(uploadRow);

                // ---- HTML attribute: plain text input (alt, href, title, etc.) ----
                } else if (ATTR_PROPS.has(prop)) {
                    var attrInp = document.createElement('input');
                    attrInp.type        = 'text';
                    attrInp.value       = el.getAttribute(prop) || '';
                    attrInp.placeholder = prop;
                    attrInp.addEventListener('input', function () { applyChange(el, prop, attrInp.value); });
                    field.appendChild(attrInp);

                // ---- color swatch + hex input ----
                } else if (COLOR_PROPS.has(prop)) {
                    const cssProp = CSS_MAP[prop];
                    let computed  = '';
                    try { computed = getComputedStyle(el)[cssProp] || ''; }
                    catch (ce) { warn('buildFields: getComputedStyle failed for', prop, '-', ce); }
                    const hexVal = rgbToHex(computed);

                    const row = document.createElement('div');
                    row.className = 'pb-color-row';

                    const swatch = document.createElement('input');
                    swatch.type  = 'color';
                    swatch.value = hexVal;

                    const hexInp = document.createElement('input');
                    hexInp.type        = 'text';
                    hexInp.value       = hexVal;
                    hexInp.placeholder = '#rrggbb';

                    swatch.addEventListener('input', function () {
                        hexInp.value = swatch.value;
                        applyChange(el, prop, swatch.value);
                    });
                    hexInp.addEventListener('input', function () {
                        if (/^#[0-9a-fA-F]{6}$/.test(hexInp.value)) {
                            swatch.value = hexInp.value;
                            applyChange(el, prop, hexInp.value);
                        }
                    });
                    row.appendChild(swatch);
                    row.appendChild(hexInp);
                    field.appendChild(row);

                // ---- range slider ----
                } else if (RANGE_PROPS[prop]) {
                    const cfg    = RANGE_PROPS[prop];
                    const cssProp = CSS_MAP[prop];
                    let curVal   = cfg.min;
                    try { curVal = parseFloat(getComputedStyle(el)[cssProp]) || cfg.min; }
                    catch (ce) { warn('buildFields: getComputedStyle failed for range prop', prop, '-', ce); }

                    const row = document.createElement('div');
                    row.className = 'pb-range-row';

                    const range = document.createElement('input');
                    range.type  = 'range';
                    range.min   = String(cfg.min);
                    range.max   = String(cfg.max);
                    range.step  = String(cfg.step);
                    range.value = String(curVal);

                    const display = document.createElement('span');
                    display.className   = 'pb-range-value';
                    display.textContent = curVal + cfg.unit;

                    range.addEventListener('input', function () {
                        display.textContent = range.value + cfg.unit;
                        applyChange(el, prop, range.value);
                    });
                    row.appendChild(range);
                    row.appendChild(display);
                    field.appendChild(row);

                // ---- generic text / number input ----
                } else {
                    if (!CSS_MAP[prop]) {
                        warn('buildFields: prop "' + prop + '" has no entry in CSS_MAP and is not a special type. Rendering as raw text input.');
                    }
                    const cssProp = CSS_MAP[prop] || prop;
                    let curVal    = '';
                    try {
                        curVal = el.style[cssProp] || getComputedStyle(el)[cssProp] || '';
                        if (/^\d+(\.\d+)?px$/.test(curVal)) curVal = String(parseFloat(curVal));
                    } catch (ce) {
                        warn('buildFields: could not read current value for prop', prop, '-', ce);
                    }

                    const inp     = document.createElement('input');
                    inp.type      = 'text';
                    inp.value     = curVal;
                    inp.placeholder = 'value';
                    inp.addEventListener('input', function () { applyChange(el, prop, inp.value); });
                    field.appendChild(inp);
                }

                body.appendChild(field);

                // Divider between fields (not after the last one)
                if (i < props.length - 1) {
                    const hr = document.createElement('hr');
                    hr.className = 'pb-divider';
                    body.appendChild(hr);
                }

            } catch (fieldErr) {
                err('buildFields: failed to build field for prop "' + prop + '":', fieldErr);
            }
        });
    }

    // ------------------------------------------------------------------ //
    //  Open the editor panel at cursor position
    // ------------------------------------------------------------------ //
    function openPanel(el, x, y) {
        try {
            const p = getPanel();
            if (!p) { err('openPanel: panel could not be created'); return; }

            // Deselect previously active element
            if (activeEl && activeEl !== el) activeEl.classList.remove('pb-selected');
            activeEl = el;
            el.classList.add('pb-selected');

            // Update header label
            const idSpan = p.querySelector('.pb-panel-id');
            if (idSpan) idSpan.textContent = '#' + (el.dataset.pbId || '?');

            // Parse editable props from attribute
            const propsRaw = (el.dataset.pbProps || '')
                .split(',').map(function (s) { return s.trim(); }).filter(Boolean);

            if (!propsRaw.length) {
                warn('openPanel: element has data-pb-id but no data-pb-props - nothing to edit.', el);
                return;
            }

            buildFields(el, propsRaw);
            setStatus('ready', 'pb-status-ok');

            // Show panel before reading its rendered size for positioning
            p.style.left = '';
            p.style.top  = '';
            p.classList.add('pb-panel-open');

            // Clamp to viewport
            const pw = p.offsetWidth  || 280;
            const ph = p.offsetHeight || 300;
            const vw = window.innerWidth;
            const vh = window.innerHeight;
            const left = Math.max(8, Math.min(x + 8, vw - pw - 8));
            const top  = Math.max(8, Math.min(y + 8, vh - ph - 8));

            p.style.left = left + 'px';
            p.style.top  = top  + 'px';

        } catch (e) {
            err('openPanel failed:', e);
        }
    }

    // ------------------------------------------------------------------ //
    //  Event listeners
    // ------------------------------------------------------------------ //

    // Intercept right-click on any editable element
    document.addEventListener('contextmenu', function (e) {
        try {
            const target = e.target.closest('[data-pb-id]');
            if (!target) return;
            e.preventDefault();
            openPanel(target, e.clientX, e.clientY);
        } catch (e2) {
            err('contextmenu handler failed:', e2);
        }
    });

    // Close when clicking outside the panel and outside editable elements
    document.addEventListener('mousedown', function (e) {
        try {
            if (!panel || !panel.classList.contains('pb-panel-open')) return;
            if (panel.contains(e.target)) return;
            if (e.target.closest('[data-pb-id]')) return;
            closePanel();
        } catch (e2) {
            err('mousedown handler failed:', e2);
        }
    });

    // Close on Escape
    document.addEventListener('keydown', function (e) {
        try {
            if (e.key === 'Escape') closePanel();
        } catch (e2) {
            err('keydown handler failed:', e2);
        }
    });

    // ------------------------------------------------------------------ //
    //  Public API  (called by toolbar buttons injected by watcher.php)
    // ------------------------------------------------------------------ //
    window.PBEditor = {
        rebuild: function () {
            if (!PB_CONFIG.page) { err('PBEditor.rebuild: PB_CONFIG.page is not set'); return; }
            setStatus('building', 'pb-status-saving');
            log('rebuild requested for page:', PB_CONFIG.page);

            fetch('/page-builder/build.php?page=' + encodeURIComponent(PB_CONFIG.page))
                .then(function (res) { return parseJsonResponse(res); })
                .then(function (data) {
                    if (data && data.ok) {
                        setStatus('rebuilt - reloading', 'pb-status-ok');
                        log('rebuild ok:', data);
                        setTimeout(function () { window.location.reload(); }, 600);
                    } else {
                        const msg = (data && data.error) ? data.error : 'unknown error';
                        setStatus('build error', 'pb-status-error');
                        err('rebuild failed:', msg);
                    }
                })
                .catch(function (e) {
                    setStatus('build failed', 'pb-status-error');
                    err('rebuild fetch failed:', e);
                });
        },

        reset: function () {
            if (!PB_CONFIG.page) { err('PBEditor.reset: PB_CONFIG.page is not set'); return; }
            if (!confirm('Reset all overrides for "' + PB_CONFIG.page + '"? This cannot be undone.')) return;
            window.location.href = '?page=' + encodeURIComponent(PB_CONFIG.page) + '&reset=1';
        },

        close: closePanel,
    };

    log('init | page:', PB_CONFIG.page, '| saveUrl:', PB_CONFIG.saveUrl || '(not set)');
    log('right-click any outlined element to open the editor panel.');

}());
