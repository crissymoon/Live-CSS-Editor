#!/usr/bin/env php
<?php
/**
 * debug-tool/cli/debug-cli.php
 * CLI tool for sending and managing error tickets.
 *
 * Usage:
 *   php debug-cli.php log --level=critical --title="..." --message="..." [options]
 *   php debug-cli.php list [--level=...] [--status=...] [--limit=20]
 *   php debug-cli.php get <id_or_ticket_id>
 *   php debug-cli.php update <id_or_ticket_id> --status=fixed
 *   php debug-cli.php analyze <id_or_ticket_id>
 *   php debug-cli.php stats
 *   php debug-cli.php delete <id_or_ticket_id>
 *   php debug-cli.php help
 */

// Always flush output immediately for CLI
if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit("This script must be run from the command line.\n");
}

require_once __DIR__ . '/../api/db.php';
require_once __DIR__ . '/../ai/analyze.php';

// ---------------------------------------------------------------------------
// ANSI colors for readable output
// ---------------------------------------------------------------------------
function c(string $text, string $color): string {
    $colors = [
        'red'     => "\033[31m", 'green'  => "\033[32m", 'yellow' => "\033[33m",
        'blue'    => "\033[34m", 'cyan'   => "\033[36m", 'white'  => "\033[37m",
        'bold'    => "\033[1m",  'reset'  => "\033[0m",
    ];
    return ($colors[$color] ?? '') . $text . $colors['reset'];
}

function levelColor(string $level): string {
    return match ($level) {
        'critical' => c($level, 'red'),
        'high'     => c($level, 'red'),
        'medium'   => c($level, 'yellow'),
        'low'      => c($level, 'cyan'),
        'info'     => c($level, 'white'),
        default    => $level,
    };
}

function statusColor(string $status): string {
    return match ($status) {
        'open'        => c($status, 'red'),
        'pending'     => c($status, 'yellow'),
        'in_progress' => c($status, 'blue'),
        'fixed'       => c($status, 'green'),
        'closed'      => c($status, 'green'),
        'wontfix'     => c($status, 'white'),
        default       => $status,
    };
}

// ---------------------------------------------------------------------------
// Argument parser  --key=value / --flag
// ---------------------------------------------------------------------------
function parseArgs(array $argv): array {
    $cmd  = $argv[1] ?? 'help';
    $args = [];
    $pos  = [];

    for ($i = 2; $i < count($argv); $i++) {
        $arg = $argv[$i];
        if (strpos($arg, '--') === 0) {
            $arg = ltrim($arg, '-');
            if (strpos($arg, '=') !== false) {
                [$k, $v]  = explode('=', $arg, 2);
                $args[$k] = $v;
            } else {
                $args[$arg] = true;
            }
        } else {
            $pos[] = $arg;
        }
    }

    return [$cmd, $args, $pos];
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------
function cmdLog(DebugDB $db, array $args): void {
    $data = [
        'level'       => $args['level']       ?? 'info',
        'title'       => $args['title']       ?? '',
        'message'     => $args['message']     ?? '',
        'source'      => $args['source']      ?? 'cli',
        'file'        => $args['file']        ?? null,
        'line'        => isset($args['line']) ? (int)$args['line'] : null,
        'stack_trace' => $args['stack']       ?? null,
    ];

    if (empty($data['title']) || empty($data['message'])) {
        echo c("Error: --title and --message are required.\n", 'red');
        error_log('[DebugCLI] cmdLog: missing title or message');
        exit(1);
    }

    $result = $db->insertError($data);

    if ($result['success']) {
        echo c("Ticket created: ", 'green') . c($result['ticket_id'], 'bold') . " (id={$result['id']})\n";
    } else {
        echo c("Failed: " . $result['error'] . "\n", 'red');
        error_log('[DebugCLI] cmdLog failed: ' . $result['error']);
        exit(1);
    }
}

function cmdList(DebugDB $db, array $args): void {
    $filters = [
        'level'  => $args['level']  ?? '',
        'status' => $args['status'] ?? '',
        'source' => $args['source'] ?? '',
        'search' => $args['search'] ?? '',
        'limit'  => $args['limit']  ?? 20,
        'offset' => $args['offset'] ?? 0,
        'order'  => $args['order']  ?? 'desc',
    ];

    $result = $db->listErrors($filters);

    if (!$result['success']) {
        echo c("Error: " . $result['error'] . "\n", 'red');
        error_log('[DebugCLI] cmdList failed: ' . $result['error']);
        exit(1);
    }

    echo c("Total matching: {$result['total']} | showing " . count($result['data']) . "\n", 'cyan');
    echo str_repeat('-', 80) . "\n";

    foreach ($result['data'] as $row) {
        printf(
            "[%s] %s  %s  %s  %s\n  %s\n\n",
            c($row['id'], 'bold'),
            c($row['ticket_id'], 'cyan'),
            levelColor($row['level']),
            statusColor($row['status']),
            $row['source'] ?? '-',
            $row['title']
        );
    }

    if (empty($result['data'])) {
        echo c("No tickets found.\n", 'yellow');
    }
}

function cmdGet(DebugDB $db, array $args, array $pos): void {
    $id = $pos[0] ?? ($args['id'] ?? '');
    if ($id === '') {
        echo c("Error: provide a ticket id or numeric id.\n", 'red');
        exit(1);
    }

    $result = $db->getError($id);
    if (!$result['success']) {
        echo c("Not found: $id\n", 'red');
        error_log('[DebugCLI] cmdGet not found: ' . $id);
        exit(1);
    }

    $t = $result['data'];
    echo c("\n=== Ticket: {$t['ticket_id']} ===\n", 'bold');
    echo "Level    : " . levelColor($t['level'])   . "\n";
    echo "Status   : " . statusColor($t['status']) . "\n";
    echo "Source   : " . ($t['source'] ?? '-') . "\n";
    echo "File     : " . ($t['file'] ? $t['file'] . ($t['line'] ? ":{$t['line']}" : '') : '-') . "\n";
    echo "Created  : " . $t['created_at'] . "\n";
    echo "Updated  : " . $t['updated_at'] . "\n";
    if ($t['resolved_at']) echo "Resolved : " . $t['resolved_at'] . "\n";
    echo "\nTitle:\n  " . $t['title'] . "\n";
    echo "\nMessage:\n  " . $t['message'] . "\n";
    if ($t['stack_trace']) echo "\nStack Trace:\n" . $t['stack_trace'] . "\n";
    if ($t['context'])     echo "\nContext:\n" . json_encode($t['context'], JSON_PRETTY_PRINT) . "\n";
    if ($t['ai_analysis']) echo "\n" . c("AI Analysis:\n", 'cyan') . $t['ai_analysis'] . "\n";
    echo "\n";
}

function cmdUpdate(DebugDB $db, array $args, array $pos): void {
    $id = $pos[0] ?? ($args['id'] ?? '');
    if ($id === '') {
        echo c("Error: provide a ticket id.\n", 'red');
        exit(1);
    }

    $updates = [];
    foreach (['status', 'level', 'title', 'message'] as $field) {
        if (isset($args[$field])) $updates[$field] = $args[$field];
    }

    if (empty($updates)) {
        echo c("Error: provide at least one field to update (--status, --level, --title, --message).\n", 'red');
        exit(1);
    }

    $result = $db->updateError($id, $updates);
    if ($result['success']) {
        echo c("Updated ticket $id\n", 'green');
    } else {
        echo c("Failed: " . $result['error'] . "\n", 'red');
        error_log('[DebugCLI] cmdUpdate failed: ' . $result['error']);
        exit(1);
    }
}

function cmdAnalyze(DebugDB $db, array $args, array $pos): void {
    $id = $pos[0] ?? ($args['id'] ?? '');
    if ($id === '') {
        echo c("Error: provide a ticket id.\n", 'red');
        exit(1);
    }

    $result = $db->getError($id);
    if (!$result['success']) {
        echo c("Ticket not found: $id\n", 'red');
        error_log('[DebugCLI] cmdAnalyze: ticket not found ' . $id);
        exit(1);
    }

    echo c("Running AI analysis...\n", 'cyan');
    $analysis = analyzeErrorWithAI($result['data']);

    if (!$analysis['success']) {
        echo c("AI analysis failed: " . $analysis['error'] . "\n", 'red');
        error_log('[DebugCLI] AI analysis failed for ' . $id . ': ' . $analysis['error']);
        exit(1);
    }

    // Persist
    $db->updateError($id, ['ai_analysis' => $analysis['analysis']]);

    echo c("\nProvider: " . $analysis['provider'] . "\n", 'cyan');
    echo c("Analysis:\n", 'bold');
    echo $analysis['analysis'] . "\n\n";
}

function cmdStats(DebugDB $db): void {
    $result = $db->stats();
    if (!$result['success']) {
        echo c("Error: " . $result['error'] . "\n", 'red');
        error_log('[DebugCLI] cmdStats failed: ' . $result['error']);
        exit(1);
    }
    $s = $result['stats'];
    echo c("Total tickets: {$s['total']}\n\n", 'bold');
    echo c("By Level:\n", 'cyan');
    foreach ($s['by_level'] as $level => $n) {
        printf("  %-10s %d\n", levelColor($level), $n);
    }
    echo c("\nBy Status:\n", 'cyan');
    foreach ($s['by_status'] as $status => $n) {
        printf("  %-12s %d\n", statusColor($status), $n);
    }
}

function cmdDelete(DebugDB $db, array $args, array $pos): void {
    $id = $pos[0] ?? ($args['id'] ?? '');
    if ($id === '') {
        echo c("Error: provide a ticket id.\n", 'red');
        exit(1);
    }

    echo c("Are you sure you want to delete ticket $id? [y/N] ", 'yellow');
    $confirm = trim(fgets(STDIN));
    if (strtolower($confirm) !== 'y') {
        echo "Cancelled.\n";
        exit(0);
    }

    $result = $db->deleteError($id);
    if ($result['success']) {
        echo c("Deleted ticket $id\n", 'green');
    } else {
        echo c("Failed: " . $result['error'] . "\n", 'red');
        exit(1);
    }
}

function cmdHelp(): void {
    echo <<<HELP

\033[1mdebug-cli.php\033[0m - Live CSS Debug Tool CLI

\033[36mCOMMANDS\033[0m

  \033[1mlog\033[0m       Create a new error ticket
              --level=<critical|high|medium|low|info>  (default: info)
              --title="..."        (required)
              --message="..."      (required)
              --source="..."       (default: cli)
              --file="path/file.php"
              --line=42
              --stack="..."

  \033[1mlist\033[0m      List tickets
              --level=...  --status=...  --source=...  --search=...
              --limit=20   --offset=0    --order=desc

  \033[1mget\033[0m       Show a ticket in detail
              get <id|ticket_id>

  \033[1mupdate\033[0m    Update a ticket
              update <id|ticket_id> --status=fixed
              update <id|ticket_id> --level=medium

  \033[1manalyze\033[0m   Run AI analysis on a ticket
              analyze <id|ticket_id>

  \033[1mstats\033[0m     Show summary statistics

  \033[1mdelete\033[0m    Delete a ticket (asks confirmation)
              delete <id|ticket_id>

  \033[1mhelp\033[0m      Show this help

\033[36mEXAMPLES\033[0m

  php debug-cli.php log --level=critical --title="CSS parse failed" \\
      --message="Parser threw on line 42 of style.css" --source="css-editor"

  php debug-cli.php list --level=critical --status=open
  php debug-cli.php get ERR-20260228-AB12CD
  php debug-cli.php update ERR-20260228-AB12CD --status=fixed
  php debug-cli.php analyze ERR-20260228-AB12CD
  php debug-cli.php stats

HELP;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
try {
    [$cmd, $args, $pos] = parseArgs($argv);
    $db = new DebugDB();

    switch ($cmd) {
        case 'log':     cmdLog($db, $args);          break;
        case 'list':    cmdList($db, $args);         break;
        case 'get':     cmdGet($db, $args, $pos);    break;
        case 'update':  cmdUpdate($db, $args, $pos); break;
        case 'analyze': cmdAnalyze($db, $args, $pos); break;
        case 'stats':   cmdStats($db);               break;
        case 'delete':  cmdDelete($db, $args, $pos); break;
        case 'help':
        default:        cmdHelp();
    }
} catch (Throwable $e) {
    echo c("Fatal: " . $e->getMessage() . "\n", 'red');
    error_log('[DebugCLI] Fatal: ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
    exit(1);
}
