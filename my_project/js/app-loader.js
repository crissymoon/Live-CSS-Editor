/* app-loader.js
 * Injects a full-page loading overlay immediately when parsed.
 * Call LiveCSS.appLoader.dismiss() once the app has fully booted.
 * Reusable: drop the <script> tag in any page that uses cdn-loader.
 */
(function () {
    'use strict';

    // Namespace setup
    window.LiveCSS = window.LiveCSS || {};

    // Build overlay element immediately -- no DOMContentLoaded wait needed
    // because we append to documentElement which exists at <head> parse time.
    var overlay = document.createElement('div');
    overlay.className = 'lc-loader';
    overlay.id = 'lcLoadOverlay';
    overlay.innerHTML =
        '<div class="lc-loader__ring"></div>' +
        '<div class="lc-loader__label">Loading</div>';

    // Append to <html> so it is visible before <body> exists
    document.documentElement.appendChild(overlay);

    // Move to <body> once it exists (cleaner DOM order, same visual result)
    function moveToBody() {
        if (document.body && overlay.parentNode !== document.body) {
            document.body.appendChild(overlay);
        }
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', moveToBody);
    } else {
        moveToBody();
    }

    // Dismiss: fade out then remove from DOM
    function dismiss() {
        moveToBody();
        overlay.classList.add('lc-loader--done');
        // Remove after transition completes (matches transition duration in CSS)
        setTimeout(function () {
            if (overlay.parentNode) {
                overlay.parentNode.removeChild(overlay);
            }
        }, 400);
    }

    window.LiveCSS.appLoader = { dismiss: dismiss };

}());
