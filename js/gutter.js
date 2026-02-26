/**
 * gutter.js — Resizable gutters + drag-to-reorder panels
 * Attached to window.LiveCSS.gutter
 *
 * Call LiveCSS.gutter.init() after editor.init().
 *
 * Features:
 *   - Resize any panel by dragging the gutter between it and its neighbour
 *   - Reorder panels by dragging the handle (three-line grip) in a panel header
 *   - Layout is rebuilt from a panel-order array; gutters are created dynamically
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.gutter = (function () {
    'use strict';

    // Default left-to-right panel order
    var order = ['jsPanel', 'htmlPanel', 'cssPanel', 'previewPanel'];

    function getLayout() { return document.querySelector('.editor-layout'); }

    function refreshAllEditors() {
        setTimeout(function () {
            ['getJsEditor', 'getHtmlEditor', 'getCssEditor'].forEach(function (fn) {
                var ed = LiveCSS.editor[fn] && LiveCSS.editor[fn]();
                if (ed) { ed.refresh(); }
            });
        }, 50);
    }

    // ── Layout render ──────────────────────────────────────────────
    // Rebuilds panel + gutter DOM order from the `order` array.
    // Gutters are created fresh each time; event binding uses delegation.
    function renderLayout() {
        var layout  = getLayout();
        var overlay = document.getElementById('dragOverlay');

        // Remove stale gutters
        var stale = layout.querySelectorAll('.gutter');
        for (var s = 0; s < stale.length; s++) { layout.removeChild(stale[s]); }

        // Append panels in current order
        for (var p = 0; p < order.length; p++) {
            var panel = document.getElementById(order[p]);
            if (panel) { layout.appendChild(panel); }
        }

        // Keep drag-overlay as last child (covers iframe during resize drags)
        if (overlay) { layout.appendChild(overlay); }

        // Insert a gutter before every panel except the first
        for (var g = 1; g < order.length; g++) {
            var next = document.getElementById(order[g]);
            if (!next) { continue; }
            var gut = document.createElement('div');
            gut.className = 'gutter';
            layout.insertBefore(gut, next);
        }

        refreshAllEditors();
    }

    // ── Gutter resize (delegated) ──────────────────────────────────
    var resize = { active: false, el: null, left: null, right: null };

    function startResize(e, gutterEl, leftPanel, rightPanel) {
        e.preventDefault();
        resize = { active: true, el: gutterEl, left: leftPanel, right: rightPanel };
        var overlay = document.getElementById('dragOverlay');
        overlay.classList.add('active');
        document.body.classList.add('is-dragging');
        gutterEl.classList.add('active');
    }

    function stopResize() {
        if (!resize.active) { return; }
        document.getElementById('dragOverlay').classList.remove('active');
        document.body.classList.remove('is-dragging');
        if (resize.el) { resize.el.classList.remove('active'); }
        resize = { active: false, el: null, left: null, right: null };
        refreshAllEditors();
    }

    function onResizeMove(e) {
        if (!resize.active) { return; }
        e.preventDefault();

        var layout       = getLayout();
        var totalWidth   = layout.getBoundingClientRect().width;
        var originLeft   = layout.getBoundingClientRect().left;
        var leftStart    = resize.left.getBoundingClientRect().left - originLeft;
        var rightEnd     = resize.right.getBoundingClientRect().right - originLeft;
        var cursorOffset = e.clientX - originLeft;

        var newLeft  = cursorOffset - leftStart - 3;
        var newRight = rightEnd - cursorOffset - 3;

        if (newLeft < 120 || newRight < 120) { return; }

        resize.left.style.flex  = '0 0 ' + (newLeft  / totalWidth * 100) + '%';
        resize.right.style.flex = '0 0 ' + (newRight / totalWidth * 100) + '%';
    }

    // ── Panel drag-to-reorder ──────────────────────────────────────
    var pd = { active: false, panel: null, ghost: null, dropIndex: -1 };
    var dropLine = null;

    function startPanelDrag(e, panel) {
        e.preventDefault();
        pd.active = true;
        pd.panel  = panel;

        // Ghost label
        var ghost = document.createElement('div');
        ghost.className = 'panel-drag-ghost';
        var lbl = panel.querySelector('.panel-label');
        ghost.textContent = lbl ? lbl.textContent : 'Panel';
        ghost.style.left = (e.clientX + 14) + 'px';
        ghost.style.top  = (e.clientY - 12) + 'px';
        document.body.appendChild(ghost);
        pd.ghost = ghost;

        // Drop line indicator
        dropLine = document.createElement('div');
        dropLine.className = 'panel-drop-indicator hidden';
        document.body.appendChild(dropLine);

        panel.classList.add('panel-is-dragging');
        document.body.classList.add('is-panel-dragging');
    }

    function onPanelMove(e) {
        if (!pd.active) { return; }

        pd.ghost.style.left = (e.clientX + 14) + 'px';
        pd.ghost.style.top  = (e.clientY - 12) + 'px';

        // Find which slot the cursor is hovering over
        var layout = getLayout();
        var panels = Array.prototype.slice.call(
            layout.querySelectorAll('.editor-panel, .preview-panel')
        );
        var layoutRect = layout.getBoundingClientRect();
        var dropIdx    = panels.length;
        var lineX      = -1;

        for (var i = 0; i < panels.length; i++) {
            var r   = panels[i].getBoundingClientRect();
            var mid = r.left + r.width / 2;
            if (e.clientX < mid) {
                dropIdx = i;
                lineX   = r.left;
                break;
            }
            if (i === panels.length - 1) {
                lineX = r.right;
            }
        }

        pd.dropIndex = dropIdx;

        if (lineX >= 0) {
            dropLine.style.left   = lineX + 'px';
            dropLine.style.top    = layoutRect.top + 'px';
            dropLine.style.height = layoutRect.height + 'px';
            dropLine.classList.remove('hidden');
        }
    }

    function stopPanelDrag() {
        if (!pd.active) { return; }

        if (pd.ghost && pd.ghost.parentNode) { pd.ghost.parentNode.removeChild(pd.ghost); }
        if (dropLine && dropLine.parentNode) { dropLine.parentNode.removeChild(dropLine); }
        dropLine = null;

        if (pd.panel) { pd.panel.classList.remove('panel-is-dragging'); }
        document.body.classList.remove('is-panel-dragging');

        var dragId  = pd.panel ? pd.panel.id : null;
        var dropIdx = pd.dropIndex;

        pd = { active: false, panel: null, ghost: null, dropIndex: -1 };

        if (dragId && dropIdx >= 0) {
            var cur = order.indexOf(dragId);
            if (cur !== -1) {
                order.splice(cur, 1);
                if (cur < dropIdx) { dropIdx--; }
                if (dropIdx > order.length) { dropIdx = order.length; }
                order.splice(dropIdx, 0, dragId);
                renderLayout();
            }
        }
    }

    function cancelPanelDrag() {
        if (!pd.active) { return; }
        if (pd.ghost && pd.ghost.parentNode) { pd.ghost.parentNode.removeChild(pd.ghost); }
        if (dropLine && dropLine.parentNode) { dropLine.parentNode.removeChild(dropLine); }
        dropLine = null;
        if (pd.panel) { pd.panel.classList.remove('panel-is-dragging'); }
        document.body.classList.remove('is-panel-dragging');
        pd = { active: false, panel: null, ghost: null, dropIndex: -1 };
    }

    // ── Init ───────────────────────────────────────────────────────
    function init() {
        var layout = getLayout();

        // Build initial layout (creates gutters between panels)
        renderLayout();

        // Delegated mousedown: panel drag handle OR gutter resize
        layout.addEventListener('mousedown', function (e) {
            // Walk up to see if we hit a drag handle
            var el = e.target;
            while (el && el !== layout) {
                if (el.classList.contains('panel-drag-handle')) {
                    var panel = el.parentElement;
                    // Walk up from the header to find the panel
                    while (panel && panel !== layout) {
                        if (panel.classList.contains('editor-panel') ||
                            panel.classList.contains('preview-panel')) {
                            startPanelDrag(e, panel);
                            return;
                        }
                        panel = panel.parentElement;
                    }
                    return;
                }
                el = el.parentElement;
            }

            // Gutter resize
            if (e.target.classList.contains('gutter')) {
                var g    = e.target;
                var prev = g.previousElementSibling;
                var next = g.nextElementSibling;
                // Skip the drag-overlay (it is last child, not a panel)
                while (next && next.id === 'dragOverlay') { next = next.previousElementSibling; }
                if (prev && next) { startResize(e, g, prev, next); }
            }
        });

        document.addEventListener('mousemove', function (e) {
            if (resize.active) { onResizeMove(e); }
            if (pd.active)     { onPanelMove(e); }
        });

        document.addEventListener('mouseup', function () {
            if (resize.active) { stopResize(); }
            if (pd.active)     { stopPanelDrag(); }
        });

        window.addEventListener('blur', function () {
            if (resize.active) { stopResize(); }
            if (pd.active)     { cancelPanelDrag(); }
        });
    }

    return { init: init };

}());
