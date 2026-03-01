<?php
/**
 * tools/02-sessions.php
 * Lists active sessions for the current user from xcm_auth.
 */
$tool_id    = '02-sessions';
$tool_title = 'active sessions';
$tool_icon  = '&#9632;';
$tool_cols  = 2;
?>
<div id="sess-wrap">
    <div class="skeleton-line"></div>
    <div class="skeleton-line"></div>
    <div class="skeleton-line"></div>
</div>

<script>
(function() {
    function loadSessions() {
        var wrap = document.getElementById('sess-wrap');
        wrap.innerHTML = '<div class="skeleton-line"></div><div class="skeleton-line"></div>';
        apiFetch('sessions', {}, function(e, d) {
            if (e || !d || !d.ok) {
                console.error('[sessions] load error:', e || d);
                wrap.innerHTML = '<span class="err-msg">Could not load sessions: ' + (e ? e.message : (d && d.error ? d.error : 'unknown')) + '</span>';
                return;
            }
            var rows = d.data;
            if (!Array.isArray(rows) || rows.length === 0) {
                wrap.innerHTML = '<span class="empty-msg">No active sessions found.</span>';
                return;
            }
            var html = '<table class="data-table"><thead><tr>'
                + '<th>id</th><th>ip address</th><th>user agent</th><th>created</th><th>expires</th><th>status</th>'
                + '</tr></thead><tbody>';
            rows.forEach(function(s) {
                var status = s.Revoked ? '<span class="badge badge-err">revoked</span>' : '<span class="badge badge-ok">active</span>';
                var ua = (s.UserAgent || '').slice(0, 48) + ((s.UserAgent || '').length > 48 ? '...' : '');
                var created = s.CreatedAt ? s.CreatedAt.slice(0,19).replace('T',' ') : '--';
                var expires = s.ExpiresAt ? s.ExpiresAt.slice(0,19).replace('T',' ') : '--';
                html += '<tr>'
                    + '<td class="highlight">' + (s.ID || '--') + '</td>'
                    + '<td>' + escHtml(s.IPAddress || '--') + '</td>'
                    + '<td style="font-size:10px;">' + escHtml(ua) + '</td>'
                    + '<td>' + created + '</td>'
                    + '<td>' + expires + '</td>'
                    + '<td>' + status + '</td>'
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

    registerTool('02-sessions', loadSessions);
    loadSessions();
})();
</script>
