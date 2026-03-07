// webview_linux.cpp -- WebKitGTK webview implementation for Linux.
//
// Backend: webkit2gtk-4.1 (same WebKit engine as WKWebView on macOS).
// Install: sudo apt install libwebkit2gtk-4.1-dev
//
// Architecture:
//   Each tab gets one GtkOffscreenWindow hosting a WebKitWebView.
//   The rendered frame is composited into the GLFW/OpenGL window via
//   a GdkPixbuf -> OpenGL texture blit each frame.
//
//   WVHandle holds the GtkWidget* and WebKitWebView* as void* so
//   webview.h stays pure C++ with no GTK headers in the public interface.
//
// Export bridge:
//   The xcm_export postMessage from JS is received by a
//   WebKitUserMessageHandler ("xcmBridge") and dispatched to
//   platform_export_file().

#include "../webview.h"
#include "../app_state.h"
#include "../platform/platform.h"

#include <webkit2/webkit2.h>
#include <gtk/gtk.h>

#include <string>
#include <functional>
#include <unordered_map>
#include <cstdio>

// ── Internal handle ───────────────────────────────────────────────────────

struct WVHandle {
    int               tab_id  = 0;
    WebKitWebView*    wv      = nullptr;
    GtkWidget*        win     = nullptr;   // GtkOffscreenWindow
    bool              visible = false;
    int x = 0, y = 0, w = 0, h = 0;
};

static WebViewCallbacks          s_cbs;
static AppState*                 s_state  = nullptr;
static WebKitUserContentManager* s_ucm    = nullptr;
static std::unordered_map<int, WVHandle*> s_handles;

// ── Script message handler (export bridge) ────────────────────────────────

static void on_script_message(WebKitUserContentManager*,
                               WebKitJavascriptResult* result,
                               gpointer)
{
    // Parse the JSON payload posted by dispatchExport() in php-wasm.js.
    JSCValue* val = webkit_javascript_result_get_js_value(result);
    if (!jsc_value_is_object(val)) return;

    JSCValue* type_v     = jsc_value_object_get_property(val, "type");
    JSCValue* filename_v = jsc_value_object_get_property(val, "filename");
    JSCValue* content_v  = jsc_value_object_get_property(val, "content");

    char* type     = jsc_value_to_string(type_v);
    char* filename = jsc_value_to_string(filename_v);
    char* content  = jsc_value_to_string(content_v);

    if (type && std::string(type) == "xcm_export") {
        platform_export_file(filename ? filename : "export.css",
                             content  ? content  : "");
    }

    g_free(type); g_free(filename); g_free(content);
}

// ── webview_init ──────────────────────────────────────────────────────────

void webview_init(void* /*ns_window*/, AppState* state, WebViewCallbacks cbs)
{
    // gtk_init may have already been called; calling it again is safe.
    if (!gtk_is_initialized()) {
        int argc = 0; char** argv = nullptr;
        gtk_init(&argc, &argv);
    }

    s_state = state;
    s_cbs   = std::move(cbs);

    // Shared user content manager across all tabs -- one xcmBridge handler.
    s_ucm = webkit_user_content_manager_new();
    g_signal_connect(s_ucm, "script-message-received::xcmBridge",
                     G_CALLBACK(on_script_message), nullptr);
    webkit_user_content_manager_register_script_message_handler(s_ucm, "xcmBridge");
}

// ── webview_load_adblock ──────────────────────────────────────────────────

void webview_load_adblock(const std::string& /*rules_json*/) {
    // WebKit2GTK uses WebKitUserContentFilter for content blocking.
    // TODO: convert the WKContentRuleList JSON to a UserContentFilter here.
}

// ── webview_create ────────────────────────────────────────────────────────

void* webview_create(int tab_id, const std::string& url)
{
    auto* h = new WVHandle();
    h->tab_id = tab_id;

    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_javascript_enabled(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, TRUE);

    h->wv  = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(s_ucm));
    webkit_web_view_set_settings(h->wv, settings);
    g_object_unref(settings);

    // Load URL into the offscreen WebKitWebView.
    if (!url.empty()) {
        webkit_web_view_load_uri(h->wv, url.c_str());
    }

    s_handles[tab_id] = h;
    return h;
}

// ── webview_destroy ───────────────────────────────────────────────────────

void webview_destroy(void* handle)
{
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv) g_object_unref(h->wv);
    s_handles.erase(h->tab_id);
    delete h;
}

// ── Show / Hide ───────────────────────────────────────────────────────────

void webview_show(void* handle) {
    if (!handle) return;
    static_cast<WVHandle*>(handle)->visible = true;
}

void webview_hide(void* handle) {
    if (!handle) return;
    static_cast<WVHandle*>(handle)->visible = false;
}

// ── webview_resize ────────────────────────────────────────────────────────

void webview_resize(void* handle, int x, int y, int w, int h) {
    if (!handle) return;
    auto* wh = static_cast<WVHandle*>(handle);
    wh->x = x; wh->y = y; wh->w = w; wh->h = h;
    webkit_web_view_set_size_request(wh->wv, w, h);
}

// ── Navigation ────────────────────────────────────────────────────────────

void webview_load_url(void* handle, const std::string& url) {
    if (!handle) return;
    webkit_web_view_load_uri(static_cast<WVHandle*>(handle)->wv, url.c_str());
}

void webview_go_back(void* handle) {
    if (!handle) return;
    webkit_web_view_go_back(static_cast<WVHandle*>(handle)->wv);
}

void webview_go_forward(void* handle) {
    if (!handle) return;
    webkit_web_view_go_forward(static_cast<WVHandle*>(handle)->wv);
}

void webview_reload(void* handle) {
    if (!handle) return;
    webkit_web_view_reload(static_cast<WVHandle*>(handle)->wv);
}

void webview_stop(void* handle) {
    if (!handle) return;
    webkit_web_view_stop_loading(static_cast<WVHandle*>(handle)->wv);
}

// ── JavaScript ───────────────────────────────────────────────────────────

void webview_eval_js(void* handle, const std::string& script,
                     std::function<void(const std::string&)> cb)
{
    if (!handle) { if (cb) cb(""); return; }
    auto* wh = static_cast<WVHandle*>(handle);

    struct Ctx { std::function<void(const std::string&)> cb; };
    auto* ctx = new Ctx{cb};

    webkit_web_view_evaluate_javascript(
        wh->wv, script.c_str(), -1, nullptr, nullptr, nullptr,
        [](GObject* src, GAsyncResult* res, gpointer user) {
            auto* c = static_cast<Ctx*>(user);
            GError* err = nullptr;
            JSCValue* val = webkit_web_view_evaluate_javascript_finish(
                WEBKIT_WEB_VIEW(src), res, &err);
            std::string result;
            if (val) {
                char* s = jsc_value_to_string(val);
                if (s) { result = s; g_free(s); }
                g_object_unref(val);
            }
            if (err) g_error_free(err);
            if (c->cb) c->cb(result);
            delete c;
        }, ctx);
}

void webview_set_js_enabled(void* handle, bool enabled) {
    if (!handle) return;
    auto* wh = static_cast<WVHandle*>(handle);
    WebKitSettings* s = webkit_web_view_get_settings(wh->wv);
    webkit_settings_set_javascript_enabled(s, enabled ? TRUE : FALSE);
    webkit_web_view_reload(wh->wv);
}

// ── Inspector ────────────────────────────────────────────────────────────

void webview_open_inspector(void* handle) {
    if (!handle) return;
    WebKitWebInspector* insp = webkit_web_view_get_inspector(
        static_cast<WVHandle*>(handle)->wv);
    webkit_web_inspector_show(insp);
}

// ── Clipboard ─────────────────────────────────────────────────────────────

void webview_clipboard_action(void* handle, const char* action) {
    if (!handle || !action) return;
    // Dispatch as JS execCommand fallback.
    std::string js = std::string("document.execCommand('") + action + "')";
    webview_eval_js(handle, js, nullptr);
}

// ── Cache / Data ──────────────────────────────────────────────────────────

void webview_clear_cache() {
    // TODO: webkit_web_context_clear_cache(webkit_web_context_get_default());
}

void webview_clear_data() {
    WebKitWebsiteDataManager* dm =
        webkit_web_context_get_website_data_manager(
            webkit_web_context_get_default());
    webkit_website_data_manager_clear(dm,
        WEBKIT_WEBSITE_DATA_ALL, 0, nullptr, nullptr, nullptr);
}

// ── System browser / virtual popup ───────────────────────────────────────

void webview_open_in_system_browser(const std::string& url) {
    std::string cmd = "xdg-open '" + url + "' &";
    (void)system(cmd.c_str());
}

void webview_open_virt_popup(const std::string& url) {
    // TODO: open a floating GtkWindow with a WebKitWebView for OAuth flows.
    webview_open_in_system_browser(url);
}

// ── Cookies ───────────────────────────────────────────────────────────────

void webview_inject_cookies(const std::string& /*json_arr*/) {
    // TODO: parse JSON and call webkit_cookie_manager_add_cookie()
}

void webview_dump_cookies_json(std::function<void(const std::string&)> callback) {
    // TODO: webkit_cookie_manager_get_cookies()
    if (callback) callback("[]");
}

// ── User agent ────────────────────────────────────────────────────────────

void webview_set_cf_user_agent(const std::string& ua) {
    for (auto& [id, h] : s_handles) {
        if (!h || !h->wv) continue;
        WebKitSettings* s = webkit_web_view_get_settings(h->wv);
        webkit_settings_set_user_agent(s, ua.c_str());
    }
}

// ── Shutdown ──────────────────────────────────────────────────────────────

void webview_shutdown() {
    for (auto& [id, h] : s_handles) {
        if (h) { webview_destroy(h); }
    }
    s_handles.clear();
    if (s_ucm) { g_object_unref(s_ucm); s_ucm = nullptr; }
}
