#pragma once
// browser_settings.h -- startup URL and chrome-visibility preferences.
//
// Settings are stored in ~/.xcm-browser/settings.json alongside the
// existing "data_dir" key, so a single file controls everything.
//
// Supported JSON keys
// -------------------
//   "startup_url"     string   URL or file:// path opened on launch.
//                              Empty string = use the compiled default.
//   "ui_mode"         string   Shorthand preset (see UiMode below).
//                              Accepted values (case-sensitive):
//                                "full"          - tabs + toolbar visible
//                                "no_tabs"       - toolbar only, tab strip hidden
//                                "grab_bar_only" - only the thin window-drag strip
//   "show_tabs"       bool     Show / hide the tab strip row.
//   "show_toolbar"    bool     Show / hide the URL bar + nav-buttons row.
//   "show_status_bar" bool     Show / hide the bottom status bar.
//
// Individual flag keys always take effect after a ui_mode preset, so
// you can mix them:
//   { "ui_mode": "no_tabs", "show_status_bar": true }
//
// NOTE: The native-chrome path (native_chrome.mm) uses the compile-time
// constant TOTAL_CHROME_TOP for WebView placement.  When using a
// non-full ui_mode with native chrome, recompile after adjusting the
// constants in app_state.h, or let native_chrome_update() read the
// height via chrome_computed_top_height() declared below.
//
// Pure C++17 header -- no external libraries required.

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "persistence.h"   // xcm_settings_path(), json_esc()

// ── UI layout mode enum ───────────────────────────────────────────────

enum class UiMode {
    FULL,           // tab strip + toolbar (default)
    NO_TABS,        // toolbar only -- tab bar hidden
    GRAB_BAR_ONLY,  // only a slim window-drag strip; no tab bar, no toolbar
};

// String <-> enum helpers
inline const char* ui_mode_to_str(UiMode m) {
    switch (m) {
        case UiMode::FULL:          return "full";
        case UiMode::NO_TABS:       return "no_tabs";
        case UiMode::GRAB_BAR_ONLY: return "grab_bar_only";
    }
    return "full";
}

inline UiMode ui_mode_from_str(const std::string& s) {
    if (s == "no_tabs" || s == "toolbar_only") return UiMode::NO_TABS;
    if (s == "grab_bar_only")                  return UiMode::GRAB_BAR_ONLY;
    return UiMode::FULL;
}

// ── Settings struct ───────────────────────────────────────────────────

struct BrowserSettings {
    // Startup page.  Empty -> caller uses its own default (e.g. localhost:9879).
    std::string startup_url;

    // Chrome-row visibility.  Individual flags are the authoritative source;
    // the ui_mode preset writes these flags but does not store extra state.
    bool show_tabs         = true;   // top tab strip
    bool show_toolbar      = true;   // URL bar + nav buttons row
    bool show_status_bar   = false;  // bottom status bar (off by default)

    // PHP-WASM settings.
    // When wasm_enabled is true the app launches the php-wasm-project dev
    // server on startup and uses it as the initial page instead of the
    // native PHP server.  The server is the Node server.js found at
    //   <wasm_dir>/server.js
    // which defaults to the bundled php-wasm-project directory.
    //
    // JSON keys:
    //   "wasm_enabled"   bool    (default false)
    //   "wasm_port"      number  (default 8082)  port for the WASM dev server
    //   "wasm_dir"       string  absolute path to the php-wasm-project root
    //                            (default: auto-detected relative to binary)
    bool        wasm_enabled   = false;
    int         wasm_port      = 8082;
    std::string wasm_dir;   // empty = auto-detect

    // Apply a named preset.  After this you can still flip individual flags.
    void apply_mode(UiMode mode) {
        switch (mode) {
            case UiMode::FULL:
                show_tabs    = true;
                show_toolbar = true;
                break;
            case UiMode::NO_TABS:
                show_tabs    = false;
                show_toolbar = true;
                break;
            case UiMode::GRAB_BAR_ONLY:
                show_tabs    = false;
                show_toolbar = false;
                break;
        }
    }

    // Derive which preset best describes the current flags.
    UiMode ui_mode() const {
        if ( show_tabs && show_toolbar) return UiMode::FULL;
        if (!show_tabs && show_toolbar) return UiMode::NO_TABS;
        return UiMode::GRAB_BAR_ONLY;
    }

    // Pixel height that the top chrome will consume at runtime.
    // Keep in sync with the constants in app_state.h and with
    // chrome_draw_top() / draw_grab_strip().
    //   GRAB_BAR_ONLY  -> 32 px  (just the macOS traffic-lights strip)
    //   NO_TABS        -> 74 px  (30 px traffic-light pad + 44 px toolbar)
    //   FULL           -> 114 px (70 px tab bar + 44 px toolbar)
    int computed_top_height() const {
        if (!show_tabs && !show_toolbar) return 32;
        if (!show_tabs &&  show_toolbar) return 74;
        if ( show_tabs && !show_toolbar) return 70;
        return 114;   // full: 70 + 44
    }
};

// ── Persistence helpers ───────────────────────────────────────────────
// Both functions operate on ~/.xcm-browser/settings.json.
// Other keys in that file (e.g. "data_dir") are preserved on save.

// Load browser settings from settings.json.  Missing keys fall back to
// BrowserSettings defaults.
inline BrowserSettings browser_settings_load() {
    BrowserSettings out;

    std::string path = xcm_settings_path();
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return out;   // file doesn't exist yet -- return defaults

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return out; }

    std::string buf(static_cast<size_t>(sz), '\0');
    fread(&buf[0], 1, static_cast<size_t>(sz), f);
    fclose(f);

    // Minimal JSON-string extractor (no third-party parser).
    auto get_str = [&](const std::string& key) -> std::string {
        std::string needle = "\"" + key + "\"";
        auto pos = buf.find(needle);
        if (pos == std::string::npos) return "";
        pos += needle.size();
        while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == ':' ||
                                    buf[pos] == '\n' || buf[pos] == '\r')) ++pos;
        if (pos >= buf.size() || buf[pos] != '"') return "";
        ++pos;
        std::string val;
        while (pos < buf.size() && buf[pos] != '"') {
            if (buf[pos] == '\\' && pos + 1 < buf.size()) {
                ++pos; val += buf[pos];
            } else {
                val += buf[pos];
            }
            ++pos;
        }
        return val;
    };

    // Bool extractor -- returns def if key absent.
    auto get_bool = [&](const std::string& key, bool def) -> bool {
        std::string needle = "\"" + key + "\"";
        auto pos = buf.find(needle);
        if (pos == std::string::npos) return def;
        pos += needle.size();
        while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == ':' ||
                                    buf[pos] == '\n' || buf[pos] == '\r')) ++pos;
        if (pos + 4 <= buf.size() && buf.substr(pos, 4) == "true")  return true;
        if (pos + 5 <= buf.size() && buf.substr(pos, 5) == "false") return false;
        return def;
    };

    // Int extractor -- returns def if key absent or non-numeric.
    auto get_int = [&](const std::string& key, int def) -> int {
        std::string needle = "\"" + key + "\"";
        auto pos = buf.find(needle);
        if (pos == std::string::npos) return def;
        pos += needle.size();
        while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == ':' ||
                                    buf[pos] == '\n' || buf[pos] == '\r')) ++pos;
        if (pos >= buf.size() || !std::isdigit((unsigned char)buf[pos])) return def;
        int v = 0;
        while (pos < buf.size() && std::isdigit((unsigned char)buf[pos]))
            v = v * 10 + (buf[pos++] - '0');
        return v;
    };

    // 1. Apply ui_mode preset first (lowest priority).
    std::string mode_str = get_str("ui_mode");
    if (!mode_str.empty())
        out.apply_mode(ui_mode_from_str(mode_str));

    // 2. Apply individual flag overrides (higher priority than preset).
    //    Only override if the key is explicitly present in the file.
    auto key_present = [&](const std::string& key) -> bool {
        return buf.find("\"" + key + "\"") != std::string::npos;
    };
    if (key_present("show_tabs"))
        out.show_tabs = get_bool("show_tabs", out.show_tabs);
    if (key_present("show_toolbar"))
        out.show_toolbar = get_bool("show_toolbar", out.show_toolbar);
    if (key_present("show_status_bar"))
        out.show_status_bar = get_bool("show_status_bar", out.show_status_bar);

    // 3. Startup URL (empty string in JSON means "use default").
    out.startup_url = get_str("startup_url");

    // 4. WASM settings.
    if (key_present("wasm_enabled"))
        out.wasm_enabled = get_bool("wasm_enabled", out.wasm_enabled);
    if (key_present("wasm_port"))
        out.wasm_port = get_int("wasm_port", out.wasm_port);
    out.wasm_dir = get_str("wasm_dir");

    return out;
}

// Save browser settings back to settings.json, preserving the "data_dir"
// key (and any other unknown keys are dropped -- safe because only
// "data_dir" and the browser-settings keys have meaning to this app).
inline void browser_settings_save(const BrowserSettings& s) {
    // Read the existing data_dir before touching the file.
    // xcm_data_dir() reads settings.json internally and creates the dir;
    // that is an acceptable side-effect here.
    std::string data_dir = xcm_data_dir();

    std::string path = xcm_settings_path();
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;

    std::string dd  = json_esc(data_dir);
    std::string su  = json_esc(s.startup_url);
    std::string wdir = json_esc(s.wasm_dir);

    fprintf(f,
        "{\n"
        "  \"data_dir\": \"%s\",\n"
        "  \"startup_url\": \"%s\",\n"
        "  \"ui_mode\": \"%s\",\n"
        "  \"show_tabs\": %s,\n"
        "  \"show_toolbar\": %s,\n"
        "  \"show_status_bar\": %s,\n"
        "  \"wasm_enabled\": %s,\n"
        "  \"wasm_port\": %d,\n"
        "  \"wasm_dir\": \"%s\"\n"
        "}\n",
        dd.c_str(),
        su.c_str(),
        ui_mode_to_str(s.ui_mode()),
        s.show_tabs         ? "true" : "false",
        s.show_toolbar      ? "true" : "false",
        s.show_status_bar   ? "true" : "false",
        s.wasm_enabled      ? "true" : "false",
        s.wasm_port,
        wdir.c_str());

    fclose(f);
}

// Ensure ~/.xcm-browser/settings.json exists with at least the default
// browser-settings keys.  Safe to call every launch.
inline void browser_settings_ensure_defaults() {
    std::string path = xcm_settings_path();
    FILE* probe = fopen(path.c_str(), "r");
    if (probe) {
        // File exists.  Check whether our keys are already there.
        fseek(probe, 0, SEEK_END);
        long sz = ftell(probe); rewind(probe);
        bool has_su    = false;
        bool has_uitm  = false;
        if (sz > 0) {
            std::string buf(static_cast<size_t>(sz), '\0');
            fread(&buf[0], 1, static_cast<size_t>(sz), probe);
            has_su   = buf.find("\"startup_url\"") != std::string::npos;
            has_uitm = buf.find("\"ui_mode\"")     != std::string::npos;
            // Also require wasm keys -- if missing, rewrite to add them.
            bool has_wasm = buf.find("\"wasm_enabled\"") != std::string::npos;
            if (!has_wasm) has_uitm = false;
        }
        fclose(probe);
        if (has_su && has_uitm) return;  // already written (all keys present)
    }
    // Write out full defaults (preserves data_dir via browser_settings_save).
    BrowserSettings defaults;
    browser_settings_save(defaults);
}
