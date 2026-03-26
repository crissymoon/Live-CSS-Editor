// chrome_traffic_lights.cpp -- Native GDI+ traffic light overlay (Windows).
//
// A small WS_CHILD window sits in the top-left corner of the main HWND.
// It handles its own WM_PAINT (GDI+ anti-aliased circles + line glyphs),
// WM_MOUSEMOVE / WM_MOUSELEAVE (hover tracking), and WM_LBUTTONDOWN
// (close / minimize / maximize).  Completely independent from ImGui/OpenGL
// -- no z-order issues, no float-precision pipeline quirks.

#ifndef __APPLE__
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>     // GET_X_LPARAM, GET_Y_LPARAM
#include <objidl.h>
#include <gdiplus.h>

#include "chrome_traffic_lights.h"
#include "app_state.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cmath>
#include <algorithm>

// ── Module state ──────────────────────────────────────────────────────
static HWND          s_hwnd         = nullptr;
static HWND          s_parent       = nullptr;
static AppState*     s_state        = nullptr;
static ULONG_PTR     s_gdiplus_tok  = 0;
static int           s_hover        = -1;
static bool          s_tracking     = false;

// Layout -- device pixels, matching ImGui coordinate system directly.
// On DPI-aware Windows, ImGui coords = device pixels (FontGlobalScale=1.0),
// so these values must NOT be multiplied by dpi_scale for HWND sizing.
static constexpr int TL_W     = 82;   // TRAFFIC_LIGHT_W
static constexpr int TL_H     = 30;   // TOP_PAD in chrome_tab_row.cpp
static constexpr float BASE_R   = 7.0f;
static constexpr float BASE_HIT = 9.0f;
static constexpr float BASE_X0  = 8.0f;
static constexpr float BASE_GAP = 4.0f;
static constexpr float BASE_SZ  = 3.0f;

// Circle colours (close=red, min=yellow, zoom=green)
static const Gdiplus::Color COLS[3] = {
    Gdiplus::Color(255, 248, 135, 115),
    Gdiplus::Color(255, 251, 192,  66),
    Gdiplus::Color(255,  52, 211, 153),
};

// ── Helpers ───────────────────────────────────────────────────────────
// All coordinates are in device pixels (no DPI multiplication needed).
static float btn_cx(int i) {
    return BASE_X0 + BASE_HIT + (BASE_HIT * 2.0f + BASE_GAP) * (float)i;
}
static float btn_cy() { return (float)TL_H * 0.5f; }

static int hit_test(int px, int py) {
    float cy = btn_cy();
    for (int i = 0; i < 3; ++i) {
        float cx = btn_cx(i);
        float dx = (float)px - cx;
        float dy = (float)py - cy;
        if (dx*dx + dy*dy <= BASE_HIT * BASE_HIT) return i;
    }
    return -1;
}

// ── WndProc ───────────────────────────────────────────────────────────
static LRESULT CALLBACK tl_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        // Double-buffer to offscreen bitmap
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

        {
            Gdiplus::Graphics gfx(mem);
            gfx.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

            // Background: COL_SURFACE = RGB(16, 16, 24)
            Gdiplus::SolidBrush bg(Gdiplus::Color(255, 16, 16, 24));
            gfx.FillRectangle(&bg, 0, 0, w, h);

            float r  = BASE_R;
            float cy = btn_cy();

            for (int i = 0; i < 3; ++i) {
                float cx  = btn_cx(i);
                bool  hov = (s_hover == i);

                // Circle fill (slightly brighter on hover)
                Gdiplus::Color fc = COLS[i];
                if (hov) {
                    fc = Gdiplus::Color(255,
                        (std::min)(255, (int)fc.GetR() + 18),
                        (std::min)(255, (int)fc.GetG() + 18),
                        (std::min)(255, (int)fc.GetB() + 18));
                }
                Gdiplus::SolidBrush cb(fc);
                gfx.FillEllipse(&cb, cx - r, cy - r, r * 2.0f, r * 2.0f);

                // Subtle dark ring
                int ra = hov ? 100 : 55;
                Gdiplus::Pen ring(Gdiplus::Color(ra, 8, 8, 13), 1.0f);
                gfx.DrawEllipse(&ring, cx - r, cy - r, r * 2.0f, r * 2.0f);

                // Glyph lines -- always visible (faint idle, strong hover)
                int ga = hov ? 220 : 70;
                Gdiplus::Pen gp(Gdiplus::Color(ga, 10, 10, 12), 1.2f);
                gp.SetLineCap(Gdiplus::LineCapRound,
                              Gdiplus::LineCapRound,
                              Gdiplus::DashCapRound);
                float sz = BASE_SZ;

                if (i == 0) { // x -- close
                    gfx.DrawLine(&gp, cx - sz, cy - sz, cx + sz, cy + sz);
                    gfx.DrawLine(&gp, cx + sz, cy - sz, cx - sz, cy + sz);
                } else if (i == 1) { // - -- minimize
                    gfx.DrawLine(&gp, cx - sz, cy, cx + sz, cy);
                } else { // + -- zoom
                    gfx.DrawLine(&gp, cx - sz, cy, cx + sz, cy);
                    gfx.DrawLine(&gp, cx, cy - sz, cx, cy + sz);
                }
            }
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int prev = s_hover;
        s_hover = hit_test(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (s_hover != prev) InvalidateRect(hwnd, nullptr, FALSE);
        if (!s_tracking) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize    = sizeof(tme);
            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            s_tracking = true;
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        s_tracking = false;
        if (s_hover != -1) { s_hover = -1; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;

    case WM_LBUTTONDOWN: {
        int btn = hit_test(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (btn == 0) {
            GLFWwindow* gw = glfwGetCurrentContext();
            if (gw) glfwSetWindowShouldClose(gw, GLFW_TRUE);
        } else if (btn == 1) {
            ShowWindow(s_parent, SW_MINIMIZE);
        } else if (btn == 2) {
            if (IsZoomed(s_parent)) ShowWindow(s_parent, SW_RESTORE);
            else                    ShowWindow(s_parent, SW_MAXIMIZE);
        }
        return 0;
    }

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        if (hit_test(pt.x, pt.y) >= 0) return HTCLIENT;
        return HTCAPTION;   // allow drag through non-button area
    }

    default: break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// ── Public API ────────────────────────────────────────────────────────
void chrome_tl_create(void* parent_hwnd, AppState* st) {
    s_parent = (HWND)parent_hwnd;
    s_state  = st;

    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&s_gdiplus_tok, &gsi, nullptr);

    static bool reg = false;
    if (!reg) {
        WNDCLASSEXA wc = {};
        wc.cbSize       = sizeof(wc);
        wc.style        = CS_OWNDC;
        wc.lpfnWndProc  = tl_wndproc;
        wc.hInstance     = GetModuleHandleA(nullptr);
        wc.hCursor      = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "ChrTrafficLights";
        RegisterClassExA(&wc);
        reg = true;
    }

    s_hwnd = CreateWindowExA(0, "ChrTrafficLights", nullptr,
                             WS_CHILD | WS_VISIBLE,
                             0, 0, TL_W, TL_H,
                             s_parent, nullptr,
                             GetModuleHandleA(nullptr), nullptr);
}

void chrome_tl_update() {
    if (!s_hwnd) return;
    // Repaint every frame so the overlay stays in sync
    InvalidateRect(s_hwnd, nullptr, FALSE);
}

void chrome_tl_destroy() {
    if (s_hwnd) { DestroyWindow(s_hwnd); s_hwnd = nullptr; }
    if (s_gdiplus_tok) { Gdiplus::GdiplusShutdown(s_gdiplus_tok); s_gdiplus_tok = 0; }
}

#else
// Linux stub
#include "chrome_traffic_lights.h"
void chrome_tl_create(void*, AppState*) {}
void chrome_tl_update() {}
void chrome_tl_destroy() {}
#endif // _WIN32
#endif // !__APPLE__