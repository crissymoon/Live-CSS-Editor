<?php
/**
 * page-builder/build.php
 *
 * Reads JSON config files from pages/{page}/ and generates a complete HTML file
 * at pages/{page}/index.html. Each editable element gets a data-pb-id attribute
 * so the DOM watcher can target it for live edits.
 *
 * Usage (CLI):   php build.php --page=demo
 * Usage (Web):   build.php?page=demo
 */

declare(strict_types=1);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function buildErr(string $msg): void {
    if (PHP_SAPI === 'cli') {
        fwrite(STDERR, '[page-builder] ERROR: ' . $msg . PHP_EOL);
        exit(1);
    }
    header('Content-Type: application/json');
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function buildInfo(string $msg): void {
    if (PHP_SAPI === 'cli') {
        echo '[page-builder] ' . $msg . PHP_EOL;
    }
}

function loadJson(string $path): array {
    if (!file_exists($path)) {
        buildErr('File not found: ' . $path);
    }
    $raw = file_get_contents($path);
    if ($raw === false) {
        buildErr('Cannot read: ' . $path);
    }
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        buildErr('Invalid JSON in ' . $path . ': ' . json_last_error_msg());
    }
    return $data ?? [];
}

function loadOverrides(string $pageDir): array {
    $path = $pageDir . '/overrides.json';
    if (!file_exists($path)) return [];
    $raw = file_get_contents($path);
    if (!$raw) return [];
    $data = json_decode($raw, true);
    return is_array($data) ? $data : [];
}

/** Merge base settings with any saved overrides for an element ID */
function mergeSettings(string $id, array $settings, array $overrides): array {
    if (isset($overrides[$id]) && is_array($overrides[$id])) {
        foreach ($overrides[$id] as $k => $v) {
            if ($k !== 'text') { // text is a special case handled separately
                $settings[$k] = $v;
            }
        }
    }
    return $settings;
}

function overrideText(string $id, string $text, array $overrides): string {
    return $overrides[$id]['text'] ?? $text;
}

/** Convert a settings array to an inline style string */
function toStyle(array $settings, array $skip = []): string {
    // Map JSON keys to CSS properties
    $camelToKebab = [
        'bg'             => 'background',
        'borderColor'    => 'border-color',
        'borderTop'      => 'border-top',
        'fontWeight'     => 'font-weight',
        'fontSize'       => 'font-size',
        'lineHeight'     => 'line-height',
        'letterSpacing'  => 'letter-spacing',
        'textTransform'  => 'text-transform',
        'textAlign'      => 'text-align',
        'maxWidth'       => 'max-width',
        'marginBottom'   => 'margin-bottom',
        'marginTop'      => 'margin-top',
        'flexDirection'  => 'flex-direction',
    ];

    $skipKeys = array_merge(['id', 'maxWidth', 'align', 'gap', 'columns', 'borderTop'], $skip);

    $parts = [];
    foreach ($settings as $k => $v) {
        if (in_array($k, $skipKeys, true)) continue;
        $prop = $camelToKebab[$k] ?? preg_replace_callback('/[A-Z]/', fn($m) => '-' . strtolower($m[0]), $k);
        $parts[] = $prop . ': ' . htmlspecialchars((string)$v, ENT_QUOTES);
    }
    return implode('; ', $parts);
}

/** Build data-pb-* attribute string for an element */
function pbAttrs(string $id, array $editableProps): string {
    return 'data-pb-id="' . htmlspecialchars($id, ENT_QUOTES) . '"'
        . ' data-pb-props="' . htmlspecialchars(implode(',', $editableProps), ENT_QUOTES) . '"';
}

// ---------------------------------------------------------------------------
// Section renderers
// ---------------------------------------------------------------------------

function renderHeader(array $h, array $overrides): string {
    $s   = mergeSettings('__header', $h['settings'] ?? [], $overrides);
    $b   = $h['brand'] ?? [];
    $nav = $h['nav']   ?? [];
    $ns  = $h['navSettings'] ?? [];

    $headerStyle = 'background:' . ($s['bg'] ?? '#1a1a2e') . ';'
        . 'border-bottom:1px solid ' . ($s['borderColor'] ?? 'rgba(255,255,255,0.06)') . ';'
        . 'position:sticky;top:0;z-index:100;';

    $innerStyle = 'max-width:' . ($s['maxWidth'] ?? '1100px') . ';'
        . 'margin:0 auto;padding:' . ($s['padding'] ?? '0 32px') . ';'
        . 'height:' . ($s['height'] ?? '56px') . ';'
        . 'display:flex;align-items:center;justify-content:space-between;';

    $bSettings = mergeSettings('header-brand', $b['settings'] ?? $b, $overrides);
    $brandStyle = 'color:' . ($bSettings['color'] ?? '#6366f1') . ';'
        . 'font-size:' . ($bSettings['fontSize'] ?? '13px') . ';'
        . 'font-weight:' . ($bSettings['fontWeight'] ?? 'bold') . ';'
        . 'letter-spacing:' . ($bSettings['letterSpacing'] ?? '0.08em') . ';'
        . 'text-decoration:none;text-transform:uppercase;font-family:inherit;';

    $brandText = overrideText('header-brand', $b['text'] ?? 'Site', $overrides);

    $navItems = '';
    foreach ($nav as $item) {
        $iid      = 'nav-' . strtolower(preg_replace('/[^a-z0-9]/i', '-', $item['label']));
        $itext    = overrideText($iid, $item['label'], $overrides);
        $iSettings = mergeSettings($iid, [], $overrides);
        $isCta    = !empty($item['cta']);
        $color    = $overrides[$iid]['color'] ?? ($isCta ? ($ns['ctaColor'] ?? '#6366f1') : ($ns['linkColor'] ?? '#8888a0'));
        $istyle   = 'color:' . $color . ';'
            . 'font-size:' . ($ns['fontSize'] ?? '12px') . ';'
            . 'text-decoration:none;font-family:inherit;padding:6px 12px;'
            . 'letter-spacing:0.06em;';
        if ($isCta) {
            $border = $overrides[$iid]['border'] ?? 'rgba(255,255,255,0.06)';
            $istyle .= 'border:1px solid ' . ($overrides[$iid]['borderColor'] ?? ($ns['ctaBorder'] ?? '#6366f1')) . ';';
        }
        $navItems .= '<li><a href="' . htmlspecialchars($item['href'] ?? '#', ENT_QUOTES) . '"'
            . ' style="' . $istyle . '"'
            . ' ' . pbAttrs($iid, ['color', 'fontSize', 'text']) . '>'
            . htmlspecialchars($itext) . '</a></li>';
    }

    return '<header data-pb-section="header" ' . pbAttrs('__header', ['bg', 'borderColor', 'height']) . ' style="' . $headerStyle . '">'
        . '<div style="' . $innerStyle . '">'
        . '<a href="' . htmlspecialchars($b['href'] ?? '/', ENT_QUOTES) . '"'
        . ' style="' . $brandStyle . '"'
        . ' ' . pbAttrs('header-brand', ['color', 'fontSize', 'text']) . '>'
        . htmlspecialchars($brandText) . '</a>'
        . '<nav><ul style="list-style:none;display:flex;gap:' . ($ns['gap'] ?? '6px') . ';margin:0;padding:0;">'
        . $navItems . '</ul></nav>'
        . '</div></header>';
}

function renderBlock(array $block, array $overrides): string {
    $id   = $block['id'] ?? ('block-' . substr(md5(json_encode($block)), 0, 6));
    $type = $block['type'] ?? 'text';
    $s    = mergeSettings($id, $block['settings'] ?? [], $overrides);

    try {
        switch ($type) {
            case 'heading': {
                $tag   = in_array($block['tag'] ?? 'h2', ['h1','h2','h3','h4','h5','h6']) ? $block['tag'] : 'h2';
                $style = toStyle($s, ['maxWidth']);
                if (isset($s['maxWidth'])) $style .= ';max-width:' . htmlspecialchars($s['maxWidth'], ENT_QUOTES);
                $text  = overrideText($id, $block['text'] ?? '', $overrides);
                return "<{$tag} style=\"{$style}\" " . pbAttrs($id, ['color', 'fontSize', 'fontWeight', 'text']) . '>'
                    . htmlspecialchars($text) . "</{$tag}>";
            }
            case 'text': {
                $tag   = $block['tag'] ?? 'p';
                $style = toStyle($s, ['maxWidth']);
                if (isset($s['maxWidth'])) $style .= ';max-width:' . htmlspecialchars($s['maxWidth'], ENT_QUOTES);
                $text  = overrideText($id, $block['text'] ?? '', $overrides);
                return "<{$tag} style=\"{$style}\" " . pbAttrs($id, ['color', 'fontSize', 'lineHeight', 'text']) . '>'
                    . htmlspecialchars($text) . "</{$tag}>";
            }
            case 'button': {
                $style = 'display:inline-block;cursor:pointer;text-decoration:none;font-family:inherit;'
                    . toStyle($s, ['marginTop']);
                if (isset($s['marginTop'])) $style .= ';margin-top:' . htmlspecialchars($s['marginTop'], ENT_QUOTES);
                $text = overrideText($id, $block['text'] ?? 'Button', $overrides);
                return '<a href="' . htmlspecialchars($block['href'] ?? '#', ENT_QUOTES) . '"'
                    . ' style="' . $style . '"'
                    . ' ' . pbAttrs($id, ['bg', 'color', 'border', 'padding', 'fontSize', 'text']) . '>'
                    . htmlspecialchars($text) . '</a>';
            }
            case 'image': {
                // src and alt can be saved as overrides - read them directly so
                // they are not emitted into the style string by toStyle()
                $src = $overrides[$id]['src'] ?? $block['src'] ?? '';
                $alt = $overrides[$id]['alt'] ?? $block['alt'] ?? '';
                $style = toStyle($s, ['src', 'alt']);
                if (isset($s['maxWidth'])) {
                    $style .= ';max-width:' . htmlspecialchars($s['maxWidth'], ENT_QUOTES);
                }
                return '<img src="' . htmlspecialchars($src, ENT_QUOTES) . '"'
                    . ' alt="' . htmlspecialchars($alt, ENT_QUOTES) . '"'
                    . ' style="' . $style . '"'
                    . ' ' . pbAttrs($id, ['src', 'alt', 'width', 'borderRadius', 'opacity']) . '>';
            }
            case 'card': {
                $cardStyle = 'display:flex;flex-direction:column;gap:' . ($s['gap'] ?? '12px') . ';'
                    . toStyle($s, ['gap']);
                $inner = '';
                foreach ($block['children'] ?? [] as $child) {
                    $inner .= renderBlock($child, $overrides);
                }
                return '<div style="' . $cardStyle . '" ' . pbAttrs($id, ['bg', 'border', 'padding']) . '>'
                    . $inner . '</div>';
            }
            default:
                return '<!-- unknown block type: ' . htmlspecialchars($type) . ' -->';
        }
    } catch (Throwable $e) {
        error_log('[page-builder] renderBlock error for id=' . $id . ': ' . $e->getMessage());
        return '<!-- renderBlock error: ' . htmlspecialchars($e->getMessage()) . ' -->';
    }
}

function renderSection(array $sec, array $overrides): string {
    $id     = $sec['id'] ?? ('section-' . substr(md5(json_encode($sec)), 0, 6));
    $layout = $sec['layout'] ?? 'column';
    $s      = mergeSettings($id, $sec['settings'] ?? [], $overrides);

    $bg       = $s['bg']      ?? 'transparent';
    $padding  = $s['padding'] ?? '80px 32px';
    $maxWidth = $s['maxWidth'] ?? '1100px';
    $gap      = $s['gap']     ?? '32px';
    $align    = $s['align']   ?? 'flex-start';
    $columns  = (int)($s['columns'] ?? 1);

    $bt    = $s['borderTop'] ?? '';
    $btCss = $bt ? 'border-top:' . htmlspecialchars($bt, ENT_QUOTES) . ';' : '';

    $wrapStyle = 'background:' . htmlspecialchars($bg, ENT_QUOTES) . ';' . $btCss;
    $innerStyle = 'max-width:' . htmlspecialchars($maxWidth, ENT_QUOTES) . ';'
        . 'margin:0 auto;padding:' . htmlspecialchars($padding, ENT_QUOTES) . ';';

    // Optional section-level heading
    $headingHtml = '';
    if (isset($sec['heading'])) {
        $headingHtml = renderBlock(array_merge(['type' => 'heading', 'tag' => 'h2'], $sec['heading']), $overrides);
    }

    if ($layout === 'row' && $columns > 1) {
        $gridStyle = 'display:grid;grid-template-columns:repeat(' . $columns . ',1fr);gap:' . htmlspecialchars($gap, ENT_QUOTES) . ';';
        $blocksHtml = '<div style="' . $gridStyle . '">';
        foreach ($sec['blocks'] ?? [] as $block) {
            $blocksHtml .= renderBlock($block, $overrides);
        }
        $blocksHtml .= '</div>';
    } else {
        $flexStyle = 'display:flex;flex-direction:column;align-items:' . htmlspecialchars($align, ENT_QUOTES) . ';gap:' . htmlspecialchars($gap, ENT_QUOTES) . ';';
        $blocksHtml = '<div style="' . $flexStyle . '">';
        foreach ($sec['blocks'] ?? [] as $block) {
            $blocksHtml .= renderBlock($block, $overrides);
        }
        $blocksHtml .= '</div>';
    }

    return '<section id="' . htmlspecialchars($id, ENT_QUOTES) . '" data-pb-section="' . htmlspecialchars($id, ENT_QUOTES) . '"'
        . ' ' . pbAttrs($id, ['bg', 'padding']) . ' style="' . $wrapStyle . '">'
        . '<div style="' . $innerStyle . '">'
        . $headingHtml
        . $blocksHtml
        . '</div></section>';
}

function renderFooter(array $f, array $overrides): string {
    $s    = mergeSettings('__footer', $f['settings'] ?? [], $overrides);
    $copy = $f['copy']  ?? [];
    $links= $f['links'] ?? [];
    $ls   = $f['linkSettings'] ?? [];

    $bg     = $s['bg']      ?? '#16162b';
    $bt     = $s['borderTop'] ?? '1px solid rgba(255,255,255,0.06)';
    $pad    = $s['padding'] ?? '28px 32px';
    $maxW   = $s['maxWidth'] ?? '1100px';

    $wrapStyle  = 'background:' . htmlspecialchars($bg, ENT_QUOTES) . ';border-top:' . htmlspecialchars($bt, ENT_QUOTES) . ';';
    $innerStyle = 'max-width:' . htmlspecialchars($maxW, ENT_QUOTES) . ';margin:0 auto;padding:' . htmlspecialchars($pad, ENT_QUOTES) . ';'
        . 'display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px;';

    $cid      = is_array($copy) ? ($copy['id'] ?? 'footer-copy') : 'footer-copy';
    $ctext    = overrideText($cid, is_array($copy) ? ($copy['text'] ?? '') : $copy, $overrides);
    $copySettings = mergeSettings($cid, is_array($copy) ? ($copy['settings'] ?? []) : [], $overrides);
    $copyStyle = 'color:' . ($copySettings['color'] ?? '#8888a0') . ';'
        . 'font-size:' . ($copySettings['fontSize'] ?? '11px') . ';'
        . 'letter-spacing:' . ($copySettings['letterSpacing'] ?? '0.08em') . ';'
        . 'font-family:inherit;';

    $linkItems = '';
    foreach ($links as $link) {
        $lid    = $link['id'] ?? ('footer-link-' . strtolower(preg_replace('/[^a-z0-9]/i', '-', $link['label'])));
        $ltext  = overrideText($lid, $link['label'], $overrides);
        $lcolor = $overrides[$lid]['color'] ?? ($ls['color'] ?? '#8888a0');
        $lstyle = 'color:' . $lcolor . ';font-size:' . ($ls['fontSize'] ?? '11px') . ';'
            . 'letter-spacing:' . ($ls['letterSpacing'] ?? '0.06em') . ';'
            . 'text-decoration:none;padding:4px 10px;font-family:inherit;';
        $linkItems .= '<li><a href="' . htmlspecialchars($link['href'] ?? '#', ENT_QUOTES) . '"'
            . ' style="' . $lstyle . '"'
            . ' ' . pbAttrs($lid, ['color', 'fontSize', 'text']) . '>'
            . htmlspecialchars($ltext) . '</a></li>';
    }

    return '<footer data-pb-section="footer" ' . pbAttrs('__footer', ['bg', 'borderTop', 'padding']) . ' style="' . $wrapStyle . '">'
        . '<div style="' . $innerStyle . '">'
        . '<span style="' . $copyStyle . '" ' . pbAttrs($cid, ['color', 'fontSize', 'text']) . '>'
        . htmlspecialchars($ctext) . '</span>'
        . '<ul style="list-style:none;display:flex;gap:' . ($ls['gap'] ?? '4px') . ';margin:0;padding:0;">'
        . $linkItems . '</ul>'
        . '</div></footer>';
}

// ---------------------------------------------------------------------------
// HTML shell
// ---------------------------------------------------------------------------

function buildHtmlShell(string $body, string $pageTitle, array $allSettings): string {
    $headerBg = $allSettings['headerBg'] ?? '#1a1a2e';
    return <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{$pageTitle}</title>
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body { background: {$headerBg}; font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace; }
a { transition: color 0.16s, background 0.16s, border-color 0.16s; }
ul { list-style: none; }
@media (max-width: 700px) {
  nav ul { gap: 2px !important; }
  [data-pb-section="hero"] { padding: 60px 16px 48px !important; }
  [data-pb-section="features"] .pb-grid { grid-template-columns: 1fr !important; }
  footer .footer-inner { flex-direction: column; align-items: flex-start; }
}
</style>
</head>
<body>
{$body}
<!-- Built by page-builder/build.php | DO NOT EDIT MANUALLY -->
</body>
</html>
HTML;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

// Resolve page name
$page = '';
if (PHP_SAPI === 'cli') {
    foreach ($argv ?? [] as $arg) {
        if (str_starts_with($arg, '--page=')) {
            $page = substr($arg, 7);
        }
    }
} else {
    $page = $_GET['page'] ?? '';
}

if (!$page || !preg_match('/^[a-z0-9_-]+$/i', $page)) {
    buildErr('No valid --page argument. Usage: php build.php --page=demo');
}

$pageDir  = __DIR__ . '/pages/' . $page;
$outFile  = $pageDir . '/index.html';

if (!is_dir($pageDir)) {
    buildErr('Page directory not found: ' . $pageDir);
}

buildInfo("Building page: {$page}");

// Load overrides
$overrides = loadOverrides($pageDir);
buildInfo('Overrides loaded: ' . count($overrides) . ' element(s) have saved changes');

// Load required files
$header = loadJson($pageDir . '/header.json');
$footer = loadJson($pageDir . '/footer.json');

// Load sections in order (section-1.json, section-2.json, ...)
$sectionFiles = glob($pageDir . '/section-*.json');
if ($sectionFiles === false) {
    buildErr('glob() failed on: ' . $pageDir);
}
natsort($sectionFiles);
$sections = [];
foreach ($sectionFiles as $sf) {
    $sections[] = loadJson($sf);
    buildInfo('  Loaded: ' . basename($sf));
}

// Render
$body = '';
try {
    $body .= renderHeader($header, $overrides);
    foreach ($sections as $sec) {
        $body .= renderSection($sec, $overrides);
    }
    $body .= renderFooter($footer, $overrides);
} catch (Throwable $e) {
    buildErr('Render failed: ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
}

// Get page title from header brand or first section heading
$pageTitle = $header['brand']['text'] ?? ucfirst($page);

$allSettings = ['headerBg' => $header['settings']['bg'] ?? '#1a1a2e'];
$html = buildHtmlShell($body, $pageTitle, $allSettings);

// Write output
if (file_put_contents($outFile, $html) === false) {
    buildErr('Cannot write output file: ' . $outFile);
}

$size = strlen($html);
buildInfo("Done. Written " . number_format($size) . " bytes to: {$outFile}");

if (PHP_SAPI !== 'cli') {
    header('Content-Type: application/json');
    echo json_encode(['ok' => true, 'page' => $page, 'file' => $outFile, 'bytes' => $size]);
}
