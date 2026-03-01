<?php
/**
 * pb_admin/config.php
 * All configuration for the admin panel and its connection to xcm_auth.
 * Edit XCMAUTH_BASE_URL to match wherever you start the Go auth server.
 */

// Base URL of the running xcm_auth Go server.
// Start it with: cd xcm_auth && TWOFA_ENABLED=false SERVER_ADDR=:9100 go run ./cmd/main.go
define('XCMAUTH_BASE_URL', getenv('XCMAUTH_BASE_URL') ?: 'http://localhost:9100');

// Cookie/session name for storing the admin session.
define('ADMIN_SESSION_NAME', 'pb_admin_sess');

// How long (seconds) a stored access token is considered usable before we
// try to refresh it.  Access tokens expire at 15 min by default.
define('TOKEN_REFRESH_BEFORE_SECONDS', 120);

// Application display name shown in the nav.
define('APP_NAME', 'PB Admin');

// Dev mode flag - set to true to show extra debug info in the UI.
define('DEV_MODE', true);

// Absolute web path to the pb_admin directory.
// Used for all Location: redirects so they are never ambiguous relative paths.
// Change this if you move pb_admin to a different URL prefix.
define('ADMIN_URL_PATH', '/pb_admin');
