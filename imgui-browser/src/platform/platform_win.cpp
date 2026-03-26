// platform_win.cpp -- Windows platform implementation.
// GLFW handles the event loop and window creation.
// WebView2 is wired through webview_win.cpp.
// ImGui chrome (chrome.cpp) is used in place of native NSView chrome.

#include "platform.h"
#include "../app_state.h"
#include "../top-of-gui/chrome.h"
#include "../webview.h"

#include "imgui.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shlwapi.h>     // PathRemoveFileSpecA
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#pragma comment(lib, "dwmapi.lib")

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// Forward declarations for main/ globals -- defined in main_globals.mm.
// Needed so the Win32 wndproc can drive live-resize updates while the
// GLFW render loop is blocked inside Windows' sizing modal loop.
extern AppState g_state;
extern int      g_prev_top;
extern int      g_prev_bot;
extern int      g_fb_w;
extern int      g_fb_h;
extern bool     g_resize_dirty;
void reposition_webviews(int chrome_top, int chrome_bot, int w, int h);

static int      s_bottom_h    = 0;
static bool     s_has_hover   = false;
static HWND     s_hwnd        = nullptr;
static WNDPROC  s_prev_wndproc = nullptr;
static HBRUSH   s_bg_brush    = nullptr;

static bool point_in_drag_strip(HWND hwnd, int client_x, int client_y) {
    RECT cr{};
    GetClientRect(hwnd, &cr);
    const int w = cr.right - cr.left;
    const int drag_strip_h = 30;  // match TOP_PAD; don't overlap tab area
    const int drag_left = TRAFFIC_LIGHT_W + 14;
    const int drag_right = w - 14;
    return client_y >= 0 && client_y < drag_strip_h &&
           client_x >= drag_left && client_x < drag_right;
}

// Radius for rounded window corners when not maximized (pixels).
static const int CORNER_RADIUS = 10;

// Clear any window region so DwmExtendFrameIntoClientArea's invisible resize
// border is not clipped.  Rounded corners are handled natively by
// DWMWA_WINDOW_CORNER_PREFERENCE on Windows 11+.
static void update_window_region(HWND hwnd) {
    if (!hwnd) return;
    SetWindowRgn(hwnd, nullptr, TRUE);
}

static LRESULT hit_test_non_client(HWND hwnd, LPARAM lParam) {
    RECT wr{};
    GetWindowRect(hwnd, &wr);

    const int screen_x = GET_X_LPARAM(lParam);
    const int screen_y = GET_Y_LPARAM(lParam);
    const int x = screen_x - wr.left;
    const int y = screen_y - wr.top;
    const int w = wr.right - wr.left;
    const int h = wr.bottom - wr.top;
    const bool maximized = IsZoomed(hwnd) != FALSE;
    // Respect user/system frame metrics; keep a sensible floor so edge grabs
    // stay easy in custom frameless mode.
    const int sys_frame = GetSystemMetrics(SM_CXSIZEFRAME) +
                          GetSystemMetrics(SM_CXPADDEDBORDER);
    // Slightly larger than system frame for easier edge grabbing.
    const int resize_border = maximized ? 0 : std::max(12, sys_frame + 2);

    if (resize_border > 0) {
        // Include a small range just outside the window rect so hit-testing
        // still works on the invisible DWM resize frame.
        const bool left   = (x < resize_border) && (x >= -resize_border);
        const bool right  = (x >= w - resize_border) && (x < w + resize_border);
        const bool top    = (y < resize_border) && (y >= -resize_border);
        const bool bottom = (y >= h - resize_border) && (y < h + resize_border);

        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
    }

    if (point_in_drag_strip(hwnd, x, y)) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

static LRESULT CALLBACK platform_win_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCHITTEST:
        return hit_test_non_client(hwnd, lParam);
    case WM_SETCURSOR: {
        // GLFW overrides the cursor for undecorated windows, preventing the
        // resize arrows from appearing. Handle non-client hit tests ourselves
        // so DefWindowProc can set the correct resize cursor.
        WORD ht = LOWORD(lParam);
        if (ht != HTCLIENT) {
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
        break;  // let GLFW handle client-area cursor
    }
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_NCMOUSEMOVE:
        // Route all non-client mouse messages to DefWindowProc so Windows can
        // handle resize drags, caption drags, and system menu interactions.
        // GLFW doesn't handle these for undecorated windows.
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    case WM_NCCALCSIZE:
        // Remove all non-client area: client rect = full window rect.
        // wParam==0 is a simple RECT query; returning 0 is safe for both.
        return 0;
    case WM_NCACTIVATE:
        return TRUE;
    case WM_NCPAINT:
        return 0;
    case WM_ERASEBKGND:
        // We paint the entire window via OpenGL every frame.
        // Returning 1 tells Windows the background is already erased so it
        // never fills the client area with the (possibly white) window brush.
        return 1;
    case WM_SIZE: {
        // WM_SIZE fires inside Windows' modal sizing loop while glfwPollEvents
        // is blocked.  Update our logical size and reposition the WebView2
        // controller immediately so the content tracks the resize handle live.
        if (wParam != SIZE_MINIMIZED) {
            int new_w = static_cast<int>(LOWORD(lParam));
            int new_h = static_cast<int>(HIWORD(lParam));
            if (new_w > 0 && new_h > 0 &&
                (new_w != g_state.win_w || new_h != g_state.win_h)) {
                g_state.win_w = new_w;
                g_state.win_h = new_h;
                g_fb_w = new_w;  // 1:1 on non-HiDPI; proper value restored by fb callback
                g_fb_h = new_h;
                g_resize_dirty = true;
                reposition_webviews(g_prev_top, g_prev_bot, new_w, new_h);
            }
        }
        update_window_region(hwnd);
        break;  // let GLFW also process WM_SIZE for its own bookkeeping
    }
    case WM_NCLBUTTONDBLCLK:
        // Frameless windows can lose default maximize behavior depending on
        // style/caption semantics, so force it for caption double-click.
        if (wParam == HTCAPTION) {
            if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
            else                ShowWindow(hwnd, SW_MAXIMIZE);
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    case WM_LBUTTONDBLCLK: {
        // Safety net: if Windows routes the double-click as client-area, still
        // maximize/restore when the pointer is over our custom drag strip.
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (point_in_drag_strip(hwnd, pt.x, pt.y)) {
            if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
            else                ShowWindow(hwnd, SW_MAXIMIZE);
            return 0;
        }
        break;
    }
    case WM_GETMINMAXINFO: {
        // Enforce a sensible minimum window size.
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 320;
        mmi->ptMinTrackSize.y = 200;
        // Maximized window must fill the work area (monitor minus the taskbar)
        // on whichever monitor the window is on.  Without this, returning 0 from
        // WM_NCCALCSIZE causes Windows to over-extend the window and cover the
        // taskbar, or leave a gap, depending on the DWM frame calculation.
        {
            HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            if (hmon && GetMonitorInfo(hmon, &mi)) {
                // ptMaxPosition is relative to the upper-left corner of the monitor.
                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top  - mi.rcMonitor.top;
                mmi->ptMaxSize.x     = mi.rcWork.right  - mi.rcWork.left;
                mmi->ptMaxSize.y     = mi.rcWork.bottom - mi.rcWork.top;
            }
        }
        return 0;
    }
    default:
        break;
    }

    return CallWindowProcA(s_prev_wndproc, hwnd, msg, wParam, lParam);
}

// ── App lifecycle ─────────────────────────────────────────────────────────

void platform_app_preinit() {
    // Tell Windows this process is DPI-aware so GLFW window sizes are in
    // physical pixels and the webview content is not blurry on HiDPI displays.
#if defined(WINVER) && WINVER >= 0x0605
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#else
    SetProcessDPIAware();
#endif
}

void platform_app_postinit(void* glfw_window) {
    GLFWwindow* window = static_cast<GLFWwindow*>(glfw_window);
    if (!window) return;

    s_hwnd = glfwGetWin32Window(window);
    if (!s_hwnd) return;

    // WS_POPUP = no system border at all.
    // WS_THICKFRAME = keep Windows resize-snap zones (Win11 snapping).
    // WS_MINIMIZEBOX | WS_MAXIMIZEBOX = enable Win+Arrow snap, taskbar actions.
    // Preserve WS_VISIBLE from whatever GLFW set so the window stays on screen.
    LONG_PTR old_style = GetWindowLongPtrA(s_hwnd, GWL_STYLE);
    LONG_PTR style = (WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
                   | (old_style & WS_VISIBLE);
    SetWindowLongPtrA(s_hwnd, GWL_STYLE, style);

    LONG_PTR ex_style = GetWindowLongPtrA(s_hwnd, GWL_EXSTYLE);
    ex_style &= ~static_cast<LONG_PTR>(WS_EX_CLIENTEDGE);
    SetWindowLongPtrA(s_hwnd, GWL_EXSTYLE, ex_style);

    // Let DWM handle non-client rendering so the native DWMWA_BORDER_COLOR
    // border appears around the whole window (on top of child HWNDs like WebView2).
    const DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(s_hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

    // Match DWM caption / backdrop colour to our app background.
    // glClearColor(0.047, 0.047, 0.063) ≈ RGB(12, 12, 16).
    // On Windows 11 this prevents the 1-px DWM activation border appearing white.
    // These attributes are no-ops on older Windows versions.
    COLORREF bg_col = RGB(12, 12, 16);
    COLORREF border_col = RGB(170, 178, 214);
    DwmSetWindowAttribute(s_hwnd, 35 /* DWMWA_CAPTION_COLOR */,    &bg_col, sizeof(bg_col));
    DwmSetWindowAttribute(s_hwnd, 36 /* DWMWA_TEXT_COLOR */,       &bg_col, sizeof(bg_col));
    DwmSetWindowAttribute(s_hwnd, 34 /* DWMWA_BORDER_COLOR */,     &border_col, sizeof(border_col));
    BOOL dark = TRUE;
    DwmSetWindowAttribute(s_hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    DwmSetWindowAttribute(s_hwnd, 19 /* pre-RTM dark mode attr */,         &dark, sizeof(dark));

    // Extend the DWM frame into the client area so Windows provides the
    // invisible resize border beyond the window edges and the drop shadow.
    // A bottom margin of 1px is the minimum needed to enable both features.
    MARGINS dwm_margins = { 0, 0, 0, 1 };
    DwmExtendFrameIntoClientArea(s_hwnd, &dwm_margins);

    // On Windows 11 (build 22000+), request rounded corners natively.
    // Attribute 33 = DWMWA_WINDOW_CORNER_PREFERENCE, value 2 = DWMWCP_ROUND.
    DWORD corner_pref = 2;
    DwmSetWindowAttribute(s_hwnd, 33, &corner_pref, sizeof(corner_pref));

    // Replace the GLFW window-class brush with one that matches our background.
    // This prevents Windows from flashing a white rectangle on the very first
    // WM_ERASEBKGND that fires before our OpenGL frame completes.
    HBRUSH new_brush = CreateSolidBrush(bg_col);
    HBRUSH old_brush = reinterpret_cast<HBRUSH>(
        SetClassLongPtrA(s_hwnd, GCLP_HBRBACKGROUND,
                         reinterpret_cast<LONG_PTR>(new_brush)));
    if (s_bg_brush) DeleteObject(s_bg_brush);
    if (old_brush)  DeleteObject(old_brush);
    s_bg_brush = new_brush;

    SetWindowPos(s_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

    if (!s_prev_wndproc) {
        s_prev_wndproc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(platform_win_wndproc)));
    }

    // Apply initial rounded corners (the wndproc is now in place to handle
    // future WM_SIZE updates).
    update_window_region(s_hwnd);
}

void platform_app_cleanup() {
    if (s_hwnd && s_prev_wndproc) {
        SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(s_prev_wndproc));
        s_prev_wndproc = nullptr;
    }
    s_hwnd = nullptr;
    if (s_bg_brush) { DeleteObject(s_bg_brush); s_bg_brush = nullptr; }
    // WebView2 COM teardown is handled by webview_win.cpp.
}

// ── Executable path ───────────────────────────────────────────────────────

bool platform_exe_path(char* out, size_t size) {
    DWORD written = GetModuleFileNameA(nullptr, out, (DWORD)size);
    return written > 0 && written < (DWORD)size;
}

// ── Native menu bar ───────────────────────────────────────────────────────

void platform_menu_init() {
    // No system-level menu bar on Windows.
    // The ImGui toolbar in chrome.cpp provides all user-facing controls.
    // A Win32 HMENU could be installed here in the future if needed.
}

// ── Native chrome ─────────────────────────────────────────────────────────
// Windows uses the ImGui chrome layer (chrome.cpp) rather than a native toolkit.
// These functions are no-ops; chrome.cpp draws directly into the GLFW window.

void platform_chrome_create(void* glfw_window, AppState* state, int php_port) {
    (void)glfw_window; (void)state; (void)php_port;
}

int platform_chrome_update(AppState* state) {
    bool new_tab_requested = false;
    int close_tab_idx = -1;
    int top_h = chrome_draw_top(state, state->win_w, state->win_h,
                                new_tab_requested, close_tab_idx);
    s_bottom_h = chrome_draw_bottom(state, state->win_w, state->win_h);
    s_has_hover = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
                  ImGui::IsAnyItemHovered();

    if (new_tab_requested) {
        int idx = state->new_tab("about:blank");
        state->tabs[idx].wv_handle = webview_create(state->tabs[idx].id, "about:blank");
    }

    if (close_tab_idx >= 0 && close_tab_idx < (int)state->tabs.size()) {
        if (state->tabs[close_tab_idx].wv_handle) {
            webview_destroy(state->tabs[close_tab_idx].wv_handle);
            state->tabs[close_tab_idx].wv_handle = nullptr;
        }
        state->close_tab(close_tab_idx);
    }

    return top_h;
}

int platform_chrome_status_h() {
    return s_bottom_h;
}

bool platform_chrome_has_hover() {
    return s_has_hover;
}

void platform_chrome_resize(int win_w, int win_h) {
    (void)win_w; (void)win_h;
}

void platform_chrome_focus_url() {
    // TODO: set ImGui keyboard focus to the URL input widget in chrome.cpp.
}

void platform_chrome_destroy() {}

// ── Export bridge ─────────────────────────────────────────────────────────

void platform_export_file(const std::string& filename, const std::string& content) {
    char exe[MAX_PATH] = {};
    if (!platform_exe_path(exe, sizeof(exe))) {
        fprintf(stderr, "[platform_win] could not resolve exe path\n");
        return;
    }
    // Strip the filename from the path to get the directory.
    PathRemoveFileSpecA(exe);
    std::string path = std::string(exe) + "\\" + filename;

    std::ofstream f(path);
    if (f) {
        f << content;
        fprintf(stderr, "[platform_win] exported: %s\n", path.c_str());
    } else {
        fprintf(stderr, "[platform_win] export failed: %s\n", path.c_str());
    }
}
