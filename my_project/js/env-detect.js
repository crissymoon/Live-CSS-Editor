/*
 * Crissy's Style Tool -- environment / path detection helper.
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License -- see LICENSE file for full text.
 *
 * Loaded as a classic (non-module) script, before all other JS.
 * Sets window.LiveCSS.env so every other module can use correct paths
 * regardless of whether the project is served by:
 *   - PHP built-in server at port 8080  (root = my_project/)
 *     URL example: http://localhost:8080/index.php
 *   - Node WASM server at port 8080     (root = php-wasm-project/, my_project symlinked)
 *     URL example: http://localhost:8080/my_project/index.php
 *   - nginx/Apache                      (any configured root)
 */
(function () {
    'use strict';

    window.LiveCSS = window.LiveCSS || {};

    // Detect path prefix from the URL of the current page.
    // Strip the filename so we get the directory where index.php lives.
    var pathname = window.location.pathname;                // e.g. /my_project/index.php
    var dir      = pathname.substring(0, pathname.lastIndexOf('/') + 1); // e.g. /my_project/

    // Normalise: always ends with '/', never double '//'
    var basePath = dir.replace(/\/+$/, '') || '';           // e.g. /my_project  or  (empty)

    window.LiveCSS.env = {
        // basePath: the prefix before any project-relative path.
        // e.g. '' when at /index.php, or '/my_project' when at /my_project/index.php
        basePath: basePath,

        // Resolve a project-root-relative path to an absolute URL path.
        // Usage: LiveCSS.env.resolve('/vscode-bridge/api/projects.php')
        //        -> '/my_project/vscode-bridge/api/projects.php'  (WASM build)
        //        -> '/vscode-bridge/api/projects.php'             (PHP built-in)
        resolve: function (projectPath) {
            // projectPath must start with '/'
            return basePath + projectPath;
        },

        // True when running under the WASM node server (project is in a subdirectory)
        isSubdir: basePath !== ''
    };

    console.log('[env-detect] basePath="' + basePath + '"  resolve test: ' +
                window.LiveCSS.env.resolve('/vscode-bridge/api/projects.php'));
}());
