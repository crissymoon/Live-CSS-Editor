<?php
/**
 * pb_admin/tools/_template.php
 * Copy this file to create a new tool card on the admin dashboard.
 *
 * Required variables (set before any HTML output):
 *   $tool_id    - unique slug, used as the DOM anchor id
 *   $tool_title - displayed in the card header
 *   $tool_icon  - HTML entity or text for the header icon (e.g. '&bull;')
 *   $tool_cols  - grid column span: 1 (default), 2, or 3 (full width)
 *
 * After the variable block, echo any HTML for the card body.
 *
 * Use registerTool(id, fn) in JavaScript so the refresh button works.
 * Use apiFetch(action, params, callback) to call api_proxy.php endpoints.
 * All errors should be logged with console.error() for debug visibility.
 */
$tool_id    = 'my-tool';
$tool_title = 'my tool';
$tool_icon  = '&rsaquo;';
$tool_cols  = 1;
?>

<div id="mytool-wrap">
    <div class="skeleton-line"></div>
    <div class="skeleton-line"></div>
</div>

<script>
(function() {
    function load() {
        var wrap = document.getElementById('mytool-wrap');
        wrap.innerHTML = '<div class="skeleton-line"></div>';

        // Example: use apiFetch to call api_proxy.php
        apiFetch('health', {}, function(err, data) {
            if (err || !data || !data.ok) {
                console.error('[my-tool] load error:', err || data);
                wrap.innerHTML = '<span class="err-msg">Failed to load: ' + (err ? err.message : (data && data.error ? data.error : 'unknown')) + '</span>';
                return;
            }
            wrap.innerHTML = '<span style="color:#10b981;font-size:11px;">Tool is working. Replace this with your output.</span>';
        });
    }

    // Register with the dashboard refresh button
    registerTool('my-tool', load);
    load();
})();
</script>
