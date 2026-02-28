<?php
/**
 * Stylesheet directory scanner.
 *
 * Scans the style-sheets/ directory relative to the project root and returns
 * every .css file found.  New stylesheets placed in that directory are picked
 * up automatically -- no manual registration needed.
 *
 * When requested with ?json, returns a JSON array suitable for JavaScript
 * consumers such as the theme randomizer.  Otherwise, expose the PHP function
 * for use by other PHP scripts via require_once.
 */

declare(strict_types=1);

/**
 * Return an array of descriptor objects for every CSS file in the given
 * directory.  Sorted alphabetically by filename.
 *
 * Each item has:
 *   file   -- plain filename, e.g. "dark-neu.css"
 *   name   -- slug without extension, e.g. "dark-neu"
 *   label  -- human-readable label, e.g. "Dark Neu"
 *   path   -- root-relative path for use in <link href="...">, e.g. "style-sheets/dark-neu.css"
 */
function getAvailableStylesheets(string $dir = ''): array
{
    if ($dir === '') {
        $dir = dirname(__DIR__) . '/style-sheets';
    }

    if (!is_dir($dir)) {
        return [];
    }

    $entries = scandir($dir);
    if ($entries === false) {
        return [];
    }

    $sheets = [];

    foreach ($entries as $entry) {
        if ($entry === '.' || $entry === '..') {
            continue;
        }
        if (!str_ends_with(strtolower($entry), '.css')) {
            continue;
        }

        $slug    = pathinfo($entry, PATHINFO_FILENAME);
        $label   = ucwords(str_replace(['-', '_', '.'], ' ', $slug));

        $sheets[] = [
            'file'  => $entry,
            'name'  => $slug,
            'label' => $label,
            'path'  => 'style-sheets/' . $entry,
        ];
    }

    usort($sheets, static fn($a, $b) => strcmp($a['name'], $b['name']));

    return $sheets;
}

// --------------------------------------------------------------------------
// JSON endpoint -- called by js/theme-randomizer.js
// --------------------------------------------------------------------------

if (
    php_sapi_name() !== 'cli'
    && isset($_GET['json'])
) {
    header('Content-Type: application/json');
    header('Cache-Control: no-store');

    $sheets = getAvailableStylesheets();
    echo json_encode(['stylesheets' => $sheets], JSON_UNESCAPED_UNICODE);
    exit;
}
