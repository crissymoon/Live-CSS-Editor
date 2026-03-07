// main_funcs.h -- Forward declarations for all extracted functions.
// Include this in any file that needs to call them.
#pragma once
#include "main_globals.h"
#include "main_args.h"

// Content area layout (main_layout.mm)
void reposition_webviews(int chrome_top, int chrome_bot, int w, int h);

// Navigation command dispatcher (main_dispatch.mm)
void dispatch_nav(AppState::NavCmd& cmd);

// Menu bar builder -- called once after GLFW creates the window (main_menu.mm)
void build_app_menubar();

// WebView callback struct constructor + JS script loading (main_wvcbs.mm)
WebViewCallbacks build_webview_callbacks(const Args& args);

// Server process startup -- PHP and Node (main_servers.mm)
void start_all_servers(const Args& args);

// Render loop -- runs until the window is closed (main_render.mm)
void main_render_loop(const Args& args);

// GLFW callbacks -- defined in main_callbacks.mm, registered in main()
void xcm_cb_error(int code, const char* desc);
void xcm_cb_window_size(GLFWwindow* win, int w, int h);
void xcm_cb_framebuffer_size(GLFWwindow* win, int w, int h);
void xcm_cb_window_refresh(GLFWwindow* win);
