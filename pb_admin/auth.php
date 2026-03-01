<?php
/**
 * pb_admin/auth.php
 * Session management and xcm_auth API helpers.
 *
 * All API calls log errors to PHP error_log() so they always appear in the
 * server console for debugging without breaking the user-facing flow.
 */

require_once __DIR__ . '/config.php';

// ── Session bootstrap ─────────────────────────────────────────────────────────

if (session_status() === PHP_SESSION_NONE) {
    session_name(ADMIN_SESSION_NAME);
    session_set_cookie_params([
        'lifetime' => 0,
        'path'     => '/',
        'samesite' => 'Lax',
        'httponly' => true,
        'secure'   => false, // set to true when behind HTTPS
    ]);
    session_start();
}

// ── Public helpers ─────────────────────────────────────────────────────────────

/**
 * Returns true if there is a stored access token in the session.
 * Does NOT verify the token with the server on every call.
 */
function is_logged_in(): bool {
    return !empty($_SESSION['access_token']) && !empty($_SESSION['user']);
}

/**
 * Returns the stored SafeUser array or null.
 */
function current_user(): ?array {
    return $_SESSION['user'] ?? null;
}

/**
 * Returns the stored access token string or null.
 */
function access_token(): ?string {
    return $_SESSION['access_token'] ?? null;
}

/**
 * Attempts to log in with the given identifier (username or email) and
 * password by calling POST /auth/login on the xcm_auth server.
 *
 * Returns ['ok' => true, 'user' => [...]] on success.
 * Returns ['ok' => false, 'error' => '...'] on failure.
 */
function xcm_login(string $identifier, string $password): array {
    $resp = xcm_post('/auth/login', [
        'identifier' => $identifier,
        'password'   => $password,
    ]);

    error_log('[pb_admin/auth] xcm_login response: ' . json_encode($resp));

    if (!$resp['ok']) {
        return ['ok' => false, 'error' => $resp['error'] ?? 'Login failed'];
    }

    $data = $resp['data'] ?? [];

    // xcm_auth returns twofa_required=true when 2FA is enabled.
    // In dev mode (TWOFA_ENABLED=false) this is always false.
    if (!empty($data['twofa_required'])) {
        return ['ok' => false, 'error' => '2FA is enabled on the auth server. Set TWOFA_ENABLED=false and restart xcm_auth.'];
    }

    $tokens = $data['tokens'] ?? null;
    $user   = $data['user']   ?? null;

    if (!$tokens || !$user) {
        error_log('[pb_admin/auth] xcm_login: missing tokens or user in response');
        return ['ok' => false, 'error' => 'Unexpected response from auth server'];
    }

    // Store in session
    $_SESSION['access_token']  = $tokens['access_token'];
    $_SESSION['refresh_token'] = $tokens['refresh_token'];
    $_SESSION['token_expires'] = $tokens['expires_at'] ?? 0;
    $_SESSION['user']          = $user;

    error_log('[pb_admin/auth] xcm_login: session set for user ' . ($user['username'] ?? '?'));
    return ['ok' => true, 'user' => $user];
}

/**
 * Logs the current session out by calling POST /auth/logout on xcm_auth,
 * then destroys the local session.
 */
function xcm_logout(): void {
    if (!empty($_SESSION['refresh_token'])) {
        $resp = xcm_post('/auth/logout', [
            'refresh_token' => $_SESSION['refresh_token'],
        ], $_SESSION['access_token'] ?? null);
        error_log('[pb_admin/auth] xcm_logout response: ' . json_encode($resp));
    }
    $_SESSION = [];
    if (isset($_COOKIE[session_name()])) {
        setcookie(session_name(), '', time() - 3600, '/');
    }
    session_destroy();
}

/**
 * Tries to refresh the access token using the stored refresh token.
 * Updates the session on success.  Returns true if refresh succeeded.
 */
function xcm_refresh_token(): bool {
    if (empty($_SESSION['refresh_token'])) {
        return false;
    }
    $resp = xcm_post('/auth/refresh', [
        'refresh_token' => $_SESSION['refresh_token'],
    ]);
    error_log('[pb_admin/auth] xcm_refresh_token response: ' . json_encode($resp));
    if (!$resp['ok']) {
        return false;
    }
    $data = $resp['data'] ?? [];
    if (empty($data['access_token'])) {
        return false;
    }
    $_SESSION['access_token']  = $data['access_token'];
    $_SESSION['refresh_token'] = $data['refresh_token'] ?? $_SESSION['refresh_token'];
    $_SESSION['token_expires'] = $data['expires_at'] ?? 0;
    return true;
}

/**
 * Ensures the stored access token is still fresh.
 * Attempts a silent refresh if it is close to expiry.
 * Redirects to login if the session cannot be recovered.
 */
function require_auth(): void {
    if (!is_logged_in()) {
        $loginUrl = ADMIN_URL_PATH . '/login.php?next=' . urlencode($_SERVER['REQUEST_URI']);
        error_log('[pb_admin/auth] require_auth: no session, redirecting to ' . $loginUrl);
        header('Location: ' . $loginUrl);
        exit;
    }

    // Proactively refresh if the access token expires soon
    $expires = (int)($_SESSION['token_expires'] ?? 0);
    if ($expires > 0 && ($expires - time()) < TOKEN_REFRESH_BEFORE_SECONDS) {
        error_log('[pb_admin/auth] require_auth: token near expiry, attempting refresh');
        if (!xcm_refresh_token()) {
            error_log('[pb_admin/auth] require_auth: refresh failed, forcing logout');
            xcm_logout();
            header('Location: ' . ADMIN_URL_PATH . '/login.php?reason=expired');
            exit;
        }
    }
}

// ── Low-level HTTP helpers ────────────────────────────────────────────────────

/**
 * POST JSON to the xcm_auth server.
 * Returns ['ok' => bool, 'data' => mixed, 'error' => string, 'http_code' => int].
 */
function xcm_post(string $path, array $body, ?string $bearerToken = null): array {
    return xcm_request('POST', $path, $body, $bearerToken);
}

/**
 * GET from the xcm_auth server with optional Bearer token.
 */
function xcm_get(string $path, ?string $bearerToken = null): array {
    return xcm_request('GET', $path, null, $bearerToken);
}

/**
 * Internal HTTP request wrapper using file_get_contents + stream context.
 * All errors are written to error_log for console visibility.
 */
function xcm_request(string $method, string $path, ?array $body, ?string $bearerToken): array {
    $url = rtrim(XCMAUTH_BASE_URL, '/') . $path;

    $headers = ['Content-Type: application/json', 'Accept: application/json'];
    if ($bearerToken) {
        $headers[] = 'Authorization: Bearer ' . $bearerToken;
    }

    $options = [
        'http' => [
            'method'        => $method,
            'header'        => implode("\r\n", $headers),
            'ignore_errors' => true,
            'timeout'       => 8,
        ],
    ];
    if ($body !== null) {
        $options['http']['content'] = json_encode($body);
    }

    $ctx    = stream_context_create($options);
    $start  = microtime(true);
    $raw    = @file_get_contents($url, false, $ctx);
    $ms     = round((microtime(true) - $start) * 1000);

    $httpCode = 0;
    if (isset($http_response_header)) {
        // e.g. "HTTP/1.1 200 OK"
        if (preg_match('#HTTP/\d+\.\d+\s+(\d+)#', $http_response_header[0] ?? '', $m)) {
            $httpCode = (int)$m[1];
        }
    }

    error_log("[pb_admin/auth] xcm_request {$method} {$path} -> HTTP {$httpCode} ({$ms}ms)");

    if ($raw === false) {
        error_log("[pb_admin/auth] xcm_request: file_get_contents failed for {$url}");
        return ['ok' => false, 'error' => 'Could not reach auth server at ' . XCMAUTH_BASE_URL, 'http_code' => 0];
    }

    $json = json_decode($raw, true);
    if ($json === null) {
        error_log("[pb_admin/auth] xcm_request: invalid JSON response: " . substr($raw, 0, 200));
        return ['ok' => false, 'error' => 'Invalid JSON from auth server', 'http_code' => $httpCode];
    }

    // xcm_auth wraps responses as {"ok": true/false, "data": ..., "message": ...}
    $ok = ($json['ok'] ?? false) === true;
    if (!$ok) {
        $errMsg = $json['message'] ?? $json['error'] ?? 'Auth server returned error';
        error_log("[pb_admin/auth] xcm_request: server error: {$errMsg}");
        return ['ok' => false, 'error' => $errMsg, 'http_code' => $httpCode, 'data' => $json['data'] ?? null];
    }

    return ['ok' => true, 'data' => $json['data'] ?? $json, 'http_code' => $httpCode];
}
