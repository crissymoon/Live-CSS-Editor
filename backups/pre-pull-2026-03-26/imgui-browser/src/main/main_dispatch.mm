// main_dispatch.mm -- Navigation command dispatcher.
// dispatch_nav() is called from the render loop once per frame to drain the
// lockless AppState::NavCmd queue.  All WKWebView mutations must happen on
// the main thread; the queue is only drained from the render loop, so this
// is always safe.

#include "main_funcs.h"

void dispatch_nav(AppState::NavCmd& cmd) {
    // Resolve tab
    Tab* tab = nullptr;
    if (cmd.tab_id == -1) {
        tab = g_state.current_tab();
    } else if (cmd.tab_id == -2) {
        // New tab request from cmd_server /newtab
        int idx = g_state.new_tab(cmd.url);
        tab = &g_state.tabs[idx];
        tab->wv_handle = webview_create(tab->id, cmd.url);
        reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
        return;
    // tab_id -5: close tab -- url is the tab .id as a decimal string
    } else if (cmd.tab_id == -5) {
        int tid = 0;
        try { tid = std::stoi(cmd.url); } catch (...) {}
        for (int i = 0; i < (int)g_state.tabs.size(); i++) {
            if (g_state.tabs[i].id == tid) {
                webview_destroy(g_state.tabs[i].wv_handle);
                g_state.tabs[i].wv_handle = nullptr;
                g_state.close_tab(i);
                reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
                return;
            }
        }
        return;
    } else if (cmd.tab_id == -3) {
        // Eval request: "__eval__:<js>"
        tab = g_state.current_tab();
        if (tab && tab->wv_handle && cmd.url.substr(0, 8) == "__eval__") {
            webview_eval_js(tab->wv_handle, cmd.url.substr(8), nullptr);
        }
        return;
    } else {
        for (auto& t : g_state.tabs)
            if (t.id == cmd.tab_id) { tab = &t; break; }
    }

    if (!tab) return;
    void* h = tab->wv_handle;
    if (!h) return;

    if      (cmd.url == "__back__")    webview_go_back(h);
    else if (cmd.url == "__forward__") webview_go_forward(h);
    else if (cmd.url == "__reload__")  webview_reload(h);
    else if (cmd.url == "__stop__")    webview_stop(h);
    else if (cmd.url == "__js_on__")  { tab->js_enabled = true;  webview_set_js_enabled(h, true); }
    else if (cmd.url == "__js_off__") { tab->js_enabled = false; webview_set_js_enabled(h, false); }
    else if (cmd.url == "__bookmark_toggle__") {
        bool found = false;
        for (int bi = 0; bi < (int)g_state.bookmarks.size(); bi++) {
            if (g_state.bookmarks[bi].url == tab->url) {
                g_state.bookmarks.erase(g_state.bookmarks.begin() + bi);
                found = true;
                break;
            }
        }
        if (!found)
            g_state.bookmarks.push_back({tab->url, tab->title});
        persist_save_bookmarks(g_state.bookmarks);
    }
    else if (cmd.url == "__devtools__") {
        g_state.dev_tools_open = !g_state.dev_tools_open;
        webview_open_inspector(h);
    }
    else {
        webview_load_url(h, cmd.url);
    }
}
