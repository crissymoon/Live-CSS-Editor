// main_wvcbs.mm -- Builds the WebViewCallbacks struct passed to webview_init()
// and loads performance-enhancement scripts from dev-browser/src/.
//
// Returned by value; caller passes the result straight to webview_init().

#include "main_funcs.h"

WebViewCallbacks build_webview_callbacks(const Args& args) {
    WebViewCallbacks wv_cbs;

    wv_cbs.on_url_change = [](int tab_id, const std::string& url) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) {
                t.url       = url;
                t.is_secure = url.size() >= 8 && url.substr(0, 8) == "https://";
                break;
            }
        // Push to browsing history (skip consecutive duplicates)
        auto& hist = g_state.history;
        if (!url.empty() &&
            (hist.empty() || hist.back().url != url)) {
            HistoryEntry he;
            he.url = url;
            he.ts  = (int64_t)time(nullptr);
            if (hist.size() >= 1000) hist.erase(hist.begin());
            hist.push_back(he);
            // Throttled disk save (every 5 navigations)
            static int s_hist_n = 0;
            if (++s_hist_n % 5 == 0) persist_save_history(g_state.history);
        }
        // Always hide the toolbar badge when the webview navigates to a new URL.
        Tab* active = g_state.current_tab();
        if (active && active->id == tab_id) {
            native_chrome_eval_toolbar_js("xcmHideSecureBadge&&xcmHideSecureBadge()");
        }
    };

    wv_cbs.on_favicon_change = [](int tab_id, const std::string& fav_url) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.favicon = fav_url; break; }
    };

    wv_cbs.on_title_change = [](int tab_id, const std::string& title) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) {
                t.title = title;
                for (int hi = (int)g_state.history.size() - 1; hi >= 0; hi--) {
                    if (g_state.history[hi].url == t.url) {
                        g_state.history[hi].title = title;
                        break;
                    }
                }
                break;
            }
    };

    wv_cbs.on_progress = [](int tab_id, float p) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.progress = p; break; }
    };

    wv_cbs.on_loading = [](int tab_id, bool loading) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.loading = loading; break; }
    };

    wv_cbs.on_nav_state = [](int tab_id, bool back, bool fwd) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.can_back = back; t.can_forward = fwd; break; }
    };

    wv_cbs.on_wkwv_fps = [](double fps) {
        g_state.fps_wkwv = fps;
    };

    // ── Performance-enhancement script loading ────────────────────────
    // Scripts live in dev-browser/src/ (one level above apps_dir).
    if (!args.apps_dir.empty()) {
        std::string src_dir = args.apps_dir + "/../src";
        auto load_file = [](const std::string& p) -> std::string {
            FILE* f = fopen(p.c_str(), "r");
            if (!f) {
                fprintf(stderr, "[wvcbs] WARNING: cannot load script: %s\n", p.c_str());
                return "";
            }
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            rewind(f);
            std::string s(sz, '\0');
            fread(&s[0], 1, sz, f);
            fclose(f);
            return s;
        };

        // Injection order matters -- ticker-lite must run before any script
        // that reads __xcmIdleThreshold / __xcmTick.
        for (const char* name : {
            "xcm-ticker-lite.js",
            "xcm-clip-watcher.js",
            "xcm-smooth-scroll.js",
            "xcm-media-preload.js",
            "lazy-inject.js",
            "virtualizer-inject.js",
            "compress-inject.js",
            "xcm-app-helper.js",
            "xcm-scroll-restore.js",
            "input-watcher.js",
            "chrome-gl-compositor.js",
        }) {
            std::string src = load_file(src_dir + "/" + name);
            if (!src.empty()) {
                wv_cbs.extra_scripts.push_back(std::move(src));
                fprintf(stderr, "[wvcbs] loaded extra script: %s\n", name);
            }
        }

        // All-frames scripts: injected into every sub-frame including third-
        // party iframes (forMainFrameOnly:NO).  Must have host-gate guards.
        for (const char* name : {
            "xcm-stripe-shim.js"
        }) {
            std::string src = load_file(src_dir + "/" + name);
            if (!src.empty()) {
                wv_cbs.extra_scripts_all_frames.push_back(std::move(src));
                fprintf(stderr, "[wvcbs] loaded all-frames script: %s\n", name);
            }
        }
    }

    return wv_cbs;
}
