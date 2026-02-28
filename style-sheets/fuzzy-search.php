<?php
/**
 * Fuzzy Search Tool
 * Searches CSS files, rules.json, and theme_handler.json for content related to a query.
 * Uses token-based fuzzy matching: the query is split into terms, and each source
 * is scored by how many terms appear (exact or partial match).
 *
 * Usage from CLI:
 *   php fuzzy-search.php "button primary glow"
 *   php fuzzy-search.php "modal overlay dark"
 *
 * Or as a library:
 *   require_once 'fuzzy-search.php';
 *   $results = FuzzySearch::search('button primary glow', '/path/to/style-sheets');
 */

class FuzzySearch
{
    /** Minimum score (0-1) to include in results. */
    private const MIN_SCORE = 0.15;

    /** Maximum results to return per category. */
    private const MAX_PER_CAT = 15;

    /**
     * Search all available sources for content related to the query.
     *
     * Returns: [
     *   'variables'  => [ { name, value, file, score } ... ],
     *   'classes'    => [ { name, file, context, score } ... ],
     *   'components' => [ { name, variants, note, score } ... ],
     *   'themes'     => [ { name, description, score } ... ],
     *   'rules'      => [ { section, content, score } ... ],
     * ]
     */
    public static function search(string $query, string $baseDir = ''): array
    {
        if ($baseDir === '') {
            $baseDir = __DIR__;
        }

        $terms = self::tokenize($query);
        if (empty($terms)) {
            return self::emptyResult();
        }

        $results = self::emptyResult();

        // 1. Search CSS files for variables and classes
        $cssFiles = glob($baseDir . '/*.css');
        foreach ($cssFiles as $cssFile) {
            $css  = file_get_contents($cssFile);
            $name = basename($cssFile);
            self::searchCSS($css, $name, $terms, $results);
        }

        // 2. Search rules.json for component definitions
        $rulesFile = $baseDir . '/rules.json';
        if (file_exists($rulesFile)) {
            $rules = json_decode(file_get_contents($rulesFile), true);
            if ($rules) {
                self::searchRules($rules, $terms, $results);
            }
        }

        // 3. Search theme_handler.json for theme metadata and palette keywords
        $handlerFile = $baseDir . '/theme_handler.json';
        if (file_exists($handlerFile)) {
            $learn = json_decode(file_get_contents($handlerFile), true);
            if ($learn) {
                self::searchLearn($learn, $terms, $results);
            }
        }

        // Sort each category by score descending, limit results
        foreach ($results as $cat => &$items) {
            usort($items, fn($a, $b) => $b['score'] <=> $a['score']);
            $items = array_slice($items, 0, self::MAX_PER_CAT);
        }

        return $results;
    }

    /**
     * Tokenize a query string into lowercase search terms.
     */
    private static function tokenize(string $query): array
    {
        $query = strtolower(trim($query));
        // Split on spaces, hyphens, underscores, commas
        $tokens = preg_split('/[\s,_-]+/', $query, -1, PREG_SPLIT_NO_EMPTY);
        return array_unique($tokens);
    }

    /**
     * Score a haystack string against search terms.
     * Returns 0.0 to 1.0 based on how many terms match.
     */
    private static function score(string $haystack, array $terms): float
    {
        if (empty($terms)) { return 0; }
        $haystack = strtolower($haystack);
        $hits = 0;
        foreach ($terms as $term) {
            if (str_contains($haystack, $term)) {
                $hits++;
            }
        }
        return $hits / count($terms);
    }

    /**
     * Search CSS content for matching variables and classes.
     */
    private static function searchCSS(string $css, string $filename, array $terms, array &$results): void
    {
        // Variables
        preg_match_all('/(--[a-zA-Z0-9_-]+)\s*:\s*([^;]+);/', $css, $matches, PREG_SET_ORDER);
        foreach ($matches as $m) {
            $varName  = $m[1];
            $varValue = trim($m[2]);
            $s = self::score($varName . ' ' . $varValue, $terms);
            if ($s >= self::MIN_SCORE) {
                $results['variables'][] = [
                    'name'  => $varName,
                    'value' => $varValue,
                    'file'  => $filename,
                    'score' => round($s, 3),
                ];
            }
        }

        // Classes: extract with surrounding context
        $lines = explode("\n", $css);
        foreach ($lines as $i => $line) {
            preg_match_all('/\.([a-zA-Z][a-zA-Z0-9_-]*)/', $line, $clsMatches);
            foreach ($clsMatches[1] as $cls) {
                $s = self::score($cls, $terms);
                if ($s >= self::MIN_SCORE) {
                    // Grab a few lines of context
                    $ctxStart = max(0, $i - 1);
                    $ctxEnd   = min(count($lines) - 1, $i + 3);
                    $context  = implode("\n", array_slice($lines, $ctxStart, $ctxEnd - $ctxStart + 1));
                    $results['classes'][] = [
                        'name'    => $cls,
                        'file'    => $filename,
                        'line'    => $i + 1,
                        'context' => $context,
                        'score'   => round($s, 3),
                    ];
                }
            }
        }

        // Deduplicate classes by name+file
        $seen = [];
        $deduped = [];
        foreach ($results['classes'] as $item) {
            $key = $item['name'] . ':' . $item['file'];
            if (!isset($seen[$key]) || $item['score'] > $seen[$key]) {
                $seen[$key] = $item['score'];
                $deduped[$key] = $item;
            }
        }
        $results['classes'] = array_values($deduped);
    }

    /**
     * Search rules.json for matching component definitions.
     */
    private static function searchRules(array $rules, array $terms, array &$results): void
    {
        // Core components
        $components = array_merge(
            $rules['components']['core'] ?? [],
            $rules['components']['extended'] ?? []
        );
        foreach ($components as $comp) {
            $searchable = ($comp['name'] ?? '') . ' '
                        . implode(' ', $comp['variants'] ?? []) . ' '
                        . implode(' ', $comp['children'] ?? []) . ' '
                        . ($comp['note'] ?? '');
            $s = self::score($searchable, $terms);
            if ($s >= self::MIN_SCORE) {
                $results['components'][] = [
                    'name'     => $comp['name'],
                    'tag'      => $comp['tag'] ?? 'div',
                    'variants' => $comp['variants'] ?? [],
                    'children' => $comp['children'] ?? [],
                    'note'     => $comp['note'] ?? '',
                    'score'    => round($s, 3),
                ];
            }
        }

        // Utility categories
        foreach ($rules['utilities']['categories'] ?? [] as $cat => $info) {
            $searchable = $cat . ' ' . implode(' ', $info['names'] ?? []) . ' ' . ($info['note'] ?? '');
            $s = self::score($searchable, $terms);
            if ($s >= self::MIN_SCORE) {
                $results['rules'][] = [
                    'section' => 'utility:' . $cat,
                    'content' => implode(', ', $info['names'] ?? []),
                    'note'    => $info['note'] ?? '',
                    'score'   => round($s, 3),
                ];
            }
        }

        // Naming/prefix info
        $prefixInfo = json_encode($rules['naming']['prefix'] ?? []);
        $s = self::score($prefixInfo, $terms);
        if ($s >= self::MIN_SCORE) {
            $results['rules'][] = [
                'section' => 'naming:prefix',
                'content' => $rules['naming']['prefix']['rule'] ?? '',
                'score'   => round($s, 3),
            ];
        }
    }

    /**
     * Search learn.json for matching theme info and palette keywords.
     */
    private static function searchLearn(array $learn, array $terms, array &$results): void
    {
        // Theme entries
        foreach ($learn['themes'] ?? [] as $key => $theme) {
            if (!is_array($theme)) { continue; }
            $searchable = $key . ' '
                        . ($theme['full_name'] ?? '') . ' '
                        . ($theme['description'] ?? '') . ' '
                        . ($theme['era'] ?? '') . ' '
                        . ($theme['vibe'] ?? '') . ' '
                        . implode(' ', $theme['best_for'] ?? []);
            $s = self::score($searchable, $terms);
            if ($s >= self::MIN_SCORE) {
                $results['themes'][] = [
                    'name'        => $key,
                    'full_name'   => $theme['full_name'] ?? $key,
                    'description' => $theme['description'] ?? '',
                    'prefix'      => $theme['prefix'] ?? '',
                    'vibe'        => $theme['vibe'] ?? '',
                    'score'       => round($s, 3),
                ];
            }
        }

        // Palette keywords
        foreach ($learn['palette_keywords'] ?? [] as $keyword => $themesArr) {
            if (!is_array($themesArr)) { continue; }
            $searchable = $keyword . ' ' . implode(' ', $themesArr);
            $s = self::score($searchable, $terms);
            if ($s >= self::MIN_SCORE) {
                $results['themes'][] = [
                    'name'        => 'palette:' . $keyword,
                    'full_name'   => $keyword,
                    'description' => 'Themes: ' . implode(', ', $themesArr),
                    'score'       => round($s, 3),
                ];
            }
        }

        // Component coverage
        foreach ($learn['component_coverage']['matrix'] ?? [] as $comp => $coverage) {
            $s = self::score($comp, $terms);
            if ($s >= self::MIN_SCORE) {
                $has = [];
                foreach ($coverage as $thm => $val) {
                    if ($val) { $has[] = $thm; }
                }
                $results['components'][] = [
                    'name'       => $comp,
                    'has_themes' => $has,
                    'score'      => round($s, 3),
                ];
            }
        }
    }

    private static function emptyResult(): array
    {
        return [
            'variables'  => [],
            'classes'    => [],
            'components' => [],
            'themes'     => [],
            'rules'      => [],
        ];
    }

    /**
     * Format results as a compact text block for model consumption.
     */
    public static function toText(array $results): string
    {
        $out = [];

        if (!empty($results['themes'])) {
            $out[] = '## Matching Themes';
            foreach ($results['themes'] as $t) {
                $out[] = "  [{$t['full_name']}] {$t['description']} (score: {$t['score']})";
            }
            $out[] = '';
        }

        if (!empty($results['components'])) {
            $out[] = '## Matching Components';
            foreach ($results['components'] as $c) {
                $extra = '';
                if (!empty($c['variants'])) {
                    $extra .= ' variants: ' . implode(', ', $c['variants']);
                }
                if (!empty($c['has_themes'])) {
                    $extra .= ' themes: ' . implode(', ', $c['has_themes']);
                }
                $out[] = "  {$c['name']}{$extra} (score: {$c['score']})";
            }
            $out[] = '';
        }

        if (!empty($results['variables'])) {
            $out[] = '## Matching Variables';
            foreach ($results['variables'] as $v) {
                $out[] = "  {$v['name']}: {$v['value']} [{$v['file']}] (score: {$v['score']})";
            }
            $out[] = '';
        }

        if (!empty($results['classes'])) {
            $out[] = '## Matching Classes';
            foreach ($results['classes'] as $c) {
                $out[] = "  .{$c['name']} [{$c['file']}:{$c['line']}] (score: {$c['score']})";
            }
            $out[] = '';
        }

        if (!empty($results['rules'])) {
            $out[] = '## Matching Rules';
            foreach ($results['rules'] as $r) {
                $out[] = "  [{$r['section']}] {$r['content']} (score: {$r['score']})";
            }
        }

        if (empty($out)) {
            return 'No matches found.';
        }

        return implode("\n", $out);
    }
}

// CLI mode
if (php_sapi_name() === 'cli' && isset($argv[1])) {
    $query   = $argv[1];
    $baseDir = $argv[2] ?? __DIR__;
    $format  = $argv[3] ?? 'text';

    $results = FuzzySearch::search($query, $baseDir);

    if ($format === 'json') {
        echo json_encode($results, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES) . "\n";
    } else {
        echo FuzzySearch::toText($results) . "\n";
    }
}
