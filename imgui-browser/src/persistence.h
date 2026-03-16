#pragma once
// persistence.h -- bookmarks and browsing history for imgui_browser
// Default data dir: ~/Desktop/crissys-style-tool/
// Settings file (always at fixed location): ~/.xcm-browser/settings.json
//   Supported keys:
//     "data_dir"  -- absolute path where history.json and bookmarks.json live
// Pure C++17 header; no external libraries required.

#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#if defined(_WIN32)
#  include <direct.h>
#endif

// ── Data structures ───────────────────────────────────────────────────

struct HistoryEntry {
    std::string url;
    std::string title;
    int64_t     ts = 0;   // unix seconds (0 = unknown)
};

struct BookmarkEntry {
    std::string url;
    std::string title;
};

inline int xcm_mkdir_portable(const char* path) {
#if defined(_WIN32)
    return _mkdir(path);
#else
    return mkdir(path, 0700);
#endif
}

// ── Settings + storage directory ─────────────────────────────────────

// Fixed path for the settings file -- always here regardless of data_dir.
inline std::string xcm_settings_path() {
    const char* home = getenv("HOME");
#if defined(_WIN32)
    if (!home || !*home) home = getenv("USERPROFILE");
#endif
    std::string cfg_dir = std::string(home ? home : ".") + "/.xcm-browser";
    xcm_mkdir_portable(cfg_dir.c_str());
    return cfg_dir + "/settings.json";
}

// Default data directory shown to the user.
inline std::string xcm_default_data_dir() {
    const char* home = getenv("HOME");
#if defined(_WIN32)
    if (!home || !*home) home = getenv("USERPROFILE");
#endif
    return std::string(home ? home : ".") + "/Desktop/crissys-style-tool";
}

// Write the default settings file if it does not exist yet.
static inline void xcm_ensure_settings() {
    std::string path = xcm_settings_path();
    FILE* probe = fopen(path.c_str(), "r");
    if (probe) { fclose(probe); return; }
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;
    fprintf(f, "{\n  \"data_dir\": \"%s\"\n}\n",
            xcm_default_data_dir().c_str());
    fclose(f);
}

// Read data_dir from settings, create the directory, return the path.
inline std::string xcm_data_dir() {
    xcm_ensure_settings();
    // Read settings file
    FILE* f = fopen(xcm_settings_path().c_str(), "r");
    std::string data_dir = xcm_default_data_dir();
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f); rewind(f);
        if (sz > 0) {
            std::string buf(sz, '\0');
            fread(&buf[0], 1, (size_t)sz, f);
            // Find "data_dir" value (simple scan, no full parser needed)
            const char* key = "\"data_dir\"";
            auto pos = buf.find(key);
            if (pos != std::string::npos) {
                pos += strlen(key);
                while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == ':')) pos++;
                if (pos < buf.size() && buf[pos] == '"') {
                    ++pos;
                    std::string val;
                    while (pos < buf.size() && buf[pos] != '"') {
                        if (buf[pos] == '\\' && pos + 1 < buf.size()) {
                            ++pos;
                            val += buf[pos];
                        } else {
                            val += buf[pos];
                        }
                        ++pos;
                    }
                    if (!val.empty()) data_dir = val;
                }
            }
        }
        fclose(f);
    }
    xcm_mkdir_portable(data_dir.c_str());
    return data_dir;
}

// ── Minimal JSON helpers ──────────────────────────────────────────────

static inline std::string json_esc(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        if      (c == '"')  { o += "\\\""; }
        else if (c == '\\') { o += "\\\\"; }
        else if (c == '\n') { o += "\\n";  }
        else if (c == '\r') { o += "\\r";  }
        else if (c == '\t') { o += "\\t";  }
        else if (c < 0x20)  { /* skip control chars */ }
        else                { o += (char)c; }
    }
    return o;
}

// Extract the string or number value for a given key from a flat JSON
// object literal.  Not a general parser -- handles one level of nesting.
static inline std::string json_get(const std::string& obj, const char* key) {
    std::string needle = std::string("\"") + key + "\"";
    auto pos = obj.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == ':')) pos++;
    if (pos >= obj.size()) return {};
    if (obj[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < obj.size() && obj[pos] != '"') {
            if (obj[pos] == '\\' && pos + 1 < obj.size()) {
                ++pos;
                if      (obj[pos] == 'n') val += '\n';
                else if (obj[pos] == 'r') val += '\r';
                else if (obj[pos] == 't') val += '\t';
                else                      val += obj[pos];
            } else {
                val += obj[pos];
            }
            ++pos;
        }
        return val;
    } else {
        std::string val;
        while (pos < obj.size() && obj[pos] != ',' && obj[pos] != '}' && obj[pos] != ' ')
            val += obj[pos++];
        return val;
    }
}

static inline std::string xcm_read_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return {};
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return {}; }
    std::string s(sz, '\0');
    fread(&s[0], 1, (size_t)sz, f);
    fclose(f);
    return s;
}

// ── History ───────────────────────────────────────────────────────────

inline void persist_save_history(const std::vector<HistoryEntry>& hist,
                                  size_t max_keep = 800) {
    std::string dir = xcm_data_dir();
    FILE* f = fopen((dir + "/history.json").c_str(), "w");
    if (!f) return;
    fprintf(f, "[\n");
    size_t start = hist.size() > max_keep ? hist.size() - max_keep : 0;
    for (size_t i = start; i < hist.size(); i++) {
        const auto& e = hist[i];
        fprintf(f, "  {\"url\":\"%s\",\"title\":\"%s\",\"ts\":%lld}%s\n",
                json_esc(e.url).c_str(),
                json_esc(e.title).c_str(),
                (long long)e.ts,
                (i + 1 < hist.size()) ? "," : "");
    }
    fprintf(f, "]\n");
    fclose(f);
}

inline std::vector<HistoryEntry> persist_load_history() {
    std::string src = xcm_read_file(xcm_data_dir() + "/history.json");
    std::vector<HistoryEntry> out;
    const char* p = src.c_str();
    while ((p = strchr(p, '{'))) {
        const char* end = strchr(p, '}');
        if (!end) break;
        std::string obj(p, end + 1);
        HistoryEntry e;
        e.url   = json_get(obj, "url");
        e.title = json_get(obj, "title");
        std::string ts = json_get(obj, "ts");
        e.ts    = ts.empty() ? 0 : (int64_t)atoll(ts.c_str());
        if (!e.url.empty()) out.push_back(std::move(e));
        p = end + 1;
    }
    return out;
}

// ── Bookmarks ─────────────────────────────────────────────────────────

inline void persist_save_bookmarks(const std::vector<BookmarkEntry>& bm) {
    std::string dir = xcm_data_dir();
    FILE* f = fopen((dir + "/bookmarks.json").c_str(), "w");
    if (!f) return;
    fprintf(f, "[\n");
    for (size_t i = 0; i < bm.size(); i++) {
        const auto& e = bm[i];
        fprintf(f, "  {\"url\":\"%s\",\"title\":\"%s\"}%s\n",
                json_esc(e.url).c_str(),
                json_esc(e.title).c_str(),
                (i + 1 < bm.size()) ? "," : "");
    }
    fprintf(f, "]\n");
    fclose(f);
}

inline std::vector<BookmarkEntry> persist_load_bookmarks() {
    std::string src = xcm_read_file(xcm_data_dir() + "/bookmarks.json");
    std::vector<BookmarkEntry> out;
    const char* p = src.c_str();
    while ((p = strchr(p, '{'))) {
        const char* end = strchr(p, '}');
        if (!end) break;
        std::string obj(p, end + 1);
        BookmarkEntry e;
        e.url   = json_get(obj, "url");
        e.title = json_get(obj, "title");
        if (!e.url.empty()) out.push_back(std::move(e));
        p = end + 1;
    }
    return out;
}
