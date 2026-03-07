#pragma once
#include <string>

/*
 * virt_overlay -- Qt Chromium overlay for pages that WKWebView cannot render.
 *
 * The overlay is hosted by cf_bridge_runner.py (port 9925) and shown by
 * POSTing JSON to /virt-show, /virt-hide, /virt-move.
 *
 * URL patterns are read from virt-pages.json at startup.
 * Coordinates are in Qt screen points: origin at top-left of primary screen,
 * y increases downward (same as what webview_get_content_screen_rect returns).
 */

// Load URL patterns from a JSON file.  Call once at startup.
void virt_overlay_init(const std::string& json_path);

// Returns true if the URL matches any configured pattern.
bool virt_overlay_check_url(const std::string& url);

// Show (or navigate) the overlay at the given screen rect.
// Blocks briefly (2-second libcurl timeout) if cf_bridge is not up yet.
void virt_overlay_show(const std::string& url, int x, int y, int w, int h);

// Hide the overlay.  No-op if already hidden.
void virt_overlay_hide();

// Update overlay position.  No-op if hidden.  Only POSTs when position changed.
// Call this every frame (cheap when not active or position unchanged).
void virt_overlay_tick(int x, int y, int w, int h);

// True while the overlay is shown or fading in.
bool virt_overlay_is_active();
