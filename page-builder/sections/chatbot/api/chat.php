<?php
/**
 * page-builder/sections/chatbot/api/chat.php
 *
 * Onsite chatbot endpoint. Handles a single chat turn with:
 *   - Input validation
 *   - Rate limiting  (file-based per IP, configurable via config.json)
 *   - Prompt injection guard  (calls localhost:8765/classify; fail-open)
 *   - Company context assembly from an agent-flow JSON flow
 *   - OpenAI chat completion (uses ai/config.php for keys + base URL)
 *
 * POST JSON:
 *   {
 *     message:  string,            -- the user's current message (required)
 *     flow_id:  string,            -- agent-flow flow name, no extension (optional, default: "chatbot-company-context")
 *     history:  [{role, content}]  -- previous turns, newest last (optional, max history_limit)
 *   }
 *
 * Success response:
 *   { ok: true, reply: string, flagged: false, guard_status: string }
 *
 * Error response:
 *   { ok: false, error: string, flagged: bool, guard_status: string }
 *
 * Fallback error handling: every failure path calls chatErr() which writes to
 * error_log and emits a structured JSON body so the browser always has context.
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('X-Content-Type-Options: nosniff');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

define('CHATBOT_DIR',  __DIR__ . '/..');
define('CONFIG_FILE',  CHATBOT_DIR . '/config.json');
define('FLOWS_DIR',    __DIR__ . '/../../../../agent-flow/flows');
define('AI_CONFIG',    __DIR__ . '/../../../../ai/config.php');
define('RATE_DIR',     sys_get_temp_dir() . '/pb_chatbot_rl');

// ---------------------------------------------------------------------------
// Error helper
// ---------------------------------------------------------------------------

function chatErr(string $msg, int $http = 400, bool $flagged = false, string $guardStatus = 'unknown'): void {
    http_response_code($http);
    error_log('[chatbot/api/chat.php] ' . $msg);
    echo json_encode([
        'ok'           => false,
        'error'        => $msg,
        'flagged'      => $flagged,
        'guard_status' => $guardStatus,
        'reply'        => '',
    ]);
    exit;
}

// ---------------------------------------------------------------------------
// Load config
// ---------------------------------------------------------------------------

function loadConfig(): array {
    if (!file_exists(CONFIG_FILE)) {
        error_log('[chatbot/api/chat.php] config.json not found at ' . CONFIG_FILE . ', using defaults');
        return [];
    }
    $raw = file_get_contents(CONFIG_FILE);
    if ($raw === false) {
        error_log('[chatbot/api/chat.php] could not read config.json');
        return [];
    }
    $cfg = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[chatbot/api/chat.php] config.json malformed: ' . json_last_error_msg());
        return [];
    }
    return is_array($cfg) ? $cfg : [];
}

$config = loadConfig();

$rl_window   = (int)($config['rate_limit']['window_seconds'] ?? 60);
$rl_max      = (int)($config['rate_limit']['max_requests']   ?? 15);
$guard_url   = $config['guard']['url']      ?? 'http://localhost:8765/classify';
$guard_tout  = (int)($config['guard']['timeout'] ?? 5);
$guard_fail_open = (bool)($config['guard']['fail_open'] ?? true);
$ai_provider = $config['ai']['provider']     ?? 'openai';
$ai_model    = $config['ai']['model']        ?? 'gpt-4o-mini';
$ai_tokens   = (int)($config['ai']['max_tokens']    ?? 512);
$history_lim = (int)($config['ai']['history_limit'] ?? 10);
$ai_timeout  = (int)($config['ai']['timeout']       ?? 30);

// ---------------------------------------------------------------------------
// Methods
// ---------------------------------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    chatErr('Method not allowed -- use POST', 405);
}

// ---------------------------------------------------------------------------
// Parse body
// ---------------------------------------------------------------------------

$raw = file_get_contents('php://input');
if (!$raw || !trim($raw)) {
    chatErr('Empty request body', 400);
}

$body = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    chatErr('Invalid JSON body: ' . json_last_error_msg(), 400);
}

$message = trim((string)($body['message'] ?? ''));
$flowId  = preg_replace('/[^a-zA-Z0-9_\-]/', '', (string)($body['flow_id'] ?? 'chatbot-company-context'));
$history = $body['history'] ?? [];

if ($message === '') {
    chatErr('message is required and cannot be empty', 400);
}
if (strlen($message) > 4000) {
    chatErr('message too long (max 4000 characters)', 400);
}
if (!is_array($history)) {
    $history = [];
}

// Sanitise history: keep only valid role/content pairs
$safeHistory = [];
foreach (array_slice($history, -$history_lim) as $turn) {
    $role    = $turn['role']    ?? '';
    $content = $turn['content'] ?? '';
    if (!in_array($role, ['user', 'assistant'], true)) continue;
    if (!is_string($content) || trim($content) === '') continue;
    $safeHistory[] = [
        'role'    => $role,
        'content' => substr(trim($content), 0, 4000),
    ];
}

// ---------------------------------------------------------------------------
// Rate limiting  (file-based, per hashed IP, sliding window)
// ---------------------------------------------------------------------------

$clientIp = $_SERVER['HTTP_X_FORWARDED_FOR'] ?? $_SERVER['REMOTE_ADDR'] ?? 'unknown';
// Take only the first IP if a forwarded chain is present
$clientIp = trim(explode(',', $clientIp)[0]);
$ipHash   = hash('sha256', $clientIp);
$rlFile   = RATE_DIR . '/' . $ipHash . '.json';

if (!is_dir(RATE_DIR)) {
    if (!@mkdir(RATE_DIR, 0700, true)) {
        error_log('[chatbot/api/chat.php] could not create rate-limit dir: ' . RATE_DIR);
        // Non-fatal: skip rate limiting rather than blocking all requests
    }
}

$now = time();
$rlData = ['count' => 0, 'window_start' => $now];

if (is_dir(RATE_DIR) && file_exists($rlFile)) {
    $rlRaw = @file_get_contents($rlFile);
    if ($rlRaw) {
        $parsed = json_decode($rlRaw, true);
        if (is_array($parsed)) {
            $rlData = $parsed;
        }
    }
}

// Reset window if expired
if ($now - (int)($rlData['window_start'] ?? 0) >= $rl_window) {
    $rlData = ['count' => 0, 'window_start' => $now];
}

if ((int)($rlData['count'] ?? 0) >= $rl_max) {
    $retry = $rl_window - ($now - (int)$rlData['window_start']);
    error_log('[chatbot/api/chat.php] rate limit hit for ip_hash=' . substr($ipHash, 0, 8) . ' retry_in=' . $retry . 's');
    http_response_code(429);
    echo json_encode([
        'ok'           => false,
        'error'        => 'Too many requests. Please wait ' . max(1, $retry) . ' seconds.',
        'flagged'      => false,
        'guard_status' => 'n/a',
        'reply'        => '',
        'retry_after'  => max(1, $retry),
    ]);
    exit;
}

$rlData['count'] = (int)($rlData['count'] ?? 0) + 1;
if (is_dir(RATE_DIR)) {
    @file_put_contents($rlFile, json_encode($rlData), LOCK_EX);
}

// ---------------------------------------------------------------------------
// Prompt injection guard
// ---------------------------------------------------------------------------

$guardStatus = 'skipped';
$flagged     = false;

$guardPayload = json_encode(['text' => $message]);

$ch = curl_init($guard_url);
if ($ch === false) {
    error_log('[chatbot/api/chat.php] curl_init failed for guard -- skipping guard check');
} else {
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => $guardPayload,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => $guard_tout,
        CURLOPT_CONNECTTIMEOUT => 3,
        CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
    ]);

    $guardResp    = curl_exec($ch);
    $guardCurlErr = curl_error($ch);
    $guardHttp    = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($guardCurlErr) {
        $guardStatus = 'unreachable';
        error_log('[chatbot/api/chat.php] guard server unreachable: ' . $guardCurlErr);
        if (!$guard_fail_open) {
            chatErr('Safety check temporarily unavailable. Please try again later.', 503, false, 'unreachable');
        }
    } elseif ($guardHttp >= 400) {
        $guardStatus = 'error_http_' . $guardHttp;
        error_log('[chatbot/api/chat.php] guard server HTTP ' . $guardHttp . ': ' . substr((string)$guardResp, 0, 200));
        if (!$guard_fail_open) {
            chatErr('Safety check error. Please try again later.', 502, false, $guardStatus);
        }
    } else {
        $guardData = json_decode((string)$guardResp, true);
        if (json_last_error() !== JSON_ERROR_NONE) {
            $guardStatus = 'invalid_response';
            error_log('[chatbot/api/chat.php] guard returned non-JSON: ' . substr((string)$guardResp, 0, 200));
        } else {
            $flagged     = (bool)($guardData['flagged'] ?? false);
            $guardLabel  = $guardData['label']      ?? 'unknown';
            $guardConf   = $guardData['confidence'] ?? 0;
            $guardStatus = $guardLabel . ':' . round((float)$guardConf, 3);

            if ($flagged) {
                error_log('[chatbot/api/chat.php] BLOCKED: injection detected label=' . $guardLabel . ' conf=' . $guardConf . ' msg_excerpt=' . substr($message, 0, 80));
                http_response_code(400);
                echo json_encode([
                    'ok'           => false,
                    'error'        => 'Your message was flagged as a potential security risk and was not sent.',
                    'flagged'      => true,
                    'guard_status' => $guardStatus,
                    'reply'        => '',
                ]);
                exit;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Load company context from agent-flow flow JSON
// ---------------------------------------------------------------------------

/**
 * Parse a flow JSON and extract the system prompt from context / prompt nodes.
 * "context" nodes have props.text and props.role == "system".
 * Falls back to scanning prompt nodes marked as system context.
 */
function extractSystemPromptFromFlow(string $flowId): string {
    $flowFile = FLOWS_DIR . '/' . $flowId . '.json';

    if (!file_exists($flowFile)) {
        error_log('[chatbot/api/chat.php] flow file not found: ' . $flowFile . ' -- using generic prompt');
        return 'You are a helpful customer support assistant. Be concise and friendly.';
    }

    $raw = @file_get_contents($flowFile);
    if ($raw === false) {
        error_log('[chatbot/api/chat.php] could not read flow file: ' . $flowFile);
        return 'You are a helpful customer support assistant. Be concise and friendly.';
    }

    $flow = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[chatbot/api/chat.php] flow JSON malformed (' . $flowId . '): ' . json_last_error_msg());
        return 'You are a helpful customer support assistant. Be concise and friendly.';
    }

    $nodes   = $flow['nodes'] ?? [];
    $parts   = [];

    foreach ($nodes as $node) {
        if (!is_array($node)) continue;
        $type  = $node['type']  ?? '';
        $props = $node['props'] ?? [];

        // "context" node type is dedicated to chatbot system context
        if ($type === 'context') {
            $text = trim((string)($props['text'] ?? ''));
            if ($text !== '') {
                $parts[] = $text;
            }
            continue;
        }

        // prompt nodes flagged as system context
        if ($type === 'prompt' && strtolower((string)($props['role'] ?? '')) === 'system') {
            $text = trim((string)($props['text'] ?? ''));
            if ($text !== '') {
                $parts[] = $text;
            }
        }
    }

    if (empty($parts)) {
        // Try the flow-level system_prompt key as a convenience shorthand
        $topLevel = trim((string)($flow['system_prompt'] ?? ''));
        if ($topLevel !== '') {
            return $topLevel;
        }
        error_log('[chatbot/api/chat.php] flow "' . $flowId . '" has no context/system nodes -- using generic prompt');
        return 'You are a helpful customer support assistant. Be concise and friendly.';
    }

    return implode("\n\n", $parts);
}

$systemPrompt = extractSystemPromptFromFlow($flowId);

// ---------------------------------------------------------------------------
// Load AI config
// ---------------------------------------------------------------------------

if (!file_exists(AI_CONFIG)) {
    chatErr('AI config not found at ' . AI_CONFIG . '. Check server setup.', 500, false, $guardStatus);
}

try {
    require_once AI_CONFIG;
    $providerCfg = AIConfig::provider($ai_provider);
} catch (Throwable $e) {
    error_log('[chatbot/api/chat.php] AIConfig error: ' . $e->getMessage());
    chatErr('AI configuration error: ' . $e->getMessage(), 500, false, $guardStatus);
}

$apiKey     = $providerCfg['api_key']  ?? '';
$baseUrl    = $providerCfg['base_url'] ?? 'https://api.openai.com/v1';
$aiTimeout  = $providerCfg['timeout']  ?? $ai_timeout;

if ($apiKey === '') {
    chatErr('AI API key is not configured. Check ai/config.json.', 500, false, $guardStatus);
}

// ---------------------------------------------------------------------------
// Build message array (system + history + user turn)
// ---------------------------------------------------------------------------

$messages = [['role' => 'system', 'content' => $systemPrompt]];
foreach ($safeHistory as $turn) {
    $messages[] = $turn;
}
$messages[] = ['role' => 'user', 'content' => $message];

// ---------------------------------------------------------------------------
// Call AI provider
// ---------------------------------------------------------------------------

$aiPayload = json_encode([
    'model'              => $ai_model,
    'messages'           => $messages,
    'max_completion_tokens' => $ai_tokens,
    'stream'             => false,
]);

$ch = curl_init(rtrim($baseUrl, '/') . '/chat/completions');
if ($ch === false) {
    chatErr('curl_init failed for AI call', 500, false, $guardStatus);
}

curl_setopt_array($ch, [
    CURLOPT_POST           => true,
    CURLOPT_POSTFIELDS     => $aiPayload,
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_TIMEOUT        => $aiTimeout,
    CURLOPT_CONNECTTIMEOUT => 10,
    CURLOPT_HTTPHEADER     => [
        'Content-Type: application/json',
        'Authorization: Bearer ' . $apiKey,
    ],
]);

$aiResp   = curl_exec($ch);
$aiCurlE  = curl_error($ch);
$aiHttp   = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

if ($aiCurlE) {
    error_log('[chatbot/api/chat.php] AI cURL error: ' . $aiCurlE);
    chatErr('Network error calling AI provider: ' . $aiCurlE, 502, false, $guardStatus);
}

if ($aiHttp >= 400) {
    $decoded = json_decode((string)$aiResp, true);
    $errMsg  = $decoded['error']['message'] ?? ('HTTP ' . $aiHttp . ': ' . substr((string)$aiResp, 0, 200));
    error_log('[chatbot/api/chat.php] AI provider HTTP ' . $aiHttp . ': ' . $errMsg);
    chatErr('AI provider error: ' . $errMsg, 502, false, $guardStatus);
}

$aiData = json_decode((string)$aiResp, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    error_log('[chatbot/api/chat.php] AI returned non-JSON: ' . substr((string)$aiResp, 0, 300));
    chatErr('AI returned an unexpected response format.', 502, false, $guardStatus);
}

$reply = $aiData['choices'][0]['message']['content'] ?? null;
if ($reply === null) {
    error_log('[chatbot/api/chat.php] AI response missing choices[0].message.content. Raw: ' . substr((string)$aiResp, 0, 400));
    chatErr('AI response was empty or malformed.', 502, false, $guardStatus);
}

// ---------------------------------------------------------------------------
// Success
// ---------------------------------------------------------------------------

echo json_encode([
    'ok'           => true,
    'reply'        => $reply,
    'flagged'      => false,
    'guard_status' => $guardStatus,
]);
