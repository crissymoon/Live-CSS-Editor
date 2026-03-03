<?php
/**
 * pb_admin/dashboard.php
 * Main admin dashboard.  Tools are auto-discovered from the tools/ directory.
 * Each tool file sets: $tool_id, $tool_title, $tool_icon, $tool_cols (1|2|3)
 * and then outputs its card body HTML.
 *
 * To add a new tool: create a file in tools/ following the same pattern.
 */
require_once __DIR__ . '/auth.php';
require_auth();

$user = current_user();

// ── Discover and load tools ───────────────────────────────────────────────────
$toolsDir = __DIR__ . '/tools';
$tools    = [];

if (is_dir($toolsDir)) {
    $files = glob($toolsDir . '/*.php');
    sort($files);
    foreach ($files as $file) {
        // Skip template / private files (prefixed with _)
        if (strpos(basename($file), '_') === 0) continue;
        // Each tool file sets these variables and outputs its body HTML
        $tool_id   = basename($file, '.php');
        $tool_title = $tool_id;
        $tool_icon  = '&gt;';
        $tool_cols  = 1; // 1 = single column card, 2 = spans two, 3 = full width

        ob_start();
        include $file;
        $tool_html = ob_get_clean();

        $tools[] = [
            'id'    => $tool_id,
            'title' => $tool_title,
            'icon'  => $tool_icon,
            'cols'  => (int)$tool_cols,
            'html'  => $tool_html,
        ];
    }
}
?><!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?= APP_NAME ?> — Dashboard</title>
    <!-- No-flash theme init: read preference before first paint -->
    <script>
    (function() {
        try {
            var t = localStorage.getItem('pb_admin_theme');
            if (t === 'light' || t === 'dark') {
                document.documentElement.setAttribute('data-theme', t);
            }
        } catch(e) {
            console.warn('[pb_admin] theme init error:', e);
        }
    })();
    </script>
    <link rel="stylesheet" href="<?= ADMIN_URL_PATH ?>/admin.css">
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--c-bg);
            color: var(--c-text);
            font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
            min-height: 100vh;
        }

        /* ── Header ── */
        header {
            background: var(--c-bg-1);
            border-bottom: 1px solid var(--c-border-acc);
            padding: 0 24px;
            height: 52px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            position: sticky;
            top: 0;
            z-index: 50;
        }

        .header-left {
            display: flex;
            align-items: center;
            gap: 16px;
        }

        header h1 {
            font-size: 13px;
            font-weight: 600;
            letter-spacing: 0.08em;
            color: var(--c-text-2);
            white-space: nowrap;
        }

        header h1 span { color: var(--c-acc); }

        .dev-badge {
            font-size: 9px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--c-dev-badge);
            background: var(--c-dev-badge-bg);
            border: 1px solid var(--c-dev-badge-border);
            padding: 2px 6px;
        }

        .header-right {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .user-label {
            font-size: 11px;
            color: var(--c-text-faint);
            letter-spacing: 0.04em;
        }

        .user-label strong { color: var(--c-text-dim); font-weight: 600; }

        .btn {
            background: var(--c-acc-bg);
            border: 1px solid var(--c-border-acc);
            color: var(--c-text-dim);
            font-family: inherit;
            font-size: 11px;
            padding: 5px 12px;
            cursor: pointer;
            letter-spacing: 0.04em;
            text-decoration: none;
            display: inline-flex;
            align-items: center;
            gap: 5px;
            transition: background 0.15s, color 0.15s;
        }

        .btn:hover {
            background: var(--c-acc-bg2);
            color: var(--c-text);
        }

        .btn-danger {
            border-color: var(--c-err-border);
        }

        .btn-danger:hover {
            background: var(--c-err-bg);
            border-color: var(--c-err-border);
            color: var(--c-err);
        }

        /* ── Layout ── */
        .page-wrap {
            display: flex;
            min-height: calc(100vh - 52px);
        }

        /* ── Sidebar ── */
        nav.sidebar {
            width: 180px;
            flex-shrink: 0;
            background: var(--c-bg-2);
            border-right: 1px solid var(--c-border-2);
            padding: 20px 0;
        }

        .sidebar-section {
            padding: 0 16px;
            margin-bottom: 20px;
        }

        .sidebar-label {
            font-size: 9px;
            letter-spacing: 0.14em;
            text-transform: uppercase;
            color: var(--c-sidebar-label);
            margin-bottom: 8px;
            padding: 0 4px;
        }

        .sidebar-link {
            display: block;
            padding: 6px 8px;
            font-size: 11px;
            color: var(--c-sidebar-link);
            text-decoration: none;
            letter-spacing: 0.04em;
            transition: color 0.12s, background 0.12s;
        }

        .sidebar-link:hover,
        .sidebar-link.active {
            color: var(--c-text-1);
            background: var(--c-acc-bg2);
        }

        .sidebar-link.active {
            color: var(--c-text-2);
            border-left: 2px solid var(--c-acc);
            padding-left: 6px;
        }

        /* ── Main content ── */
        main {
            flex: 1;
            padding: 24px;
            min-width: 0;
        }

        .page-header {
            display: flex;
            align-items: baseline;
            justify-content: space-between;
            margin-bottom: 20px;
            gap: 12px;
        }

        .page-title {
            font-size: 11px;
            letter-spacing: 0.12em;
            text-transform: uppercase;
            color: var(--c-text-faint);
        }

        .page-subtitle {
            font-size: 10px;
            color: var(--c-text-ghost);
            margin-top: 4px;
        }

        /* ── Tool grid ── */
        .tool-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 14px;
        }

        .tool-card {
            background: var(--c-bg-1);
            border: 1px solid var(--c-border);
            display: flex;
            flex-direction: column;
            min-height: 180px;
            transition: border-color 0.15s;
        }

        .tool-card:hover {
            border-color: var(--c-acc-border2);
        }

        .tool-card.cols-2 { grid-column: span 2; }
        .tool-card.cols-3 { grid-column: span 3; }

        .tool-card-header {
            background: var(--c-bg-3);
            border-bottom: 1px solid var(--c-border-2);
            padding: 9px 14px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 8px;
            flex-shrink: 0;
        }

        .tool-card-title {
            font-size: 10px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--c-text-muted);
            display: flex;
            align-items: center;
            gap: 7px;
        }

        .tool-card-icon {
            color: var(--c-acc);
            font-size: 11px;
        }

        .tool-card-controls {
            display: flex;
            gap: 5px;
        }

        .tool-ctrl-btn {
            background: none;
            border: none;
            color: var(--c-ctrl-btn);
            font-family: inherit;
            font-size: 10px;
            cursor: pointer;
            padding: 2px 5px;
            transition: color 0.12s;
        }

        .tool-ctrl-btn:hover { color: var(--c-text-dim); }

        .tool-card-body {
            padding: 14px;
            flex: 1;
            overflow: auto;
        }

        /* ── Empty tool grid ── */
        .no-tools {
            grid-column: 1 / -1;
            color: var(--c-text-ghost);
            font-size: 12px;
            padding: 40px 0;
            text-align: center;
        }

        /* ── Loading skeleton ── */
        .skeleton-line {
            height: 10px;
            background: var(--c-skeleton);
            border-radius: 2px;
            margin-bottom: 8px;
            animation: pulse 1.6s ease-in-out infinite;
        }

        .skeleton-line:nth-child(2) { width: 80%; }
        .skeleton-line:nth-child(3) { width: 60%; }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50%       { opacity: 0.4; }
        }

        /* ── Status dot ── */
        .status-dot {
            display: inline-block;
            width: 7px;
            height: 7px;
            border-radius: 50%;
            background: var(--c-ctrl-btn);
            vertical-align: middle;
            margin-right: 4px;
        }
        .status-dot.online  { background: var(--c-ok); }
        .status-dot.offline { background: var(--c-err); }
        .status-dot.pending { background: var(--c-warn); animation: pulse 1s ease-in-out infinite; }

        /* ── Tables shared across tools ── */
        .data-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 11px;
        }

        .data-table th {
            font-size: 9px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--c-ctrl-btn);
            text-align: left;
            padding: 6px 8px 8px;
            border-bottom: 1px solid var(--c-border);
        }

        .data-table td {
            padding: 7px 8px;
            color: var(--c-text-dim);
            border-bottom: 1px solid var(--c-border-3);
            vertical-align: top;
        }

        .data-table tr:last-child td { border-bottom: none; }

        .data-table td.highlight { color: var(--c-text-1); }

        .badge {
            display: inline-block;
            padding: 1px 6px;
            font-size: 9px;
            letter-spacing: 0.06em;
            background: var(--c-acc-bg);
            border: 1px solid var(--c-border-acc2);
            color: var(--c-text-muted);
        }

        .badge.badge-ok     { background: var(--c-ok-bg);   border-color: var(--c-ok-border);   color: var(--c-ok); }
        .badge.badge-err    { background: var(--c-err-bg);  border-color: var(--c-err-border);  color: var(--c-err); }
        .badge.badge-warn   { background: var(--c-warn-bg); border-color: var(--c-warn-border); color: var(--c-warn); }
        .badge.badge-admin  { background: var(--c-acc-bg);  border-color: var(--c-border-acc);  color: var(--c-text-dim); }

        .err-msg {
            color: var(--c-err);
            font-size: 11px;
        }

        .empty-msg {
            color: var(--c-ctrl-btn);
            font-size: 11px;
        }

        /* ── Footer ── */
        footer {
            border-top: 1px solid var(--c-border-2);
            padding: 10px 24px;
            font-size: 10px;
            color: var(--c-text-deep);
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
    </style>
</head>
<body>

<script>
/* -- Shared globals declared here so tool <script> blocks that run during
   HTML parsing can call registerTool() and apiFetch() safely. ---------- */
var toolRefreshHandlers = {};

function registerTool(id, fn) {
    toolRefreshHandlers[id] = fn;
}

function refreshTool(id) {
    var handler = toolRefreshHandlers[id];
    if (typeof handler === 'function') {
        console.log('[pb_admin/dashboard] refreshing tool:', id);
        handler();
    } else {
        console.warn('[pb_admin/dashboard] no refresh handler registered for tool:', id);
    }
}

function apiFetch(action, params, cb) {
    var qs = Object.keys(params || {}).map(function(k) {
        return encodeURIComponent(k) + '=' + encodeURIComponent(params[k]);
    }).join('&');
    var url = 'api_proxy.php?action=' + encodeURIComponent(action) + (qs ? '&' + qs : '');
    console.log('[pb_admin] apiFetch:', url);
    fetch(url)
        .then(function(r) { return r.json(); })
        .then(function(d) { cb(null, d); })
        .catch(function(e) {
            console.error('[pb_admin] apiFetch error:', action, e);
            cb(e, null);
        });
}
</script>

<header>
    <div class="header-left">
        <h1><span>&gt;</span> <?= htmlspecialchars(APP_NAME) ?></h1>
        <?php if (DEV_MODE): ?>
            <span class="dev-badge">dev mode</span>
        <?php endif; ?>
    </div>
    <div class="header-right">
        <span class="user-label">
            signed in as <strong><?= htmlspecialchars($user['username'] ?? 'unknown') ?></strong>
            <?php if (($user['role'] ?? '') === 'admin'): ?>
                <span class="badge badge-admin">admin</span>
            <?php endif; ?>
        </span>
        <button class="theme-toggle" id="themeToggle" title="toggle light/dark mode">light</button>
        <a href="<?= ADMIN_URL_PATH ?>/logout.php" class="btn btn-danger" onclick="return confirm('Log out?')">logout</a>
    </div>
</header>

<div class="page-wrap">

    <nav class="sidebar">
        <div class="sidebar-section">
            <div class="sidebar-label">navigate</div>
            <a href="dashboard.php" class="sidebar-link active">dashboard</a>
        </div>
        <div class="sidebar-section">
            <div class="sidebar-label">tools</div>
            <?php foreach ($tools as $t): ?>
                <a href="#tool-<?= htmlspecialchars($t['id']) ?>" class="sidebar-link">
                    <?= htmlspecialchars($t['title']) ?>
                </a>
            <?php endforeach; ?>
        </div>
        <div class="sidebar-section">
            <div class="sidebar-label">dev</div>
            <a href="../page-builder/index.php" class="sidebar-link">page builder</a>
            <a href="../index.php" class="sidebar-link">style tool</a>
        </div>
    </nav>

    <main>
        <div class="page-header">
            <div>
                <div class="page-title">dashboard</div>
                <div class="page-subtitle"><?= count($tools) ?> tool<?= count($tools) !== 1 ? 's' : '' ?> loaded</div>
            </div>
        </div>

        <div class="tool-grid" id="toolGrid">
            <?php if (empty($tools)): ?>
                <div class="no-tools">
                    No tools found in <code>tools/</code>. Add a PHP file to get started.
                </div>
            <?php else: ?>
                <?php foreach ($tools as $t): ?>
                    <div class="tool-card cols-<?= $t['cols'] ?>" id="tool-<?= htmlspecialchars($t['id']) ?>">
                        <div class="tool-card-header">
                            <div class="tool-card-title">
                                <span class="tool-card-icon"><?= $t['icon'] ?></span>
                                <?= htmlspecialchars($t['title']) ?>
                            </div>
                            <div class="tool-card-controls">
                                <button class="tool-ctrl-btn" title="refresh" onclick="refreshTool('<?= htmlspecialchars($t['id']) ?>')">&#8635;</button>
                            </div>
                        </div>
                        <div class="tool-card-body" id="tool-body-<?= htmlspecialchars($t['id']) ?>">
                            <?= $t['html'] ?>
                        </div>
                    </div>
                <?php endforeach; ?>
            <?php endif; ?>
        </div>
    </main>

</div>

<footer>
    <span><?= htmlspecialchars(APP_NAME) ?> &mdash; <?= htmlspecialchars(XCMAUTH_BASE_URL) ?></span>
    <span id="clockDisplay"></span>
</footer>

<script>
/* ── Theme toggle ───────────────────────────────────────────────────────── */
(function() {
    var btn = document.getElementById('themeToggle');
    if (!btn) { console.warn('[pb_admin/dashboard] themeToggle button not found'); return; }

    function getTheme() {
        try { return localStorage.getItem('pb_admin_theme') || 'dark'; }
        catch(e) { console.error('[pb_admin/dashboard] localStorage read error:', e); return 'dark'; }
    }

    function applyTheme(t) {
        document.documentElement.setAttribute('data-theme', t);
        btn.textContent = (t === 'dark') ? 'light' : 'dark';
        btn.title = 'switch to ' + ((t === 'dark') ? 'light' : 'dark') + ' mode';
    }

    function saveTheme(t) {
        try { localStorage.setItem('pb_admin_theme', t); }
        catch(e) { console.error('[pb_admin/dashboard] localStorage write error:', e); }
    }

    btn.addEventListener('click', function() {
        var next = getTheme() === 'dark' ? 'light' : 'dark';
        console.log('[pb_admin/dashboard] theme toggle:', next);
        applyTheme(next);
        saveTheme(next);
    });

    // Sync button label to current theme on load
    applyTheme(getTheme());
})();

/* ── Clock ───────────────────────────────────────────────────────────────── */
function updateClock() {
    var el = document.getElementById('clockDisplay');
    if (el) el.textContent = new Date().toISOString().replace('T', ' ').slice(0, 19) + ' UTC';
}
updateClock();
setInterval(updateClock, 1000);
</script>

</body>
</html>
