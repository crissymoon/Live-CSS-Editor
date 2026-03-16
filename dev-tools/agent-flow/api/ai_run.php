<?php
/**
 * agent-flow/api/ai_run.php
 *
 * Directly executes a flow against a real AI provider (no moon binary needed).
 * Walks nodes in topological order and calls the provider API for ai-call nodes.
 *
 * POST JSON: { nodes: [...], edges: [...] }
 *
 * Node types handled:
 *   prompt    -- stores props.text into vars[props.varName]
 *   ai-call   -- calls provider API (openai default gpt-4o-mini); stores reply
 *                into vars[props.varName]; reads input from vars[props.inputVar]
 *   condition -- evaluates vars[props.varName]; passes non-empty value through
 *   output    -- appends vars[props.varName] to $outputs[]
 *   loop      -- logs iteration count; no nested execution in this runner
 *   memory    -- stores/recalls a value in a session-scoped $memory map
 *   tool      -- emits the command string as a step result (no shell exec)
 *   agent-task -- executes an allowlisted task by ID from root smoke-tools.json
 *
 * Returns JSON:
 *   { ok: bool, output: string, steps: [{id, type, label, result, error?}], error? }
 *
 * Fallback error handling: every failure path writes to error_log and returns
 * a structured JSON body so the browser always has context.
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// Load shared AI config (relative to this file: agent-flow/api/ -> ../../ai/).
$configPath = __DIR__ . '/../../ai/config.php';
if (!file_exists($configPath)) {
    jsonErr('AI config not found at: ' . realpath(__DIR__ . '/../../ai') . '/config.php', 500);
}
require_once $configPath;

define('AI_RUN_TIMEOUT', 60);
define('TASK_RUN_TIMEOUT_DEFAULT', 240);

// ---- helpers ---------------------------------------------------------------

function jsonErr(string $msg, int $http = 400): void {
    http_response_code($http);
    error_log('agent-flow/ai_run.php: ' . $msg);
    echo json_encode(['ok' => false, 'error' => $msg, 'output' => '', 'steps' => []]);
    exit;
}

/**
 * Topological sort (Kahn's algorithm).
 * Returns an ordered array of node IDs.
 * Any disconnected nodes are appended at the end.
 */
function topoSort(array $nodes, array $edges): array {
    $ids        = array_column($nodes, 'id');
    $inDegree   = array_fill_keys($ids, 0);
    $successors = array_fill_keys($ids, []);

    foreach ($edges as $e) {
        $from = $e['from']['nodeId'] ?? null;
        $to   = $e['to']['nodeId']   ?? null;
        if ($from && $to && isset($inDegree[$to])) {
            $inDegree[$to]++;
            $successors[$from][] = $to;
        }
    }

    $queue   = array_keys(array_filter($inDegree, fn($d) => $d === 0));
    $ordered = [];
    $visited = [];

    while (!empty($queue)) {
        $id = array_shift($queue);
        if (isset($visited[$id])) continue;
        $visited[$id] = true;
        $ordered[]    = $id;
        foreach (($successors[$id] ?? []) as $sid) {
            $inDegree[$sid]--;
            if ($inDegree[$sid] === 0) {
                $queue[] = $sid;
            }
        }
    }

    foreach ($ids as $id) {
        if (!isset($visited[$id])) {
            $ordered[] = $id;
        }
    }

    return $ordered;
}

/**
 * Call OpenAI chat completions (non-streaming).
 * Returns the assistant reply string, or throws RuntimeException on error.
 */
function callOpenAI(string $apiKey, string $baseUrl, string $model, array $messages, int $timeout): string {
    $payload = json_encode([
        'model'    => $model,
        'messages' => $messages,
        'max_completion_tokens' => 2048,
        'stream'   => false,
    ]);

    $ch = curl_init(rtrim($baseUrl, '/') . '/chat/completions');
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => $payload,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => $timeout,
        CURLOPT_HTTPHEADER     => [
            'Content-Type: application/json',
            'Authorization: Bearer ' . $apiKey,
        ],
    ]);

    $resp      = curl_exec($ch);
    $curlErr   = curl_error($ch);
    $httpCode  = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($curlErr) {
        throw new RuntimeException('cURL error: ' . $curlErr);
    }

    if ($httpCode >= 400) {
        $decoded = json_decode($resp, true);
        $apiMsg  = $decoded['error']['message'] ?? $resp;
        throw new RuntimeException('OpenAI HTTP ' . $httpCode . ': ' . $apiMsg);
    }

    $decoded = json_decode($resp, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        throw new RuntimeException('OpenAI returned non-JSON: ' . substr($resp, 0, 200));
    }

    $content = $decoded['choices'][0]['message']['content'] ?? null;
    if ($content === null) {
        throw new RuntimeException('OpenAI response missing choices[0].message.content. Raw: ' . substr($resp, 0, 300));
    }

    return $content;
}

function repoRootPath(): string {
    $root = realpath(__DIR__ . '/../../..');
    if (!$root) {
        throw new RuntimeException('Could not resolve repository root from api directory');
    }
    return $root;
}

function loadSmokeManifest(string $root): array {
    $manifestPath = $root . '/smoke-tools.json';
    if (!is_file($manifestPath)) {
        throw new RuntimeException('smoke-tools.json not found at repo root');
    }
    $raw = file_get_contents($manifestPath);
    if ($raw === false) {
        throw new RuntimeException('Could not read smoke-tools.json');
    }
    $json = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE || !is_array($json)) {
        throw new RuntimeException('Invalid smoke-tools.json: ' . json_last_error_msg());
    }
    $tools = $json['tools'] ?? null;
    if (!is_array($tools)) {
        throw new RuntimeException('smoke-tools.json missing tools array');
    }
    return $tools;
}

function findToolById(array $tools, string $taskId): ?array {
    foreach ($tools as $tool) {
        if (!is_array($tool)) continue;
        if (($tool['id'] ?? '') === $taskId) {
            return $tool;
        }
    }
    return null;
}

function startsWithPath(string $path, string $prefix): bool {
    $pathNorm = str_replace('\\', '/', $path);
    $prefixNorm = rtrim(str_replace('\\', '/', $prefix), '/');
    return $pathNorm === $prefixNorm || str_starts_with($pathNorm, $prefixNorm . '/');
}

function runAllowlistedTask(array $tool, string $root, int $timeoutSec): array {
    $cmd = $tool['command'] ?? [];
    if (!is_array($cmd) || count($cmd) === 0) {
        throw new RuntimeException('task command is missing or invalid');
    }

    $cwdRel = (string)($tool['cwd'] ?? '.');
    $cwdResolved = realpath($root . '/' . ltrim($cwdRel, '/'));
    if (!$cwdResolved || !is_dir($cwdResolved)) {
        throw new RuntimeException('task cwd does not exist: ' . $cwdRel);
    }
    if (!startsWithPath($cwdResolved, $root)) {
        throw new RuntimeException('task cwd escapes repository root');
    }

    $cmdParts = [];
    foreach ($cmd as $part) {
        if (!is_scalar($part)) {
            throw new RuntimeException('task command contains non-scalar argument');
        }
        $cmdParts[] = escapeshellarg((string)$part);
    }
    $commandString = implode(' ', $cmdParts);

    $desc = [
        0 => ['pipe', 'r'],
        1 => ['pipe', 'w'],
        2 => ['pipe', 'w'],
    ];
    $proc = proc_open($commandString, $desc, $pipes, $cwdResolved);
    if (!is_resource($proc)) {
        throw new RuntimeException('proc_open failed for task command');
    }

    fclose($pipes[0]);
    stream_set_blocking($pipes[1], false);
    stream_set_blocking($pipes[2], false);

    $stdout = '';
    $stderr = '';
    $start = time();

    while (true) {
        $status = proc_get_status($proc);
        $stdout .= stream_get_contents($pipes[1]);
        $stderr .= stream_get_contents($pipes[2]);

        if (!$status['running']) {
            break;
        }
        if ((time() - $start) > $timeoutSec) {
            proc_terminate($proc, 9);
            $stderr .= "\n[agent-flow] Task killed after timeout {$timeoutSec}s.";
            break;
        }
        usleep(50000);
    }

    $stdout .= stream_get_contents($pipes[1]);
    $stderr .= stream_get_contents($pipes[2]);
    fclose($pipes[1]);
    fclose($pipes[2]);
    $exitCode = proc_close($proc);

    return [
        'exit_code' => $exitCode,
        'stdout' => $stdout,
        'stderr' => $stderr,
        'cwd' => $cwdResolved,
    ];
}

// ---- request validation ----------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonErr('Method not allowed -- use POST', 405);
}

$raw = file_get_contents('php://input');
if (!$raw) {
    jsonErr('Empty request body', 400);
}

$body = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    jsonErr('Invalid JSON body: ' . json_last_error_msg(), 400);
}

$nodes = $body['nodes'] ?? [];
$edges = $body['edges'] ?? [];

if (!is_array($nodes) || empty($nodes)) {
    jsonErr('No nodes in flow', 400);
}

// ---- prepare provider (openai) ---------------------------------------------

try {
    $openaiCfg = AIConfig::provider('openai');
} catch (Throwable $e) {
    jsonErr('Could not load OpenAI config: ' . $e->getMessage(), 500);
}

$apiKey  = $openaiCfg['api_key'];
$baseUrl = $openaiCfg['base_url'];
$timeout = $openaiCfg['timeout'] ?? AI_RUN_TIMEOUT;

// ---- execute flow ----------------------------------------------------------

$nodeMap = [];
foreach ($nodes as $n) {
    $nodeMap[$n['id']] = $n;
}

$ordered = topoSort($nodes, $edges);

$vars    = [];   // variable name -> value
$memory  = [];   // memory store for memory nodes
$outputs = [];   // collected output values
$steps   = [];   // step log
$halted  = false; // set true when a guard node blocks the flow

foreach ($ordered as $nodeId) {
    $node  = $nodeMap[$nodeId] ?? null;
    if (!$node) continue;

    $type  = $node['type']  ?? 'unknown';
    $label = $node['label'] ?? $type;
    $props = $node['props'] ?? [];

    $step = ['id' => $nodeId, 'type' => $type, 'label' => $label, 'result' => ''];

    /* if a guard node halted the flow, skip all subsequent nodes */
    if ($halted) {
        $step['result'] = 'skipped: flow halted by guard node';
        $step['skipped'] = true;
        $steps[] = $step;
        continue;
    }

    try {
        switch ($type) {

            case 'prompt': {
                $varName = $props['varName'] ?? 'prompt';
                $text    = $props['text']    ?? '';
                $vars[$varName] = $text;
                $step['result'] = 'set $' . $varName . ' = "' . substr($text, 0, 80) . (strlen($text) > 80 ? '...' : '') . '"';
                break;
            }

            case 'ai-call': {
                $provider  = strtolower($props['provider'] ?? 'openai');
                $model     = $props['model']     ?? 'gpt-4o-mini';
                $varName   = $props['varName']   ?? 'response';
                $inputVar  = $props['inputVar']  ?? 'prompt';
                $systemMsg = $props['system']    ?? 'You are a helpful assistant.';

                // Resolve input text from the variable map.
                $inputText = $vars[$inputVar] ?? '';
                if ($inputText === '' && isset($vars['prompt'])) {
                    $inputText = $vars['prompt'];  // fallback to 'prompt' if inputVar unset
                }

                if ($inputText === '') {
                    $step['result'] = 'skipped -- inputVar $' . $inputVar . ' is empty';
                    $vars[$varName] = '';
                    break;
                }

                // Only openai is wired for direct calling.
                if ($provider !== 'openai') {
                    $step['result'] = 'provider "' . $provider . '" not supported in ai_run.php -- only openai is wired here';
                    $step['error']  = 'unsupported provider';
                    $vars[$varName] = '';
                    break;
                }

                $messages = [
                    ['role' => 'system', 'content' => $systemMsg],
                    ['role' => 'user',   'content' => $inputText],
                ];

                $reply = callOpenAI($apiKey, $baseUrl, $model, $messages, $timeout);
                $vars[$varName]  = $reply;
                $step['result']  = $reply;
                $step['model']   = $model;
                $step['inputLen'] = strlen($inputText);
                break;
            }

            case 'condition': {
                $varName = $props['varName'] ?? '';
                $value   = $varName !== '' ? ($vars[$varName] ?? '') : '';
                $branch  = ($value !== '' && $value !== null && $value !== false)
                    ? ($props['trueLabel']  ?? 'yes')
                    : ($props['falseLabel'] ?? 'no');
                $step['result'] = 'branch: ' . $branch . ' (value of $' . $varName . ': "' . substr((string)$value, 0, 60) . '")';
                break;
            }

            case 'loop': {
                $count    = (int)($props['count']    ?? 5);
                $indexVar = $props['indexVar'] ?? 'i';
                $step['result'] = 'loop: would iterate ' . $count . ' times using $' . $indexVar . ' (use moon runner for actual iteration)';
                break;
            }

            case 'memory': {
                $op       = $props['op']       ?? 'keep';
                $varName  = $props['varName']  ?? 'result';
                $inputVar = $props['inputVar'] ?? '';
                if ($op === 'recall') {
                    $recalled       = $memory[$inputVar] ?? '';
                    $vars[$varName] = $recalled;
                    $step['result'] = 'recalled $' . $inputVar . ' -> $' . $varName . ': "' . substr($recalled, 0, 60) . '"';
                } else {
                    $val           = $vars[$inputVar] ?? ($vars[$varName] ?? '');
                    $memory[$inputVar ?: $varName] = $val;
                    $step['result'] = 'kept $' . ($inputVar ?: $varName) . ': "' . substr((string)$val, 0, 60) . '"';
                }
                break;
            }

            case 'tool': {
                $cmd = $props['command'] ?? '';
                $step['result'] = 'tool node (no exec in ai_run) -- command: ' . $cmd;
                break;
            }

            case 'agent-task': {
                $taskId = trim((string)($props['taskId'] ?? ''));
                $timeoutSec = (int)($props['timeoutSec'] ?? TASK_RUN_TIMEOUT_DEFAULT);
                if ($timeoutSec < 5) $timeoutSec = 5;
                if ($timeoutSec > 1800) $timeoutSec = 1800;
                $failFlowOnError = (($props['failFlowOnError'] ?? 'true') === 'true');

                if ($taskId === '') {
                    throw new RuntimeException('agent-task requires taskId');
                }

                $root = repoRootPath();
                $tools = loadSmokeManifest($root);
                $tool = findToolById($tools, $taskId);
                if (!$tool) {
                    throw new RuntimeException('taskId not found in smoke-tools.json: ' . $taskId);
                }

                $run = runAllowlistedTask($tool, $root, $timeoutSec);
                $step['task_id'] = $taskId;
                $step['task_exit_code'] = $run['exit_code'];
                $step['task_cwd'] = $run['cwd'];

                $stdoutTail = trim(substr((string)$run['stdout'], -800));
                $stderrTail = trim(substr((string)$run['stderr'], -800));

                $summary = 'task ' . $taskId . ' exit=' . $run['exit_code'];
                if ($stdoutTail !== '') {
                    $summary .= "\nstdout tail:\n" . $stdoutTail;
                }
                if ($stderrTail !== '') {
                    $summary .= "\nstderr tail:\n" . $stderrTail;
                }
                $step['result'] = $summary;

                if ((int)$run['exit_code'] !== 0) {
                    $step['error'] = 'task failed with exit code ' . $run['exit_code'];
                    if ($failFlowOnError) {
                        $halted = true;
                        $step['halted'] = true;
                        $step['result'] .= "\nflow halted (failFlowOnError=true)";
                    }
                }
                break;
            }

            case 'output': {
                $varName = $props['varName'] ?? 'response';
                $value   = $vars[$varName]  ?? '';
                $outputs[] = $value;
                $step['result'] = $value;
                break;
            }

            case 'guard': {
                $inputVar    = $props['inputVar']    ?? 'prompt';
                $varName     = $props['varName']     ?? 'guard_result';
                $guardUrl    = $props['guardUrl']    ?? 'http://localhost:8765/classify';
                $blockOnFlag = ($props['blockOnFlag'] ?? 'true') === 'true';

                $inputText = $vars[$inputVar] ?? ($vars['prompt'] ?? '');

                $guardPayload = json_encode(['text' => $inputText]);
                $ch = curl_init($guardUrl);
                curl_setopt_array($ch, [
                    CURLOPT_POST           => true,
                    CURLOPT_POSTFIELDS     => $guardPayload,
                    CURLOPT_RETURNTRANSFER => true,
                    CURLOPT_TIMEOUT        => 10,
                    CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
                    CURLOPT_CONNECTTIMEOUT => 5,
                ]);
                $guardResp = curl_exec($ch);
                $guardErr  = curl_error($ch);
                $guardCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
                curl_close($ch);

                if ($guardErr || $guardCode >= 400) {
                    // Guard server unreachable -- log and continue without blocking
                    $errDetail = $guardErr ?: 'HTTP ' . $guardCode;
                    error_log('agent-flow/ai_run.php guard node ' . $nodeId . ': guard server error: ' . $errDetail);
                    $vars[$varName] = json_encode([
                        'ok'        => false,
                        'error'     => 'guard server unreachable: ' . $errDetail,
                        'flagged'   => false,
                        'label'     => 'clean',
                        'confidence'=> 0,
                    ]);
                    $step['result'] = 'guard server unreachable (' . $errDetail . ') -- flow continues (fail-open)';
                    $step['guard_error'] = $errDetail;
                    break;
                }

                $guardData = json_decode($guardResp, true);
                if (json_last_error() !== JSON_ERROR_NONE || !isset($guardData['ok'])) {
                    error_log('agent-flow/ai_run.php guard node ' . $nodeId . ': invalid JSON from guard server: ' . substr($guardResp, 0, 200));
                    $vars[$varName] = json_encode(['ok' => false, 'error' => 'invalid guard response', 'flagged' => false, 'label' => 'clean']);
                    $step['result'] = 'guard returned invalid JSON -- flow continues (fail-open)';
                    break;
                }

                $vars[$varName] = $guardResp; // store raw JSON string
                $step['guard_label']      = $guardData['label']      ?? 'unknown';
                $step['guard_confidence'] = $guardData['confidence'] ?? 0;
                $step['guard_flagged']    = $guardData['flagged']    ?? false;
                $step['guard_source']     = $guardData['source']     ?? 'unknown';

                if ($guardData['flagged'] ?? false) {
                    $summary = 'FLAGGED as ' . ($guardData['label'] ?? 'unknown')
                        . ' (confidence: ' . number_format(($guardData['confidence'] ?? 0) * 100, 1) . '%)';
                    if ($blockOnFlag) {
                        $halted = true;
                        $step['result'] = $summary . ' -- flow halted (blockOnFlag=true)';
                        $step['halted'] = true;
                    } else {
                        $step['result'] = $summary . ' -- flow continues (blockOnFlag=false)';
                    }
                } else {
                    $label_str = $guardData['label'] ?? 'clean';
                    $step['result'] = 'clean (' . $label_str . ', confidence: '
                        . number_format(($guardData['confidence'] ?? 0) * 100, 1) . '%) -- flow continues';
                }
                break;
            }

            default: {
                $step['result'] = 'unknown node type: ' . $type;
                $step['error']  = 'unhandled type';
                break;
            }
        }
    } catch (Throwable $e) {
        $errMsg = $e->getMessage();
        error_log('agent-flow/ai_run.php node ' . $nodeId . ' (' . $type . '): ' . $errMsg);
        $step['error']  = $errMsg;
        $step['result'] = 'ERROR: ' . $errMsg;
    }

    $steps[] = $step;
}

$finalOutput = implode("\n\n", array_filter($outputs, fn($o) => $o !== ''));
if ($finalOutput === '' && !empty($steps)) {
    // No output node -- show the last ai-call result or last step result.
    foreach (array_reverse($steps) as $s) {
        if (!empty($s['result']) && !isset($s['error']) && !isset($s['skipped'])) {
            $finalOutput = $s['result'];
            break;
        }
    }
}

if ($halted && $finalOutput === '') {
    $finalOutput = '[flow halted by guard node -- input was flagged]';
}

echo json_encode([
    'ok'     => true,
    'halted' => $halted,
    'output' => $finalOutput,
    'steps'  => $steps,
]);
