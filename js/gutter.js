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
        zCounter = Math.min(zCounter + 1, Z_OVERLAY - 2);
        panel.style.zIndex = zCounter;
        PANEL_IDS.forEach(function (id) {
            var p = document.getElementById(id);
            if (p) { p.classList.remove('is-top'); }
        });
        panel.classList.add('is-top');
    }

    function showOverlay(cur) {
        overlay.classList.add('active');
        overlay.style.cursor = cur || 'default';
        document.body.classList.add('is-dragging');
        document.body.style.cursor = cur || 'default';
    }

    function hideOverlay() {
        overlay.classList.remove('active');
        overlay.style.cursor = '';
        document.body.classList.remove('is-dragging', 'is-resizing');
        document.body.style.cursor = '';
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
            var proposedH = resize.origH - dy;
            if (proposedH >= MIN_H) {
                newTop = resize.origTop + dy;
                newH   = proposedH;
            }
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

            // Resize handle mousedown
            panel.addEventListener('mousedown', function (e) {
                if (e.target.dataset && e.target.dataset.dir) {
                    startResize(e, panel, e.target.dataset.dir);
                }
            });
        });

        // Apply default tiled layout
        applyDefaultLayout();

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
        applyDefaultLayout();
        refreshEditors();
    }

    return { init: init, resetLayout: resetLayout };

}());
