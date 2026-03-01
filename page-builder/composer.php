<?php
/**
 * page-builder/composer.php
 *
 * Visual page composer - drag-and-drop section assembly with JSON editor.
 * Usage: composer.php?page=<page-name>
 *
 * Left panel:  section template library (templated from /sections/)
 * Center:      assembled page canvas (ordered sections, drag to reorder)
 * Right panel: JSON editor for the selected section (dev/designer view)
 * Top bar:     page name, theme toggle, build, preview, back links
 *
 * Section data is persisted in pages/{name}/page.json (manifest) and
 * individual JSON files for each section content.
 */

$page = preg_replace('/[^a-z0-9_-]/i', '', $_GET['page'] ?? '');
?><!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>composer<?= $page ? ' -- ' . htmlspecialchars($page) : '' ?> | page-builder</title>
<script>
/* No-flash theme init - must run before first paint */
(function () {
    try {
        var t = localStorage.getItem('pb_theme');
        if (t === 'light' || t === 'dark') {
            document.documentElement.setAttribute('data-theme', t);
        }
    } catch (e) { /* localStorage unavailable */ }
})();
</script>
<link rel="stylesheet" href="css/pb-theme.css">
<link rel="stylesheet" href="css/pb-composer.css">
</head>
<body>

<div class="pbc-app">

    <!-- =================== Top bar =================== -->
    <div class="pbc-topbar">
        <span class="pbc-topbar-logo"><span>&gt;</span> page-builder</span>
        <div class="pbc-topbar-sep"></div>
        <?php if ($page): ?>
        <span class="pbc-topbar-page" id="pbc-page-label"><?= htmlspecialchars($page) ?></span>
        <?php else: ?>
        <span class="pbc-topbar-page" style="color:var(--pb-err)">no page specified</span>
        <?php endif; ?>

        <div class="pbc-topbar-spacer"></div>

        <div class="pbc-topbar-actions">
            <span class="pb-msg" id="pbc-msg"></span>
            <button id="pbc-theme-btn" class="pb-theme-toggle" onclick="PBC.toggleTheme()">light mode</button>
            <?php if ($page): ?>
            <button id="pbc-build-btn" class="pb-btn pb-btn-primary" onclick="PBC.buildPage()">build</button>
            <a href="pages/<?= urlencode($page) ?>/index.html" target="_blank" class="pb-btn">preview</a>
            <button class="pb-btn" onclick="PBC.saveOrder()">save order</button>
            <?php endif; ?>
            <a href="index.php" class="pb-btn">back</a>
            <span id="pbc-status" style="font-size:10px;color:var(--pb-text-faint);">ready</span>
        </div>
    </div>

    <!-- =================== Body (3 cols) =================== -->
    <div class="pbc-body">

        <!-- ========= Left: Section library ========= -->
        <div class="pbc-library">
            <div class="pbc-lib-header">
                <p class="pbc-lib-title">section library</p>
                <div class="pbc-lib-tabs">
                    <button class="pbc-lib-tab active" data-filter="all"     onclick="PBC.setLibFilter('all')">all</button>
                    <button class="pbc-lib-tab"         data-filter="header"  onclick="PBC.setLibFilter('header')">header</button>
                    <button class="pbc-lib-tab"         data-filter="section" onclick="PBC.setLibFilter('section')">section</button>
                    <button class="pbc-lib-tab"         data-filter="panel"   onclick="PBC.setLibFilter('panel')">panel</button>
                    <button class="pbc-lib-tab"         data-filter="footer"  onclick="PBC.setLibFilter('footer')">footer</button>
                </div>
            </div>
            <div class="pbc-lib-search">
                <input
                    type="search"
                    class="pb-input"
                    placeholder="search templates..."
                    oninput="PBC.libSearch(this.value)"
                    style="font-size:11px;">
            </div>
            <div class="pbc-lib-scroll" id="pbc-lib-scroll">
                <span style="color:var(--pb-text-faint);font-size:11px;padding:8px 4px;">loading...</span>
            </div>
        </div>

        <!-- ========= Center: Canvas ========= -->
        <div class="pbc-canvas-wrap" id="pbc-canvas-wrap">
            <?php if (!$page): ?>
            <p class="pbc-canvas-empty" style="color:var(--pb-err)">
                No page specified. Add <code>?page=my-page-name</code> to this URL.
            </p>
            <?php else: ?>
            <p class="pbc-canvas-title">
                sections &mdash; drag to reorder, click <em>edit JSON</em> to view or update a section
            </p>

            <div id="pbc-canvas-inner">
                <div class="pbc-canvas-empty">loading page...</div>
            </div>

            <!-- Divider before library reminder -->
            <div class="pbc-add-bar" style="margin-top:20px;">
                <div class="pbc-add-bar-line"></div>
                <span class="pbc-add-bar-label">add from library</span>
                <div class="pbc-add-bar-line"></div>
            </div>

            <p style="font-size:10px;color:var(--pb-text-faint);margin-top:6px;text-align:center;">
                Click <strong style="color:var(--pb-text-muted)">+ add</strong> on any template in the left panel to append it to this page.
            </p>

            <?php endif; ?>
        </div>

        <!-- ========= Right: JSON editor ========= -->
        <div class="pbc-json-panel collapsed" id="pbc-json-panel">
            <div class="pbc-json-header">
                <span class="pbc-json-title" id="pbc-json-title">json editor</span>
                <button class="pb-btn" style="font-size:9px;padding:2px 7px;" onclick="PBC.closeJsonPanel()">close</button>
            </div>
            <div class="pbc-json-body">
                <div id="pbc-json-placeholder" class="pbc-json-placeholder">
                    Select a section's <strong>edit JSON</strong> button to open its raw JSON here.<br><br>
                    Devs and designers can edit the full JSON structure. Lower-level users can use the live editor via the Preview link.
                </div>

                <div id="pbc-json-info" class="pbc-json-info" style="display:none;"></div>

                <div class="pbc-json-label-row" id="pbc-json-label-row" style="display:none;">
                    <span class="pbc-json-label-label">label</span>
                    <input type="text" class="pb-input" id="pbc-json-label-input"
                        style="font-size:11px;"
                        placeholder="Section label"
                        onblur="PBC.saveLabelFromInput()">
                </div>

                <textarea
                    class="pbc-json-textarea"
                    id="pbc-json-textarea"
                    style="display:none;"
                    spellcheck="false"
                    autocorrect="off"
                    autocapitalize="off"
                    placeholder="Select a section to edit its JSON..."></textarea>

                <div class="pbc-json-err" id="pbc-json-err"></div>

                <div class="pbc-json-actions" id="pbc-json-actions" style="display:none;">
                    <button class="pb-btn pb-btn-primary" id="pbc-json-save-btn" onclick="PBC.saveJsonEdit()">save JSON</button>
                    <button class="pb-btn" onclick="PBC.formatJson()">format</button>
                    <button class="pb-btn pb-btn-danger" onclick="PBC.closeJsonPanel()">cancel</button>
                </div>
            </div>
        </div>

    </div><!-- /.pbc-body -->

    <!-- =================== Bottom bar =================== -->
    <div class="pbc-bottombar">
        <div class="pbc-bottombar-msg">
            <span>
                lower-level users edit via
                <?php if ($page): ?>
                <a href="watcher.php?page=<?= urlencode($page) ?>" target="_blank"
                   style="color:var(--pb-acc);text-decoration:none;">live editor</a>
                <?php else: ?>
                live editor
                <?php endif; ?>
                &nbsp;|&nbsp;
                devs/designers use this composer
            </span>
        </div>
        <?php if ($page): ?>
        <span style="font-size:10px;color:var(--pb-text-faint);">
            page: <strong style="color:var(--pb-text-dim);"><?= htmlspecialchars($page) ?></strong>
        </span>
        <?php endif; ?>
    </div>

</div><!-- /.pbc-app -->

<script src="js/pb-composer.js"></script>
<script>
/* ---- Extra helpers wired to DOM elements not in the module ---- */

// Show/hide info + label + actions when panel opens/closes
(function () {
    var origSelect = PBC.selectSection;
    PBC.selectSection = function (id) {
        var info    = document.getElementById('pbc-json-info');
        var lr      = document.getElementById('pbc-json-label-row');
        var actions = document.getElementById('pbc-json-actions');
        var ph      = document.getElementById('pbc-json-placeholder');
        if (info)    info.style.display    = '';
        if (lr)      lr.style.display      = '';
        if (actions) actions.style.display = '';
        if (ph)      ph.style.display      = 'none';
        origSelect.call(PBC, id);
    };

    var origClose = PBC.closeJsonPanel;
    PBC.closeJsonPanel = function () {
        var info    = document.getElementById('pbc-json-info');
        var lr      = document.getElementById('pbc-json-label-row');
        var actions = document.getElementById('pbc-json-actions');
        var ph      = document.getElementById('pbc-json-placeholder');
        var ta      = document.getElementById('pbc-json-textarea');
        if (info)    info.style.display    = 'none';
        if (lr)      lr.style.display      = 'none';
        if (actions) actions.style.display = 'none';
        if (ph)      ph.style.display      = '';
        if (ta)      ta.style.display      = 'none';
        origClose.call(PBC);
    };
})();

/* ---- Format JSON in textarea -------------------------------------------- */
PBC.formatJson = function () {
    var ta  = document.getElementById('pbc-json-textarea');
    var err = document.getElementById('pbc-json-err');
    if (!ta) { console.error('[composer] pbc-json-textarea not found'); return; }
    try {
        var parsed = JSON.parse(ta.value);
        ta.value = JSON.stringify(parsed, null, 2);
        if (err) err.className = 'pbc-json-err';
    } catch (e) {
        if (err) { err.textContent = 'Cannot format: invalid JSON - ' + e.message; err.className = 'pbc-json-err show'; }
        console.error('[composer] formatJson error:', e);
    }
};

/* ---- Save label from the label input ------------------------------------- */
PBC.saveLabelFromInput = function () {
    var input = document.getElementById('pbc-json-label-input');
    if (!input || !PBC.selectedId) return;
    var newLabel = input.value.trim();
    if (!newLabel) return;

    var section = (PBC.manifest.sections || []).find(function (s) { return s.id === PBC.selectedId; });
    if (!section || section.label === newLabel) return;

    console.log('[composer] saveLabelFromInput id=' + PBC.selectedId + ' label=' + newLabel);

    fetch('section-api.php?action=rename_section&page=' + encodeURIComponent(PBC.page), {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify({ id: PBC.selectedId, label: newLabel }),
    })
    .then(function (r) { return r.json(); })
    .then(function (d) {
        if (!d || !d.ok) {
            console.error('[composer] saveLabelFromInput api error:', d);
            return;
        }
        PBC.manifest.sections = d.sections;
        PBC.renderCanvas && PBC.renderCanvas();
        console.log('[composer] label saved:', newLabel);
    })
    .catch(function (e) { console.error('[composer] saveLabelFromInput network error:', e); });
};

/* Init call is in pb-composer.js - nothing more needed here */
</script>

</body>
</html>
