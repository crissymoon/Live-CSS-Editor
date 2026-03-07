<?php
/**
 * tools/01-auth-status.php
 * Shows auth server health status and config summary.
 * Refresh: live fetch to api_proxy health endpoint every 15 seconds.
 */
$tool_id    = '01-auth-status';
$tool_title = 'auth server';
$tool_icon  = '&bull;';
$tool_cols  = 1;
?>
<div id="ast-wrap">
    <div style="display:flex;align-items:center;gap:10px;margin-bottom:12px;">
        <span class="status-dot pending" id="ast-dot"></span>
        <span id="ast-status-text" style="font-size:11px;color:#5555a0;">connecting...</span>
    </div>

    <div id="ast-detail" style="display:none;">
        <div style="font-size:10px;color:#3a3a5a;margin-bottom:8px;letter-spacing:0.06em;">server info</div>
        <div class="data-table" style="font-size:11px;">
            <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(255,255,255,0.04);">
                <span style="color:#444470;">endpoint</span>
                <span id="ast-endpoint" style="color:#8888a0;text-align:right;"></span>
            </div>
            <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(255,255,255,0.04);">
                <span style="color:#444470;">server time</span>
                <span id="ast-ts" style="color:#8888a0;"></span>
            </div>
            <div style="display:flex;justify-content:space-between;padding:4px 0;">
                <span style="color:#444470;">last checked</span>
                <span id="ast-checked" style="color:#8888a0;"></span>
            </div>
        </div>
    </div>

    <div id="ast-err" style="display:none;" class="err-msg"></div>
</div>

<script>
(function() {
    function checkHealth() {
        var dot   = document.getElementById('ast-dot');
        var txt   = document.getElementById('ast-status-text');
        var det   = document.getElementById('ast-detail');
        var ep    = document.getElementById('ast-endpoint');
        var ts    = document.getElementById('ast-ts');
        var chk   = document.getElementById('ast-checked');
        var err   = document.getElementById('ast-err');

        dot.className = 'status-dot pending';
        apiFetch('health', {}, function(e, d) {
            var now = new Date().toISOString().replace('T',' ').slice(0,19);
            if (e || !d || !d.ok) {
                dot.className = 'status-dot offline';
                txt.textContent = 'offline - start xcm_auth server';
                txt.style.color = '#a06060';
                det.style.display = 'none';
                err.style.display = 'block';
                err.textContent = e ? e.message : (d && d.error ? d.error : 'unreachable');
                console.error('[auth-status] health check failed:', e || d);
            } else {
                dot.className = 'status-dot online';
                txt.textContent = 'online';
                txt.style.color = '#10b981';
                det.style.display = 'block';
                err.style.display = 'none';
                ep.textContent   = d.server || '';
                ts.textContent   = (d.data && d.data.ts) ? d.data.ts : '--';
                chk.textContent  = now + ' UTC';
            }
        });
    }

    registerTool('01-auth-status', checkHealth);
    checkHealth();
    setInterval(checkHealth, 15000);
})();
</script>
