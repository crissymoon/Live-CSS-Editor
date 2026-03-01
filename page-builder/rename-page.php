<?php
declare(strict_types=1);

/**
 * rename-page.php
 * POST-only API. Renames a page directory and updates project.json references.
 *
 * Body JSON: { "page": "old-name", "new_name": "new-name" }
 * Response:  { "ok": true } | { "ok": false, "error": "..." }
 */

header('Content-Type: application/json; charset=utf-8');

// ------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------

function renameOk(mixed $extra = []): never {
    echo json_encode(array_merge(['ok' => true], is_array($extra) ? $extra : []));
    exit;
}

function renameErr(string $msg, int $code = 400): never {
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    error_log('[rename-page] error: ' . $msg);
    exit;
}

/** Validate a page name: lowercase letters, digits, hyphens, underscores only. */
function validName(string $n): bool {
    return $n !== '' && (bool) preg_match('/^[a-z0-9_-]+$/', $n);
}

// ------------------------------------------------------------------
// Route
// ------------------------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    renameErr('POST required', 405);
}

$body = (string) file_get_contents('php://input');
$req  = json_decode($body, true);

if (!is_array($req)) {
    renameErr('Invalid JSON body');
}

$oldName = trim((string) ($req['page']     ?? ''));
$newName = trim((string) ($req['new_name'] ?? ''));

if (!validName($oldName)) {
    renameErr('Invalid current page name');
}
if (!validName($newName)) {
    renameErr('Invalid new page name -- use lowercase letters, digits, hyphens and underscores only');
}
if ($oldName === $newName) {
    renameErr('New name is the same as the current name');
}

$pagesRoot = __DIR__ . '/pages';
$oldDir    = $pagesRoot . '/' . $oldName;
$newDir    = $pagesRoot . '/' . $newName;

if (!is_dir($oldDir)) {
    renameErr('Page "' . $oldName . '" does not exist', 404);
}
if (file_exists($newDir)) {
    renameErr('A page named "' . $newName . '" already exists');
}

// ------------------------------------------------------------------
// Rename the directory
// ------------------------------------------------------------------

if (!rename($oldDir, $newDir)) {
    renameErr('Failed to rename directory -- check filesystem permissions', 500);
}

error_log('[rename-page] renamed "' . $oldName . '" -> "' . $newName . '"');

// ------------------------------------------------------------------
// Update project.json (index_page + slugs keys)
// ------------------------------------------------------------------

$projectFile = __DIR__ . '/project.json';
if (file_exists($projectFile)) {
    $raw = file_get_contents($projectFile);
    if ($raw !== false) {
        $cfg = json_decode($raw, true);
        if (is_array($cfg)) {
            $changed = false;

            // index_page
            if (isset($cfg['index_page']) && $cfg['index_page'] === $oldName) {
                $cfg['index_page'] = $newName;
                $changed = true;
                error_log('[rename-page] updated project.json index_page');
            }

            // slugs map key
            if (isset($cfg['slugs']) && is_array($cfg['slugs'])) {
                if (array_key_exists($oldName, $cfg['slugs'])) {
                    $cfg['slugs'][$newName] = $cfg['slugs'][$oldName];
                    unset($cfg['slugs'][$oldName]);
                    $changed = true;
                    error_log('[rename-page] updated project.json slugs key');
                }
            }

            if ($changed) {
                $cfg['updated_at'] = date('c');
                $out = json_encode($cfg, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
                if ($out !== false) {
                    file_put_contents($projectFile, $out);
                } else {
                    error_log('[rename-page] WARNING: failed to encode project.json after rename');
                }
            }
        }
    }
}

// ------------------------------------------------------------------
// Rename deployed copy if it exists (best-effort, non-fatal)
// ------------------------------------------------------------------

$deployOld = __DIR__ . '/deploy/' . $oldName;
$deployNew = __DIR__ . '/deploy/' . $newName;
if (is_dir($deployOld)) {
    if (!rename($deployOld, $deployNew)) {
        error_log('[rename-page] WARNING: could not rename deploy dir "' . $oldName . '" -> "' . $newName . '"');
    } else {
        error_log('[rename-page] renamed deploy dir "' . $oldName . '" -> "' . $newName . '"');
    }
}

renameOk(['old_name' => $oldName, 'new_name' => $newName]);
