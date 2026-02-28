<?php
/**
 * Agent Context Module
 * Maintains a per-file JSON context that tracks what the user is requesting,
 * which files are connected, and what changes were made.
 * Each file gets its own context JSON under context/sessions/.
 * The module learns over the session by appending to a request log.
 */

class AgentContext
{
    private static string $sessionsDir = '';

    private static function sessionsDir(): string
    {
        if (self::$sessionsDir === '') {
            self::$sessionsDir = __DIR__ . '/sessions';
            if (!is_dir(self::$sessionsDir)) {
                mkdir(self::$sessionsDir, 0755, true);
            }
        }
        return self::$sessionsDir;
    }

    /** Derive a safe filename from a file path. */
    private static function contextFile(string $filePath): string
    {
        $safe = preg_replace('/[^a-zA-Z0-9._-]/', '_', $filePath);
        $safe = trim($safe, '_');
        return self::sessionsDir() . '/' . $safe . '.context.json';
    }

    // -------------------------------------------------------------------------
    // Load / save
    // -------------------------------------------------------------------------

    public static function load(string $filePath): array
    {
        $file = self::contextFile($filePath);
        if (!file_exists($file)) {
            return self::defaultContext($filePath);
        }
        $raw  = file_get_contents($file);
        $data = json_decode($raw, true);
        return is_array($data) ? $data : self::defaultContext($filePath);
    }

    public static function save(string $filePath, array $ctx): void
    {
        $file = self::contextFile($filePath);
        file_put_contents($file, json_encode($ctx, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));
    }

    private static function defaultContext(string $filePath): array
    {
        return [
            'file_path'    => $filePath,
            'language'     => strtolower(pathinfo($filePath, PATHINFO_EXTENSION)),
            'created_at'   => date('c'),
            'updated_at'   => date('c'),
            'requests'     => [],
            'connections'  => [],
            'change_log'   => [],
            'learned'      => [
                'patterns'  => [],
                'avoidList' => [],
                'style'     => [],
            ],
            'meta'         => [],
        ];
    }

    // -------------------------------------------------------------------------
    // Append a user request
    // -------------------------------------------------------------------------

    /**
     * Log a user request for a file.
     * task: short task identifier (fix, refactor, explain, modernize, etc.)
     * prompt: the full user text
     * model: provider:model used
     */
    public static function addRequest(
        string $filePath,
        string $task,
        string $prompt,
        string $model = ''
    ): array {
        $ctx = self::load($filePath);
        $ctx['requests'][] = [
            'id'         => uniqid('req_'),
            'task'       => $task,
            'prompt'     => $prompt,
            'model'      => $model,
            'timestamp'  => date('c'),
        ];
        // Keep only last 50 requests
        if (count($ctx['requests']) > 50) {
            $ctx['requests'] = array_slice($ctx['requests'], -50);
        }
        $ctx['updated_at'] = date('c');
        self::save($filePath, $ctx);
        return $ctx;
    }

    // -------------------------------------------------------------------------
    // Log a change
    // -------------------------------------------------------------------------

    /**
     * Record that a change was applied to the file.
     * versionId: DB row id from AgentDB::saveVersion
     */
    public static function addChange(
        string $filePath,
        string $summary,
        int    $versionId,
        string $model = ''
    ): array {
        $ctx = self::load($filePath);
        $ctx['change_log'][] = [
            'id'         => uniqid('chg_'),
            'summary'    => $summary,
            'version_id' => $versionId,
            'model'      => $model,
            'timestamp'  => date('c'),
        ];
        if (count($ctx['change_log']) > 50) {
            $ctx['change_log'] = array_slice($ctx['change_log'], -50);
        }
        $ctx['updated_at'] = date('c');
        self::save($filePath, $ctx);
        return $ctx;
    }

    // -------------------------------------------------------------------------
    // Connection tracking (which files are related)
    // -------------------------------------------------------------------------

    /**
     * Register that $filePath references or depends on $relatedPath.
     * type: 'import', 'require', 'include', 'uses', 'related'
     */
    public static function addConnection(
        string $filePath,
        string $relatedPath,
        string $type = 'related'
    ): array {
        $ctx = self::load($filePath);
        // Avoid duplicate
        foreach ($ctx['connections'] as $conn) {
            if ($conn['path'] === $relatedPath && $conn['type'] === $type) {
                return $ctx;
            }
        }
        $ctx['connections'][] = [
            'path'      => $relatedPath,
            'type'      => $type,
            'added_at'  => date('c'),
        ];
        $ctx['updated_at'] = date('c');
        self::save($filePath, $ctx);
        return $ctx;
    }

    /**
     * Auto-detect connections from file content by scanning for common
     * require/import/include patterns and CSS @import.
     */
    public static function detectConnections(string $filePath, string $content): void
    {
        $ext   = strtolower(pathinfo($filePath, PATHINFO_EXTENSION));
        $found = [];

        if (in_array($ext, ['php'])) {
            preg_match_all('/(?:require|include)(?:_once)?\s*[\(\'"]([^\'")\s]+)/', $content, $m);
            foreach ($m[1] as $path) { $found[] = [$path, 'require']; }
        }

        if (in_array($ext, ['js', 'ts', 'jsx', 'tsx'])) {
            preg_match_all('/import\s+.*?from\s+[\'"]([^\'"]+)[\'"]/', $content, $m);
            foreach ($m[1] as $path) { $found[] = [$path, 'import']; }
            preg_match_all('/require\([\'"]([^\'"]+)[\'"]\)/', $content, $m);
            foreach ($m[1] as $path) { $found[] = [$path, 'require']; }
        }

        if (in_array($ext, ['css', 'scss', 'less'])) {
            preg_match_all('/@import\s+[\'"]([^\'"]+)[\'"]/', $content, $m);
            foreach ($m[1] as $path) { $found[] = [$path, 'import']; }
        }

        foreach ($found as [$path, $type]) {
            self::addConnection($filePath, $path, $type);
        }
    }

    // -------------------------------------------------------------------------
    // Learning layer
    // -------------------------------------------------------------------------

    /**
     * Record a coding pattern the agent noticed or was told about.
     * This grows the learned context so future requests stay consistent.
     */
    public static function learnPattern(string $filePath, string $pattern, string $category = 'style'): void
    {
        $ctx = self::load($filePath);
        if (!in_array($pattern, $ctx['learned'][$category] ?? [])) {
            $ctx['learned'][$category][] = $pattern;
        }
        $ctx['updated_at'] = date('c');
        self::save($filePath, $ctx);
    }

    /**
     * Add something to the avoid list (things the agent should not do for this file).
     */
    public static function avoid(string $filePath, string $item): void
    {
        $ctx = self::load($filePath);
        if (!in_array($item, $ctx['learned']['avoidList'])) {
            $ctx['learned']['avoidList'][] = $item;
        }
        $ctx['updated_at'] = date('c');
        self::save($filePath, $ctx);
    }

    /**
     * Build a compact context summary string to inject into AI prompts.
     * This is the learning distillation passed to the model as context.
     */
    public static function buildPromptContext(string $filePath): string
    {
        $ctx  = self::load($filePath);
        $lang = $ctx['language'] ?? 'unknown';
        $out  = ["File: $filePath (language: $lang)"];

        if (!empty($ctx['change_log'])) {
            $last = array_slice($ctx['change_log'], -3);
            $out[] = "Recent changes: " . implode('; ', array_column($last, 'summary'));
        }

        if (!empty($ctx['connections'])) {
            $conns = array_map(fn($c) => $c['type'] . ':' . $c['path'], $ctx['connections']);
            $out[] = "Connected files: " . implode(', ', array_slice($conns, 0, 8));
        }

        if (!empty($ctx['learned']['style'])) {
            $out[] = "Coding style notes: " . implode('; ', array_slice($ctx['learned']['style'], 0, 5));
        }

        if (!empty($ctx['learned']['avoidList'])) {
            $out[] = "Do NOT: " . implode(', ', $ctx['learned']['avoidList']);
        }

        if (!empty($ctx['requests'])) {
            $last = end($ctx['requests']);
            $out[] = "Last task: " . ($last['task'] ?? '') . ' -- ' . substr($last['prompt'] ?? '', 0, 120);
        }

        return implode("\n", $out);
    }

    /** Return all context files as an array of loaded contexts. */
    public static function allContexts(): array
    {
        $dir   = self::sessionsDir();
        $files = glob($dir . '/*.context.json');
        $out   = [];
        foreach ($files as $f) {
            $raw  = file_get_contents($f);
            $data = json_decode($raw, true);
            if ($data) { $out[] = $data; }
        }
        return $out;
    }
}
