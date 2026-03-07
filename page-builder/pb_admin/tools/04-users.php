<?php
/**
 * tools/04-users.php
 * User management panel.
 * Admin can view all users, add new users, change roles, and deactivate accounts.
 * All actions go through api_proxy.php -> xcm_auth Go API.
 * Non-admin users see a graceful notice.
 */
$tool_id    = '04-users';
$tool_title = 'user management';
$tool_icon  = '&#9671;';
$tool_cols  = 3;
?>
<style>
/* Scoped styles for this tool only */
#users-tool-wrap .um-toolbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    margin-bottom: 12px;
    flex-wrap: wrap;
}
#users-tool-wrap .um-btn {
    background: var(--c-acc-bg);
    border: 1px solid var(--c-border-acc);
    color: var(--c-text-dim);
    font-family: inherit;
    font-size: 10px;
    letter-spacing: 0.06em;
    padding: 4px 10px;
    cursor: pointer;
    transition: background 0.13s, color 0.13s;
}
#users-tool-wrap .um-btn:hover:not(:disabled) { background: var(--c-acc-bg2); color: var(--c-text); }
#users-tool-wrap .um-btn:disabled             { opacity: 0.45; cursor: not-allowed; }
#users-tool-wrap .um-msg {
    font-size: 10px;
    padding: 5px 8px;
    border: 1px solid;
    margin-bottom: 10px;
    display: none;
}
#users-tool-wrap .um-msg.show { display: block; }
#users-tool-wrap .um-msg.ok   { color: var(--c-ok);  background: var(--c-ok-bg);  border-color: var(--c-ok-border); }
#users-tool-wrap .um-msg.err  { color: var(--c-err); background: var(--c-err-bg); border-color: var(--c-err-border); }
#users-tool-wrap .um-form-panel {
    display: none;
    background: var(--c-bg-3);
    border: 1px solid var(--c-border-acc2);
    padding: 14px;
    margin-bottom: 14px;
}
#users-tool-wrap .um-form-panel.open { display: block; }
#users-tool-wrap .um-form-title {
    font-size: 10px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--c-text-faint);
    margin-bottom: 12px;
}
#users-tool-wrap .um-form-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(155px, 1fr));
    gap: 10px;
    margin-bottom: 10px;
}
#users-tool-wrap .um-field { display: flex; flex-direction: column; gap: 4px; }
#users-tool-wrap .um-label {
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--c-text-dim);
}
#users-tool-wrap .um-input,
#users-tool-wrap .um-select {
    background: var(--c-bg-1);
    border: 1px solid var(--c-border);
    color: var(--c-text);
    font-family: inherit;
    font-size: 11px;
    padding: 5px 7px;
    outline: none;
    width: 100%;
    transition: border-color 0.13s;
}
#users-tool-wrap .um-input:focus,
#users-tool-wrap .um-select:focus { border-color: var(--c-acc-border); }
#users-tool-wrap .um-form-actions { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
#users-tool-wrap .um-form-err { font-size: 10px; color: var(--c-err); display: none; }
#users-tool-wrap .um-form-err.show { display: inline; }
#users-tool-wrap .um-actions { display: flex; gap: 4px; flex-wrap: wrap; }
#users-tool-wrap .um-row-btn {
    background: none;
    border: 1px solid var(--c-border);
    color: var(--c-text-muted);
    font-family: inherit;
    font-size: 9px;
    letter-spacing: 0.05em;
    padding: 2px 6px;
    cursor: pointer;
    transition: all 0.12s;
}
#users-tool-wrap .um-row-btn:hover:not(:disabled) { background: var(--c-acc-bg2); border-color: var(--c-border-acc2); color: var(--c-text); }
#users-tool-wrap .um-row-btn.danger:hover:not(:disabled) { background: var(--c-err-bg); border-color: var(--c-err-border); color: var(--c-err); }
#users-tool-wrap .um-row-btn.ok:hover:not(:disabled)     { background: var(--c-ok-bg);  border-color: var(--c-ok-border);  color: var(--c-ok); }
#users-tool-wrap .um-row-btn:disabled { opacity: 0.35; cursor: not-allowed; }
#users-tool-wrap .inactive-row td { opacity: 0.5; }
</style>

<div id="users-tool-wrap">

    <div class="um-msg" id="um-global-msg"></div>

    <div class="um-toolbar">
        <span style="font-size:10px;color:var(--c-text-faint);" id="um-count">loading...</span>
        <div style="display:flex;gap:6px;">
            <button class="um-btn" id="um-add-btn" onclick="umToggleForm()">add user</button>
            <button class="um-btn" onclick="loadUsers()">refresh</button>
        </div>
    </div>

    <div class="um-form-panel" id="um-form-panel">
        <div class="um-form-title">new user</div>
        <div class="um-form-grid">
            <div class="um-field">
                <label class="um-label" for="um-username">username</label>
                <input class="um-input" type="text" id="um-username" autocomplete="off" placeholder="jane">
            </div>
            <div class="um-field">
                <label class="um-label" for="um-email">email</label>
                <input class="um-input" type="email" id="um-email" autocomplete="off" placeholder="jane@example.com">
            </div>
            <div class="um-field">
                <label class="um-label" for="um-password">password</label>
                <input class="um-input" type="password" id="um-password" autocomplete="new-password" placeholder="...">
            </div>
            <div class="um-field">
                <label class="um-label" for="um-role">role</label>
                <select class="um-select" id="um-role">
                    <option value="user">user</option>
                    <option value="admin">admin</option>
                </select>
            </div>
        </div>
        <div class="um-form-actions">
            <button class="um-btn" id="um-submit-btn" onclick="umSubmitCreate()">create user</button>
            <button class="um-btn" onclick="umToggleForm()">cancel</button>
            <span class="um-form-err" id="um-form-err"></span>
        </div>
    </div>

    <div id="um-table-wrap">
        <div class="skeleton-line"></div>
        <div class="skeleton-line"></div>
        <div class="skeleton-line"></div>
    </div>

</div>

<script>
(function() {

    /* ---- small utilities -------------------------------------------------- */

    function escHtml(s) {
        return String(s).replace(/[&<>"']/g, function(c) {
            return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];
        });
    }

    function showMsg(msg, type) {
        var el = document.getElementById('um-global-msg');
        if (!el) { console.error('[04-users] um-global-msg element not found'); return; }
        el.textContent = msg;
        el.className   = 'um-msg show ' + (type || 'ok');
        clearTimeout(el._t);
        el._t = setTimeout(function() { el.className = 'um-msg'; }, 5000);
    }

    function showFormErr(msg) {
        var el = document.getElementById('um-form-err');
        if (!el) { console.error('[04-users] um-form-err element not found'); return; }
        el.textContent = msg;
        el.className   = 'um-form-err show';
    }

    function clearFormErr() {
        var el = document.getElementById('um-form-err');
        if (el) el.className = 'um-form-err';
    }

    /* ---- POST with JSON body to api_proxy --------------------------------- */

    function apiPost(action, queryParams, bodyData, cb) {
        var qs = Object.keys(queryParams || {}).map(function(k) {
            return encodeURIComponent(k) + '=' + encodeURIComponent(queryParams[k]);
        }).join('&');
        var url = 'api_proxy.php?action=' + encodeURIComponent(action) + (qs ? '&' + qs : '');
        console.log('[04-users] apiPost:', action, queryParams, bodyData);
        fetch(url, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(bodyData)
        })
        .then(function(r) { return r.json(); })
        .then(function(d) { cb(null, d); })
        .catch(function(e) {
            console.error('[04-users] apiPost error for action=' + action + ':', e);
            cb(e, null);
        });
    }

    /* ---- load and render user table -------------------------------------- */

    function loadUsers() {
        var wrap  = document.getElementById('um-table-wrap');
        var count = document.getElementById('um-count');
        if (!wrap) { console.error('[04-users] um-table-wrap element not found'); return; }
        wrap.innerHTML = '<div class="skeleton-line"></div><div class="skeleton-line"></div><div class="skeleton-line"></div>';
        if (count) count.textContent = 'loading...';

        apiFetch('admin_users', { limit: 100, offset: 0 }, function(e, d) {
            if (e) {
                console.error('[04-users] loadUsers network error:', e);
                wrap.innerHTML = '<span class="err-msg">Network error: ' + escHtml(e.message) + '</span>';
                return;
            }
            if (!d || !d.ok) {
                var msg = (d && d.error) ? d.error : 'unknown error';
                if (msg.toLowerCase().indexOf('forbidden') !== -1 || msg.toLowerCase().indexOf('admin') !== -1) {
                    wrap.innerHTML = '<span style="font-size:11px;color:var(--c-ctrl-btn);">Admin role required to manage users.</span>';
                } else {
                    console.error('[04-users] loadUsers api error:', d);
                    wrap.innerHTML = '<span class="err-msg">Could not load users: ' + escHtml(msg) + '</span>';
                }
                return;
            }

            var rows = Array.isArray(d.data) ? d.data : [];
            if (count) count.textContent = rows.length + ' user' + (rows.length !== 1 ? 's' : '');

            if (rows.length === 0) {
                wrap.innerHTML = '<span class="empty-msg">No users found.</span>';
                return;
            }

            var html = '<table class="data-table"><thead><tr>'
                + '<th>id</th><th>username</th><th>email</th><th>role</th>'
                + '<th>active</th><th>verified</th><th>created</th><th>actions</th>'
                + '</tr></thead><tbody>';

            rows.forEach(function(u) {
                var roleBadge     = u.role === 'admin' ? '<span class="badge badge-admin">admin</span>' : '<span class="badge">user</span>';
                var activeBadge   = u.is_active   ? '<span class="badge badge-ok">yes</span>'   : '<span class="badge badge-err">no</span>';
                var verifiedBadge = u.is_verified ? '<span class="badge badge-ok">yes</span>'   : '<span class="badge badge-warn">no</span>';
                var created       = u.created_at  ? String(u.created_at).slice(0, 10) : '--';
                var otherRole     = u.role === 'admin' ? 'user' : 'admin';
                var rowClass      = u.is_active ? '' : 'inactive-row';

                var actBtns = '<button class="um-row-btn" '
                    + 'onclick="umSetRole(' + u.id + ',\'' + otherRole + '\')">'
                    + 'make ' + otherRole + '</button>';

                actBtns += u.is_active
                    ? '<button class="um-row-btn danger" onclick="umDeactivate(' + u.id + ',\'' + escHtml(u.username) + '\')">deactivate</button>'
                    : '<button class="um-row-btn ok"     onclick="umReactivate(' + u.id + ',\'' + escHtml(u.username) + '\')">reactivate</button>';

                html += '<tr class="' + rowClass + '" id="um-row-' + u.id + '">'
                    + '<td class="highlight">' + escHtml(String(u.id))          + '</td>'
                    + '<td class="highlight">' + escHtml(u.username || '--')    + '</td>'
                    + '<td>'                   + escHtml(u.email    || '--')    + '</td>'
                    + '<td>'                   + roleBadge                       + '</td>'
                    + '<td>'                   + activeBadge                     + '</td>'
                    + '<td>'                   + verifiedBadge                   + '</td>'
                    + '<td>'                   + created                         + '</td>'
                    + '<td><div class="um-actions">' + actBtns + '</div></td>'
                    + '</tr>';
            });

            html += '</tbody></table>';
            wrap.innerHTML = html;
        });
    }

    /* ---- form toggle ------------------------------------------------------ */

    window.umToggleForm = function() {
        var panel = document.getElementById('um-form-panel');
        var btn   = document.getElementById('um-add-btn');
        if (!panel) { console.error('[04-users] um-form-panel element not found'); return; }
        var open = panel.classList.toggle('open');
        if (btn) btn.textContent = open ? 'cancel' : 'add user';
        clearFormErr();
    };

    /* ---- create user ------------------------------------------------------ */

    window.umSubmitCreate = function() {
        var username = ((document.getElementById('um-username') || {}).value || '').trim();
        var email    = ((document.getElementById('um-email')    || {}).value || '').trim();
        var password = ((document.getElementById('um-password') || {}).value || '');
        var role     = ((document.getElementById('um-role')     || {}).value || 'user');
        clearFormErr();

        if (!username || !email || !password) {
            showFormErr('username, email, and password are required');
            return;
        }

        var submitBtn = document.getElementById('um-submit-btn');
        if (submitBtn) submitBtn.disabled = true;

        apiPost('admin_create_user', {}, { username: username, email: email, password: password, role: role }, function(e, d) {
            if (submitBtn) submitBtn.disabled = false;
            if (e) {
                console.error('[04-users] create user network error:', e);
                showFormErr('Network error: ' + e.message);
                return;
            }
            if (!d || !d.ok) {
                var errMsg = (d && d.error) ? d.error : 'unknown error';
                console.error('[04-users] create user api error:', d);
                showFormErr(errMsg);
                return;
            }
            console.log('[04-users] user created ok:', d.data);
            ['um-username','um-email','um-password'].forEach(function(id) {
                var el = document.getElementById(id);
                if (el) el.value = '';
            });
            var panel = document.getElementById('um-form-panel');
            var addBtn = document.getElementById('um-add-btn');
            if (panel)  panel.classList.remove('open');
            if (addBtn) addBtn.textContent = 'add user';
            showMsg('User "' + username + '" created.', 'ok');
            loadUsers();
        });
    };

    /* ---- change role ------------------------------------------------------ */

    window.umSetRole = function(id, role) {
        console.log('[04-users] umSetRole id=' + id + ' role=' + role);
        var row = document.getElementById('um-row-' + id);
        var btns = row ? row.querySelectorAll('.um-row-btn') : [];
        btns.forEach(function(b) { b.disabled = true; });

        apiPost('admin_update_user', { id: id }, { role: role }, function(e, d) {
            btns.forEach(function(b) { b.disabled = false; });
            if (e) {
                console.error('[04-users] setRole network error:', e);
                showMsg('Network error: ' + e.message, 'err');
                return;
            }
            if (!d || !d.ok) {
                console.error('[04-users] setRole api error:', d);
                showMsg('Role update failed: ' + ((d && d.error) ? d.error : 'unknown'), 'err');
                return;
            }
            console.log('[04-users] role updated ok:', d.data);
            showMsg('Role changed to "' + role + '".', 'ok');
            loadUsers();
        });
    };

    /* ---- deactivate ------------------------------------------------------- */

    window.umDeactivate = function(id, username) {
        if (!confirm('Deactivate "' + username + '"? They will not be able to log in.')) return;
        console.log('[04-users] umDeactivate id=' + id);
        var row  = document.getElementById('um-row-' + id);
        var btns = row ? row.querySelectorAll('.um-row-btn') : [];
        btns.forEach(function(b) { b.disabled = true; });

        apiFetch('admin_deactivate_user', { id: id }, function(e, d) {
            btns.forEach(function(b) { b.disabled = false; });
            if (e) {
                console.error('[04-users] deactivate network error:', e);
                showMsg('Network error: ' + e.message, 'err');
                return;
            }
            if (!d || !d.ok) {
                console.error('[04-users] deactivate api error:', d);
                showMsg('Deactivate failed: ' + ((d && d.error) ? d.error : 'unknown'), 'err');
                return;
            }
            console.log('[04-users] deactivated id=' + id);
            showMsg('"' + username + '" deactivated.', 'ok');
            loadUsers();
        });
    };

    /* ---- reactivate ------------------------------------------------------- */

    window.umReactivate = function(id, username) {
        console.log('[04-users] umReactivate id=' + id);
        var row  = document.getElementById('um-row-' + id);
        var btns = row ? row.querySelectorAll('.um-row-btn') : [];
        btns.forEach(function(b) { b.disabled = true; });

        apiPost('admin_reactivate_user', { id: id }, {}, function(e, d) {
            btns.forEach(function(b) { b.disabled = false; });
            if (e) {
                console.error('[04-users] reactivate network error:', e);
                showMsg('Network error: ' + e.message, 'err');
                return;
            }
            if (!d || !d.ok) {
                console.error('[04-users] reactivate api error:', d);
                showMsg('Reactivate failed: ' + ((d && d.error) ? d.error : 'unknown'), 'err');
                return;
            }
            console.log('[04-users] reactivated id=' + id);
            showMsg('"' + username + '" reactivated.', 'ok');
            loadUsers();
        });
    };

    /* ---- init ------------------------------------------------------------- */

    registerTool('04-users', loadUsers);
    loadUsers();

})();
</script>
