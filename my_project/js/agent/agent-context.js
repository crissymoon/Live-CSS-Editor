/**
 * agent-context.js -- Context tab loader.
 * Depends on: agent-core.js
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    function loadContext() {
        if (!state.filePath) { return; }
        C.agentPost({ action: 'context_get', file_path: state.filePath }).then(function (data) {
            if (data.error) { dom.contextBody.textContent = data.error; return; }
            dom.contextBody.innerHTML = [
                '<div class="agent-context-section-title">Requests (' + (data.requests || []).length + ')</div>',
                '<div class="agent-context-json">' + C.esc(JSON.stringify(data.requests   || [], null, 2)) + '</div>',
                '<div class="agent-context-section-title">Connections</div>',
                '<div class="agent-context-json">' + C.esc(JSON.stringify(data.connections || [], null, 2)) + '</div>',
                '<div class="agent-context-section-title">Learned</div>',
                '<div class="agent-context-json">' + C.esc(JSON.stringify(data.learned    || {}, null, 2)) + '</div>',
                '<div class="agent-context-section-title">Change Log (' + (data.change_log || []).length + ')</div>',
                '<div class="agent-context-json">' + C.esc(JSON.stringify(data.change_log  || [], null, 2)) + '</div>',
            ].join('');
        });
    }

    C.loadContext = loadContext;

}(window.LiveCSS = window.LiveCSS || {}));
