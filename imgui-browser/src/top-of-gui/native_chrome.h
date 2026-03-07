#pragma once
// native_chrome.h -- AppKit-native tab bar + toolbar + status bar.
// Replaces the ImGui chrome panels with real NSViews so that NSTextField
// and NSButton have proper focus/hit-testing with no ImGui interference.

#include "app_state.h"

// Create all native chrome views and add them to ns_window (NSWindow*).
// php_port is used for the new-tab default URL.
// Must be called AFTER glfwGetCocoaWindow and webview_init.
void native_chrome_create(void* ns_window, AppState* state, int php_port);

// Sync AppState -> NSViews each frame.  Returns TOTAL_CHROME_TOP height
// in logical points (same as the old chrome_draw_top return value).
// If focus_url_next_frame is true it focuses the URL field and clears the flag.
int  native_chrome_update(AppState* state);

// Height of the bottom status bar in logical points.
int  native_chrome_status_h();

// True when the pointer is inside any chrome view (used to suppress
// the window-drag NSEvent monitor when the user clicks a button).
bool native_chrome_has_hover();

// Call when the window is resized so views reposition correctly.
void native_chrome_resize(int win_w, int win_h);

// Screen-X (window logical-point coords) of the right edge of each
// button, used to anchor the ImGui floating bookmark/history panels.
float native_chrome_bm_btn_x();
float native_chrome_hist_btn_x();

// Focus the URL field (Cmd+L).
void native_chrome_focus_url();

// Tear down all views on shutdown.
void native_chrome_destroy();

// Evaluate arbitrary JS in the toolbar WKWebView.
// Safe to call from the render/main thread.
void native_chrome_eval_toolbar_js(const char* js);
