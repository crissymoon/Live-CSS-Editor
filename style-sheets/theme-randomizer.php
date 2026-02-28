<?php
/**
 * Style-Sheets Theme Randomizer
 *
 * Endpoint that nudges CSS custom property values with small color-harmony
 * offsets and randomizes a subset of non-color variables. When the AI model
 * generates a unique preview, the result is saved to a timestamped backup.
 *
 * GET / POST  ?mode=nudge    -- Return slightly offset CSS vars for a theme
 * GET / POST  ?mode=preview  -- Call AI, generate unique HTML, save backup
 * POST        ?mode=backup   -- Save posted CSS/HTML content with a timestamp
 *
 * Query parameters (all modes):
 *   theme    string   Theme slug, e.g. "quill-cartesian"  (required)
 *   seed     int      Optional RNG seed for reproducible results
 *
 * Preview mode POST body (JSON):
 *   vars     object   Currently applied CSS var overrides (optional)
 *   model    string   AI model override (optional)
 *
 * Backup mode POST body (JSON):
 *   content  string   HTML or CSS string to save  (required)
 *   ext      string   File extension: "html" or "css"  (default: "html")
 *   label    string   Optional label appended to filename
 *
 * All responses are JSON unless mode=preview with accept:text/html.
 *
 * Response shape (nudge):
 *   { theme, seed, vars: { "--qc-gold": "#d5ac52", ... }, duration_vars: {...} }
 *
 * Response shape (preview):
 *   { theme, backup_file, html }
 *
 * Response shape (backup):
 *   { theme, backup_file, written: true }
 */

header('Content-Type: application/json');
header('Cache-Control: no-store');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

define('THEMES_DIR',   __DIR__ . '/themes');
define('SHEETS_DIR',   __DIR__);
define('BACKUPS_DIR',  __DIR__ . '/backups');
define('AI_CONFIG',    __DIR__ . '/../ai/config.php');

// ---------------------------------------------------------------------------
// Route
// ---------------------------------------------------------------------------

$mode  = strtolower(trim($_GET['mode']  ?? 'nudge'));
$theme = trim($_GET['theme'] ?? 'quill-cartesian');
$seed  = isset($_GET['seed']) ? (int)$_GET['seed'] : null;

$body = [];
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $raw = file_get_contents('php://input');
    if ($raw) {
        $body = json_decode($raw, true) ?? [];
    }
}

switch ($mode) {
    case 'nudge':
        echo json_encode(handleNudge($theme, $seed), JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT);
        break;
    case 'preview':
        echo json_encode(handlePreview($theme, $body, $seed), JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT);
        break;
    case 'backup':
        echo json_encode(handleBackup($theme, $body), JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT);
        break;
    default:
        http_response_code(400);
        echo json_encode(['error' => 'Unknown mode. Use: nudge, preview, backup']);
}

// ---------------------------------------------------------------------------
// Mode: nudge
// ---------------------------------------------------------------------------

function handleNudge(string $theme, ?int $seed): array
{
    $themeData = loadThemeJson($theme);
    if (!$themeData) {
        http_response_code(404);
        return ['error' => "Theme JSON not found for: $theme"];
    }

    if ($seed === null) {
        $seed = mt_rand(100000, 999999);
    }
    mt_srand($seed);

    $rangeH = (int)($themeData['harmony']['nudge']['hue_range']   ?? 8);
    $rangeS = (int)($themeData['harmony']['nudge']['sat_range']   ?? 5);
    $rangeL = (int)($themeData['harmony']['nudge']['light_range'] ?? 5);

    $protected = $themeData['harmony']['protected_vars'] ?? [];
    $randVars  = $themeData['randomizable_vars']         ?? [];

    $result      = [];
    $durationResult = [];

    foreach ($randVars as $spec) {
        $varName = $spec['var'] ?? null;
        if (!$varName) { continue; }

        if (in_array($varName, $protected, true)) { continue; }

        $type = $spec['type'] ?? 'color';

        if ($type === 'color') {
            $hex = $spec['hex'] ?? null;
            if (!$hex) { continue; }
            $nudged = nudgeHex($hex, $rangeH, $rangeS, $rangeL);
            $result[$varName] = $nudged;
        } elseif ($type === 'duration') {
            $base = (float)($spec['base'] ?? 0.1);
            $min  = (float)($spec['min']  ?? $base * 0.7);
            $max  = (float)($spec['max']  ?? $base * 1.3);
            $unit = $spec['unit'] ?? 's';
            $val  = randomFloat($min, $max);
            $durationResult[$varName] = round($val, 3) . $unit;
        }
    }

    return [
        'theme'          => $theme,
        'seed'           => $seed,
        'vars'           => $result,
        'duration_vars'  => $durationResult,
        'hue_range_used' => $rangeH,
        'sat_range_used' => $rangeS,
        'light_range_used' => $rangeL,
    ];
}

// ---------------------------------------------------------------------------
// Mode: preview
// ---------------------------------------------------------------------------

function handlePreview(string $theme, array $body, ?int $seed): array
{
    if (!file_exists(AI_CONFIG)) {
        http_response_code(500);
        return ['error' => 'AI config not found. Cannot generate preview.'];
    }
    require_once AI_CONFIG;

    $themeData = loadThemeJson($theme);
    if (!$themeData) {
        http_response_code(404);
        return ['error' => "Theme JSON not found for: $theme"];
    }

    // Load the CSS file
    $cssFile = SHEETS_DIR . '/' . ($themeData['file'] ?? "$theme.css");
    if (!file_exists($cssFile)) {
        http_response_code(404);
        return ['error' => "CSS file not found: $cssFile"];
    }
    $cssContent = file_get_contents($cssFile);

    // Optionally apply nudged vars from the body
    $nudgedVars     = $body['vars']          ?? [];
    $nudgedDuration = $body['duration_vars'] ?? [];

    // Build the context block for the AI
    $varOverrideBlock = '';
    if (!empty($nudgedVars) || !empty($nudgedDuration)) {
        $varOverrideBlock = "\n\n/* RANDOMIZER OVERRIDES (applied before preview render) */\n:root {\n";
        foreach (array_merge($nudgedVars, $nudgedDuration) as $k => $v) {
            $varOverrideBlock .= "  $k: $v;\n";
        }
        $varOverrideBlock .= "}\n";
    }

    // Pull AI preview directives from the theme JSON
    $previewPrompt  = $themeData['ai_preview_prompt'] ?? [];
    $styleDirective = $previewPrompt['style_directive'] ?? 'Make the preview unique and modern.';
    $layoutHint     = $previewPrompt['layout_hint']     ?? '';
    $modernTwist    = $previewPrompt['modern_twist']    ?? '';

    $bodyClass    = $themeData['body_class']   ?? $theme;
    $prefix       = detectPrefix($themeData);
    $components   = implode(', ', array_slice($themeData['components_available'] ?? [], 0, 30));
    $uniqueComps  = implode(', ', ($themeData['unique_components'] ?? []));

    $systemPrompt = buildPreviewSystemPrompt();
    $userMessage  = buildPreviewUserMessage(
        $theme,
        $bodyClass,
        $prefix,
        $components,
        $uniqueComps,
        $styleDirective,
        $layoutHint,
        $modernTwist,
        $cssContent,
        $varOverrideBlock
    );

    // Call AI
    $model = $body['model'] ?? null;
    $html  = callAI($systemPrompt, $userMessage, $model);

    if ($html === null) {
        http_response_code(502);
        return ['error' => 'AI provider call failed. Check API key and connectivity.'];
    }

    // Extract just the HTML code block if the model wrapped it
    $html = extractHtmlBlock($html);

    // Save timestamped backup
    $backupFile = saveBackup($theme, $html, 'html', 'preview');

    return [
        'theme'       => $theme,
        'seed'        => $seed,
        'backup_file' => $backupFile,
        'html'        => $html,
    ];
}

// ---------------------------------------------------------------------------
// Mode: backup
// ---------------------------------------------------------------------------

function handleBackup(string $theme, array $body): array
{
    $content = $body['content'] ?? null;
    if (!$content) {
        http_response_code(400);
        return ['error' => '"content" field is required in POST body.'];
    }
    $ext   = preg_replace('/[^a-z]/', '', strtolower($body['ext']  ?? 'html'));
    $label = preg_replace('/[^a-z0-9_-]/', '', strtolower($body['label'] ?? ''));
    if (!in_array($ext, ['html', 'css'], true)) { $ext = 'html'; }

    $backupFile = saveBackup($theme, $content, $ext, $label);

    return [
        'theme'       => $theme,
        'backup_file' => $backupFile,
        'written'     => true,
    ];
}

// ---------------------------------------------------------------------------
// AI helpers
// ---------------------------------------------------------------------------

function buildPreviewSystemPrompt(): string
{
    return implode("\n\n", [
        "You are an expert front-end engineer and visual designer specializing in CSS design systems.",
        "You follow strict rules: no emojis, no em-dashes, no inline hardcoded colors (all colors must use CSS custom properties via var()). Return only raw HTML with no markdown fences, no explanation text, nothing outside the HTML document.",
        "Your previews are standalone HTML files that link the theme CSS via a <link> tag in <head>. They apply the correct body class. They showcase the theme's personality through bold, purposeful layout choices -- not generic card grids.",
        "Every preview must demonstrate unique, modern design thinking: unexpected but coherent layouts, deliberate use of whitespace, strong typographic hierarchy, and clear interaction between the theme's unique components.",
        "Preview HTML must be complete: <!DOCTYPE html>..<html>..<head>..<body>.. tags present. The CSS link path is relative (e.g. style-sheets/quill-cartesian.css from the project root, or just the filename if the preview will live in the same directory).",
    ]);
}

function buildPreviewUserMessage(
    string $theme,
    string $bodyClass,
    string $prefix,
    string $components,
    string $uniqueComps,
    string $styleDirective,
    string $layoutHint,
    string $modernTwist,
    string $cssContent,
    string $varOverrideBlock
): string {
    $lines = [
        "Generate a unique, modern, standalone HTML preview page for the CSS theme: $theme.",
        "",
        "BODY CLASS: Apply class=\"$bodyClass\" to the <body> element.",
        "PREFIX: All component classes use the \"$prefix\" prefix. Example: .$prefix-btn, .$prefix-card.",
        "",
        "AVAILABLE COMPONENTS (partial list): $components.",
        "UNIQUE SIGNATURE COMPONENTS (use prominently): $uniqueComps.",
        "",
        "DESIGN DIRECTIVE: $styleDirective",
        "",
        ($layoutHint ? "LAYOUT SUGGESTION: $layoutHint" : ""),
        ($modernTwist ? "MODERN TWIST: $modernTwist" : ""),
        "",
        "RULES:",
        "- Link the CSS file correctly in <head>: <link rel=\"stylesheet\" href=\"quill-cartesian.css\">",
        "- No inline style attributes with hardcoded hex values. All colors via var().",
        "- No external CDN libraries. Only the theme CSS.",
        "- The page must be visually rich and immediately demonstrate the theme character.",
        "- Do not add Lorem Ipsum filler text alone -- write contextually appropriate content that fits the theme narrative.",
        "- Return ONLY the complete raw HTML document. Nothing else. No markdown code fences.",
        "",
    ];

    if ($varOverrideBlock) {
        $lines[] = "CURRENTLY ACTIVE RANDOMIZER OVERRIDES (include as an inline <style> block in <head>):";
        $lines[] = $varOverrideBlock;
    }

    return implode("\n", array_filter($lines, fn($l) => $l !== null));
}

function callAI(string $system, string $userMsg, ?string $modelOverride): ?string
{
    try {
        $provider = AIConfig::provider('anthropic');
    } catch (\Throwable $e) {
        return null;
    }

    $model  = $modelOverride ?: ($provider['default_model'] ?? 'claude-sonnet-4-5-20250929');
    $apiKey = $provider['api_key'] ?? '';
    $apiVer = $provider['api_version'] ?? '2023-06-01';
    $base   = rtrim($provider['base_url'] ?? 'https://api.anthropic.com/v1', '/');

    $payload = json_encode([
        'model'      => $model,
        'max_tokens' => 8192,
        'system'     => $system,
        'messages'   => [
            ['role' => 'user', 'content' => $userMsg],
        ],
    ]);

    $ch = curl_init($base . '/messages');
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => $payload,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => $provider['timeout'] ?? 120,
        CURLOPT_HTTPHEADER     => [
            'Content-Type: application/json',
            "x-api-key: $apiKey",
            "anthropic-version: $apiVer",
        ],
    ]);

    $resp = curl_exec($ch);
    $err  = curl_error($ch);
    curl_close($ch);

    if ($err || !$resp) { return null; }

    $data = json_decode($resp, true);
    if (!$data) { return null; }

    // Standard Anthropic response: content[0].text
    $content = $data['content'] ?? [];
    foreach ($content as $block) {
        if (($block['type'] ?? '') === 'text') {
            return $block['text'] ?? null;
        }
    }

    return null;
}

function extractHtmlBlock(string $raw): string
{
    // If model wrapped output in ```html ... ```, unwrap it
    if (preg_match('/```html\s*([\s\S]+?)```/i', $raw, $m)) {
        return trim($m[1]);
    }
    if (preg_match('/```\s*(<!DOCTYPE|<html)/i', $raw, $m, PREG_OFFSET_CAPTURE)) {
        $offset = $m[0][1];
        $end    = strpos($raw, '```', $offset + 3);
        $chunk  = $end !== false ? substr($raw, $offset + 3, $end - $offset - 3) : substr($raw, $offset + 3);
        return trim($chunk);
    }
    return trim($raw);
}

// ---------------------------------------------------------------------------
// Backup helper
// ---------------------------------------------------------------------------

function saveBackup(string $theme, string $content, string $ext, string $label): string
{
    if (!is_dir(BACKUPS_DIR)) {
        mkdir(BACKUPS_DIR, 0755, true);
    }

    $ts       = (new DateTimeImmutable('now', new DateTimeZone('UTC')))->format('Ymd_His');
    $slug     = preg_replace('/[^a-z0-9-]/', '-', strtolower($theme));
    $labelPart = $label ? "_$label" : '';
    $filename  = "{$slug}{$labelPart}_{$ts}.{$ext}";
    $path      = BACKUPS_DIR . '/' . $filename;

    file_put_contents($path, $content);

    return $filename;
}

// ---------------------------------------------------------------------------
// Color math: Hex <-> HSL nudge
// ---------------------------------------------------------------------------

/**
 * Nudge a hex color by random offsets within the given ranges.
 * Returns the new hex color.
 */
function nudgeHex(string $hex, int $rangeH, int $rangeS, int $rangeL): string
{
    $rgb = hexToRgb($hex);
    if ($rgb === null) { return $hex; }

    [$h, $s, $l] = rgbToHsl($rgb[0], $rgb[1], $rgb[2]);

    $dh = randomInt(-$rangeH, $rangeH);
    $ds = randomInt(-$rangeS, $rangeS);
    $dl = randomInt(-$rangeL, $rangeL);

    $h = fmod($h + $dh + 360.0, 360.0);
    $s = max(0.0, min(100.0, $s + $ds));
    $l = max(0.0, min(100.0, $l + $dl));

    [$r, $g, $b] = hslToRgb($h, $s, $l);

    return rgbToHex($r, $g, $b);
}

/** Parse hex string (#rrggbb or #rgb) to [r, g, b] (0-255). */
function hexToRgb(string $hex): ?array
{
    $hex = ltrim($hex, '#');
    if (strlen($hex) === 3) {
        $hex = $hex[0].$hex[0].$hex[1].$hex[1].$hex[2].$hex[2];
    }
    if (strlen($hex) !== 6) { return null; }
    return [
        hexdec(substr($hex, 0, 2)),
        hexdec(substr($hex, 2, 2)),
        hexdec(substr($hex, 4, 2)),
    ];
}

/** Convert RGB (0-255) to HSL (H: 0-360, S: 0-100, L: 0-100). */
function rgbToHsl(int $r, int $g, int $b): array
{
    $r /= 255.0; $g /= 255.0; $b /= 255.0;
    $max = max($r, $g, $b);
    $min = min($r, $g, $b);
    $l   = ($max + $min) / 2.0;
    $d   = $max - $min;

    if ($d < 1e-10) {
        return [0.0, 0.0, $l * 100.0];
    }

    $s = $l > 0.5 ? $d / (2.0 - $max - $min) : $d / ($max + $min);

    if ($max === $r) {
        $h = fmod(($g - $b) / $d + 6.0, 6.0) / 6.0;
    } elseif ($max === $g) {
        $h = (($b - $r) / $d + 2.0) / 6.0;
    } else {
        $h = (($r - $g) / $d + 4.0) / 6.0;
    }

    return [$h * 360.0, $s * 100.0, $l * 100.0];
}

/** Convert HSL (H: 0-360, S: 0-100, L: 0-100) to RGB (0-255). */
function hslToRgb(float $h, float $s, float $l): array
{
    $h /= 360.0; $s /= 100.0; $l /= 100.0;

    if ($s < 1e-10) {
        $v = (int)round($l * 255);
        return [$v, $v, $v];
    }

    $q = $l < 0.5 ? $l * (1.0 + $s) : $l + $s - $l * $s;
    $p = 2.0 * $l - $q;

    $toRgb = function (float $t) use ($p, $q): int {
        if ($t < 0.0) { $t += 1.0; }
        if ($t > 1.0) { $t -= 1.0; }
        if ($t < 1.0/6.0) { return (int)round(($p + ($q - $p) * 6.0 * $t) * 255); }
        if ($t < 1.0/2.0) { return (int)round($q * 255); }
        if ($t < 2.0/3.0) { return (int)round(($p + ($q - $p) * (2.0/3.0 - $t) * 6.0) * 255); }
        return (int)round($p * 255);
    };

    return [$toRgb($h + 1.0/3.0), $toRgb($h), $toRgb($h - 1.0/3.0)];
}

/** Convert RGB (0-255) to lowercase hex string #rrggbb. */
function rgbToHex(int $r, int $g, int $b): string
{
    return sprintf('#%02x%02x%02x', max(0, min(255, $r)), max(0, min(255, $g)), max(0, min(255, $b)));
}

// ---------------------------------------------------------------------------
// Random helpers (use mt_rand so seed is respected)
// ---------------------------------------------------------------------------

function randomInt(int $min, int $max): int
{
    return mt_rand($min, $max);
}

function randomFloat(float $min, float $max): float
{
    return $min + mt_rand(0, 1000000) / 1000000.0 * ($max - $min);
}

// ---------------------------------------------------------------------------
// Theme JSON loader
// ---------------------------------------------------------------------------

function loadThemeJson(string $theme): ?array
{
    $slug = preg_replace('/[^a-z0-9-]/', '-', strtolower($theme));
    $path = THEMES_DIR . '/' . $slug . '.json';
    if (!file_exists($path)) { return null; }
    $data = json_decode(file_get_contents($path), true);
    return is_array($data) ? $data : null;
}

function detectPrefix(array $themeData): string
{
    // Detect the CSS var prefix from the first variable name. e.g. "--qc-gold" -> "qc"
    $vars = $themeData['variables'] ?? [];
    if (!empty($vars)) {
        // "--qc-gold" -> strip leading "--" -> "qc-gold" -> take up to first "-" -> "qc"
        $first = ltrim($vars[0] ?? '', '-');
        $dashPos = strpos($first, '-');
        if ($dashPos !== false) {
            return substr($first, 0, $dashPos);
        }
    }
    return 'qc';
}
