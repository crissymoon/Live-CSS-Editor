// native_chrome_stub.cpp -- ImGui chrome for non-Apple platforms.
//
// On macOS native_chrome.mm creates real NSView-based toolbar and tab bar
// widgets inside the window.  On Linux and Windows the ImGui chrome layer
// (chrome.cpp) is used directly from this stub.

#include "chrome.h"
#include "app_state.h"
#include "../webview.h"
#include "imgui.h"
#include <string>

static int s_php_port = 9879;

void native_chrome_create(void*, AppState*, int php_port) {
    s_php_port = php_port;
}

int native_chrome_update(AppState* st) {
    bool new_tab   = false;
    int  close_idx = -1;
    int  h = chrome_draw_top(st, st->win_w, st->win_h, new_tab, close_idx);

    if (close_idx >= 0 && (int)st->tabs.size() > 1 &&
        close_idx < (int)st->tabs.size()) {
        if (st->tabs[close_idx].wv_handle)
            webview_destroy(st->tabs[close_idx].wv_handle);
        st->tabs[close_idx].wv_handle = nullptr;
        st->close_tab(close_idx);
    }

    if (new_tab) {
        std::string url = "http://127.0.0.1:" + std::to_string(s_php_port) + "/";
        int idx = st->new_tab(url);
        st->tabs[idx].wv_handle = webview_create(st->tabs[idx].id, url);
    }

    return h;
}

int   native_chrome_status_h()                  { return STATUS_HEIGHT_PX; }
bool  native_chrome_has_hover()                 { return ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive(); }
void  native_chrome_resize(int, int)            {}
float native_chrome_bm_btn_x()                  { return 0.0f; }
float native_chrome_hist_btn_x()                { return 0.0f; }
void  native_chrome_focus_url()                 {}
void  native_chrome_destroy()                   {}
void  native_chrome_eval_toolbar_js(const char*){}
