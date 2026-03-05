#pragma once
// app_state.h -- shared application state (tabs, URL, FPS counters)
// All fields are plain C++17 data structures; no Obj-C types.
// Modules that need Obj-C bridging include webview.h separately.

#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <deque>
#include <chrono>
#include "persistence.h"

// ── Constants ─────────────────────────────────────────────────────────
constexpr int    CHROME_HEIGHT_PX  = 44;   // toolbar row (URL bar + nav buttons)
constexpr int    STATUS_HEIGHT_PX  = 0;    // status bar removed (info lives in URL bar)
constexpr int    TAB_BAR_HEIGHT_PX = 70;   // title strip + tab row (includes 30px top padding for traffic lights)
constexpr int    TOTAL_CHROME_TOP  = TAB_BAR_HEIGHT_PX + CHROME_HEIGHT_PX;
constexpr int    TRAFFIC_LIGHT_W   = 82;   // px to skip on left for macOS traffic lights
constexpr int    MAX_TABS          = 24;
constexpr int    URL_BUF_SIZE      = 2048;
constexpr int    CMD_PORT          = 9878;
constexpr int    PHP_PORT          = 9879;

// ── Tab ───────────────────────────────────────────────────────────────
struct Tab {
    int         id          = 0;
    std::string url         = "";
    std::string title       = "New Tab";
    bool        loading     = false;
    bool        can_back    = false;
    bool        can_forward = false;
    float       progress    = 0.0f;  // 0-1 while loading
    // webview handle stored as void* to keep this header pure C++
    void*       wv_handle   = nullptr;
    char        url_buf[URL_BUF_SIZE] = {};

    // Security / JS
    bool        js_enabled  = true;   // per-tab JavaScript toggle
    bool        is_secure   = false;  // true when current URL is https://

    Tab() { url_buf[0] = '\0'; }
    explicit Tab(int i, const std::string& u = "")
        : id(i), url(u), title("Loading...") {
        snprintf(url_buf, URL_BUF_SIZE, "%s", u.c_str());
    }
};

// ── FPS ring buffer ───────────────────────────────────────────────────
struct FpsCounter {
    static constexpr int RING = 120;
    std::array<double, RING> samples{};
    int  head   = 0;
    int  count  = 0;
    double last_ts = 0.0;  // seconds

    void tick(double now_sec) {
        if (last_ts > 0.0) {
            double dt = now_sec - last_ts;
            if (dt > 0.0 && dt < 1.0) {
                samples[head] = 1.0 / dt;
                head   = (head + 1) % RING;
                if (count < RING) count++;
            }
        }
        last_ts = now_sec;
    }

    double fps() const {
        if (!count) return 0.0;
        int i = (head - 1 + RING) % RING;
        return samples[i];
    }
    double fps_avg() const {
        if (!count) return 0.0;
        double s = 0.0;
        for (int i = 0; i < count; i++) s += samples[i];
        return s / count;
    }
};

// ── App-wide state ────────────────────────────────────────────────────
struct AppState {
    // Tabs
    std::vector<Tab> tabs;
    int   active_tab     = 0;
    int   next_tab_id    = 1;

    // Window geometry (pixels, not points)
    int   win_w          = 1400;
    int   win_h          = 900;
    float dpi_scale      = 1.0f;

    // FPS
    FpsCounter fps_host;    // host (ImGui render loop)
    double     fps_wkwv    = 0.0;  // fed back from WKWebView JS bridge
    double     fps_engine  = 0.0;  // WASM compositor

    // Status bar text
    std::string status_text;
    std::string hover_url;

    // Command / PHP servers
    bool   php_server_ok  = false;
    bool   node_server_ok = false;
    int    php_pid        = -1;
    int    node_pid       = -1;

    // Settings
    bool   dev_tools_open         = false;
    bool   show_fps_bar           = true;
    float  ui_scale               = 1.0f;
    bool   focus_url_next_frame   = false;  // set true to steal focus to URL bar

    // Bookmarks and history -- persisted to ~/.xcm-browser/
    std::vector<HistoryEntry>  history;
    std::vector<BookmarkEntry> bookmarks;

    // Panel visibility flags
    bool   show_bookmarks_panel   = false;
    bool   show_history_panel     = false;
    // Search filter for history panel
    char   history_filter[128]    = {};

    // Thread-safe navigation queue (cmd_server pushes, main loop pops)
    struct NavCmd { int tab_id; std::string url; };
    std::mutex          cmd_mutex;
    std::deque<NavCmd>  cmd_queue;

    void push_nav(int tab_id, const std::string& url) {
        std::lock_guard<std::mutex> lk(cmd_mutex);
        cmd_queue.push_back({tab_id, url});
    }
    bool pop_nav(NavCmd& out) {
        std::lock_guard<std::mutex> lk(cmd_mutex);
        if (cmd_queue.empty()) return false;
        out = cmd_queue.front();
        cmd_queue.pop_front();
        return true;
    }

    // Convenience: get current tab (nullptr if tabs is empty)
    Tab* current_tab() {
        if (tabs.empty() || active_tab < 0 || active_tab >= (int)tabs.size())
            return nullptr;
        return &tabs[active_tab];
    }

    // Open a new tab. Returns the index.
    int new_tab(const std::string& url = "") {
        if ((int)tabs.size() >= MAX_TABS) return active_tab;
        int idx = (int)tabs.size();
        tabs.emplace_back(next_tab_id++, url);
        active_tab = idx;
        return idx;
    }

    void close_tab(int idx) {
        if (tabs.size() <= 1) return;  // always keep at least one tab
        tabs.erase(tabs.begin() + idx);
        if (active_tab >= (int)tabs.size())
            active_tab = (int)tabs.size() - 1;
    }
};
