<?php
$tool_id = '09-live-editor-ai';
$tool_title = 'live editor + ai';
$tool_icon = '&#9632;';
$tool_cols = 1;

$pbRoot = rtrim(dirname(ADMIN_URL_PATH), '/');
if ($pbRoot === '/' || $pbRoot === '\\') {
    $pbRoot = '';
}
$liveEditorHref = ADMIN_URL_PATH . '/live_editor.php';
$mcpPath = 'C:\\Users\\criss\\Documents\\spreadsheet_tool\\imgui-browser\\mcp-server';
?>
<div style="display:flex;flex-direction:column;gap:10px;font-size:11px;line-height:1.45;">
  <div style="color:#a0a0c0;">No-code live editing with responsive preview and AI assistance.</div>
  <div style="color:#7c7ca8;">Chat/context model: <strong>gpt-4o-mini</strong><br>Render/apply model: <strong>gpt-4o</strong></div>
  <div style="color:#7c7ca8;">MCP context path:<br><code style="color:#9dc1ff;"><?= htmlspecialchars($mcpPath, ENT_QUOTES) ?></code></div>
  <a href="<?= htmlspecialchars($liveEditorHref, ENT_QUOTES) ?>" style="text-decoration:none;color:#9dc1ff;border:1px solid rgba(99,102,241,0.25);padding:6px 10px;display:inline-block;width:max-content;">open live editor panel</a>
</div>
