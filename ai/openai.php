<?php
/**
 * OpenAI Handler
 * Accepts POST with a conversation array and streams the response
 * back to the client using Server-Sent Events.
 *
 * POST body (JSON):
 *   messages    array   Required. Full conversation: [{role, content}, ...]
 *   model       string  Optional. Overrides provider default.
 *   system      string  Optional. System prompt injected as first message.
 *   max_tokens  int     Optional. Defaults to 4096.
 */

require_once __DIR__ . '/config.php';

header('Content-Type: text/event-stream');
header('Cache-Control: no-cache');
header('X-Accel-Buffering: no');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    sseError('Only POST is accepted.');
    exit;
}

$body = json_decode(file_get_contents('php://input'), true);

if (json_last_error() !== JSON_ERROR_NONE || !isset($body['messages'])) {
    sseError('Invalid JSON body. Expected { messages: [...] }');
    exit;
}

$provider  = AIConfig::provider('openai');
$apiKey    = $provider['api_key'];
$baseUrl   = rtrim($provider['base_url'], '/');
$model     = ($body['model'] ?? '') ?: $provider['default_model'];
$system    = $body['system']     ?? 'You are a helpful CSS and web development assistant.';
$maxTokens = $body['max_tokens'] ?? 4096;
$messages  = $body['messages'];

if (!is_array($messages) || count($messages) === 0) {
    sseError('messages array is empty.');
    exit;
}

// Prepend system message in OpenAI format
$openaiMessages = array_merge(
    [['role' => 'system', 'content' => $system]],
    array_map(function (array $msg): array {
        return [
            'role'    => in_array($msg['role'], ['assistant', 'user', 'system']) ? $msg['role'] : 'user',
            'content' => (string) ($msg['content'] ?? ''),
        ];
    }, $messages)
);

$payload = json_encode([
    'model'      => $model,
    'max_tokens' => (int) $maxTokens,
    'stream'     => true,
    'messages'   => $openaiMessages,
]);

$ch = curl_init($baseUrl . '/chat/completions');
curl_setopt_array($ch, [
    CURLOPT_POST           => true,
    CURLOPT_POSTFIELDS     => $payload,
    CURLOPT_RETURNTRANSFER => false,
    CURLOPT_TIMEOUT        => $provider['timeout'] ?? 120,
    CURLOPT_HTTPHEADER     => [
        'Content-Type: application/json',
        'Authorization: Bearer ' . $apiKey,
    ],
    CURLOPT_WRITEFUNCTION  => function ($curl, $chunk) {
        $lines = explode("\n", $chunk);
        foreach ($lines as $line) {
            $line = trim($line);
            if (strpos($line, 'data: ') !== 0) {
                continue;
            }
            $json = substr($line, 6);
            if ($json === '[DONE]') {
                sseDone();
                continue;
            }
            $event = json_decode($json, true);
            if (!$event) {
                continue;
            }
            $delta   = $event['choices'][0]['delta'] ?? [];
            $content = $delta['content'] ?? null;
            if ($content !== null && $content !== '') {
                sseChunk($content);
            }
            $finishReason = $event['choices'][0]['finish_reason'] ?? null;
            if ($finishReason === 'stop') {
                sseDone();
            }
        }
        return strlen($chunk);
    },
]);

curl_exec($ch);
$curlError = curl_error($ch);
$httpCode  = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

if ($curlError) {
    sseError('cURL error: ' . $curlError);
}

if ($httpCode >= 400) {
    sseError('OpenAI API returned HTTP ' . $httpCode);
}

// ---- SSE helpers --------------------------------------------------------

function sseChunk(string $text): void
{
    echo 'event: chunk' . "\n";
    echo 'data: ' . json_encode(['text' => $text]) . "\n\n";
    flush();
}

function sseDone(): void
{
    echo 'event: done' . "\n";
    echo 'data: {}' . "\n\n";
    flush();
}

function sseError(string $message): void
{
    echo 'event: error' . "\n";
    echo 'data: ' . json_encode(['error' => $message]) . "\n\n";
    flush();
}
