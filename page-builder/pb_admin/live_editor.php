<?php
require_once __DIR__ . '/auth.php';
require_auth();

$user = current_user();
$pbRoot = rtrim(dirname(ADMIN_URL_PATH), '/');
if ($pbRoot === '/' || $pbRoot === '\\') {
    $pbRoot = '';
}
$base = ($pbRoot !== '' ? $pbRoot : '');
?>
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title><?= APP_NAME ?> | Live Editor</title>
  <link rel="stylesheet" href="<?= ADMIN_URL_PATH ?>/admin.css">
  <style>
    *{box-sizing:border-box}
    body{margin:0;font-family:'JetBrains Mono','Fira Code','Consolas',monospace;background:var(--c-bg);color:var(--c-text)}
    header{height:52px;display:flex;align-items:center;justify-content:space-between;padding:0 18px;border-bottom:1px solid var(--c-border-acc);background:var(--c-bg-1);position:sticky;top:0;z-index:20}
    header h1{font-size:12px;letter-spacing:.08em;color:var(--c-text-2)}
    .btn{border:1px solid var(--c-border-acc);background:var(--c-acc-bg);color:var(--c-text-dim);padding:6px 10px;text-decoration:none;font-size:11px;cursor:pointer}
    .layout{display:grid;grid-template-columns:minmax(300px,420px) minmax(0,1fr);gap:12px;padding:12px;min-height:calc(100vh - 52px)}
    .panel{border:1px solid var(--c-border);background:var(--c-bg-1);border-radius:8px;padding:12px;display:flex;flex-direction:column;gap:10px;min-height:0}
    .panel h2{font-size:11px;text-transform:uppercase;letter-spacing:.1em;color:var(--c-text-faint);margin:0}
    .field{display:flex;flex-direction:column;gap:5px}
    .field label{font-size:11px;color:var(--c-text-faint)}
    .field input,.field textarea,.field select{background:var(--c-bg-2);color:var(--c-text);border:1px solid var(--c-border);padding:8px;font-family:inherit;font-size:12px}
    .field textarea{min-height:96px;resize:vertical}
    .preview-wrap{display:grid;grid-template-rows:auto 1fr;gap:8px;min-height:0}
    .status{font-size:11px;color:var(--c-text-faint)}
    iframe{width:100%;height:100%;min-height:500px;border:1px solid var(--c-border);background:#fff}
    .ai-out{white-space:pre-wrap;font-size:12px;line-height:1.45;background:var(--c-bg-2);border:1px solid var(--c-border);padding:10px;min-height:120px;overflow:auto}
    .controls{display:flex;gap:8px;flex-wrap:wrap}
    .crumb{font-size:11px;color:#8ab4f8;padding:6px 0;min-height:18px}
    @media (max-width:1000px){.layout{grid-template-columns:1fr}}
  </style>
</head>
<body>
<header>
  <h1>LIVE EDITOR | NO CODE PANEL</h1>
  <div style="display:flex;gap:8px;align-items:center">
    <span style="font-size:11px;color:var(--c-text-faint)"><?= htmlspecialchars($user['username'] ?? 'admin', ENT_QUOTES) ?></span>
    <a class="btn" href="<?= ADMIN_URL_PATH ?>/dashboard.php">back</a>
  </div>
</header>

<main class="layout">
  <section class="panel" aria-label="editor controls">
    <h2>Live Content</h2>
    <div class="field"><label for="app-name">App Name</label><input id="app-name" value="XCM Live Builder" /></div>
    <div class="field"><label for="page-title">Page Title</label><input id="page-title" value="Live Preview Page" /></div>
    <div class="field"><label for="intro">Intro</label><textarea id="intro">Edit fields and content updates instantly with responsive grid layout.</textarea></div>
    <div class="field"><label for="cards">Cards JSON (array of {title, body})</label><textarea id="cards">[
  {"title":"Performance","body":"Fast static output with responsive layout and image assets."},
  {"title":"Navigation","body":"Breadcrumb manager compatible shell for public content."},
  {"title":"Security","body":"Use XCM Auth role checks and Crystal Auth 2FA for admin actions."}
]</textarea></div>

    <h2>AI Assistant</h2>
    <div class="field"><label for="ai-mode">Mode</label><select id="ai-mode"><option value="chat">Chat / Context (gpt-4o-mini)</option><option value="render">Render / Apply (gpt-4o)</option></select></div>
    <div class="field"><label for="ai-prompt">Prompt</label><textarea id="ai-prompt" placeholder="Ask for improvements, responsive layout ideas, copy edits, or render changes."></textarea></div>
    <div class="controls">
      <button class="btn" id="ai-send" type="button">ask ai</button>
      <button class="btn" id="apply-ai" type="button">apply title+intro from ai json</button>
    </div>
    <div id="ai-out" class="ai-out">AI output will appear here.</div>
    <p style="font-size:11px;color:var(--c-text-faint)">Key file is read outside repository from Desktop keys path, or OPENAI_API_KEY_FILE env var.</p>
  </section>

  <section class="panel preview-wrap" aria-label="live preview">
    <h2>Preview</h2>
    <div id="bc-trail" class="crumb">Loading breadcrumbs...</div>
    <div class="status" id="preview-status">Ready</div>
    <iframe id="preview-frame" title="Live page preview"></iframe>
  </section>
</main>

<script src="<?= htmlspecialchars($base, ENT_QUOTES) ?>/js/pb-live-editor.js"></script>
<script src="<?= htmlspecialchars($base, ENT_QUOTES) ?>/js/pb-ai-assist.js"></script>
<script>
(function(){
  var refs = {
    appNameInput: document.getElementById('app-name'),
    pageTitleInput: document.getElementById('page-title'),
    introInput: document.getElementById('intro'),
    cardsInput: document.getElementById('cards'),
    iframe: document.getElementById('preview-frame'),
    statusEl: document.getElementById('preview-status')
  };

  if (window.PbLiveEditor && typeof window.PbLiveEditor.installLiveEditor === 'function') {
    window.PbLiveEditor.installLiveEditor(refs);
  }

  var aiOut = document.getElementById('ai-out');
  var aiMode = document.getElementById('ai-mode');
  var aiPrompt = document.getElementById('ai-prompt');
  var aiSend = document.getElementById('ai-send');
  var applyAi = document.getElementById('apply-ai');

  function editorContext(){
    return JSON.stringify({
      title: refs.pageTitleInput.value,
      intro: refs.introInput.value,
      cards: refs.cardsInput.value
    });
  }

  aiSend.addEventListener('click', async function(){
    aiOut.textContent = 'requesting...';
    if (!window.PbAiAssist || typeof window.PbAiAssist.requestAssist !== 'function') {
      aiOut.textContent = 'AI module not loaded';
      return;
    }
    var res = await window.PbAiAssist.requestAssist('<?= ADMIN_URL_PATH ?>/ai_assist_proxy.php', {
      mode: aiMode.value,
      prompt: aiPrompt.value,
      context: editorContext()
    });
    if (!res.ok) {
      aiOut.textContent = 'Error: ' + (res.error || 'unknown');
      return;
    }
    aiOut.textContent = '[' + (res.model || 'model') + ']\n\n' + (res.output || '');
  });

  applyAi.addEventListener('click', function(){
    try {
      var raw = aiOut.textContent || '';
      var start = raw.indexOf('{');
      var end = raw.lastIndexOf('}');
      if (start < 0 || end <= start) return;
      var obj = JSON.parse(raw.slice(start, end + 1));
      if (obj.title) refs.pageTitleInput.value = String(obj.title);
      if (obj.intro) refs.introInput.value = String(obj.intro);
      window.PbLiveEditor.renderPreview(refs);
    } catch (_err) {
    }
  });

  var trailEl = document.getElementById('bc-trail');
  function loadScript(src){
    return new Promise(function(resolve,reject){
      var s=document.createElement('script'); s.src=src; s.onload=resolve; s.onerror=reject; document.head.appendChild(s);
    });
  }

  (async function(){
    try {
      if (typeof window.createBcMgrWasmAPI !== 'function') {
        await loadScript('<?= htmlspecialchars($base, ENT_QUOTES) ?>/public_html/breadcrumb-manager/bc_mgr_wasm.js');
        await loadScript('<?= htmlspecialchars($base, ENT_QUOTES) ?>/public_html/breadcrumb-manager/bc_mgr_wasm_adapter.js');
      }
      if (typeof window.createBcMgrWasmAPI !== 'function') {
        trailEl.textContent = 'bc_mgr not available';
        return;
      }
      var api = await window.createBcMgrWasmAPI({ baseUrl: '<?= htmlspecialchars($base, ENT_QUOTES) ?>/public_html/breadcrumb-manager' });
      api.addPage('/pb_admin/dashboard.php', 'Admin Dashboard');
      api.addPage('/pb_admin/live_editor.php', 'Live Editor');
      var trail = api.getTrail() || [];
      trailEl.textContent = trail.map(function(x){ return x && x.title ? x.title : ''; }).filter(Boolean).join(' / ');
    } catch (_err) {
      trailEl.textContent = 'breadcrumb fallback: Admin Dashboard / Live Editor';
    }
  }());
}());
</script>
</body>
</html>
