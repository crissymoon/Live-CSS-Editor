<?php
/**
 * tools/03-audit-log.php
 * Shows the 20 most recent audit log entries for the current user.
 */
$tool_id    = '03-audit-log';
$tool_title = 'audit log';
$tool_icon  = '&#9660;';
$tool_cols  = 3;
?>
<div id="audit-wrap">
    <div class="skeleton-line"></div>
    <div class="skeleton-line"></div>
    <div class="skeleton-line"></div>
</div>

<script>
(function() {
    function loadAudit() {
        var wrap = document.getElementById('audit-wrap');
        wrap.innerHTML = '<div class="skeleton-line"></div><div class="skeleton-line"></div>';
        apiFetch('audit', { limit: 20, offset: 0 }, function(e, d) {
            if (e || !d || !d.ok) {
                console.error('[audit-log] load error:', e || d);
                wrap.innerHTML = '<span class="err-msg">Could not load audit log: ' + (e ? e.message : (d && d.error ? d.error : 'unknown')) + '</span>';
                return;
            }
            var rows = d.data;
            if (!Array.isArray(rows) || rows.length === 0) {
                wrap.innerHTML = '<span class="empty-msg">No audit entries found.</span>';
                return;
            }
            var html = '<table class="data-table"><thead><tr>'
                + '<th>time</th><th>action</th><th>result</th><th>ip</th><th>detail</th>'
                + '</tr></thead><tbody>';
            rows.forEach(function(r) {
                var ts     = r.CreatedAt ? r.CreatedAt.slice(0,19).replace('T',' ') : '--';
                var status = r.Success
                    ? '<span class="badge badge-ok">ok</span>'
                    : '<span class="badge badge-err">fail</span>';
                var actionColor = r.Success ? '#c7c7f0' : '#a06060';
                html += '<tr>'
                    + '<td style="white-space:nowrap;">' + ts + '</td>'
                    + '<td style="color:' + actionColor + ';font-weight:600;">' + escHtml(r.Action || '--') + '</td>'
                    + '<td>' + status + '</td>'
                    + '<td>' + escHtml(r.IPAddress || '--') + '</td>'
                    + '<td style="font-size:10px;color:#5555a0;">' + escHtml(r.Detail || '') + '</td>'
                    + '</tr>';
            });
            html += '</tbody></table>';
            wrap.innerHTML = html;
        });
    }

    function escHtml(s) {
        return String(s).replace(/[&<>"']/g, function(c) {
            return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];
        });
    }

    registerTool('03-audit-log', loadAudit);
    loadAudit();
})();
</script>
