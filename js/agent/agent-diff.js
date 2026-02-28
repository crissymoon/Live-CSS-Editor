/**
 * agent-diff.js -- Version diff and version list views.
 * Depends on: agent-core.js
 * Cross-calls: C.switchTab (registered by agent.js)
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    function refreshDiffSelectors() {
        // stub – selectors are static for now
    }

    function showDiff() {
        if (!state.filePath) { C.toast('Load a file first.', 'error'); return; }
        C.setStatus('busy', 'Computing diff...');
        C.agentPost({ action: 'diff', file_path: state.filePath })
            .then(function (data) {
                if (data.error) { C.setStatus('error', data.error); return; }
                dom.diffTable.innerHTML = data.html || '';
                var sum = data.summary || {};
                dom.diffSummary.style.display = 'flex';
                dom.diffSummary.innerHTML = [
                    '<span class="diff-added">+'  + (sum.added   || 0) + ' added</span>',
                    '<span class="diff-removed">-' + (sum.removed || 0) + ' removed</span>',
                ].join('');
                if (data.message) {
                    dom.diffSummary.innerHTML += '<span style="color:var(--ag-text-muted)">' + data.message + '</span>';
                }
                C.setStatus('ok', 'Diff ready');
                C.switchTab('diff');
            }).catch(function (e) { C.setStatus('error', e.message); });
    }

    function renderVersionList() {
        C.agentPost({ action: 'get_versions', file_path: state.filePath }).then(function (data) {
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            dom.versionPath.textContent = state.filePath;
            dom.versionList.innerHTML   = '';

            if (!state.versions.length) {
                dom.versionList.innerHTML = '<li style="padding:14px;color:var(--ag-text-muted);font-size:12px;">No versions stored.</li>';
                return;
            }

            state.versions.forEach(function (v, idx) {
                var cur    = state.history.current_version;
                var count  = state.versions.length;
                var posIdx = (count - 1) - idx;
                var isCur  = posIdx === parseInt(cur || 0);
                var li     = C.el('li', 'agent-version-item' + (isCur ? ' version-current' : ''));
                var date   = new Date(parseInt(v.created_at) * 1000);
                li.innerHTML = [
                    '<div class="agent-version-meta">',
                    '  <div class="agent-version-label">' + (v.label || 'Version ' + (idx + 1)) + '</div>',
                    '  <div class="agent-version-time">' + date.toLocaleString() + '</div>',
                    '</div>',
                    '<button class="agent-btn agent-btn-ghost" style="font-size:10px;" data-vid="' + v.id + '">View Diff</button>',
                ].join('');
                li.querySelector('button').addEventListener('click', function () {
                    showDiffForVersion(v.id, state.versions);
                });
                dom.versionList.appendChild(li);
            });
        });
    }

    function showDiffForVersion(vid, versions) {
        var idx = versions.findIndex(function (v) { return v.id === vid; });
        if (idx < 0 || idx >= versions.length - 1) { C.toast('No previous version to diff against.'); return; }
        var newer = versions[idx].content;
        var older = versions[idx + 1].content;
        C.agentPost({ action: 'diff', file_path: state.filePath, old_text: older, new_text: newer })
            .then(function (data) {
                dom.diffTable.innerHTML = data.html || '';
                var sum = data.summary || {};
                dom.diffSummary.style.display = 'flex';
                dom.diffSummary.innerHTML = '<span class="diff-added">+' + sum.added + '</span><span class="diff-removed">-' + sum.removed + '</span>';
                C.switchTab('diff');
            });
    }

    // Register
    C.refreshDiffSelectors = refreshDiffSelectors;
    C.showDiff             = showDiff;
    C.renderVersionList    = renderVersionList;
    C.showDiffForVersion   = showDiffForVersion;

}(window.LiveCSS = window.LiveCSS || {}));
