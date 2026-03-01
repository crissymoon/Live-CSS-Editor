<?php
/**
 * pb_admin/login.php
 * Login form - authenticates via the xcm_auth Go API.
 * Dev mode: TWOFA_ENABLED=false removes the 2FA step entirely.
 */
require_once __DIR__ . '/auth.php';

// If already logged in, go straight to dashboard
if (is_logged_in()) {
    error_log('[pb_admin/login] already logged in, redirecting to ' . ADMIN_URL_PATH . '/dashboard.php');
    header('Location: ' . ADMIN_URL_PATH . '/dashboard.php');
    exit;
}

$error  = '';
$notice = '';

// Query-string notices
if (isset($_GET['reason'])) {
    $map = [
        'expired' => 'Your session expired. Please log in again.',
        'logout'  => 'You have been logged out.',
    ];
    $notice = $map[$_GET['reason']] ?? '';
}

// Handle POST login
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $identifier = trim($_POST['identifier'] ?? '');
    $password   = $_POST['password'] ?? '';

    if ($identifier === '' || $password === '') {
        $error = 'Username/email and password are required.';
    } else {
        $result = xcm_login($identifier, $password);
        if ($result['ok']) {
            // $next must be a path inside pb_admin; anything else falls back to dashboard.
            // We only allow paths that start with ADMIN_URL_PATH or are bare filenames.
            $rawNext = $_GET['next'] ?? '';
            $defaultDash = ADMIN_URL_PATH . '/dashboard.php';
            if ($rawNext === '') {
                $next = $defaultDash;
            } elseif (preg_match('/^[a-zA-Z0-9_\-]+\.php$/', $rawNext)) {
                // bare filename like "dashboard.php" -- scope it to admin dir
                $next = ADMIN_URL_PATH . '/' . $rawNext;
            } elseif (strpos($rawNext, ADMIN_URL_PATH . '/') === 0 &&
                      preg_match('/^[a-zA-Z0-9\/_\-\.]+\.php([?].*)?$/', $rawNext)) {
                // absolute path already scoped under ADMIN_URL_PATH
                $next = $rawNext;
            } else {
                error_log('[pb_admin/login] rejected unsafe next param: ' . $rawNext . ' -- falling back to dashboard');
                $next = $defaultDash;
            }
            error_log('[pb_admin/login] login ok for ' . ($result['user']['username'] ?? '?') . ' -> redirect to ' . $next);
            header('Location: ' . $next);
            exit;
        } else {
            $error = $result['error'] ?? 'Login failed.';
            error_log('[pb_admin/login] login failed: ' . $error);
        }
    }
}
?><!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?= APP_NAME ?> — Login</title>
    <!-- No-flash theme init: read preference before first paint -->
    <script>
    (function() {
        try {
            var t = localStorage.getItem('pb_admin_theme');
            if (t === 'light' || t === 'dark') {
                document.documentElement.setAttribute('data-theme', t);
            }
        } catch(e) {
            console.warn('[pb_admin] theme init error:', e);
        }
    })();
    </script>
    <link rel="stylesheet" href="<?= ADMIN_URL_PATH ?>/admin.css">
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--c-bg);
            color: var(--c-text);
            font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .login-wrap {
            width: 100%;
            max-width: 360px;
            padding: 28px;
        }

        .login-logo {
            font-size: 12px;
            letter-spacing: 0.14em;
            text-transform: uppercase;
            color: var(--c-text-faint);
            margin-bottom: 28px;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .login-logo span {
            color: var(--c-acc);
            font-size: 16px;
        }

        .login-card {
            background: var(--c-bg-1);
            border: 1px solid var(--c-border-acc2);
            padding: 24px;
        }

        .login-title {
            font-size: 13px;
            font-weight: 600;
            color: var(--c-text-1);
            letter-spacing: 0.06em;
            margin-bottom: 22px;
        }

        .field {
            display: flex;
            flex-direction: column;
            gap: 6px;
            margin-bottom: 14px;
        }

        .field label {
            font-size: 10px;
            letter-spacing: 0.12em;
            text-transform: uppercase;
            color: var(--c-text-dim);
        }

        .field input {
            background: var(--c-bg-3);
            border: 1px solid var(--c-border);
            color: var(--c-text);
            font-family: inherit;
            font-size: 13px;
            padding: 9px 10px;
            outline: none;
            width: 100%;
            transition: border-color 0.15s;
        }

        .field input:focus {
            border-color: var(--c-acc-border);
        }

        .field input::placeholder {
            color: var(--c-text-ghost);
        }

        .btn-submit {
            width: 100%;
            background: var(--c-acc-bg);
            border: 1px solid var(--c-acc-border);
            color: var(--c-text-1);
            font-family: inherit;
            font-size: 12px;
            letter-spacing: 0.08em;
            padding: 10px;
            cursor: pointer;
            margin-top: 8px;
            transition: background 0.15s, color 0.15s;
        }

        .btn-submit:hover {
            background: var(--c-acc-bg2);
            color: var(--c-text);
        }

        .btn-submit:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .alert {
            font-size: 11px;
            padding: 8px 10px;
            margin-bottom: 16px;
            border: 1px solid;
        }

        .alert-error {
            color: var(--c-err);
            background: var(--c-err-bg);
            border-color: var(--c-err-border);
        }

        .alert-notice {
            color: var(--c-text-dim);
            background: var(--c-acc-bg3);
            border-color: var(--c-border-acc2);
        }

        .dev-badge {
            display: inline-block;
            font-size: 9px;
            letter-spacing: 0.1em;
            text-transform: uppercase;
            color: var(--c-dev-badge);
            background: var(--c-dev-badge-bg);
            border: 1px solid var(--c-dev-badge-border);
            padding: 2px 6px;
            margin-left: 6px;
            vertical-align: middle;
        }

        .server-status {
            margin-top: 20px;
            font-size: 10px;
            display: flex;
            align-items: center;
            gap: 6px;
        }

        .status-dot {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background: var(--c-ctrl-btn);
            flex-shrink: 0;
        }

        .status-dot.online  { background: var(--c-ok); }
        .status-dot.offline { background: var(--c-err); }

        .server-status-text { color: var(--c-text-faint); }
    </style>
</head>
<body>

<div class="login-wrap">
    <button class="theme-toggle theme-toggle-float" id="themeToggle" title="toggle light/dark mode">light</button>
    <div class="login-logo">
        <span>&gt;</span> <?= htmlspecialchars(APP_NAME) ?>
        <?php if (DEV_MODE): ?>
            <span class="dev-badge">dev</span>
        <?php endif; ?>
    </div>

    <div class="login-card">
        <div class="login-title">sign in</div>

        <?php if ($error): ?>
            <div class="alert alert-error"><?= htmlspecialchars($error) ?></div>
        <?php endif; ?>

        <?php if ($notice): ?>
            <div class="alert alert-notice"><?= htmlspecialchars($notice) ?></div>
        <?php endif; ?>

        <form method="POST" id="loginForm">
            <div class="field">
                <label for="identifier">username or email</label>
                <input
                    type="text"
                    id="identifier"
                    name="identifier"
                    autocomplete="username"
                    placeholder="admin"
                    value="<?= htmlspecialchars($_POST['identifier'] ?? '') ?>"
                    autofocus
                >
            </div>

            <div class="field">
                <label for="password">password</label>
                <input
                    type="password"
                    id="password"
                    name="password"
                    autocomplete="current-password"
                    placeholder="..."
                >
            </div>

            <button type="submit" class="btn-submit" id="submitBtn">
                login
            </button>
        </form>

        <div style="margin-top:16px;padding-top:14px;border-top:1px solid var(--c-border-2);font-size:10px;color:var(--c-text-ghost);text-align:center;">
            no account yet? <a href="<?= ADMIN_URL_PATH ?>/setup.php" style="color:var(--c-text-faint);text-decoration:none;letter-spacing:0.04em;">first-time setup</a>
        </div>

        <div class="server-status" id="serverStatus">
            <div class="status-dot" id="statusDot"></div>
            <span class="server-status-text" id="statusText">checking auth server...</span>
        </div>
    </div>
</div>

<script>
(function() {
    // Theme toggle
    var toggleBtn = document.getElementById('themeToggle');
    if (!toggleBtn) { console.warn('[pb_admin/login] themeToggle not found'); }

    function getTheme() {
        try { return localStorage.getItem('pb_admin_theme') || 'dark'; }
        catch(e) { console.error('[pb_admin/login] localStorage read error:', e); return 'dark'; }
    }

    function applyTheme(t) {
        document.documentElement.setAttribute('data-theme', t);
        if (toggleBtn) {
            toggleBtn.textContent = (t === 'dark') ? 'light' : 'dark';
            toggleBtn.title = 'switch to ' + ((t === 'dark') ? 'light' : 'dark') + ' mode';
        }
    }

    function saveTheme(t) {
        try { localStorage.setItem('pb_admin_theme', t); }
        catch(e) { console.error('[pb_admin/login] localStorage write error:', e); }
    }

    if (toggleBtn) {
        toggleBtn.addEventListener('click', function() {
            var next = getTheme() === 'dark' ? 'light' : 'dark';
            console.log('[pb_admin/login] theme toggle:', next);
            applyTheme(next);
            saveTheme(next);
        });
    }

    applyTheme(getTheme());

    // Disable submit until we know the server is reachable
    var btn      = document.getElementById('submitBtn');
    var dot      = document.getElementById('statusDot');
    var statusTx = document.getElementById('statusText');

    function checkServer() {
        fetch('api_proxy.php?action=health')
            .then(function(r) { return r.json(); })
            .then(function(d) {
                if (d && d.ok) {
                    dot.className      = 'status-dot online';
                    statusTx.textContent = 'auth server: online (' + (d.server || '<?= XCMAUTH_BASE_URL ?>') + ')';
                    btn.disabled = false;
                } else {
                    dot.className      = 'status-dot offline';
                    statusTx.textContent = 'auth server: offline - start xcm_auth first';
                    btn.disabled = true;
                }
            })
            .catch(function(err) {
                console.error('[pb_admin] server health check error:', err);
                dot.className      = 'status-dot offline';
                statusTx.textContent = 'auth server: unreachable';
                btn.disabled = true;
            });
    }

    btn.disabled = true;
    checkServer();
    setInterval(checkServer, 10000);

    // Prevent double-submit
    document.getElementById('loginForm').addEventListener('submit', function() {
        btn.disabled  = true;
        btn.textContent = 'signing in...';
    });
})();
</script>

</body>
</html>
