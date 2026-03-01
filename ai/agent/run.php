<?php
/**
 * Agent Run Handler
 * Routes a code task to the correct AI provider, streaming if supported.
 * Providers that support streaming emit SSE. Those that do not return JSON.
 *
 * POST body (JSON):
 *   provider    string  Required. 'anthropic' | 'openai' | 'deepseek'
 *   model       string  Optional.
 *   task        string  Required. Key from prompts.json tasks.
 *   file_path   string  Required. Path of the file being edited.
 *   content     string  Required. Current file content.
 *   instruction string  Optional. Additional user instruction.
 *   messages    array   Optional. Prior conversation turns for this session.
 */

require_once __DIR__ . '/../config.php';
require_once __DIR__ . '/context/context.php';

$promptsCfg = json_decode(file_get_contents(__DIR__ . '/prompts.json'), true);

// Streaming providers list
const STREAMING_PROVIDERS = ['anthropic', 'openai', 'deepseek'];

// -------------------------------------------------------------------------
// CORS + method guard
// -------------------------------------------------------------------------

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonError('Only POST is accepted.', 405);
}

$body = json_decode(file_get_contents('php://input'), true);

if (json_last_error() !== JSON_ERROR_NONE) {
    jsonError('Malformed JSON body.');
}

$provider    = $body['provider']    ?? 'anthropic';
$task        = $body['task']        ?? 'fix';
$filePath    = $body['file_path']   ?? '';
$content     = $body['content']     ?? '';
$instruction = $body['instruction'] ?? '';
$model       = $body['model']       ?? '';
$messages    = $body['messages']    ?? [];
$mode        = $body['mode']        ?? 'repair';

if ($filePath === '') {
    jsonError('file_path is required.');
}

// For preview task, content can be empty (we use CSS outline instead)
if ($content === '' && $task !== 'preview') {
    jsonError('content is required for non-preview tasks.');
}

// -------------------------------------------------------------------------
// Build system + user prompt (mode-aware)
// -------------------------------------------------------------------------

// Check for mode-specific system/persona overrides
$modeConfig = $promptsCfg['agent_modes'][$mode] ?? null;
$system     = ($modeConfig ? $modeConfig['system'] : null) ?? $promptsCfg['system'] ?? '';
$persona    = ($modeConfig ? $modeConfig['persona'] : null) ?? $promptsCfg['persona'] ?? '';
$taskPrompt = $promptsCfg['tasks'][$task] ?? $promptsCfg['tasks']['fix'];

// Check model override for this task.
// When a model_override is defined in prompts.json for this task it always
// wins, regardless of what the client sent. This ensures preview always
// uses claude-haiku-4-5-20251001 even when the UI has a different model selected.
$modelOverride = $promptsCfg['model_overrides'][$task] ?? '';
if ($modelOverride !== '') {
    $model = $modelOverride;
}

$ctxText    = AgentContext::buildPromptContext($filePath);
$ctxInject  = str_replace('{context}', $ctxText, $promptsCfg['context_injection'] ?? '{context}');

// -------------------------------------------------------------------------
// Temporal and cultural context injection
// Reads the client timezone from the X-Timezone header (sent by the JS
// modules) and Accept-Language for cultural region inference.
// -------------------------------------------------------------------------

$temporalBlock = buildTemporalBlock($promptsCfg);

$systemFull = implode("\n\n", array_filter([$system, $persona, $temporalBlock, $ctxInject]));

$lang   = strtolower(pathinfo($filePath, PATHINFO_EXTENSION));
$userMsgParts = array_filter([
    $promptsCfg['reasoning_prefix'] ?? '',
    $taskPrompt,
    $instruction,
    "File: `$filePath`",
]);
if ($content !== '') {
    $userMsgParts[] = "```$lang\n$content\n```";
}
$userMsg = implode("\n\n", $userMsgParts);

// Append current request to context
AgentContext::addRequest($filePath, $task, $instruction ?: $taskPrompt, "$provider:$model");
AgentContext::detectConnections($filePath, $content);

// Build conversation
$conversation = $messages;
$conversation[] = ['role' => 'user', 'content' => $userMsg];

// -------------------------------------------------------------------------
// Streaming path
// -------------------------------------------------------------------------

$supportsStream = in_array($provider, STREAMING_PROVIDERS, true);

if ($supportsStream) {
    header('Content-Type: text/event-stream');
    header('Cache-Control: no-cache');
    header('X-Accel-Buffering: no');
    // Disable PHP output buffering so SSE events reach the client immediately.
    ob_implicit_flush(true);
    if (ob_get_level() > 0) { ob_end_clean(); }
    runStreaming($provider, $model, $systemFull, $conversation);
} else {
    header('Content-Type: application/json');
    $result = runBlocking($provider, $model, $systemFull, $conversation);
    echo json_encode($result);
}
exit;

// -------------------------------------------------------------------------
// Implementation
// -------------------------------------------------------------------------

function runStreaming(
    string $provider,
    string $model,
    string $system,
    array  $conversation
): void {
    $residualBuf = '';
    $cfg         = AIConfig::provider($provider);
    $apiKey  = $cfg['api_key'];
    $baseUrl = rtrim($cfg['base_url'], '/');
    $model   = $model ?: $cfg['default_model'];

    $payload = buildPayload($provider, $model, $system, $conversation, true);

    $ch = curl_init(endpoint($provider, $baseUrl));
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => json_encode($payload),
        CURLOPT_RETURNTRANSFER => false,
        CURLOPT_TIMEOUT        => $cfg['timeout'] ?? 120,
        CURLOPT_HTTPHEADER     => headers($provider, $apiKey, $cfg),
        CURLOPT_WRITEFUNCTION  => streamHandler($provider, $residualBuf),
    ]);

    curl_exec($ch);
    $err  = curl_error($ch);
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    // Flush any residual SSE bytes that were not terminated by \n\n.
    if (trim($residualBuf) !== '') {
        $residualParts = explode("\n\n", $residualBuf);
        foreach ($residualParts as $block) {
            processStreamBlock($provider, $block);
        }
    }

    if ($err)         { sseError('cURL: ' . $err); return; }
    if ($code >= 400) { sseError('API HTTP ' . $code); return; }

    // Fallback: guarantee the client always receives a done event even when
    // message_stop / finish_reason was missed due to buffering. The JS
    // client guards against double-processing with its finalized flag.
    sseDone();
}

function runBlocking(
    string $provider,
    string $model,
    string $system,
    array  $conversation
): array {
    $cfg     = AIConfig::provider($provider);
    $apiKey  = $cfg['api_key'];
    $baseUrl = rtrim($cfg['base_url'], '/');
    $model   = $model ?: $cfg['default_model'];
    $payload = buildPayload($provider, $model, $system, $conversation, false);

    $ch = curl_init(endpoint($provider, $baseUrl));
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => json_encode($payload),
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => $cfg['timeout'] ?? 120,
        CURLOPT_HTTPHEADER     => headers($provider, $apiKey, $cfg),
    ]);

    $raw  = curl_exec($ch);
    $err  = curl_error($ch);
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($err)       { return ['error' => 'cURL: ' . $err]; }
    if ($code >= 400) { return ['error' => 'API HTTP ' . $code, 'raw' => $raw]; }

    $data = json_decode($raw, true);
    $text = extractBlockingText($provider, $data);
    return ['text' => $text, 'provider' => $provider, 'model' => $model, 'stream' => false];
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

function endpoint(string $provider, string $baseUrl): string
{
    return match ($provider) {
        'anthropic' => $baseUrl . '/messages',
        default     => $baseUrl . '/chat/completions',
    };
}

function headers(string $provider, string $apiKey, array $cfg): array
{
    return match ($provider) {
        'anthropic' => [
            'Content-Type: application/json',
            'x-api-key: ' . $apiKey,
            'anthropic-version: ' . ($cfg['api_version'] ?? '2023-06-01'),
        ],
        default => [
            'Content-Type: application/json',
            'Authorization: Bearer ' . $apiKey,
        ],
    };
}

function buildPayload(
    string $provider,
    string $model,
    string $system,
    array  $conversation,
    bool   $stream
): array {
    $base = ['model' => $model, 'stream' => $stream, 'max_tokens' => 8192];

    if ($provider === 'anthropic') {
        return array_merge($base, [
            'system'   => $system,
            'messages' => array_map(fn($m) => [
                'role'    => $m['role'] === 'assistant' ? 'assistant' : 'user',
                'content' => $m['content'],
            ], $conversation),
        ]);
    }

    // OpenAI-compatible (openai, deepseek)
    $msgs = array_merge(
        [['role' => 'system', 'content' => $system]],
        array_map(fn($m) => [
            'role'    => in_array($m['role'], ['assistant', 'user', 'system']) ? $m['role'] : 'user',
            'content' => $m['content'],
        ], $conversation)
    );
    return array_merge($base, ['messages' => $msgs]);
}

/**
 * Process a single SSE block from the upstream provider and emit the
 * appropriate sseChunk / sseDone / sseError event to the browser.
 */
function processStreamBlock(string $provider, string $block): void
{
    foreach (explode("\n", $block) as $line) {
        $line = trim($line);
        if (!str_starts_with($line, 'data: ')) { continue; }
        $json = substr($line, 6);
        if ($json === '[DONE]') { sseDone(); continue; }
        $ev = json_decode($json, true);
        if (!$ev) { continue; }

        if ($provider === 'anthropic') {
            $type = $ev['type'] ?? '';
            if ($type === 'content_block_delta') {
                $delta = $ev['delta'] ?? [];
                if (($delta['type'] ?? '') === 'text_delta') {
                    sseChunk($delta['text'] ?? '');
                }
            } elseif ($type === 'message_stop') {
                sseDone();
            } elseif ($type === 'error') {
                sseError($ev['error']['message'] ?? 'Anthropic error');
            }
        } else {
            $delta   = $ev['choices'][0]['delta'] ?? [];
            $content = $delta['content']          ?? null;
            $reason  = $delta['reasoning_content'] ?? null;
            if ($reason !== null && $reason !== '') { sseReasoning($reason); }
            if ($content !== null && $content !== '') { sseChunk($content); }
            if (($ev['choices'][0]['finish_reason'] ?? null) === 'stop') { sseDone(); }
        }
    }
}

function streamHandler(string $provider, string &$residual): callable
{
    return function ($curl, $chunk) use ($provider, &$residual) {
        $residual .= $chunk;
        $parts    = explode("\n\n", $residual);
        $residual = array_pop($parts);

        foreach ($parts as $block) {
            processStreamBlock($provider, $block);
        }
        return strlen($chunk);
    };
}

function extractBlockingText(string $provider, ?array $data): string
{
    if (!$data) { return ''; }
    if ($provider === 'anthropic') {
        return $data['content'][0]['text'] ?? '';
    }
    return $data['choices'][0]['message']['content'] ?? '';
}

// -------------------------------------------------------------------------
// Temporal context builder
// -------------------------------------------------------------------------

/**
 * Build a grounding block from the temporal_context and location_context
 * templates in prompts.json.  Values are resolved from the HTTP request:
 *   timezone   -- X-Timezone header (IANA string sent by JS modules)
 *   locale     -- Accept-Language header
 *
 * Returns an empty string if no template is defined.
 */
function buildTemporalBlock(array $cfg): string
{
    $tplTemporal  = $cfg['temporal_context']  ?? '';
    $tplLocation  = $cfg['location_context']  ?? '';
    if ($tplTemporal === '' && $tplLocation === '') { return ''; }

    // Resolve timezone from client header
    $validZones   = array_flip(DateTimeZone::listIdentifiers());
    $rawTZ        = trim($_SERVER['HTTP_X_TIMEZONE'] ?? 'UTC');
    $tzId         = isset($validZones[$rawTZ]) ? $rawTZ : 'UTC';
    $tz           = new DateTimeZone($tzId);
    $now          = new DateTimeImmutable('now', $tz);

    $offsetH      = (int) round($now->getOffset() / 3600, 0);
    $offsetStr    = $offsetH >= 0 ? '+' . $offsetH : (string) $offsetH;

    // Resolve locale and cultural area from Accept-Language
    $acceptLang   = $_SERVER['HTTP_ACCEPT_LANGUAGE'] ?? 'en-US';
    $primaryTag   = strtolower(explode(',', explode(';', $acceptLang)[0])[0]);
    $parts        = explode('-', $primaryTag);
    $langCode     = $parts[0];
    $regionCode   = strtoupper($parts[1] ?? '');
    $cultural     = resolveTemporalCulturalArea($langCode, $regionCode);
    $direction    = resolveTemporalDirection($langCode);

    // Time of day band
    $hour     = (int) $now->format('G');
    $timeOfDay = $hour >= 5 && $hour < 12 ? 'Morning'
               : ($hour >= 12 && $hour < 17 ? 'Afternoon'
               : ($hour >= 17 && $hour < 21 ? 'Evening' : 'Night'));

    // Season
    $month     = (int) $now->format('n');
    $southern  = in_array($regionCode, ['AU','NZ','ZA','AR','BR','CL','BO','PE','EC','CO','PY','UY'], true);
    if ($southern) {
        $season = ($month >= 12 || $month <= 2) ? 'Summer'
                : ($month <= 5 ? 'Autumn' : ($month <= 8 ? 'Winter' : 'Spring'));
    } else {
        $season = ($month >= 12 || $month <= 2) ? 'Winter'
                : ($month <= 5 ? 'Spring' : ($month <= 8 ? 'Summer' : 'Autumn'));
    }

    $tokens = [
        '{date_long}'     => $now->format('l, F j, Y'),
        '{time_24}'       => $now->format('H:i'),
        '{time_of_day}'   => $timeOfDay,
        '{timezone}'      => $tzId . ' (UTC' . $offsetStr . ')',
        '{season}'        => $season,
        '{locale}'        => $primaryTag,
        '{cultural_area}' => $cultural,
        '{direction}'     => $direction,
        '{region}'        => $regionCode ?: 'US',
    ];

    $parts = [];
    if ($tplTemporal !== '') {
        $parts[] = strtr($tplTemporal, $tokens);
    }
    if ($tplLocation !== '') {
        $parts[] = strtr($tplLocation, $tokens);
    }

    return implode("\n\n", $parts);
}

/**
 * Mirror of the client-side resolveLocaleRegion function in js/ai-chat.js.
 */
function resolveTemporalCulturalArea(string $lang, string $region): string
{
    static $eastAsian  = ['zh', 'ja', 'ko'];
    static $southAsian = ['hi', 'ur', 'bn', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa'];
    static $rtlLangs   = ['ar', 'he', 'fa', 'ps', 'dv', 'yi'];
    static $nordic     = ['sv', 'no', 'nb', 'nn', 'da', 'fi', 'is', 'et', 'lv', 'lt'];
    static $slavic     = ['ru', 'pl', 'cs', 'sk', 'uk', 'bg', 'sr', 'hr', 'sl', 'mk', 'bs'];
    static $latin      = ['es', 'pt', 'fr', 'it', 'ro', 'ca', 'gl'];
    static $germanic   = ['de', 'nl', 'af', 'lb'];
    static $latamCarib = ['MX','GT','BZ','HN','SV','NI','CR','PA','CU','DO','PR','JM','TT'];
    static $latamSouth = ['AR','BO','BR','CL','CO','EC','PE','PY','UY','VE'];
    static $australas  = ['AU','NZ'];

    if (in_array($lang, $eastAsian,  true)) { return 'East Asia'; }
    if (in_array($lang, $southAsian, true)) { return 'South Asia'; }
    if (in_array($lang, $rtlLangs,   true)) { return 'Middle East / North Africa'; }
    if (in_array($lang, $nordic,     true)) { return 'Northern Europe'; }
    if (in_array($lang, $slavic,     true)) { return 'Eastern Europe'; }
    if (in_array($lang, $germanic,   true)) { return 'Western Europe / Germanic'; }
    if (in_array($lang, $latin, true)) {
        if (in_array($region, $latamCarib, true)) { return 'Latin America / Caribbean'; }
        if (in_array($region, $latamSouth, true)) { return 'Latin America / South'; }
        return 'Western Europe / Latin';
    }
    if (in_array($region, $australas, true)) { return 'Australasia'; }
    if ($region === 'IN') { return 'South Asia'; }
    return 'North America / English';
}

/**
 * Return writing direction for a language code.
 */
function resolveTemporalDirection(string $lang): string
{
    static $rtl = ['ar', 'he', 'fa', 'ur', 'ps', 'dv', 'yi'];
    return in_array($lang, $rtl, true) ? 'rtl' : 'ltr';
}

function sseChunk(string $text): void
{
    echo 'event: chunk' . "\n" . 'data: ' . json_encode(['text' => $text]) . "\n\n";
    flush();
}

function sseReasoning(string $text): void
{
    echo 'event: reasoning' . "\n" . 'data: ' . json_encode(['text' => $text]) . "\n\n";
    flush();
}

function sseDone(): void
{
    echo 'event: done' . "\n" . 'data: {}' . "\n\n";
    flush();
}

function sseError(string $msg): void
{
    echo 'event: error' . "\n" . 'data: ' . json_encode(['error' => $msg]) . "\n\n";
    flush();
}

function jsonError(string $msg, int $code = 400): never
{
    http_response_code($code);
    header('Content-Type: application/json');
    echo json_encode(['error' => $msg]);
    exit;
}
