#pragma once
// webview.h -- WKWebView lifecycle + bridge (macOS only)
// All Obj-C types are hidden behind void* so this header is pure C++ and
// can be included from any .cpp without pulling in Cocoa headers.

#include "app_state.h"
#include <string>
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

// Tear down everything (called on app exit).
void  webview_shutdown();
