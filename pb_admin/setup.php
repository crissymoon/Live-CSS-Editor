<?php
/**
 * pb_admin/setup.php
 * First-time setup page -- creates the initial admin account via xcm_auth.
 *
 * This page calls POST /auth/register to create a user, then runs a direct
 * SQL UPDATE to promote that user to the "admin" role (since registration
 * always creates role="user").
 *
 * Security: delete or move this file after your first account is created.
 * Access is blocked if any users already exist in the database.
 */
require_once __DIR__ . '/config.php';
require_once __DIR__ . '/auth.php';

// ── Block access if a user is already registered ──────────────────────────────
// We check by attempting a health call then check the admin users list.
// If xcm_auth is not running we show an error; if users exist we block.

$serverUp  = false;
$hasUsers  = false;
$blockMsg  = '';

$healthResp = xcm_get('/health');
if ($healthResp['ok']) {
    $serverUp = true;
}

// Only block if we can verify users exist (requires admin token - not available
// at setup time). Instead we show a warning after successful registration.

$error   = '';
$success = '';
$dbPath  = __DIR__ . '/../xcm_auth/xcm_auth_dev.db';

// ── Handle POST ───────────────────────────────────────────────────────────────
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $username = trim($_POST['username'] ?? '');
    $email    = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';
    $confirm  = $_POST['confirm']  ?? '';

    if ($username === '' || $email === '' || $password === '') {
        $error = 'All fields are required.';
    } elseif ($password !== $confirm) {
        $error = 'Passwords do not match.';
    } elseif (!$serverUp) {
        $error = 'xcm_auth server is not running. Start it with start-auth.sh first.';
    } else {
        // Step 1: Register via xcm_auth API
        $resp = xcm_post('/auth/register', [
            'username' => $username,
            'email'    => $email,
            'password' => $password,
        ]);
        error_log('[pb_admin/setup] register response: ' . json_encode($resp));

        if (!$resp['ok']) {
            $error = $resp['error'] ?? 'Registration failed.';
        } else {
            $userId = $resp['data']['id'] ?? null;

            // Step 2: Promote to admin role directly in SQLite
            // xcm_auth always registers as role="user"; we patch it here.
            $promoted = false;
            $promoteErr = '';

            if ($userId && file_exists($dbPath) && class_exists('SQLite3')) {
                try {
                    $db     = new SQLite3($dbPath);
                    $stmt   = $db->prepare('UPDATE users SET role = "admin" WHERE id = :id');
                    $stmt->bindValue(':id', (int)$userId, SQLITE3_INTEGER);
                    $result = $stmt->execute();
                    if ($result !== false && $db->changes() > 0) {
                        $promoted = true;
                        error_log('[pb_admin/setup] promoted user ' . $userId . ' to admin');
                    } else {
                        $promoteErr = 'Row updated: ' . $db->changes() . ' (expected 1)';
                        error_log('[pb_admin/setup] promote failed: ' . $promoteErr);
                    }
                    $db->close();
                } catch (Exception $e) {
                    $promoteErr = $e->getMessage();
                    error_log('[pb_admin/setup] SQLite3 promote exception: ' . $promoteErr);
                }
            } elseif ($userId) {
                // SQLite3 extension not available or DB not found - show manual SQL
                $promoteErr = 'Could not auto-promote. Run the SQL shown below manually.';
                error_log('[pb_admin/setup] SQLite3 not available or DB missing at ' . $dbPath);
            }

            if ($promoted) {
                $success = 'Account created and promoted to admin. You can now log in.';
            } elseif ($promoteErr) {
                $success = 'Account created as role "user". Promotion to admin failed: ' . htmlspecialchars($promoteErr)
                    . ' -- Run this SQL manually in xcm_auth_dev.db: UPDATE users SET role = \'admin\' WHERE id = ' . (int)$userId . ';';
            } else {
                $success = 'Account created. Promotion to admin skipped (no user ID returned).';
            }

            // Write dev-credentials.json so the launcher can display the password.
            // This file is gitignored and is for local dev use only.
            $credFile = __DIR__ . '/../xcm_auth/dev-credentials.json';
            $credData = json_encode([
                'username' => $username,
                'email'    => $email,
                'password' => $password,
                'role'     => $promoted ? 'admin' : 'user',
                'note'     => 'dev only -- do not commit',
            ], JSON_PRETTY_PRINT);
            file_put_contents($credFile, $credData, LOCK_EX);
        }
    }
}
?><!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?= APP_NAME ?> -- Setup</title>
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

        .wrap {
            width: 100%;
            max-width: 420px;
            padding: 28px;
        }

        .logo {
            font-size: 12px;
            letter-spacing: 0.14em;
            text-transform: uppercase;
            color: var(--c-text-faint);
            margin-bottom: 6px;
        }

        .logo span { color: var(--c-acc); }

        .subtitle {
            font-size: 10px;
            color: var(--c-text-ghost);
            letter-spacing: 0.08em;
            margin-bottom: 28px;
        }

        .card {
            background: var(--c-bg-1);
            border: 1px solid var(--c-border-acc2);
            padding: 24px;
        }

        .card-title {
            font-size: 13px;
            font-weight: 600;
            color: var(--c-text-1);
            letter-spacing: 0.06em;
            margin-bottom: 6px;
        }

        .card-desc {
            font-size: 10px;
            color: var(--c-ctrl-btn);
            letter-spacing: 0.04em;
            margin-bottom: 22px;
            line-height: 1.6;
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

        .btn {
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
            transition: background 0.15s;
        }

        .btn:hover { background: var(--c-acc-bg2); }
        .btn:disabled { opacity: 0.4; cursor: not-allowed; }

        .alert {
            font-size: 11px;
            padding: 10px 12px;
            margin-bottom: 16px;
            border: 1px solid;
            line-height: 1.6;
            word-break: break-word;
        }

        .alert-error   { color: var(--c-err);  background: var(--c-err-bg);  border-color: var(--c-err-border); }
        .alert-success { color: var(--c-ok);   background: var(--c-ok-bg);   border-color: var(--c-ok-border); }
        .alert-warn    { color: var(--c-warn); background: var(--c-warn-bg); border-color: var(--c-warn-border); }

        .server-row {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 10px;
            color: var(--c-ctrl-btn);
            margin-top: 18px;
        }

        .dot {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            flex-shrink: 0;
            background: <?= $serverUp ? 'var(--c-ok)' : 'var(--c-err)' ?>;
        }

        .login-link {
            display: block;
            text-align: center;
            margin-top: 16px;
            font-size: 11px;
            color: var(--c-text-faint);
            text-decoration: none;
            letter-spacing: 0.04em;
        }

        .login-link:hover { color: var(--c-text-dim); }

        .schema-block {
            background: var(--c-bg-3);
            border: 1px solid var(--c-border);
            padding: 10px 12px;
            font-size: 10px;
            color: var(--c-text-dim);
            margin-top: 10px;
            white-space: pre;
            overflow-x: auto;
        }
    </style>
</head>
<body>

<div class="wrap">
    <button class="theme-toggle theme-toggle-float" id="themeToggle" title="toggle light/dark mode">light</button>
    <div class="logo"><span>&gt;</span> <?= APP_NAME ?></div>
    <div class="subtitle">first-time setup -- create admin account</div>

    <div class="card">
        <div class="card-title">create admin user</div>
        <div class="card-desc">
            This creates the first account in xcm_auth and promotes it to admin role.
            Delete this file after setup is complete.
        </div>

        <?php if (!$serverUp): ?>
            <div class="alert alert-error">
                xcm_auth server is not responding at <?= htmlspecialchars(XCMAUTH_BASE_URL) ?>.
                Run <code>bash pb_admin/start-auth.sh</code> first.
            </div>
        <?php endif; ?>

        <?php if ($error): ?>
            <div class="alert alert-error"><?= htmlspecialchars($error) ?></div>
        <?php endif; ?>

        <?php if ($success): ?>
            <div class="alert alert-success"><?= $success ?></div>
            <a href="<?= ADMIN_URL_PATH ?>/login.php" class="login-link">go to login</a>

            <div class="alert alert-warn" style="margin-top:16px;">
                Security reminder: delete or restrict access to setup.php now that your account is created.
            </div>
        <?php else: ?>
        <form method="POST" autocomplete="off">
            <div class="field">
                <label for="username">username</label>
                <input type="text" id="username" name="username"
                    value="<?= htmlspecialchars($_POST['username'] ?? '') ?>"
                    autocomplete="off" autofocus>
            </div>
            <div class="field">
                <label for="email">email</label>
                <input type="email" id="email" name="email"
                    value="<?= htmlspecialchars($_POST['email'] ?? '') ?>"
                    autocomplete="off">
            </div>
            <div class="field">
                <label for="password">password</label>
                <input type="password" id="password" name="password" autocomplete="new-password">
            </div>
            <div class="field">
                <label for="confirm">confirm password</label>
                <input type="password" id="confirm" name="confirm" autocomplete="new-password">
            </div>
            <button type="submit" class="btn" <?= !$serverUp ? 'disabled' : '' ?>>create account</button>
        </form>

        <div style="margin-top:20px;font-size:10px;color:#3a3a58;">
            <div style="margin-bottom:6px;color:#444470;">manual sql (if needed)</div>
            <div class="schema-block">-- xcm_auth_dev.db schema reference
-- The users table (relevant columns):

CREATE TABLE users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    email         TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,  -- bcrypt, cost 10+
    role          TEXT NOT NULL DEFAULT 'user',
    is_active     INTEGER NOT NULL DEFAULT 1,
    is_verified   INTEGER NOT NULL DEFAULT 0
);

-- To promote an existing user to admin:
UPDATE users SET role = 'admin' WHERE username = 'your-username';

-- To check current users:
SELECT id, username, email, role, is_active FROM users;</div>
        </div>
        <?php endif; ?>

        <div class="server-row">
            <span class="dot"></span>
            <span>xcm_auth: <?= $serverUp ? 'online' : 'offline' ?> at <?= htmlspecialchars(XCMAUTH_BASE_URL) ?></span>
        </div>
    </div>

    <?php if (!$success): ?>
        <a href="<?= ADMIN_URL_PATH ?>/login.php" class="login-link">back to login</a>
    <?php endif; ?>
</div>

</body>
</html>
<script>
(function() {
    var toggleBtn = document.getElementById('themeToggle');
    if (!toggleBtn) { console.warn('[pb_admin/setup] themeToggle not found'); return; }

    function getTheme() {
        try { return localStorage.getItem('pb_admin_theme') || 'dark'; }
        catch(e) { console.error('[pb_admin/setup] localStorage read error:', e); return 'dark'; }
    }

    function applyTheme(t) {
        document.documentElement.setAttribute('data-theme', t);
        toggleBtn.textContent = (t === 'dark') ? 'light' : 'dark';
        toggleBtn.title = 'switch to ' + ((t === 'dark') ? 'light' : 'dark') + ' mode';
    }

    function saveTheme(t) {
        try { localStorage.setItem('pb_admin_theme', t); }
        catch(e) { console.error('[pb_admin/setup] localStorage write error:', e); }
    }

    toggleBtn.addEventListener('click', function() {
        var next = getTheme() === 'dark' ? 'light' : 'dark';
        console.log('[pb_admin/setup] theme toggle:', next);
        applyTheme(next);
        saveTheme(next);
    });

    applyTheme(getTheme());
})();
</script>
