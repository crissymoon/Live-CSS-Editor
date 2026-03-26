// chrome.cpp -- orchestration: init, theme, top/bottom/panels dispatch.
// All rendering is split into focused modules:
//   chrome_traffic_lights.cpp  -- close / min / zoom buttons (Win/Linux)
//   chrome_tab_row.cpp         -- tab strip with drag-to-reorder
//   chrome_toolbar.cpp         -- URL bar, nav buttons, three-dot menu
//   chrome_drawer.cpp          -- inline more-panel, bookmarks, history

#include "chrome.h"
#include "chrome_shared.h"
#include "chrome_tab_row.h"
#include "chrome_toolbar.h"
#include "chrome_drawer.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <algorithm>

static AppState* s_state = nullptr;

void chrome_set_logo(uint32_t tex_id, int src_w, int src_h) {
    chrome_tab_set_logo(tex_id, src_w, src_h);
}

// ── Palette lives in chrome_shared.h (inline const, ODR-safe) ────────

void chrome_apply_theme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding      = {0, 0};
    s.FramePadding       = {8, 5};
    s.ItemSpacing        = {6, 4};
    s.ItemInnerSpacing   = {4, 4};
    s.ScrollbarSize      = 8;
    s.GrabMinSize        = 6;
    s.WindowRounding     = 0;
    s.ChildRounding      = 6;
    s.FrameRounding      = 6;
    s.PopupRounding      = 6;
    s.ScrollbarRounding  = 4;
    s.GrabRounding       = 4;
    s.TabRounding        = 6;
    s.WindowBorderSize   = 0;
    s.ChildBorderSize    = 0;
    s.FrameBorderSize    = 0;
    s.TabBorderSize      = 0;
    s.SeparatorTextBorderSize = 0;

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]            = COL_SURFACE;
    c[ImGuiCol_ChildBg]             = COL_BASE;
    c[ImGuiCol_PopupBg]             = {0.07f, 0.07f, 0.11f, 0.97f};
    c[ImGuiCol_Border]              = COL_SEP;
    c[ImGuiCol_FrameBg]             = COL_BASE;
    c[ImGuiCol_FrameBgHovered]      = COL_RAISED;
    c[ImGuiCol_FrameBgActive]       = {0.10f, 0.10f, 0.16f, 1.0f};
    c[ImGuiCol_TitleBg]             = COL_SURFACE;
    c[ImGuiCol_TitleBgActive]       = COL_SURFACE;
    c[ImGuiCol_MenuBarBg]           = COL_SURFACE;
    c[ImGuiCol_ScrollbarBg]         = COL_BASE;
    c[ImGuiCol_ScrollbarGrab]       = {0.22f, 0.22f, 0.35f, 1.0f};
    c[ImGuiCol_ScrollbarGrabHovered]= COL_ACCENT;
    c[ImGuiCol_Button]              = COL_RAISED;
    c[ImGuiCol_ButtonHovered]       = COL_ACCENT_MID;
    c[ImGuiCol_ButtonActive]        = COL_ACCENT;
    c[ImGuiCol_Header]              = COL_TAB_ACT;
    c[ImGuiCol_HeaderHovered]       = COL_TAB_HOV;
    c[ImGuiCol_HeaderActive]        = COL_ACCENT_MID;
    c[ImGuiCol_Tab]                 = COL_TAB_IDLE;
    c[ImGuiCol_TabHovered]          = COL_TAB_HOV;
    c[ImGuiCol_TabActive]           = COL_TAB_ACT;
    c[ImGuiCol_TabUnfocused]        = COL_TAB_IDLE;
    c[ImGuiCol_TabUnfocusedActive]  = COL_TAB_ACT;
    c[ImGuiCol_Text]                = COL_TEXT;
    c[ImGuiCol_TextDisabled]        = COL_TEXT_DIM;
    c[ImGuiCol_CheckMark]           = COL_ACCENT;
    c[ImGuiCol_SliderGrab]          = COL_ACCENT;
    c[ImGuiCol_SliderGrabActive]    = COL_ACCENT;
    c[ImGuiCol_PlotHistogram]       = COL_ACCENT;
    c[ImGuiCol_Separator]           = COL_SEP;
    c[ImGuiCol_SeparatorHovered]    = COL_ACCENT;
    c[ImGuiCol_SeparatorActive]     = COL_ACCENT;
    c[ImGuiCol_ResizeGrip]          = {0,0,0,0};
    c[ImGuiCol_ResizeGripHovered]   = COL_ACCENT_LO;
    c[ImGuiCol_ResizeGripActive]    = COL_ACCENT;
}

void chrome_init(AppState* state) {
    s_state = state;
    chrome_apply_theme();
}

// ── Minimal grab strip (fallback when both tabs and toolbar are hidden) ─
static int draw_grab_strip(int win_w) {
    const float H = 32.0f;
    begin_panel("##grabstrip", 0, 0, (float)win_w, H, COL_SURFACE);
    ImGui::GetForegroundDrawList()->AddLine(
        {0.0f, 0.5f}, {(float)win_w, 0.5f},
        to_u32({0.60f, 0.58f, 1.00f, 0.18f}), 1.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine({0.0f, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);
    dl->AddRectFilledMultiColor(
        {0.0f, 0.0f}, {(float)win_w, H},
        to_u32({0.40f, 0.38f, 0.88f, 0.04f}),
        to_u32({0.40f, 0.38f, 0.88f, 0.04f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));
    end_panel();
    return (int)H;
}

// ── Top chrome (tab strip + toolbar + optional drawer) ────────────────
int chrome_draw_top(AppState* st,
                    int win_w, int /*win_h*/,
                    bool& new_tab_requested,
                    int&  close_tab_idx) {
    const bool want_tabs    = st->settings.show_tabs;
    const bool want_toolbar = st->settings.show_toolbar;

    int h = 0;

    if (want_tabs) {
        h += chrome_draw_tab_row(st, win_w, new_tab_requested, close_tab_idx);
    } else {
        close_tab_idx     = -1;
        new_tab_requested = false;

        if (!want_toolbar) {
            h += draw_grab_strip(win_w);
        } else {
            // Reserve space so traffic lights don't overlap nav buttons on left
            h += 30;
        }
    }

    if (want_toolbar)
        h += chrome_draw_toolbar(st, win_w, h);

    // Inline drawer extends chrome area, pushing WebView2 down
    h += chrome_draw_more_drawer(st, win_w, h);

    return h;
}

// ── Bottom status bar ─────────────────────────────────────────────────
int chrome_draw_bottom(AppState* st, int win_w, int win_h) {
    const float H = (float)STATUS_HEIGHT_PX;   // currently 0 -- no visible bar
    begin_panel("##status", 0, (float)(win_h - (int)H), (float)win_w, H, COL_STATUS_BG);

    if (H > 0.0f) {
        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2      wpos = ImGui::GetWindowPos();
        dl->AddLine({0, wpos.y}, {(float)win_w, wpos.y}, to_u32(COL_SEP), 1.0f);

        float cy = (H - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPos({10.0f, cy});

        const std::string& disp = st->hover_url.empty() ? st->status_text : st->hover_url;
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
        ImGui::TextUnformatted(disp.c_str());
        ImGui::PopStyleColor();

        char vp_str[48];
        int content_h = win_h - TAB_BAR_HEIGHT_PX - CHROME_HEIGHT_PX - STATUS_HEIGHT_PX;
        snprintf(vp_str, sizeof(vp_str), "%d x %d", win_w, content_h);
        float tw = ImGui::CalcTextSize(vp_str).x;
        ImGui::SameLine((float)win_w - tw - 76.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
        ImGui::TextUnformatted(vp_str);
        ImGui::PopStyleColor();

        float lx = (float)win_w - 58.0f;
        float ly = wpos.y + H * 0.5f;
        float r  = 3.5f;
        dl->AddCircleFilled({lx,       ly}, r,
            to_u32(st->php_server_ok  ? COL_OK : COL_BAD), 8);
        dl->AddText({lx + r + 3, ly - ImGui::GetTextLineHeight() * 0.5f},
            to_u32(COL_TEXT_DIM), "php");
        dl->AddCircleFilled({lx + 36.0f, ly}, r,
            to_u32(st->node_server_ok ? COL_OK : COL_BAD), 8);
        dl->AddText({lx + 36.0f + r + 3, ly - ImGui::GetTextLineHeight() * 0.5f},
            to_u32(COL_TEXT_DIM), "js");
    }

    end_panel();
    return (int)H;
}

// ── Floating panels (called from main render loop) ────────────────────
void chrome_draw_panels(AppState* st,
                        float anchor_bm_x, float anchor_hist_x,
                        int   panel_top) {
#if defined(__APPLE__)
    chrome_draw_bookmarks_panel(st, anchor_bm_x,   (float)panel_top);
    chrome_draw_history_panel  (st, anchor_hist_x, (float)panel_top);
#else
    (void)st; (void)anchor_bm_x; (void)anchor_hist_x; (void)panel_top;
#endif
}

// ── Panel anchor getters (used by native_chrome_stub) ─────────────────
float chrome_bm_btn_x()   { return chrome_drawer_bm_btn_x(); }
float chrome_hist_btn_x() { return chrome_drawer_hist_btn_x(); }
