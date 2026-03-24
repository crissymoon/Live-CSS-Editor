<?php
require_once __DIR__ . '/auth.php';
require_auth();
header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'error' => 'Method not allowed']);
    exit;
}

$raw = file_get_contents('php://input');
$data = json_decode($raw ?: '{}', true);
if (!is_array($data)) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Invalid JSON payload']);
    exit;
}

$mode = strtolower(trim((string)($data['mode'] ?? 'chat')));
$prompt = trim((string)($data['prompt'] ?? ''));
$context = trim((string)($data['context'] ?? ''));

if ($prompt === '') {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Prompt is required']);
    exit;
}

$modelMap = [
    'chat' => 'gpt-4o-mini',
    'render' => 'gpt-4o',
];
$model = $modelMap[$mode] ?? $modelMap['chat'];

$keyFileCandidates = [];
$envKeyPath = getenv('OPENAI_API_KEY_FILE');
if (is_string($envKeyPath) && $envKeyPath !== '') {
    $keyFileCandidates[] = $envKeyPath;
}
$keyFileCandidates[] = 'C:\\Users\\criss\\Desktop\\keys\\openai_api_key.txt';

$apiKey = '';
$keyPathUsed = '';
foreach ($keyFileCandidates as $candidate) {
    if (!is_string($candidate) || $candidate === '') continue;
    if (!file_exists($candidate)) continue;
    $read = file_get_contents($candidate);
    if ($read === false) continue;
    $trimmed = trim($read);
    if ($trimmed === '') continue;
    $apiKey = $trimmed;
    $keyPathUsed = $candidate;
    break;
}

if ($apiKey === '') {
    http_response_code(500);
    echo json_encode([
        'ok' => false,
        'error' => 'OpenAI API key not found. Set OPENAI_API_KEY_FILE or place key in Desktop/keys file outside repo.'
    ]);
    exit;
}

$systemPrompt = 'You assist a no-code page builder admin panel. Keep responses concise, actionable, and safe for public web content. For render mode, return suggestions that improve responsive layout and deploy-ready output.';

$messages = [
    ['role' => 'system', 'content' => $systemPrompt],
    ['role' => 'user', 'content' => "Mode: {$mode}\nContext:\n{$context}\n\nPrompt:\n{$prompt}"],
];

$payload = [
    'model' => $model,
    'messages' => $messages,
    'temperature' => $mode === 'render' ? 0.2 : 0.4,
];

$ch = curl_init('https://api.openai.com/v1/chat/completions');
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Content-Type: application/json',
    'Authorization: Bearer ' . $apiKey,
]);
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload));
curl_setopt($ch, CURLOPT_TIMEOUT, 30);

$resp = curl_exec($ch);
$errno = curl_errno($ch);
$err = curl_error($ch);
$status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

if ($errno !== 0 || $resp === false) {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'AI request failed: ' . $err]);
    exit;
}

$parsed = json_decode($resp, true);
if (!is_array($parsed)) {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'AI response parse error']);
    exit;
}

if ($status >= 400) {
    $apiError = $parsed['error']['message'] ?? ('OpenAI HTTP ' . $status);
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => $apiError]);
    exit;
}

$output = '';
if (!empty($parsed['choices'][0]['message']['content'])) {
    $output = (string)$parsed['choices'][0]['message']['content'];
}

echo json_encode([
    'ok' => true,
    'model' => $model,
    'output' => $output,
    'meta' => [
        'mode' => $mode,
        'key_path' => $keyPathUsed,
    ],
]);
