<?php
// ── API endpoints ─────────────────────────────────────────────────────────────
$action = $_GET['action'] ?? '';

if ($action === 'generate' || $action === 'enhance') {

    require_once __DIR__ . '/../../../ai/config.php';

    header('Content-Type: application/json');
    header('Access-Control-Allow-Origin: *');
    header('Access-Control-Allow-Headers: Content-Type');
    if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(204); exit; }
    if ($_SERVER['REQUEST_METHOD'] !== 'POST') { echo json_encode(['error' => 'POST required']); exit; }

    $body = json_decode(file_get_contents('php://input'), true);
    if (!$body) { echo json_encode(['error' => 'Invalid JSON']); exit; }

    $provider = AIConfig::provider('openai');
    $apiKey   = $provider['api_key'];
    $baseUrl  = rtrim($provider['base_url'], '/');

    // ── Prompt enhancement via GPT-4o ────────────────────────────────────────
    if ($action === 'enhance') {
        $raw      = $body['prompt']   ?? '';
        $category = $body['category'] ?? 'ui';

        $systemMap = [
            'ui'        => 'You are an expert prompt engineer for AI image generation. Your task is to expand a short description of a UI element or interface component into a highly detailed, precise image generation prompt. Focus on: visual style (flat, material, glassmorphism, etc.), colors, lighting, shadows, background (transparent by default for UI), scale, pixel density, and technical quality markers. Output ONLY the improved prompt, no commentary.',
            'avatar'    => 'You are an expert prompt engineer for AI image generation. Your task is to expand a short avatar/portrait description into a rich, detailed image generation prompt. Include: art style, lighting setup, color palette, mood, camera angle, background treatment, and quality markers like "professional headshot quality". Output ONLY the improved prompt, no commentary.',
            'marketing' => 'You are an expert prompt engineer for AI image generation. Your task is to expand a short marketing image description into a detailed, high-impact image generation prompt. Include: composition, visual hierarchy, color palette, mood/tone, style (photorealistic, illustrated, etc.), brand feeling, and quality markers. Output ONLY the improved prompt, no commentary.',
        ];
        $system = $systemMap[$category] ?? $systemMap['ui'];

        $payload = json_encode([
            'model'                 => 'gpt-4o',
            'max_completion_tokens' => 400,
            'stream'                => false,
            'messages'              => [
                ['role' => 'system',  'content' => $system],
                ['role' => 'user',    'content' => $raw],
            ],
        ]);

        $ch = curl_init($baseUrl . '/chat/completions');
        curl_setopt_array($ch, [
            CURLOPT_POST           => true,
            CURLOPT_POSTFIELDS     => $payload,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_TIMEOUT        => 30,
            CURLOPT_HTTPHEADER     => [
                'Content-Type: application/json',
                'Authorization: Bearer ' . $apiKey,
            ],
        ]);
        $raw_resp = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $curlErr  = curl_error($ch);
        curl_close($ch);

        if ($curlErr || $httpCode >= 400) {
            echo json_encode(['error' => $curlErr ?: 'HTTP ' . $httpCode]);
            exit;
        }
        $resp = json_decode($raw_resp, true);
        $enhanced = $resp['choices'][0]['message']['content'] ?? '';
        echo json_encode(['prompt' => trim($enhanced)]);
        exit;
    }

    // ── Image generation ─────────────────────────────────────────────────────
    $prompt   = $body['prompt']        ?? '';
    $model    = $body['model']         ?? 'gpt-image-1';
    $size     = $body['size']          ?? '1024x1024';
    $quality  = $body['quality']       ?? 'high';
    $bgStyle  = $body['background']    ?? 'auto';
    $outFmt   = $body['output_format'] ?? 'png';
    $style    = $body['style']         ?? 'vivid'; // dall-e-3 only

    if (trim($prompt) === '') { echo json_encode(['error' => 'Prompt is required']); exit; }

    if ($model === 'gpt-image-1' || $model === 'gpt-image-1.5') {
        $imgPayload = [
            'model'         => $model,
            'prompt'        => $prompt,
            'n'             => 1,
            'size'          => $size,
            'quality'       => $quality,
            'output_format' => $outFmt,
        ];
        // background and output_compression only valid for gpt-image-1
        if ($bgStyle !== 'auto') {
            $imgPayload['background'] = $bgStyle;
        }
        if ($outFmt === 'jpeg' || $outFmt === 'webp') {
            $imgPayload['output_compression'] = 90;
        }
    } else {
        // dall-e-3
        $imgPayload = [
            'model'           => 'dall-e-3',
            'prompt'          => $prompt,
            'n'               => 1,
            'size'            => $size,
            'quality'         => ($quality === 'high' || $quality === 'hd') ? 'hd' : 'standard',
            'style'           => $style,
            'response_format' => 'b64_json',
        ];
    }

    $ch = curl_init($baseUrl . '/images/generations');
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => json_encode($imgPayload),
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => 120,
        CURLOPT_HTTPHEADER     => [
            'Content-Type: application/json',
            'Authorization: Bearer ' . $apiKey,
        ],
    ]);
    $raw_resp = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $curlErr  = curl_error($ch);
    curl_close($ch);

    if ($curlErr) { echo json_encode(['error' => 'cURL: ' . $curlErr]); exit; }
    if ($httpCode >= 400) {
        $errBody = json_decode($raw_resp, true);
        $msg = $errBody['error']['message'] ?? ('HTTP ' . $httpCode);
        echo json_encode(['error' => $msg]);
        exit;
    }

    $resp = json_decode($raw_resp, true);
    if (json_last_error() !== JSON_ERROR_NONE || !isset($resp['data'])) {
        echo json_encode(['error' => 'Unexpected API response']);
        exit;
    }

    $b64   = $resp['data'][0]['b64_json']       ?? null;
    $revised = $resp['data'][0]['revised_prompt'] ?? null;

    if (!$b64) { echo json_encode(['error' => 'No image data in response']); exit; }

    $out = ['b64' => $b64, 'format' => $outFmt, 'model' => $model];
    if ($revised) $out['revised_prompt'] = $revised;
    echo json_encode($out);
    exit;
}

// ── Render UI ────────────────────────────────────────────────────────────────
?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Image Generator</title>
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
:root {
  --bg:        #0d1117;
  --surface:   #161b27;
  --raised:    #1e2535;
  --border:    rgba(99,102,240,.2);
  --accent:    #6366f1;
  --accent-lo: rgba(99,102,240,.14);
  --accent-md: rgba(99,102,240,.28);
  --txt:       #e2e6f5;
  --dim:       #68708e;
  --ok:        #34d39a;
  --warn:      #f59e0b;
  --bad:       #f87171;
  --radius:    8px;
  --sidebar:   310px;
  --header:    50px;
}
html, body { height: 100%; overflow: hidden; background: var(--bg); color: var(--txt); font: 13px/1.5 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }

/* ── Layout ── */
#app { display: flex; height: 100%; flex-direction: column; }
#header {
  height: var(--header); flex-shrink: 0;
  display: flex; align-items: center; gap: 10px;
  padding: 0 16px;
  background: var(--surface);
  border-bottom: 1px solid var(--border);
}
#header h1 { font-size: 14px; font-weight: 600; letter-spacing: .01em; }
#header .model-badge {
  font-size: 10px; padding: 2px 7px; border-radius: 20px;
  background: var(--accent-lo); color: var(--accent);
  border: 1px solid var(--border); font-weight: 600;
}
#body { display: flex; flex: 1; min-height: 0; }

/* ── Sidebar ── */
#sidebar {
  width: var(--sidebar); flex-shrink: 0;
  display: flex; flex-direction: column;
  background: var(--surface);
  border-right: 1px solid var(--border);
  overflow-y: auto;
}

/* ── Tabs ── */
#tabs {
  display: flex; flex-shrink: 0;
  border-bottom: 1px solid var(--border);
}
.tab {
  flex: 1; padding: 10px 4px; font-size: 11px; font-weight: 600;
  text-align: center; cursor: pointer; color: var(--dim);
  border-bottom: 2px solid transparent;
  transition: color .12s, border-color .12s;
  text-transform: uppercase; letter-spacing: .05em;
  user-select: none;
}
.tab:hover { color: var(--txt); }
.tab.active { color: var(--accent); border-color: var(--accent); }

/* ── Sidebar form ── */
.sb-section { padding: 12px 14px 0; }
.sb-label {
  font-size: 10px; font-weight: 600; text-transform: uppercase;
  letter-spacing: .06em; color: var(--dim); margin-bottom: 5px;
  display: flex; align-items: center; justify-content: space-between;
}
.sb-label a {
  font-size: 10px; color: var(--accent); cursor: pointer;
  font-weight: 400; text-transform: none; letter-spacing: 0;
  text-decoration: none;
}
.sb-label a:hover { text-decoration: underline; }

/* Quick-fill chips */
.chips { display: flex; flex-wrap: wrap; gap: 5px; margin-bottom: 8px; }
.chip {
  font-size: 11px; padding: 3px 9px; border-radius: 20px;
  background: var(--raised); border: 1px solid var(--border);
  color: var(--txt); cursor: pointer; user-select: none;
  transition: background .1s, border-color .1s;
}
.chip:hover { background: var(--accent-lo); border-color: var(--accent); color: var(--accent); }

/* Prompt textarea */
#prompt-wrap { position: relative; }
#prompt {
  width: 100%; min-height: 90px; max-height: 180px;
  background: var(--raised); border: 1px solid var(--border);
  border-radius: var(--radius); color: var(--txt); font: inherit;
  font-size: 12px; line-height: 1.55; padding: 8px 10px;
  resize: vertical; outline: none;
  transition: border-color .15s;
}
#prompt:focus { border-color: var(--accent); }
#prompt::placeholder { color: var(--dim); }
#enhance-btn {
  position: absolute; bottom: 8px; right: 8px;
  font-size: 10px; font-weight: 600; padding: 3px 8px; border-radius: 5px;
  background: var(--accent-lo); border: 1px solid var(--border);
  color: var(--accent); cursor: pointer; transition: background .1s;
  white-space: nowrap;
}
#enhance-btn:hover { background: var(--accent-md); }
#enhance-btn.busy { opacity: .5; pointer-events: none; }

/* Options grid */
.opt-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
.opt-row  { display: flex; flex-direction: column; gap: 3px; }
.opt-row label { font-size: 10px; color: var(--dim); font-weight: 600; text-transform: uppercase; letter-spacing: .05em; }
select, .opt-row select {
  background: var(--raised); border: 1px solid var(--border);
  border-radius: 6px; color: var(--txt); font: inherit; font-size: 12px;
  padding: 5px 8px; outline: none; cursor: pointer;
  transition: border-color .15s; width: 100%;
}
select:focus { border-color: var(--accent); }

/* Generate btn */
#gen-btn {
  margin: 12px 14px 16px;
  width: calc(100% - 28px);
  padding: 10px; border-radius: var(--radius);
  background: var(--accent); border: none;
  color: #fff; font: 600 13px/1 inherit; cursor: pointer;
  transition: opacity .15s, transform .1s;
  display: flex; align-items: center; justify-content: center; gap: 7px;
}
#gen-btn:hover { opacity: .88; }
#gen-btn:active { transform: scale(.98); }
#gen-btn.busy { opacity: .5; pointer-events: none; }
#gen-btn .spinner {
  width: 13px; height: 13px; border: 2px solid rgba(255,255,255,.3);
  border-top-color: #fff; border-radius: 50%; display: none;
  animation: spin .7s linear infinite;
}
#gen-btn.busy .spinner { display: block; }
@keyframes spin { to { transform: rotate(360deg); } }

/* ── Canvas / Results ── */
#canvas {
  flex: 1; overflow-y: auto; padding: 16px;
  display: flex; flex-direction: column; gap: 16px;
}
/* Empty state */
#empty {
  flex: 1; display: flex; flex-direction: column;
  align-items: center; justify-content: center; gap: 10px;
  color: var(--dim); user-select: none;
}
#empty svg { opacity: .25; }
#empty p { font-size: 13px; }

/* Results grid */
#results { display: grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 14px; }

.result-card {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: 10px;
  overflow: hidden;
  display: flex; flex-direction: column;
}
.result-img-wrap {
  position: relative; background: repeating-conic-gradient(#1a1a2a 0% 25%, #111120 0% 50%) 0 0 / 20px 20px;
  cursor: zoom-in;
}
.result-img-wrap img { width: 100%; display: block; }
.result-meta { padding: 8px 10px; flex: 1; display: flex; flex-direction: column; gap: 4px; }
.result-info { font-size: 10px; color: var(--dim); display: flex; gap: 8px; flex-wrap: wrap; }
.result-info span { background: var(--raised); padding: 1px 6px; border-radius: 4px; }
.result-revised { font-size: 10px; color: var(--dim); font-style: italic; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }
.result-actions { display: flex; gap: 6px; margin-top: 4px; }
.r-btn {
  flex: 1; padding: 5px 8px; border-radius: 6px; font-size: 11px; font-weight: 600;
  background: var(--raised); border: 1px solid var(--border); color: var(--txt);
  cursor: pointer; transition: background .1s;
}
.r-btn:hover { background: var(--accent-lo); color: var(--accent); border-color: var(--accent); }
.r-btn.primary { background: var(--accent); border-color: var(--accent); color: #fff; }
.r-btn.primary:hover { opacity: .85; }

/* Error toast */
#toast {
  position: fixed; bottom: 16px; left: 50%; transform: translateX(-50%);
  background: #2a1717; border: 1px solid var(--bad); color: var(--bad);
  padding: 8px 16px; border-radius: 8px; font-size: 12px;
  display: none; z-index: 999; max-width: 420px; text-align: center;
}
#toast.show { display: block; }

/* Lightbox */
#lb {
  position: fixed; inset: 0; background: rgba(0,0,0,.88);
  display: none; align-items: center; justify-content: center;
  z-index: 1000; cursor: zoom-out; padding: 24px;
}
#lb.show { display: flex; }
#lb img { max-width: 100%; max-height: 100%; border-radius: 8px; object-fit: contain; }

/* Revised prompt note */
#revised-note {
  margin: 0 14px 10px;
  background: var(--raised); border: 1px solid var(--border);
  border-radius: 6px; padding: 7px 10px;
  font-size: 11px; color: var(--dim); display: none;
}
#revised-note strong { color: var(--warn); }

/* Scrollbar */
::-webkit-scrollbar { width: 5px; }
::-webkit-scrollbar-track { background: transparent; }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 4px; }
</style>
</head>
<body>
<div id="app">

  <div id="header">
    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="var(--accent)" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
      <rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/>
      <polyline points="21 15 16 10 5 21"/>
    </svg>
    <h1>Image Generator</h1>
    <span class="model-badge" id="model-badge">gpt-image-1</span>
    <div style="flex:1"></div>
    <span style="font-size:11px;color:var(--dim)" id="count-label"></span>
  </div>

  <div id="body">

    <!-- Sidebar -->
    <div id="sidebar">

      <div id="tabs">
        <div class="tab active" data-cat="ui"        onclick="setCategory('ui')">UI Elements</div>
        <div class="tab"        data-cat="avatar"    onclick="setCategory('avatar')">Avatar</div>
        <div class="tab"        data-cat="marketing" onclick="setCategory('marketing')">Marketing</div>
      </div>

      <!-- Category-specific quick fills -->
      <div class="sb-section" style="margin-top:10px;">
        <div class="sb-label">Quick fill <a onclick="clearPrompt()">Clear</a></div>

        <div id="chips-ui" class="chips">
          <div class="chip" onclick="addChip('a sleek dark toggle switch, off state')">Toggle</div>
          <div class="chip" onclick="addChip('glass morphism card with soft glow border')">Card</div>
          <div class="chip" onclick="addChip('a modern primary action button, rounded, gradient')">Button</div>
          <div class="chip" onclick="addChip('minimal icon set, line style, uniform weight')">Icon set</div>
          <div class="chip" onclick="addChip('mobile app bottom navigation bar, dark theme')">Nav bar</div>
          <div class="chip" onclick="addChip('settings panel / preferences UI, dark mode')">Settings</div>
          <div class="chip" onclick="addChip('a popup modal dialog with form fields')">Modal</div>
          <div class="chip" onclick="addChip('floating input field with label animation')">Input</div>
          <div class="chip" onclick="addChip('a sidebar menu with icons and active state')">Sidebar</div>
          <div class="chip" onclick="addChip('a data table with alternating row colors')">Table</div>
        </div>

        <div id="chips-avatar" class="chips" style="display:none">
          <div class="chip" onclick="addChip('photorealistic professional headshot')">Realistic</div>
          <div class="chip" onclick="addChip('illustrated cartoon portrait, friendly')">Cartoon</div>
          <div class="chip" onclick="addChip('3D rendered character, stylized, smooth')">3D</div>
          <div class="chip" onclick="addChip('anime style portrait, detailed')">Anime</div>
          <div class="chip" onclick="addChip('oil painting portrait, classical style')">Portrait</div>
          <div class="chip" onclick="addChip('pixel art character, 64x64 style')">Pixel art</div>
          <div class="chip" onclick="addChip('low-poly geometric avatar')">Low-poly</div>
          <div class="chip" onclick="addChip('watercolor illustration portrait')">Watercolor</div>
        </div>

        <div id="chips-marketing" class="chips" style="display:none">
          <div class="chip" onclick="addChip('wide hero banner, tech product, dramatic lighting')">Hero banner</div>
          <div class="chip" onclick="addChip('social media post, square, bold typography style')">Social post</div>
          <div class="chip" onclick="addChip('product shot on clean background, studio lighting')">Product shot</div>
          <div class="chip" onclick="addChip('email header, minimal, elegantly branded')">Email header</div>
          <div class="chip" onclick="addChip('app store screenshot mockup, lifestyle feel')">App preview</div>
          <div class="chip" onclick="addChip('tech startup promotional image, futuristic')">Tech promo</div>
          <div class="chip" onclick="addChip('SaaS dashboard onboarding illustration')">SaaS illus.</div>
          <div class="chip" onclick="addChip('blog post cover image, editorial style')">Blog cover</div>
        </div>
      </div>

      <!-- Prompt -->
      <div class="sb-section">
        <div class="sb-label">Prompt</div>
        <div id="prompt-wrap">
          <textarea id="prompt" placeholder="Describe what you want to generate..."></textarea>
          <button id="enhance-btn" onclick="enhancePrompt()">Enhance</button>
        </div>
      </div>

      <!-- Revised prompt note (after generation) -->
      <div id="revised-note"></div>

      <!-- Options -->
      <div class="sb-section">
        <div class="sb-label" style="margin-bottom:8px;">Options</div>
        <div class="opt-grid">
          <div class="opt-row">
            <label>Model</label>
            <select id="opt-model" onchange="onModelChange()">
              <option value="gpt-image-1" selected>gpt-image-1</option>
              <option value="gpt-image-1.5">gpt-image-1.5</option>
              <option value="dall-e-3">dall-e-3</option>
            </select>
          </div>
          <div class="opt-row">
            <label>Size</label>
            <select id="opt-size"></select>
          </div>
          <div class="opt-row">
            <label>Quality</label>
            <select id="opt-quality"></select>
          </div>
          <div class="opt-row" id="row-background">
            <label>Background</label>
            <select id="opt-background">
              <option value="auto">Auto</option>
              <option value="transparent">Transparent</option>
              <option value="opaque">Opaque</option>
            </select>
          </div>
          <div class="opt-row" id="row-format">
            <label>Format</label>
            <select id="opt-format">
              <option value="png">PNG</option>
              <option value="webp">WebP</option>
              <option value="jpeg">JPEG</option>
            </select>
          </div>
          <div class="opt-row" id="row-style" style="display:none">
            <label>Style</label>
            <select id="opt-style">
              <option value="vivid">Vivid</option>
              <option value="natural">Natural</option>
            </select>
          </div>
        </div>
      </div>

      <button id="gen-btn" onclick="generate()">
        <div class="spinner"></div>
        <span id="gen-label">Generate</span>
      </button>

    </div>
    <!-- /sidebar -->

    <!-- Canvas -->
    <div id="canvas">
      <div id="empty">
        <svg width="60" height="60" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
          <rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/>
        </svg>
        <p>Generated images will appear here</p>
      </div>
      <div id="results"></div>
    </div>

  </div>
</div>

<!-- Lightbox -->
<div id="lb" onclick="closeLb()"><img id="lb-img" src="" alt=""></div>

<!-- Toast -->
<div id="toast"></div>

<script>
// ────────────────────────────────────────────────────────────────────────────
// State
// ────────────────────────────────────────────────────────────────────────────
var _cat     = 'ui';
var _results = [];
var _busy    = false;

// Category defaults: [model, size, quality, background, format, style]
var CAT_DEFAULTS = {
  ui:        { model: 'gpt-image-1', size: '1024x1024', quality: 'high',     background: 'transparent', format: 'png',  style: 'natural' },
  avatar:    { model: 'gpt-image-1', size: '1024x1024', quality: 'high',     background: 'opaque',      format: 'png',  style: 'vivid'   },
  marketing: { model: 'gpt-image-1', size: '1536x1024', quality: 'high',     background: 'opaque',      format: 'png',  style: 'vivid'   },
};

var SIZES = {
  'gpt-image-1':   ['1024x1024', '1536x1024', '1024x1536'],
  'gpt-image-1.5': ['1024x1024', '1536x1024', '1024x1536'],
  'dall-e-3':      ['1024x1024', '1792x1024', '1024x1792'],
};
var QUALITIES = {
  'gpt-image-1':   ['auto', 'high', 'medium', 'low'],
  'gpt-image-1.5': ['auto', 'high', 'medium', 'low'],
  'dall-e-3':      ['hd', 'standard'],
};

// ────────────────────────────────────────────────────────────────────────────
// Init
// ────────────────────────────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', function() {
  populateSelects('gpt-image-1');
  applyDefaults('ui');
});

function populateSelects(model) {
  var sizeEl    = document.getElementById('opt-size');
  var qualEl    = document.getElementById('opt-quality');
  var prevSize  = sizeEl.value;
  var prevQual  = qualEl.value;

  sizeEl.innerHTML = '';
  SIZES[model].forEach(function(s) {
    var o = document.createElement('option'); o.value = s; o.textContent = s; sizeEl.appendChild(o);
  });
  qualEl.innerHTML = '';
  QUALITIES[model].forEach(function(q) {
    var o = document.createElement('option'); o.value = q; o.textContent = q.charAt(0).toUpperCase() + q.slice(1); qualEl.appendChild(o);
  });

  // Restore prev values if they still exist
  if (prevSize && SIZES[model].includes(prevSize))       sizeEl.value = prevSize;
  if (prevQual && QUALITIES[model].includes(prevQual))   qualEl.value = prevQual;

  // Show/hide model-specific controls
  var isGpt = (model === 'gpt-image-1' || model === 'gpt-image-1.5');
  document.getElementById('row-background').style.display = isGpt ? '' : 'none';
  document.getElementById('row-format').style.display     = isGpt ? '' : 'none';
  document.getElementById('row-style').style.display      = isGpt ? 'none' : '';

  document.getElementById('model-badge').textContent = model;
}

function applyDefaults(cat) {
  var d = CAT_DEFAULTS[cat];
  var modelEl = document.getElementById('opt-model');
  modelEl.value = d.model;
  populateSelects(d.model);
  document.getElementById('opt-size').value       = d.size;
  document.getElementById('opt-quality').value    = d.quality;
  document.getElementById('opt-background').value = d.background;
  document.getElementById('opt-format').value     = d.format;
  document.getElementById('opt-style').value      = d.style;
}

function onModelChange() {
  var model = document.getElementById('opt-model').value;
  populateSelects(model);
}

// ────────────────────────────────────────────────────────────────────────────
// Category / chip helpers
// ────────────────────────────────────────────────────────────────────────────
function setCategory(cat) {
  _cat = cat;
  document.querySelectorAll('.tab').forEach(function(t) {
    t.classList.toggle('active', t.dataset.cat === cat);
  });
  document.getElementById('chips-ui').style.display        = cat === 'ui'        ? '' : 'none';
  document.getElementById('chips-avatar').style.display    = cat === 'avatar'    ? '' : 'none';
  document.getElementById('chips-marketing').style.display = cat === 'marketing' ? '' : 'none';
  applyDefaults(cat);
}

function addChip(text) {
  var el = document.getElementById('prompt');
  var v  = el.value.trim();
  el.value = v ? v + ', ' + text : text;
  el.focus();
}

function clearPrompt() {
  document.getElementById('prompt').value = '';
  document.getElementById('prompt').focus();
}

// ────────────────────────────────────────────────────────────────────────────
// Enhance prompt
// ────────────────────────────────────────────────────────────────────────────
function enhancePrompt() {
  var promptEl = document.getElementById('prompt');
  var raw = promptEl.value.trim();
  if (!raw) { showToast('Enter a prompt first.'); return; }

  var btn = document.getElementById('enhance-btn');
  btn.textContent = 'Enhancing...';
  btn.classList.add('busy');

  fetch('?action=enhance', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ prompt: raw, category: _cat }),
  })
  .then(function(r) { return r.json(); })
  .then(function(data) {
    if (data.error) { showToast(data.error); return; }
    promptEl.value = data.prompt;
  })
  .catch(function(e) { showToast('Enhance failed: ' + e.message); })
  .finally(function() {
    btn.textContent = 'Enhance';
    btn.classList.remove('busy');
  });
}

// ────────────────────────────────────────────────────────────────────────────
// Generate
// ────────────────────────────────────────────────────────────────────────────
function generate() {
  if (_busy) return;
  var prompt = document.getElementById('prompt').value.trim();
  if (!prompt) { showToast('Enter a prompt first.'); return; }

  _busy = true;
  var btn = document.getElementById('gen-btn');
  btn.classList.add('busy');
  document.getElementById('gen-label').textContent = 'Generating...';

  var model  = document.getElementById('opt-model').value;
  var size   = document.getElementById('opt-size').value;
  var qual   = document.getElementById('opt-quality').value;
  var bg     = document.getElementById('opt-background').value;
  var fmt    = document.getElementById('opt-format').value;
  var style  = document.getElementById('opt-style').value;

  var payload = { prompt: prompt, model: model, size: size, quality: qual, background: bg, output_format: fmt, style: style };

  fetch('?action=generate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  })
  .then(function(r) { return r.json(); })
  .then(function(data) {
    if (data.error) { showToast(data.error); return; }

    document.getElementById('empty').style.display = 'none';

    // Show revised prompt if dall-e-3 changed it
    var note = document.getElementById('revised-note');
    if (data.revised_prompt && data.revised_prompt !== prompt) {
      note.innerHTML = '<strong>Revised:</strong> ' + escHtml(data.revised_prompt);
      note.style.display = '';
    } else {
      note.style.display = 'none';
    }

    var mimeMap = { png: 'image/png', jpeg: 'image/jpeg', webp: 'image/webp' };
    var mime    = mimeMap[data.format] || 'image/png';
    var src     = 'data:' + mime + ';base64,' + data.b64;

    var entry = {
      src:      src,
      b64:      data.b64,
      format:   data.format,
      model:    data.model,
      size:     size,
      quality:  qual,
      prompt:   prompt,
      revised:  data.revised_prompt || null,
      id:       Date.now(),
    };
    _results.unshift(entry);
    renderCard(entry, true);
    updateCount();
  })
  .catch(function(e) { showToast('Request failed: ' + e.message); })
  .finally(function() {
    _busy = false;
    btn.classList.remove('busy');
    document.getElementById('gen-label').textContent = 'Generate';
  });
}

function renderCard(entry, prepend) {
  var grid = document.getElementById('results');
  var card = document.createElement('div');
  card.className = 'result-card';
  card.id = 'card-' + entry.id;

  var hasTrans = (entry.format === 'png' || entry.format === 'webp');

  card.innerHTML = [
    '<div class="result-img-wrap" onclick="openLb(\'' + escAttr(entry.src) + '\')">',
      '<img src="' + escAttr(entry.src) + '" alt="Generated image">',
    '</div>',
    '<div class="result-meta">',
      '<div class="result-info">',
        '<span>' + escHtml(entry.model) + '</span>',
        '<span>' + escHtml(entry.size)  + '</span>',
        '<span>' + escHtml(entry.quality) + '</span>',
        '<span>.' + escHtml(entry.format) + '</span>',
      '</div>',
      entry.revised
        ? '<div class="result-revised" title="' + escAttr(entry.revised) + '">' + escHtml(entry.revised) + '</div>'
        : '',
      '<div class="result-actions">',
        '<button class="r-btn primary" onclick="dlImg(' + entry.id + ')">Download</button>',
        '<button class="r-btn" onclick="copyPrompt(' + entry.id + ')">Copy prompt</button>',
        '<button class="r-btn" onclick="reuse(' + entry.id + ')">Reuse</button>',
      '</div>',
    '</div>',
  ].join('');

  if (prepend && grid.firstChild) {
    grid.insertBefore(card, grid.firstChild);
  } else {
    grid.appendChild(card);
  }
}

// ────────────────────────────────────────────────────────────────────────────
// Card actions
// ────────────────────────────────────────────────────────────────────────────
function dlImg(id) {
  var entry = _results.find(function(r) { return r.id === id; });
  if (!entry) return;
  var a = document.createElement('a');
  a.href = entry.src;
  a.download = 'image-gen-' + id + '.' + entry.format;
  a.click();
}

function copyPrompt(id) {
  var entry = _results.find(function(r) { return r.id === id; });
  if (!entry) return;
  navigator.clipboard.writeText(entry.prompt).then(function() {
    showToastOk('Prompt copied.');
  }).catch(function() {
    showToast('Copy failed.');
  });
}

function reuse(id) {
  var entry = _results.find(function(r) { return r.id === id; });
  if (!entry) return;
  document.getElementById('prompt').value = entry.prompt;
  document.getElementById('opt-model').value = entry.model;
  onModelChange();
  if (SIZES[entry.model] && SIZES[entry.model].includes(entry.size)) {
    document.getElementById('opt-size').value = entry.size;
  }
}

// ────────────────────────────────────────────────────────────────────────────
// Lightbox
// ────────────────────────────────────────────────────────────────────────────
function openLb(src) {
  document.getElementById('lb-img').src = src;
  document.getElementById('lb').classList.add('show');
}
function closeLb() {
  document.getElementById('lb').classList.remove('show');
}
document.addEventListener('keydown', function(e) {
  if (e.key === 'Escape') closeLb();
});

// ────────────────────────────────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────────────────────────────────
function updateCount() {
  var n = _results.length;
  document.getElementById('count-label').textContent = n === 0 ? '' : n + (n === 1 ? ' image' : ' images');
}

function showToast(msg) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.style.background = '#2a1717';
  t.style.borderColor = 'var(--bad)';
  t.style.color = 'var(--bad)';
  t.classList.add('show');
  clearTimeout(t._timer);
  t._timer = setTimeout(function() { t.classList.remove('show'); }, 4000);
}

function showToastOk(msg) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.style.background = '#0f2a1e';
  t.style.borderColor = 'var(--ok)';
  t.style.color = 'var(--ok)';
  t.classList.add('show');
  clearTimeout(t._timer);
  t._timer = setTimeout(function() { t.classList.remove('show'); }, 2000);
}

function escHtml(s) {
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function escAttr(s) {
  return String(s).replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}
</script>
</body>
</html>
