<?php
require_once __DIR__ . '/config.php';
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type');
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(204); exit; }
$dbPath = __DIR__ . '/data/phrases.db';
if (!file_exists(dirname($dbPath))) { mkdir(dirname($dbPath), 0755, true); }
$db = new PDO('sqlite:' . $dbPath);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
$db->exec('CREATE TABLE IF NOT EXISTS phrases (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    text TEXT NOT NULL UNIQUE,
    category TEXT DEFAULT "general",
    use_count INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
)');
$action = $_GET['action'] ?? 'get';
if ($action === 'store' && $_SERVER['REQUEST_METHOD'] === 'POST') {
    $body     = json_decode(file_get_contents('php://input'), true) ?? [];
    $phrases  = $body['phrases']  ?? [];
    $category = $body['category'] ?? 'general';
    $stmt = $db->prepare('INSERT OR IGNORE INTO phrases (text, category) VALUES (?, ?)');
    $count = 0;
    foreach ($phrases as $text) {
        $text = trim((string)$text);
        if ($text) { $stmt->execute([$text, $category]); $count++; }
    }
    echo json_encode(['stored' => $count]);
    exit;
}
$total = (int)$db->query('SELECT COUNT(*) FROM phrases')->fetchColumn();
if ($total < 30) {
    $provider = AIConfig::provider('openai');
    $apiKey   = $provider['api_key'];
    $baseUrl  = rtrim($provider['base_url'], '/');
    $systemPrompt = 'You are a creative writer. Generate exactly 60 short conversational phrases ' .
        '(6-14 words each) that an AI assistant might display while actively working on code or design tasks. ' .
        'Cover: analyzing, exploring patterns, crafting, optimizing, discovering, refining, building. ' .
        'Phrases should feel alive and intelligent. ' .
        'Return ONLY a valid JSON array of 60 strings. No markdown, no explanation.';
    $payload = json_encode([
        'model'      => 'gpt-4o-mini',
        'max_tokens' => 2000,
        'messages'   => [
            ['role' => 'system', 'content' => $systemPrompt],
            ['role' => 'user',   'content' => 'Generate the 60 phrases now.']
        ]
    ]);
    $ch = curl_init($baseUrl . '/chat/completions');
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => $payload,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => 30,
        CURLOPT_HTTPHEADER     => [
            'Content-Type: application/json',
            'Authorization: Bearer ' . $apiKey
        ]
    ]);
    $response = curl_exec($ch);
    curl_close($ch);
    if ($response) {
        $data    = json_decode($response, true);
        $content = $data['choices'][0]['message']['content'] ?? '';
        if (preg_match('/\[[\s\S]*\]/', $content, $m)) {
            $newPhrases = json_decode($m[0], true);
            if (is_array($newPhrases)) {
                $stmt = $db->prepare('INSERT OR IGNORE INTO phrases (text, category) VALUES (?, ?)');
                foreach ($newPhrases as $text) {
                    $text = trim((string)$text);
                    if ($text) { $stmt->execute([$text, 'ai-working']); }
                }
            }
        }
    }
    $fallback = [
        'Analyzing your code structure...','Mapping the design patterns...',
        'Weaving the CSS threads together...','Exploring layout possibilities...',
        'Crafting pixel-perfect adjustments...','Running pattern recognition...',
        'Synthesizing style rules...','Tracing the cascade hierarchy...',
        'Building the response...','Inspecting component boundaries...',
        'Calculating visual weight...','Refining the color harmony...',
        'Indexing style properties...','Discovering optimization paths...',
        'Assembling the solution...','Threading through selectors...',
        'Connecting design tokens...','Resolving inherited values...',
        'Checking responsive breakpoints...','Painting the final output...',
        'Navigating the DOM tree...','Balancing specificity weights...',
        'Calibrating transition curves...','Reading the style manifest...',
        'Aligning flex containers...','Processing selector chains...'
    ];
    $stmt = $db->prepare('INSERT OR IGNORE INTO phrases (text, category) VALUES (?, ?)');
    foreach ($fallback as $text) { $stmt->execute([$text, 'fallback']); }
}
$total   = (int)$db->query('SELECT COUNT(*) FROM phrases')->fetchColumn();
$phrases = $db->query('SELECT text FROM phrases ORDER BY RANDOM() LIMIT 60')->fetchAll(PDO::FETCH_COLUMN);
echo json_encode(['phrases' => array_values($phrases), 'total' => $total]);
