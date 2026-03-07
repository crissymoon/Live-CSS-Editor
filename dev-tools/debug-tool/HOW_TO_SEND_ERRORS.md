# HOW_TO_SEND_ERRORS.md

How to send errors to the Live CSS Debug Tool.

---

## Overview

The debug tool accepts error tickets via three channels:

| Channel | Use case |
|---|---|
| HTTP API | Other PHP files, cURL, fetch, any HTTP client |
| CLI | Shell scripts, cron jobs, terminal |
| JS client | Browser-side JS inside the Live CSS app |

Each ticket gets a unique ID like `ERR-20260228-AB12CD` and a status that moves through a lifecycle.

---

## Error levels

| Level | When to use |
|---|---|
| `critical` | App is broken, data loss risk, cannot continue |
| `high` | Major feature broken, severe UX impact |
| `medium` | Non-blocking bug, wrong output, degraded experience |
| `low` | Minor cosmetic issue, edge case |
| `info` | Informational log, not really an error |

---

## Ticket statuses

| Status | Meaning |
|---|---|
| `open` | New, not yet looked at (default) |
| `pending` | Awaiting more info or a dependency |
| `in_progress` | Actively being worked on |
| `fixed` | Resolved, `resolved_at` is set automatically |
| `closed` | Acknowledged and closed without a fix |
| `wontfix` | Will not be fixed |

---

## 1. HTTP API (PHP or cURL)

### Create a ticket

```bash
curl -s -X POST http://localhost/debug-tool/api/ \
  -H "Content-Type: application/json" \
  -d '{
    "level":   "critical",
    "title":   "CSS parser crash",
    "message": "Uncaught exception in parser.php on line 88",
    "source":  "style-sheets/parser.php",
    "file":    "style-sheets/parser.php",
    "line":    88,
    "context": { "user": "admin", "css_file": "atom-age.css" }
  }'
```

Response:

```json
{
  "success": true,
  "id": 1,
  "ticket_id": "ERR-20260228-AB12CD"
}
```

### From PHP

```php
<?php
// In any PHP file in the Live CSS project

function reportError(string $level, string $title, string $message, array $extra = []): ?string {
    $payload = array_merge([
        'level'   => $level,
        'title'   => $title,
        'message' => $message,
        'source'  => basename(__FILE__),
        'file'    => __FILE__,
        'line'    => __LINE__,
    ], $extra);

    $ch = curl_init('/debug-tool/api/');
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => json_encode($payload),
        CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
    ]);
    $response = curl_exec($ch);
    $errno    = curl_errno($ch);
    curl_close($ch);

    if ($errno) {
        error_log("reportError: cURL failed errno=$errno");
        return null;
    }

    $json = json_decode($response, true);
    if (!empty($json['ticket_id'])) {
        error_log("Debug ticket: {$json['ticket_id']}");
        return $json['ticket_id'];
    }
    return null;
}

// Usage
reportError('high', 'AI provider timeout', 'OpenAI call exceeded 30s', [
    'source'  => 'ai/openai.php',
    'context' => ['model' => 'gpt-4o', 'attempt' => 2],
]);
```

### Or - include db.php directly (no HTTP needed)

```php
<?php
require_once __DIR__ . '/../debug-tool/api/db.php';

$db     = new DebugDB();
$result = $db->insertError([
    'level'   => 'medium',
    'title'   => 'Theme randomizer produced invalid CSS',
    'message' => 'Property value out of range: opacity -0.2',
    'source'  => 'ai/theme-randomizer.php',
    'context' => ['property' => 'opacity', 'value' => '-0.2'],
]);

if ($result['success']) {
    error_log('Ticket: ' . $result['ticket_id']);
} else {
    error_log('Failed to log error: ' . $result['error']);
}
```

---

## 2. CLI

```bash
# From the project root
php debug-tool/cli/debug-cli.php log \
  --level=critical \
  --title="DB migration failed" \
  --message="SQLite WAL error on startup" \
  --source="db-browser" \
  --file="db-browser/core/db_manager.c" \
  --line=204

# List open critical tickets
php debug-tool/cli/debug-cli.php list --level=critical --status=open

# View a ticket
php debug-tool/cli/debug-cli.php get ERR-20260228-AB12CD

# Mark as fixed
php debug-tool/cli/debug-cli.php update ERR-20260228-AB12CD --status=fixed

# Run AI analysis on a ticket
php debug-tool/cli/debug-cli.php analyze ERR-20260228-AB12CD

# Stats overview
php debug-tool/cli/debug-cli.php stats
```

---

## 3. JavaScript (browser)

Include the client once per page load:

```html
<script src="/debug-tool/js/debug-client.js"></script>
```

Then configure and use it:

```js
// Optional: override defaults once on app init
DebugTool.configure({
    endpoint:     '/debug-tool/api/',
    source:       'live-css-frontend',
    minPostLevel: 'low',     // do not POST info-level, only log to console
});

// Auto-capture all uncaught JS errors
DebugTool.hookUncaughtErrors();

// Manual calls
DebugTool.critical('CodeMirror failed to init', 'Editor container not found in DOM', {
    file:    'js/editor.js',
    context: { selector: '#editor', readyState: document.readyState },
});

DebugTool.medium('Color swatch render failed', 'Canvas context returned null');

DebugTool.info('Theme loaded', 'atom-age.css applied successfully');

// Full options
DebugTool.log({
    level:   'high',
    title:   'Fuzzy search timeout',
    message: 'Search took > 500ms, results may be incomplete',
    source:  'js/fuzzy.js',
    file:    'js/fuzzy.js',
    line:    112,
    context: { query: 'border-rad', resultCount: 0 },
});
```

Errors always print to the browser console first. The API POST is a fire-and-forget and will not throw on failure.

---

## 4. AI Analysis

When a ticket is created, you can request an AI root-cause analysis:

```bash
# CLI
php debug-tool/cli/debug-cli.php analyze ERR-20260228-AB12CD

# API
curl -X POST "http://localhost/debug-tool/api/?action=analyze&id=ERR-20260228-AB12CD"
```

The analysis is stored in the `ai_analysis` field of the ticket and uses whichever AI provider key is available (OpenAI, Anthropic, or DeepSeek). Keys are read from:
- Environment variables: `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, `DEEPSEEK_API_KEY`
- The app's existing `ai/config.json`

---

## 5. API Reference

| Method | URL | Action |
|---|---|---|
| `POST` | `/debug-tool/api/` | Create ticket |
| `GET` | `/debug-tool/api/` | List tickets |
| `GET` | `/debug-tool/api/?id=X` | Get single ticket |
| `PATCH` | `/debug-tool/api/?id=X` | Update ticket |
| `DELETE` | `/debug-tool/api/?id=X` | Delete ticket |
| `GET` | `/debug-tool/api/?action=stats` | Stats summary |
| `POST` | `/debug-tool/api/?action=analyze&id=X` | AI analysis |

### Query filters for GET list

| Param | Values |
|---|---|
| `level` | critical, high, medium, low, info |
| `status` | open, pending, in_progress, fixed, closed, wontfix |
| `source` | any string |
| `search` | searches title and message |
| `limit` | 1-200 (default 50) |
| `offset` | for pagination |
| `order` | asc or desc (default desc) |

---

## 6. Optional: API key protection

Set the environment variable `DEBUG_API_KEY` to a secret string, then pass it in requests:

```bash
export DEBUG_API_KEY="my-secret-key"
```

```bash
curl -H "X-Debug-Key: my-secret-key" http://localhost/debug-tool/api/?action=stats
```

```js
DebugTool.configure({ apiKey: 'my-secret-key' });
```

---

## Database location

```
debug-tool/db/errors.db
```

Direct web access is blocked by `.htaccess`. The DB is auto-created on first use.
