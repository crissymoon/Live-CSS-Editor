/**
 * gutter.js — Free-floating panel manager
 * Attached to window.LiveCSS.gutter
 *
 * Converts all .editor-panel and .preview-panel elements to
 * absolutely positioned floating panels that can be:
 *   - Dragged freely in 2D by their header
 *   - Resized from any edge or corner
 *   - Brought to front by clicking
 *
 * Call LiveCSS.gutter.init() after editor.init().
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.gutter = (function () {
    'use strict';

    var PANEL_IDS = ['jsPanel', 'htmlPanel', 'cssPanel', 'previewPanel'];
    var MIN_W     = 180;
    var MIN_H     = 100;
    var zCounter  = 10;
    var Z_OVERLAY = 100;   // drag-overlay z-index (must match CSS)
    var Z_ACTIVE  = 200;   // z-index of the panel being dragged/resized

    var overlay   = null;
    var layout    = null;

    // ── Helpers ──────────────────────────────────────────────────

    function getLayoutRect() { return layout.getBoundingClientRect(); }

    function refreshEditors() {
        setTimeout(function () {
            ['getJsEditor', 'getHtmlEditor', 'getCssEditor'].forEach(function (fn) {
                var ed = LiveCSS.editor[fn] && LiveCSS.editor[fn]();
                if (ed) { ed.refresh(); }
            });
        }, 30);
    }

    function bringToFront(panel) {
        // If counter is near the cap, renumber all panels to reclaim space
        if (zCounter >= Z_OVERLAY - 4) {
            // Gather visible panels sorted by their current z-index
            var ordered = [];
            PANEL_IDS.forEach(function (id) {
                var p = document.getElementById(id);
                if (p && p.style.display !== 'none') {
                    ordered.push({ el: p, z: parseInt(p.style.zIndex, 10) || 0 });
                }
            });
            ordered.sort(function (a, b) { return a.z - b.z; });
            for (var i = 0; i < ordered.length; i++) {
                ordered[i].el.style.zIndex = 10 + i;
            }
            zCounter = 10 + ordered.length;
        }

        zCounter++;
        panel.style.zIndex = zCounter;

        PANEL_IDS.forEach(function (id) {
            var p = document.getElementById(id);
            if (p) { p.classList.remove('is-top'); }
        });
        panel.classList.add('is-top');
        saveLayoutState();
    }

    function showOverlay(cur) {
        overlay.classList.add('active');
        overlay.style.cursor = cur || 'default';
        document.body.classList.add('is-dragging');
        document.body.style.cursor = cur || 'default';
        // Prevent the preview iframe from swallowing mouse events during drag/resize
        var iframe = document.getElementById('previewFrame');
        if (iframe) { iframe.style.pointerEvents = 'none'; }
    }

    function hideOverlay() {
        overlay.classList.remove('active');
        overlay.style.cursor = '';
        document.body.classList.remove('is-dragging', 'is-resizing');
        document.body.style.cursor = '';
        var iframe = document.getElementById('previewFrame');
        if (iframe) { iframe.style.pointerEvents = ''; }
    }

    // ── Default layout — four equal columns ──────────────────────

    function applyDefaultLayout() {
        var r = getLayoutRect();
        var w = r.width;
        var h = r.height;

        var cols  = PANEL_IDS.length;
        var colW  = Math.floor(w / cols);

        PANEL_IDS.forEach(function (id, i) {
            var panel = document.getElementById(id);
            if (!panel) { return; }
            panel.style.left   = (i * colW) + 'px';
            panel.style.top    = '0px';
            panel.style.width  = colW + 'px';
            panel.style.height = h + 'px';
            panel.style.zIndex = 10 + i;
        });

        zCounter = 10 + cols;
    }

    // ── Resize handles ────────────────────────────────────────────

    var DIRECTIONS = ['n', 's', 'e', 'w', 'ne', 'nw', 'se', 'sw'];

    function addResizeHandles(panel) {
        DIRECTIONS.forEach(function (dir) {
            var h = document.createElement('div');
            h.className  = 'panel-resize panel-resize-' + dir;
            h.dataset.dir = dir;
            panel.appendChild(h);
        });
    }

    // ── Drag ─────────────────────────────────────────────────────

    var drag = null;   // { panel, startX, startY, origLeft, origTop }

    function startDrag(e, panel) {
        e.preventDefault();
        drag = {
            panel    : panel,
            startX   : e.clientX,
            startY   : e.clientY,
            origLeft : panel.offsetLeft,
            origTop  : panel.offsetTop
        };
        panel.style.zIndex = Z_ACTIVE;
        showOverlay('grabbing');
    }

    function onDragMove(e) {
        if (!drag) { return; }
        var lr = getLayoutRect();
        var dx = e.clientX - drag.startX;
        var dy = e.clientY - drag.startY;

        var newLeft = drag.origLeft + dx;
        var newTop  = drag.origTop  + dy;

        // Keep dragged panel at least partially visible
        var pw = drag.panel.offsetWidth;
        var ph = drag.panel.offsetHeight;
        newLeft = Math.max(-pw + 40, Math.min(lr.width  - 40, newLeft));
        newTop  = Math.max(0,        Math.min(lr.height - 20, newTop));

        drag.panel.style.left = newLeft + 'px';
        drag.panel.style.top  = newTop  + 'px';
    }

    function stopDrag() {
        if (drag) { bringToFront(drag.panel); }
        drag = null;
        hideOverlay();
        refreshEditors();
        saveLayoutState();
    }

    // ── Resize ────────────────────────────────────────────────────

    var resize = null;  // { panel, dir, startX, startY, origLeft, origTop, origW, origH }

    function startResize(e, panel, dir) {
        e.preventDefault();
        e.stopPropagation();

        var cursorMap = {
            n: 'ns-resize', s: 'ns-resize', e: 'ew-resize', w: 'ew-resize',
            ne: 'nesw-resize', sw: 'nesw-resize', nw: 'nwse-resize', se: 'nwse-resize'
        };

        resize = {
            panel    : panel,
            dir      : dir,
            startX   : e.clientX,
            startY   : e.clientY,
            origLeft : panel.offsetLeft,
            origTop  : panel.offsetTop,
            origW    : panel.offsetWidth,
            origH    : panel.offsetHeight
        };
        panel.style.zIndex = Z_ACTIVE;
        document.body.classList.add('is-resizing');
        showOverlay(cursorMap[dir] || 'default');
    }

    function onResizeMove(e) {
        if (!resize) { return; }

        var dx  = e.clientX - resize.startX;
        var dy  = e.clientY - resize.startY;
        var dir = resize.dir;

        var newLeft = resize.origLeft;
        var newTop  = resize.origTop;
        var newW    = resize.origW;
        var newH    = resize.origH;

        if (dir.indexOf('e') !== -1) { newW = Math.max(MIN_W, resize.origW + dx); }
        if (dir.indexOf('s') !== -1) { newH = Math.max(MIN_H, resize.origH + dy); }

        if (dir.indexOf('w') !== -1) {
            var proposedW = resize.origW - dx;
            if (proposedW >= MIN_W) {
                newLeft = resize.origLeft + dx;
                newW    = proposedW;
            }
        }
        if (dir.indexOf('n') !== -1) {
            // Bottom edge stays fixed; clamp top to 0 (never under the toolbar)
            var rawTop  = resize.origTop + dy;
            var bottom  = resize.origTop + resize.origH;
            var clampedTop = Math.max(0, rawTop);
            var clampedH   = Math.max(MIN_H, bottom - clampedTop);
            newTop = clampedTop;
            newH   = clampedH;
        }

        resize.panel.style.left   = newLeft + 'px';
        resize.panel.style.top    = newTop  + 'px';
        resize.panel.style.width  = newW    + 'px';
        resize.panel.style.height = newH    + 'px';
    }

    function stopResize() {
        if (resize) { bringToFront(resize.panel); }
        resize = null;
        hideOverlay();
        refreshEditors();
        saveLayoutState();
    }

    // ── Minimize / Restore ────────────────────────────────────────

    function minimizePanel(panel) {
        if (panel.dataset.animating) { return; }
        // Save geometry before animating
        panel.dataset.savedLeft   = panel.style.left;
        panel.dataset.savedTop    = panel.style.top;
        panel.dataset.savedWidth  = panel.style.width;
        panel.dataset.savedHeight = panel.style.height;

        // Create taskbar chip (hidden until animation ends)
        var taskbar  = document.getElementById('panel-taskbar');
        if (!taskbar) { return; }
        var labelEl  = panel.querySelector('.panel-label');
        var chip     = document.createElement('button');
        chip.className        = 'taskbar-chip chip-pending';
        chip.dataset.panelId  = panel.id;
        chip.textContent      = labelEl ? labelEl.textContent : panel.id;
        chip.addEventListener('mousedown', function (e) {
            e.preventDefault();
            restorePanel(panel, chip);
        });
        taskbar.appendChild(chip);

        // Play suck-in animation
        panel.dataset.animating = '1';
        panel.classList.add('panel-minimizing');
        panel.addEventListener('animationend', function onMinEnd() {
            panel.removeEventListener('animationend', onMinEnd);
            panel.classList.remove('panel-minimizing');
            panel.style.display = 'none';
            delete panel.dataset.animating;
            // Show chip only after panel is gone
            chip.classList.remove('chip-pending');
            saveLayoutState();
        }, { once: true });
    }

    function restorePanel(panel, chip) {
        if (panel.dataset.animating) { return; }
        // Remove chip immediately
        if (chip && chip.parentNode) { chip.parentNode.removeChild(chip); }

        // Restore geometry and show
        if (panel.dataset.savedLeft)   { panel.style.left   = panel.dataset.savedLeft; }
        if (panel.dataset.savedTop)    { panel.style.top    = panel.dataset.savedTop; }
        if (panel.dataset.savedWidth)  { panel.style.width  = panel.dataset.savedWidth; }
        if (panel.dataset.savedHeight) { panel.style.height = panel.dataset.savedHeight; }
        panel.style.display = '';
        bringToFront(panel);

        // Play expand animation
        panel.dataset.animating = '1';
        panel.classList.add('panel-restoring');
        panel.addEventListener('animationend', function onRestEnd() {
            panel.removeEventListener('animationend', onRestEnd);
            panel.classList.remove('panel-restoring');
            delete panel.dataset.animating;
            refreshEditors();
            saveLayoutState();
        }, { once: true });
    }

    // ── Layout persistence ──────────────────────────────────────────

    function saveLayoutState() {
        // Debounce: called frequently during drag, so throttle actual writes
        clearTimeout(saveLayoutState._timer);
        saveLayoutState._timer = setTimeout(function () {
            var state = LiveCSS.storage.loadUIState() || {};
            state.panels = getLayoutState();
            LiveCSS.storage.saveUIState(state);
        }, 200);
    }

    /** Capture current panel positions, sizes, z-indices, and minimized state */
    function getLayoutState() {
        var panels = {};
        PANEL_IDS.forEach(function (id) {
            var panel = document.getElementById(id);
            if (!panel) return;
            var minimized = panel.style.display === 'none';
            panels[id] = {
                left:      minimized ? (panel.dataset.savedLeft   || panel.style.left)   : panel.style.left,
                top:       minimized ? (panel.dataset.savedTop    || panel.style.top)     : panel.style.top,
                width:     minimized ? (panel.dataset.savedWidth  || panel.style.width)   : panel.style.width,
                height:    minimized ? (panel.dataset.savedHeight || panel.style.height)  : panel.style.height,
                zIndex:    panel.style.zIndex || '',
                minimized: minimized
            };
        });
        return panels;
    }

    /** Restore panels from saved state. Returns true if restore succeeded. */
    function restoreLayoutState(savedPanels) {
        if (!savedPanels) return false;
        var anyRestored = false;
        var taskbar = document.getElementById('panel-taskbar');

        PANEL_IDS.forEach(function (id) {
            var panel = document.getElementById(id);
            var saved = savedPanels[id];
            if (!panel || !saved) return;

            panel.style.left   = saved.left   || '';
            panel.style.top    = saved.top    || '';
            panel.style.width  = saved.width  || '';
            panel.style.height = saved.height || '';
            if (saved.zIndex) { panel.style.zIndex = saved.zIndex; }

            if (saved.minimized) {
                // Minimized without animation on restore
                panel.dataset.savedLeft   = saved.left   || '';
                panel.dataset.savedTop    = saved.top    || '';
                panel.dataset.savedWidth  = saved.width  || '';
                panel.dataset.savedHeight = saved.height || '';
                panel.style.display = 'none';

                // Create taskbar chip
                if (taskbar) {
                    var labelEl = panel.querySelector('.panel-label');
                    var chip    = document.createElement('button');
                    chip.className       = 'taskbar-chip';
                    chip.dataset.panelId = panel.id;
                    chip.textContent     = labelEl ? labelEl.textContent : panel.id;
                    chip.addEventListener('mousedown', function (e) {
                        e.preventDefault();
                        restorePanel(panel, chip);
                    });
                    taskbar.appendChild(chip);
                }
            }

            anyRestored = true;
        });

        if (anyRestored) {
            // Determine highest z-index
            var maxZ = 10;
            PANEL_IDS.forEach(function (id) {
                var p = document.getElementById(id);
                if (p) { maxZ = Math.max(maxZ, parseInt(p.style.zIndex, 10) || 10); }
            });
            zCounter = maxZ + 1;
        }

        return anyRestored;
    }

    // ── Init ──────────────────────────────────────────────────────

    function init() {
        layout  = document.querySelector('.editor-layout');
        overlay = document.getElementById('dragOverlay');

        // Set up each panel
        PANEL_IDS.forEach(function (id, i) {
            var panel = document.getElementById(id);
            if (!panel) { return; }

            // Add the 6-dot grip spans to the drag handle
            var grip = panel.querySelector('.panel-drag-handle');
            if (grip && !grip.children.length) {
                for (var s = 0; s < 6; s++) {
                    var dot = document.createElement('span');
                    grip.appendChild(dot);
                }
            }

            // Add edge/corner resize handles
            addResizeHandles(panel);

            // Minimize button
            var minBtn = panel.querySelector('.panel-min-btn');
            if (minBtn) {
                minBtn.addEventListener('mousedown', function (e) {
                    e.preventDefault();
                    e.stopPropagation();
                    minimizePanel(panel);
                });
            }

            // Header drag: start panel move
            var header = panel.querySelector('.panel-header');
            if (header) {
                header.addEventListener('mousedown', function (e) {
                    // Don't start drag if clicking a resize handle
                    if (e.target.classList.contains('panel-resize')) { return; }
                    startDrag(e, panel);
                });
            }

            // Click anywhere on panel → bring to front
            panel.addEventListener('mousedown', function (e) {
                if (!e.target.classList.contains('panel-resize')) {
                    bringToFront(panel);
                }
            });

            // Also catch clicks on the panel header explicitly
            var headerEl = panel.querySelector('.panel-header');
            if (headerEl) {
                headerEl.addEventListener('click', function () {
                    bringToFront(panel);
                });
            }

            // For CodeMirror editors: catch clicks on the CM wrapper
            var cmWrap = panel.querySelector('.CodeMirror');
            if (cmWrap) {
                cmWrap.addEventListener('mousedown', function () {
                    bringToFront(panel);
                });
            } else {
                // CM may not exist yet at init time; observe DOM for it
                var observer = new MutationObserver(function (mutations, obs) {
                    var cm = panel.querySelector('.CodeMirror');
                    if (cm) {
                        cm.addEventListener('mousedown', function () {
                            bringToFront(panel);
                        });
                        obs.disconnect();
                    }
                });
                observer.observe(panel, { childList: true, subtree: true });
            }

            // For the preview panel: detect iframe focus to bring to front
            var iframe = panel.querySelector('iframe');
            if (iframe) {
                iframe.addEventListener('mouseenter', function () {
                    var pollId = setInterval(function () {
                        if (document.activeElement === iframe) {
                            bringToFront(panel);
                            clearInterval(pollId);
                        }
                    }, 80);
                    iframe.addEventListener('mouseleave', function onLeave() {
                        clearInterval(pollId);
                        iframe.removeEventListener('mouseleave', onLeave);
                    });
                });
                // Also use focus event on the window to detect iframe clicks
                window.addEventListener('blur', function () {
                    setTimeout(function () {
                        if (document.activeElement === iframe) {
                            bringToFront(panel);
                        }
                    }, 0);
                });
            }

            // Resize handle mousedown
            panel.addEventListener('mousedown', function (e) {
                if (e.target.dataset && e.target.dataset.dir) {
                    startResize(e, panel, e.target.dataset.dir);
                }
            });
        });

        // Try to restore saved layout; fall back to default tiled layout
        var uiState = LiveCSS.storage.loadUIState();
        if (!uiState || !uiState.panels || !restoreLayoutState(uiState.panels)) {
            applyDefaultLayout();
        }

        // Global move/up handlers
        document.addEventListener('mousemove', function (e) {
            if (drag)   { onDragMove(e); }
            if (resize) { onResizeMove(e); }
        });

        document.addEventListener('mouseup', function () {
            if (drag)   { stopDrag(); }
            if (resize) { stopResize(); }
        });

        window.addEventListener('blur', function () {
            if (drag)   { stopDrag(); }
            if (resize) { stopResize(); }
        });

        // Re-layout on window resize (scale panels proportionally)
        var lastW = getLayoutRect().width;
        var lastH = getLayoutRect().height;
        window.addEventListener('resize', function () {
            var r  = getLayoutRect();
            var rw = r.width;
            var rh = r.height;
            if (!rw || !rh || (rw === lastW && rh === lastH)) { return; }

            var scaleX = rw / lastW;
            var scaleY = rh / lastH;

            PANEL_IDS.forEach(function (id) {
                var panel = document.getElementById(id);
                if (!panel) { return; }
                panel.style.left   = (panel.offsetLeft * scaleX) + 'px';
                panel.style.top    = (panel.offsetTop  * scaleY) + 'px';
                panel.style.width  = (panel.offsetWidth  * scaleX) + 'px';
                panel.style.height = (panel.offsetHeight * scaleY) + 'px';
            });

            lastW = rw;
            lastH = rh;
            refreshEditors();
        });
    }

    // Public: reset all panels to default tiled positions
    function resetLayout() {
        // Restore any minimized panels first
        var taskbar = document.getElementById('panel-taskbar');
        if (taskbar) {
            var chips = taskbar.querySelectorAll('.taskbar-chip');
            chips.forEach(function (chip) {
                var panel = document.getElementById(chip.dataset.panelId);
                if (panel) { panel.style.display = ''; }
                chip.parentNode.removeChild(chip);
            });
        }
        applyDefaultLayout();
        refreshEditors();
        // Clear saved layout so next reload uses default
        var state = LiveCSS.storage.loadUIState() || {};
        delete state.panels;
        LiveCSS.storage.saveUIState(state);
    }

    return { init: init, resetLayout: resetLayout, minimizePanel: minimizePanel, restorePanel: restorePanel, getLayoutState: getLayoutState };

}());
