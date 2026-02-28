<?php
/**
 * Agent Outline Extractor
 * Parses PHP, JS, CSS, HTML, and plain text files and returns a structured
 * outline tree for display in the outline panel.
 *
 * Each node: { type, name, line, depth, children[] }
 */

class AgentOutline
{
    /**
     * Auto-detect language from extension and extract outline.
     */
    public static function extract(string $content, string $filePath = ''): array
    {
        $ext = strtolower(pathinfo($filePath, PATHINFO_EXTENSION));
        return match ($ext) {
            'php'        => self::php($content),
            'js', 'ts',
            'jsx', 'tsx' => self::javascript($content),
            'css', 'scss',
            'less'       => self::css($content),
            'html', 'htm' => self::html($content),
            'json'       => self::json($content),
            default      => self::plainText($content),
        };
    }

    // -------------------------------------------------------------------------
    // PHP
    // -------------------------------------------------------------------------

    public static function php(string $content): array
    {
        $nodes = [];
        $lines = explode("\n", $content);

        $currentClass  = null;
        $currentClassDepth = 0;
        $braceDepth    = 0;

        foreach ($lines as $lineNo => $raw) {
            $line = trim($raw);
            $n    = $lineNo + 1;

            // Count braces for depth tracking
            $open  = substr_count($raw, '{');
            $close = substr_count($raw, '}');

            // Namespace
            if (preg_match('/^namespace\s+([\w\\\\]+)/', $line, $m)) {
                $nodes[] = ['type' => 'namespace', 'name' => $m[1], 'line' => $n, 'depth' => 0, 'children' => []];
            }

            // Class / interface / trait / enum
            if (preg_match('/(?:class|interface|trait|enum)\s+(\w+)/', $line, $m)) {
                $node = ['type' => 'class', 'name' => $m[1], 'line' => $n, 'depth' => $braceDepth, 'children' => []];
                $nodes[]           = $node;
                $currentClass      = count($nodes) - 1;
                $currentClassDepth = $braceDepth;
            }

            // Function / method
            if (preg_match('/(?:public|protected|private|static|final|abstract|\s)*function\s+(\w+)\s*\(/', $line, $m)) {
                $methName = $m[1];
                if ($methName !== '__construct' || $currentClass !== null) {
                    $visibility = 'method';
                    if (preg_match('/\b(public|protected|private)\b/', $line, $vm)) {
                        $visibility = $vm[1];
                    }
                    $node = [
                        'type'     => $currentClass !== null && $braceDepth > $currentClassDepth ? 'method' : 'function',
                        'name'     => $methName,
                        'line'     => $n,
                        'depth'    => $braceDepth,
                        'children' => [],
                        'meta'     => $visibility,
                    ];
                    if ($currentClass !== null && $braceDepth > $currentClassDepth) {
                        $nodes[$currentClass]['children'][] = $node;
                    } else {
                        $nodes[] = $node;
                    }
                }
            }

            // Constants
            if (preg_match('/^\s*(?:const|define\()\s+[\'"]?(\w+)/', $line, $m)) {
                $nodes[] = ['type' => 'constant', 'name' => $m[1], 'line' => $n, 'depth' => $braceDepth, 'children' => []];
            }

            $braceDepth += $open - $close;
            if ($braceDepth < 0) { $braceDepth = 0; }
        }

        return $nodes;
    }

    // -------------------------------------------------------------------------
    // JavaScript / TypeScript
    // -------------------------------------------------------------------------

    public static function javascript(string $content): array
    {
        $nodes = [];
        $lines = explode("\n", $content);

        foreach ($lines as $lineNo => $raw) {
            $line = trim($raw);
            $n    = $lineNo + 1;

            // Class declarations
            if (preg_match('/^(?:export\s+)?(?:default\s+)?class\s+(\w+)/', $line, $m)) {
                $nodes[] = ['type' => 'class', 'name' => $m[1], 'line' => $n, 'depth' => 0, 'children' => []];
                continue;
            }

            // Function declarations
            if (preg_match('/^(?:export\s+)?(?:async\s+)?function\s*\*?\s*(\w+)\s*\(/', $line, $m)) {
                $nodes[] = ['type' => 'function', 'name' => $m[1], 'line' => $n, 'depth' => 0, 'children' => []];
                continue;
            }

            // const/let/var = function or arrow
            if (preg_match('/^(?:export\s+)?(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s+)?(?:function|\()/', $line, $m)) {
                $nodes[] = ['type' => 'function', 'name' => $m[1], 'line' => $n, 'depth' => 0, 'children' => []];
                continue;
            }

            // const/let/var plain
            if (preg_match('/^(?:export\s+)?(?:const|let|var)\s+(\w+)\s*=\s*[^(]/', $line, $m)) {
                $nodes[] = ['type' => 'variable', 'name' => $m[1], 'line' => $n, 'depth' => 0, 'children' => []];
                continue;
            }

            // Method inside object / class body (indented)
            if (preg_match('/^\s{2,}(?:async\s+)?(\w+)\s*\([^)]*\)\s*\{/', $line, $m)) {
                $nodes[] = ['type' => 'method', 'name' => $m[1], 'line' => $n, 'depth' => 1, 'children' => []];
                continue;
            }
        }
        return $nodes;
    }

    // -------------------------------------------------------------------------
    // CSS / SCSS
    // -------------------------------------------------------------------------

    public static function css(string $content): array
    {
        $nodes  = [];
        $lines  = explode("\n", $content);
        $depth  = 0;

        foreach ($lines as $lineNo => $raw) {
            $line = trim($raw);
            $n    = $lineNo + 1;

            if ($line === '' || str_starts_with($line, '//') || str_starts_with($line, '/*')) {
                continue;
            }

            // At-rules: @media, @keyframes, @layer, etc.
            if (preg_match('/^(@[\w-]+[^{]*)\{/', $line, $m)) {
                $nodes[] = ['type' => 'at-rule', 'name' => rtrim($m[1]), 'line' => $n, 'depth' => $depth, 'children' => []];
                $depth++;
                continue;
            }

            // Selector block
            if (str_ends_with($line, '{')) {
                $selector = rtrim(substr($line, 0, -1));
                if ($selector !== '') {
                    $nodes[] = ['type' => 'selector', 'name' => $selector, 'line' => $n, 'depth' => $depth, 'children' => []];
                }
                $depth++;
                continue;
            }

            if ($line === '}') {
                $depth = max(0, $depth - 1);
            }
        }
        return $nodes;
    }

    // -------------------------------------------------------------------------
    // HTML
    // -------------------------------------------------------------------------

    public static function html(string $content): array
    {
        $nodes = [];
        $lines = explode("\n", $content);

        foreach ($lines as $lineNo => $raw) {
            $n = $lineNo + 1;
            // Headings
            if (preg_match('/<h([1-6])[^>]*>(.*?)<\/h\1>/i', $raw, $m)) {
                $text = strip_tags($m[2]);
                $nodes[] = [
                    'type'     => 'heading',
                    'name'     => $text,
                    'line'     => $n,
                    'depth'    => (int) $m[1] - 1,
                    'children' => [],
                ];
            }
            // Elements with id
            if (preg_match('/<(\w+)[^>]+\bid=["\']([^"\']+)["\']/', $raw, $m)) {
                $nodes[] = [
                    'type'     => 'id',
                    'name'     => '#' . $m[2] . ' (' . $m[1] . ')',
                    'line'     => $n,
                    'depth'    => 0,
                    'children' => [],
                ];
            }
        }
        return $nodes;
    }

    // -------------------------------------------------------------------------
    // JSON
    // -------------------------------------------------------------------------

    public static function json(string $content): array
    {
        $data = json_decode($content, true);
        if (!$data || !is_array($data)) {
            return [['type' => 'value', 'name' => '(invalid JSON)', 'line' => 1, 'depth' => 0, 'children' => []]];
        }
        $nodes = [];
        foreach (array_keys($data) as $key) {
            $nodes[] = [
                'type'     => is_array($data[$key]) ? 'object' : 'value',
                'name'     => (string) $key,
                'line'     => 1,
                'depth'    => 0,
                'children' => [],
            ];
        }
        return $nodes;
    }

    // -------------------------------------------------------------------------
    // Plain text (headings by indentation / markdown-style)
    // -------------------------------------------------------------------------

    public static function plainText(string $content): array
    {
        $nodes = [];
        $lines = explode("\n", $content);
        foreach ($lines as $lineNo => $raw) {
            $n = $lineNo + 1;
            if (preg_match('/^(#{1,6})\s+(.+)/', $raw, $m)) {
                $nodes[] = [
                    'type'     => 'heading',
                    'name'     => $m[2],
                    'line'     => $n,
                    'depth'    => strlen($m[1]) - 1,
                    'children' => [],
                ];
            }
        }
        return $nodes;
    }

    /**
     * Render outline nodes to a nested HTML list (for server-side use).
     */
    public static function toHtml(array $nodes, int $maxDepth = 4): string
    {
        if (empty($nodes)) { return '<p class="outline-empty">No outline</p>'; }
        $html = '<ul class="outline-list">';
        foreach ($nodes as $node) {
            $type  = htmlspecialchars($node['type'], ENT_QUOTES, 'UTF-8');
            $name  = htmlspecialchars($node['name'], ENT_QUOTES, 'UTF-8');
            $line  = (int) ($node['line'] ?? 0);
            $depth = min((int) ($node['depth'] ?? 0), $maxDepth);
            $html .= '<li class="outline-node outline-' . $type . '" data-line="' . $line . '" style="--depth:' . $depth . '">'
                   . '<span class="outline-icon outline-icon-' . $type . '"></span>'
                   . '<span class="outline-name">' . $name . '</span>'
                   . '<span class="outline-line">' . $line . '</span>';
            if (!empty($node['children'])) {
                $html .= self::toHtml($node['children']);
            }
            $html .= '</li>';
        }
        $html .= '</ul>';
        return $html;
    }
}
