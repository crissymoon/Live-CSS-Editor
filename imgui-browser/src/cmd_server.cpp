// cmd_server.cpp -- HTTP command API (uses cpp-httplib, header-only)
// Compatible with the existing push.sh / command payloads used by the
// Python dev-browser.  Runs on a background thread so it never blocks
// the render loop.

#define CPPHTTPLIB_OPENSSL_SUPPORT 0
#include "httplib.h"   // vendor/httplib/httplib.h
#include "cmd_server.h"
#include "app_state.h"
#include <thread>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdio>

static std::thread      s_thread;
static std::atomic_bool s_running{false};
static httplib::Server* s_srv = nullptr;

// Minimal JSON value extractor (avoids bringing in a full JSON lib).
// Looks for "key":"value" or "key":value patterns.
static std::string json_str(const std::string& body, const char* key) {
    std::string needle = std::string("\"") + key + "\":\"";
    auto pos = body.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    auto end = body.find('"', pos);
    if (end == std::string::npos) return {};
    return body.substr(pos, end - pos);
}

void cmd_server_start(AppState* state, int port) {
    s_running = true;
    s_thread = std::thread([state, port]() {
        httplib::Server srv;
        s_srv = &srv;

        // GET /ping -- health check
        srv.Get("/ping", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("pong", "text/plain");
        });

        // POST /navigate  {"url": "https://..."}
        // POST /load      (alias)
        auto nav_handler = [state](const httplib::Request& req, httplib::Response& res) {
            std::string url = json_str(req.body, "url");
            if (url.empty()) url = json_str(req.body, "href");
            if (url.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"missing url\"}", "application/json");
                return;
            }
            // URL-decode basic %XX sequences
            std::string decoded;
            for (size_t i = 0; i < url.size(); ) {
                if (url[i] == '%' && i + 2 < url.size()) {
                    char hex[3] = {url[i+1], url[i+2], 0};
                    decoded += (char)strtol(hex, nullptr, 16);
                    i += 3;
                } else {
                    decoded += url[i++];
                }
            }
            state->push_nav(-1, decoded);  // -1 = active tab
            res.set_content("{\"ok\":true}", "application/json");
        };
        srv.Post("/navigate", nav_handler);
        srv.Post("/load",     nav_handler);

        // POST /newtab {"url": "https://..."}
        srv.Post("/newtab", [state](const httplib::Request& req, httplib::Response& res) {
            std::string url = json_str(req.body, "url");
            state->push_nav(-2, url.empty() ? "about:blank" : url);  // -2 = new tab
            res.set_content("{\"ok\":true}", "application/json");
        });

        // POST /eval  {"js": "document.title"}  -- runs JS in active tab
        srv.Post("/eval", [state](const httplib::Request& req, httplib::Response& res) {
            std::string js = json_str(req.body, "js");
            if (js.empty()) { res.status = 400; return; }
            state->push_nav(-3, std::string("__eval__:") + js);
            res.set_content("{\"ok\":true}", "application/json");
        });

        // GET /status
        srv.Get("/status", [state](const httplib::Request&, httplib::Response& res) {
            Tab* t = state->current_tab();
            char buf[512];
            snprintf(buf, sizeof(buf),
                     "{\"fps_dev\":%.1f,\"fps_eng\":%.1f,\"fps_host\":%.1f,"
                     "\"url\":\"%s\",\"title\":\"%s\","
                     "\"php_ok\":%s,\"node_ok\":%s}",
                     state->fps_wkwv, state->fps_engine, state->fps_host.fps_avg(),
                     t ? t->url.c_str()   : "",
                     t ? t->title.c_str() : "",
                     state->php_server_ok  ? "true" : "false",
                     state->node_server_ok ? "true" : "false");
            res.set_content(buf, "application/json");
        });

        printf("[cmd] HTTP API listening on 127.0.0.1:%d\n", port);
        srv.listen("127.0.0.1", port);
        s_srv = nullptr;
    });
}

void cmd_server_stop() {
    s_running = false;
    if (s_srv) s_srv->stop();
    if (s_thread.joinable()) s_thread.join();
}
