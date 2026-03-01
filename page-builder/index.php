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
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Page Builder</title>
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: #0a0a14;
            color: #e8e8f0;
            font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
            min-height: 100vh;
        }

        /* ---- Header ---- */
        header {
            background: #0d0d18;
            border-bottom: 1px solid rgba(99, 102, 241, 0.25);
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
            color: #a5a5c0;
        }

        header h1 span {
            color: #6366f1;
        }

        .header-actions {
            display: flex;
            gap: 8px;
        }

        .btn {
            background: rgba(99, 102, 241, 0.12);
            border: 1px solid rgba(99, 102, 241, 0.3);
            color: #a5a5c0;
            font-family: inherit;
            font-size: 12px;
            padding: 6px 14px;
            cursor: pointer;
            letter-spacing: 0.04em;
            text-decoration: none;
            display: inline-flex;
            align-items: center;
            gap: 5px;
            transition: background 0.15s, color 0.15s;
        }

        .btn:hover {
            background: rgba(99, 102, 241, 0.25);
            color: #e8e8f0;
        }

        .btn-primary {
            background: rgba(99, 102, 241, 0.22);
            border-color: rgba(99, 102, 241, 0.5);
            color: #c7c7f0;
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
            color: #5555a0;
            margin-bottom: 16px;
        }

        /* ---- Page Grid ---- */
        .page-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
            gap: 14px;
        }

        .page-card {
            background: #0d0d18;
            border: 1px solid rgba(255, 255, 255, 0.06);
            padding: 18px;
            display: flex;
            flex-direction: column;
            gap: 12px;
            transition: border-color 0.15s;
        }

        .page-card:hover {
            border-color: rgba(99, 102, 241, 0.35);
        }

        .page-card-name {
            font-size: 14px;
            font-weight: 600;
            color: #c7c7f0;
            letter-spacing: 0.04em;
        }

        .page-card-meta {
            font-size: 11px;
            color: #5555a0;
            display: flex;
            flex-direction: column;
            gap: 3px;
        }

        .page-card-meta .meta-line {
            display: flex;
            gap: 6px;
        }

        .meta-key   { color: #5555a0; }
        .meta-value { color: #8888a0; }

        .badge {
            display: inline-block;
            padding: 2px 7px;
            font-size: 10px;
            letter-spacing: 0.05em;
            background: rgba(99, 102, 241, 0.1);
            border: 1px solid rgba(99, 102, 241, 0.2);
            color: #8888c0;
        }

        .badge.badge-built {
            background: rgba(16, 185, 129, 0.08);
            border-color: rgba(16, 185, 129, 0.2);
            color: #5da882;
        }

        .badge.badge-unbuilt {
            background: rgba(239, 68, 68, 0.08);
            border-color: rgba(239, 68, 68, 0.2);
            color: #a06060;
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
            color: #5555a0;
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
            background: #0d0d18;
            border: 1px solid rgba(255, 255, 255, 0.06);
            padding: 14px 16px;
        }

        .create-form input[type="text"] {
            background: #13131f;
            border: 1px solid rgba(255, 255, 255, 0.08);
            color: #e8e8f0;
            font-family: inherit;
            font-size: 12px;
            padding: 7px 10px;
            outline: none;
            flex: 1;
            transition: border-color 0.15s;
        }

        .create-form input[type="text"]:focus {
            border-color: rgba(99, 102, 241, 0.5);
        }

        .create-form input[type="text"]::placeholder {
            color: #3a3a5a;
        }

        /* ---- Back link ---- */
        .back-link {
            color: #5555a0;
            font-size: 11px;
            text-decoration: none;
            letter-spacing: 0.06em;
        }

        .back-link:hover { color: #8888a0; }

        /* ---- Inline message ---- */
        #msg {
            font-size: 11px;
            padding: 5px 10px;
            display: none;
        }

        #msg.ok  { color: #10b981; background: rgba(16,185,129,0.08); border: 1px solid rgba(16,185,129,0.2); }
        #msg.err { color: #ef4444; background: rgba(239,68,68,0.08);  border: 1px solid rgba(239,68,68,0.2);  }
    </style>
</head>
<body>

<header>
    <h1><span>&gt;</span> page-builder</h1>
    <div class="header-actions">
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
                <a href="watcher.php?page=<?= urlencode($page['name']) ?>" class="btn btn-primary">
                    ▶ open
                </a>
                <button
                    class="btn"
                    onclick="buildPage('<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>', this)">
                    ⟳ rebuild
                </button>
                <a href="watcher.php?page=<?= urlencode($page['name']) ?>&reset=1"
                   class="btn"
                   onclick="return confirm('Reset all overrides for &quot;<?= htmlspecialchars($page['name'], ENT_QUOTES) ?>&quot;?')">
                    ✕ reset
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
    const m = document.getElementById('msg');
    m.textContent = text;
    m.className = ok ? 'ok' : 'err';
    m.style.display = 'inline-block';
    setTimeout(() => { m.style.display = 'none'; }, 3000);
}

function buildPage(name, btn) {
    const orig = btn.textContent;
    btn.textContent = '…';
    btn.disabled = true;
    fetch('build.php?page=' + encodeURIComponent(name))
        .then(r => r.json())
        .then(d => {
            if (d.ok) {
                showMsg('Built "' + name + '"', true);
                setTimeout(() => location.reload(), 800);
            } else {
                showMsg('Error: ' + (d.error || '?'), false);
                btn.textContent = orig;
                btn.disabled = false;
            }
        })
        .catch(() => {
            showMsg('Network error', false);
            btn.textContent = orig;
            btn.disabled = false;
        });
}

function createPage() {
    const input = document.getElementById('newPageName');
    const name  = input.value.trim().toLowerCase().replace(/[^a-z0-9-_]/g, '-');
    if (!name) { showMsg('Enter a page name', false); return; }

    fetch('create-page.php', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify({ name }),
    })
        .then(r => r.json())
        .then(d => {
            if (d.ok) {
                showMsg('Created "' + name + '" — building…', true);
                input.value = '';
                /* Auto-build then reload */
                fetch('build.php?page=' + encodeURIComponent(name))
                    .finally(() => setTimeout(() => location.reload(), 600));
            } else {
                showMsg('Error: ' + (d.error || '?'), false);
            }
        })
        .catch(() => showMsg('Network error', false));
}

/* Allow Enter key in the new-page input */
document.getElementById('newPageName').addEventListener('keydown', function(e) {
    if (e.key === 'Enter') createPage();
});
</script>

</body>
</html>
