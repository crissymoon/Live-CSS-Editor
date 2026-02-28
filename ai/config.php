<?php
/**
 * AI Provider Configuration Loader
 * Reads config.json for provider settings, then loads API keys from
 * the external keys path on disk. No keys are stored in this repository.
 */

class AIConfig
{
    private static ?array $config = null;
    private static ?array $keys   = null;

    /**
     * Load and cache the full config array.
     */
    public static function load(): array
    {
        if (self::$config !== null) {
            return self::$config;
        }

        $jsonPath = __DIR__ . '/config.json';

        if (!file_exists($jsonPath)) {
            self::jsonError('config.json not found at ' . $jsonPath);
        }

        $raw = file_get_contents($jsonPath);
        $data = json_decode($raw, true);

        if (json_last_error() !== JSON_ERROR_NONE) {
            self::jsonError('config.json is malformed: ' . json_last_error_msg());
        }

        self::$config = $data;
        return self::$config;
    }

    /**
     * Return the full provider array for a given provider slug.
     * Injects the API key from the external keys path.
     */
    public static function provider(string $slug): array
    {
        $cfg = self::load();

        if (!isset($cfg['providers'][$slug])) {
            self::jsonError("Unknown provider: $slug");
        }

        $provider = $cfg['providers'][$slug];
        $provider['api_key'] = self::key($slug);

        return $provider;
    }

    /**
     * Read an API key from the external keys file.
     * Strips whitespace/newlines that text editors often append.
     */
    public static function key(string $slug): string
    {
        if (isset(self::$keys[$slug])) {
            return self::$keys[$slug];
        }

        $cfg      = self::load();
        $keysPath = $cfg['keys_path'] ?? '';
        $keyFile  = $cfg['key_files'][$slug] ?? null;

        if (!$keyFile) {
            self::jsonError("No key file configured for provider: $slug");
        }

        $fullPath = rtrim($keysPath, '/') . '/' . $keyFile;

        if (!file_exists($fullPath)) {
            self::jsonError("Key file not found: $fullPath");
        }

        $value = trim(file_get_contents($fullPath));

        if ($value === '') {
            self::jsonError("Key file is empty: $fullPath");
        }

        self::$keys[$slug] = $value;
        return $value;
    }

    /**
     * Return an array of all available provider slugs.
     */
    public static function providerSlugs(): array
    {
        return array_keys(self::load()['providers'] ?? []);
    }

    /**
     * Emit a JSON error response and exit. Used when config is broken.
     */
    private static function jsonError(string $message): never
    {
        header('Content-Type: application/json');
        http_response_code(500);
        echo json_encode(['error' => $message]);
        exit;
    }
}
