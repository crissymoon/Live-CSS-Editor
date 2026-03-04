<?php
/**
 * tools/07-accounting.php
 * Accounting database manager.
 * Renders inside the dashboard tool-card frame.
 *
 * Database: dev-tools/db-browser/databases/accounting.db
 * Actions routed through db_bridge via api_proxy.php?action=db
 *
 * Tabs:
 *   accounts      -- chart of accounts, filterable by type
 *   journal       -- journal entries list + new entry modal
 *   trial_balance -- per-account debit/credit totals (posted only)
 *   periods       -- fiscal periods
 */
$tool_id    = '07-accounting';
$tool_title = 'accounting';
$tool_icon  = '&#36;';
$tool_cols  = 3;
?>
<style>
/* ---- scoped to #acct-wrap ---- */
#acct-wrap {
    font-size: 11px;
}

#acct-wrap .acct-tabs {
    display: flex;
    gap: 2px;
    border-bottom: 1px solid var(--c-border);
    margin-bottom: 14px;
}

#acct-wrap .acct-tab {
    background: none;
    border: 1px solid transparent;
    border-bottom: none;
    color: var(--c-text-faint);
    font-family: inherit;
    font-size: 10px;
    letter-spacing: 0.08em;
    padding: 5px 14px;
    cursor: pointer;
    position: relative;
    bottom: -1px;
    transition: color 0.12s, background 0.12s;
}

#acct-wrap .acct-tab:hover {
    color: var(--c-text-dim);
    background: var(--c-acc-bg);
}

#acct-wrap .acct-tab.active {
    color: var(--c-text-1);
    background: var(--c-bg-1);
    border-color: var(--c-border);
    border-bottom-color: var(--c-bg-1);
}

#acct-wrap .acct-pane { display: none; }
#acct-wrap .acct-pane.active { display: block; }

#acct-wrap .acct-toolbar {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 10px;
    flex-wrap: wrap;
}

#acct-wrap .acct-toolbar label {
    font-size: 10px;
    color: var(--c-text-faint);
    letter-spacing: 0.06em;
}

#acct-wrap .acct-select,
#acct-wrap .acct-input {
    background: var(--c-bg-3);
    border: 1px solid var(--c-border);
    color: var(--c-text);
    font-family: inherit;
    font-size: 10px;
    padding: 4px 8px;
    outline: none;
    min-width: 120px;
}

#acct-wrap .acct-select:focus,
#acct-wrap .acct-input:focus {
    border-color: var(--c-acc-border);
}

#acct-wrap .acct-btn {
    background: var(--c-acc-bg);
    border: 1px solid var(--c-border-acc);
    color: var(--c-text-dim);
    font-family: inherit;
    font-size: 10px;
    letter-spacing: 0.06em;
    padding: 4px 12px;
    cursor: pointer;
    transition: background 0.12s, color 0.12s;
}

#acct-wrap .acct-btn:hover   { background: var(--c-acc-bg2); color: var(--c-text); }
#acct-wrap .acct-btn:disabled { opacity: 0.45; cursor: not-allowed; }
#acct-wrap .acct-btn.primary {
    background: var(--c-ok-bg);
    border-color: var(--c-ok-border);
    color: var(--c-ok);
}
#acct-wrap .acct-btn.primary:hover { opacity: 0.85; }

#acct-wrap .acct-msg {
    font-size: 10px;
    padding: 5px 8px;
    border: 1px solid;
    margin-bottom: 10px;
    display: none;
}
#acct-wrap .acct-msg.show { display: block; }
#acct-wrap .acct-msg.ok   { color: var(--c-ok);  background: var(--c-ok-bg);  border-color: var(--c-ok-border); }
#acct-wrap .acct-msg.err  { color: var(--c-err); background: var(--c-err-bg); border-color: var(--c-err-border); }

/* Type chips */
#acct-wrap .type-chip {
    display: inline-block;
    font-size: 9px;
    letter-spacing: 0.08em;
    padding: 1px 6px;
    border: 1px solid var(--c-border);
    color: var(--c-text-faint);
}
#acct-wrap .type-ASSET     { color: #60a5fa; border-color: #1e3a5f; background: #0d1f33; }
#acct-wrap .type-LIABILITY { color: #f87171; border-color: #5f1e1e; background: #330d0d; }
#acct-wrap .type-EQUITY    { color: #a78bfa; border-color: #3d1f5f; background: #1e0d33; }
#acct-wrap .type-REVENUE   { color: #34d399; border-color: #1a4d34; background: #0a2118; }
#acct-wrap .type-EXPENSE   { color: #fbbf24; border-color: #5f4b0a; background: #2b1f03; }

/* Pagination */
#acct-wrap .acct-pages {
    display: flex;
    gap: 6px;
    align-items: center;
    margin-top: 10px;
    font-size: 10px;
    color: var(--c-text-faint);
}

/* Trial balance totals row */
#acct-wrap .tb-section-head td {
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--c-ctrl-btn);
    padding-top: 14px;
}

#acct-wrap .tb-total-row td {
    border-top: 1px solid var(--c-border);
    color: var(--c-text-dim);
    font-weight: 600;
}

/* ---- Modal overlay ---- */
#acct-modal-overlay {
    display: none;
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.72);
    z-index: 200;
    align-items: center;
    justify-content: center;
}
#acct-modal-overlay.open { display: flex; }

#acct-modal {
    background: var(--c-bg-1);
    border: 1px solid var(--c-border-acc);
    width: 680px;
    max-width: 96vw;
    max-height: 88vh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

#acct-modal .modal-header {
    padding: 12px 16px;
    border-bottom: 1px solid var(--c-border);
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-shrink: 0;
}

#acct-modal .modal-header h3 {
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 0.06em;
    color: var(--c-text-1);
}

#acct-modal .modal-close {
    background: none;
    border: none;
    color: var(--c-text-faint);
    font-size: 16px;
    cursor: pointer;
    padding: 0 4px;
    line-height: 1;
}
#acct-modal .modal-close:hover { color: var(--c-text); }

#acct-modal .modal-body {
    padding: 16px;
    overflow-y: auto;
    flex: 1;
}

#acct-modal .modal-footer {
    padding: 10px 16px;
    border-top: 1px solid var(--c-border);
    display: flex;
    gap: 8px;
    justify-content: flex-end;
    flex-shrink: 0;
}

#acct-modal .modal-field {
    display: flex;
    flex-direction: column;
    gap: 5px;
    margin-bottom: 12px;
}

#acct-modal .modal-field label {
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--c-text-faint);
}

#acct-modal .modal-field input,
#acct-modal .modal-field select,
#acct-modal .modal-field textarea {
    background: var(--c-bg-3);
    border: 1px solid var(--c-border);
    color: var(--c-text);
    font-family: inherit;
    font-size: 11px;
    padding: 6px 8px;
    outline: none;
    width: 100%;
}
#acct-modal .modal-field input:focus,
#acct-modal .modal-field select:focus,
#acct-modal .modal-field textarea:focus { border-color: var(--c-acc-border); }
#acct-modal .modal-field input::placeholder { color: var(--c-text-ghost); }

#acct-modal .modal-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
}

/* Journal lines editor */
#acct-modal .lines-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 11px;
    margin-top: 8px;
}
#acct-modal .lines-table th {
    font-size: 9px;
    letter-spacing: 0.09em;
    text-transform: uppercase;
    color: var(--c-ctrl-btn);
    text-align: left;
    padding: 5px 6px 7px;
    border-bottom: 1px solid var(--c-border);
}
#acct-modal .lines-table td { padding: 4px 4px; vertical-align: middle; }
#acct-modal .lines-table input,
#acct-modal .lines-table select {
    background: var(--c-bg-3);
    border: 1px solid var(--c-border);
    color: var(--c-text);
    font-family: inherit;
    font-size: 10px;
    padding: 4px 6px;
    outline: none;
    width: 100%;
}
#acct-modal .lines-table input:focus,
#acct-modal .lines-table select:focus { border-color: var(--c-acc-border); }

#acct-modal .lines-balance {
    font-size: 10px;
    margin-top: 8px;
    padding: 5px 8px;
    background: var(--c-bg-3);
    border: 1px solid var(--c-border);
    display: flex;
    gap: 16px;
}
#acct-modal .lines-balance span { color: var(--c-text-faint); }
#acct-modal .lines-balance .bal-ok  { color: var(--c-ok); }
#acct-modal .lines-balance .bal-err { color: var(--c-err); }

#acct-modal .modal-err {
    font-size: 10px;
    color: var(--c-err);
    background: var(--c-err-bg);
    border: 1px solid var(--c-err-border);
    padding: 6px 10px;
    margin-bottom: 10px;
    display: none;
}
#acct-modal .modal-err.show { display: block; }

/* Entry detail panel */
#acct-wrap .acct-detail {
    background: var(--c-bg-3);
    border: 1px solid var(--c-border);
    padding: 12px 14px;
    margin-top: 10px;
    font-size: 11px;
}
#acct-wrap .acct-detail-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 10px;
}
#acct-wrap .acct-detail-meta {
    font-size: 10px;
    color: var(--c-text-faint);
    margin-bottom: 8px;
    display: flex;
    gap: 14px;
    flex-wrap: wrap;
}
</style>

<div id="acct-wrap">

    <div id="acct-msg" class="acct-msg"></div>

    <div class="acct-tabs">
        <button class="acct-tab active" data-tab="accounts" onclick="acctTab('accounts')">accounts</button>
        <button class="acct-tab"        data-tab="journal"  onclick="acctTab('journal')">journal</button>
        <button class="acct-tab"        data-tab="trial"    onclick="acctTab('trial')">trial balance</button>
        <button class="acct-tab"        data-tab="periods"  onclick="acctTab('periods')">periods</button>
    </div>

    <!-- ========= ACCOUNTS ========= -->
    <div id="acct-pane-accounts" class="acct-pane active">
        <div class="acct-toolbar">
            <label>type:</label>
            <select class="acct-select" id="acct-type-filter" onchange="acctLoadAccounts()">
                <option value="">all types</option>
                <option value="ASSET">asset</option>
                <option value="LIABILITY">liability</option>
                <option value="EQUITY">equity</option>
                <option value="REVENUE">revenue</option>
                <option value="EXPENSE">expense</option>
            </select>
            <button class="acct-btn primary" onclick="acctOpenNewAccount()">+ new account</button>
        </div>
        <div id="acct-accounts-body">
            <div class="skeleton-line"></div>
            <div class="skeleton-line"></div>
            <div class="skeleton-line"></div>
        </div>
    </div>

    <!-- ========= JOURNAL ========= -->
    <div id="acct-pane-journal" class="acct-pane">
        <div class="acct-toolbar">
            <button class="acct-btn primary" onclick="acctOpenNewJournal()">+ new entry</button>
            <button class="acct-btn" onclick="acctLoadJournal()">refresh</button>
            <span id="acct-journal-count" style="color:var(--c-text-faint);font-size:10px;margin-left:4px;"></span>
        </div>
        <div id="acct-journal-body">
            <div class="skeleton-line"></div>
            <div class="skeleton-line"></div>
        </div>
        <div class="acct-pages" id="acct-journal-pager" style="display:none;">
            <button class="acct-btn" id="acct-jrn-prev" onclick="acctJrnPage(-1)">prev</button>
            <span id="acct-jrn-page-label"></span>
            <button class="acct-btn" id="acct-jrn-next" onclick="acctJrnPage(1)">next</button>
        </div>
        <div id="acct-entry-detail" class="acct-detail" style="display:none;"></div>
    </div>

    <!-- ========= TRIAL BALANCE ========= -->
    <div id="acct-pane-trial" class="acct-pane">
        <div class="acct-toolbar">
            <span style="color:var(--c-text-faint);font-size:10px;">balances from posted entries only</span>
            <button class="acct-btn" style="margin-left:auto;" onclick="acctLoadTrial()">refresh</button>
        </div>
        <div id="acct-trial-body">
            <div class="skeleton-line"></div>
        </div>
    </div>

    <!-- ========= PERIODS ========= -->
    <div id="acct-pane-periods" class="acct-pane">
        <div id="acct-periods-body">
            <div class="skeleton-line"></div>
        </div>
    </div>

</div>

<!-- ========= MODAL ========= -->
<div id="acct-modal-overlay" onclick="acctModalClose(event)">
    <div id="acct-modal">
        <div class="modal-header">
            <h3 id="acct-modal-title">new journal entry</h3>
            <button class="modal-close" onclick="acctCloseModal()">&times;</button>
        </div>
        <div class="modal-body" id="acct-modal-body">
        </div>
        <div class="modal-footer" id="acct-modal-footer">
        </div>
    </div>
</div>

<script>
(function() {

    var DB_KEY  = 'dev-tools/db-browser/databases/accounting';
    var PROXY   = 'api_proxy.php';

    // ---- low-level helpers --------------------------------------------------

    function acctMsg(text, type) {
        var m = document.getElementById('acct-msg');
        if (!m) return;
        m.textContent = text;
        m.className   = 'acct-msg show ' + (type || 'ok');
        clearTimeout(m._t);
        m._t = setTimeout(function() { m.className = 'acct-msg'; }, 5000);
    }

    function acctPost(dbAction, params, cb) {
        var body = JSON.stringify({ action: dbAction, db: DB_KEY, params: params || {} });
        fetch(PROXY + '?action=db', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    body,
        })
        .then(function(r) { return r.json(); })
        .then(function(d) { cb(null, d); })
        .catch(function(e) {
            console.error('[07-accounting] acctPost error:', dbAction, e);
            cb(e, null);
        });
    }

    function fmt(v) {
        if (v === null || v === undefined) return '';
        return String(v);
    }

    function fmtMoney(v) {
        var n = parseFloat(v);
        if (isNaN(n)) return '0.00';
        return n.toFixed(2);
    }

    function fmtDate(v) {
        return v ? String(v).substring(0, 10) : '';
    }

    function esc(s) {
        var d = document.createElement('div');
        d.textContent = String(s || '');
        return d.innerHTML;
    }

    function typeChip(type) {
        return '<span class="type-chip type-' + esc(type) + '">' + esc((type || '').toLowerCase()) + '</span>';
    }

    // ---- tab switching ------------------------------------------------------

    window.acctTab = function(name) {
        document.querySelectorAll('#acct-wrap .acct-tab').forEach(function(btn) {
            btn.classList.toggle('active', btn.dataset.tab === name);
        });
        document.querySelectorAll('#acct-wrap .acct-pane').forEach(function(p) {
            p.classList.toggle('active', p.id === 'acct-pane-' + name);
        });

        if (name === 'journal'  && !_jrn.loaded)  { acctLoadJournal(); }
        if (name === 'trial'    && !_trial.loaded) { acctLoadTrial(); }
        if (name === 'periods'  && !_per.loaded)   { acctLoadPeriods(); }
    };

    // ---- state objects ------------------------------------------------------

    var _acct  = { type: '' };
    var _jrn   = { page: 0, perPage: 30, loaded: false };
    var _trial = { loaded: false };
    var _per   = { loaded: false };

    // ---- ACCOUNTS -----------------------------------------------------------

    window.acctLoadAccounts = function() {
        var type   = document.getElementById('acct-type-filter').value;
        var body   = document.getElementById('acct-accounts-body');
        body.innerHTML = '<div class="skeleton-line"></div><div class="skeleton-line"></div>';

        acctPost('accounting.accounts_list', { type: type }, function(err, resp) {
            if (err || !resp || !resp.ok) {
                body.innerHTML = '<span class="err-msg">Failed to load accounts.</span>';
                console.error('[07-accounting] accounts_list', err || (resp && resp.error));
                return;
            }

            var rows = resp.data || [];
            if (!rows.length) {
                body.innerHTML = '<span class="empty-msg">No accounts found.</span>';
                return;
            }

            var html = '<table class="data-table">'
                + '<thead><tr>'
                + '<th>code</th><th>name</th><th>type</th><th>normal bal</th><th>description</th>'
                + '</tr></thead><tbody>';

            rows.forEach(function(r) {
                html += '<tr>'
                    + '<td class="highlight" style="font-family:monospace;">' + esc(r.account_code) + '</td>'
                    + '<td>' + esc(r.account_name) + '</td>'
                    + '<td>' + typeChip(r.account_type) + '</td>'
                    + '<td style="color:var(--c-text-faint);font-size:10px;">' + esc(r.normal_balance) + '</td>'
                    + '<td style="color:var(--c-text-faint);">' + esc(r.description || '') + '</td>'
                    + '</tr>';
            });

            html += '</tbody></table>';
            body.innerHTML = html;
        });
    };

    // ---- JOURNAL ------------------------------------------------------------

    window.acctLoadJournal = function() {
        var body = document.getElementById('acct-journal-body');
        body.innerHTML = '<div class="skeleton-line"></div><div class="skeleton-line"></div>';
        document.getElementById('acct-entry-detail').style.display = 'none';

        var offset = _jrn.page * _jrn.perPage;

        acctPost('accounting.journal_list', { limit: _jrn.perPage, offset: offset }, function(err, resp) {
            _jrn.loaded = true;
            if (err || !resp || !resp.ok) {
                body.innerHTML = '<span class="err-msg">Failed to load journal.</span>';
                console.error('[07-accounting] journal_list', err || (resp && resp.error));
                return;
            }

            var rows = resp.data || [];
            var countEl = document.getElementById('acct-journal-count');
            if (countEl) countEl.textContent = rows.length + ' entries (page ' + (_jrn.page + 1) + ')';

            if (!rows.length) {
                body.innerHTML = '<span class="empty-msg">No journal entries found.</span>';
                document.getElementById('acct-journal-pager').style.display = 'none';
                return;
            }

            var html = '<table class="data-table">'
                + '<thead><tr>'
                + '<th>no.</th><th>date</th><th>description</th><th>ref</th>'
                + '<th style="text-align:right;">debit total</th><th>status</th><th></th>'
                + '</tr></thead><tbody>';

            rows.forEach(function(r) {
                var postedBadge = r.posted
                    ? '<span class="badge badge-ok">posted</span>'
                    : '<span class="badge badge-warn">draft</span>';

                html += '<tr>'
                    + '<td class="highlight" style="font-family:monospace;">' + esc(r.entry_number) + '</td>'
                    + '<td>' + esc(fmtDate(r.entry_date)) + '</td>'
                    + '<td>' + esc(r.description) + '</td>'
                    + '<td style="color:var(--c-text-faint);font-size:10px;">' + esc(r.reference || '') + '</td>'
                    + '<td style="text-align:right;font-family:monospace;">' + esc(fmtMoney(r.total_debits)) + '</td>'
                    + '<td>' + postedBadge + '</td>'
                    + '<td><button class="acct-btn" onclick="acctViewEntry(' + r.id + ')">view</button>'
                    + (!r.posted ? ' <button class="acct-btn primary" onclick="acctPostEntry(' + r.id + ')">post</button>' : '')
                    + '</td>'
                    + '</tr>';
            });

            html += '</tbody></table>';
            body.innerHTML = html;

            var pager = document.getElementById('acct-journal-pager');
            pager.style.display = 'flex';
            document.getElementById('acct-jrn-prev').disabled = (_jrn.page === 0);
            document.getElementById('acct-jrn-next').disabled = (rows.length < _jrn.perPage);
            document.getElementById('acct-jrn-page-label').textContent = 'page ' + (_jrn.page + 1);
        });
    };

    window.acctJrnPage = function(dir) {
        _jrn.page = Math.max(0, _jrn.page + dir);
        acctLoadJournal();
    };

    window.acctViewEntry = function(id) {
        var detail = document.getElementById('acct-entry-detail');
        detail.innerHTML = '<div class="skeleton-line"></div>';
        detail.style.display = 'block';
        detail.scrollIntoView({ behavior: 'smooth', block: 'nearest' });

        acctPost('accounting.journal_get', { id: id }, function(err, resp) {
            if (err || !resp || !resp.ok || !resp.data || !resp.data.id) {
                detail.innerHTML = '<span class="err-msg">Failed to load entry.</span>';
                return;
            }
            var e = resp.data;
            var lines = e.lines || [];

            var linesHtml = '';
            if (lines.length) {
                linesHtml = '<table class="data-table" style="margin-top:10px;">'
                    + '<thead><tr>'
                    + '<th>account</th><th style="text-align:right;">debit</th>'
                    + '<th style="text-align:right;">credit</th><th>memo</th>'
                    + '</tr></thead><tbody>';
                lines.forEach(function(l) {
                    linesHtml += '<tr>'
                        + '<td style="font-family:monospace;">'
                        + esc(l.account_code) + ' ' + esc(l.account_name) + '</td>'
                        + '<td style="text-align:right;font-family:monospace;">'
                        + (parseFloat(l.debit_amount) > 0 ? fmtMoney(l.debit_amount) : '') + '</td>'
                        + '<td style="text-align:right;font-family:monospace;">'
                        + (parseFloat(l.credit_amount) > 0 ? fmtMoney(l.credit_amount) : '') + '</td>'
                        + '<td style="color:var(--c-text-faint);">' + esc(l.memo || '') + '</td>'
                        + '</tr>';
                });
                linesHtml += '</tbody></table>';
            }

            var postedBadge = e.posted
                ? '<span class="badge badge-ok">posted</span>'
                : '<span class="badge badge-warn">draft</span>';

            detail.innerHTML = '<div class="acct-detail-header">'
                + '<strong style="font-size:12px;color:var(--c-text-1);">' + esc(e.entry_number) + '</strong>'
                + postedBadge
                + '</div>'
                + '<div class="acct-detail-meta">'
                + '<span>date: ' + esc(fmtDate(e.entry_date)) + '</span>'
                + '<span>ref: ' + esc(e.reference || '-') + '</span>'
                + (e.created_by ? '<span>by: ' + esc(e.created_by) + '</span>' : '')
                + (e.posted_at ? '<span>posted: ' + esc(fmtDate(e.posted_at)) + '</span>' : '')
                + '</div>'
                + '<div style="color:var(--c-text-dim);margin-bottom:6px;">' + esc(e.description) + '</div>'
                + linesHtml;
        });
    };

    window.acctPostEntry = function(id) {
        if (!confirm('Post this journal entry? This action cannot be undone.')) return;

        acctPost('accounting.journal_post', { id: id }, function(err, resp) {
            if (err || !resp || !resp.ok) {
                acctMsg('Failed to post entry: ' + ((resp && resp.error) || 'unknown error'), 'err');
                console.error('[07-accounting] journal_post', err || resp);
                return;
            }
            acctMsg('Entry posted.', 'ok');
            acctLoadJournal();
        });
    };

    // ---- TRIAL BALANCE ------------------------------------------------------

    window.acctLoadTrial = function() {
        var body = document.getElementById('acct-trial-body');
        body.innerHTML = '<div class="skeleton-line"></div>';

        acctPost('accounting.trial_balance', {}, function(err, resp) {
            _trial.loaded = true;
            if (err || !resp || !resp.ok) {
                body.innerHTML = '<span class="err-msg">Failed to load trial balance.</span>';
                console.error('[07-accounting] trial_balance', err || (resp && resp.error));
                return;
            }

            var rows = resp.data || [];
            if (!rows.length) {
                body.innerHTML = '<span class="empty-msg">No account data.</span>';
                return;
            }

            // Group by account_type
            var groups = {};
            var order  = ['ASSET', 'LIABILITY', 'EQUITY', 'REVENUE', 'EXPENSE'];
            order.forEach(function(t) { groups[t] = []; });
            rows.forEach(function(r) {
                if (groups[r.account_type]) groups[r.account_type].push(r);
            });

            var html = '<table class="data-table">'
                + '<thead><tr>'
                + '<th>code</th><th>account</th><th style="text-align:right;">debits</th>'
                + '<th style="text-align:right;">credits</th><th style="text-align:right;">balance</th>'
                + '</tr></thead><tbody>';

            order.forEach(function(type) {
                var group = groups[type];
                if (!group || !group.length) return;

                var groupDebit = 0, groupCredit = 0, groupBalance = 0;

                html += '<tr class="tb-section-head">'
                    + '<td colspan="5">' + typeChip(type) + ' ' + type.toLowerCase() + '</td>'
                    + '</tr>';

                group.forEach(function(r) {
                    var dr = parseFloat(r.total_debits)  || 0;
                    var cr = parseFloat(r.total_credits) || 0;
                    var bl = parseFloat(r.balance)       || 0;
                    groupDebit   += dr;
                    groupCredit  += cr;
                    groupBalance += bl;

                    var balColor = bl >= 0 ? 'var(--c-ok)' : 'var(--c-err)';

                    html += '<tr>'
                        + '<td style="font-family:monospace;color:var(--c-text-faint);">' + esc(r.account_code) + '</td>'
                        + '<td>' + esc(r.account_name) + '</td>'
                        + '<td style="text-align:right;font-family:monospace;">' + (dr > 0 ? fmtMoney(dr) : '') + '</td>'
                        + '<td style="text-align:right;font-family:monospace;">' + (cr > 0 ? fmtMoney(cr) : '') + '</td>'
                        + '<td style="text-align:right;font-family:monospace;color:' + balColor + ';">' + fmtMoney(bl) + '</td>'
                        + '</tr>';
                });

                html += '<tr class="tb-total-row">'
                    + '<td colspan="2" style="color:var(--c-text-faint);font-size:10px;">subtotal</td>'
                    + '<td style="text-align:right;font-family:monospace;">' + fmtMoney(groupDebit) + '</td>'
                    + '<td style="text-align:right;font-family:monospace;">' + fmtMoney(groupCredit) + '</td>'
                    + '<td style="text-align:right;font-family:monospace;">' + fmtMoney(groupBalance) + '</td>'
                    + '</tr>';
            });

            html += '</tbody></table>';
            body.innerHTML = html;
        });
    };

    // ---- PERIODS ------------------------------------------------------------

    window.acctLoadPeriods = function() {
        var body = document.getElementById('acct-periods-body');
        body.innerHTML = '<div class="skeleton-line"></div>';

        acctPost('accounting.period_list', {}, function(err, resp) {
            _per.loaded = true;
            if (err || !resp || !resp.ok) {
                body.innerHTML = '<span class="err-msg">Failed to load periods.</span>';
                return;
            }

            var rows = resp.data || [];
            if (!rows.length) {
                body.innerHTML = '<span class="empty-msg">No fiscal periods defined.</span>';
                return;
            }

            var html = '<table class="data-table">'
                + '<thead><tr>'
                + '<th>period</th><th>start</th><th>end</th><th>status</th>'
                + '</tr></thead><tbody>';

            rows.forEach(function(r) {
                var badge = r.is_closed
                    ? '<span class="badge">closed</span>'
                    : '<span class="badge badge-ok">open</span>';
                html += '<tr>'
                    + '<td class="highlight">' + esc(r.period_name) + '</td>'
                    + '<td>' + esc(fmtDate(r.start_date)) + '</td>'
                    + '<td>' + esc(fmtDate(r.end_date)) + '</td>'
                    + '<td>' + badge + '</td>'
                    + '</tr>';
            });

            html += '</tbody></table>';
            body.innerHTML = html;
        });
    };

    // ---- MODAL helpers ------------------------------------------------------

    window.acctModalClose = function(e) {
        if (e.target === document.getElementById('acct-modal-overlay')) {
            acctCloseModal();
        }
    };

    window.acctCloseModal = function() {
        document.getElementById('acct-modal-overlay').classList.remove('open');
    };

    function acctOpenModal(title, bodyHtml, footerHtml) {
        document.getElementById('acct-modal-title').textContent  = title;
        document.getElementById('acct-modal-body').innerHTML     = bodyHtml;
        document.getElementById('acct-modal-footer').innerHTML   = footerHtml;
        document.getElementById('acct-modal-overlay').classList.add('open');
    }

    // ---- NEW ACCOUNT modal --------------------------------------------------

    window.acctOpenNewAccount = function() {
        var body = '<div id="acc-modal-err" class="modal-err"></div>'
            + '<div class="modal-row">'
            + '  <div class="modal-field">'
            + '    <label>account code *</label>'
            + '    <input id="acct-nacct-code" type="text" placeholder="e.g. 1150" maxlength="20">'
            + '  </div>'
            + '  <div class="modal-field">'
            + '    <label>account name *</label>'
            + '    <input id="acct-nacct-name" type="text" placeholder="e.g. Petty Cash" maxlength="100">'
            + '  </div>'
            + '</div>'
            + '<div class="modal-row">'
            + '  <div class="modal-field">'
            + '    <label>account type *</label>'
            + '    <select id="acct-nacct-type">'
            + '      <option value="ASSET">ASSET</option>'
            + '      <option value="LIABILITY">LIABILITY</option>'
            + '      <option value="EQUITY">EQUITY</option>'
            + '      <option value="REVENUE">REVENUE</option>'
            + '      <option value="EXPENSE">EXPENSE</option>'
            + '    </select>'
            + '  </div>'
            + '  <div class="modal-field">'
            + '    <label>normal balance *</label>'
            + '    <select id="acct-nacct-bal">'
            + '      <option value="DEBIT">DEBIT</option>'
            + '      <option value="CREDIT">CREDIT</option>'
            + '    </select>'
            + '  </div>'
            + '</div>'
            + '<div class="modal-field">'
            + '  <label>description</label>'
            + '  <input id="acct-nacct-desc" type="text" placeholder="optional description" maxlength="500">'
            + '</div>';

        var footer = '<button class="acct-btn" onclick="acctCloseModal()">cancel</button>'
            + '<button class="acct-btn primary" onclick="acctSaveAccount()">save account</button>';

        acctOpenModal('new account', body, footer);
    };

    window.acctSaveAccount = function() {
        var code  = document.getElementById('acct-nacct-code').value.trim();
        var name  = document.getElementById('acct-nacct-name').value.trim();
        var type  = document.getElementById('acct-nacct-type').value;
        var bal   = document.getElementById('acct-nacct-bal').value;
        var desc  = document.getElementById('acct-nacct-desc').value.trim();
        var errEl = document.getElementById('acc-modal-err');

        if (!code || !name) {
            errEl.textContent = 'Account code and name are required.';
            errEl.classList.add('show');
            return;
        }

        acctPost('accounting.account_create', {
            account_code:   code,
            account_name:   name,
            account_type:   type,
            normal_balance: bal,
            description:    desc,
        }, function(err, resp) {
            if (err || !resp || !resp.ok) {
                var msg = (resp && resp.error) ? resp.error : 'Failed to create account.';
                errEl.textContent = msg;
                errEl.classList.add('show');
                console.error('[07-accounting] account_create', err || resp);
                return;
            }
            acctCloseModal();
            acctMsg('Account ' + code + ' created.', 'ok');
            acctLoadAccounts();
        });
    };

    // ---- NEW JOURNAL ENTRY modal --------------------------------------------

    var _nacctCache = [];
    var _lineCount  = 2;

    function acctRenderLines() {
        var opts = '<option value="">-- select account</option>';
        _nacctCache.forEach(function(a) {
            opts += '<option value="' + a.id + '">'
                + esc(a.account_code) + ' ' + esc(a.account_name) + '</option>';
        });

        var html = '<table class="lines-table" id="acct-lines-table">'
            + '<thead><tr>'
            + '<th style="width:40%;">account</th>'
            + '<th style="width:18%;text-align:right;">debit</th>'
            + '<th style="width:18%;text-align:right;">credit</th>'
            + '<th>memo</th>'
            + '<th style="width:28px;"></th>'
            + '</tr></thead><tbody id="acct-lines-body">';

        for (var i = 0; i < _lineCount; i++) {
            html += '<tr id="acct-line-' + i + '">'
                + '<td><select class="acct-line-acct" data-idx="' + i + '" onchange="acctUpdateBalance()">' + opts + '</select></td>'
                + '<td><input type="number" step="0.01" min="0" class="acct-line-dr" data-idx="' + i + '" placeholder="0.00" oninput="acctUpdateBalance()"></td>'
                + '<td><input type="number" step="0.01" min="0" class="acct-line-cr" data-idx="' + i + '" placeholder="0.00" oninput="acctUpdateBalance()"></td>'
                + '<td><input type="text" class="acct-line-memo" data-idx="' + i + '" placeholder="memo" maxlength="255"></td>'
                + '<td><button class="acct-btn" style="padding:2px 6px;" onclick="acctRemoveLine(' + i + ')">&times;</button></td>'
                + '</tr>';
        }

        html += '</tbody></table>'
            + '<div class="lines-balance" id="acct-bal-summary">'
            + '<span>debits: <strong id="acct-bal-dr">0.00</strong></span>'
            + '<span>credits: <strong id="acct-bal-cr">0.00</strong></span>'
            + '<span id="acct-bal-status" class="bal-err">not balanced</span>'
            + '</div>'
            + '<button class="acct-btn" style="margin-top:8px;" onclick="acctAddLine()">+ add line</button>';

        document.getElementById('acct-modal-lines').innerHTML = html;
    }

    window.acctUpdateBalance = function() {
        var dr = 0, cr = 0;
        document.querySelectorAll('.acct-line-dr').forEach(function(el) {
            dr += parseFloat(el.value) || 0;
        });
        document.querySelectorAll('.acct-line-cr').forEach(function(el) {
            cr += parseFloat(el.value) || 0;
        });
        document.getElementById('acct-bal-dr').textContent = dr.toFixed(2);
        document.getElementById('acct-bal-cr').textContent = cr.toFixed(2);
        var status = document.getElementById('acct-bal-status');
        if (Math.abs(dr - cr) < 0.005 && dr > 0) {
            status.textContent = 'balanced';
            status.className   = 'bal-ok';
        } else {
            status.textContent = Math.abs(dr - cr) < 0.005 ? 'add amounts' : 'not balanced';
            status.className   = 'bal-err';
        }
    };

    window.acctAddLine = function() {
        _lineCount++;
        var opts = '<option value="">-- select account</option>';
        _nacctCache.forEach(function(a) {
            opts += '<option value="' + a.id + '">'
                + esc(a.account_code) + ' ' + esc(a.account_name) + '</option>';
        });
        var i = _lineCount - 1;
        var row = document.createElement('tr');
        row.id = 'acct-line-' + i;
        row.innerHTML = '<td><select class="acct-line-acct" data-idx="' + i + '" onchange="acctUpdateBalance()">' + opts + '</select></td>'
            + '<td><input type="number" step="0.01" min="0" class="acct-line-dr" data-idx="' + i + '" placeholder="0.00" oninput="acctUpdateBalance()"></td>'
            + '<td><input type="number" step="0.01" min="0" class="acct-line-cr" data-idx="' + i + '" placeholder="0.00" oninput="acctUpdateBalance()"></td>'
            + '<td><input type="text" class="acct-line-memo" data-idx="' + i + '" placeholder="memo" maxlength="255"></td>'
            + '<td><button class="acct-btn" style="padding:2px 6px;" onclick="acctRemoveLine(' + i + ')">&times;</button></td>';
        document.getElementById('acct-lines-body').appendChild(row);
    };

    window.acctRemoveLine = function(i) {
        var row = document.getElementById('acct-line-' + i);
        if (row) { row.remove(); acctUpdateBalance(); }
    };

    window.acctOpenNewJournal = function() {
        _lineCount = 2;

        var body = '<div id="acct-je-err" class="modal-err"></div>'
            + '<div class="modal-row">'
            + '  <div class="modal-field">'
            + '    <label>entry date *</label>'
            + '    <input id="acct-je-date" type="date" value="' + esc(new Date().toISOString().substring(0, 10)) + '">'
            + '  </div>'
            + '  <div class="modal-field">'
            + '    <label>reference</label>'
            + '    <input id="acct-je-ref" type="text" placeholder="INV-001 / PO-123" maxlength="100">'
            + '  </div>'
            + '</div>'
            + '<div class="modal-field">'
            + '  <label>description *</label>'
            + '  <input id="acct-je-desc" type="text" placeholder="describe this transaction" maxlength="500">'
            + '</div>'
            + '<div class="modal-field">'
            + '  <label>created by</label>'
            + '  <input id="acct-je-by" type="text" placeholder="your name" maxlength="100">'
            + '</div>'
            + '<div style="margin-top:12px;border-top:1px solid var(--c-border);padding-top:12px;">'
            + '  <div style="font-size:9px;letter-spacing:0.1em;text-transform:uppercase;'
            + '       color:var(--c-text-faint);margin-bottom:8px;">line items</div>'
            + '  <div id="acct-modal-lines"><div class="skeleton-line"></div></div>'
            + '</div>';

        var footer = '<button class="acct-btn" onclick="acctCloseModal()">cancel</button>'
            + '<button class="acct-btn primary" onclick="acctSaveJournal()">save entry</button>';

        acctOpenModal('new journal entry', body, footer);

        // Load accounts if not cached
        if (_nacctCache.length) {
            acctRenderLines();
        } else {
            acctPost('accounting.accounts_list', { type: '' }, function(err, resp) {
                _nacctCache = (resp && resp.ok && resp.data) ? resp.data : [];
                acctRenderLines();
            });
        }
    };

    window.acctSaveJournal = function() {
        var date  = document.getElementById('acct-je-date').value;
        var desc  = document.getElementById('acct-je-desc').value.trim();
        var ref   = document.getElementById('acct-je-ref').value.trim();
        var by    = document.getElementById('acct-je-by').value.trim();
        var errEl = document.getElementById('acct-je-err');

        var lines = [];
        document.querySelectorAll('#acct-lines-body tr').forEach(function(row) {
            var acct = row.querySelector('.acct-line-acct');
            var dr   = row.querySelector('.acct-line-dr');
            var cr   = row.querySelector('.acct-line-cr');
            var memo = row.querySelector('.acct-line-memo');
            if (!acct || !acct.value) return; // skip empty rows
            lines.push({
                account_id:    parseInt(acct.value, 10),
                debit_amount:  parseFloat(dr.value)  || 0,
                credit_amount: parseFloat(cr.value)  || 0,
                memo:          memo ? memo.value.trim() : '',
            });
        });

        if (!date || !desc) {
            errEl.textContent = 'Entry date and description are required.';
            errEl.classList.add('show');
            return;
        }
        if (lines.length < 2) {
            errEl.textContent = 'Add at least two line items and select accounts.';
            errEl.classList.add('show');
            return;
        }

        acctPost('accounting.journal_create', {
            entry_date:  date,
            description: desc,
            reference:   ref,
            created_by:  by,
            lines:       lines,
        }, function(err, resp) {
            if (err || !resp || !resp.ok) {
                var msg = (resp && resp.error) ? resp.error : 'Failed to create entry.';
                errEl.textContent = msg;
                errEl.classList.add('show');
                console.error('[07-accounting] journal_create', err || resp);
                return;
            }
            acctCloseModal();
            acctMsg('Entry ' + resp.data.entry_number + ' saved.', 'ok');
            _jrn.page   = 0;
            _jrn.loaded = false;
            acctLoadJournal();
        });
    };

    // ---- register and boot --------------------------------------------------

    registerTool('07-accounting', function() {
        _jrn.loaded   = false;
        _trial.loaded = false;
        _per.loaded   = false;
        acctLoadAccounts();
        // Only reload visible tab
        var active = document.querySelector('#acct-wrap .acct-tab.active');
        if (active && active.dataset.tab === 'journal')  { acctLoadJournal(); }
        if (active && active.dataset.tab === 'trial')    { acctLoadTrial(); }
        if (active && active.dataset.tab === 'periods')  { acctLoadPeriods(); }
    });

    acctLoadAccounts();

})();
</script>
