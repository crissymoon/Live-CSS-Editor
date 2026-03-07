// cf_client.cpp -- libcurl bridge to the Python cf_bridge service.
// Calls http://127.0.0.1:9925/solve?url=<encoded> and injects the
// resulting cookies into WKWebsiteDataStore via webview_inject_cookies().

#include "cf_client.h"
#include <curl/curl.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <chrono>

// Declared in webview.mm (Obj-C++ cannot be included from plain .cpp).
extern void webview_inject_cookies(const std::string& json);

// ── libcurl write callback ────────────────────────────────────────────────
static size_t _write_cb(char* ptr, size_t size, size_t nmemb, void* user) {
    std::string* buf = reinterpret_cast<std::string*>(user);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

// ── Extract "cookies":[...] array from the cf_bridge JSON response ────────
// We do NOT pull in a full JSON library.  The response format is fixed:
//   {"ok":true/false,"cookies":[...],"elapsed":N.N,"url":"..."}
// We find the key, then use bracket counting to extract the array.
static std::string _extract_cookies_array(const std::string& body) {
    const char* needle = "\"cookies\":";
    auto pos = body.find(needle);
    if (pos == std::string::npos) return {};
    pos += strlen(needle);
    // Skip whitespace
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) ++pos;
    if (pos >= body.size() || body[pos] != '[') return {};

    int depth = 0;
    size_t start = pos;
    bool   in_str = false;
    for (size_t i = pos; i < body.size(); ++i) {
        char c = body[i];
        if (in_str) {
            if (c == '\\')           { ++i; continue; } // skip escaped char
            if (c == '"')            { in_str = false; }
        } else {
            if (c == '"')            { in_str = true;  }
            else if (c == '[')       { ++depth;         }
            else if (c == ']')       { if (--depth == 0) return body.substr(start, i - start + 1); }
        }
    }
    return {}; // unterminated
}

// ── Rate-limit: avoid blasting the bridge with duplicate requests ─────────
static std::mutex             s_mu;
static std::unordered_set<std::string> s_in_flight;  // URLs currently solving
static std::unordered_set<std::string> s_done_hosts; // hosts solved this session

// Derive scheme+host key from a full URL (simple, no dep on a URL parser).
static std::string _host_key(const std::string& url) {
    auto s = url.find("//");
    if (s == std::string::npos) return url;
    s += 2;
    auto e = url.find('/', s);
    return url.substr(0, e == std::string::npos ? url.size() : e);
}

// Return true if we should fire a new solve, false if already solved/in-flight.
static bool _claim(const std::string& url) {
    std::lock_guard<std::mutex> lk(s_mu);
    std::string key = _host_key(url);
    if (s_in_flight.count(url) || s_done_hosts.count(key)) return false;
    s_in_flight.insert(url);
    return true;
}
static void _release(const std::string& url, bool got_cookies) {
    std::lock_guard<std::mutex> lk(s_mu);
    s_in_flight.erase(url);
    if (got_cookies) s_done_hosts.insert(_host_key(url));
}

// ── Core libcurl GET helper ───────────────────────────────────────────────
static std::string _curl_get(const std::string& endpoint_url, int timeout_sec) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[cf_client] curl_easy_init failed\n");
        return {};
    }
    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL,            endpoint_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  _write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        (long)timeout_sec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L); // safe in multithreaded apps
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        fprintf(stderr, "[cf_client] curl error: %s\n", curl_easy_strerror(rc));
    }
    curl_easy_cleanup(curl);
    return body;
}

// ── URL-encode a string for use as a query parameter value ───────────────
static std::string _url_encode(const std::string& raw) {
    CURL* curl = curl_easy_init();
    if (!curl) return raw;
    char* enc = curl_easy_escape(curl, raw.c_str(), (int)raw.size());
    std::string result = enc ? enc : raw;
    if (enc) curl_free(enc);
    curl_easy_cleanup(curl);
    return result;
}

// ── Public API ────────────────────────────────────────────────────────────

bool cf_client_bridge_alive() {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL,            "http://127.0.0.1:9925/health");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  _write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);
    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK && !body.empty();
}

std::string cf_client_solve_sync(const std::string& url, int timeout_sec) {
    std::string end = "http://127.0.0.1:9925/solve?url=" + _url_encode(url);
    fprintf(stderr, "[cf_client] solve_sync -> %s\n", url.c_str());

    auto t0   = std::chrono::steady_clock::now();
    std::string resp = _curl_get(end, timeout_sec + 5);
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();

    if (resp.empty()) {
        fprintf(stderr, "[cf_client] solve_sync: no response after %.1fs (bridge running?)\n",
                elapsed);
        return {};
    }
    fprintf(stderr, "[cf_client] solve_sync: got %zu bytes in %.1fs\n",
            resp.size(), elapsed);

    // Extract cookie array and relay it to WKWebsiteDataStore.
    std::string arr = _extract_cookies_array(resp);
    if (arr.empty() || arr == "[]") {
        fprintf(stderr, "[cf_client] solve_sync: no cookies in response\n");
    } else {
        fprintf(stderr, "[cf_client] inject %zu-byte cookie array\n", arr.size());
        webview_inject_cookies(arr);
    }
    return resp;
}

void cf_client_solve_async(const std::string& url) {
    if (!_claim(url)) {
        fprintf(stderr, "[cf_client] already solved/in-flight for %s -- skipping\n",
                url.c_str());
        return;
    }
    std::string url_copy = url;
    std::thread([url_copy]() {
        std::string resp = cf_client_solve_sync(url_copy);
        bool got = !_extract_cookies_array(resp).empty();
        _release(url_copy, got);
    }).detach();
}
