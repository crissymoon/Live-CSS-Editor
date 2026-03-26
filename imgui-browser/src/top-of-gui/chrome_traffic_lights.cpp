// chrome_traffic_lights.cpp -- GDI+ traffic lights painted directly on parent HDC.
//
// No child HWND is created.  After each glfwSwapBuffers, chrome_tl_update()
// paints circles + glyphs onto the parent window's front buffer via GDI+.
// This sidesteps the OpenGL pipeline entirely (no z-order / float issues)
// and avoids the child-over-OpenGL flicker that a WS_CHILD approach causes.
//
// Clicks are intercepted by the parent wndproc calling chrome_tl_try_click().

#ifndef __APPLE__
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>

#include "chrome_traffic_lights.h"
#include "app_state.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>

// ── State ─────────────────────────────────────────────────────────────
static HWND      s_parent      = nullptr;
static AppState* s_state       = nullptr;
static ULONG_PTR s_gdiplus_tok = 0;
static HDC       s_memdc       = nullptr;
static HBITMAP   s_bmp         = nullptr;
static HBITMAP   s_oldbmp      = nullptr;
static int       s_hover       = -1;

// Layout (device pixels -- matches ImGui coordinate system 1:1)
static constexpr int   TL_W     = 82;
static constexpr int   TL_H     = 30;
static constexpr float BASE_R   = 7.0f;
static constexpr float BASE_HIT = 9.0f;
static constexpr float BASE_X0  = 8.0f;
static constexpr float BASE_GAP = 4.0f;
static constexpr float BASE_SZ  = 3.0f;

static const Gdiplus::Color COLS[3] = {
    Gdiplus::Color(255, 248, 135, 115),   // close  (red)
    Gdiplus::Color(255, 251, 192,  66),   // min    (yellow)
    Gdiplus::Color(255,  52, 211, 153),   // zoom   (green)
};

// ── Helpers ───────────────────────────────────────────────────────────
static float btn_cx(int i) {
    return BASE_X0 + BASE_HIT + (BASE_HIT * 2.0f + BASE_GAP) * (float)i;
}
static float btn_cy() { return (float)TL_H * 0.5f; }

static int hit_test(int px, int py) {
    if (px < 0 || px >= TL_W || py < 0 || py >= TL_H) return -1;
    float cy = btn_cy();
    for (int i = 0; i < 3; ++i) {
        float cx = btn_cx(i);
        float dx = (float)px - cx;
        float dy = (float)py - cy;
        if (dx * dx + dy * dy <= BASE_HIT * BASE_HIT) return i;
    }
    return -1;
}

static void paint_to_memdc() {
    Gdiplus::Graphics gfx(s_memdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    // Background must match the surface behind the traffic lights.
    // COL_SURFACE = rgba(0.063, 0.063, 0.094, 1) = RGB(16, 16, 24)
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 16, 16, 24));
    gfx.FillRectangle(&bg, 0, 0, TL_W, TL_H);

    float r  = BASE_R;
    float cy = btn_cy();

    for (int i = 0; i < 3; ++i) {
        float cx  = btn_cx(i);
        bool  hov = (s_hover == i);

        // Filled circle
        Gdiplus::Color fc = COLS[i];
        if (hov) {
            fc = Gdiplus::Color(255,
                (std::min)(255, (int)fc.GetR() + 18),
                (std::min)(255, (int)fc.GetG() + 18),
                (std::min)(255, (int)fc.GetB() + 18));
        }
        Gdiplus::SolidBrush cb(fc);
        gfx.FillEllipse(&cb, cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Dark ring
        int ra = hov ? 100 : 55;
        Gdiplus::Pen ring(Gdiplus::Color(ra, 8, 8, 13), 1.0f);
        gfx.DrawEllipse(&ring, cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Glyph lines (always visible; stronger on hover)
        int ga = hov ? 220 : 70;
        Gdiplus::Pen gp(Gdiplus::Color(ga, 10, 10, 12), 1.2f);
        gp.SetLineCap(Gdiplus::LineCapRound,
                      Gdiplus::LineCapRound,
                      Gdiplus::DashCapRound);
        float sz = BASE_SZ;

        if (i == 0) {
            gfx.DrawLine(&gp, cx - sz, cy - sz, cx + sz, cy + sz);
            gfx.DrawLine(&gp, cx + sz, cy - sz, cx - sz, cy + sz);
        } else if (i == 1) {
            gfx.DrawLine(&gp, cx - sz, cy, cx + sz, cy);
        } else {
            gfx.DrawLine(&gp, cx - sz, cy, cx + sz, cy);
            gfx.DrawLine(&gp, cx, cy - sz, cx, cy + sz);
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────
void chrome_tl_create(void* parent_hwnd, AppState* st) {
    s_parent = (HWND)parent_hwnd;
    s_state  = st;

    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&s_gdiplus_tok, &gsi, nullptr);

    // Persistent offscreen bitmap (avoids per-frame allocation)
    HDC screenDC = GetDC(nullptr);
    s_memdc  = CreateCompatibleDC(screenDC);
    s_bmp    = CreateCompatibleBitmap(screenDC, TL_W, TL_H);
    s_oldbmp = (HBITMAP)SelectObject(s_memdc, s_bmp);
    ReleaseDC(nullptr, screenDC);
}

void chrome_tl_update() {
    if (!s_parent || !s_memdc) return;

    // Update hover from cursor position
    POINT cur;
    GetCursorPos(&cur);
    ScreenToClient(s_parent, &cur);
    s_hover = hit_test(cur.x, cur.y);

    // Paint to the offscreen bitmap
    paint_to_memdc();

    // Blit to the parent's front buffer
    HDC hdc = GetDC(s_parent);
    BitBlt(hdc, 0, 0, TL_W, TL_H, s_memdc, 0, 0, SRCCOPY);
    ReleaseDC(s_parent, hdc);
}

bool chrome_tl_try_click(int client_x, int client_y) {
    int btn = hit_test(client_x, client_y);
    if (btn < 0) return false;

    if (btn == 0) {
        GLFWwindow* gw = glfwGetCurrentContext();
        if (gw) glfwSetWindowShouldClose(gw, GLFW_TRUE);
    } else if (btn == 1) {
        ShowWindow(s_parent, SW_MINIMIZE);
    } else if (btn == 2) {
        if (IsZoomed(s_parent)) ShowWindow(s_parent, SW_RESTORE);
        else                    ShowWindow(s_parent, SW_MAXIMIZE);
    }
    return true;
}

bool chrome_tl_hit(int client_x, int client_y) {
    return hit_test(client_x, client_y) >= 0;
}

void chrome_tl_destroy() {
    if (s_memdc) {
        SelectObject(s_memdc, s_oldbmp);
        DeleteObject(s_bmp);
        DeleteDC(s_memdc);
        s_memdc = nullptr;
        s_bmp = nullptr;
    }
    if (s_gdiplus_tok) {
        Gdiplus::GdiplusShutdown(s_gdiplus_tok);
        s_gdiplus_tok = 0;
    }
    s_parent = nullptr;
}

#else
// Linux stub
#include "chrome_traffic_lights.h"
void chrome_tl_create(void*, AppState*) {}
void chrome_tl_update() {}
bool chrome_tl_try_click(int, int) { return false; }
bool chrome_tl_hit(int, int) { return false; }
void chrome_tl_destroy() {}
#endif // _WIN32
#endif // !__APPLE__