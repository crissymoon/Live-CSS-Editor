#pragma once
// webview.h -- WKWebView lifecycle + bridge (macOS only)
// All Obj-C types are hidden behind void* so this header is pure C++ and
// can be included from any .cpp without pulling in Cocoa headers.

#include "app_state.h"
#include <string>
#include <vector>
#include <functional>

// Callbacks the app can register
struct WebViewCallbacks {
    std::function<void(int tab_id, const std::string& url)>   on_url_change;
    std::function<void(int tab_id, const std::string& title)> on_title_change;
    std::function<void(int tab_id, float progress)>           on_progress;
    std::function<void(int tab_id, bool loading)>             on_loading;
    std::function<void(int tab_id, bool back, bool fwd)>      on_nav_state;
    std::function<void(const std::string& url)>               on_hover_url;
    std::function<void(double fps)>                           on_wkwv_fps;
    std::function<void(int tab_id, const std::string& url)>   on_favicon_change;
    // Called (on main thread) when a navigation matches a virt-pages pattern.
    // The delegate has already cancelled the navigation -- caller should spawn
    // the secure popup and show the badge.
    // Additional JS strings injected at document-start into every frame
    // (main tab and all popups). Injected after JS_INIT and JS_MASK_WEBVIEW
    // in the order they appear in this vector.
    // Use this for xcm performance scripts (input-watcher, chrome-gl-compositor)
    // and any app-specific bootstrap code.
    std::vector<std::string> extra_scripts;

    // Like extra_scripts but injected with forMainFrameOnly:NO -- runs in
    // every sub-frame including third-party payment/challenge iframes.
    // Use only for shims that have their own host-gate guards.
    std::vector<std::string> extra_scripts_all_frames;
};

// Initialise the WebView subsystem.  Must be called from the main thread
// AFTER the GLFW window is created.  Passes the NSWindow handle.
void webview_init(void* ns_window, AppState* state, WebViewCallbacks cbs);

// Compile and apply ad blocking content rules to every WKWebView tab.
// Pass the full JSON string (WKContentRuleList format).
// Asynchronous: compilation runs on a background thread; existing and future
// tabs receive the rule list once compilation completes.
void webview_load_adblock(const std::string& rules_json);

// Create a WKWebView for a tab and add it as a subview.
// Returns the opaque handle stored in Tab::wv_handle.
void* webview_create(int tab_id, const std::string& url);

// Destroy and remove a WKWebView.
void  webview_destroy(void* handle);

// Show/hide a view (switching tabs).
void  webview_show(void* handle);
void  webview_hide(void* handle);

// Position + size the content area (below chrome, above status bar).
// Call whenever window resizes or chrome height changes.
void  webview_resize(void* handle, int x, int y, int w, int h);

// Navigation
void  webview_load_url(void* handle, const std::string& url);
void  webview_go_back(void* handle);
void  webview_go_forward(void* handle);
void  webview_reload(void* handle);
void  webview_stop(void* handle);

// Run JS and return the result as a string via callback (main thread only).
void  webview_eval_js(void* handle,
                      const std::string& script,
                      std::function<void(const std::string& result)> cb);

// Enable or disable JavaScript for a tab. Takes effect on the next
// navigation (a reload is triggered automatically so the change is immediate).
// Security note: disabling JS prevents page scripts from running; WKUserScript
// injections (e.g. Chrome masking) are unaffected as they run in the host world.
void  webview_set_js_enabled(void* handle, bool enabled);

// Open the WKWebView Web Inspector (requires developerExtrasEnabled = YES).
// Equivalent to right-click -> Inspect Element, opened programmatically.
void  webview_open_inspector(void* handle);

// Send a clipboard action to the page content.
// action: "copy" | "cut" | "paste" | "selectAll"
// Targets the WKWebView directly so it works regardless of which NSWindow
// is currently key.  Must be called from the main thread.
void  webview_clipboard_action(void* handle, const char* action);

// Clear only the HTTP disk/memory cache. Preserves cookies and auth state.
// Use this to flush stale 301 redirects or cached responses.
void  webview_clear_cache();

// Clear all stored website data (cookies, localStorage, IndexedDB, cache,
// service worker registrations). Use this to flush a stuck auth state.
void  webview_clear_data();

// Open a URL in the user's default system browser (real Safari, Chrome, etc.).
// Use this for Google OAuth and any site that blocks embedded-WebView login
// flows by policy. The auth happens in the system browser where there are no
// TLS fingerprint or embedded-WebView detection issues. See webview.mm for
// full details on why this is needed.
void  webview_open_in_system_browser(const std::string& url);

// Open a URL in a native floating WKWebView panel (no controls) -- the same
// mechanism as OAuth popups. Shares the app cookie/session store.
void  webview_open_virt_popup(const std::string& url);

// Inject cookies from cf_bridge (or any external source) into the shared
// WKWebsiteDataStore.  json_arr must be a JSON array where each element is
// an object with at minimum { name, value, domain } string keys.
// Optional per-cookie keys: path (string), secure (bool), httpOnly (bool),
// expiresAt (number, Unix epoch seconds).
// This function is safe to call from any C++ thread.
void  webview_inject_cookies(const std::string& json_arr);

// Async-read all cookies from WKWebsiteDataStore and deliver them as a
// JSON array string to |callback| on the main thread.
// Each element matches the inject_cookies format so the output can be
// forwarded directly to a Qt WebEngine profile via /virt-show cookies field.
void  webview_dump_cookies_json(std::function<void(const std::string&)> callback);

// Set the customUserAgent on every open WKWebView tab to the Chrome UA
// that cf_bridge used when solving the Cloudflare challenge.  CF binds
// cf_clearance to the UA that solved it -- requests carrying the cookie
// must use the same UA string or CF will reject them.
// Safe to call from any thread; dispatches to the main thread internally.
void  webview_set_cf_user_agent(const std::string& ua);

// Reapply rounded bottom corners on the active WebView2 child HWND.
// WebView2's compositor may reset SetWindowRgn during page rendering.
// Call once per frame after SwapBuffers (Windows-only; no-op elsewhere).
void  webview_reapply_corners();

// Tear down everything (called on app exit).
void  webview_shutdown();
