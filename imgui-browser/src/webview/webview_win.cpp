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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Unknwn.h>
#include <shellapi.h>

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cstdio>

// WebView2 headers -- present only when the SDK is installed via NuGet.
#if __has_include(<WebView2.h>)
#  include <WebView2.h>
#  include <wrl.h>
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
    bool closed  = false;
    bool open_devtools_requested = false;
    std::vector<std::string> pending_urls;
    std::mutex mu;

#if XCM_WEBVIEW2_AVAILABLE
    ICoreWebView2Controller* ctrl = nullptr;
    ICoreWebView2*           wv   = nullptr;
#endif
};

static WebViewCallbacks                   s_cbs;
static AppState*                          s_state   = nullptr;
static HWND                               s_hwnd    = nullptr;
static std::unordered_map<int, std::shared_ptr<WVHandle>> s_handles;

static void apply_bottom_corner_radius(HWND parent, int x, int y, int w, int h);

static std::wstring widen(const std::string& value) {
    if (value.empty()) return {};
    int needed = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (needed <= 1) return {};
    std::wstring out((size_t)needed - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), needed);
    return out;
}

static std::string narrow(LPCWSTR wide) {
    if (!wide || !wide[0]) return {};
    int needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) return {};
    std::string out((size_t)needed - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &out[0], needed, nullptr, nullptr);
    return out;
}

static std::string discover_webview2_runtime_dir() {
    if (const char* env = std::getenv("XCM_WEBVIEW2_RUNTIME_DIR")) {
        if (GetFileAttributesA(env) != INVALID_FILE_ATTRIBUTES) return env;
    }
    if (const char* env = std::getenv("WEBVIEW2_BROWSER_EXECUTABLE_FOLDER")) {
        if (GetFileAttributesA(env) != INVALID_FILE_ATTRIBUTES) return env;
    }

    char exe[MAX_PATH] = {};
    if (!platform_exe_path(exe, sizeof(exe))) return {};

    std::vector<std::string> candidates;
    std::string dir(exe);
    auto slash = dir.find_last_of("\\/");
    if (slash != std::string::npos) dir = dir.substr(0, slash);
    candidates.push_back(dir + "\\runtime\\webview2");
    slash = dir.find_last_of("\\/");
    if (slash != std::string::npos) {
        auto parent = dir.substr(0, slash);
        candidates.push_back(parent + "\\runtime\\webview2");
        auto slash2 = parent.find_last_of("\\/");
        if (slash2 != std::string::npos) {
            candidates.push_back(parent.substr(0, slash2) + "\\runtime\\webview2");
        }
    }

    for (const auto& candidate : candidates) {
        std::string exe_path = candidate + "\\msedgewebview2.exe";
        if (GetFileAttributesA(exe_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
    }
    return {};
}

// ── webview_init ──────────────────────────────────────────────────────────

void webview_init(void* glfw_window_opaque, AppState* state, WebViewCallbacks cbs)
{
    s_state = state;
    s_cbs   = std::move(cbs);

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
    auto holder = std::make_shared<WVHandle>();
    auto* h     = holder.get();
    h->tab_id   = tab_id;
    if (!url.empty()) h->pending_urls.push_back(url);

    s_handles[tab_id] = holder;

#if XCM_WEBVIEW2_AVAILABLE
    // WebView2 creation is async -- CreateCoreWebView2ControllerAsync spins
    // an internal message loop thread and calls us back on the main thread.
    const std::wstring runtime_dir = widen(discover_webview2_runtime_dir());
    CreateCoreWebView2EnvironmentWithOptions(
        runtime_dir.empty() ? nullptr : runtime_dir.c_str(), nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [holder](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                auto* h = holder.get();
                if (FAILED(result) || !env) {
                    fprintf(stderr, "[webview_win] env creation failed: 0x%08lx\n", result);
                    return result;
                }
                env->CreateCoreWebView2Controller(
                    s_hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [holder](HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT {
                            auto* h = holder.get();
                            std::lock_guard<std::mutex> lock(h->mu);
                            if (h->closed) return E_ABORT;
                            if (FAILED(result) || !ctrl) {
                                fprintf(stderr, "[webview_win] controller creation failed: 0x%08lx\n", result);
                                return result;
                            }
                            // Callback argument lifetime ends when this lambda returns.
                            // Keep our own COM reference for subsequent calls.
                            ctrl->AddRef();
                            h->ctrl = ctrl;
                            HRESULT hr_wv = ctrl->get_CoreWebView2(&h->wv);
                            if (FAILED(hr_wv) || !h->wv) {
                                fprintf(stderr, "[webview_win] get_CoreWebView2 failed: 0x%08lx\n", hr_wv);
                                return FAILED(hr_wv) ? hr_wv : E_POINTER;
                            }

                            // Export bridge: receive postMessage from JS.
                            h->wv->add_WebMessageReceived(
                                Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        if (!args) return E_POINTER;
                                        LPWSTR msg = nullptr;
                                        args->TryGetWebMessageAsString(&msg);
                                        if (msg) {
                                            // TODO: parse JSON, check type=="xcm_export",
                                            // call platform_export_file(filename, content).
                                            CoTaskMemFree(msg);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            // ── Navigation state event handlers ──────────────
                            // These mirror the WKWebView delegates on macOS.

                            h->wv->add_NavigationStarting(
                                Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [tabId = h->tab_id](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*) -> HRESULT {
                                        if (s_cbs.on_loading)  s_cbs.on_loading(tabId, true);
                                        if (s_cbs.on_progress) s_cbs.on_progress(tabId, 0.1f);
                                        return S_OK;
                                    }).Get(), nullptr);

                            h->wv->add_NavigationCompleted(
                                Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [holder](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                                        int tabId = holder->tab_id;
                                        if (s_cbs.on_loading)  s_cbs.on_loading(tabId, false);
                                        if (s_cbs.on_progress) s_cbs.on_progress(tabId, 1.0f);
                                        if (s_cbs.on_nav_state && sender) {
                                            BOOL canBack = FALSE, canFwd = FALSE;
                                            sender->get_CanGoBack(&canBack);
                                            sender->get_CanGoForward(&canFwd);
                                            s_cbs.on_nav_state(tabId, canBack == TRUE, canFwd == TRUE);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            h->wv->add_SourceChanged(
                                Microsoft::WRL::Callback<ICoreWebView2SourceChangedEventHandler>(
                                    [tabId = h->tab_id](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
                                        if (s_cbs.on_url_change && sender) {
                                            LPWSTR uri = nullptr;
                                            sender->get_Source(&uri);
                                            if (uri) {
                                                s_cbs.on_url_change(tabId, narrow(uri));
                                                CoTaskMemFree(uri);
                                            }
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            h->wv->add_DocumentTitleChanged(
                                Microsoft::WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                                    [tabId = h->tab_id](ICoreWebView2* sender, IUnknown*) -> HRESULT {
                                        if (s_cbs.on_title_change && sender) {
                                            LPWSTR title = nullptr;
                                            sender->get_DocumentTitle(&title);
                                            if (title) {
                                                s_cbs.on_title_change(tabId, narrow(title));
                                                CoTaskMemFree(title);
                                            }
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            h->wv->add_HistoryChanged(
                                Microsoft::WRL::Callback<ICoreWebView2HistoryChangedEventHandler>(
                                    [holder](ICoreWebView2* sender, IUnknown*) -> HRESULT {
                                        if (s_cbs.on_nav_state && sender) {
                                            BOOL canBack = FALSE, canFwd = FALSE;
                                            sender->get_CanGoBack(&canBack);
                                            sender->get_CanGoForward(&canFwd);
                                            s_cbs.on_nav_state(holder->tab_id, canBack == TRUE, canFwd == TRUE);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Resize to last known bounds.
                            RECT bounds = { h->x, h->y, h->x + h->w, h->y + h->h };
                            ctrl->put_Bounds(bounds);
                            ctrl->put_IsVisible(h->visible ? TRUE : FALSE);
                            apply_bottom_corner_radius(s_hwnd, h->x, h->y, h->w, h->h);

                            // Prevent white flash before first page paint.
                            // Keep the embedded surface close to host clear color.
                            ICoreWebView2Controller2* ctrl2 = nullptr;
                            if (SUCCEEDED(ctrl->QueryInterface(IID_PPV_ARGS(&ctrl2))) && ctrl2) {
                                COREWEBVIEW2_COLOR bg{};
                                bg.A = 255;
                                bg.R = 12;
                                bg.G = 12;
                                bg.B = 16;
                                ctrl2->put_DefaultBackgroundColor(bg);
                                ctrl2->Release();
                            }

                            h->ready = true;
                            // Replay any queued navigation.
                            if (!h->pending_urls.empty()) {
                                h->wv->Navigate(widen(h->pending_urls.back()).c_str());
                                h->pending_urls.clear();
                            }
                            if (h->open_devtools_requested) {
                                h->wv->OpenDevToolsWindow();
                                h->open_devtools_requested = false;
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
    {
        std::lock_guard<std::mutex> lock(h->mu);
        h->closed = true;
#if XCM_WEBVIEW2_AVAILABLE
        if (h->wv)   { h->wv->Release(); h->wv = nullptr; }
        if (h->ctrl) { h->ctrl->Close(); h->ctrl->Release(); h->ctrl = nullptr; }
#endif
        h->pending_urls.clear();
    }
    s_handles.erase(h->tab_id);
}

// ── Show / Hide ───────────────────────────────────────────────────────────

void webview_show(void* handle) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed || h->visible) return;  // already visible — avoid redundant COM call
    h->visible = true;
#if XCM_WEBVIEW2_AVAILABLE
    if (h->ctrl && h->ready) h->ctrl->put_IsVisible(TRUE);
#endif
}

void webview_hide(void* handle) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed || !h->visible) return;  // already hidden — avoid redundant COM call
    h->visible = false;
#if XCM_WEBVIEW2_AVAILABLE
    if (h->ctrl && h->ready) h->ctrl->put_IsVisible(FALSE);
#endif
}

// ── webview_resize ────────────────────────────────────────────────────────

// Clip the WebView2 child HWND so the bottom corners are rounded.
// Creates a region that is rectangular at the top and rounded at the bottom.
static void apply_bottom_corner_radius(HWND parent, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    const int r = 10;
    // Find the direct child HWND positioned at (x, y) within the parent.
    HWND child = GetWindow(parent, GW_CHILD);
    while (child) {
        if (GetParent(child) == parent) {
            RECT wr;
            GetWindowRect(child, &wr);
            POINT tl = { wr.left, wr.top };
            ScreenToClient(parent, &tl);
            if (tl.x == x && tl.y == y) {
                HRGN rr  = CreateRoundRectRgn(0, 0, w + 1, h + 1, r * 2, r * 2);
                HRGN top = CreateRectRgn(0, 0, w + 1, r);
                CombineRgn(rr, rr, top, RGN_OR);
                SetWindowRgn(child, rr, FALSE);
                DeleteObject(top);
                break;
            }
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

void webview_resize(void* handle, int x, int y, int w, int h) {
    if (!handle) return;
    auto* wh = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(wh->mu);
    if (wh->closed) return;
    wh->x = x; wh->y = y; wh->w = w; wh->h = h;
#if XCM_WEBVIEW2_AVAILABLE
    if (wh->ctrl && wh->ready) {
        RECT bounds = { x, y, x + w, y + h };
        wh->ctrl->put_Bounds(bounds);
        apply_bottom_corner_radius(s_hwnd, x, y, w, h);
    }
#endif
}

void webview_reapply_corners() {
#if XCM_WEBVIEW2_AVAILABLE
    if (!s_hwnd || !s_state) return;
    for (auto& [id, holder] : s_handles) {
        if (!holder || holder->closed) continue;
        std::lock_guard<std::mutex> lock(holder->mu);
        if (holder->ctrl && holder->ready && holder->visible &&
            holder->w > 0 && holder->h > 0)
            apply_bottom_corner_radius(s_hwnd, holder->x, holder->y,
                                       holder->w, holder->h);
    }
#endif
}

// ── Navigation ────────────────────────────────────────────────────────────

void webview_load_url(void* handle, const std::string& url) {
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
#if XCM_WEBVIEW2_AVAILABLE
    if (h->wv && h->ready) {
        h->wv->Navigate(widen(url).c_str());
    } else {
        h->pending_urls.push_back(url);
    }
#endif
}

void webview_go_back(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
    if (h->wv && h->ready) h->wv->GoBack();
#endif
}

void webview_go_forward(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
    if (h->wv && h->ready) h->wv->GoForward();
#endif
}

void webview_reload(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
    if (h->wv && h->ready) h->wv->Reload();
#endif
}

void webview_stop(void* handle) {
#if XCM_WEBVIEW2_AVAILABLE
    if (!handle) return;
    auto* h = static_cast<WVHandle*>(handle);
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
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
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) { if (cb) cb(""); return; }
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
    std::lock_guard<std::mutex> lock(h->mu);
    if (h->closed) return;
    if (h->wv && h->ready) {
        h->wv->OpenDevToolsWindow();
        h->open_devtools_requested = false;
    } else {
        h->open_devtools_requested = true;
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
    for (auto& kv : s_handles) {
    auto h = kv.second;
        if (!h) continue;
    std::lock_guard<std::mutex> lock(h->mu);
    h->closed = true;
#if XCM_WEBVIEW2_AVAILABLE
        if (h->wv)   { h->wv->Release(); h->wv = nullptr; }
        if (h->ctrl) { h->ctrl->Close(); h->ctrl->Release(); h->ctrl = nullptr; }
#endif
    h->pending_urls.clear();
    }
    s_handles.clear();
}
