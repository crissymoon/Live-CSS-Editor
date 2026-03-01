<?php
/**
 * pb_admin/index.php
 * Entry point - redirects to dashboard if logged in, otherwise to login.
 */
require_once __DIR__ . '/auth.php';

error_log('[pb_admin/index] is_logged_in=' . (is_logged_in() ? 'true' : 'false'));

if (is_logged_in()) {
    error_log('[pb_admin/index] redirecting to dashboard');
    header('Location: ' . ADMIN_URL_PATH . '/dashboard.php');
} else {
    error_log('[pb_admin/index] redirecting to login');
    header('Location: ' . ADMIN_URL_PATH . '/login.php');
}
exit;
