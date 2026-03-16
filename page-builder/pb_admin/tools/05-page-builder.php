<?php
/**
 * tools/05-page-builder.php
 * Page Builder analytics widget for the admin dashboard.
 * Shows staging history from page-builder/deploy/build-log.json.
 */
$tool_id    = '05-page-builder';
$tool_title = 'page builder';
$tool_icon  = '&#9632;';
$tool_cols  = 2;  // spans two grid columns for the table

// Derive page-builder root from the admin mount path.
$pbRoot = rtrim(dirname(ADMIN_URL_PATH), '/');
if ($pbRoot === '/' || $pbRoot === '\\') {
    $pbRoot = '';
}

// Read build log from disk (server-side, no network call needed)
$buildLogPath = dirname(dirname(__DIR__)) . '/deploy/build-log.json';
$buildLog     = null;
$buildLogErr  = null;

$projectCfgPath = dirname(dirname(__DIR__)) . '/project.json';
$projectCfg = [];
if (file_exists($projectCfgPath)) {
    $projectRaw = file_get_contents($projectCfgPath);
    if ($projectRaw !== false) {
        $decoded = json_decode($projectRaw, true);
        if (is_array($decoded)) {
            $projectCfg = $decoded;
        }
    }
}

$builderScriptLanguage = strtolower((string)($projectCfg['builder_script_language'] ?? 'javascript'));
if (!in_array($builderScriptLanguage, ['javascript', 'typescript'], true)) {
    $builderScriptLanguage = 'javascript';
}

$supportedScriptLanguages = $projectCfg['supported_script_languages'] ?? ['javascript'];
if (!is_array($supportedScriptLanguages)) {
    $supportedScriptLanguages = ['javascript'];
}
$supportedScriptLanguages = array_values(array_unique(array_filter(array_map(
    static fn($x) => strtolower(trim((string)$x)),
    $supportedScriptLanguages
), static fn($x) => in_array($x, ['javascript', 'typescript'], true))));
if (empty($supportedScriptLanguages)) {
    $supportedScriptLanguages = ['javascript'];
}

$breadcrumbEnabled = !empty($projectCfg['breadcrumb_manager_enabled']);
$breadcrumbPackage = (string)($projectCfg['breadcrumb_manager_package'] ?? 'bc_mgr_wasm_with_storage');
if (!in_array($breadcrumbPackage, ['bc_mgr_wasm_with_storage', 'bc_mgr_wasm_dropin'], true)) {
    $breadcrumbPackage = 'bc_mgr_wasm_with_storage';
}

if (!file_exists($buildLogPath)) {
    $buildLogErr = 'No deploy found yet. Run a staging from the page builder.';
} else {
    $raw = file_get_contents($buildLogPath);
    if ($raw === false) {
        $buildLogErr = 'Could not read build-log.json';
        error_log('[pb-admin/page-builder] failed to read ' . $buildLogPath);
    } else {
        $parsed = json_decode($raw, true);
        if (!is_array($parsed)) {
            $buildLogErr = 'build-log.json contains invalid JSON: ' . json_last_error_msg();
            error_log('[pb-admin/page-builder] invalid JSON in build-log.json: ' . json_last_error_msg());
        } else {
            $buildLog = $parsed;
        }
    }
}
?>

<div id="pb-tool-wrap">

<div class="data-table" style="font-size:11px;margin-bottom:14px;">
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:0;border:1px solid rgba(99,102,241,0.15);">
        <div style="padding:10px 12px;border-right:1px solid rgba(99,102,241,0.1);">
            <div style="color:#3a3a5a;font-size:10px;letter-spacing:0.08em;margin-bottom:4px;">visual editor script mode</div>
            <div style="color:#a0a0c0;"><?= htmlspecialchars($builderScriptLanguage) ?></div>
        </div>
        <div style="padding:10px 12px;border-right:1px solid rgba(99,102,241,0.1);">
            <div style="color:#3a3a5a;font-size:10px;letter-spacing:0.08em;margin-bottom:4px;">enabled script languages</div>
            <div style="color:#a0a0c0;"><?= htmlspecialchars(implode(', ', $supportedScriptLanguages)) ?></div>
        </div>
        <div style="padding:10px 12px;">
            <div style="color:#3a3a5a;font-size:10px;letter-spacing:0.08em;margin-bottom:4px;">breadcrumb runtime</div>
            <div style="color:#a0a0c0;"><?= $breadcrumbEnabled ? 'enabled' : 'disabled' ?> (<?= htmlspecialchars($breadcrumbPackage) ?>)</div>
        </div>
    </div>
</div>

<?php if ($buildLogErr): ?>
    <p style="font-size:11px;color:#5555a0;"><?= htmlspecialchars($buildLogErr) ?></p>
    <div style="margin-top:12px;">
        <a href="<?= htmlspecialchars(($pbRoot !== '' ? $pbRoot : '') . '/', ENT_QUOTES) ?>" style="font-size:11px;color:#6366f1;text-decoration:none;letter-spacing:0.06em;">
            open page builder &rsaquo;
        </a>
    </div>

<?php else:
    $lastDeploy  = htmlspecialchars($buildLog['last_deploy']  ?? '--');
    $elapsed     = htmlspecialchars((string)($buildLog['elapsed_sec'] ?? '--'));
    $staged      = (int)($buildLog['pages_staged']   ?? 0);
    $skipped     = (int)($buildLog['pages_skipped']  ?? 0);
    $errored     = (int)($buildLog['pages_errored']  ?? 0);
    $bytesRaw    = (int)($buildLog['total_bytes_raw'] ?? 0);
    $bytesMin    = (int)($buildLog['total_bytes_min'] ?? 0);
    $savedPct    = $buildLog['total_saved_pct'] ?? '0';
    $deployUrl   = $buildLog['deploy_url'] ?? (($pbRoot !== '' ? $pbRoot : '') . '/deploy/');
    $stagedPages = $buildLog['staged_pages'] ?? [];
    $errors      = $buildLog['errors'] ?? [];
?>

<div class="data-table" style="font-size:11px;margin-bottom:14px;">
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr 1fr;gap:0;border:1px solid rgba(99,102,241,0.15);">
        <?php
        $cells = [
            'last deploy'   => $lastDeploy,
            'pages staged'  => $staged . ($skipped ? ' / ' . $skipped . ' blocked' : ''),
            'output size'   => number_format($bytesMin) . ' B (' . $savedPct . '% saved)',
            'build time'    => $elapsed . 's',
        ];
        foreach ($cells as $k => $v):
        ?>
        <div style="padding:10px 12px;border-right:1px solid rgba(99,102,241,0.1);">
            <div style="color:#3a3a5a;font-size:10px;letter-spacing:0.08em;margin-bottom:4px;"><?= htmlspecialchars($k) ?></div>
            <div style="color:#a0a0c0;"><?= htmlspecialchars($v) ?></div>
        </div>
        <?php endforeach; ?>
    </div>
</div>

<?php if (!empty($stagedPages)): ?>
<div style="margin-bottom:14px;">
    <div style="font-size:10px;color:#3a3a5a;letter-spacing:0.08em;margin-bottom:6px;text-transform:uppercase;">staged pages</div>
    <div style="font-size:11px;border:1px solid rgba(99,102,241,0.12);">
        <?php foreach ($stagedPages as $i => $sp):
            $bg = $i % 2 === 0 ? 'rgba(99,102,241,0.03)' : 'transparent';
        ?>
        <div style="display:flex;align-items:center;justify-content:space-between;padding:6px 12px;background:<?= $bg ?>;border-bottom:1px solid rgba(99,102,241,0.07);">
            <a href="<?= htmlspecialchars($sp['url'] ?? '#', ENT_QUOTES) ?>" target="_blank"
               style="color:#8888c0;text-decoration:none;letter-spacing:0.04em;">
                <?= htmlspecialchars($sp['name'] ?? '?') ?>
            </a>
            <div style="display:flex;gap:14px;align-items:center;color:#3a3a5a;">
                <span><?= number_format((int)($sp['bytes_min'] ?? 0)) ?> B</span>
                <span style="color:<?= (float)($sp['saved_pct'] ?? 0) > 10 ? '#10b981' : '#5555a0'; ?>"><?= htmlspecialchars((string)($sp['saved_pct'] ?? '0')) ?>% saved</span>
            </div>
        </div>
        <?php endforeach; ?>
    </div>
</div>
<?php endif; ?>

<?php if (!empty($errors)): ?>
<div style="margin-bottom:14px;">
    <div style="font-size:10px;color:#a04040;letter-spacing:0.08em;margin-bottom:6px;">errors during last build</div>
    <?php foreach ($errors as $err): ?>
    <div style="font-size:11px;color:#a04040;padding:4px 0;border-bottom:1px solid rgba(239,68,68,0.1);">
        <?= htmlspecialchars($err) ?>
    </div>
    <?php endforeach; ?>
</div>
<?php endif; ?>

<div style="display:flex;gap:12px;align-items:center;flex-wrap:wrap;">
    <a href="<?= htmlspecialchars(($pbRoot !== '' ? $pbRoot : '') . '/', ENT_QUOTES) ?>" style="font-size:11px;color:#6366f1;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.3);">
        open page builder
    </a>
    <a href="<?= htmlspecialchars(($pbRoot !== '' ? $pbRoot : '') . '/watcher.php', ENT_QUOTES) ?>" style="font-size:11px;color:#6366f1;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.3);">
        open visual editor
    </a>
    <a href="<?= htmlspecialchars(($pbRoot !== '' ? $pbRoot : '') . '/public_html/breadcrumb-manager/', ENT_QUOTES) ?>" target="_blank"
       style="font-size:11px;color:#8888a0;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.15);">
        breadcrumb manager
    </a>
    <a href="<?= htmlspecialchars($deployUrl, ENT_QUOTES) ?>" target="_blank"
       style="font-size:11px;color:#8888a0;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.15);">
        view staged site
    </a>
    <button id="pb-tool-refresh-btn" onclick="pbToolRefresh()"
        style="font-size:11px;color:#8888a0;background:none;border:1px solid rgba(99,102,241,0.15);font-family:inherit;padding:5px 12px;cursor:pointer;letter-spacing:0.04em;">
        refresh stats
    </button>
</div>

<?php endif; ?>

</div>

<script>
(function () {
    function pbToolRefresh() {
        try {
            var btn = document.getElementById('pb-tool-refresh-btn');
            if (btn) { btn.textContent = 'refreshing...'; btn.disabled = true; }
            console.log('[pb-tool] refreshing build log');

            fetch('<?= htmlspecialchars(($pbRoot !== '' ? $pbRoot : '') . '/stage.php', ENT_QUOTES) ?>')
                .then(function (r) {
                    if (!r.ok) throw new Error('HTTP ' + r.status);
                    return r.json();
                })
                .then(function (d) {
                    if (d && typeof d.pages === 'object') {
                        console.log('[pb-tool] stage info refreshed, pages:', d.pages.length);
                    }
                    // Reload just the tool widget by reloading the page
                    location.reload();
                })
                .catch(function (e) {
                    console.error('[pb-tool] refresh error:', e);
                    if (btn) { btn.textContent = 'refresh stats'; btn.disabled = false; }
                });
        } catch (e) {
            console.error('[pb-tool] pbToolRefresh outer error:', e);
        }
    }

    // Expose so the inline onclick can call it
    window.pbToolRefresh = pbToolRefresh;

    try {
        if (typeof registerTool === 'function') {
            registerTool('05-page-builder', function () {});
        }
        console.log('[pb-tool] page-builder widget loaded');
    } catch (e) {
        console.error('[pb-tool] init error:', e);
    }
})();
</script>
