<?php
/**
 * debug-tool/ai/analyze.php
 * Bridges the debug-tool with the existing AI providers in ai/config.php.
 * Reads an error ticket and returns an AI-generated root-cause analysis.
 */

// Pull in the Live CSS app's existing AI config if available
$appConfig = __DIR__ . '/../../ai/config.php';
if (file_exists($appConfig)) {
    require_once $appConfig;
}

/**
 * analyzeErrorWithAI()
 * Sends an error ticket to an AI model and returns structured analysis.
 *
 * @param  array  $ticket   Row from errors table
 * @return array  ['success' => bool, 'analysis' => string, 'provider' => string]
 */
function analyzeErrorWithAI(array $ticket): array {
    try {
        // Build the prompt
        $prompt = buildAnalysisPrompt($ticket);

        // Try providers in order of preference
        $providers = resolveProviders();

        foreach ($providers as $provider) {
            $result = callProvider($provider, $prompt);
            if ($result['success']) {
                error_log('[DebugAI] Analysis completed via ' . $provider . ' for ticket ' . ($ticket['ticket_id'] ?? '?'));
                return $result;
            }
            error_log('[DebugAI] Provider ' . $provider . ' failed, trying next...');
        }

        error_log('[DebugAI] All providers failed for ticket ' . ($ticket['ticket_id'] ?? '?'));
        return ['success' => false, 'error' => 'All AI providers failed or none configured', 'analysis' => null];

    } catch (Throwable $e) {
        error_log('[DebugAI] analyzeErrorWithAI exception: ' . $e->getMessage());
        return ['success' => false, 'error' => $e->getMessage(), 'analysis' => null];
    }
}

// ---------------------------------------------------------------------------
// Prompt builder
// ---------------------------------------------------------------------------
function buildAnalysisPrompt(array $ticket): string {
    $ctx = '';
    if (!empty($ticket['context'])) {
        $ctxData = is_array($ticket['context']) ? $ticket['context'] : json_decode($ticket['context'], true);
        if ($ctxData) {
            $ctx = "\nContext:\n" . json_encode($ctxData, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
        }
    }

    $stack = $ticket['stack_trace'] ? "\nStack trace:\n" . $ticket['stack_trace'] : '';
    $file  = $ticket['file']  ? "File: {$ticket['file']}" . ($ticket['line'] ? " line {$ticket['line']}" : '') : '';

    return <<<PROMPT
You are a senior developer debugging a CSS/PHP/JavaScript web application called "Live CSS Editor".
Analyze the following error ticket and provide:
1. A likely root cause (1-2 sentences)
2. Suggested fix (step-by-step, max 5 steps)
3. Severity assessment and why
4. Any related areas of the codebase that might also be affected

Error Ticket: {$ticket['ticket_id']}
Level: {$ticket['level']}
Status: {$ticket['status']}
Source: {$ticket['source']}
{$file}
Title: {$ticket['title']}
Message: {$ticket['message']}{$stack}{$ctx}

Respond in plain text, no markdown headers needed.
PROMPT;
}

// ---------------------------------------------------------------------------
// Provider resolution - read from existing app config or env vars
// ---------------------------------------------------------------------------
function resolveProviders(): array {
    $providers = [];

    // Check what keys are available in the environment or app config
    if (!empty(getenv('OPENAI_API_KEY')) || (defined('OPENAI_KEY') && OPENAI_KEY)) {
        $providers[] = 'openai';
    }
    if (!empty(getenv('ANTHROPIC_API_KEY')) || (defined('ANTHROPIC_KEY') && ANTHROPIC_KEY)) {
        $providers[] = 'anthropic';
    }
    if (!empty(getenv('DEEPSEEK_API_KEY')) || (defined('DEEPSEEK_KEY') && DEEPSEEK_KEY)) {
        $providers[] = 'deepseek';
    }

    // Also try to read from the app's config.json
    $configJson = __DIR__ . '/../../ai/config.json';
    if (file_exists($configJson)) {
        $cfg = json_decode(file_get_contents($configJson), true);
        if (!empty($cfg['openai_key'])    && !in_array('openai',    $providers)) $providers[] = 'openai';
        if (!empty($cfg['anthropic_key']) && !in_array('anthropic', $providers)) $providers[] = 'anthropic';
        if (!empty($cfg['deepseek_key'])  && !in_array('deepseek',  $providers)) $providers[] = 'deepseek';
    }

    if (empty($providers)) {
        error_log('[DebugAI] No AI providers configured. Set OPENAI_API_KEY, ANTHROPIC_API_KEY, or DEEPSEEK_API_KEY.');
    }

    return $providers;
}

// ---------------------------------------------------------------------------
// Provider callers
// ---------------------------------------------------------------------------
function callProvider(string $provider, string $prompt): array {
    switch ($provider) {
        case 'openai':    return callOpenAI($prompt);
        case 'anthropic': return callAnthropic($prompt);
        case 'deepseek':  return callDeepSeek($prompt);
        default:
            error_log('[DebugAI] Unknown provider: ' . $provider);
            return ['success' => false, 'error' => 'Unknown provider'];
    }
}

function callOpenAI(string $prompt): array {
    $key = getenv('OPENAI_API_KEY') ?: (defined('OPENAI_KEY') ? OPENAI_KEY : readConfigJsonKey('openai_key'));
    if (!$key) {
        error_log('[DebugAI] callOpenAI: no API key found');
        return ['success' => false, 'error' => 'No OpenAI key'];
    }

    $payload = json_encode([
        'model'    => 'gpt-4o-mini',
        'messages' => [['role' => 'user', 'content' => $prompt]],
        'max_tokens' => 1000,
    ]);

    $response = httpPost('https://api.openai.com/v1/chat/completions', $payload, [
        'Authorization: Bearer ' . $key,
        'Content-Type: application/json',
    ]);

    if ($response['error']) {
        error_log('[DebugAI] OpenAI HTTP error: ' . $response['error']);
        return ['success' => false, 'error' => $response['error']];
    }

    $data = json_decode($response['body'], true);
    if (empty($data['choices'][0]['message']['content'])) {
        error_log('[DebugAI] OpenAI unexpected response: ' . $response['body']);
        return ['success' => false, 'error' => 'Unexpected OpenAI response'];
    }

    return ['success' => true, 'analysis' => trim($data['choices'][0]['message']['content']), 'provider' => 'openai'];
}

function callAnthropic(string $prompt): array {
    $key = getenv('ANTHROPIC_API_KEY') ?: (defined('ANTHROPIC_KEY') ? ANTHROPIC_KEY : readConfigJsonKey('anthropic_key'));
    if (!$key) {
        error_log('[DebugAI] callAnthropic: no API key found');
        return ['success' => false, 'error' => 'No Anthropic key'];
    }

    $payload = json_encode([
        'model'      => 'claude-3-5-haiku-20241022',
        'max_tokens' => 1000,
        'messages'   => [['role' => 'user', 'content' => $prompt]],
    ]);

    $response = httpPost('https://api.anthropic.com/v1/messages', $payload, [
        'x-api-key: ' . $key,
        'anthropic-version: 2023-06-01',
        'Content-Type: application/json',
    ]);

    if ($response['error']) {
        error_log('[DebugAI] Anthropic HTTP error: ' . $response['error']);
        return ['success' => false, 'error' => $response['error']];
    }

    $data = json_decode($response['body'], true);
    if (empty($data['content'][0]['text'])) {
        error_log('[DebugAI] Anthropic unexpected response: ' . $response['body']);
        return ['success' => false, 'error' => 'Unexpected Anthropic response'];
    }

    return ['success' => true, 'analysis' => trim($data['content'][0]['text']), 'provider' => 'anthropic'];
}

function callDeepSeek(string $prompt): array {
    $key = getenv('DEEPSEEK_API_KEY') ?: (defined('DEEPSEEK_KEY') ? DEEPSEEK_KEY : readConfigJsonKey('deepseek_key'));
    if (!$key) {
        error_log('[DebugAI] callDeepSeek: no API key found');
        return ['success' => false, 'error' => 'No DeepSeek key'];
    }

    $payload = json_encode([
        'model'    => 'deepseek-chat',
        'messages' => [['role' => 'user', 'content' => $prompt]],
        'max_tokens' => 1000,
    ]);

    $response = httpPost('https://api.deepseek.com/v1/chat/completions', $payload, [
        'Authorization: Bearer ' . $key,
        'Content-Type: application/json',
    ]);

    if ($response['error']) {
        error_log('[DebugAI] DeepSeek HTTP error: ' . $response['error']);
        return ['success' => false, 'error' => $response['error']];
    }

    $data = json_decode($response['body'], true);
    if (empty($data['choices'][0]['message']['content'])) {
        error_log('[DebugAI] DeepSeek unexpected response: ' . $response['body']);
        return ['success' => false, 'error' => 'Unexpected DeepSeek response'];
    }

    return ['success' => true, 'analysis' => trim($data['choices'][0]['message']['content']), 'provider' => 'deepseek'];
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------
function httpPost(string $url, string $payload, array $headers): array {
    $ch = curl_init($url);
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => $payload,
        CURLOPT_HTTPHEADER     => $headers,
        CURLOPT_TIMEOUT        => 30,
        CURLOPT_SSL_VERIFYPEER => true,
    ]);
    $body  = curl_exec($ch);
    $errno = curl_errno($ch);
    $err   = curl_error($ch);
    curl_close($ch);

    if ($errno || $body === false) {
        return ['body' => '', 'error' => "cURL error $errno: $err"];
    }
    return ['body' => $body, 'error' => null];
}

function readConfigJsonKey(string $key): string {
    static $cfg = null;
    if ($cfg === null) {
        $path = __DIR__ . '/../../ai/config.json';
        $cfg  = file_exists($path) ? (json_decode(file_get_contents($path), true) ?? []) : [];
    }
    return $cfg[$key] ?? '';
}
