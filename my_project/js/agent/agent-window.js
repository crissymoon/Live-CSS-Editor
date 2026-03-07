/**
 * agent-window.js -- Minimize / restore genie animation + drag.
 * Depends on: agent-core.js
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    function minimize() {
        state.minimized = true;

        dom.modal.classList.add('agent-genie-out');
        setTimeout(function () {
            dom.modal.classList.remove('agent-genie-out');
            dom.modal.classList.remove('agent-open');
            dom.modal.classList.add('agent-is-minimized');
        }, 300);

        var chip = document.getElementById('agent-taskbar-chip');
        if (!chip) {
            chip = document.createElement('button');
            chip.id        = 'agent-taskbar-chip';
            chip.className = 'taskbar-chip';
            chip.textContent = 'AGENT';
            chip.addEventListener('click', restore);
            var taskbar = document.getElementById('panel-taskbar');
            if (taskbar) { taskbar.appendChild(chip); }
        }
    }

    function restore() {
        state.minimized = false;

        dom.modal.classList.remove('agent-is-minimized');
        dom.modal.classList.add('agent-open');
        dom.modal.classList.add('agent-genie-in');
        setTimeout(function () { dom.modal.classList.remove('agent-genie-in'); }, 320);

        var chip = document.getElementById('agent-taskbar-chip');
        if (chip) { chip.remove(); }
    }

    function initDrag() {
        var dragging = false;
        var startX, startY, origLeft, origTop;

        dom.header.style.cursor = 'grab';

        dom.header.addEventListener('mousedown', function (e) {
            if (e.target.closest('button') || e.target.closest('.agent-theme-dot')) { return; }

            dragging = true;
            dom.header.style.cursor = 'grabbing';

            var rect = dom.modal.getBoundingClientRect();
            dom.modal.style.right  = 'auto';
            dom.modal.style.left   = rect.left + 'px';
            dom.modal.style.top    = rect.top  + 'px';
            dom.modal.style.bottom = 'auto';

            startX   = e.clientX;
            startY   = e.clientY;
            origLeft = rect.left;
            origTop  = rect.top;

            e.preventDefault();
        });

        document.addEventListener('mousemove', function (e) {
            if (!dragging) { return; }
            var dx = e.clientX - startX;
            var dy = e.clientY - startY;
            dom.modal.style.left = Math.max(0, origLeft + dx) + 'px';
            dom.modal.style.top  = Math.max(0, origTop  + dy) + 'px';
        });

        document.addEventListener('mouseup', function () {
            if (!dragging) { return; }
            dragging = false;
            dom.header.style.cursor = 'grab';
        });
    }

    C.minimize = minimize;
    C.restore  = restore;
    C.initDrag = initDrag;

}(window.LiveCSS = window.LiveCSS || {}));
