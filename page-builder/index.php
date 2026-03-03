<?php
/**
 * page-builder/index.php
 * Launcher UI for the Page Builder — lists all pages/ subdirectories.
 */

$pagesDir = __DIR__ . '/pages';

// Scan page directories
$pages = [];
if (is_dir($pagesDir)) {
    foreach (scandir($pagesDir) as $name) {
        if ($name === '.' || $name === '..') continue;
        $pageDir = $pagesDir . '/' . $name;
        if (!is_dir($pageDir)) continue;

        $builtAt  = null;
        $htmlFile = $pageDir . '/index.html';
        if (file_exists($htmlFile)) {
            $builtAt = date('Y-m-d H:i', filemtime($htmlFile));
        }

        $overrides     = [];
        $overridesFile = $pageDir . '/overrides.json';
        if (file_exists($overridesFile)) {
            $overrides = json_decode(file_get_contents($overridesFile), true) ?: [];
        }

                // Read block flag
        $blocked    = false;
        $flagsFile  = $pageDir . '/.flags.json';
        if (file_exists($flagsFile)) {
            $flagsRaw = file_get_contents($flagsFile);
            if ($flagsRaw !== false) {
                $flagsData = json_decode($flagsRaw, true);
                if (is_array($flagsData) && !empty($flagsData['blocked'])) {
                    $blocked = true;
                }
            }
        }

        $pages[] = [
            'name'          => $name,
            'built'         => $builtAt,
            'overrideCount' => count($overrides),
            'blocked'       => $blocked,
        ];
    }
}

// Sort alphabetically
usort($pages, fn($a, $b) => strcasecmp($a['name'], $b['name']));

// Read last deploy analytics
$lastDeploy = null;
$deployLog  = __DIR__ . '/deploy/build-log.json';
if (file_exists($deployLog)) {
    $logRaw = file_get_contents($deployLog);
    if ($logRaw !== false) {
        $logData = json_decode($logRaw, true);
        if (is_array($logData)) {
            $lastDeploy = $logData;
        }
    }
}

// Read project config (index page + slug map)
$projectConfig    = ['index_page' => '', 'slugs' => []];
$projectConfigFile = __DIR__ . '/project.json';
if (file_exists($projectConfigFile)) {
    $pcRaw = file_get_contents($projectConfigFile);
    if ($pcRaw !== false) {
        $pcData = json_decode($pcRaw, true);
        if (is_array($pcData)) $projectConfig = $pcData;
    }
}
$projectConfig['slugs'] = is_array($projectConfig['slugs'] ?? null) ? $projectConfig['slugs'] : [];
?><!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Page Builder</title>
    <script>
    (function(){
        try{
            var t=localStorage.getItem('pb_theme');
            if(t==='light'||t==='dark') document.documentElement.setAttribute('data-theme',t);
        }catch(e){}
    })();
    </script>
    <link rel="stylesheet" href="css/pb-theme.css">
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--pb-bg);
            color: var(--pb-text);
            font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
            min-height: 100vh;
        }

        /* ---- Header ---- */
        header {
            background: var(--pb-bg-2);
            border-bottom: 1px solid var(--pb-border-acc);
            padding: 0 28px;
            height: 52px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            position: sticky;
            top: 0;
            z-index: 10;
        }

        header h1 {
            font-size: 14px;
            font-weight: 600;
            letter-spacing: 0.08em;
            color: var(--pb-text-dim);
        }

        header h1 span {
            color: var(--pb-acc);
        }

        .header-actions {
            display: flex;
            gap: 8px;
        }

        .btn {
            background: var(--pb-acc-bg);
            border: 1px solid var(--pb-border-acc);
            color: var(--pb-text-dim);
            font-family: inherit;
            font-size: 12px;
            padding: 6px 14px;
            cursor: pointer;
            letter-spacing: 0.04em;
            text-decoration: none;
            display: inline-flex;
            align-items: center;
            gap: 5px;
            transition: background 0.15s, color 0.15s, border-color 0.15s;
        }

        .btn:hover {
            background: var(--pb-acc-bg2);
            border-color: var(--pb-border-acc2);
            color: var(--pb-text);
        }

        .btn-primary {
            background: var(--pb-acc-bg2);
            border-color: var(--pb-border-acc2);
            color: var(--pb-acc-text);
        }

        .btn-blocked {
            border-color: rgba(239,68,68,0.4);
            color: #ef4444;
        }

        .btn-blocked:hover {
            background: rgba(239,68,68,0.1);
            border-color: rgba(239,68,68,0.7);
        }

        /* ---- Main ---- */
        main {
            padding: 28px;
            max-width: 960px;
            margin: 0 auto;
        }

        .section-title {
            font-size: 11px;
            letter-spacing: 0.12em;
            text-transform: uppercase;
            color: var(--pb-text-faint);
            margin-bottom: 16px;
        }

        /* ---- Project Settings Panel ---- */
        .settings-panel {
            background: var(--pb-bg-2);
            border: 1px solid var(--pb-border);
            border-radius: 8px;
            padding: 20px 24px;
            margin-bottom: 28px;
        }
        .settings-panel .section-title { margin-bottom: 14px; }
        .settings-row {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 14px;
        }
        .settings-row label {
            font-size: 12px;
            color: var(--pb-text-faint);
            min-width: 90px;
        }
        .settings-row select {
            background: var(--pb-bg-1);
            border: 1px solid var(--pb-border);
            color: var(--pb-text);
            padding: 5px 10px;
            border-radius: 5px;
            font-size: 13px;
        }
        .slug-table {
            width: 100%;
            margin-bottom: 14px;
            border-collapse: collapse;
        }
        .slug-table-header, .slug-row {
            display: grid;
            grid-template-columns: 160px 1fr;
            gap: 10px;
            align-items: center;
            padding: 4px 0;
        }
        .slug-table-header {
            font-size: 10px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--pb-text-faint);
            border-bottom: 1px solid var(--pb-border);
            margin-bottom: 4px;
            padding-bottom: 6px;
        }
        .slug-page-name {
            font-size: 13px;
            font-family: monospace;
            color: var(--pb-text);
        }
        .slug-input {
            background: var(--pb-bg-1);
            border: 1px solid var(--pb-border);
            color: var(--pb-text);
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 13px;
            font-family: monospace;
            width: 100%;
            box-sizing: border-box;
        }
        .slug-input:focus { outline: none; border-color: var(--pb-accent); }
        #pb-settings-msg {
            font-size: 12px;
            margin-left: 10px;
        }
        #pb-settings-msg.ok  { color: #4ade80; }
        #pb-settings-msg.err { color: #f87171; }

        /* ---- Landing + Rename ---- */
        .page-card-header {
            display: flex;
            align-items: center;
            gap: 8px;
            flex-wrap: wrap;
        }
        .badge.badge-landing {
            background: rgba(250,204,21,0.12);
            border-color: rgba(250,204,21,0.4);
            color: #fbbf24;
        }
        .btn-rename-icon {
            margin-left: auto;
            font-size: 10px;
            padding: 2px 7px;
            opacity: 0.45;
            cursor: pointer;
        }
        .btn-rename-icon:hover { opacity: 1; }
        .page-rename-row {
            display: flex;
            gap: 6px;
            align-items: center;
        }
        .page-rename-input {
            flex: 1;
            background: var(--pb-bg-1);
            border: 1px solid var(--pb-border-acc);
            color: var(--pb-text);
            padding: 4px 8px;
            border-radius: 4px;
            font-family: monospace;
            font-size: 12px;
            min-width: 0;
        }
        .page-rename-input:focus { outline: none; border-color: var(--pb-accent, #6366f1); }
        .btn-landing-set {
            font-size: 10px;
            opacity: 0.75;
        }
        .btn-landing-set:hover { opacity: 1; }

        /* ---- Page Grid ---- */
        .page-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
            gap: 14px;
        }

        .page-card {
            background: var(--pb-bg-2);
            border: 1px solid var(--pb-border);
            padding: 18px;
            display: flex;
            flex-direction: column;
            gap: 12px;
            transition: border-color 0.15s;
        }

        .page-card:hover {
            border-color: var(--pb-border-acc);
        }

        .page-card-name {
            font-size: 14px;
            font-weight: 600;
            color: var(--pb-acc-text);
            letter-spacing: 0.04em;
        }

        .page-card-meta {
            font-size: 11px;
            color: var(--pb-text-faint);
            display: flex;
            flex-direction: column;
            gap: 3px;
        }

        .page-card-meta .meta-line {
            display: flex;
            gap: 6px;
        }

        .meta-key   { color: var(--pb-text-faint); }
        .meta-value { color: var(--pb-text-muted); }

        .badge {
            display: inline-block;
            padding: 2px 7px;
            font-size: 10px;
            letter-spacing: 0.05em;
            background: var(--pb-acc-bg);
            border: 1px solid var(--pb-border-acc);
            color: var(--pb-text-muted);
        }

        .badge.badge-built {
            background: var(--pb-ok-bg);
            border-color: var(--pb-ok-border);
            color: var(--pb-ok);
        }

        .badge.badge-unbuilt {
            background: var(--pb-err-bg);
            border-color: var(--pb-err-border);
            color: var(--pb-err);
        }

        .badge.badge-blocked {
            background: rgba(239,68,68,0.1);
            border-color: rgba(239,68,68,0.35);
            color: #ef4444;
        }

        .badge.badge-ok {
            background: var(--pb-ok-bg);
            border-color: var(--pb-ok-border);
            color: var(--pb-ok);
        }

        /* ---- Deploy bar ---- */
        .deploy-bar {
            background: var(--pb-bg-2);
            border: 1px solid var(--pb-border);
            padding: 12px 16px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 14px;
            flex-wrap: wrap;
        }

        .deploy-bar-meta {
            font-size: 11px;
            color: var(--pb-text-faint);
            display: flex;
            gap: 16px;
            flex-wrap: wrap;
        }

        .deploy-bar-meta span { color: var(--pb-text-muted); }

        /* ---- Stage progress panel ---- */
        .stage-panel {
            display: none;
            background: var(--pb-bg-2);
            border: 1px solid var(--pb-border-acc);
            padding: 18px;
            margin-bottom: 20px;
        }

        .stage-panel.visible { display: block; }

        .stage-panel h3 {
            font-size: 11px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--pb-text-faint);
            margin-bottom: 12px;
        }

        .stage-log {
            font-size: 11px;
            color: var(--pb-text-muted);
            line-height: 1.8;
            max-height: 200px;
            overflow-y: auto;
        }

        .stage-log .log-ok   { color: var(--pb-ok); }
        .stage-log .log-err  { color: var(--pb-err); }
        .stage-log .log-skip { color: #5555a0; }
        .stage-log .log-info { color: var(--pb-text-faint); }

        .page-card-actions {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 6px;
        }

        .page-card-actions .btn {
            font-size: 11px;
            padding: 5px 10px;
            justify-content: center;
        }

        /* Full-width actions that should not share a row */
        .page-card-actions .btn-blocked,
        .page-card-actions .btn-block-toggle,
        .page-card-actions .btn-landing-set {
            grid-column: 1 / -1;
        }

        /* ---- Empty state ---- */
        .empty-state {
            color: var(--pb-text-faint);
            font-size: 13px;
            padding: 32px 0;
        }

        /* ---- Quick create form ---- */
        .create-section {
            margin-top: 36px;
        }

        .create-form {
            display: flex;
            align-items: center;
            gap: 10px;
            background: var(--pb-bg-2);
            border: 1px solid var(--pb-border);
            padding: 14px 16px;
        }

        .create-form input[type="text"] {
            background: var(--pb-bg-input);
            border: 1px solid var(--pb-border-2);
            color: var(--pb-text);
            font-family: inherit;
            font-size: 12px;
            padding: 7px 10px;
            outline: none;
            flex: 1;
            transition: border-color 0.15s;
        }

        .create-form input[type="text"]:focus {
            border-color: var(--pb-border-acc2);
        }

        .create-form input[type="text"]::placeholder {
            color: var(--pb-text-faint);
        }

        /* ---- Back link ---- */
        .back-link {
            color: var(--pb-text-faint);
            font-size: 11px;
            text-decoration: none;
            letter-spacing: 0.06em;
        }

        .back-link:hover { color: var(--pb-text-muted); }

        /* ---- Inline message ---- */
        #msg {
            font-size: 11px;
            padding: 5px 10px;
            display: none;
        }

        #msg.ok  { color: var(--pb-ok);  background: var(--pb-ok-bg);  border: 1px solid var(--pb-ok-border);  }
        #msg.err { color: var(--pb-err); background: var(--pb-err-bg); border: 1px solid var(--pb-err-border); }
    </style>
</head>
<body>

<header>
    <h1><span>&gt;</span> page-builder</h1>
    <div class="header-actions">
        <button class="btn btn-primary" id="pb-stage-btn" onclick="stageAll()">stage all</button>
        <button class="btn" id="pb-idx-theme-btn" onclick="toggleTheme()">light mode</button>
        <a href="/pb_admin/dashboard.php" class="back-link">admin dashboard</a>
        <a href="../index.php" class="back-link">style tool</a>
    </div>
</header>

<main>
    <?php if ($lastDeploy): ?>
    <div class="deploy-bar">
        <div class="deploy-bar-meta">
            <div>last deploy <span><?= htmlspecialchars($lastDeploy['last_deploy'] ?? '?') ?></span></div>
            <div>staged <span><?= (int)($lastDeploy['pages_staged'] ?? 0) ?></span> &middot; skipped <span><?= (int)($lastDeploy['pages_skipped'] ?? 0) ?></span></div>
            <div>size <span><?= number_format((int)($lastDeploy['total_bytes_min'] ?? 0)) ?> B</span> (<?= $lastDeploy['total_saved_pct'] ?? '0' ?>% saved)</div>
        </div>
        <a href="deploy/" class="btn" target="_blank">view staged site</a>
    </div>
    <?php endif; ?>

    <!-- Stage progress panel -->
    <div class="stage-panel" id="pb-stage-panel">
        <h3>staging</h3>
        <div class="stage-log" id="pb-stage-log"></div>
    </div>

    <!-- Project Settings -->
    <div class="settings-panel" id="pb-settings-panel">
        <p class="section-title">project settings</p>

        <div class="settings-row">
            <label>index page</label>
            <select id="pb-index-page-select">
                <option value="">(none - nav page)</option>
                <?php foreach ($pages as $p): ?>
                <option value="<?= htmlspecialchars($p['name'], ENT_QUOTES) ?>"
                    <?= ($projectConfig['index_page'] ?? '') === $p['name'] ? 'selected' : '' ?>>
                    <?= htmlspecialchars($p['name']) ?>
                </option>
                <?php endforeach; ?>
            </select>
        </div>

        <div class="slug-table" id="pb-slug-table">
            <div class="slug-table-header">
                <span>page</span>
                <span>slug / path</span>
            </div>
            <?php foreach ($pages as $p): ?>
            <div class="slug-row">
                <span class="slug-page-name"><?= htmlspecialchars($p['name']) ?></span>
                <input
                    type="text"
                    class="slug-input"
                    data-page="<?= htmlspecialchars($p['name'], ENT_QUOTES) ?>"
                    value="<?= htmlspecialchars($projectConfig['slugs'][$p['name']] ?? '', ENT_QUOTES) ?>"
                    placeholder="/<?= htmlspecialchars($p['name']) ?>/">
            </div>
            <?php endforeach; ?>
        </div>

        <button class="btn btn-primary" onclick="saveProjectConfig()">save settings</button>
        <span id="pb-settings-msg"></span>
    </div>

    <p class="section-title">pages</p>

    <?php if (empty($pages)): ?>
        <p class="empty-state">No pages found in <code>pages/</code>. Create a subdirectory and add JSON config files.</p>
    <?php else: ?>
    <div class="page-grid">
        <?php foreach ($pages as $page): ?>
        <div class="page-card" data-page-name="<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>">
            <div class="page-card-header">
                <span class="page-card-name"><?= htmlspecialchars($page['name']) ?></span>
                <?php if (($projectConfig['index_page'] ?? '') === $page['name']): ?>
                <span class="badge badge-landing">landing</span>
                <?php endif; ?>
                <button
                    class="btn btn-rename-icon"
                    title="rename page"
                    onclick="startRename(this.closest('.page-card'), '<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>')">
                    rename
                </button>
            </div>
            <div class="page-rename-row" style="display:none;">
                <input type="text" class="page-rename-input" value="" maxlength="64" placeholder="new-name">
                <button class="btn btn-primary" onclick="confirmRename(this.closest('.page-card'), '<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>')">ok</button>
                <button class="btn" onclick="cancelRename(this.closest('.page-card'))">cancel</button>
            </div>

            <div class="page-card-meta">
                <div class="meta-line">
                    <span class="meta-key">built</span>
                    <span class="meta-value">
                        <?php if ($page['built']): ?>
                            <span class="badge badge-built"><?= htmlspecialchars($page['built']) ?></span>
                        <?php else: ?>
                            <span class="badge badge-unbuilt">not built</span>
                        <?php endif; ?>
                    </span>
                </div>
                <div class="meta-line">
                    <span class="meta-key">overrides</span>
                    <span class="meta-value">
                        <span class="badge"><?= $page['overrideCount'] ?></span>
                    </span>
                </div>
                <div class="meta-line">
                    <span class="meta-key">staging</span>
                    <span class="meta-value">
                        <?php if ($page['blocked']): ?>
                            <span class="badge badge-blocked">blocked</span>
                        <?php else: ?>
                            <span class="badge badge-ok">allowed</span>
                        <?php endif; ?>
                    </span>
                </div>
            </div>

            <div class="page-card-actions">
                <a href="composer.php?page=<?= urlencode($page['name']) ?>" class="btn btn-primary">
                    compose
                </a>
                <a href="watcher.php?page=<?= urlencode($page['name']) ?>" class="btn">
                    live edit
                </a>
                <button
                    class="btn"
                    onclick="buildPage('<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>', this)">
                    rebuild
                </button>
                <a href="watcher.php?page=<?= urlencode($page['name']) ?>&reset=1"
                   class="btn"
                   onclick="return confirm('Reset all overrides for &quot;<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>&quot;?')">
                    reset
                </a>
                <button
                    class="btn <?= $page['blocked'] ? 'btn-blocked' : 'btn-block-toggle' ?>"
                    data-page="<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>"
                    data-blocked="<?= $page['blocked'] ? 'true' : 'false' ?>"
                    onclick="toggleBlock(this)">
                    <?= $page['blocked'] ? 'unblock staging' : 'block staging' ?>
                </button>
                <?php if (($projectConfig['index_page'] ?? '') !== $page['name']): ?>
                <button
                    class="btn btn-landing-set"
                    onclick="setLandingPage('<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>')">
                    set as landing
                </button>
                <?php endif; ?>
            </div>
        </div>
        <?php endforeach; ?>
    </div>
    <?php endif; ?>

    <!-- Quick create -->
    <div class="create-section">
        <p class="section-title">new page</p>
        <div class="create-form">
            <input type="text" id="newPageName" placeholder="page-name (lowercase, hyphens)" maxlength="64">
            <button class="btn btn-primary" onclick="createPage()">+ create</button>
            <span id="msg"></span>
        </div>
    </div>
</main>

<script>
function showMsg(text, ok) {
    var m = document.getElementById('msg');
    if (!m) { console.error('[pb-index] msg element not found'); return; }
    m.textContent = text;
    m.className = ok ? 'ok' : 'err';
    m.style.display = 'inline-block';
    setTimeout(function() { m.style.display = 'none'; }, 3000);
}

// ---- Staging token (fetched once on load) ----
var PB_STAGE_TOKEN = null;

(function fetchToken() {
    try {
        fetch('stage.php')
            .then(function(r) {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(function(d) {
                if (d && d.token) {
                    PB_STAGE_TOKEN = d.token;
                    console.log('[pb-index] stage token loaded');
                } else {
                    console.error('[pb-index] stage.php did not return a token:', d);
                }
            })
            .catch(function(e) {
                console.error('[pb-index] failed to fetch stage token:', e);
            });
    } catch(e) {
        console.error('[pb-index] fetchToken error:', e);
    }
})();

function logStage(msg, cls) {
    try {
        var log = document.getElementById('pb-stage-log');
        if (!log) { console.warn('[pb-index] pb-stage-log element missing'); return; }
        var line = document.createElement('div');
        line.className = cls || '';
        line.textContent = msg;
        log.appendChild(line);
        log.scrollTop = log.scrollHeight;
    } catch(e) {
        console.error('[pb-index] logStage error:', e);
    }
}

function stageAll() {
    if (!PB_STAGE_TOKEN) {
        showMsg('Stage token not loaded yet - try again in a moment', false);
        console.error('[pb-index] stageAll: no token available yet');
        return;
    }
    var btn   = document.getElementById('pb-stage-btn');
    var panel = document.getElementById('pb-stage-panel');
    var log   = document.getElementById('pb-stage-log');
    if (!btn || !panel || !log) {
        console.error('[pb-index] stageAll: required elements not found');
        return;
    }
    btn.textContent = 'staging...';
    btn.disabled    = true;
    log.innerHTML   = '';
    panel.classList.add('visible');
    logStage('starting deploy...', 'log-info');
    console.log('[pb-index] staging all pages');

    fetch('stage.php', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + PB_STAGE_TOKEN },
        body:    JSON.stringify({ token: PB_STAGE_TOKEN }),
    })
        .then(function(r) {
            if (!r.ok) return r.json().then(function(e) { throw new Error(e.error || 'HTTP ' + r.status); });
            return r.json();
        })
        .then(function(d) {
            if (!d.ok) throw new Error(d.error || 'unknown error');
            logStage('staged ' + d.pages_staged + ' page(s)', 'log-ok');
            if (d.pages_skipped > 0) {
                logStage('skipped (blocked): ' + d.skipped.join(', '), 'log-skip');
            }
            if (d.pages_errored > 0) {
                d.errors.forEach(function(e) { logStage('ERROR: ' + e, 'log-err'); });
            }
            d.staged.forEach(function(p) {
                logStage(
                    '  ' + p.name + ' - ' + p.bytes_min + 'B (' + p.saved_pct + '% smaller)',
                    'log-ok'
                );
            });
            logStage('done in ' + d.elapsed_sec + 's - ' + (d.total_saved_pct||0) + '% overall size reduction', 'log-info');
            showMsg('Staged ' + d.pages_staged + ' pages', true);
            console.log('[pb-index] staging result:', d);
            setTimeout(function() { location.reload(); }, 1200);
        })
        .catch(function(e) {
            logStage('FAILED: ' + e.message, 'log-err');
            showMsg('Stage error: ' + e.message, false);
            console.error('[pb-index] stageAll error:', e);
        })
        .finally(function() {
            btn.textContent = 'stage all';
            btn.disabled    = false;
        });
}

function toggleBlock(btn) {
    if (!btn) { console.error('[pb-index] toggleBlock: no button element'); return; }
    var page    = btn.dataset.page;
    var blocked = btn.dataset.blocked === 'true';
    var newVal  = !blocked;
    if (!page) { console.error('[pb-index] toggleBlock: missing data-page on', btn); return; }
    console.log('[pb-index] toggling block for:', page, '->', newVal);
    btn.disabled    = true;
    btn.textContent = '...';

    fetch('flag-page.php', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify({ page: page, blocked: newVal }),
    })
        .then(function(r) { return r.json(); })
        .then(function(d) {
            if (!d.ok) throw new Error(d.error || 'unknown error');
            btn.dataset.blocked = newVal ? 'true' : 'false';
            btn.textContent     = newVal ? 'unblock staging' : 'block staging';
            console.log('[pb-index] block toggled for', page, ':', newVal);
            // Update the staging badge in the same card
            try {
                var card  = btn.closest('.page-card');
                var badge = card ? card.querySelector('.badge.badge-blocked, .badge.badge-ok') : null;
                if (badge) {
                    badge.textContent  = newVal ? 'blocked' : 'allowed';
                    badge.className    = 'badge ' + (newVal ? 'badge-blocked' : 'badge-ok');
                }
                if (newVal) btn.classList.add('btn-blocked');
                else btn.classList.remove('btn-blocked');
            } catch(ue) {
                console.warn('[pb-index] badge update error (non-critical):', ue);
            }
        })
        .catch(function(e) {
            showMsg('Block toggle error: ' + e.message, false);
            console.error('[pb-index] toggleBlock error for', page, ':', e);
            btn.textContent = newVal ? 'block staging' : 'unblock staging';
        })
        .finally(function() {
            btn.disabled = false;
        });
}

function toggleTheme() {
    var html = document.documentElement;
    var next = (html.getAttribute('data-theme') || 'dark') === 'dark' ? 'light' : 'dark';
    html.setAttribute('data-theme', next);
    try { localStorage.setItem('pb_theme', next); } catch(e) { console.warn('[pb-index] localStorage write failed:', e); }
    var btn = document.getElementById('pb-idx-theme-btn');
    if (btn) btn.textContent = next === 'dark' ? 'light mode' : 'dark mode';
    console.log('[pb-index] theme set to:', next);
}

(function() {
    try {
        var t = localStorage.getItem('pb_theme') || 'dark';
        var btn = document.getElementById('pb-idx-theme-btn');
        if (btn) btn.textContent = t === 'dark' ? 'light mode' : 'dark mode';
    } catch(e) { console.warn('[pb-index] localStorage read failed:', e); }
})();

function buildPage(name, btn) {
    var orig = btn.textContent;
    btn.textContent = 'building...';
    btn.disabled = true;
    console.log('[pb-index] building page:', name);
    fetch('build.php?page=' + encodeURIComponent(name))
        .then(function(r) { return r.json(); })
        .then(function(d) {
            if (d.ok) {
                showMsg('Built "' + name + '"', true);
                setTimeout(function() { location.reload(); }, 800);
            } else {
                console.error('[pb-index] build error:', d);
                showMsg('Error: ' + (d.error || '?'), false);
                btn.textContent = orig;
                btn.disabled = false;
            }
        })
        .catch(function(e) {
            console.error('[pb-index] build network error:', e);
            showMsg('Network error', false);
            btn.textContent = orig;
            btn.disabled = false;
        });
}

function createPage() {
    var input = document.getElementById('newPageName');
    var name  = input.value.trim().toLowerCase().replace(/[^a-z0-9-_]/g, '-');
    if (!name) { showMsg('Enter a page name', false); return; }
    console.log('[pb-index] creating page:', name);

    fetch('create-page.php', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify({ name: name }),
    })
        .then(function(r) { return r.json(); })
        .then(function(d) {
            if (d.ok) {
                showMsg('Created "' + name + '" - building...', true);
                input.value = '';
                fetch('build.php?page=' + encodeURIComponent(name))
                    .catch(function(e) { console.error('[pb-index] auto-build error:', e); })
                    .finally(function() { setTimeout(function() { location.reload(); }, 600); });
            } else {
                console.error('[pb-index] create-page error:', d);
                showMsg('Error: ' + (d.error || '?'), false);
            }
        })
        .catch(function(e) {
            console.error('[pb-index] create-page network error:', e);
            showMsg('Network error', false);
        });
}

function saveProjectConfig() {
    const indexPage = document.getElementById('pb-index-page-select').value;
    const slugs     = collectSlugs();

    const msgEl = document.getElementById('pb-settings-msg');
    if (!msgEl) { console.error('[pb-index] saveProjectConfig: #pb-settings-msg not found'); return; }
    msgEl.textContent = 'saving...';
    msgEl.className = '';

    fetch('project-config.php', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ index_page: indexPage, slugs: slugs })
    })
    .then(function(r) { return r.json(); })
    .then(function(d) {
        if (d.ok) {
            msgEl.textContent = 'saved';
            msgEl.className   = 'ok';
            console.log('[pb] project config saved', d);
        } else {
            msgEl.textContent = d.error || 'error saving';
            msgEl.className   = 'err';
            console.error('[pb] project config save error', d);
        }
        setTimeout(function() { msgEl.textContent = ''; msgEl.className = ''; }, 3000);
    })
    .catch(function(err) {
        msgEl.textContent = 'network error';
        msgEl.className   = 'err';
        console.error('[pb] saveProjectConfig fetch error', err);
    });
}

function collectSlugs() {
    var slugs = {};
    document.querySelectorAll('.slug-input').forEach(function(input) {
        var page = input.getAttribute('data-page');
        var val  = input.value.trim();
        if (page) slugs[page] = val;
    });
    return slugs;
}

function setLandingPage(name) {
    if (!name) { console.error('[pb-index] setLandingPage: no name provided'); return; }
    // Mirror to settings panel select so it stays in sync if user saves from there
    var sel = document.getElementById('pb-index-page-select');
    if (sel) { sel.value = name; } else { console.warn('[pb-index] setLandingPage: select element not found'); }

    fetch('project-config.php', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ index_page: name, slugs: collectSlugs() })
    })
    .then(function(r) { return r.json(); })
    .then(function(d) {
        if (d.ok) {
            console.log('[pb-index] landing page set to "' + name + '"');
            location.reload();
        } else {
            console.error('[pb-index] setLandingPage error', d);
            alert('Failed to set landing page: ' + (d.error || 'unknown error'));
        }
    })
    .catch(function(e) {
        console.error('[pb-index] setLandingPage network error', e);
        alert('Network error -- could not set landing page');
    });
}

function startRename(card, oldName) {
    if (!card) { console.error('[pb-index] startRename: card element is null'); return; }
    var headerRow   = card.querySelector('.page-card-header');
    var renameRow   = card.querySelector('.page-rename-row');
    var renameInput = card.querySelector('.page-rename-input');
    if (!renameRow || !renameInput) { console.error('[pb-index] startRename: rename elements not found in card for', oldName); return; }
    renameInput.value    = oldName;
    renameInput.disabled = false;
    if (headerRow) headerRow.style.display = 'none';
    renameRow.style.display = 'flex';
    renameInput.focus();
    renameInput.select();
    renameInput.onkeydown = function(e) {
        if (e.key === 'Enter')  confirmRename(card, oldName);
        if (e.key === 'Escape') cancelRename(card);
    };
}

function cancelRename(card) {
    if (!card) { console.error('[pb-index] cancelRename: no card'); return; }
    var headerRow = card.querySelector('.page-card-header');
    var renameRow = card.querySelector('.page-rename-row');
    if (renameRow) renameRow.style.display = 'none';
    if (headerRow) headerRow.style.display = 'flex';
}

function confirmRename(card, oldName) {
    if (!card) { console.error('[pb-index] confirmRename: card is null'); return; }
    var input = card.querySelector('.page-rename-input');
    if (!input) { console.error('[pb-index] confirmRename: input not found for', oldName); return; }
    var newName = input.value.trim();
    if (!newName) { console.error('[pb-index] confirmRename: empty new name'); input.focus(); return; }
    if (newName === oldName) { cancelRename(card); return; }
    if (!/^[a-z0-9_-]+$/.test(newName)) {
        console.error('[pb-index] confirmRename: invalid name "' + newName + '"');
        alert('Name must be lowercase letters, digits, hyphens, or underscores only.');
        input.focus();
        return;
    }
    input.disabled = true;
    fetch('rename-page.php', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ page: oldName, new_name: newName })
    })
    .then(function(r) { return r.json(); })
    .then(function(d) {
        if (d.ok) {
            console.log('[pb-index] renamed "' + oldName + '" -> "' + newName + '"');
            location.reload();
        } else {
            input.disabled = false;
            console.error('[pb-index] rename error', d);
            alert('Rename failed: ' + (d.error || 'unknown error'));
        }
    })
    .catch(function(e) {
        input.disabled = false;
        console.error('[pb-index] confirmRename network error', e);
        alert('Network error -- could not rename page');
    });
}

document.getElementById('newPageName').addEventListener('keydown', function(e) {
    if (e.key === 'Enter') createPage();
});
</script>

</body>
</html>
