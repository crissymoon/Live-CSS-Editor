<?php
// app/index.php -- front controller / router.
// All requests from the JS layer hit this file via wasm_exec().
// Routes are matched on REQUEST_METHOD + PATH_INFO.

declare(strict_types=1);

require_once __DIR__ . '/db/db.php';

header('Content-Type: application/json; charset=utf-8');

$method = strtoupper($_SERVER['REQUEST_METHOD'] ?? 'GET');
$path   = $_SERVER['PATH_INFO'] ?? '/';

// Strip trailing slash for consistency, but keep bare "/"
if ($path !== '/' && str_ends_with($path, '/')) {
    $path = rtrim($path, '/');
}

// Simple prefix router
$routes = [
    'GET'  => [
        '/'                    => 'handle_index',
        '/api/stylesheets'     => 'api_stylesheets_list',
        '/api/stylesheets/get' => 'api_stylesheets_get',
    ],
    'POST' => [
        '/api/stylesheets/save'   => 'api_stylesheets_save',
        '/api/stylesheets/delete' => 'api_stylesheets_delete',
        '/api/export'             => 'api_export',
    ],
];

$handler = $routes[$method][$path] ?? null;

if ($handler === null) {
    http_response_code(404);
    echo json_encode(['error' => 'Not found', 'path' => $path, 'method' => $method]);
    exit;
}

$handler();

// ── Route handlers ────────────────────────────────────────────────────────────

function handle_index(): void {
    echo json_encode(['status' => 'ok', 'runtime' => 'PHP/' . PHP_VERSION . ' WASM']);
}

function api_stylesheets_list(): void {
    $db   = db_open();
    $rows = $db->query('SELECT id, name, updated_at FROM stylesheets ORDER BY name ASC')
               ->fetchAll(PDO::FETCH_ASSOC);
    echo json_encode(['stylesheets' => $rows]);
}

function api_stylesheets_get(): void {
    $name = $_GET['name'] ?? '';
    if ($name === '') {
        http_response_code(400);
        echo json_encode(['error' => 'name param required']);
        return;
    }
    $db   = db_open();
    $stmt = $db->prepare('SELECT id, name, css, updated_at FROM stylesheets WHERE name = ?');
    $stmt->execute([$name]);
    $row  = $stmt->fetch(PDO::FETCH_ASSOC);
    if (!$row) {
        http_response_code(404);
        echo json_encode(['error' => 'Stylesheet not found']);
        return;
    }
    echo json_encode($row);
}

function api_stylesheets_save(): void {
    $body = json_decode(file_get_contents('php://input'), true);
    $name = trim($body['name'] ?? '');
    $css  = $body['css'] ?? '';

    if ($name === '') {
        http_response_code(400);
        echo json_encode(['error' => 'name is required']);
        return;
    }

    $db   = db_open();
    $stmt = $db->prepare(
        'INSERT INTO stylesheets (name, css, updated_at)
         VALUES (?, ?, datetime("now"))
         ON CONFLICT(name) DO UPDATE SET css = excluded.css, updated_at = excluded.updated_at'
    );
    $stmt->execute([$name, $css]);

    echo json_encode(['saved' => true, 'name' => $name]);
}

function api_stylesheets_delete(): void {
    $body = json_decode(file_get_contents('php://input'), true);
    $name = trim($body['name'] ?? '');

    if ($name === '') {
        http_response_code(400);
        echo json_encode(['error' => 'name is required']);
        return;
    }

    $db   = db_open();
    $stmt = $db->prepare('DELETE FROM stylesheets WHERE name = ?');
    $stmt->execute([$name]);

    echo json_encode(['deleted' => true, 'name' => $name]);
}

function api_export(): void {
    $body     = json_decode(file_get_contents('php://input'), true);
    $name     = trim($body['name'] ?? '');
    $filename = trim($body['filename'] ?? $name);

    if ($name === '') {
        http_response_code(400);
        echo json_encode(['error' => 'name is required']);
        return;
    }

    $db   = db_open();
    $stmt = $db->prepare('SELECT css FROM stylesheets WHERE name = ?');
    $stmt->execute([$name]);
    $row  = $stmt->fetch(PDO::FETCH_ASSOC);

    if (!$row) {
        http_response_code(404);
        echo json_encode(['error' => 'Stylesheet not found']);
        return;
    }

    // The JS bridge watches for "export" in the response headers and
    // forwards the content to the C++ native layer for disk write.
    header('X-Export-File: ' . $filename . '.css');
    echo json_encode([
        'export'   => true,
        'filename' => $filename . '.css',
        'content'  => $row['css'],
    ]);
}
