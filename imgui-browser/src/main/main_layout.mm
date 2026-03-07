// main_layout.mm -- Content area positioning.
// reposition_webviews() is called from dispatch_nav, the GLFW refresh
// callback, and the render loop whenever chrome height changes.

#include "main_funcs.h"

void reposition_webviews(int chrome_top, int chrome_bot, int w, int h) {
    int content_y = chrome_top;
    int content_h = h - chrome_top - chrome_bot;
    if (content_h < 1) content_h = 1;
    for (auto& tab : g_state.tabs) {
        if (!tab.wv_handle) continue;
        webview_resize(tab.wv_handle, 0, content_y, w, content_h);
        if (&tab == g_state.current_tab())
            webview_show(tab.wv_handle);
        else
            webview_hide(tab.wv_handle);
    }
}
