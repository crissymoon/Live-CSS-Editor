<?php
/**
 * CSS Theme Parser / Router
 *
 * Takes a user request (description of what they want to build) and:
 *   1. Loads rules.json and learn.json
 *   2. Matches the request to the best theme(s)
 *   3. Resolves naming (prefix, body class, available components)
 *   4. Returns structured context for an AI model to generate
 *      correct HTML/CSS with the right class names
 *
 * Usage:
 *   From PHP:
 *     $router = new CSSRouter(__DIR__);
 *     $context = $router->route("dark dashboard with cards and stats");
 *
 *   From CLI:
 *     php parser.php "neon cyberpunk login form"
 *
 *   Returns JSON with everything the AI needs to write correct markup.
 */

class CSSRouter {
    private $rules;
    private $learn;
    private $baseDir;

    public function __construct(string $baseDir) {
        $this->baseDir = rtrim($baseDir, '/');
        $this->rules = json_decode(file_get_contents($this->baseDir . '/rules.json'), true);
        $this->learn = json_decode(file_get_contents($this->baseDir . '/learn.json'), true);
    }

    /**
     * Main entry: route a user request to the best theme and return
     * structured context for the AI model.
     */
    public function route(string $request, ?string $forceTheme = null): array {
        $request = strtolower(trim($request));

        // If a theme is forced, use it directly
        if ($forceTheme !== null) {
            $themeKey = $this->resolveThemeKey($forceTheme);
            if ($themeKey === null) {
                return ['error' => 'Unknown theme: ' . $forceTheme];
            }
            $theme = $this->learn['themes'][$themeKey];
            return $this->buildContext($themeKey, $theme, $request, 1.0);
        }

        // Score all themes against the request
        $scores = $this->scoreThemes($request);

        // Sort by score descending
        arsort($scores);

        $bestKey = array_key_first($scores);
        $bestScore = $scores[$bestKey];
        $theme = $this->learn['themes'][$bestKey];

        $result = $this->buildContext($bestKey, $theme, $request, $bestScore);

        // Include runner-up if close
        $keys = array_keys($scores);
        if (count($keys) > 1) {
            $runnerKey = $keys[1];
            $runnerScore = $scores[$runnerKey];
            if ($runnerScore > 0 && $runnerScore >= $bestScore * 0.6) {
                $result['alternative'] = [
                    'theme' => $runnerKey,
                    'score' => round($runnerScore, 3),
                    'prefix' => $this->learn['themes'][$runnerKey]['prefix'],
                    'file' => $this->learn['themes'][$runnerKey]['file']
                ];
            }
        }

        return $result;
    }

    /**
     * Score every theme against the user request using keyword matching,
     * component matching, and palette keyword matching.
     */
    private function scoreThemes(string $request): array {
        $scores = [];
        $words = preg_split('/[\s,]+/', $request);

        foreach ($this->learn['themes'] as $key => $theme) {
            $score = 0.0;

            // 1. Direct theme name match (strongest signal)
            $nameVariants = [$key, str_replace('-', ' ', $key), $theme['prefix'] ?? ''];
            foreach ($nameVariants as $name) {
                if ($name && strpos($request, $name) !== false) {
                    $score += 5.0;
                }
            }

            // 2. Palette keyword matching from learn.json
            if (isset($this->learn['palette_keywords'])) {
                foreach ($this->learn['palette_keywords'] as $keyword => $themes) {
                    if (strpos($request, $keyword) !== false) {
                        if (in_array($key, $themes)) {
                            $score += 2.0;
                        }
                    }
                }
            }

            // 3. Description/title word overlap
            $desc = strtolower(($theme['title'] ?? '') . ' ' . ($theme['description'] ?? ''));
            foreach ($words as $w) {
                if (strlen($w) >= 3 && strpos($desc, $w) !== false) {
                    $score += 0.5;
                }
            }

            // 4. best_for matching
            if (isset($theme['best_for'])) {
                foreach ($theme['best_for'] as $useCase) {
                    $ucLower = strtolower($useCase);
                    foreach ($words as $w) {
                        if (strlen($w) >= 3 && strpos($ucLower, $w) !== false) {
                            $score += 1.5;
                        }
                    }
                }
            }

            // 5. Visual identity keyword matching
            if (isset($theme['visual_identity'])) {
                $viText = strtolower(implode(' ', array_values($theme['visual_identity'])));
                foreach ($words as $w) {
                    if (strlen($w) >= 3 && strpos($viText, $w) !== false) {
                        $score += 0.8;
                    }
                }
            }

            // 6. Component requests: check if the theme has what they need
            $componentWords = ['card', 'button', 'btn', 'form', 'input', 'table', 'modal',
                               'nav', 'alert', 'badge', 'progress', 'tab', 'avatar',
                               'toggle', 'tooltip', 'skeleton', 'terminal', 'keyboard',
                               'key', 'glass', 'panel', 'breadcrumb', 'stat', 'chart'];
            foreach ($componentWords as $cw) {
                if (strpos($request, $cw) !== false) {
                    $components = $theme['components_available'] ?? [];
                    foreach ($components as $comp) {
                        if (strpos($comp, $cw) !== false) {
                            $score += 0.3;
                            break;
                        }
                    }
                }
            }

            // 7. Dark/light preference
            if (strpos($request, 'dark') !== false && ($theme['is_dark_by_default'] ?? false)) {
                $score += 1.0;
            }
            if (strpos($request, 'light') !== false && !($theme['is_dark_by_default'] ?? true)) {
                $score += 1.0;
            }

            $scores[$key] = $score;
        }

        return $scores;
    }

    /**
     * Build the full context object that gets passed to the AI model.
     */
    private function buildContext(string $themeKey, array $theme, string $request, float $score): array {
        $prefix = $theme['prefix'];
        $components = $this->extractRequestedComponents($request, $theme);

        $context = [
            'theme' => $themeKey,
            'score' => round($score, 3),
            'file' => $theme['file'],
            'prefix' => $prefix,
            'body_class' => $theme['body_class'],
            'title' => $theme['title'],
            'is_dark' => $theme['is_dark_by_default'] ?? false,

            // Naming rules for the AI
            'naming' => [
                'class_format' => $prefix ? "{$prefix}-{component}" : '{component}',
                'variant_format' => $prefix ? "{$prefix}-{component}-{variant}" : '{component}-{variant}',
                'variable_format' => $prefix ? "--{$prefix}-{token}" : '--clr-{token}',
                'examples' => $this->generateNamingExamples($prefix, $theme)
            ],

            // Components the AI should use
            'available_components' => $theme['components_available'] ?? [],
            'requested_components' => $components,

            // Resolved class names for requested components
            'resolved_classes' => $this->resolveClasses($prefix, $components, $theme),

            // Palette for inline styles if needed
            'palette' => $theme['palette'] ?? [],

            // Visual identity for AI style decisions
            'visual_identity' => $theme['visual_identity'] ?? [],

            // The full CSS file content reference
            'css_file_path' => $this->baseDir . '/' . $theme['file']
        ];

        // Add instruction block for the AI
        $context['ai_instruction'] = $this->buildInstruction($context);

        return $context;
    }

    /**
     * Extract which components the user is asking for based on request text.
     */
    private function extractRequestedComponents(string $request, array $theme): array {
        $componentMap = [
            'button' => 'btn', 'btn' => 'btn',
            'card' => 'card', 'cards' => 'card',
            'input' => 'input', 'form' => 'input',
            'textarea' => 'textarea',
            'select' => 'select', 'dropdown' => 'select',
            'nav' => 'nav', 'navigation' => 'nav', 'navbar' => 'nav', 'menu' => 'nav',
            'table' => 'table', 'grid' => 'table',
            'modal' => 'modal', 'dialog' => 'modal', 'popup' => 'modal',
            'alert' => 'alert', 'notification' => 'alert', 'message' => 'alert',
            'badge' => 'badge',
            'tag' => 'tag', 'chip' => 'chip',
            'progress' => 'progress', 'loading' => 'progress',
            'tab' => 'tabs', 'tabs' => 'tabs',
            'toggle' => 'toggle', 'switch' => 'toggle',
            'avatar' => 'avatar', 'profile' => 'avatar',
            'tooltip' => 'tooltip',
            'skeleton' => 'skeleton', 'placeholder' => 'skeleton',
            'breadcrumb' => 'breadcrumb',
            'panel' => 'panel',
            'stat' => 'stat', 'stats' => 'stat', 'metric' => 'stat',
            'divider' => 'divider', 'separator' => 'divider',
            'terminal' => 'terminal', 'console' => 'terminal',
            'keyboard' => 'key', 'key' => 'key', 'shortcut' => 'shortcut',
            'glass' => 'glass',
            'login' => 'input', 'signin' => 'input', 'signup' => 'input',
            'dashboard' => 'card',
            'toolbar' => 'toolbar',
            'toast' => 'sys-toast'
        ];

        $found = [];
        $words = preg_split('/[\s,]+/', $request);

        foreach ($words as $w) {
            if (isset($componentMap[$w])) {
                $found[$componentMap[$w]] = true;
            }
        }

        // If nothing specific found, return common defaults
        if (empty($found)) {
            $found = ['btn' => true, 'card' => true, 'input' => true];
        }

        return array_keys($found);
    }

    /**
     * Resolve actual class names for requested components given a prefix.
     */
    private function resolveClasses(?string $prefix, array $components, array $theme): array {
        $available = array_flip($theme['components_available'] ?? []);
        $resolved = [];

        foreach ($components as $comp) {
            $entry = ['component' => $comp];

            if ($prefix) {
                $base = "{$prefix}-{$comp}";
            } else {
                $base = $comp;
            }

            $entry['base_class'] = $base;

            // Find all variants available
            $variants = [];
            foreach ($available as $cls => $_) {
                if (strpos($cls, $comp) === 0 || ($prefix && strpos($cls, "{$comp}-") === 0)) {
                    $fullCls = $prefix ? "{$prefix}-{$cls}" : $cls;
                    if ($cls !== $comp) {
                        $variants[] = $fullCls;
                    }
                }
            }
            $entry['variants'] = $variants;

            // Look up children from rules.json
            $children = $this->findChildren($comp);
            if (!empty($children)) {
                $entry['children'] = array_map(function($child) use ($prefix) {
                    return $prefix ? "{$prefix}-{$child}" : $child;
                }, $children);
            }

            $resolved[] = $entry;
        }

        return $resolved;
    }

    /**
     * Find child components from rules.json component definitions.
     */
    private function findChildren(string $component): array {
        $allComponents = array_merge(
            $this->rules['components']['core'] ?? [],
            $this->rules['components']['extended'] ?? []
        );

        foreach ($allComponents as $def) {
            if ($def['name'] === $component && isset($def['children'])) {
                $children = [];
                foreach ($def['children'] as $child) {
                    // Handle pipe-separated alternatives: "alert-title|alert-body"
                    $parts = explode('|', $child);
                    $children[] = $parts[0];
                }
                return $children;
            }
        }

        return [];
    }

    /**
     * Generate concrete naming examples for the AI.
     */
    private function generateNamingExamples(?string $prefix, array $theme): array {
        if (!$prefix) {
            return [
                'button' => 'class="btn"',
                'primary_button' => 'class="btn btn-primary"',
                'card' => 'class="card"',
                'card_header' => 'class="card-header"',
                'input' => 'class="input"',
                'variable' => 'var(--clr-accent)'
            ];
        }

        return [
            'button' => "class=\"{$prefix}-btn\"",
            'primary_button' => "class=\"{$prefix}-btn {$prefix}-btn-primary\"",
            'card' => "class=\"{$prefix}-card\"",
            'card_header' => "class=\"{$prefix}-card-header\"",
            'input' => "class=\"{$prefix}-input\"",
            'variable' => "var(--{$prefix}-primary)"
        ];
    }

    /**
     * Build a plain-text instruction block the AI model can follow directly.
     */
    private function buildInstruction(array $context): string {
        $p = $context['prefix'] ?? '';
        $body = $context['body_class'] ?? '';
        $lines = [];

        $lines[] = "THEME: {$context['title']}";
        $lines[] = "CSS FILE: {$context['file']}";
        $lines[] = "";

        if ($body) {
            $lines[] = "BODY TAG: <body class=\"{$body}\">";
        } else {
            $lines[] = "BODY TAG: <body>";
        }
        $lines[] = "LINK TAG: <link rel=\"stylesheet\" href=\"{$context['file']}\">";
        $lines[] = "";

        $lines[] = "NAMING RULES:";
        if ($p) {
            $lines[] = "- All classes start with \"{$p}-\"";
            $lines[] = "- Component: {$p}-{component}";
            $lines[] = "- Variant: {$p}-{component}-{variant}";
            $lines[] = "- Variants combine with base: class=\"{$p}-btn {$p}-btn-primary\"";
            $lines[] = "- CSS variables: --{$p}-{token}";
        } else {
            $lines[] = "- Classes are unprefixed: btn, card, input";
            $lines[] = "- Variant: {component}-{variant}";
            $lines[] = "- CSS variables use category prefixes: --clr-*, --sz-*, --type-*";
        }
        $lines[] = "";

        if (!empty($context['resolved_classes'])) {
            $lines[] = "RESOLVED CLASSES FOR YOUR REQUEST:";
            foreach ($context['resolved_classes'] as $rc) {
                $lines[] = "  {$rc['component']}: {$rc['base_class']}";
                if (!empty($rc['variants'])) {
                    $lines[] = "    variants: " . implode(', ', array_slice($rc['variants'], 0, 8));
                }
                if (!empty($rc['children'])) {
                    $lines[] = "    children: " . implode(', ', $rc['children']);
                }
            }
            $lines[] = "";
        }

        $lines[] = "IMPORTANT:";
        $lines[] = "- Never mix prefixes. Every class in the output must use \"{$p}-\" (or no prefix for clean-system).";
        $lines[] = "- Do not invent class names. Only use classes listed in available_components.";
        $lines[] = "- Do not use border-radius in inline styles.";
        $lines[] = "- Do not hardcode color hex values. Use var(--{$p}-{token}) or the theme's CSS classes.";

        return implode("\n", $lines);
    }

    /**
     * Resolve a user-provided theme name to the internal key.
     */
    private function resolveThemeKey(string $input): ?string {
        $input = strtolower(trim($input));

        // Direct key match
        if (isset($this->learn['themes'][$input])) {
            return $input;
        }

        // Match by prefix
        foreach ($this->learn['themes'] as $key => $theme) {
            if (($theme['prefix'] ?? '') === $input) {
                return $key;
            }
        }

        // Match by filename
        foreach ($this->learn['themes'] as $key => $theme) {
            if (strtolower($theme['file']) === $input || strtolower($theme['file']) === $input . '.css') {
                return $key;
            }
        }

        // Fuzzy: partial match
        foreach ($this->learn['themes'] as $key => $theme) {
            if (strpos($key, $input) !== false || strpos($input, $key) !== false) {
                return $key;
            }
        }

        return null;
    }

    /**
     * List all available themes with summary info.
     */
    public function listThemes(): array {
        $list = [];
        foreach ($this->learn['themes'] as $key => $theme) {
            $list[] = [
                'key' => $key,
                'prefix' => $theme['prefix'],
                'file' => $theme['file'],
                'title' => $theme['title'],
                'is_dark' => $theme['is_dark_by_default'] ?? false,
                'component_count' => count($theme['components_available'] ?? []),
                'best_for' => $theme['best_for'] ?? []
            ];
        }
        return $list;
    }

    /**
     * Get the raw rules for AI consumption.
     */
    public function getRules(): array {
        return $this->rules;
    }

    /**
     * Get the raw learn data.
     */
    public function getLearn(): array {
        return $this->learn;
    }

    /**
     * Add a note to learn.json (persistent learning).
     */
    public function addNote(string $note): void {
        $this->learn['notes'][] = [
            'text' => $note,
            'added' => date('Y-m-d H:i:s')
        ];
        file_put_contents(
            $this->baseDir . '/learn.json',
            json_encode($this->learn, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES)
        );
    }
}

// -------------------------------------------------------------------------
// CLI interface
// -------------------------------------------------------------------------
if (php_sapi_name() === 'cli' && isset($argv[0]) && realpath($argv[0]) === __FILE__) {
    $dir = __DIR__;
    $router = new CSSRouter($dir);

    if ($argc < 2) {
        echo "Usage:\n";
        echo "  php parser.php \"describe what you want to build\"\n";
        echo "  php parser.php --theme=neon-grid \"dashboard with stats\"\n";
        echo "  php parser.php --list\n";
        exit(0);
    }

    $forceTheme = null;
    $query = [];

    foreach (array_slice($argv, 1) as $arg) {
        if ($arg === '--list') {
            echo json_encode($router->listThemes(), JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES) . "\n";
            exit(0);
        }
        if (strpos($arg, '--theme=') === 0) {
            $forceTheme = substr($arg, 8);
            continue;
        }
        $query[] = $arg;
    }

    $request = implode(' ', $query);
    if (empty($request)) {
        $request = 'general preview';
    }

    $result = $router->route($request, $forceTheme);
    echo json_encode($result, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES) . "\n";
}
