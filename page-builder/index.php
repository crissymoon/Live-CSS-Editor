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

        $pages[] = [
            'name'          => $name,
            'built'         => $builtAt,
            'overrideCount' => count($overrides),
        ];
    }
}

// Sort alphabetically
usort($pages, fn($a, $b) => strcasecmp($a['name'], $b['name']));
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

        .page-card-actions {
            display: flex;
            gap: 6px;
            flex-wrap: wrap;
        }

        .page-card-actions .btn {
            font-size: 11px;
            padding: 5px 10px;
            flex: 1;
            justify-content: center;
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
        <button class="pb-theme-toggle" id="pb-idx-theme-btn" onclick="toggleTheme()" style="font-size:11px;padding:4px 10px;">light mode</button>
        <a href="/pb_admin/dashboard.php" class="back-link">← admin dashboard</a>
        <a href="../index.php" class="back-link">← style tool</a>
    </div>
</header>

<main>
    <p class="section-title">pages</p>

    <?php if (empty($pages)): ?>
        <p class="empty-state">No pages found in <code>pages/</code>. Create a subdirectory and add JSON config files.</p>
    <?php else: ?>
    <div class="page-grid">
        <?php foreach ($pages as $page): ?>
        <div class="page-card">
            <div class="page-card-name"><?= htmlspecialchars($page['name']) ?></div>

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

document.getElementById('newPageName').addEventListener('keydown', function(e) {
    if (e.key === 'Enter') createPage();
});
</script>

</body>
</html>
