<?php
/**
 * Provider configuration test.
 *
 * Tests that every provider in config.json is correctly configured:
 *   - Config loads without errors
 *   - API key file exists and is non-empty
 *   - Key format is plausible (prefix pattern check)
 *   - Base URL is reachable (HEAD or GET to a known path, 5 second timeout)
 *   - Required config fields are present
 *   - Model list is non-empty and default_model is listed
 *
 * Run from the repository root:
 *   php ai/test-providers.php
 *
 * Can also be called from a browser -- output is plain text when invoked via
 * HTTP, or pre-formatted when running in a terminal.
 */

declare(strict_types=1);

require_once __DIR__ . '/config.php';

// ---------------------------------------------------------------------------
// ANSI color helpers (terminal only)
// ---------------------------------------------------------------------------

$isCLI = php_sapi_name() === 'cli';

function ok(string $msg): void
{
    global $isCLI;
    if ($isCLI) {
        echo "\033[32m  PASS\033[0m  " . $msg . "\n";
    } else {
        echo "  PASS  " . $msg . "\n";
    }
}

function fail(string $msg): void
{
    global $isCLI;
    if ($isCLI) {
        echo "\033[31m  FAIL\033[0m  " . $msg . "\n";
    } else {
        echo "  FAIL  " . $msg . "\n";
    }
}

function info(string $msg): void
{
    global $isCLI;
    if ($isCLI) {
        echo "\033[36m  INFO\033[0m  " . $msg . "\n";
    } else {
        echo "  INFO  " . $msg . "\n";
    }
}

function section(string $title): void
{
    global $isCLI;
    echo "\n";
    if ($isCLI) {
        echo "\033[1m" . $title . "\033[0m\n";
        echo str_repeat('-', strlen($title)) . "\n";
    } else {
        echo $title . "\n";
        echo str_repeat('-', strlen($title)) . "\n";
    }
}

// ---------------------------------------------------------------------------
// Known key prefix patterns for basic sanity checks
// ---------------------------------------------------------------------------

$KEY_PREFIXES = [
    'anthropic' => ['sk-ant-'],
    'openai'    => ['sk-', 'sk-proj-'],
    'deepseek'  => ['sk-'],
];

// ---------------------------------------------------------------------------
// Connectivity check via cURL HEAD request
// ---------------------------------------------------------------------------

/**
 * Attempt a HEAD request to $url with a 5-second timeout.
 * Returns an array with keys: reachable (bool), http_code (int), error (string).
 */
function probeEndpoint(string $url, string $key): array
{
    if (!extension_loaded('curl')) {
        return ['reachable' => false, 'http_code' => 0, 'error' => 'cURL extension not loaded'];
    }

    $ch = curl_init($url);
    curl_setopt_array($ch, [
        CURLOPT_NOBODY            => true,
        CURLOPT_RETURNTRANSFER    => true,
        CURLOPT_TIMEOUT           => 5,
        CURLOPT_CONNECTTIMEOUT    => 5,
        CURLOPT_FOLLOWLOCATION    => true,
        CURLOPT_MAXREDIRS         => 3,
        CURLOPT_SSL_VERIFYPEER    => true,
        CURLOPT_SSL_VERIFYHOST    => 2,
        CURLOPT_HTTPHEADER        => [
            'Authorization: Bearer ' . $key,
            'User-Agent: LiveCSS-ConfigTest/1.0',
        ],
    ]);

    $result   = curl_exec($ch);
    $httpCode = (int) curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $error    = curl_error($ch);
    curl_close($ch);

    // Any HTTP response (including 401 Unauthorized) means the host is reachable.
    // Only curl errors (connection refused, DNS failure, timeout) indicate unreachable.
    $reachable = ($result !== false) && ($httpCode > 0);

    return [
        'reachable' => $reachable,
        'http_code' => $httpCode,
        'error'     => $error,
    ];
}

// ---------------------------------------------------------------------------
// Run tests
// ---------------------------------------------------------------------------

if (!$isCLI) {
    header('Content-Type: text/plain; charset=utf-8');
    header('Cache-Control: no-store');
}

echo "Live CSS -- Provider Configuration Test\n";
echo "Timestamp: " . date('Y-m-d H:i:s T') . "\n";

// -- 1. Config file load -------------------------------------------------- //

section('1. Config file');

$config = null;
try {
    $config = AIConfig::load();
    ok('config.json loaded and parsed');
} catch (Throwable $e) {
    fail('config.json failed to load: ' . $e->getMessage());
    exit(1);
}

$keysPath = $config['keys_path'] ?? '';
if ($keysPath !== '') {
    $expanded = str_replace('~', getenv('HOME') ?: posix_getpwuid(posix_getuid())['dir'] ?? '', $keysPath);
    if (is_dir($expanded)) {
        ok('Keys directory exists: ' . $expanded);
    } else {
        fail('Keys directory not found: ' . $expanded);
    }
} else {
    fail('keys_path is not set in config.json');
}

$providers = array_keys($config['providers'] ?? []);
if (count($providers) > 0) {
    ok('Providers defined: ' . implode(', ', $providers));
} else {
    fail('No providers defined in config.json');
    exit(1);
}

// -- 2. Per-provider field checks ----------------------------------------- //

section('2. Provider field validation');

$fieldResults = [];

foreach ($providers as $slug) {
    $p      = $config['providers'][$slug];
    $errors = [];

    $required = ['name', 'base_url', 'default_model', 'timeout', 'models', 'supports_streaming'];
    foreach ($required as $field) {
        if (empty($p[$field]) && $p[$field] !== 0 && $p[$field] !== false) {
            $errors[] = "missing field: $field";
        }
    }

    if (!empty($p['models']) && !in_array($p['default_model'], $p['models'], true)) {
        $errors[] = "default_model '{$p['default_model']}' is not in the models list";
    }

    if (!empty($p['models'])) {
        info("$slug models: " . implode(', ', $p['models']));
    }

    if (empty($errors)) {
        ok("$slug: all required fields present, default_model in list");
    } else {
        foreach ($errors as $e) {
            fail("$slug: $e");
        }
    }

    $fieldResults[$slug] = $errors;
}

// -- 3. API key checks ---------------------------------------------------- //

section('3. API key checks');

$keyResults = [];

foreach ($providers as $slug) {
    $key = '';
    try {
        $key = AIConfig::key($slug);
        ok("$slug: key file found and non-empty");
    } catch (Throwable $e) {
        fail("$slug: " . $e->getMessage());
        $keyResults[$slug] = false;
        continue;
    }

    // Prefix plausibility check
    $prefixes = $KEY_PREFIXES[$slug] ?? [];
    $matched  = false;
    foreach ($prefixes as $prefix) {
        if (str_starts_with($key, $prefix)) {
            $matched = true;
            break;
        }
    }

    if (!empty($prefixes)) {
        if ($matched) {
            ok("$slug: key prefix looks correct (starts with one of: " . implode(', ', $prefixes) . ')');
        } else {
            fail("$slug: key does not start with expected prefix(es): " . implode(', ', $prefixes));
        }
    }

    // Print partial key for visual confirmation (first 8 chars + redacted)
    $redacted = substr($key, 0, 8) . str_repeat('*', max(0, strlen($key) - 12)) . substr($key, -4);
    info("$slug: key preview: $redacted");

    $keyResults[$slug] = $matched || empty($prefixes);
}

// -- 4. Network connectivity check ---------------------------------------- //

section('4. Network connectivity (5 second timeout per provider)');

$netResults = [];

foreach ($providers as $slug) {
    $p       = $config['providers'][$slug];
    $baseUrl = rtrim($p['base_url'] ?? '', '/');

    if ($baseUrl === '') {
        fail("$slug: base_url is empty, skipping connectivity check");
        $netResults[$slug] = false;
        continue;
    }

    // Use a known model listing endpoint where available, else just probe the base
    $probeUrl = match ($slug) {
        'anthropic' => $baseUrl . '/models',
        'openai'    => $baseUrl . '/models',
        'deepseek'  => $baseUrl . '/models',
        default     => $baseUrl,
    };

    $key = '';
    try {
        $key = AIConfig::key($slug);
    } catch (Throwable $ignored) {}

    $result = probeEndpoint($probeUrl, $key);

    if ($result['reachable']) {
        $httpCode = $result['http_code'];
        // 401,403 = host up but auth required (expected on HEAD with bearer).
        // 200,206  = fully open.
        $codeNote = match (true) {
            $httpCode === 200 => 'HTTP 200 OK',
            $httpCode === 401 => 'HTTP 401 (host up, auth required)',
            $httpCode === 403 => 'HTTP 403 (host up, forbidden)',
            $httpCode === 404 => 'HTTP 404 (host up, path not found)',
            default           => 'HTTP ' . $httpCode,
        };
        ok("$slug: host reachable -- $codeNote -- $probeUrl");
    } else {
        $errMsg = $result['error'] ?: ('HTTP ' . $result['http_code']);
        fail("$slug: host NOT reachable -- $errMsg -- $probeUrl");
    }

    $netResults[$slug] = $result['reachable'];
}

// -- Summary -------------------------------------------------------------- //

section('Summary');

$totalPass = 0;
$totalFail = 0;

foreach ($providers as $slug) {
    $fieldOk = empty($fieldResults[$slug]);
    $keyOk   = $keyResults[$slug] ?? false;
    $netOk   = $netResults[$slug] ?? false;

    $overall = ($fieldOk && $keyOk && $netOk) ? 'READY' : 'ISSUES';

    if ($overall === 'READY') {
        $totalPass++;
        ok("$slug: $overall");
    } else {
        $totalFail++;
        $problems = [];
        if (!$fieldOk) { $problems[] = 'config fields'; }
        if (!$keyOk)   { $problems[] = 'key'; }
        if (!$netOk)   { $problems[] = 'network'; }
        fail("$slug: $overall -- problems in: " . implode(', ', $problems));
    }
}

echo "\n";
printf("Providers tested: %d  |  Ready: %d  |  Issues: %d\n", count($providers), $totalPass, $totalFail);
echo "\n";
