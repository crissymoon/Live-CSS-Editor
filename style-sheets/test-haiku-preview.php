<?php
/**
 * Haiku 4.5 Preview Test Script
 * Tests the full pipeline: CSS outline -> fuzzy search -> Haiku 4.5 preview generation.
 * Run from CLI: php test-haiku-preview.php [theme.css]
 *
 * Default: tests against neon-grid.css
 *
 * Evaluates:
 *   1. CSS outline extraction works correctly
 *   2. Fuzzy search returns relevant results
 *   3. Haiku 4.5 generates valid preview HTML
 *   4. Preview HTML uses only linked CSS (no inline colors)
 *   5. Preview HTML applies the correct body class
 *   6. Preview HTML uses theme classes correctly
 */

require_once __DIR__ . '/css-outline.php';
require_once __DIR__ . '/fuzzy-search.php';
require_once __DIR__ . '/../ai/config.php';

// -------------------------------------------------------------------------
// Config
// -------------------------------------------------------------------------

$targetCSS   = $argv[1] ?? 'neon-grid.css';
$baseDir     = __DIR__;
$configFile  = __DIR__ . '/../ai/config.json';
$verbose     = in_array('--verbose', $argv ?? [], true) || in_array('-v', $argv ?? [], true);
$dryRun      = in_array('--dry-run', $argv ?? [], true);

// -------------------------------------------------------------------------
// Logger
// -------------------------------------------------------------------------

class TestLogger {
    private string $logFile;
    private float  $startTime;
    private array  $entries = [];
    private int    $passCount = 0;
    private int    $failCount = 0;

    public function __construct(string $baseDir, string $targetCSS) {
        $this->logFile   = $baseDir . '/test-preview-debug.log';
        $this->startTime = microtime(true);
        // Fresh log each run
        file_put_contents($this->logFile, '');
        $this->section('Test Run Started');
        $this->write('target_css', $targetCSS);
        $this->write('timestamp', date('Y-m-d H:i:s'));
        $this->write('php_version', PHP_VERSION);
        $this->write('cwd', getcwd());
    }

    /** Elapsed seconds since start, 4 decimal places. */
    private function elapsed(): string {
        return number_format(microtime(true) - $this->startTime, 4);
    }

    /** Write a key=value line to the log. */
    public function write(string $key, string $value): void {
        $line = "[" . $this->elapsed() . "s] $key = $value";
        $this->entries[] = $line;
        file_put_contents($this->logFile, $line . "\n", FILE_APPEND);
    }

    /** Write a section header. */
    public function section(string $title): void {
        $border = str_repeat('-', 60);
        $block  = "\n$border\n  $title\n$border";
        $this->entries[] = $block;
        file_put_contents($this->logFile, $block . "\n", FILE_APPEND);
    }

    /** Log a multi-line block (prompts, responses, HTML). */
    public function block(string $label, string $content, int $maxLen = 0): void {
        $display = ($maxLen > 0 && strlen($content) > $maxLen)
            ? substr($content, 0, $maxLen) . "\n... (truncated, full length: " . strlen($content) . ")"
            : $content;
        $block = "[" . $this->elapsed() . "s] --- $label (" . strlen($content) . " chars) ---\n$display\n--- end $label ---";
        $this->entries[] = $block;
        file_put_contents($this->logFile, $block . "\n", FILE_APPEND);
    }

    /** Log a PASS result. */
    public function pass(string $msg): void {
        $this->passCount++;
        $this->write('PASS', $msg);
    }

    /** Log a FAIL result. */
    public function fail(string $msg): void {
        $this->failCount++;
        $this->write('FAIL', $msg);
    }

    /** Log an INFO note. */
    public function info(string $msg): void {
        $this->write('INFO', $msg);
    }

    /** Log raw curl diagnostics. */
    public function curlInfo(array $info): void {
        $this->section('cURL Diagnostics');
        $keys = [
            'http_code', 'total_time', 'namelookup_time', 'connect_time',
            'starttransfer_time', 'size_download', 'speed_download',
            'primary_ip', 'ssl_verify_result',
        ];
        foreach ($keys as $k) {
            if (isset($info[$k])) {
                $this->write("curl.$k", (string)$info[$k]);
            }
        }
    }

    /** Write final summary. */
    public function summary(): void {
        $this->section('Final Summary');
        $total = $this->passCount + $this->failCount;
        $this->write('total_checks', (string)$total);
        $this->write('passed', (string)$this->passCount);
        $this->write('failed', (string)$this->failCount);
        $this->write('elapsed_total', $this->elapsed() . 's');
        $this->write('log_file', $this->logFile);
    }

    public function getLogFile(): string { return $this->logFile; }
    public function getPassCount(): int  { return $this->passCount; }
    public function getFailCount(): int  { return $this->failCount; }
}

$log = new TestLogger($baseDir, $targetCSS);

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

function pass(string $msg): void {
    global $log; $log->pass($msg);
    echo "\033[32m  PASS\033[0m $msg\n";
}
function fail(string $msg): void {
    global $log; $log->fail($msg);
    echo "\033[31m  FAIL\033[0m $msg\n";
}
function info(string $msg): void {
    global $log; $log->info($msg);
    echo "\033[36m  INFO\033[0m $msg\n";
}
function heading(string $msg): void {
    global $log; $log->section($msg);
    echo "\n\033[1m== $msg ==\033[0m\n";
}

// -------------------------------------------------------------------------
// Phase 1: CSS Outline Extraction
// -------------------------------------------------------------------------

heading("Phase 1: CSS Outline Extraction ($targetCSS)");

$cssPath = $baseDir . '/' . $targetCSS;
if (!file_exists($cssPath)) {
    fail("File not found: $cssPath");
    exit(1);
}

$css     = file_get_contents($cssPath);
$outline = CSSOutline::extract($css, $targetCSS);

// Validate structure
$requiredKeys = ['file', 'prefix', 'body_class', 'sections', 'variables', 'classes', 'at_rules', 'stats'];
foreach ($requiredKeys as $key) {
    if (array_key_exists($key, $outline)) {
        pass("Outline has key: $key");
    } else {
        fail("Outline missing key: $key");
    }
}

// Validate stats
$stats = $outline['stats'];
if ($stats['variables'] > 0) {
    pass("Found {$stats['variables']} variables");
} else {
    fail("No variables found");
}

if ($stats['classes'] > 0) {
    pass("Found {$stats['classes']} classes");
} else {
    fail("No classes found");
}

if (!empty($outline['sections'])) {
    pass("Found " . count($outline['sections']) . " sections");
} else {
    fail("No sections found");
}

if ($outline['prefix'] !== '') {
    pass("Detected prefix: {$outline['prefix']}");
} else {
    fail("No prefix detected");
}

if ($outline['body_class'] !== null) {
    pass("Detected body class: {$outline['body_class']}");
} else {
    info("No body class detected (might be clean-system)");
}

$outlineText = CSSOutline::toText($outline);
info("Outline text length: " . strlen($outlineText) . " chars");

// Log full outline for debugging
$log->block('outline_text', $outlineText, 3000);
$log->write('outline_vars', (string)$stats['variables']);
$log->write('outline_classes', (string)$stats['classes']);
$log->write('outline_sections', (string)count($outline['sections']));
$log->write('outline_prefix', $outline['prefix']);
$log->write('outline_body_class', $outline['body_class'] ?? '(none)');

if ($verbose) {
    echo "\n--- Outline Text (first 500 chars) ---\n";
    echo substr($outlineText, 0, 500) . "\n---\n";
}

// -------------------------------------------------------------------------
// Phase 2: Fuzzy Search
// -------------------------------------------------------------------------

heading("Phase 2: Fuzzy Search");

$testQueries = [
    'button primary',
    'card shadow',
    'navigation bar',
    'modal overlay',
    'color palette',
];

foreach ($testQueries as $query) {
    $results = FuzzySearch::search($query, $baseDir);
    $total = 0;
    foreach ($results as $items) { $total += count($items); }
    $log->write("fuzzy_query[$query]", "$total results");
    if ($total > 0) {
        pass("Query '$query': $total results");
    } else {
        fail("Query '$query': no results");
    }
}

// Theme-specific search
$themeName = pathinfo($targetCSS, PATHINFO_FILENAME);
$themeResults = FuzzySearch::search($themeName, $baseDir);
$themeTotal = 0;
foreach ($themeResults as $items) { $themeTotal += count($items); }
if ($themeTotal > 0) {
    pass("Theme search '$themeName': $themeTotal results");
} else {
    fail("Theme search '$themeName': no results");
}

// -------------------------------------------------------------------------
// Phase 3: Haiku 4.5 Preview Generation (API call)
// -------------------------------------------------------------------------

heading("Phase 3: Haiku 4.5 Preview Generation");

if ($dryRun) {
    info("Dry run mode -- skipping API call");
    info("Would send outline + fuzzy search context to Haiku 4.5");
    info("Outline: " . strlen($outlineText) . " chars");
    $searchContext = FuzzySearch::toText(FuzzySearch::search("component preview $themeName", $baseDir));
    info("Search context: " . strlen($searchContext) . " chars");
    heading("Dry Run Complete");
    exit(0);
}

// Load API config
if (!file_exists($configFile)) {
    fail("Config file not found: $configFile");
    exit(1);
}

$config = json_decode(file_get_contents($configFile), true);
$keyPath = ($config['keys_path'] ?? '') . '/' . ($config['key_files']['anthropic'] ?? '');

if (!file_exists($keyPath)) {
    fail("Anthropic key file not found: $keyPath");
    info("Skipping API test. Run with --dry-run to test everything except API.");
    exit(1);
}

$apiKey = trim(file_get_contents($keyPath));
if (empty($apiKey)) {
    fail("Anthropic API key is empty");
    exit(1);
}
pass("API key loaded");

// Build the prompt
$searchContext = FuzzySearch::toText(FuzzySearch::search("component preview $themeName", $baseDir));

// Load rules.json for context
$rulesJson = '';
$rulesPath = $baseDir . '/rules.json';
if (file_exists($rulesPath)) {
    $rulesJson = file_get_contents($rulesPath);
}

$systemPrompt = "You are a CSS preview generator. You create HTML preview pages for CSS themes. "
    . "The HTML must:\n"
    . "1. Link the CSS file via <link rel=\"stylesheet\" href=\"{$targetCSS}\">\n"
    . "2. Apply the correct body class: body class=\"{$outline['body_class']}\"\n"
    . "3. Use ONLY the theme's own classes from the outline below\n"
    . "4. NO inline styles with hardcoded hex colors\n"
    . "5. Show variable color swatches using var() references\n"
    . "6. Demonstrate each component found in the CSS (buttons, cards, inputs, etc.)\n"
    . "7. Keep it clean, minimal, well-organized\n\n"
    . "No emojis. No em-dashes. No contractions.\n\n"
    . "Rules reference:\n" . substr($rulesJson, 0, 2000) . "\n";

$userPrompt = "Generate a preview HTML page for this CSS theme.\n\n"
    . "CSS Outline:\n" . $outlineText . "\n\n"
    . "Related search context:\n" . $searchContext . "\n\n"
    . "Return ONLY the complete HTML file inside a fenced code block. No explanation.";

info("System prompt: " . strlen($systemPrompt) . " chars");
info("User prompt: " . strlen($userPrompt) . " chars");
info("Calling Haiku 4.5...");

// Log the full prompts for debugging
$log->block('system_prompt', $systemPrompt);
$log->block('user_prompt', $userPrompt);

$model = 'claude-haiku-4-5-20251001';
$payload = [
    'model'      => $model,
    'max_tokens' => 8192,
    'messages'   => [
        ['role' => 'user', 'content' => $userPrompt],
    ],
    'system' => $systemPrompt,
];

$ch = curl_init('https://api.anthropic.com/v1/messages');
curl_setopt_array($ch, [
    CURLOPT_POST           => true,
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_TIMEOUT        => 120,
    CURLOPT_HTTPHEADER     => [
        'Content-Type: application/json',
        'x-api-key: ' . $apiKey,
        'anthropic-version: 2023-06-01',
    ],
    CURLOPT_POSTFIELDS => json_encode($payload),
]);

$startTime = microtime(true);
$response  = curl_exec($ch);
$curlError = curl_error($ch);
$curlErrno = curl_errno($ch);
$httpCode  = curl_getinfo($ch, CURLINFO_HTTP_CODE);
$curlInfo  = curl_getinfo($ch);
$elapsed   = round(microtime(true) - $startTime, 2);
curl_close($ch);

info("Response time: {$elapsed}s, HTTP: $httpCode");

// Log curl diagnostics
$log->curlInfo($curlInfo);
if ($curlError) {
    $log->write('curl_error', "[$curlErrno] $curlError");
}
$log->write('api_http_code', (string)$httpCode);
$log->write('api_response_length', (string)strlen($response));

// Log raw response (truncated for sanity)
$log->block('api_raw_response', $response, 5000);

if ($httpCode !== 200) {
    fail("API returned HTTP $httpCode");
    $log->block('api_error_response', $response);
    if ($verbose) {
        echo "Response: " . substr($response, 0, 500) . "\n";
    }
    exit(1);
}

$data = json_decode($response, true);
if (!$data || !isset($data['content'][0]['text'])) {
    fail("Invalid API response structure");
    exit(1);
}

$text = $data['content'][0]['text'];
pass("Got response: " . strlen($text) . " chars");

// Log parsed API fields
$log->write('api_model', $data['model'] ?? 'unknown');
$log->write('api_id', $data['id'] ?? 'unknown');
$log->write('api_stop_reason', $data['stop_reason'] ?? 'unknown');
$log->write('api_text_length', (string)strlen($text));

// Log the full generated text
$log->block('api_generated_text', $text);

// Check for truncation
$stopReason = $data['stop_reason'] ?? 'unknown';
if ($stopReason === 'max_tokens') {
    fail("Response was TRUNCATED (hit max_tokens). Increase max_tokens or simplify the prompt.");
    $log->write('TRUNCATION', 'Last 200 chars: ' . substr($text, -200));
} else {
    pass("Response completed fully (stop_reason: $stopReason)");
}

// Extract usage stats
$usage = $data['usage'] ?? [];
if (!empty($usage)) {
    $inputTokens  = $usage['input_tokens'] ?? 0;
    $outputTokens = $usage['output_tokens'] ?? 0;
    $cost = ($inputTokens / 1000) * 0.001 + ($outputTokens / 1000) * 0.005;
    info("Tokens: {$inputTokens} in / {$outputTokens} out, est. cost: \$" . number_format($cost, 5));
    $log->write('tokens_input', (string)$inputTokens);
    $log->write('tokens_output', (string)$outputTokens);
    $log->write('tokens_cost_usd', number_format($cost, 5));
}

// -------------------------------------------------------------------------
// Phase 4: Evaluate the generated HTML
// -------------------------------------------------------------------------

heading("Phase 4: Evaluate Generated HTML");

// Extract HTML from code block
if (preg_match('/```(?:html)?\s*\n([\s\S]*?)\n```/', $text, $m)) {
    $html = $m[1];
    pass("Extracted HTML from code block: " . strlen($html) . " chars");
    $log->write('html_extraction', 'code_block');
} else {
    // Try raw text
    $html = $text;
    info("No code block found, using raw response");
    $log->write('html_extraction', 'raw_text_fallback');
}
$log->write('html_length', (string)strlen($html));
$log->block('html_first_500', substr($html, 0, 500));
$log->block('html_last_500', substr($html, -500));

// Check 1: Links the CSS file
if (preg_match('/<link[^>]+href=["\']' . preg_quote($targetCSS, '/') . '["\']/', $html)) {
    pass("Links CSS file: $targetCSS");
} elseif (str_contains($html, $targetCSS)) {
    pass("References CSS file (non-standard link): $targetCSS");
} else {
    fail("Does not link CSS file: $targetCSS");
}

// Check 2: Body class
if ($outline['body_class']) {
    if (str_contains($html, $outline['body_class'])) {
        pass("Uses body class: {$outline['body_class']}");
    } else {
        fail("Missing body class: {$outline['body_class']}");
    }
}

// Check 3: No hardcoded hex colors in inline styles
$inlineHexCount = 0;
preg_match_all('/style="[^"]*#[0-9a-fA-F]{3,8}[^"]*"/', $html, $hexMatches);
$inlineHexCount = count($hexMatches[0]);
if ($inlineHexCount === 0) {
    pass("No hardcoded hex colors in inline styles");
} else {
    fail("Found $inlineHexCount inline style(s) with hardcoded hex colors");
    foreach ($hexMatches[0] as $i => $match) {
        $log->write("bad_inline_color[$i]", $match);
        if ($verbose) {
            echo "    $match\n";
        }
    }
}

// Check 4: Uses theme classes
$prefix = $outline['prefix'];
$themeClassCount = 0;
if ($prefix) {
    preg_match_all('/class="[^"]*\b' . preg_quote($prefix, '/') . '-\w+/', $html, $classMatches);
    $themeClassCount = count($classMatches[0]);
}
if ($themeClassCount > 5) {
    pass("Uses $themeClassCount theme-prefixed class references");
} elseif ($themeClassCount > 0) {
    info("Uses $themeClassCount theme-prefixed class references (low count)");
} else {
    fail("No theme-prefixed classes found in HTML");
}

// Check 5: Uses CSS variables (var(--...))
preg_match_all('/var\(--[a-zA-Z0-9_-]+\)/', $html, $varMatches);
$varUseCount = count($varMatches[0]);
info("Uses $varUseCount CSS variable references in HTML");

// Check 6: Valid HTML structure
if (str_contains($html, '<!DOCTYPE') || str_contains($html, '<html')) {
    pass("Has HTML document structure");
} else {
    fail("Missing HTML document structure");
}

// Check 7: HTML is complete (has closing tags)
if (str_contains($html, '</html>') && str_contains($html, '</body>')) {
    pass("HTML is complete (has closing html and body tags)");
} else {
    fail("HTML appears truncated (missing </html> or </body>)");
}

// -------------------------------------------------------------------------
// Summary
// -------------------------------------------------------------------------

heading("Summary");
info("Theme: $targetCSS (prefix: $prefix)");
info("Outline: {$stats['variables']} vars, {$stats['classes']} classes, " . count($outline['sections']) . " sections");
info("API: {$elapsed}s, $model");
info("HTML: " . strlen($html) . " chars, $themeClassCount theme classes, $inlineHexCount bad inline colors");

// Save the generated HTML for manual inspection
// Strip any remaining markdown code fences
$htmlClean = $html;
$htmlClean = preg_replace('/^```(?:html)?\s*\n/', '', $htmlClean);
$htmlClean = preg_replace('/\n```\s*$/', '', $htmlClean);

$outFile = $baseDir . '/test-preview-output.html';
file_put_contents($outFile, $htmlClean);
info("Saved generated HTML to: $outFile");

// Write final summary to log
$log->write('output_file', $outFile);
$log->write('output_file_size', (string)strlen($htmlClean));
$log->summary();

echo "\n";
echo "\033[36m  LOG \033[0m " . $log->getLogFile() . "\n";
if ($log->getFailCount() > 0) {
    echo "\033[31m  " . $log->getFailCount() . " FAILED\033[0m -- check the log for details\n";
}
echo "\n";
