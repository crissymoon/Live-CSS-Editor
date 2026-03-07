<?php
/**
 * pb_admin/logout.php
 * Revokes the session on xcm_auth and destroys the local PHP session.
 */
require_once __DIR__ . '/auth.php';

xcm_logout();
error_log('[pb_admin/logout] session destroyed, redirecting to login');
header('Location: ' . ADMIN_URL_PATH . '/login.php?reason=logout');
exit;
