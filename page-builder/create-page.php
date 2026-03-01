<?php
/**
 * page-builder/create-page.php
 * POST { "name": "my-page" } -> creates pages/{name}/ with template JSON files.
 */

header('Content-Type: application/json');

$raw = file_get_contents('php://input');
$body = $raw ? json_decode($raw, true) : [];

$name = $body['name'] ?? '';

// Strict validation
if (!$name || !preg_match('/^[a-z0-9][a-z0-9\-_]{0,62}$/', $name)) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Invalid page name']);
    exit;
}

$pageDir = __DIR__ . '/pages/' . $name;

if (is_dir($pageDir)) {
    echo json_encode(['ok' => false, 'error' => 'Page already exists']);
    exit;
}

if (!mkdir($pageDir, 0755, true)) {
    echo json_encode(['ok' => false, 'error' => 'Could not create directory']);
    exit;
}

// ---- Template JSON files ---- //

$header = [
    'type'     => 'header',
    'settings' => [
        'bg'       => '#0d0d18',
        'height'   => '60px',
        'maxWidth' => '1100px',
        'padding'  => '0 24px',
    ],
    'brand'         => ['text' => ucfirst($name), 'href' => '#', 'color' => '#e8e8f0', 'fontSize' => '18px'],
    'nav'           => [['label' => 'Home', 'href' => '#'], ['label' => 'About', 'href' => '#', 'cta' => true]],
    'navSettings'   => ['linkColor' => '#a5a5c0', 'ctaColor' => '#6366f1', 'gap' => '20px', 'fontSize' => '14px'],
];

$section1 = [
    'type'    => 'section',
    'id'      => $name . '-hero',
    'layout'  => 'column',
    'settings' => [
        'bg'       => '#0a0a14',
        'padding'  => '80px 24px',
        'maxWidth' => '800px',
        'align'    => 'center',
        'gap'      => '20px',
    ],
    'blocks' => [
        ['type' => 'heading', 'id' => $name . '-heading', 'text' => ucfirst($name),         'tag' => 'h1', 'settings' => ['color' => '#e8e8f0', 'fontSize' => '48px', 'fontWeight' => '700']],
        ['type' => 'text',    'id' => $name . '-sub',     'text' => 'Your subtitle here.',                  'settings' => ['color' => '#8888a0', 'fontSize' => '18px', 'lineHeight' => '1.6']],
        ['type' => 'button',  'id' => $name . '-cta',     'text' => 'Get Started',           'href' => '#', 'settings' => ['bg' => '#6366f1', 'color' => '#fff', 'padding' => '12px 28px', 'fontSize' => '15px', 'borderRadius' => '4px']],
    ],
];

$footer = [
    'type'     => 'footer',
    'settings' => ['bg' => '#0d0d18', 'borderTop' => '1px solid #1a1a2e', 'padding' => '24px', 'maxWidth' => '1100px'],
    'copy'     => ['id' => $name . '-copy', 'text' => '© ' . date('Y') . ' ' . ucfirst($name), 'settings' => ['color' => '#5555a0', 'fontSize' => '13px']],
    'links'    => [],
    'linkSettings' => ['color' => '#5555a0', 'hoverColor' => '#8888a0', 'gap' => '16px', 'fontSize' => '13px'],
];

file_put_contents($pageDir . '/header.json',    json_encode($header,   JSON_PRETTY_PRINT));
file_put_contents($pageDir . '/section-1.json', json_encode($section1, JSON_PRETTY_PRINT));
file_put_contents($pageDir . '/footer.json',    json_encode($footer,   JSON_PRETTY_PRINT));
file_put_contents($pageDir . '/overrides.json', '{}');

// ---- Create page.json manifest ---- //

$manifest = [
    'title'    => ucfirst($name),
    'sections' => [
        [
            'id'    => 'pb-header',
            'file'  => 'header.json',
            'type'  => 'header',
            'label' => 'Header',
        ],
        [
            'id'    => 'pb-' . $name . '-hero',
            'file'  => 'section-1.json',
            'type'  => 'section',
            'label' => ucfirst($name) . ' Hero',
        ],
        [
            'id'    => 'pb-footer',
            'file'  => 'footer.json',
            'type'  => 'footer',
            'label' => 'Footer',
        ],
    ],
];

$manifestWritten = file_put_contents($pageDir . '/page.json', json_encode($manifest, JSON_PRETTY_PRINT));
if ($manifestWritten === false) {
    error_log('[create-page] WARNING: Could not write page.json for: ' . $name);
}

error_log('[create-page] Created page: ' . $name . ' in ' . $pageDir);
echo json_encode(['ok' => true, 'name' => $name]);
