<?php
/**
 * tools/08-agent-flow.php
 * Quick links to the Agent Flow service from the admin dashboard.
 */
$tool_id    = '08-agent-flow';
$tool_title = 'agent flow';
$tool_icon  = '&#9881;';
$tool_cols  = 1;

$agentFlowUrl = 'http://localhost:9090';
?>

<div style="display:flex;flex-direction:column;gap:10px;">
    <div style="font-size:11px;color:#a0a0c0;line-height:1.5;">
        launch and manage company-context flows used by chatbot sections and page-builder integrations.
    </div>

    <div class="data-table" style="font-size:11px;">
        <div style="display:grid;grid-template-columns:1fr;gap:0;border:1px solid rgba(99,102,241,0.15);">
            <div style="padding:10px 12px;">
                <div style="color:#3a3a5a;font-size:10px;letter-spacing:0.08em;margin-bottom:4px;">default endpoint</div>
                <div style="color:#a0a0c0;"><?= htmlspecialchars($agentFlowUrl, ENT_QUOTES) ?></div>
            </div>
        </div>
    </div>

    <div style="display:flex;gap:10px;flex-wrap:wrap;">
        <a href="<?= htmlspecialchars($agentFlowUrl, ENT_QUOTES) ?>" target="_blank"
           style="font-size:11px;color:#6366f1;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.3);">
            open agent flow
        </a>
        <a href="../sections/chatbot/api/chat.php" target="_blank"
           style="font-size:11px;color:#8888a0;text-decoration:none;letter-spacing:0.06em;padding:5px 12px;border:1px solid rgba(99,102,241,0.15);">
            chatbot api
        </a>
    </div>
</div>

<script>
(function () {
    try {
        if (typeof registerTool === 'function') {
            registerTool('08-agent-flow', function () {});
        }
        console.log('[pb-tool] agent-flow widget loaded');
    } catch (e) {
        console.error('[pb-tool] agent-flow widget init error:', e);
    }
})();
</script>
