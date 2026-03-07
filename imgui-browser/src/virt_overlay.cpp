#include "virt_overlay.h"

#include <curl/curl.h>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ── Internal state ────────────────────────────────────────────────────────────

static std::vector<std::string> s_patterns;
static std::atomic<bool>        s_active{false};
static std::mutex               s_pos_mu;
static int  s_last_x{-1}, s_last_y{-1}, s_last_w{-1}, s_last_h{-1};

// ── HTTP helpers ──────────────────────────────────────────────────────────────

static size_t _null_write(char*, size_t sz, size_t n, void*) { return sz * n; }

static bool _http_post(const char* path, const std::string& body) {
    CURL* c = curl_easy_init();
    if (!c) return false;

    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:9925%s", path);

    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(c, CURLOPT_URL,           url);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS,    body.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER,    hdrs);
    curl_easy_setopt(c, CURLOPT_TIMEOUT,       2L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL,      1L);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, _null_write);

    CURLcode rc = curl_easy_perform(c);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return (rc == CURLE_OK);
}

// ── JSON parsing ──────────────────────────────────────────────────────────────
// Extracts all "pattern": "..." values from the virt-pages.json file.
// Intentionally simple -- works for the expected well-formed config format.

static std::vector<std::string> _parse_patterns(const std::string& json) {
    std::vector<std::string> out;
    const std::string key = "\"pattern\"";
    size_t pos = 0;
    while ((pos = json.find(key, pos)) != std::string::npos) {
        pos += key.size();
        // Skip whitespace and the colon
        while (pos < json.size() &&
               (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t'))
            ++pos;
        if (pos < json.size() && json[pos] == '"') {
            ++pos;
            size_t end = json.find('"', pos);
            if (end != std::string::npos) {
                out.push_back(json.substr(pos, end - pos));
                pos = end + 1;
            }
        }
    }
    return out;
}

// ── Public API ────────────────────────────────────────────────────────────────

void virt_overlay_init(const std::string& json_path) {
    std::ifstream f(json_path);
    if (!f) {
        fprintf(stderr, "[virt] config not found: %s\n", json_path.c_str());
        return;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    s_patterns = _parse_patterns(ss.str());
    fprintf(stderr, "[virt] loaded %zu URL pattern(s) from %s\n",
            s_patterns.size(), json_path.c_str());
    for (const auto& p : s_patterns)
        fprintf(stderr, "[virt]   pattern: %s\n", p.c_str());
}

bool virt_overlay_check_url(const std::string& url) {
    for (const auto& p : s_patterns)
        if (url.find(p) != std::string::npos) return true;
    return false;
}

void virt_overlay_show(const std::string& url, int x, int y, int w, int h,
                        const std::string& cookies_json) {
    {
        std::lock_guard<std::mutex> g(s_pos_mu);
        s_last_x = x; s_last_y = y; s_last_w = w; s_last_h = h;
    }

    // Escape the URL for JSON
    std::string safe_url;
    safe_url.reserve(url.size());
    for (char ch : url) {
        if (ch == '"' || ch == '\\') safe_url += '\\';
        safe_url += ch;
    }

    // Build JSON body as a string so it can hold a large cookies array
    std::string body;
    body.reserve(256 + cookies_json.size());
    body += "{\"url\":\"";
    body += safe_url;
    body += "\",\"x\":";
    body += std::to_string(x);
    body += ",\"y\":";
    body += std::to_string(y);
    body += ",\"w\":";
    body += std::to_string(w);
    body += ",\"h\":";
    body += std::to_string(h);
    if (!cookies_json.empty() && cookies_json != "[]") {
        body += ",\"cookies\":";
        body += cookies_json;
    }
    body += "}";

    fprintf(stderr, "[virt] show %dx%d+%d+%d  url=%s  cookies=%zu bytes\n",
            w, h, x, y, url.c_str(), cookies_json.size());

    // Set active immediately (optimistic) then fire async so render loop never blocks.
    s_active = true;
    std::thread([body]() { _http_post("/virt-show", body); }).detach();
}

void virt_overlay_hide() {
    if (!s_active) return;
    fprintf(stderr, "[virt] hide\n");
    s_active = false;
    std::thread([]() { _http_post("/virt-hide", "{}"); }).detach();
}

void virt_overlay_tick(int x, int y, int w, int h) {
    if (!s_active) return;

    bool changed = false;
    {
        std::lock_guard<std::mutex> g(s_pos_mu);
        if (x != s_last_x || y != s_last_y || w != s_last_w || h != s_last_h) {
            s_last_x = x; s_last_y = y; s_last_w = w; s_last_h = h;
            changed = true;
        }
    }
    if (!changed) return;

    char json[128];
    snprintf(json, sizeof(json), "{\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}",
             x, y, w, h);
    std::string body(json);
    std::thread([body]() { _http_post("/virt-move", body); }).detach();
}

bool virt_overlay_is_active() {
    return s_active.load();
}
