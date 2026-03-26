// main_layout.mm -- Content area positioning.
// reposition_webviews() is called from dispatch_nav, the GLFW refresh
// callback, and the render loop whenever chrome height changes.

#include "main_funcs.h"
#include "../platform/platform.h"

void reposition_webviews(int chrome_top, int chrome_bot, int w, int h) {
    // On Windows the WebView2 child HWND swallows mouse messages, preventing
    // resize grips from working at the window edges.  Inset the content area
    // so the parent HWND receives WM_NCHITTEST at the left/right/bottom.
    int inset = platform_resize_inset();
    int content_x = inset;
    int content_y = chrome_top;
    int content_w = w - 2 * inset;
    int content_h = h - chrome_top - chrome_bot - inset;
    if (content_w < 1) content_w = 1;
    if (content_h < 1) content_h = 1;
    for (auto& tab : g_state.tabs) {
        if (!tab.wv_handle) continue;
        webview_resize(tab.wv_handle, content_x, content_y, content_w, content_h);
        if (&tab == g_state.current_tab())
            webview_show(tab.wv_handle);
        else
            webview_hide(tab.wv_handle);
    }
}
