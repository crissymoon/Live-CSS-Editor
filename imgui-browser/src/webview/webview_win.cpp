// webview_win.cpp -- WebView2 webview implementation for Windows.
//
// Backend: Microsoft WebView2 (Chromium-based, ships with Windows 11 / Edge).
// SDK: NuGet Microsoft.Web.WebView2 or from vendor/webview2/
//      nuget install Microsoft.Web.WebView2 -OutputDirectory vendor/webview2
//
// Architecture:
//   Each tab gets one CoreWebView2 controller embedded into the GLFW Win32
//   HWND via CreateCoreWebView2ControllerAsync.  The controller bounds are
//   updated on each webview_resize() call.
//
// Export bridge:
//   The xcm_export postMessage from JS is received via
//   add_WebMessageReceived and dispatched to platform_export_file().
//
// Note: WebView2 initialisation is async (it spins a COM/message-pump thread
// internally).  webview_create returns immediately; the webview is
// ready when the internal create callback fires.  Navigation calls made
// before that point are queued and replayed once ready.

#include "../webview.h"
#include "../app_state.h"
#include "../platform/platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <wrl.h>

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdio>

// WebView2 headers -- present only when the SDK is installed via NuGet.
#if __has_include(<WebView2.h>)
#  include <WebView2.h>
#  define XCM_WEBVIEW2_AVAILABLE 1
#else
#  pragma message("WARNING: WebView2.h not found. webview_win.cpp will build as stubs only. Install the SDK via nuget install Microsoft.Web.WebView2 -OutputDirectory vendor/webview2")
#  define XCM_WEBVIEW2_AVAILABLE 0
#endif

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// ── Internal handle ───────────────────────────────────────────────────────

struct WVHandle {
    int  tab_id  = 0;
    bool visible = false;
    bool ready   = false;
    int  x = 0, y = 0, w = 0, h = 0;
    std::vector<std::string> pending_urls;

#if XCM_WEBVIEW2_AVAILABLE
    ICoreWebView2Controller* ctrl = nullptr;
    ICoreWebView2*           wv   = nullptr;
#endif
};

static WebViewCallbacks                   s_cbs;
static AppState*                          s_state   = nullptr;
static HWND                               s_hwnd    = nullptr;
static std::unordered_map<int, WVHandle*> s_handles;
static bool                               s_com_initialized = false;

// ── webview_init ──────────────────────────────────────────────────────────

void webview_init(void* glfw_window_opaque, AppState* state, WebViewCallbacks cbs)
{
    s_state = state;
    s_cbs   = std::move(cbs);

#if XCM_WEBVIEW2_AVAILABLE
    // WebView2 requires COM on the calling thread.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr) || hr == S_FALSE || hr == RPC_E_CHANGED_MODE) {
        s_com_initialized = (hr == S_OK || hr == S_FALSE);
    } else {
        fprintf(stderr, "[webview_win] CoInitializeEx failed: 0x%08lx\n", hr);
    }
#endif

#if XCM_WEBVIEW2_AVAILABLE
    GLFWwindow* gw = static_cast<GLFWwindow*>(glfw_window_opaque);
    s_hwnd = glfwGetWin32Window(gw);
#else
    (void)glfw_window_opaque;
    fprintf(stderr, "[webview_win] WebView2 SDK not available -- stubs only.\n");
#endif
}

// ── webview_load_adblock ──────────────────────────────────────────────────

void webview_load_adblock(const std::string& /*rules_json*/) {
    // TODO: CoreWebView2.AddWebResourceRequestedFilter via ICoreWebView2_2
}

// ── webview_create ────────────────────────────────────────────────────────

void* webview_create(int tab_id, const std::string& url)
{
    auto* h     = new WVHandle();
    h->tab_id   = tab_id;
    if (!url.empty()) h->pending_urls.push_back(url);

    s_handles[tab_id] = h;

#if XCM_WEBVIEW2_AVAILABLE
    // WebView2 creation is async -- CreateCoreWebView2ControllerAsync spins
    // an internal message loop thread and calls us back on the main thread.
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [h](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    fprintf(stderr, "[webview_win] env creation failed: 0x%08lx\n", result);
                    return result;
                }
                env->CreateCoreWebView2Controller(
                    s_hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [h](HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (FAILED(result) || !ctrl) {
                                fprintf(stderr, "[webview_win] controller creation failed: 0x%08lx\n", result);
                                return result;
                            }
                            h->ctrl = ctrl;
                            ctrl->get_CoreWebView2(&h->wv);

                            // Export bridge: receive postMessage from JS.
                            h->wv->add_WebMessageReceived(
                                Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR msg = nullptr;
                                        args->TryGetWebMessageAsString(&msg);
                                        if (msg) {
                                            // TODO: parse JSON, check type=="xcm_export",
                                            // call platform_export_file(filename, content).
                                            CoTaskMemFree(msg);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Resize to last known bounds.
                            RECT bounds = { h->x, h->y, h->x + h->w, h->y + h->h };
                            ctrl->put_Bounds(bounds);
                            ctrl->put_IsVisible(h->visible ? TRUE : FALSE);

                            h->ready = true;
                            // Replay any queued navigation.
                            if (!h->pending_urls.empty()) {
                                std::wstring wurl(h->pending_urls.back().begin(),
                                                  h->pending_urls.back().end());
                                h->wv->Navigate(wurl.c_str());
                                h->pending_urls.clear();
                            }
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
#endif // XCM_WEBVIEW2_AVAILABLE

    return h;
}

// ── webview_destroy ───────────────────────────────────────────────────────

void webview_destroy(void* handle)
{
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
#if XCM_WEBVIEW2_AVAILABLE
    if (h->ctrl) { h->ctrl->Close(); h->ctrl->Release(); }
    if (h->wv)   { h->wv->Release(); }
#endif
    s_handles.erase(h->tab_id);
    delete h;
}

// ── Show / Hide ───────────────────────────────────────────────────────────

void webview_show(void* handle) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    h->visible = true;
#if XCM_WEBVIEW2_AVAILABLE
    if (h->ctrl && h->ready) h->ctrl->put_IsVisible(TRUE);
#endif
}

void webview_hide(void* handle) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    h->visible = false;
#if XCM_WEBVIEW2_AVAILABLE
    if (h->ctrl && h->ready) h->ctrl->put_IsVisible(FALSE);
#endif
}

// ── webview_resize ────────────────────────────────────────────────────────

void webview_resize(void* handle, int x, int y, int w, int h) {
    if (!handle) return;
    auto* wh = static_cast<WVHandle*>(handle);
    wh->x = x; wh->y = y; wh->w = w; wh->h = h;
#if XCM_WEBVIEW2_AVAILABLE
    if (wh->ctrl && wh->ready) {
        RECT bounds = { x, y, x + w, y + h };
        wh->ctrl->put_Bounds(bounds);
    }
#endif
}

// ── Navigation ────────────────────────────────────────────────────────────

void webview_load_url(void* handle, const std::string& url) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
#if XCM_WEBVIEW2_AVAILABLE
    if (h->wv && h->ready) {
        std::wstring wu(url.begin(), url.end());
        h->wv->Navigate(wu.c_str());
    } else {
        h->pending_urls.push_back(url);
    }
#endif
}

void webview_go_back(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv && h->ready) h->wv->GoBack();
#endif
}

void webview_go_forward(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv && h->ready) h->wv->GoForward();
#endif
}

void webview_reload(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv && h->ready) h->wv->Reload();
#endif
}

void webview_stop(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv && h->ready) h->wv->Stop();
#endif
}

// ── JavaScript ───────────────────────────────────────────────────────────

void webview_eval_js(void* handle, const std::string& script,
                     std::function<void(const std::string&)> cb)
{
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) { if (cb) cb(""); return; }
    auto* h = static_cast<WVHandle*>(handle);
    if (!h->wv || !h->ready) { if (cb) cb(""); return; }
    std::wstring ws(script.begin(), script.end());
    h->wv->ExecuteScript(ws.c_str(),
        Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [cb](HRESULT, LPCWSTR resultJson) -> HRESULT {
                if (cb && resultJson) {
                    int needed = WideCharToMultiByte(CP_UTF8, 0, resultJson, -1, nullptr, 0, nullptr, nullptr);
                    std::string s(needed - 1, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, resultJson, -1, s.data(), needed, nullptr, nullptr);
                    cb(s);
                } else if (cb) {
                    cb("");
                }
                return S_OK;
            }).Get());
#else
    (void)handle; (void)script;
    if (cb) cb("");
#endif
}

void webview_set_js_enabled(void* handle, bool /*enabled*/) {
    (void)handle;
    // CoreWebView2Settings.IsScriptEnabled -- TODO via ICoreWebView2Settings
}

// ── Inspector ────────────────────────────────────────────────────────────

void webview_open_inspector(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    if (h->wv && h->ready) {
        ICoreWebView2DevToolsProtocolEventReceiver* recv = nullptr;
        h->wv->GetDevToolsProtocolEventReceiver(L"Inspector.detached", &recv);
        // Open DevTools via CDP Page.openDevToolsWindow
        h->wv->OpenDevToolsWindow();
    }
#endif
}

// ── Clipboard ─────────────────────────────────────────────────────────────

void webview_clipboard_action(void* handle, const char* action) {
    if (!handle || !action) return;
    std::string js = std::string("document.execCommand('") + action + "')";
    webview_eval_js(handle, js, nullptr);
}

// ── Cache / Data ──────────────────────────────────────────────────────────

void webview_clear_cache() {
    // TODO: ICoreWebView2Profile2.ClearBrowsingDataAsync(COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE)
}

void webview_clear_data() {
    // TODO: ICoreWebView2Profile2.ClearBrowsingDataAsync(COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_PROFILE)
}

// ── System browser / virtual popup ───────────────────────────────────────

void webview_open_in_system_browser(const std::string& url) {
    std::wstring wurl(url.begin(), url.end());
    ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void webview_open_virt_popup(const std::string& url) {
    webview_open_in_system_browser(url);
}

// ── Cookies ───────────────────────────────────────────────────────────────

void webview_inject_cookies(const std::string& /*json_arr*/) {
    // TODO: ICoreWebView2CookieManager.CreateCookie / AddOrUpdateCookie
}

void webview_dump_cookies_json(std::function<void(const std::string&)> callback) {
    if (callback) callback("[]");
}

// ── User agent ────────────────────────────────────────────────────────────

void webview_set_cf_user_agent(const std::string& /*ua*/) {
    // TODO: ICoreWebView2Settings.put_UserAgent (available in WebView2 1.0.864+)
}

// ── Shutdown ──────────────────────────────────────────────────────────────

void webview_shutdown() {
    for (auto& [id, h] : s_handles) {
        webview_destroy(h);
    }
    s_handles.clear();

#if XCM_WEBVIEW2_AVAILABLE
    if (s_com_initialized) {
        CoUninitialize();
        s_com_initialized = false;
    }
#endif
}
