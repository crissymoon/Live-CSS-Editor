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

/**
 * Resolve a nav/button href using the project slug map.
 * Rules:
 *   - href starts with #, /, or contains :// -> returned as-is (anchor, abs path, full URL)
 *   - href exactly matches a key in $slugMap -> replaced with slug value
 *   - otherwise returned unchanged
 *
 * Logs a debug note when a resolution happens so developers can trace it.
 */
function resolveHref(string $href, array $slugMap): string {
    if ($href === '' || $href === '#') return $href;
    // Anchors, absolute paths, and full URLs are never touched
    if ($href[0] === '#' || $href[0] === '/') return $href;
    if (strpos($href, '://') !== false)        return $href;
    // Plain page-name reference: try slug map
    if (isset($slugMap[$href])) {
        error_log('[resolveHref] resolved "' . $href . '" -> "' . $slugMap[$href] . '"');
        return $slugMap[$href];
    }
    return $href;
}

function renderHeader(array $h, array $overrides, array $slugMap = []): string {
    $s   = mergeSettings('__header', $h['settings'] ?? [], $overrides);
    $b   = $h['brand'] ?? [];
    $nav = $h['nav']   ?? [];
    $ns  = $h['navSettings'] ?? [];

    $navBg       = $s['bg'] ?? '#1a1a2e';
    $navBorder   = $s['borderColor'] ?? 'rgba(255,255,255,0.06)';
    $headerStyle = 'background:' . $navBg . ';'
        . 'border-bottom:1px solid ' . $navBorder . ';'
        . 'position:sticky;top:0;z-index:100;'
        . '--pb-nav-bg:' . $navBg . ';'
        . '--pb-nav-border:' . $navBorder . ';';
    error_log('[renderHeader] --pb-nav-bg=' . $navBg . ' --pb-nav-border=' . $navBorder);

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
        if (!is_array($item) || empty($item['label'])) {
            error_log('[renderHeader] nav item missing label, skipping');
            continue;
        }
        $iid         = 'nav-' . strtolower(preg_replace('/[^a-z0-9]/i', '-', $item['label']));
        $itext       = overrideText($iid, $item['label'], $overrides);
        $iSettings   = mergeSettings($iid, [], $overrides);
        $isCta       = !empty($item['cta']);
        $hasDropdown = !empty($item['dropdown']) && is_array($item['dropdown']);
        $color       = $overrides[$iid]['color'] ?? ($isCta ? ($ns['ctaColor'] ?? '#6366f1') : ($ns['linkColor'] ?? '#8888a0'));
        $istyle      = 'color:' . $color . ';'
            . 'font-size:' . ($ns['fontSize'] ?? '12px') . ';'
            . 'text-decoration:none;font-family:inherit;padding:6px 12px;'
            . 'letter-spacing:0.06em;';
        if ($isCta) {
            $istyle .= 'border:1px solid ' . ($overrides[$iid]['borderColor'] ?? ($ns['ctaBorder'] ?? '#6366f1')) . ';';
        }

        $ddClass  = $hasDropdown ? ' class="pb-has-dropdown"' : '';
        $itemHref = resolveHref($item['href'] ?? '#', $slugMap);
        $liHtml  = '<a href="' . htmlspecialchars($itemHref, ENT_QUOTES) . '"'
            . $ddClass
            . ' style="' . $istyle . '"'
            . ' ' . pbAttrs($iid, ['color', 'fontSize', 'text']) . '>'
            . htmlspecialchars($itext) . '</a>';

        // Sub-menu (dropdown)
        if ($hasDropdown) {
            $ddBg       = $ns['dropdownBg']      ?? ($h['settings']['bg'] ?? '#0d0d18');
            $ddColor    = $ns['dropdownColor']   ?? ($ns['linkColor']     ?? '#8888a0');
            $ddFontSize = $ns['dropdownFontSize'] ?? ($ns['fontSize']      ?? '12px');
            $ddItems    = '';

            foreach ($item['dropdown'] as $sub) {
                if (!is_array($sub)) {
                    error_log('[renderHeader] dropdown entry is not an array, skipping');
                    continue;
                }
                // Separator divider
                if (!empty($sub['separator'])) {
                    $ddItems .= '<li style="border-top:1px solid rgba(255,255,255,0.08);margin:4px 0;" role="separator"></li>';
                    continue;
                }
                if (empty($sub['label'])) {
                    error_log('[renderHeader] dropdown item missing label, skipping');
                    continue;
                }
                $sid    = $iid . '-' . strtolower(preg_replace('/[^a-z0-9]/i', '-', $sub['label']));
                $stext  = overrideText($sid, $sub['label'], $overrides);
                $scolor = $overrides[$sid]['color'] ?? ($sub['color'] ?? $ddColor);
                $sstyle = 'color:' . $scolor . ';'
                    . 'font-size:' . $ddFontSize . ';'
                    . 'text-decoration:none;font-family:inherit;'
                    . 'letter-spacing:0.05em;';
                $subHref  = resolveHref($sub['href'] ?? '#', $slugMap);
                $ddItems .= '<li><a href="' . htmlspecialchars($subHref, ENT_QUOTES) . '"'
                    . ' style="' . $sstyle . '"'
                    . ' ' . pbAttrs($sid, ['color', 'fontSize', 'text']) . '>'
                    . htmlspecialchars($stext) . '</a></li>';
            }

            $liHtml .= '<ul class="pb-dropdown"'
                . ' style="background:' . htmlspecialchars($ddBg, ENT_QUOTES) . ';">'
                . $ddItems . '</ul>';
        }

        $navItems .= '<li>' . $liHtml . '</li>';
    }

    $navCollapseAt = (int)($h['navSettings']['navCollapseAt'] ?? 640);

    // Hamburger button - shown by pb-responsive.js via CSS class toggle
    $hamburger = '<button class="pb-hamburger" aria-label="Toggle navigation" aria-expanded="false">'
        . '<span></span><span></span><span></span>'
        . '</button>';

    return '<header data-pb-section="header" data-pb-nav-collapse="' . $navCollapseAt . '"'
        . ' ' . pbAttrs('__header', ['bg', 'borderColor', 'height']) . ' style="' . $headerStyle . '">'
        . '<div style="' . $innerStyle . '">'
        . '<a href="' . htmlspecialchars(resolveHref($b['href'] ?? '/', $slugMap), ENT_QUOTES) . '"'
        . ' style="' . $brandStyle . '"'
        . ' ' . pbAttrs('header-brand', ['color', 'fontSize', 'text']) . '>'
        . htmlspecialchars($brandText) . '</a>'
        . '<nav><ul style="list-style:none;display:flex;gap:' . ($ns['gap'] ?? '6px') . ';margin:0;padding:0;">'
        . $navItems . '</ul></nav>'
        . $hamburger
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
                /*
                 * If the button's href is "#" or empty, render as a <button type="submit">
                 * so it works as a form submit button when placed inside a <form>.
                 * Otherwise render as an <a> link as before.
                 */
                $href = $block['href'] ?? '#';
                if ($href === '#' || $href === '') {
                    return '<button type="submit"'
                        . ' style="' . $style . ';border:none;-webkit-appearance:none;-moz-appearance:none;appearance:none;"'
                        . ' ' . pbAttrs($id, ['bg', 'color', 'border', 'padding', 'fontSize', 'text']) . '>'
                        . htmlspecialchars($text) . '</button>';
                }
                return '<a href="' . htmlspecialchars($href, ENT_QUOTES) . '"'
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
            /*
             * FORM BLOCK
             * Renders a <form> element containing child blocks (inputs, textareas,
             * buttons). The form action and method are configurable. When no action
             * is provided the built-in JS handler catches the submit event and logs
             * the data to the console for debugging.
             */
            case 'form': {
                $formAction = htmlspecialchars($block['action'] ?? '', ENT_QUOTES);
                $formMethod = htmlspecialchars($block['method'] ?? 'POST', ENT_QUOTES);
                $formStyle  = 'display:flex;flex-direction:column;'
                    . 'gap:' . ($s['gap'] ?? '16px') . ';'
                    . 'width:100%;'
                    . toStyle($s, ['gap', 'maxWidth']);
                if (isset($s['maxWidth'])) {
                    $formStyle .= ';max-width:' . htmlspecialchars($s['maxWidth'], ENT_QUOTES);
                }
                $inner = '';
                foreach ($block['children'] ?? [] as $child) {
                    if (!is_array($child)) {
                        error_log('[renderBlock] form child is not an array, skipping');
                        continue;
                    }
                    $inner .= renderBlock($child, $overrides);
                }
                return '<form class="pb-form" id="' . htmlspecialchars($id, ENT_QUOTES) . '"'
                    . ' action="' . $formAction . '"'
                    . ' method="' . $formMethod . '"'
                    . ' style="' . $formStyle . '"'
                    . ' ' . pbAttrs($id, ['bg', 'padding', 'border', 'borderRadius']) . '>'
                    . $inner . '</form>';
            }
            /*
             * INPUT BLOCK
             * Renders a labeled <input> or <textarea>. Supports types: text, email,
             * tel, url, number, password, textarea, select, checkbox, hidden.
             * Each input gets a unique name derived from its block id.
             */
            case 'input': {
                $inputType   = $block['inputType'] ?? 'text';
                $inputName   = htmlspecialchars($block['name'] ?? $id, ENT_QUOTES);
                $placeholder = htmlspecialchars(
                    overrideText($id . '-placeholder', $block['placeholder'] ?? '', $overrides),
                    ENT_QUOTES
                );
                $labelText = overrideText($id, $block['label'] ?? '', $overrides);
                $required  = !empty($block['required']) ? ' required' : '';
                $inputStyle = 'width:100%;font-family:inherit;'
                    . 'background:' . ($s['bg'] ?? 'rgba(255,255,255,0.04)') . ';'
                    . 'color:' . ($s['color'] ?? '#e0e0f0') . ';'
                    . 'border:' . ($s['border'] ?? '1px solid rgba(255,255,255,0.1)') . ';'
                    . 'border-radius:' . ($s['borderRadius'] ?? '4px') . ';'
                    . 'padding:' . ($s['padding'] ?? '10px 14px') . ';'
                    . 'font-size:' . ($s['fontSize'] ?? '14px') . ';'
                    . 'letter-spacing:0.02em;'
                    . '-webkit-appearance:none;-moz-appearance:none;appearance:none;';
                $labelStyle = 'display:flex;flex-direction:column;gap:6px;width:100%;';
                $labelSpanStyle = 'color:' . ($s['labelColor'] ?? '#8888a0') . ';'
                    . 'font-size:' . ($s['labelFontSize'] ?? '12px') . ';'
                    . 'letter-spacing:0.04em;font-family:inherit;';
                $html = '<label style="' . $labelStyle . '">';
                if ($labelText !== '') {
                    $html .= '<span style="' . $labelSpanStyle . '"'
                        . ' ' . pbAttrs($id, ['labelColor', 'labelFontSize', 'text']) . '>'
                        . htmlspecialchars($labelText) . '</span>';
                }
                if ($inputType === 'textarea') {
                    $rows = (int)($block['rows'] ?? 4);
                    $html .= '<textarea name="' . $inputName . '"'
                        . ' placeholder="' . $placeholder . '"'
                        . ' rows="' . $rows . '"'
                        . ' style="' . $inputStyle . 'resize:vertical;min-height:80px;"'
                        . ' class="pb-input"'
                        . $required
                        . ' ' . pbAttrs($id, ['bg', 'color', 'border', 'borderRadius', 'padding', 'fontSize']) . '>'
                        . '</textarea>';
                } elseif ($inputType === 'select') {
                    $options = $block['options'] ?? [];
                    $html .= '<select name="' . $inputName . '"'
                        . ' style="' . $inputStyle . 'cursor:pointer;"'
                        . ' class="pb-input"'
                        . $required
                        . ' ' . pbAttrs($id, ['bg', 'color', 'border', 'borderRadius', 'padding', 'fontSize']) . '>';
                    if ($placeholder) {
                        $html .= '<option value="" disabled selected>'
                            . htmlspecialchars($placeholder) . '</option>';
                    }
                    foreach ($options as $opt) {
                        if (!is_array($opt)) {
                            error_log('[renderBlock] select option is not an array, skipping');
                            continue;
                        }
                        $html .= '<option value="' . htmlspecialchars($opt['value'] ?? '', ENT_QUOTES) . '">'
                            . htmlspecialchars($opt['label'] ?? $opt['value'] ?? '') . '</option>';
                    }
                    $html .= '</select>';
                } elseif ($inputType === 'checkbox') {
                    $cbStyle = 'display:flex;flex-direction:row;align-items:center;gap:8px;width:100%;cursor:pointer;';
                    $html = '<label style="' . $cbStyle . '">'
                        . '<input type="checkbox" name="' . $inputName . '"'
                        . ' style="width:auto;accent-color:' . ($s['accentColor'] ?? '#6366f1') . ';"'
                        . ' class="pb-input"'
                        . $required . '>'
                        . '<span style="' . $labelSpanStyle . '"'
                        . ' ' . pbAttrs($id, ['labelColor', 'labelFontSize', 'text']) . '>'
                        . htmlspecialchars($labelText) . '</span>';
                } elseif ($inputType === 'hidden') {
                    $val = htmlspecialchars($block['value'] ?? '', ENT_QUOTES);
                    return '<input type="hidden" name="' . $inputName . '" value="' . $val . '">';
                } else {
                    $safeType = in_array($inputType, ['text','email','tel','url','number','password','date']) ? $inputType : 'text';
                    $html .= '<input type="' . $safeType . '"'
                        . ' name="' . $inputName . '"'
                        . ' placeholder="' . $placeholder . '"'
                        . ' style="' . $inputStyle . '"'
                        . ' class="pb-input"'
                        . $required
                        . ' ' . pbAttrs($id, ['bg', 'color', 'border', 'borderRadius', 'padding', 'fontSize']) . '>';
                }
                $html .= '</label>';
                return $html;
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
        $gridStyle  = 'display:grid;grid-template-columns:repeat(' . $columns . ',1fr);gap:' . htmlspecialchars($gap, ENT_QUOTES) . ';';
        $blocksHtml = '<div class="pb-grid" data-pb-cols="' . $columns . '" style="' . $gridStyle . '">';
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

    // Build data-pb-responsive attribute from the JSON 'responsive' config key
    $responsiveCfg = $sec['responsive'] ?? [];
    $responsiveAttr = '';
    if (!empty($responsiveCfg) && is_array($responsiveCfg)) {
        $rJson = json_encode($responsiveCfg);
        if ($rJson) {
            $responsiveAttr = ' data-pb-responsive="' . htmlspecialchars($rJson, ENT_QUOTES) . '"';
        }
    }

    return '<section id="' . htmlspecialchars($id, ENT_QUOTES) . '" data-pb-section="' . htmlspecialchars($id, ENT_QUOTES) . '"'
        . $responsiveAttr
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
// HTML minifier  (safe - preserves <script>/<style>/<pre> content)
// ---------------------------------------------------------------------------

function minifyHtml(string $html): string {
    try {
        $preserved = [];
        $idx = 0;
        // Preserve <script>, <style>, <pre> verbatim so we never mangle JS/CSS/PRE content
        $html = preg_replace_callback(
            '/<(script|style|pre)(\s[^>]*)?>.*?<\/\1>/si',
            function ($m) use (&$preserved, &$idx) {
                $key = "\x00PB_PRESERVE_{$idx}\x00";
                $preserved[$key] = $m[0];
                $idx++;
                return $key;
            },
            $html
        );
        if ($html === null) {
            error_log('[minifyHtml] preg_replace_callback failed for preserve step, returning original');
            return func_get_args()[0];
        }
        // Remove HTML comments (keep IE conditional and downlevel-hidden blocks)
        $html = preg_replace('/<!--(?!\[if\s)(?!>)(?!<![^>]*>).*?-->/s', '', $html) ?? $html;
        // Collapse whitespace between tags
        $html = preg_replace('/>\s+</', '><', $html) ?? $html;
        // Collapse runs of spaces / tabs on the same line
        $html = preg_replace('/[ \t]{2,}/', ' ', $html) ?? $html;
        // Remove blank lines
        $html = preg_replace('/^[ \t]*[\r\n]/m', '', $html) ?? $html;
        // Restore preserved blocks
        foreach ($preserved as $k => $v) {
            $html = str_replace($k, $v, $html);
        }
        return trim($html);
    } catch (Throwable $e) {
        error_log('[minifyHtml] error: ' . $e->getMessage() . ' - returning original HTML');
        return func_get_args()[0];
    }
}

// ---------------------------------------------------------------------------
// Extract page build data from a page directory (used by stage.php)
// Returns ['html'=>string, 'title'=>string, 'bytes_raw'=>int] or throws.
// ---------------------------------------------------------------------------

function buildPageData(string $page, string $pagesRoot, array $slugMap = []): array {
    $pageDir   = $pagesRoot . '/' . $page;
    $overrides = loadOverrides($pageDir);
    $header    = null;
    $footer    = null;
    $sections  = [];
    $manifest  = $pageDir . '/page.json';

    if (file_exists($manifest)) {
        $man = loadJson($manifest);
        foreach ($man['sections'] ?? [] as $entry) {
            $type = $entry['type'] ?? 'section';
            $file = $entry['file'] ?? '';
            if (!$file || !preg_match('/^[a-z0-9_.\-]+$/i', $file)) continue;
            $fp   = $pageDir . '/' . $file;
            if (!file_exists($fp)) continue;
            $data = loadJson($fp);
            switch ($type) {
                case 'header': $header = $data; break;
                case 'footer': $footer = $data; break;
                default:       $sections[] = $data; break;
            }
        }
        if ($header === null && file_exists($pageDir . '/header.json')) {
            $header = loadJson($pageDir . '/header.json');
        }
        if ($footer === null && file_exists($pageDir . '/footer.json')) {
            $footer = loadJson($pageDir . '/footer.json');
        }
    } else {
        if (!file_exists($pageDir . '/header.json')) {
            throw new RuntimeException('header.json not found for page: ' . $page);
        }
        if (!file_exists($pageDir . '/footer.json')) {
            throw new RuntimeException('footer.json not found for page: ' . $page);
        }
        $header = loadJson($pageDir . '/header.json');
        $footer = loadJson($pageDir . '/footer.json');
        $files  = glob($pageDir . '/section-*.json') ?: [];
        natsort($files);
        foreach ($files as $sf) {
            $sections[] = loadJson($sf);
        }
    }

    if ($header === null) throw new RuntimeException('No header for page: ' . $page);
    if ($footer === null) throw new RuntimeException('No footer for page: ' . $page);

    $body = '';
    $body .= renderHeader($header, $overrides, $slugMap);
    foreach ($sections as $sec) {
        $body .= renderSection($sec, $overrides);
    }
    $body .= renderFooter($footer, $overrides);

    $title = $header['brand']['text'] ?? ucfirst($page);
    $html  = buildHtmlShell($body, $title, [
        'headerBg'      => $header['settings']['bg'] ?? '#1a1a2e',
        'navCollapseAt' => (int)($header['navSettings']['navCollapseAt'] ?? 640),
    ]);

    return ['html' => $html, 'title' => $title, 'bytes_raw' => strlen($html)];
}

// ---------------------------------------------------------------------------
// HTML shell
// ---------------------------------------------------------------------------

function buildHtmlShell(string $body, string $pageTitle, array $allSettings): string {
    $headerBg = $allSettings['headerBg'] ?? '#1a1a2e';
    $bp       = (int)($allSettings['navCollapseAt'] ?? 640); /* mobile nav breakpoint in px */
    $desc     = htmlspecialchars($allSettings['description'] ?? $pageTitle, ENT_QUOTES);
    return <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<meta name="description" content="{$desc}">
<meta name="theme-color" content="{$headerBg}">
<meta name="color-scheme" content="dark">
<title>{$pageTitle}</title>
<!-- Preconnect to font CDN used by monospace stack -->
<link rel="preconnect" href="https://fonts.bunny.net" crossorigin>
<link rel="dns-prefetch" href="https://fonts.bunny.net">
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html { scroll-behavior: smooth; -webkit-text-size-adjust: 100%; -moz-text-size-adjust: 100%; text-size-adjust: 100%; }
body { background: {$headerBg}; color: #e0e0f0; font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace; -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; text-rendering: optimizeLegibility; }
a { -webkit-transition: color 0.16s, background 0.16s, border-color 0.16s; -moz-transition: color 0.16s, background 0.16s, border-color 0.16s; transition: color 0.16s, background 0.16s, border-color 0.16s; }
a:focus-visible { outline: 2px solid #6366f1; outline-offset: 2px; }
button:focus-visible { outline: 2px solid #6366f1; outline-offset: 2px; }
ul { list-style: none; }
img { max-width: 100%; height: auto; display: block; }
/* Apply content-visibility to off-screen sections for faster paint */
[data-pb-section] + [data-pb-section] { content-visibility: auto; contain-intrinsic-size: 0 400px; }
@media (max-width: 900px) {
  .pb-grid { grid-template-columns: repeat(2, 1fr) !important; }
}
@media (max-width: {$bp}px) {
  .pb-grid { grid-template-columns: 1fr !important; }
  /*
   * SECTION PADDING OVERRIDE (mobile)
   * padding-left: -1px is technically invalid CSS. Browsers clamp
   * negative padding to 0, which effectively removes all left padding
   * on sections without needing to write 0. This works across WebKit
   * (Safari/Chrome), Gecko (Firefox), and Blink (Edge/Opera) because
   * the CSS spec says negative padding values are invalid and UAs must
   * ignore or clamp them. The result is the same as padding-left: 0.
   * DO NOT CHANGE -- tested and working on all target browsers.
   */
  [data-pb-section] { padding-left: -1px !important; padding-right: 16px !important; }
  .pb-hamburger { display: -webkit-flex !important; display: flex !important; }
  /*
   * HEADER INNER CONTAINER PADDING (mobile)
   * padding-left: -5px is technically invalid CSS. Browsers clamp
   * negative padding to 0, which pulls the brand logo flush against
   * the left edge of the header. This is intentional and works across
   * WebKit, Gecko, and Blink because the spec treats negative padding
   * as invalid (UAs clamp to 0). The net effect is identical to
   * padding-left: 0 but was discovered empirically.
   * padding-bottom: 10px adds vertical breathing room below the brand
   * and hamburger so the header does not feel cramped.
   * DO NOT CHANGE -- tested and working on all target browsers.
   */
  header[data-pb-nav-collapse] > div { padding-left: -5px !important; padding-right: 16px !important; padding-bottom: 10px !important; }
  /* !important required: the inline style on nav>ul is display:flex which
     beats stylesheet rules without !important, leaving nav visible on mobile */
  header nav > ul { display: none !important; }
  header.pb-nav-open nav > ul {
    display: -webkit-flex !important; display: flex !important; -webkit-flex-direction: column; flex-direction: column;
    position: absolute; top: 100%; left: 0; right: 0;
    /* background and border auto-inherit from header CSS variables */
    background: var(--pb-nav-bg, #0d0d18); border-bottom: 1px solid var(--pb-nav-border, rgba(99,102,241,0.25));
    padding: 4px 0; z-index: 200;
  }
  /* 16px left padding matches the brand container padding-left */
  header.pb-nav-open nav > ul > li > a { display: -webkit-flex; display: flex; -webkit-align-items: center; align-items: center; -webkit-justify-content: space-between; justify-content: space-between; padding: 8px 16px !important; border: none !important; }
  header { position: relative !important; }
  h1 { font-size: clamp(28px, 8vw, 48px) !important; line-height: 1.2 !important; }
  p  { font-size: clamp(14px, 4vw, 18px)  !important; }
  footer .footer-inner { flex-direction: column; align-items: flex-start; }
  /*
   * ACCORDION SUB-MENU indent and item padding (mobile)
   * margin: 0 0 2px 8px -- the 8px left margin indents the accordion
   * sub-menu slightly under its parent item, visually nesting it.
   * The 2px bottom margin prevents successive sub-menus from touching.
   */
  header nav > ul > li > .pb-dropdown {
    position: static !important;
    box-shadow: none !important;
    border: none !important;
    border-left: 2px solid var(--pb-nav-border, rgba(99,102,241,0.25)) !important;
    background: transparent !important;
    padding: 0 !important;
    margin: 0 0 2px 8px;
    overflow: hidden;
    max-height: 0;
    -webkit-transition: max-height 0.3s ease;
    -moz-transition: max-height 0.3s ease;
    transition: max-height 0.3s ease;
  }
  header nav > ul > li.pb-dd-open > .pb-dropdown { /* max-height set inline by JS */ }
  /*
   * SUB-MENU ITEM PADDING (mobile)
   * padding: 12px 20px -- generous tap targets for touch devices.
   * The 20px horizontal padding keeps sub-item text comfortable
   * inside the indented accordion block.
   */
  header nav > ul > li > .pb-dropdown > li > a {
    padding: 12px 20px !important;
    opacity: 0.85;
    font-size: 10px !important;
  }
  /* Chevron: visible on mobile, rotates when open */
  header nav > ul > li > a.pb-has-dropdown::after {
    display: inline-block !important;
    content: '\25BE' !important;
    font-size: 12px;
    opacity: 0.55;
    margin-left: 0;
    margin-right: 4px;
    -webkit-transition: -webkit-transform 0.22s ease;
    -moz-transition: -moz-transform 0.22s ease;
    transition: transform 0.22s ease;
    float: right;
  }
  header nav > ul > li.pb-dd-open > a.pb-has-dropdown::after {
    -webkit-transform: rotate(180deg);
    -moz-transform: rotate(180deg);
    transform: rotate(180deg);
    opacity: 0.85;
  }
}
/* Nav dropdowns ----------------------------------------------------- */
@-webkit-keyframes pbDdIn {
  from { opacity: 0; -webkit-transform: translateY(-5px); transform: translateY(-5px); }
  to   { opacity: 1; -webkit-transform: translateY(0); transform: translateY(0); }
}
@-moz-keyframes pbDdIn {
  from { opacity: 0; -moz-transform: translateY(-5px); transform: translateY(-5px); }
  to   { opacity: 1; -moz-transform: translateY(0); transform: translateY(0); }
}
@keyframes pbDdIn {
  from { opacity: 0; -webkit-transform: translateY(-5px); transform: translateY(-5px); }
  to   { opacity: 1; -webkit-transform: translateY(0); transform: translateY(0); }
}
header nav > ul > li { position: relative; }
header nav > ul > li > .pb-dropdown {
  display: none; list-style: none;
  position: absolute; top: calc(100% + 4px); left: 0;
  min-width: 190px;
  border: 1px solid rgba(99,102,241,0.22);
  padding: 6px 0; z-index: 300;
  box-shadow: 0 10px 28px rgba(0,0,0,0.48);
}
header nav > ul > li:hover > .pb-dropdown,
header nav > ul > li.pb-dd-open > .pb-dropdown { display: block; -webkit-animation: pbDdIn 0.14s ease; -moz-animation: pbDdIn 0.14s ease; animation: pbDdIn 0.14s ease; }
header nav > ul > li > a.pb-has-dropdown::after {
  content: ' ▾'; font-size: 0.75em; opacity: 0.55; margin-left: 2px;
}
header nav > ul > li > .pb-dropdown > li > a {
  display: block; padding: 8px 18px;
  white-space: nowrap; font-family: inherit;
  text-decoration: none;
  -webkit-transition: background 0.13s, color 0.13s;
  -moz-transition: background 0.13s, color 0.13s;
  transition: background 0.13s, color 0.13s;
}
header nav > ul > li > .pb-dropdown > li > a:hover { background: rgba(99,102,241,0.12); }
header nav > ul > li > .pb-dropdown > li > a:focus-visible { background: rgba(99,102,241,0.12); outline: 2px solid #6366f1; outline-offset: -2px; }
/* ------------------------------------------------------------------ */
/* Form fields                                                         */
/* ------------------------------------------------------------------ */
.pb-form {
  -webkit-box-sizing: border-box;
  -moz-box-sizing: border-box;
  box-sizing: border-box;
}
.pb-input {
  -webkit-transition: border-color 0.16s, box-shadow 0.16s;
  -moz-transition: border-color 0.16s, box-shadow 0.16s;
  transition: border-color 0.16s, box-shadow 0.16s;
  outline: none;
}
.pb-input:focus {
  border-color: #6366f1 !important;
  box-shadow: 0 0 0 2px rgba(99,102,241,0.25);
}
.pb-input:focus-visible {
  border-color: #6366f1 !important;
  box-shadow: 0 0 0 2px rgba(99,102,241,0.25);
  outline: none;
}
.pb-input::placeholder {
  color: rgba(136,136,160,0.5);
  opacity: 1; /* Firefox */
}
.pb-input::-webkit-input-placeholder { color: rgba(136,136,160,0.5); }
.pb-input::-moz-placeholder { color: rgba(136,136,160,0.5); opacity: 1; }
.pb-input:-ms-input-placeholder { color: rgba(136,136,160,0.5); }
/* Form status messages */
.pb-form-status {
  padding: 10px 14px;
  border-radius: 4px;
  font-size: 13px;
  font-family: inherit;
  letter-spacing: 0.02em;
  display: none;
}
.pb-form-status.pb-form-success {
  display: block;
  background: rgba(16,185,129,0.1);
  border: 1px solid rgba(16,185,129,0.3);
  color: #34d399;
}
.pb-form-status.pb-form-error {
  display: block;
  background: rgba(239,68,68,0.1);
  border: 1px solid rgba(239,68,68,0.3);
  color: #ef4444;
}
/* ------------------------------------------------------------------  */
</style>
</head>
<body>
{$body}
<!-- Built by page-builder/build.php | DO NOT EDIT MANUALLY -->
<script>
/* Hamburger nav toggle -- needed when pb-responsive.js is not loaded */
(function () {
  try {
    var hdr = document.querySelector('header[data-pb-nav-collapse]');
    if (!hdr) { console.warn('[pb-nav] no header with data-pb-nav-collapse found'); return; }
    var btn = hdr.querySelector('.pb-hamburger');
    if (!btn) { console.warn('[pb-nav] .pb-hamburger not found inside header'); return; }
    var collapseAt = parseInt(hdr.getAttribute('data-pb-nav-collapse') || '640', 10);
    function updateNav() {
      try {
        var w = hdr.getBoundingClientRect().width || window.innerWidth;
        var shouldShow = w < collapseAt;
        btn.style.display = shouldShow ? 'flex' : 'none';
        if (!shouldShow && hdr.classList.contains('pb-nav-open')) {
          hdr.classList.remove('pb-nav-open');
          btn.setAttribute('aria-expanded', 'false');
        }
      } catch (e) { console.error('[pb-nav] updateNav error:', e); }
    }
    btn.addEventListener('click', function () {
      try {
        var open = hdr.classList.toggle('pb-nav-open');
        btn.setAttribute('aria-expanded', String(open));
        console.log('[pb-nav] hamburger toggled, open:', open);
      } catch (e) { console.error('[pb-nav] hamburger click error:', e); }
    });
    updateNav();
    window.addEventListener('resize', updateNav);
    if (typeof ResizeObserver !== 'undefined') {
      try { new ResizeObserver(updateNav).observe(hdr); } catch (e) { console.warn('[pb-nav] ResizeObserver failed:', e); }
    }
    console.log('[pb-nav] hamburger initialized, collapseAt:', collapseAt);
  } catch (e) {
    console.error('[pb-nav] init failed:', e);
  }
})();

/* Lazy-load images: defer offscreen images */
(function () {
  try {
    if ('loading' in HTMLImageElement.prototype) {
      document.querySelectorAll('img:not([loading])').forEach(function (img) { img.setAttribute('loading', 'lazy'); });
    } else if (typeof IntersectionObserver !== 'undefined') {
      var lazyImgs = [];
      document.querySelectorAll('img:not([loading])').forEach(function (img) {
        img.dataset.pbSrc = img.src;
        img.src = '';
        lazyImgs.push(img);
      });
      var io = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
          if (e.isIntersecting) {
            try { e.target.src = e.target.dataset.pbSrc || ''; io.unobserve(e.target); } catch (ie) { console.error('[pb-lazy] swap error:', ie); }
          }
        });
      }, { rootMargin: '200px' });
      lazyImgs.forEach(function (img) { io.observe(img); });
    }
  } catch (e) { console.error('[pb-lazy] init failed:', e); }
})();

/* pb-dropdown: animated accordion on mobile, hover/click on desktop */
(function () {
  try {
    var hdrDD = document.querySelector('header[data-pb-nav-collapse]');
    var breakAt = hdrDD ? parseInt(hdrDD.getAttribute('data-pb-nav-collapse') || '640', 10) : 640;

    function isMobileDD() {
      /* Use the smaller of viewport width and header element width.
         In watcher.php the editor panel narrows the header below the
         viewport width, so checking element width gives accurate results. */
      var vw = window.innerWidth;
      var hw = hdrDD ? (hdrDD.getBoundingClientRect().width || vw) : vw;
      var effective = Math.min(vw, hw);
      return effective < breakAt;
    }

    /* Close a single <li> that has pb-dd-open, resetting max-height on mobile */
    function closeLi(li) {
      try {
        li.classList.remove('pb-dd-open');
        if (isMobileDD()) {
          var dd = li.querySelector(':scope > .pb-dropdown');
          if (dd) dd.style.maxHeight = '0';
        }
      } catch (e) { console.error('[pb-dropdown] closeLi error:', e); }
    }

    /* Close all open dropdowns, optionally skip one */
    function closeAll(skip) {
      try {
        document.querySelectorAll('header nav li.pb-dd-open').forEach(function (li) {
          if (li !== skip) closeLi(li);
        });
      } catch (e) { console.error('[pb-dropdown] closeAll error:', e); }
    }

    document.addEventListener('click', function (e) {
      try {
        var link = e.target.closest('header nav a.pb-has-dropdown');
        if (link) {
          e.preventDefault();
          var li = link.parentElement;
          var dd = li ? li.querySelector(':scope > .pb-dropdown') : null;
          var wasOpen = li && li.classList.contains('pb-dd-open');
          closeAll(null);
          if (!wasOpen) {
            if (li) li.classList.add('pb-dd-open');
            if (dd && isMobileDD()) {
              /* Force a reflow so transition fires from 0 */
              dd.style.maxHeight = '0';
              void dd.offsetHeight;
              dd.style.maxHeight = dd.scrollHeight + 'px';
              console.log('[pb-dropdown] mobile accordion open, scrollHeight:', dd.scrollHeight);
            }
            console.log('[pb-dropdown] opened:', link.textContent.trim());
          } else {
            if (li) li.classList.remove('pb-dd-open');
            if (dd && isMobileDD()) dd.style.maxHeight = '0';
          }
        } else if (!e.target.closest('header nav li.pb-dd-open')) {
          closeAll(null);
        }
      } catch (inner) {
        console.error('[pb-dropdown] click handler error:', inner);
      }
    });

    /* Keyboard: Escape closes open dropdown */
    document.addEventListener('keydown', function (e) {
      try {
        if (e.key === 'Escape') closeAll(null);
      } catch (ke) {
        console.error('[pb-dropdown] keydown handler error:', ke);
      }
    });

    /* On resize: reset inline max-height so nothing stays stuck */
    window.addEventListener('resize', function () {
      try {
        document.querySelectorAll('header nav li > .pb-dropdown').forEach(function (dd) {
          var li = dd.parentElement;
          if (!li) return;
          if (isMobileDD()) {
            /* keep accordion state consistent */
            if (!li.classList.contains('pb-dd-open')) dd.style.maxHeight = '0';
            else dd.style.maxHeight = dd.scrollHeight + 'px';
          } else {
            /* desktop: clear inline style, let CSS position:absolute handle display */
            dd.style.maxHeight = '';
          }
        });
      } catch (e) { console.error('[pb-dropdown] resize handler error:', e); }
    });

    /* Init: set max-height 0 on all dropdowns so first open animates cleanly */
    if (isMobileDD()) {
      document.querySelectorAll('header nav li > .pb-dropdown').forEach(function (dd) {
        try { dd.style.maxHeight = '0'; } catch (e) { console.error('[pb-dropdown] init maxHeight error:', e); }
      });
    }

    console.log('[pb-dropdown] initialized, breakAt:', breakAt);
  } catch (e) {
    console.error('[pb-dropdown] init failed:', e);
  }
})();

/* ------------------------------------------------------------------
 * Form submission handler
 * If the form has a non-empty action URL, submit normally via fetch.
 * Otherwise, collect the data and log it to the console for debugging.
 * A .pb-form-status element is appended to show success/error feedback.
 * ------------------------------------------------------------------ */
(function () {
  try {
    var forms = document.querySelectorAll('.pb-form');
    if (!forms.length) { console.log('[pb-form] no forms found on page'); return; }

    forms.forEach(function (form) {
      try {
        /* Append a status element if not already present */
        var status = form.querySelector('.pb-form-status');
        if (!status) {
          status = document.createElement('div');
          status.className = 'pb-form-status';
          status.setAttribute('role', 'alert');
          status.setAttribute('aria-live', 'polite');
          form.appendChild(status);
        }

        form.addEventListener('submit', function (e) {
          try {
            var action = form.getAttribute('action') || '';
            var method = (form.getAttribute('method') || 'POST').toUpperCase();
            var formData = new FormData(form);
            var dataObj = {};
            formData.forEach(function (val, key) { dataObj[key] = val; });

            console.log('[pb-form] submit id=' + form.id, 'action=' + action, 'method=' + method, dataObj);

            /* If no action URL, prevent default and just log */
            if (!action) {
              e.preventDefault();
              status.className = 'pb-form-status pb-form-success';
              status.textContent = 'Form data captured (no action URL configured). Check browser console.';
              console.log('[pb-form] No action URL -- data logged to console:', JSON.stringify(dataObj, null, 2));
              return;
            }

            /* Has action URL -- submit via fetch for better UX */
            e.preventDefault();
            status.className = 'pb-form-status';
            status.style.display = 'none';
            status.textContent = '';

            var fetchOpts = { method: method, mode: 'cors' };
            if (method === 'GET') {
              var params = new URLSearchParams(formData).toString();
              action = action + (action.indexOf('?') === -1 ? '?' : '&') + params;
            } else {
              fetchOpts.body = formData;
            }

            fetch(action, fetchOpts)
              .then(function (res) {
                console.log('[pb-form] response status:', res.status);
                if (res.ok) {
                  status.className = 'pb-form-status pb-form-success';
                  status.textContent = 'Submitted successfully.';
                  form.reset();
                } else {
                  status.className = 'pb-form-status pb-form-error';
                  status.textContent = 'Submission failed (status ' + res.status + '). Please try again.';
                  console.error('[pb-form] server returned', res.status);
                }
              })
              .catch(function (err) {
                status.className = 'pb-form-status pb-form-error';
                status.textContent = 'Network error. Please check your connection and try again.';
                console.error('[pb-form] fetch error:', err);
              });
          } catch (submitErr) {
            console.error('[pb-form] submit handler error:', submitErr);
          }
        });

        console.log('[pb-form] handler attached to form id=' + form.id);
      } catch (formErr) {
        console.error('[pb-form] error attaching handler:', formErr);
      }
    });

    console.log('[pb-form] initialized,', forms.length, 'form(s) found');
  } catch (e) {
    console.error('[pb-form] init failed:', e);
  }
})();
</script>
</body>
</html>
HTML;
}

// ---------------------------------------------------------------------------
// Main  (skipped when included by stage.php via define('PB_STAGE_INCLUDE',1))
// ---------------------------------------------------------------------------
if (!defined('PB_STAGE_INCLUDE')):

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

// ---------------------------------------------------------------------------
// Determine section order from page.json manifest (if present).
// Falls back to the original behaviour: header.json, section-*.json, footer.json.
// ---------------------------------------------------------------------------

$manifestPath = $pageDir . '/page.json';
$header = null;
$footer = null;
$sections = [];

if (file_exists($manifestPath)) {
    buildInfo('Using page.json manifest');
    $manifest = loadJson($manifestPath);

    foreach ($manifest['sections'] ?? [] as $entry) {
        $type = $entry['type'] ?? 'section';
        $file = $entry['file'] ?? '';

        if (!$file || !preg_match('/^[a-z0-9_.\-]+$/i', $file)) {
            buildInfo('  Skipping manifest entry with invalid file: ' . $file);
            continue;
        }

        $fullPath = $pageDir . '/' . $file;
        if (!file_exists($fullPath)) {
            buildInfo('  WARNING: section file not found, skipping: ' . $file);
            continue;
        }

        $data = loadJson($fullPath);
        buildInfo('  Loaded [' . $type . ']: ' . $file);

        switch ($type) {
            case 'header':
                $header = $data;
                break;
            case 'footer':
                $footer = $data;
                break;
            case 'section':
            case 'panel':
            default:
                $sections[] = $data;
                break;
        }
    }

    if ($header === null && file_exists($pageDir . '/header.json')) {
        buildInfo('  Manifest has no header; loading header.json as fallback');
        $header = loadJson($pageDir . '/header.json');
    }
    if ($footer === null && file_exists($pageDir . '/footer.json')) {
        buildInfo('  Manifest has no footer; loading footer.json as fallback');
        $footer = loadJson($pageDir . '/footer.json');
    }
} else {
    buildInfo('No page.json found; using legacy glob-based loading');

    // Load required files (original behaviour)
    if (!file_exists($pageDir . '/header.json')) {
        buildErr('header.json not found and no page.json manifest exists');
    }
    if (!file_exists($pageDir . '/footer.json')) {
        buildErr('footer.json not found and no page.json manifest exists');
    }

    $header = loadJson($pageDir . '/header.json');
    $footer = loadJson($pageDir . '/footer.json');

    $sectionFiles = glob($pageDir . '/section-*.json');
    if ($sectionFiles === false) {
        buildErr('glob() failed on: ' . $pageDir);
    }
    natsort($sectionFiles);
    foreach ($sectionFiles as $sf) {
        $sections[] = loadJson($sf);
        buildInfo('  Loaded: ' . basename($sf));
    }
}

if ($header === null) {
    buildErr('No header found to render. Add a header section via the composer or create header.json.');
}
if ($footer === null) {
    buildErr('No footer found to render. Add a footer section via the composer or create footer.json.');
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

$allSettings = [
    'headerBg'      => $header['settings']['bg'] ?? '#1a1a2e',
    'navCollapseAt' => (int)($header['navSettings']['navCollapseAt'] ?? 640),
];
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

endif; // PB_STAGE_INCLUDE
