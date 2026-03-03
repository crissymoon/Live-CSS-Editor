/**
 * pb-composer.js
 * Client-side logic for the page composer (composer.php).
 * No framework dependencies - vanilla JS only.
 *
 * Global state:
 *   PBC.page        - page name (string, from URL param)
 *   PBC.manifest    - page manifest { title, sections }
 *   PBC.library     - array of section templates from section-library.php
 *   PBC.selectedId  - id of the section currently open in the JSON panel
 *   PBC.dirty       - true if the JSON editor has unsaved changes
 *   PBC.dragging    - drag state
 */

(function () {
    'use strict';

    /* ========== State ====================================================== */

    var PBC = window.PBC = {
        page:       '',
        manifest:   { title: '', sections: [] },
        library:    [],
        selectedId: null,
        dirty:      false,
        dragging:   null,
        _libFilter: 'all',
        _libSearch: '',
    };

    /* ========== DOM refs =================================================== */

    function el(id)  { return document.getElementById(id); }
    function qs(sel) { return document.querySelector(sel); }

    /* ========== Utilities ================================================== */

    function escHtml(s) {
        return String(s).replace(/[&<>"']/g, function (c) {
            return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
        });
    }

    function showStatus(msg, type) {
        var s = el('pbc-status');
        if (!s) { console.error('[pb-composer] pbc-status element not found'); return; }
        s.textContent = msg;
        s.style.color = type === 'err'  ? 'var(--pb-err)'  :
                        type === 'ok'   ? 'var(--pb-ok)'   :
                        type === 'warn' ? 'var(--pb-warn)'  :
                        'var(--pb-text-faint)';
        console.log('[pb-composer] status:', msg);
    }

    function showMsg(msg, type) {
        var m = el('pbc-msg');
        if (!m) { console.error('[pb-composer] pbc-msg element not found'); return; }
        m.textContent = msg;
        m.className   = 'pb-msg show ' + (type || 'ok');
        clearTimeout(m._t);
        m._t = setTimeout(function () { m.className = 'pb-msg'; }, 4500);
    }

    function typeBadge(type) {
        var lbl = type || 'section';
        return '<span class="pb-badge t-' + escHtml(lbl) + '">' + escHtml(lbl) + '</span>';
    }

    /* ========== API helpers ================================================ */

    function apiFetch(url, cb) {
        console.log('[pb-composer] GET', url);
        fetch(url)
            .then(function (r) { return r.json(); })
            .then(function (d) { cb(null, d); })
            .catch(function (e) {
                console.error('[pb-composer] GET error', url, e);
                cb(e, null);
            });
    }

    function apiPost(url, body, cb) {
        console.log('[pb-composer] POST', url, body);
        fetch(url, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(body),
        })
            .then(function (r) { return r.json(); })
            .then(function (d) { cb(null, d); })
            .catch(function (e) {
                console.error('[pb-composer] POST error', url, e);
                cb(e, null);
            });
    }

    /* ========== Load manifest ============================================== */

    function loadManifest(cb) {
        showStatus('loading page...');
        apiFetch('section-api.php?action=get_page&page=' + encodeURIComponent(PBC.page), function (e, d) {
            if (e) {
                showStatus('network error', 'err');
                console.error('[pb-composer] loadManifest error:', e);
                if (cb) cb(e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showStatus(msg, 'err');
                console.error('[pb-composer] loadManifest api error:', d);
                if (cb) cb(new Error(msg));
                return;
            }
            PBC.manifest = d.data;
            console.log('[pb-composer] manifest loaded:', PBC.manifest);
            showStatus('page: ' + PBC.manifest.sections.length + ' sections');
            renderCanvas();
            if (cb) cb(null);
        });
    }

    /* ========== Load library =============================================== */

    function loadLibrary(cb) {
        apiFetch('section-library.php?action=list', function (e, d) {
            if (e) {
                console.error('[pb-composer] loadLibrary error:', e);
                if (cb) cb(e);
                return;
            }
            if (!d || !d.ok) {
                console.error('[pb-composer] loadLibrary api error:', d);
                if (cb) cb(new Error((d && d.error) || 'unknown'));
                return;
            }
            PBC.library = d.data || [];
            console.log('[pb-composer] library loaded:', PBC.library.length, 'templates');
            renderLibrary();
            if (cb) cb(null);
        });
    }

    /* ========== Render library panel ======================================= */

    function renderLibrary() {
        var scroll = el('pbc-lib-scroll');
        if (!scroll) { console.error('[pb-composer] pbc-lib-scroll not found'); return; }

        var filter = PBC._libFilter;
        var search = PBC._libSearch.toLowerCase();

        var items = PBC.library.filter(function (t) {
            if (filter !== 'all' && t.type !== filter) return false;
            if (search && t.name.toLowerCase().indexOf(search) === -1 &&
                t.description.toLowerCase().indexOf(search) === -1) return false;
            return true;
        });

        if (items.length === 0) {
            scroll.innerHTML = '<span style="color:var(--pb-text-faint);font-size:11px;padding:10px 4px;">No templates match.</span>';
            return;
        }

        var html = '';
        items.forEach(function (t) {
            html += '<div class="pbc-tpl-card" data-tpl-path="' + escHtml(t.path) + '" data-tpl-name="' + escHtml(t.name) + '">'
                + '<div class="pbc-tpl-card-meta">' + typeBadge(t.type) + '</div>'
                + '<div class="pbc-tpl-card-name">' + escHtml(t.name) + '</div>'
                + (t.description ? '<div class="pbc-tpl-card-desc">' + escHtml(t.description) + '</div>' : '')
                + '<button class="pb-btn pbc-tpl-add-btn" '
                + 'onclick="PBC.addSectionFromTemplate(\'' + escHtml(t.path) + '\',\'' + escHtml(t.name) + '\')">'
                + '+ add</button>'
                + '</div>';
        });
        scroll.innerHTML = html;
    }

    /* ========== Render canvas ============================================== */

    function renderCanvas() {
        var wrap = el('pbc-canvas-inner');
        if (!wrap) { console.error('[pb-composer] pbc-canvas-inner not found'); return; }

        var sections = PBC.manifest.sections || [];

        if (sections.length === 0) {
            wrap.innerHTML = '<p class="pbc-canvas-empty">No sections yet. Add from the library on the left.</p>';
            return;
        }

        var html = '';
        sections.forEach(function (s, idx) {
            var isSelected = (s.id === PBC.selectedId);
            var rowCls = 'pbc-sec-row' + (isSelected ? ' selected' : '');
            var editActive = isSelected ? ' active' : '';

            // Background colour chip - shows section's settings.bg, click to edit 
            var bgColor = s.settings_bg || '';
            var chipStyle = bgColor
                ? 'background:' + escHtml(bgColor) + ';'
                : 'background:repeating-linear-gradient(45deg,#2a2a3a 0 3px,#1a1a2a 3px 6px);';
            var bgTitle = bgColor ? 'bg: ' + bgColor : 'no background set — click to add';
            var bgChip = '<button class="pbc-sec-bg-chip" style="' + chipStyle + '"'
                + ' onclick="PBC.openBgEdit(\'' + escHtml(s.id) + '\',this)"'
                + ' title="' + escHtml(bgTitle) + '">'
                + '</button>';

            html += '<div class="' + rowCls + '" id="row-' + escHtml(s.id) + '" draggable="true" data-sec-id="' + escHtml(s.id) + '">'
                + '<div class="pbc-sec-handle" title="Drag to reorder">&#8597;</div>'
                + '<div class="pbc-sec-info">'
                +   '<span class="pbc-sec-label">' + escHtml(s.label || s.type) + '</span>'
                +   typeBadge(s.type)
                +   '<span class="pbc-sec-file">' + escHtml(s.file) + '</span>'
                + '</div>'
                + '<div class="pbc-sec-controls">' + bgChip + '</div>'
                + '<div class="pbc-sec-actions">'
                +   '<button class="pbc-sec-btn' + editActive + '" onclick="PBC.selectSection(\'' + escHtml(s.id) + '\')" title="Edit JSON">edit JSON</button>'
                +   '<button class="pbc-sec-btn" onclick="PBC.renameSection(\'' + escHtml(s.id) + '\')" title="Rename">rename</button>'
                +   '<button class="pbc-sec-btn danger" onclick="PBC.removeSection(\'' + escHtml(s.id) + '\')" title="Remove">remove</button>'
                + '</div>'
                + '</div>';
        });

        wrap.innerHTML = html;
        bindDragDrop();
    }

    /* ========== Section background quick-edit ============================== */

    // _bgPop: currently open bg popover element, so we can close it
    PBC._bgPop = null;

    PBC.openBgEdit = function (sectionId, chipEl) {
        // Close any already-open popover
        if (PBC._bgPop) {
            PBC._bgPop.remove();
            PBC._bgPop = null;
        }

        var section = PBC.manifest.sections.find(function (s) { return s.id === sectionId; });
        if (!section) {
            console.error('[pb-composer] openBgEdit: section not found', sectionId);
            return;
        }

        var currentBg = section.settings_bg || '#0a0a14';

        // Build popover
        var pop = document.createElement('div');
        pop.className = 'pbc-bg-pop';
        pop.innerHTML =
            '<div class="pbc-bg-pop-row">'
          +   '<input type="color" id="pbc-bg-swatch" value="' + escHtml(currentBg) + '">'
          +   '<input type="text"  id="pbc-bg-hex"    value="' + escHtml(currentBg) + '" placeholder="#rrggbb" maxlength="7">'
          +   '<button class="pbc-sec-btn" id="pbc-bg-save"  title="Apply">apply</button>'
          +   '<button class="pbc-sec-btn" id="pbc-bg-close" title="Cancel">✕</button>'
          + '</div>';

        // Position below the chip (position:fixed uses viewport coords)
        document.body.appendChild(pop);
        var chipRect = chipEl.getBoundingClientRect();
        var popW = 200;
        var left = Math.min(chipRect.left, window.innerWidth - popW - 8);
        pop.style.top  = (chipRect.bottom + 4) + 'px';
        pop.style.left = Math.max(8, left) + 'px';

        var swatch = pop.querySelector('#pbc-bg-swatch');
        var hexInp = pop.querySelector('#pbc-bg-hex');
        var saveBtn = pop.querySelector('#pbc-bg-save');
        var closeBtn = pop.querySelector('#pbc-bg-close');

        // Sync swatch ↔ hex
        swatch.addEventListener('input', function () {
            hexInp.value = swatch.value;
        });
        hexInp.addEventListener('input', function () {
            if (/^#[0-9a-fA-F]{6}$/.test(hexInp.value)) {
                swatch.value = hexInp.value;
            }
        });

        saveBtn.addEventListener('click', function () {
            var color = /^#[0-9a-fA-F]{6}$/.test(hexInp.value) ? hexInp.value : swatch.value;
            PBC.saveBgSetting(sectionId, color, chipEl);
            pop.remove();
            PBC._bgPop = null;
        });

        closeBtn.addEventListener('click', function () {
            pop.remove();
            PBC._bgPop = null;
        });

        // Close on outside click
        function onOutside(e) {
            if (!pop.contains(e.target) && e.target !== chipEl) {
                pop.remove();
                PBC._bgPop = null;
                document.removeEventListener('mousedown', onOutside);
            }
        }
        setTimeout(function () { document.addEventListener('mousedown', onOutside); }, 0);

        PBC._bgPop = pop;
        swatch.focus();
    };

    PBC.saveBgSetting = function (sectionId, color, chipEl) {
        console.log('[pb-composer] saveBgSetting id=' + sectionId + ' color=' + color);
        showStatus('saving bg...');

        apiPost(
            'section-api.php?action=patch_section_setting&page=' + encodeURIComponent(PBC.page),
            { id: sectionId, setting: 'bg', value: color },
            function (e, d) {
                if (e || !d || !d.ok) {
                    var msg = (d && d.error) ? d.error : (e ? e.message : 'unknown error');
                    showStatus('bg save failed', 'err');
                    showMsg('Background save failed: ' + msg, 'err');
                    console.error('[pb-composer] saveBgSetting error:', msg);
                    return;
                }
                // Update in-memory manifest
                var section = PBC.manifest.sections.find(function (s) { return s.id === sectionId; });
                if (section) section.settings_bg = color;
                // Update chip colour live
                if (chipEl) {
                    chipEl.style.background = color;
                    chipEl.title = 'bg: ' + color;
                }
                showStatus('bg saved', 'ok');
                showMsg('Background updated — rebuild to apply', 'ok');
            }
        );
    };

    /* ========== Drag and drop ============================================== */

    function bindDragDrop() {
        var rows = document.querySelectorAll('.pbc-sec-row');
        rows.forEach(function (row) {
            row.addEventListener('dragstart', onDragStart);
            row.addEventListener('dragend',   onDragEnd);
            row.addEventListener('dragover',  onDragOver);
            row.addEventListener('drop',      onDrop);
        });
    }

    function onDragStart(e) {
        var id = this.dataset.secId;
        PBC.dragging = id;
        this.classList.add('dragging');
        if (e.dataTransfer) {
            e.dataTransfer.effectAllowed = 'move';
            e.dataTransfer.setData('text/plain', id);
        }
        console.log('[pb-composer] dragstart id=' + id);
    }

    function onDragEnd() {
        this.classList.remove('dragging');
        document.querySelectorAll('.pbc-sec-row').forEach(function (r) {
            r.classList.remove('drag-over-above', 'drag-over-below');
        });
        PBC.dragging = null;
    }

    function onDragOver(e) {
        e.preventDefault();
        if (!PBC.dragging || PBC.dragging === this.dataset.secId) return;
        var rect = this.getBoundingClientRect();
        var mid  = rect.top + rect.height / 2;
        document.querySelectorAll('.pbc-sec-row').forEach(function (r) {
            r.classList.remove('drag-over-above', 'drag-over-below');
        });
        if (e.clientY < mid) {
            this.classList.add('drag-over-above');
        } else {
            this.classList.add('drag-over-below');
        }
        if (e.dataTransfer) e.dataTransfer.dropEffect = 'move';
    }

    function onDrop(e) {
        e.preventDefault();
        var targetId = this.dataset.secId;
        var sourceId = PBC.dragging;
        if (!sourceId || sourceId === targetId) return;

        console.log('[pb-composer] drop src=' + sourceId + ' tgt=' + targetId);

        var sections = PBC.manifest.sections.slice();
        var srcIdx   = sections.findIndex(function (s) { return s.id === sourceId; });
        var tgtIdx   = sections.findIndex(function (s) { return s.id === targetId; });
        if (srcIdx === -1 || tgtIdx === -1) return;

        var item = sections.splice(srcIdx, 1)[0];

        // Determine above/below
        var rect = this.getBoundingClientRect();
        var mid  = rect.top + rect.height / 2;
        var newIdx = tgtIdx;
        if (srcIdx < tgtIdx) newIdx = tgtIdx;
        else newIdx = tgtIdx;
        if (e.clientY >= mid) newIdx = newIdx + 1;
        if (newIdx > sections.length) newIdx = sections.length;

        sections.splice(newIdx, 0, item);
        PBC.manifest.sections = sections;
        renderCanvas();
        PBC.saveOrder();
    }

    /* ========== JSON panel ================================================= */

    PBC.selectSection = function (id) {
        if (PBC.dirty && PBC.selectedId && PBC.selectedId !== id) {
            if (!confirm('You have unsaved changes in the JSON editor. Discard them?')) {
                return;
            }
        }

        PBC.selectedId = id;
        PBC.dirty = false;

        var section = PBC.manifest.sections.find(function (s) { return s.id === id; });
        if (!section) {
            console.error('[pb-composer] selectSection: id not found:', id);
            return;
        }

        // Highlight row
        renderCanvas();

        // Load file content
        showStatus('loading JSON...');
        apiFetch('section-api.php?action=get_page&page=' + encodeURIComponent(PBC.page), function (e, d) {
            // We need the raw section file - fetch it via section-library action=get on the page file
            // Instead, load it directly via a dedicated endpoint
        });

        // Directly load the section's JSON file
        apiFetch('section-api.php?action=get_section_file&page=' + encodeURIComponent(PBC.page) + '&id=' + encodeURIComponent(id), function (e, d) {
            if (e || !d || !d.ok) {
                // Fallback: show placeholder with file name
                console.warn('[pb-composer] get_section_file not available, showing file info only');
                renderJsonPanel(section, null);
                return;
            }
            renderJsonPanel(section, d.data);
        });

        // Show JSON panel immediately even before data loads
        renderJsonPanel(section, null);
        showStatus('section: ' + section.label);
    };

    function renderJsonPanel(section, content) {
        var panel = el('pbc-json-panel');
        if (!panel) { console.error('[pb-composer] pbc-json-panel not found'); return; }
        panel.classList.remove('collapsed');

        var titleEl = el('pbc-json-title');
        var infoEl  = el('pbc-json-info');
        var ta      = el('pbc-json-textarea');

        if (titleEl) titleEl.textContent = 'json editor';
        if (infoEl) {
            infoEl.innerHTML = 'section: <span>' + escHtml(section.label) + '</span>'
                + ' | file: <span>' + escHtml(section.file) + '</span>'
                + ' | type: <span>' + escHtml(section.type) + '</span>';
        }

        if (!ta) { console.error('[pb-composer] pbc-json-textarea not found'); return; }

        var placeholder = el('pbc-json-placeholder');
        if (placeholder) placeholder.style.display = 'none';

        if (content !== null) {
            ta.value = JSON.stringify(content, null, 2);
        } else {
            ta.value = '// loading...';
        }
        ta.style.display = 'flex';
    }

    PBC.closeJsonPanel = function () {
        if (PBC.dirty) {
            if (!confirm('Discard unsaved JSON changes?')) return;
        }
        PBC.selectedId = null;
        PBC.dirty = false;
        var panel = el('pbc-json-panel');
        if (panel) panel.classList.add('collapsed');
        renderCanvas();
    };

    PBC.saveJsonEdit = function () {
        var ta  = el('pbc-json-textarea');
        var err = el('pbc-json-err');
        if (!ta) { console.error('[pb-composer] pbc-json-textarea not found'); return; }

        var raw = ta.value;
        var parsed;
        try {
            parsed = JSON.parse(raw);
        } catch (e) {
            if (err) { err.textContent = 'Invalid JSON: ' + e.message; err.className = 'pbc-json-err show'; }
            console.error('[pb-composer] JSON parse error:', e);
            return;
        }

        if (err) err.className = 'pbc-json-err';

        if (!PBC.selectedId) { console.error('[pb-composer] no section selected'); return; }

        var saveBtn = el('pbc-json-save-btn');
        if (saveBtn) saveBtn.disabled = true;

        showStatus('saving...');
        apiPost('section-api.php?action=update_section_json&page=' + encodeURIComponent(PBC.page), {
            id:      PBC.selectedId,
            content: parsed,
        }, function (e, d) {
            if (saveBtn) saveBtn.disabled = false;
            if (e) {
                showStatus('save failed', 'err');
                showMsg('Network error: ' + e.message, 'err');
                console.error('[pb-composer] saveJsonEdit network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showStatus(msg, 'err');
                showMsg('Save failed: ' + msg, 'err');
                console.error('[pb-composer] saveJsonEdit api error:', d);
                return;
            }
            PBC.dirty = false;
            showStatus('saved', 'ok');
            showMsg('JSON saved for "' + (PBC.manifest.sections.find(function(s){return s.id===PBC.selectedId;})||{label:'section'}).label + '"', 'ok');
            console.log('[pb-composer] JSON saved for id=' + PBC.selectedId);
        });
    };

    /* ========== Add section from library =================================== */

    PBC.addSectionFromTemplate = function (templatePath, label) {
        console.log('[pb-composer] addSectionFromTemplate path=' + templatePath + ' label=' + label);
        showStatus('adding section...');

        apiPost('section-api.php?action=add_section&page=' + encodeURIComponent(PBC.page), {
            template: templatePath,
            label:    label,
        }, function (e, d) {
            if (e) {
                showStatus('network error', 'err');
                showMsg('Network error: ' + e.message, 'err');
                console.error('[pb-composer] addSection network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showStatus(msg, 'err');
                showMsg('Add section failed: ' + msg, 'err');
                console.error('[pb-composer] addSection api error:', d);
                return;
            }
            PBC.manifest.sections = d.sections;
            showStatus('section added', 'ok');
            showMsg('"' + label + '" added', 'ok');
            renderCanvas();
            console.log('[pb-composer] section added:', d.section);
        });
    };

    /* ========== Remove section ============================================= */

    PBC.removeSection = function (id) {
        var section = PBC.manifest.sections.find(function (s) { return s.id === id; });
        if (!section) { console.error('[pb-composer] removeSection: id not found:', id); return; }

        if (!confirm('Remove "' + section.label + '" from this page?')) return;

        console.log('[pb-composer] removeSection id=' + id);

        apiPost('section-api.php?action=remove_section&page=' + encodeURIComponent(PBC.page), {
            id: id,
        }, function (e, d) {
            if (e) {
                showMsg('Network error: ' + e.message, 'err');
                console.error('[pb-composer] removeSection network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showMsg('Remove failed: ' + msg, 'err');
                console.error('[pb-composer] removeSection api error:', d);
                return;
            }
            PBC.manifest.sections = d.sections;
            if (PBC.selectedId === id) {
                PBC.selectedId = null;
                var panel = el('pbc-json-panel');
                if (panel) panel.classList.add('collapsed');
            }
            showMsg('"' + section.label + '" removed', 'ok');
            renderCanvas();
            console.log('[pb-composer] removed section:', d.removed);
        });
    };

    /* ========== Rename section ============================================= */

    PBC.renameSection = function (id) {
        var section = PBC.manifest.sections.find(function (s) { return s.id === id; });
        if (!section) { console.error('[pb-composer] renameSection: id not found:', id); return; }

        var newLabel = prompt('New label for "' + section.label + '":', section.label);
        if (!newLabel || newLabel.trim() === section.label) return;
        newLabel = newLabel.trim();

        console.log('[pb-composer] renameSection id=' + id + ' newLabel=' + newLabel);

        apiPost('section-api.php?action=rename_section&page=' + encodeURIComponent(PBC.page), {
            id:    id,
            label: newLabel,
        }, function (e, d) {
            if (e) {
                showMsg('Network error: ' + e.message, 'err');
                console.error('[pb-composer] renameSection network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showMsg('Rename failed: ' + msg, 'err');
                console.error('[pb-composer] renameSection api error:', d);
                return;
            }
            PBC.manifest.sections = d.sections;
            showMsg('Renamed to "' + newLabel + '"', 'ok');
            renderCanvas();
        });
    };

    /* ========== Save order ================================================= */

    PBC.saveOrder = function () {
        console.log('[pb-composer] saving section order');
        showStatus('saving order...');

        apiPost('section-api.php?action=reorder&page=' + encodeURIComponent(PBC.page), {
            sections: PBC.manifest.sections,
        }, function (e, d) {
            if (e) {
                showStatus('save failed', 'err');
                showMsg('Could not save order: ' + e.message, 'err');
                console.error('[pb-composer] saveOrder network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showStatus(msg, 'err');
                showMsg('Order save failed: ' + msg, 'err');
                console.error('[pb-composer] saveOrder api error:', d);
                return;
            }
            showStatus('order saved', 'ok');
            console.log('[pb-composer] order saved');
        });
    };

    /* ========== Build page ================================================= */

    PBC.buildPage = function () {
        var btn = el('pbc-build-btn');
        if (btn) { btn.disabled = true; btn.textContent = 'building...'; }

        showStatus('building...');
        console.log('[pb-composer] building page:', PBC.page);

        apiFetch('build.php?page=' + encodeURIComponent(PBC.page), function (e, d) {
            if (btn) { btn.disabled = false; btn.textContent = 'build'; }
            if (e) {
                showStatus('build failed', 'err');
                showMsg('Build error: ' + e.message, 'err');
                console.error('[pb-composer] buildPage network error:', e);
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                showStatus(msg, 'err');
                showMsg('Build failed: ' + msg, 'err');
                console.error('[pb-composer] buildPage api error:', d);
                return;
            }
            showStatus('built (' + (d.bytes || 0) + ' bytes)', 'ok');
            showMsg('Page built successfully', 'ok');
            console.log('[pb-composer] build ok:', d);
        });
    };

    /* ========== Library filter / search ==================================== */

    PBC.setLibFilter = function (type) {
        PBC._libFilter = type;
        document.querySelectorAll('.pbc-lib-tab').forEach(function (t) {
            t.classList.toggle('active', t.dataset.filter === type);
        });
        renderLibrary();
    };

    PBC.libSearch = function (val) {
        PBC._libSearch = val || '';
        renderLibrary();
    };

    /* ========== Theme toggle =============================================== */

    PBC.toggleTheme = function () {
        var html    = document.documentElement;
        var current = html.getAttribute('data-theme') || 'dark';
        var next    = current === 'dark' ? 'light' : 'dark';
        html.setAttribute('data-theme', next);
        try { localStorage.setItem('pb_theme', next); } catch (e) { console.warn('[pb-composer] localStorage write failed:', e); }
        var btn = el('pbc-theme-btn');
        if (btn) btn.textContent = next === 'dark' ? 'light mode' : 'dark mode';
    };

    /* ========== Dirty tracking for JSON editor ============================= */

    function initJsonDirtyTracking() {
        var ta = el('pbc-json-textarea');
        if (!ta) { console.warn('[pb-composer] pbc-json-textarea not found for dirty tracking'); return; }
        ta.addEventListener('input', function () { PBC.dirty = true; });
    }

    /* ========== get_section_file action (needs server-side support) ======== */
    // We add a special action to section-api.php that returns a section file's
    // JSON content. Until that exists we load it via the manifest helper below.
    // The fallback reads the file directly by fetching it as a static URL.

    function loadSectionFileContent(page, section, cb) {
        // Try direct file fetch first (works when PHP serves this directory)
        var url = 'pages/' + encodeURIComponent(page) + '/' + encodeURIComponent(section.file);
        console.log('[pb-composer] loading section file:', url);
        fetch(url)
            .then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(function (d) { cb(null, d); })
            .catch(function (e) {
                console.error('[pb-composer] loadSectionFileContent error:', e);
                cb(e, null);
            });
    }

    // Override selectSection to use the correct file loader
    PBC.selectSection = function (id) {
        if (PBC.dirty && PBC.selectedId && PBC.selectedId !== id) {
            if (!confirm('You have unsaved changes in the JSON editor. Discard them?')) return;
        }
        PBC.selectedId = id;
        PBC.dirty = false;

        var section = PBC.manifest.sections.find(function (s) { return s.id === id; });
        if (!section) { console.error('[pb-composer] selectSection: id not found:', id); return; }

        renderCanvas();

        var panel = el('pbc-json-panel');
        if (panel) panel.classList.remove('collapsed');

        var titleEl = el('pbc-json-title');
        var infoEl  = el('pbc-json-info');
        var ta      = el('pbc-json-textarea');
        var ph      = el('pbc-json-placeholder');

        if (titleEl) titleEl.textContent = 'json editor';
        if (infoEl) {
            infoEl.innerHTML = 'section: <span>' + escHtml(section.label) + '</span>'
                + ' &nbsp;|&nbsp; file: <span>' + escHtml(section.file) + '</span>'
                + ' &nbsp;|&nbsp; type: <span>' + escHtml(section.type) + '</span>';
        }
        if (ph)  ph.style.display = 'none';
        if (ta) { ta.style.display = ''; ta.value = '// loading...'; }

        var labelInput = el('pbc-json-label-input');
        if (labelInput) labelInput.value = section.label;

        loadSectionFileContent(PBC.page, section, function (e, data) {
            if (!ta) return;
            if (e) {
                ta.value = '// Could not load file: ' + e.message + '\n// File: ' + section.file;
                showStatus('could not load JSON', 'warn');
                return;
            }
            ta.value = JSON.stringify(data, null, 2);
            showStatus('editing: ' + section.label);
        });
    };

    /* ========== Expose internal functions ================================= */

    PBC.renderCanvas  = renderCanvas;
    PBC.renderLibrary = renderLibrary;
    PBC.loadManifest  = loadManifest;

    /* ========== Init ======================================================= */

    function init() {
        // Read page name from URL
        var params = new URLSearchParams(window.location.search);
        PBC.page = params.get('page') || '';

        if (!PBC.page) {
            console.error('[pb-composer] No page parameter in URL');
            var wrap = el('pbc-canvas-inner');
            if (wrap) wrap.innerHTML = '<p class="pbc-canvas-empty" style="color:var(--pb-err)">No page specified. Add ?page=<name> to the URL.</p>';
            return;
        }

        // Update page title in topbar
        var pageLabel = el('pbc-page-label');
        if (pageLabel) pageLabel.textContent = PBC.page;

        // Theme
        var savedTheme = 'dark';
        try { savedTheme = localStorage.getItem('pb_theme') || 'dark'; } catch (e) { console.warn('[pb-composer] localStorage read failed:', e); }
        document.documentElement.setAttribute('data-theme', savedTheme);
        var themeBtn = el('pbc-theme-btn');
        if (themeBtn) themeBtn.textContent = savedTheme === 'dark' ? 'light mode' : 'dark mode';

        // Dirty tracking
        initJsonDirtyTracking();

        // Load data
        loadLibrary(function (libErr) {
            if (libErr) showStatus('library load error', 'warn');
        });

        loadManifest(function (mErr) {
            if (mErr) {
                showStatus('manifest load error', 'err');
                return;
            }
        });

        // Wire lib tabs
        document.querySelectorAll('.pbc-lib-tab').forEach(function (tab) {
            tab.addEventListener('click', function () {
                PBC.setLibFilter(tab.dataset.filter || 'all');
            });
        });

        // JSON textarea dirty
        var ta = el('pbc-json-textarea');
        if (ta) {
            ta.addEventListener('input', function () { PBC.dirty = true; });
        }

        console.log('[pb-composer] init complete, page=' + PBC.page);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();
