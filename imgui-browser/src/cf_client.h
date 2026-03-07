#pragma once
// cf_client.h -- libcurl client for the cf_bridge Python service (port 9925).
//
// cf_bridge runs a hidden Chromium (Qt WebEngine) window that loads target
// URLs and harvests Cloudflare clearance cookies (__cf_clearance, _cfuvid,
// session tokens) by monitoring the Chromium CookieStore.  These cookies
// are meaningless unless the TLS fingerprint of the request that carries
// them also looks like Chrome.  WKWebView uses Apple BoringSSL and produces
// the correct JA3 hash on macOS 13+ when no custom UA is set, so injecting
// the cookies Chromium produced is the correct solution.
//
// Usage (fire-and-forget, background thread):
//     cf_client_solve_async("https://platform.claude.com/settings/billing");
//
// Usage (blocking, for cmd_server /cf-solve):
//     std::string json = cf_client_solve_sync(url, 50);
//     // json is the full {"ok":...,"cookies":[...]} response from cf_bridge.
//
// The _async variant resolves cookies AND injects them into the shared
// WKWebsiteDataStore (via webview_inject_cookies) with no caller effort.

#include <string>

// Resolve CF clearance cookies for url via cf_bridge and inject them into
// the WKWebsiteDataStore.  Spawns a background thread and returns
// immediately.  Safe to call from the ObjC main thread.
void cf_client_solve_async(const std::string& url);

// Blocking version.  Blocks up to timeout_sec (default 50) seconds.
// Returns the raw JSON response body from cf_bridge, or an empty string
// on failure (bridge not running, network error, etc.).
// Also injects the cookies before returning.
std::string cf_client_solve_sync(const std::string& url, int timeout_sec = 50);

// Returns the Chrome User-Agent string that the bridge used to obtain the
// last cf_clearance cookie.  WKWebView must send this exact UA with requests
// that carry the cookie, otherwise CF will reject it.
// Returns empty string if no solve has completed yet.
std::string cf_client_get_last_ua();

// Returns true when cf_bridge is reachable on port 9925.
bool cf_client_bridge_alive();
