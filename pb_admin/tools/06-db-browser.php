<?php
/**
 * tools/06-db-browser.php
 * Launcher card for the local DB Browser dev tool.
 * Clicking "launch" opens quick-launch.sh in a new Terminal window via osascript.
 */
$tool_id    = '06-db-browser';
$tool_title = 'db browser';
$tool_icon  = '&#9635;';
$tool_cols  = 1;
?>
<style>
#dbb-wrap .dbb-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 5px 0;
    border-bottom: 1px solid rgba(255,255,255,0.04);
    font-size: 11px;
}
#dbb-wrap .dbb-row:last-child { border-bottom: none; }
#dbb-wrap .dbb-key   { color: var(--c-text-faint); }
#dbb-wrap .dbb-val   { color: var(--c-text-dim); font-family: monospace; font-size: 10px; }
#dbb-wrap .dbb-ok    { color: var(--c-ok); }
#dbb-wrap .dbb-warn  { color: var(--c-warn); }
#dbb-wrap .dbb-db-list {
    margin-top: 10px;
    font-size: 10px;
    color: var(--c-text-faint);
    letter-spacing: 0.06em;
}
#dbb-wrap .dbb-db-item {
    display: flex;
    justify-content: space-between;
    padding: 3px 0;
    border-bottom: 1px solid rgba(255,255,255,0.03);
    color: var(--c-text-muted);
    font-family: monospace;
}
#dbb-wrap .dbb-db-size { color: var(--c-text-faint); flex-shrink: 0; margin-left: 8px; }
#dbb-wrap .dbb-actions {
    display: flex;
    gap: 8px;
    margin-top: 14px;
}
#dbb-wrap .dbb-btn {
    background: var(--c-acc-bg);
    border: 1px solid var(--c-border-acc);
    color: var(--c-text-dim);
    font-family: inherit;
    font-size: 10px;
    letter-spacing: 0.06em;
    padding: 5px 12px;
    cursor: pointer;
    transition: background 0.13s, color 0.13s;
}
#dbb-wrap .dbb-btn:hover   { background: var(--c-acc-bg2); color: var(--c-text); }
#dbb-wrap .dbb-btn:disabled { opacity: 0.45; cursor: not-allowed; }
#dbb-wrap .dbb-msg {
    font-size: 10px;
    padding: 5px 8px;
    border: 1px solid;
    margin-top: 10px;
    display: none;
}
#dbb-wrap .dbb-msg.show { display: block; }
#dbb-wrap .dbb-msg.ok   { color: var(--c-ok);  background: var(--c-ok-bg);  border-color: var(--c-ok-border); }
#dbb-wrap .dbb-msg.err  { color: var(--c-err); background: var(--c-err-bg); border-color: var(--c-err-border); }
</style>

<div id="dbb-wrap">
    <div id="dbb-status-block" style="color:var(--c-text-faint);font-size:11px;">checking...</div>

    <div class="dbb-actions">
        <button class="dbb-btn" id="dbb-launch-btn" onclick="dbbLaunch()" disabled>launch in terminal</button>
        <button class="dbb-btn" id="dbb-refresh-btn" onclick="dbbLoad()">refresh</button>
    </div>

    <div class="dbb-msg" id="dbb-msg"></div>
</div>

<script>
(function() {
    function dbbMsg(text, type) {
        var m = document.getElementById('dbb-msg');
        if (!m) return;
        m.textContent = text;
        m.className   = 'dbb-msg show ' + (type || 'ok');
        clearTimeout(m._t);
        m._t = setTimeout(function() { m.className = 'dbb-msg'; }, 4000);
    }

    function humanSize(bytes) {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
        return (bytes / 1048576).toFixed(1) + ' MB';
    }

    window.dbbLoad = function() {
        var block   = document.getElementById('dbb-status-block');
        var launchBtn = document.getElementById('dbb-launch-btn');
        if (block) block.innerHTML = '<span style="color:var(--c-text-faint)">checking...</span>';
        if (launchBtn) launchBtn.disabled = true;

        apiFetch('db_browser_status', {}, function(e, d) {
            if (e || !d || !d.ok) {
                var msg = e ? e.message : (d && d.error ? d.error : 'unknown error');
                if (block) block.innerHTML = '<span style="color:var(--c-err);font-size:11px;">error: ' + msg + '</span>';
                console.error('[db-browser] status error:', msg);
                return;
            }

            var html = '<div class="data-table">';

            html += '<div class="dbb-row">'
                 +    '<span class="dbb-key">binary</span>'
                 +    '<span class="dbb-val ' + (d.binary_ok ? 'dbb-ok' : 'dbb-warn') + '">'
                 +      (d.binary_ok ? 'built' : 'not built - run quick-launch.sh to build')
                 +    '</span>'
                 + '</div>';

            html += '<div class="dbb-row">'
                 +    '<span class="dbb-key">script</span>'
                 +    '<span class="dbb-val ' + (d.script_ok ? 'dbb-ok' : 'dbb-warn') + '">'
                 +      (d.script_ok ? 'found' : 'missing')
                 +    '</span>'
                 + '</div>';

            html += '</div>';

            if (d.db_files && d.db_files.length > 0) {
                html += '<div class="dbb-db-list">';
                html += '<div style="letter-spacing:0.1em;text-transform:uppercase;font-size:9px;color:var(--c-text-faint);margin-bottom:4px;">detected databases</div>';
                d.db_files.forEach(function(f) {
                    html += '<div class="dbb-db-item">'
                         +    '<span>' + f.name + '</span>'
                         +    '<span class="dbb-db-size">' + humanSize(f.size) + '</span>'
                         + '</div>';
                });
                html += '</div>';
            }

            if (block) block.innerHTML = html;
            if (launchBtn) launchBtn.disabled = !d.script_ok;
        });
    };

    window.dbbLaunch = function() {
        var btn = document.getElementById('dbb-launch-btn');
        if (btn) btn.disabled = true;

        apiFetch('launch_db_browser', {}, function(e, d) {
            if (btn) btn.disabled = false;
            if (e || !d || !d.ok) {
                var msg = e ? e.message : (d && d.error ? d.error : 'unknown error');
                dbbMsg('launch failed: ' + msg, 'err');
                console.error('[db-browser] launch error:', msg);
                return;
            }
            dbbMsg('terminal opened', 'ok');
        });
    };

    // Register refresh handler so the card refresh button works
    if (typeof registerTool === 'function') {
        registerTool('06-db-browser', window.dbbLoad);
    }

    // Initial load
    dbbLoad();
})();
</script>
