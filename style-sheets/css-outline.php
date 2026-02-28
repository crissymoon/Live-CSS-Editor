<?php
/**
 * CSS Outline Generator
 * Extracts a structured outline from a CSS file for model consumption.
 * Returns: sections, custom properties (grouped), class names, at-rules.
 *
 * Usage:
 *   $outline = CSSOutline::extract(file_get_contents('neon-grid.css'), 'neon-grid.css');
 *   echo json_encode($outline, JSON_PRETTY_PRINT);
 *
 * Or from CLI:
 *   php css-outline.php neon-grid.css
 */

class CSSOutline
{
    /**
     * Extract a full structured outline from CSS content.
     */
    public static function extract(string $css, string $filename = ''): array
    {
        $prefix   = self::detectPrefix($css, $filename);
        $sections = self::extractSections($css);
        $vars     = self::extractVariables($css);
        $classes  = self::extractClasses($css);
        $atRules  = self::extractAtRules($css);
        $stats    = self::stats($css, $vars, $classes);

        return [
            'file'       => $filename,
            'prefix'     => $prefix,
            'body_class' => self::detectBodyClass($css),
            'sections'   => $sections,
            'variables'  => $vars,
            'classes'    => $classes,
            'at_rules'   => $atRules,
            'stats'      => $stats,
        ];
    }

    /**
     * Detect the theme prefix from variable names.
     */
    private static function detectPrefix(string $css, string $filename): string
    {
        // Try from variables: most common --XX- prefix
        preg_match_all('/--([a-z]{2,4})-/', $css, $m);
        if (!empty($m[1])) {
            $counts = array_count_values($m[1]);
            arsort($counts);
            // Skip generic prefixes
            $skip = ['clr', 'sz', 'type', 'dur', 'ease'];
            foreach ($counts as $pfx => $cnt) {
                if (!in_array($pfx, $skip, true)) {
                    return $pfx;
                }
            }
        }
        // Fallback: derive from filename
        if ($filename) {
            $base = pathinfo($filename, PATHINFO_FILENAME);
            $parts = explode('-', $base);
            if (count($parts) === 1) {
                return substr($parts[0], 0, 3);
            }
            return implode('', array_map(fn($p) => $p[0] ?? '', $parts));
        }
        return '';
    }

    /**
     * Detect the body class scope (e.g., body.dark-neu).
     */
    private static function detectBodyClass(string $css): ?string
    {
        if (preg_match('/body\.([a-z][a-z0-9-]+)\s*\{/', $css, $m)) {
            return $m[1];
        }
        return null;
    }

    /**
     * Extract section headings from block comments.
     * Looks for patterns like:
     *   /* ============= SECTION TITLE ============= *​/
     *   /* --- Section Title --- *​/
     *   /* SECTION TITLE *​/  (all caps, at least 3 chars)
     */
    private static function extractSections(string $css): array
    {
        $sections = [];
        $lines = explode("\n", $css);
        foreach ($lines as $i => $line) {
            $trimmed = trim($line);
            // Pattern: /* ==== TITLE ==== */
            if (preg_match('/\/\*\s*[=\-]{3,}\s*$/m', $trimmed)) {
                // Title is usually the next line
                if (isset($lines[$i + 1])) {
                    $next = trim($lines[$i + 1]);
                    $next = trim($next, "/* \t");
                    if ($next !== '' && !preg_match('/^[=\-]+$/', $next)) {
                        $sections[] = [
                            'name' => $next,
                            'line' => $i + 2,
                        ];
                    }
                }
            }
            // Single-line section comment: /* SECTION TITLE */
            if (preg_match('/^\/\*\s*([A-Z][A-Z0-9 &\/-]{2,})\s*\*\/\s*$/', $trimmed, $m)) {
                $sections[] = [
                    'name' => trim($m[1]),
                    'line' => $i + 1,
                ];
            }
        }
        return $sections;
    }

    /**
     * Extract CSS custom properties grouped by category.
     */
    private static function extractVariables(string $css): array
    {
        $vars = [];
        preg_match_all('/(--[a-zA-Z0-9_-]+)\s*:\s*([^;]+);/', $css, $matches, PREG_SET_ORDER);
        foreach ($matches as $m) {
            $name  = $m[1];
            $value = trim($m[2]);
            // Categorize
            $cat = self::categorizeVariable($name, $value);
            $vars[$cat][] = [
                'name'  => $name,
                'value' => $value,
            ];
        }
        return $vars;
    }

    /**
     * Categorize a variable based on name suffix and value.
     */
    private static function categorizeVariable(string $name, string $value): string
    {
        // Color detection: hex, rgb, hsl, named colors
        $isColor = preg_match('/#[0-9a-fA-F]{3,8}\b/', $value)
                || preg_match('/\b(rgb|hsl)a?\s*\(/', $value)
                || preg_match('/\b(transparent|currentColor)\b/i', $value);

        if ($isColor || preg_match('/-(bg|surface|text|primary|accent|success|warning|danger|info|error|border-color|color|hue|glow|highlight|muted)/', $name)) {
            return 'colors';
        }
        if (preg_match('/-(shadow|elevation)/', $name)) {
            return 'shadows';
        }
        if (preg_match('/-(font|family|mono|serif|size|weight|leading|tracking|line-height|letter)/', $name)) {
            return 'typography';
        }
        if (preg_match('/-(pad|margin|gap|space|sz|radius|round)/', $name)) {
            return 'spacing';
        }
        if (preg_match('/-(transition|dur|ease|anim|delay)/', $name)) {
            return 'animation';
        }
        if (preg_match('/-(border|outline|stroke)/', $name)) {
            return 'borders';
        }
        return 'other';
    }

    /**
     * Extract all class names from selectors.
     */
    private static function extractClasses(string $css): array
    {
        $classes = [];
        $seen    = [];
        // Remove contents of {} blocks to avoid matching inside values
        $stripped = preg_replace('/\{[^}]*\}/s', '{}', $css);
        preg_match_all('/\.([a-zA-Z][a-zA-Z0-9_-]*)/', $stripped, $matches);
        foreach ($matches[1] as $cls) {
            if (isset($seen[$cls])) { continue; }
            $seen[$cls] = true;
            // Group by component
            $group = self::classGroup($cls);
            $classes[$group][] = $cls;
        }
        return $classes;
    }

    /**
     * Determine the component group of a class name.
     */
    private static function classGroup(string $cls): string
    {
        // Strip prefix: xx-component or xxx-component
        if (preg_match('/^[a-z]{2,4}-(.+)$/', $cls, $m)) {
            $rest = $m[1];
            // The component is everything up to the first dash after prefix
            $parts = explode('-', $rest);
            return $parts[0];
        }
        // Unprefixed (clean-system)
        $parts = explode('-', $cls);
        return $parts[0];
    }

    /**
     * Extract @-rules (media queries, keyframes, layers, etc).
     */
    private static function extractAtRules(string $css): array
    {
        $rules = [];
        $seen  = [];
        preg_match_all('/@([\w-]+)\s+([^{;]+)/', $css, $matches, PREG_SET_ORDER);
        foreach ($matches as $m) {
            $type = $m[1];
            $val  = trim($m[2]);
            $key  = $type . ':' . $val;
            if (isset($seen[$key])) { continue; }
            $seen[$key] = true;
            $rules[] = [
                'type'  => $type,
                'value' => $val,
            ];
        }
        return $rules;
    }

    /**
     * Compute quick stats.
     */
    private static function stats(string $css, array $vars, array $classes): array
    {
        $varCount = 0;
        foreach ($vars as $group) {
            $varCount += count($group);
        }
        $classCount = 0;
        foreach ($classes as $group) {
            $classCount += count($group);
        }
        return [
            'lines'        => substr_count($css, "\n") + 1,
            'bytes'        => strlen($css),
            'variables'    => $varCount,
            'classes'      => $classCount,
            'sections'     => count(self::extractSections($css)),
        ];
    }

    /**
     * Return a compact text summary suitable for model consumption.
     */
    public static function toText(array $outline): string
    {
        $out = [];
        $out[] = "CSS Outline: {$outline['file']}";
        $out[] = "Prefix: {$outline['prefix']}";
        if ($outline['body_class']) {
            $out[] = "Body class: body.{$outline['body_class']}";
        }
        $out[] = '';

        // Stats
        $s = $outline['stats'];
        $out[] = "Stats: {$s['lines']} lines, {$s['variables']} variables, {$s['classes']} classes";
        $out[] = '';

        // Sections
        if (!empty($outline['sections'])) {
            $out[] = '## Sections';
            foreach ($outline['sections'] as $sec) {
                $out[] = "  - {$sec['name']} (line {$sec['line']})";
            }
            $out[] = '';
        }

        // Variables grouped
        if (!empty($outline['variables'])) {
            $out[] = '## Variables';
            foreach ($outline['variables'] as $cat => $items) {
                $out[] = "  [{$cat}]";
                foreach ($items as $v) {
                    $out[] = "    {$v['name']}: {$v['value']}";
                }
            }
            $out[] = '';
        }

        // Classes grouped
        if (!empty($outline['classes'])) {
            $out[] = '## Classes';
            foreach ($outline['classes'] as $group => $items) {
                $out[] = "  [{$group}]";
                $out[] = '    ' . implode(', ', $items);
            }
            $out[] = '';
        }

        // At-rules
        if (!empty($outline['at_rules'])) {
            $out[] = '## At-rules';
            foreach ($outline['at_rules'] as $r) {
                $out[] = "  @{$r['type']} {$r['value']}";
            }
        }

        return implode("\n", $out);
    }
}

// CLI mode
if (php_sapi_name() === 'cli' && isset($argv[1])) {
    $file = $argv[1];
    if (!file_exists($file)) {
        fwrite(STDERR, "File not found: $file\n");
        exit(1);
    }
    $css = file_get_contents($file);
    $outline = CSSOutline::extract($css, basename($file));

    $format = $argv[2] ?? 'json';
    if ($format === 'text') {
        echo CSSOutline::toText($outline) . "\n";
    } else {
        echo json_encode($outline, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES) . "\n";
    }
}
