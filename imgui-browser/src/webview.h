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

    // Additional JS strings injected at document-start into every frame
    // (main tab and all popups). Injected after JS_INIT and JS_MASK_WEBVIEW
    // in the order they appear in this vector.
    // Use this for xcm performance scripts (input-watcher, chrome-gl-compositor)
    // and any app-specific bootstrap code.
    std::vector<std::string> extra_scripts;
};

// Initialise the WebView subsystem.  Must be called from the main thread
// AFTER the GLFW window is created.  Passes the NSWindow handle.
void webview_init(void* ns_window, AppState* state, WebViewCallbacks cbs);

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

// Clear all stored website data (cookies, localStorage, IndexedDB, cache,
// service worker registrations). Use this to flush a stuck auth state.
void  webview_clear_data();

// Tear down everything (called on app exit).
void  webview_shutdown();
