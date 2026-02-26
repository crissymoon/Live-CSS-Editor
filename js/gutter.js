/**
 * gutter.js — Draggable gutters for resizing the three editor panels
 * Attached to window.LiveCSS.gutter
 *
 * Call LiveCSS.gutter.init() after editor.init().
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.gutter = (function () {

    function init() {
        var overlay   = document.getElementById('dragOverlay');
        var container = document.querySelector('.editor-layout');

        var drag = { active: false, gutter: null, left: null, right: null };

        function startDrag(e, gutterEl, leftEl, rightEl) {
            e.preventDefault();
            drag.active = true;
            drag.gutter = gutterEl;
            drag.left   = leftEl;
            drag.right  = rightEl;
            overlay.classList.add('active');
            document.body.classList.add('is-dragging');
            gutterEl.classList.add('active');
        }

        function stopDrag() {
            if (!drag.active) return;
            overlay.classList.remove('active');
            document.body.classList.remove('is-dragging');
            if (drag.gutter) drag.gutter.classList.remove('active');
            drag.active = false;
            drag.gutter = drag.left = drag.right = null;
            LiveCSS.editor.getHtmlEditor().refresh();
            LiveCSS.editor.getCssEditor().refresh();
        }

        function onMove(e) {
            if (!drag.active) return;
            e.preventDefault();

            var containerRect = container.getBoundingClientRect();
            var totalWidth    = containerRect.width;
            var offsetX       = e.clientX - containerRect.left;

            var leftStart = drag.left.getBoundingClientRect().left - containerRect.left;
            var rightEnd  = drag.right.getBoundingClientRect().right - containerRect.left;

            var newLeft  = offsetX - leftStart - 3;
            var newRight = rightEnd - offsetX - 3;

            // Enforce minimum panel width
            if (newLeft < 150 || newRight < 150) return;

            drag.left.style.flex  = '0 0 ' + (newLeft  / totalWidth * 100) + '%';
            drag.right.style.flex = '0 0 ' + (newRight / totalWidth * 100) + '%';
        }

        document.getElementById('gutter1').addEventListener('mousedown', function (e) {
            startDrag(e, this, document.getElementById('htmlPanel'), document.getElementById('cssPanel'));
        });

        document.getElementById('gutter2').addEventListener('mousedown', function (e) {
            startDrag(e, this, document.getElementById('cssPanel'), document.getElementById('previewPanel'));
        });

        document.addEventListener('mousemove', onMove);
        document.addEventListener('mouseup', stopDrag);
        window.addEventListener('blur', stopDrag);
        document.addEventListener('mouseleave', stopDrag);
    }

    return { init: init };

}());
