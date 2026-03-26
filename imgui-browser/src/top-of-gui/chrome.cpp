// chrome.cpp -- Dear ImGui browser chrome
// Modern dark design for Crissy's Style Tool.
// Single header row: title pill (left) + tabs (centre) + nav/URL (row 2).

#include "chrome.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>
#include <cctype>

static AppState* s_state = nullptr;

// ── Logo state ────────────────────────────────────────────────────────
static uint32_t s_logo_tex   = 0;  // OpenGL texture ID (0 = none)
static int      s_logo_src_w = 0;  // original pixel width
static int      s_logo_src_h = 0;  // original pixel height

void chrome_set_logo(uint32_t tex_id, int src_w, int src_h) {
    s_logo_tex   = tex_id;
    s_logo_src_w = src_w;
    s_logo_src_h = src_h;
}

// ── Palette ───────────────────────────────────────────────────────────
// Base surface
static const ImVec4 COL_BASE       = {0.035f, 0.035f, 0.051f, 1.0f};  // #090911
static const ImVec4 COL_SURFACE    = {0.063f, 0.063f, 0.094f, 1.0f};  // #101118
static const ImVec4 COL_RAISED     = {0.090f, 0.090f, 0.133f, 1.0f};  // #171722
// Accent
static const ImVec4 COL_ACCENT     = {0.388f, 0.400f, 0.941f, 1.0f};  // #6366F1
static const ImVec4 COL_ACCENT_LO  = {0.388f, 0.400f, 0.941f, 0.18f};
static const ImVec4 COL_ACCENT_MID = {0.388f, 0.400f, 0.941f, 0.35f};
// Tabs
static const ImVec4 COL_TAB_ACT    = {0.110f, 0.110f, 0.176f, 1.0f};  // #1C1C2D   active
static const ImVec4 COL_TAB_IDLE   = {0.000f, 0.000f, 0.000f, 0.0f};  // transparent
static const ImVec4 COL_TAB_HOV    = {0.388f, 0.400f, 0.941f, 0.10f};
// Text
static const ImVec4 COL_TEXT       = {0.900f, 0.914f, 0.961f, 1.0f};  // #E6E9F5
static const ImVec4 COL_TEXT_DIM   = {0.400f, 0.427f, 0.502f, 1.0f};  // #666E80
// Status
static const ImVec4 COL_OK         = {0.204f, 0.827f, 0.600f, 1.0f};
static const ImVec4 COL_WARN       = {0.984f, 0.753f, 0.259f, 1.0f};
static const ImVec4 COL_BAD        = {0.973f, 0.529f, 0.451f, 1.0f};
// Border / separator
static const ImVec4 COL_SEP        = {0.388f, 0.400f, 0.941f, 0.12f};
// Status bar background -- visibly lighter purple, clearly distinct from content
static const ImVec4 COL_STATUS_BG  = {0.165f, 0.130f, 0.270f, 1.0f};

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

// ── Tab drag state ────────────────────────────────────────────────────
static int   s_drag_idx   = -1;     // index of tab being dragged (-1 = none)
static float s_drag_off_x = 0.0f;   // cursor-X offset within the tab at drag start
static int   s_drop_tgt   = -1;     // computed insert position during drag

// Anchor positions for floating panels (screen X of button that opens each)
static float s_bm_btn_screen_x   = 0.0f;
static float s_hist_btn_screen_x = 0.0f;
// Frame counters: prevent panels from closing on the same frame they open.
static int   s_bm_open_frame     = -1;
static int   s_hist_open_frame   = -1;

// ── Internal helpers ──────────────────────────────────────────────────

static inline ImU32 to_u32(ImVec4 v) { return ImGui::ColorConvertFloat4ToU32(v); }

// Frameless ImGui window pinned to a rect
static bool begin_panel(const char* id, float x, float y, float w, float h,
                         ImVec4 bg = COL_SURFACE) {
    ImGui::SetNextWindowPos({x, y});
    ImGui::SetNextWindowSize({w, h});
    ImGui::SetNextWindowBgAlpha(bg.w);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    return ImGui::Begin(id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
}
static void end_panel() {
    ImGui::End();
    ImGui::PopStyleColor();
}

// Ghost button -- no background, shows hover ring
static bool ghost_btn(const char* id, const char* text, float w, float h,
                       const char* tooltip = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    bool hit = ImGui::Button(id, {w, h});
    // draw label centred (Button shows id stripped of ##, use invisible id + manual text)
    if (text) {
        ImVec2 ts   = ImGui::CalcTextSize(text);
        ImVec2 tmin = ImGui::GetItemRectMin();
        ImVec2 tmax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddText(
            {tmin.x + (tmax.x - tmin.x - ts.x) * 0.5f,
             tmin.y + (tmax.y - tmin.y - ts.y) * 0.5f},
            to_u32(COL_TEXT_DIM), text);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s", tooltip);
    return hit;
}

// ── Title / tab row ───────────────────────────────────────────────────
// Single-row tab bar with drag-to-reorder and close-on-hover.

static int draw_title_tab_row(AppState* st, int win_w,
                               bool& new_tab_req, int& close_idx) {
    close_idx   = -1;
    new_tab_req = false;

    const float H       = (float)TAB_BAR_HEIGHT_PX;
    const float TOP_PAD = 30.0f;   // clear macOS traffic lights
    const float AVAIL_H = H - TOP_PAD;

    begin_panel("##titlerow", 0, 0, (float)win_w, H, COL_SURFACE);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine({0, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);
    // Morphism: subtle vertical gradient shimmer across the full strip
    dl->AddRectFilledMultiColor(
        {0.0f, 0.0f}, {(float)win_w, H},
        to_u32({0.40f, 0.38f, 0.88f, 0.055f}),
        to_u32({0.40f, 0.38f, 0.88f, 0.055f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));
    // Single-pixel top highlight edge -- drawn on the foreground draw list
    // so it is never clipped by the ImGui window content rect.
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    fg->AddLine({0.0f, 0.5f}, {(float)win_w, 0.5f},
                to_u32({0.60f, 0.58f, 1.00f, 0.22f}), 1.0f);

    const float TL_GAP = (float)TRAFFIC_LIGHT_W;
    const float PLUS_W = 28.0f;
    float tabs_x = TL_GAP;
    float tabs_w = (float)win_w - TL_GAP - PLUS_W - 4.0f;

#ifndef __APPLE__
    // Window control buttons (close / minimize / maximize) in the 30 px grab strip.
    // macOS provides native traffic lights; on other platforms we draw our own.
    {
        const float BTN_R  = 6.0f;
        const float BTN_CY = TOP_PAD * 0.5f;
        struct WinBtn { const char* id; float cx; ImVec4 idle; ImVec4 hov; const char* cmd; };
        WinBtn btns[] = {
            { "##wc0", 14.0f, {0.55f,0.20f,0.20f,1.0f}, {0.97f,0.27f,0.27f,1.0f}, "__win_close__"    },
            { "##wc1", 34.0f, {0.45f,0.38f,0.12f,1.0f}, {0.98f,0.75f,0.13f,1.0f}, "__win_minimize__" },
            { "##wc2", 54.0f, {0.14f,0.43f,0.25f,1.0f}, {0.20f,0.83f,0.60f,1.0f}, "__win_zoom__"     },
        };
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0,0});
        for (auto& b : btns) {
            ImGui::SetCursorPos({b.cx - BTN_R, BTN_CY - BTN_R});
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0,0,0,0});
            bool clicked = ImGui::InvisibleButton(b.id, {BTN_R * 2.0f, BTN_R * 2.0f});
            bool hov     = ImGui::IsItemHovered();
            dl->AddCircleFilled({b.cx, BTN_CY}, BTN_R, to_u32(hov ? b.hov : b.idle), 16);
            // Icon on hover: small symbol inside the circle
            if (hov) {
                float arm = 3.2f;
                ImU32 ic  = to_u32({1.0f,1.0f,1.0f,0.85f});
                if (b.cx < 20.0f) { // close: X
                    dl->AddLine({b.cx-arm,BTN_CY-arm},{b.cx+arm,BTN_CY+arm},ic,1.5f);
                    dl->AddLine({b.cx+arm,BTN_CY-arm},{b.cx-arm,BTN_CY+arm},ic,1.5f);
                } else if (b.cx < 44.0f) { // minimize: dash
                    dl->AddLine({b.cx-arm,BTN_CY},{b.cx+arm,BTN_CY},ic,1.5f);
                } else { // zoom: + or square
                    dl->AddLine({b.cx-arm,BTN_CY},{b.cx+arm,BTN_CY},ic,1.5f);
                    dl->AddLine({b.cx,BTN_CY-arm},{b.cx,BTN_CY+arm},ic,1.5f);
                }
            }
            ImGui::PopStyleColor(3);
            if (clicked) {
                fprintf(stderr, "[chrome] win ctrl clicked: %s\n", b.cmd);
                st->push_nav(-1, b.cmd);
            }
        }
        ImGui::PopStyleVar();
    }

    // DEBUG: show mouse coords in the grab strip so it is immediately visible
    // whether ImGui is receiving mouse events at all.  Remove once confirmed.
    {
        ImVec2 mp = ImGui::GetMousePos();
        char dbg[32];
        snprintf(dbg, sizeof(dbg), "%.0f,%.0f", mp.x, mp.y);
        ImGui::GetForegroundDrawList()->AddText({200.0f, 8.0f},
                                               IM_COL32(255, 220, 0, 200), dbg);
    }
#endif

    int   n     = (int)st->tabs.size();
    float tab_w = std::min(180.0f, std::max(64.0f, tabs_w / (float)std::max(n, 1)));

    float cx = tabs_x;
    for (int i = 0; i < n; i++) {
        auto& tab    = st->tabs[i];
        bool  active = (i == st->active_tab);
        bool  dragging_this = (s_drag_idx == i &&
                               ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f));

        ImGui::PushID(tab.id);

        ImVec2 tMin = {cx, TOP_PAD};
        ImVec2 tMax = {cx + tab_w, H};

        // Hover detection via rect (before the invisible button so we know close_w)
        bool hov_rect = ImGui::IsMouseHoveringRect(tMin, tMax) && s_drag_idx == -1;

        // Close button shown when hovered or active (needs n > 1)
        float close_w = (n > 1 && (active || hov_rect)) ? 18.0f : 0.0f;

        // Tab background
        float fill_alpha = dragging_this ? 0.25f : 1.0f;
        if (active) {
            ImVec4 bg = COL_TAB_ACT; bg.w = fill_alpha;
            dl->AddRectFilled(tMin, tMax, to_u32(bg), 6.0f, ImDrawFlags_RoundCornersTop);
            if (!dragging_this)
                dl->AddLine({tMin.x + 4, tMin.y + 1},
                            {tMax.x - 4, tMin.y + 1},
                            to_u32(COL_ACCENT), 2.0f);
        } else if (hov_rect) {
            dl->AddRectFilled(tMin, tMax, to_u32(COL_TAB_HOV), 6.0f,
                              ImDrawFlags_RoundCornersTop);
        }
        // Rounded border on top/left/right matching the tab fill shape
        {
            ImU32 tb = to_u32(active
                ? ImVec4{0.388f, 0.400f, 0.941f, 0.55f}
                : ImVec4{0.388f, 0.400f, 0.941f, 0.18f});
            dl->AddRect(tMin, tMax, tb, 6.0f, ImDrawFlags_RoundCornersTop, 1.0f);
        }

        // Label (clipped if too long)
        {
            char disp[36];
            snprintf(disp, sizeof(disp), "%.28s", tab.title.c_str());
            float ty  = TOP_PAD + (AVAIL_H - ImGui::GetTextLineHeight()) * 0.5f;
            float tx  = cx + 10.0f;
            float right_edge = cx + tab_w - close_w - 6.0f;
            ImVec4 tc = active ? COL_TEXT : COL_TEXT_DIM;
            tc.w = fill_alpha;
            ImU32 tc32 = to_u32(tc);
            if (ImGui::CalcTextSize(disp).x > right_edge - tx) {
                ImGui::PushClipRect({tx, ty},
                                    {right_edge, ty + ImGui::GetTextLineHeight() + 2}, true);
                dl->AddText({tx, ty}, tc32, disp);
                ImGui::PopClipRect();
            } else {
                dl->AddText({tx, ty}, tc32, disp);
            }
        }

        // Loading spinner dot
        if (tab.loading && !dragging_this) {
            float dot_x = cx + tab_w - close_w - 12.0f;
            float dot_y = TOP_PAD + AVAIL_H * 0.5f;
            dl->AddCircleFilled({dot_x, dot_y}, 3.5f, to_u32(COL_ACCENT), 8);
        }

        // Invisible button for click and drag detection.
        // Only spans the non-close area so the close button gets its own
        // independent hit-test (InvisibleButton would steal the press otherwise).
        ImGui::SetCursorPos({cx, TOP_PAD});
        char btn_id[16];
        snprintf(btn_id, sizeof(btn_id), "##tb%d", tab.id);
        float ib_w = (close_w > 0.0f) ? (tab_w - close_w - 1.0f) : tab_w;
        ImGui::InvisibleButton(btn_id, {ib_w, AVAIL_H});

        if (ImGui::IsItemActivated()) {
            s_drag_idx   = i;
            s_drag_off_x = ImGui::GetMousePos().x - cx;
            st->active_tab = i;   // switch immediately on press for responsiveness
        }
        if (s_drag_idx == i && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
            float mx = ImGui::GetMousePos().x;
            s_drop_tgt = std::max(0, std::min(n - 1,
                         (int)((mx - tabs_x) / tab_w)));
        }
        if (ImGui::IsItemDeactivated() && s_drag_idx == i) {
            if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
                st->active_tab = i;     // clean click
            } else if (s_drop_tgt != -1 && s_drop_tgt != i) {
                int from = i, to = s_drop_tgt;
                int was  = st->active_tab;
                auto& tabs = st->tabs;
                if (from < to) {
                    std::rotate(tabs.begin()+from, tabs.begin()+from+1, tabs.begin()+to+1);
                    if      (was == from)              st->active_tab = to;
                    else if (was > from && was <= to)  st->active_tab--;
                } else {
                    std::rotate(tabs.begin()+to, tabs.begin()+from, tabs.begin()+from+1);
                    if      (was == from)              st->active_tab = to;
                    else if (was >= to && was < from)  st->active_tab++;
                }
            }
            s_drag_idx = -1;
            s_drop_tgt = -1;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && s_drag_idx == -1)
            ImGui::SetTooltip("%s", tab.url.c_str());

        // Close button
        if (close_w > 0.0f) {
            ImGui::SetCursorPos({cx + tab_w - close_w - 1.0f,
                                 TOP_PAD + (AVAIL_H - close_w) * 0.5f});
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f,0.3f,0.3f,0.30f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.9f,0.3f,0.3f,0.55f});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, close_w * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0, 0});
            char cid[16];
            snprintf(cid, sizeof(cid), "##cx%d", tab.id);
            if (ImGui::Button(cid, {close_w, close_w}))
                close_idx = i;
            {
                ImVec2 bmin = ImGui::GetItemRectMin();
                ImVec2 bmax = ImGui::GetItemRectMax();
                float  m    = 5.0f;
                ImU32  xc   = ImGui::IsItemHovered()
                    ? to_u32({1.0f,1.0f,1.0f,0.9f})
                    : to_u32(COL_TEXT_DIM);
                dl->AddLine({bmin.x+m, bmin.y+m}, {bmax.x-m, bmax.y-m}, xc, 1.5f);
                dl->AddLine({bmax.x-m, bmin.y+m}, {bmin.x+m, bmax.y-m}, xc, 1.5f);
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
        }

        cx += tab_w;
        ImGui::PopID();
    }

    // Ghost tab + drop indicator drawn on top during drag
    if (s_drag_idx != -1 && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
        float mx      = ImGui::GetMousePos().x;
        float ghost_x = std::max(tabs_x, std::min(mx - s_drag_off_x,
                                                   tabs_x + (float)n * tab_w - tab_w));
        ImVec2 gMin = {ghost_x, TOP_PAD};
        ImVec2 gMax = {ghost_x + tab_w, H};
        dl->AddRectFilled(gMin, gMax, to_u32({0.388f,0.400f,0.941f,0.20f}), 6.0f,
                          ImDrawFlags_RoundCornersTop);
        dl->AddRect(gMin, gMax, to_u32(COL_ACCENT), 6.0f,
                    ImDrawFlags_RoundCornersTop, 1.5f);
        if (s_drop_tgt != -1) {
            float lx = tabs_x + (float)s_drop_tgt * tab_w;
            if (s_drop_tgt > s_drag_idx) lx += tab_w;
            dl->AddLine({lx, TOP_PAD + 2}, {lx, H - 2}, to_u32(COL_ACCENT), 2.0f);
        }
    }

    // "+" new-tab button
    ImGui::SetCursorPos({cx + 2.0f, TOP_PAD + (AVAIL_H - 22.0f) * 0.5f});
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0, 0});
    if (ImGui::Button("##newtab", {22.0f, 22.0f})) new_tab_req = true;
    {
        ImVec2 c2  = {(ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
                      (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f};
        float  arm = 5.0f;
        ImU32  pc  = to_u32(ImGui::IsItemHovered() ? COL_ACCENT : COL_TEXT_DIM);
        dl->AddLine({c2.x-arm, c2.y}, {c2.x+arm, c2.y}, pc, 1.5f);
        dl->AddLine({c2.x, c2.y-arm}, {c2.x, c2.y+arm}, pc, 1.5f);
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    // Logo: Xcalibur The Cat -- drawn in the far-right corner of the tab bar.
    // Only drawn when there is enough space right of the "+" button.
    if (s_logo_tex && s_logo_src_h > 0) {
        const float logo_h  = AVAIL_H - 4.0f;
        const float logo_w  = logo_h * (float)s_logo_src_w / (float)s_logo_src_h;
        const float right   = (float)win_w - 8.0f;
        const float lx      = right - logo_w;
        const float ly      = TOP_PAD + (AVAIL_H - logo_h) * 0.5f;
        // Only draw if it doesn't overlap the last tab / "+" button area
        float used_x = cx + 24.0f + 8.0f;  // "+" button right + small gap
        if (lx > used_x) {
            ImGui::SetCursorPos({lx, ly});
            ImGui::Image((ImTextureID)(uintptr_t)s_logo_tex, {logo_w, logo_h});
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("Xcalibur The Cat");
        }
    }

    end_panel();
    return (int)H;
}

// ── Toolbar (URL bar + nav row) ───────────────────────────────────────

static int draw_toolbar(AppState* st, int win_w, int y_offset) {
    const float H   = (float)CHROME_HEIGHT_PX;
    const float pad = 8.0f;
    const float btn = 30.0f;

    begin_panel("##toolbar", 0, (float)y_offset, (float)win_w, H, COL_SURFACE);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    // GetWindowDrawList uses absolute screen coords; offset manually.
    ImVec2 wpos = ImGui::GetWindowPos();

    // Bottom hair line
    dl->AddLine({0, wpos.y + H - 1}, {(float)win_w, wpos.y + H - 1},
                to_u32(COL_SEP), 1.0f);
    // Morphism: top-to-bottom shimmer, lighter at top
    dl->AddRectFilledMultiColor(
        {0.0f, wpos.y}, {(float)win_w, wpos.y + H},
        to_u32({0.38f, 0.36f, 0.88f, 0.045f}),
        to_u32({0.38f, 0.36f, 0.88f, 0.045f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));

    Tab* tab = st->current_tab();

    float cy = (H - ImGui::GetFrameHeight()) * 0.5f;
    ImGui::SetCursorPos({pad, cy});

    // Back
    ImGui::BeginDisabled(!tab || !tab->can_back);
    if (ghost_btn("##back", "<", btn, btn, "Back (Cmd+[)") && tab)
        st->push_nav(tab->id, "__back__");
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    // Forward
    ImGui::BeginDisabled(!tab || !tab->can_forward);
    if (ghost_btn("##fwd", ">", btn, btn, "Forward (Cmd+])") && tab)
        st->push_nav(tab->id, "__forward__");
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    // Reload / Stop
    if (tab && tab->loading) {
        if (ghost_btn("##stop", "x", btn, btn, "Stop (Esc)"))
            st->push_nav(tab->id, "__stop__");
    } else {
        if (ghost_btn("##reload", "r", btn, btn, "Reload (Cmd+R)") && tab)
            st->push_nav(tab->id, "__reload__");
    }
    ImGui::SameLine(0, 6);

    // URL bar
    float url_x     = ImGui::GetCursorPosX();
    float lock_w    = 22.0f;        // security indicator
    float devtool_w = btn + 4.0f;
    float js_btn_w  = btn + 4.0f;   // JS toggle
    float bm_btn_w  = btn + 2.0f;   // bookmark star
    float hist_btn_w = btn;         // history clock
    float url_w     = (float)win_w - url_x - (lock_w + 4.0f)
                      - devtool_w - js_btn_w - bm_btn_w - hist_btn_w - pad * 2.0f;

    // ── Security indicator (HTTPS / HTTP / other) ─────────────────
    {
        const std::string& turl = tab ? tab->url : std::string();
        bool https = turl.size() >= 8 && turl.substr(0, 8) == "https://";
        bool http  = turl.size() >= 7 && turl.substr(0, 7) == "http://";
        ImVec4 sec_col = https ? COL_OK : (http ? COL_WARN : COL_TEXT_DIM);
        ImVec2 sp2 = ImGui::GetCursorScreenPos();
        float  fh2 = ImGui::GetFrameHeight();
        float  dcx = sp2.x + lock_w * 0.5f;
        float  dcy = sp2.y + fh2 * 0.5f;
        if (https) {
            // Filled circle = secure
            dl->AddCircleFilled({dcx, dcy}, 4.5f, to_u32(sec_col), 12);
        } else {
            // Hollow circle = not secure / unknown
            dl->AddCircle({dcx, dcy}, 4.5f, to_u32(sec_col), 12, 1.5f);
            if (http)
                dl->AddCircleFilled({dcx, dcy}, 1.5f, to_u32(sec_col), 8);
        }
        ImGui::InvisibleButton("##sec_ind", {lock_w, fh2});
        const char* sec_tip = https ? "Secure connection (HTTPS)"
                             : http  ? "Not secure -- data sent unencrypted (HTTP)"
                                     : nullptr;
        if (sec_tip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", sec_tip);
        ImGui::SameLine(0, 4);
    }

    // Progress underline before drawing the input
    if (tab && tab->loading && tab->progress > 0.0f) {
        ImVec2 sp = ImGui::GetCursorScreenPos();
        float  fh = ImGui::GetFrameHeight();
        // Full trough
        dl->AddRectFilled({sp.x, sp.y + fh - 2},
                          {sp.x + url_w, sp.y + fh},
                          to_u32({0.10f,0.10f,0.16f,1.0f}), 1.0f);
        // Progress fill
        ImVec4 pg = COL_ACCENT; pg.w = 0.60f;
        dl->AddRectFilled({sp.x, sp.y + fh - 2},
                          {sp.x + url_w * tab->progress, sp.y + fh},
                          to_u32(pg), 1.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BASE);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, COL_RAISED);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  COL_RAISED);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::SetNextItemWidth(url_w);

    // Focus URL bar only when explicitly requested (Cmd+L or click).
    // Do NOT auto-focus on launch -- that steals keyboard input so nav
    // buttons and tabs feel unresponsive until the user clicks elsewhere.
    if (st->focus_url_next_frame) {
        ImGui::SetKeyboardFocusHere();
        st->focus_url_next_frame = false;
    }

    bool url_activated = false;
    if (tab) {
        url_activated = ImGui::InputText("##url", tab->url_buf, URL_BUF_SIZE,
                                         ImGuiInputTextFlags_EnterReturnsTrue |
                                         ImGuiInputTextFlags_AutoSelectAll);
        if (url_activated) {
            tab->url = tab->url_buf;
            st->push_nav(tab->id, tab->url);
        }
        if (!ImGui::IsItemActive() && tab->url != tab->url_buf)
            snprintf(tab->url_buf, URL_BUF_SIZE, "%s", tab->url.c_str());
    } else {
        char empty[2] = {};
        ImGui::InputText("##url", empty, sizeof(empty));
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);

    // Dev tools toggle
    bool dt_on = st->dev_tools_open;
    if (dt_on) {
        ImGui::PushStyleColor(ImGuiCol_Button,       COL_ACCENT_MID);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,COL_ACCENT);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, COL_ACCENT);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("##devt", {btn, btn})) st->dev_tools_open = !st->dev_tools_open;
    {
        // Draw "{}" glyph
        ImVec2 bmin = ImGui::GetItemRectMin();
        ImVec2 bmax = ImGui::GetItemRectMax();
        ImU32  gc   = to_u32(dt_on ? COL_TEXT : COL_TEXT_DIM);
        float  mx   = (bmin.x + bmax.x) * 0.5f;
        float  my   = (bmin.y + bmax.y) * 0.5f;
        // Simple "{}" via two small line segments
        dl->AddText(
            {mx - ImGui::CalcTextSize("{}").x * 0.5f,
             my - ImGui::GetTextLineHeight() * 0.5f},
            gc, "{}");
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Dev Tools (Cmd+Shift+I)");

    ImGui::SameLine(0, 2);

    // ── JS toggle ──────────────────────────────────────────────────
    // Allows per-tab JavaScript enable/disable with immediate visual
    // feedback. The actual WKWebpagePreferences change is applied on the
    // next navigation via the nav delegate (triggered by a reload).
    bool js_on = !tab || tab->js_enabled;
    if (js_on) {
        ImGui::PushStyleColor(ImGuiCol_Button,        COL_ACCENT_LO);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_MID);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("##jstog", {btn, btn}) && tab) {
        // Flip the UI flag immediately so the button redraws without lag.
        // Push the desired state as a nav command so main.mm calls
        // webview_set_js_enabled (which reloads to apply the setting).
        tab->js_enabled = !tab->js_enabled;
        st->push_nav(tab->id, tab->js_enabled ? "__js_on__" : "__js_off__");
    }
    {
        const char* lbl = "JS";
        ImVec2 bmin = ImGui::GetItemRectMin();
        ImVec2 bmax = ImGui::GetItemRectMax();
        ImVec2 ts   = ImGui::CalcTextSize(lbl);
        ImU32  gc   = to_u32(js_on ? COL_TEXT : COL_TEXT_DIM);
        dl->AddText({(bmin.x + bmax.x - ts.x) * 0.5f,
                     (bmin.y + bmax.y - ts.y) * 0.5f},
                    gc, lbl);
        // Strike-through when JS is disabled
        if (!js_on) {
            float mid_y = (bmin.y + bmax.y) * 0.5f;
            dl->AddLine({bmin.x + 4, mid_y}, {bmax.x - 4, mid_y},
                        to_u32(COL_BAD), 1.5f);
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s", js_on
            ? "JavaScript enabled -- click to disable for this tab"
            : "JavaScript disabled -- click to re-enable for this tab");

    ImGui::SameLine(0, 2);

    // ── Bookmark star ──────────────────────────────────────────────
    {
        // Check whether the current URL is already bookmarked
        bool is_bm = false;
        if (tab && !tab->url.empty()) {
            for (auto& b : st->bookmarks)
                if (b.url == tab->url) { is_bm = true; break; }
        }
        bool bm_open = st->show_bookmarks_panel;
        if (is_bm || bm_open) {
            ImGui::PushStyleColor(ImGuiCol_Button,        COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_MID);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::Button("##bm", {btn, btn})) {
            st->show_bookmarks_panel = !st->show_bookmarks_panel;
            st->show_history_panel   = false;
            if (st->show_bookmarks_panel)
                s_bm_open_frame = ImGui::GetFrameCount();
        }
        // Record screen position for panel anchoring
        s_bm_btn_screen_x = ImGui::GetItemRectMax().x;
        {
            // Draw a 5-pointed star via geometry (fonts may lack U+2605/U+2606)
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            ImU32  gc   = to_u32(is_bm ? COL_ACCENT : COL_TEXT_DIM);
            ImVec2 c2   = {(bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f};
            float  ro   = (bmax.x - bmin.x) * 0.36f;   // outer radius
            float  ri   = ro * 0.40f;                    // inner radius
            const float kPi = 3.14159265f;
            for (int k = 0; k < 5; ++k) {
                float a0 = (float)k * (2.0f * kPi / 5.0f) - kPi * 0.5f;
                float a1 = a0 + kPi / 5.0f;
                float a2 = a0 + 2.0f * kPi / 5.0f;
                ImVec2 p0 = {c2.x + cosf(a0)*ro, c2.y + sinf(a0)*ro};
                ImVec2 p1 = {c2.x + cosf(a1)*ri, c2.y + sinf(a1)*ri};
                ImVec2 p2 = {c2.x + cosf(a2)*ro, c2.y + sinf(a2)*ro};
                if (is_bm) {
                    dl->AddTriangleFilled(c2, p0, p1, gc);
                    dl->AddTriangleFilled(c2, p1, p2, gc);
                } else {
                    dl->AddLine(p0, p1, gc, 1.2f);
                    dl->AddLine(p1, p2, gc, 1.2f);
                }
            }
            if (!is_bm) {   // close the outline star
                float a0 = -kPi * 0.5f;
                float a1_last = 4.0f * (2.0f * kPi / 5.0f) - kPi * 0.5f + kPi / 5.0f;
                ImVec2 tip  = {c2.x + cosf(a0)*ro,     c2.y + sinf(a0)*ro};
                ImVec2 last_inner = {c2.x + cosf(a1_last)*ri, c2.y + sinf(a1_last)*ri};
                dl->AddLine(last_inner, tip, gc, 1.2f);
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(is_bm ? "Bookmarked -- click to manage"
                                    : "Bookmark this page (Cmd+D)");
    }
    ImGui::SameLine(0, 0);

    // ── History button ─────────────────────────────────────────────
    {
        bool hist_open = st->show_history_panel;
        if (hist_open) {
            ImGui::PushStyleColor(ImGuiCol_Button,        COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_MID);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::Button("##hist", {btn, btn})) {
            st->show_history_panel   = !st->show_history_panel;
            st->show_bookmarks_panel = false;
            if (st->show_history_panel)
                s_hist_open_frame = ImGui::GetFrameCount();
        }
        s_hist_btn_screen_x = ImGui::GetItemRectMax().x;
        {
        {
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            ImU32  clk_col = to_u32(hist_open ? COL_ACCENT : COL_TEXT_DIM);
            ImVec2 c2   = {(bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f};
            float  r    = (bmax.x - bmin.x) * 0.34f;
            const float kPi = 3.14159265f;
            // Clock face circle
            dl->AddCircle(c2, r, clk_col, 14, 1.3f);
            // Hour hand (~10 o'clock)
            float ah = -kPi * 0.5f - kPi * 0.34f;
            dl->AddLine(c2, {c2.x + cosf(ah)*r*0.52f, c2.y + sinf(ah)*r*0.52f},
                        clk_col, 1.5f);
            // Minute hand (~12 o'clock, pointing up)
            float am = -kPi * 0.5f;
            dl->AddLine(c2, {c2.x + cosf(am)*r*0.72f, c2.y + sinf(am)*r*0.72f},
                        clk_col, 1.3f);
        }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("History (Cmd+H)");
    }

    end_panel();
    return (int)H;
}

// ── Status bar ────────────────────────────────────────────────────────

int chrome_draw_bottom(AppState* st, int win_w, int win_h) {
    const float H = (float)STATUS_HEIGHT_PX;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_STATUS_BG);
    begin_panel("##status", 0, (float)(win_h - (int)H), (float)win_w, H, COL_STATUS_BG);
    ImGui::PopStyleColor();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    // GetWindowDrawList uses absolute screen coords; offset manually.
    ImVec2 wpos = ImGui::GetWindowPos();
    dl->AddLine({0, wpos.y}, {(float)win_w, wpos.y}, to_u32(COL_SEP), 1.0f);

    float cy = (H - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPos({10.0f, cy});

    const std::string& disp = st->hover_url.empty() ? st->status_text : st->hover_url;
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
    ImGui::TextUnformatted(disp.c_str());
    ImGui::PopStyleColor();

    // Right side: viewport size + server LEDs
    char vp_str[48];
    // Show the usable content area (window minus all chrome panels)
    int content_h = win_h - TAB_BAR_HEIGHT_PX - CHROME_HEIGHT_PX - STATUS_HEIGHT_PX;
    snprintf(vp_str, sizeof(vp_str), "%d × %d", win_w, content_h);
    float tw = ImGui::CalcTextSize(vp_str).x;

    ImGui::SameLine((float)win_w - tw - 76.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
    ImGui::TextUnformatted(vp_str);
    ImGui::PopStyleColor();

    // Server LED dots (absolute screen coords)
    float lx = (float)win_w - 58.0f;
    float ly = wpos.y + H * 0.5f;
    float r  = 3.5f;
    dl->AddCircleFilled({lx,      ly}, r,
        to_u32(st->php_server_ok  ? COL_OK : COL_BAD), 8);
    dl->AddText({lx + r + 3, ly - ImGui::GetTextLineHeight() * 0.5f},
        to_u32(COL_TEXT_DIM), "php");
    dl->AddCircleFilled({lx + 36.0f, ly}, r,
        to_u32(st->node_server_ok ? COL_OK : COL_BAD), 8);
    dl->AddText({lx + 36.0f + r + 3, ly - ImGui::GetTextLineHeight() * 0.5f},
        to_u32(COL_TEXT_DIM), "js");

    end_panel();
    return (int)H;
}

// ── Bookmarks panel ───────────────────────────────────────────────────

static void draw_bookmarks_panel(AppState* st, float anchor_x, float panel_top) {
    if (!st->show_bookmarks_panel) return;

    const float W = 300.0f;
    ImGui::SetNextWindowPos({anchor_x - W + 2.0f, panel_top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({W, 0.0f});   // auto-height
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.0f, 8.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6.0f, 4.0f});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.070f, 0.070f, 0.110f, 0.97f});

    if (ImGui::Begin("##bm_panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove)) {

        Tab* tab = st->current_tab();

        // Current page action
        if (tab && tab->url.size() >= 4 && tab->url.substr(0,4) == "http") {
            bool is_bm = false;
            for (auto& b : st->bookmarks)
                if (b.url == tab->url) { is_bm = true; break; }
            ImGui::PushStyleColor(ImGuiCol_Text, is_bm ? COL_WARN : COL_ACCENT);
            const char* act = is_bm ? "Remove from bookmarks" : "Bookmark this page";
            if (ImGui::Selectable(act, false)) {
                st->push_nav(tab->id, "__bookmark_toggle__");
                st->show_bookmarks_panel = false;
            }
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        if (st->bookmarks.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
            ImGui::TextUnformatted("No bookmarks yet.");
            ImGui::PopStyleColor();
        } else {
            // Newest first
            for (int i = (int)st->bookmarks.size() - 1; i >= 0; i--) {
                auto& bm = st->bookmarks[i];
                const std::string& nm = bm.title.empty() ? bm.url : bm.title;
                char lbl[96];
                snprintf(lbl, sizeof(lbl), "%.62s##bm%d", nm.c_str(), i);
                if (ImGui::Selectable(lbl, false)) {
                    Tab* t = st->current_tab();
                    if (t) st->push_nav(t->id, bm.url);
                    st->show_bookmarks_panel = false;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                    ImGui::SetTooltip("%s", bm.url.c_str());
            }
        }

        // Close on click outside
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
                && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetFrameCount() != s_bm_open_frame)
            st->show_bookmarks_panel = false;
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

// ── History panel ─────────────────────────────────────────────────────

static void draw_history_panel(AppState* st, float anchor_x, float panel_top) {
    if (!st->show_history_panel) return;

    const float W  = 340.0f;
    const float MH = 380.0f;
    ImGui::SetNextWindowPos({anchor_x - W, panel_top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({W, MH});
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.0f, 8.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.070f, 0.070f, 0.110f, 0.97f});

    if (ImGui::Begin("##hist_panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {

        // Search / filter
        ImGui::SetNextItemWidth(W - 24.0f);
        ImGui::InputTextWithHint("##hfilt", "Search history...",
                                  st->history_filter, sizeof(st->history_filter));
        ImGui::Separator();

        if (st->history.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
            ImGui::TextUnformatted("No history yet.");
            ImGui::PopStyleColor();
        } else {
            std::string filt(st->history_filter);
            // Lower-case filter for case-insensitive match
            for (auto& c : filt) c = (char)tolower((unsigned char)c);

            // Iterate reverse (most recent first), show up to 200 matching
            int shown = 0;
            for (int i = (int)st->history.size() - 1; i >= 0 && shown < 200; i--) {
                auto& he = st->history[i];
                if (!filt.empty()) {
                    std::string u = he.url, t2 = he.title;
                    for (auto& c : u)  c = (char)tolower((unsigned char)c);
                    for (auto& c : t2) c = (char)tolower((unsigned char)c);
                    if (u.find(filt) == std::string::npos &&
                        t2.find(filt) == std::string::npos) continue;
                }
                const std::string& nm = he.title.empty() ? he.url : he.title;
                char lbl[100];
                snprintf(lbl, sizeof(lbl), "%.64s##he%d", nm.c_str(), i);
                if (ImGui::Selectable(lbl, false)) {
                    Tab* t = st->current_tab();
                    if (t) st->push_nav(t->id, he.url);
                    st->show_history_panel = false;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                    ImGui::SetTooltip("%s", he.url.c_str());
                shown++;
            }
        }

        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
                && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetFrameCount() != s_hist_open_frame)
            st->show_history_panel = false;
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// ── Minimal grab strip (used when both tabs and toolbar are hidden) ───
// Renders only the 32 px macOS traffic-lights clearance area so the
// window can still be dragged.  Returns height consumed.
static int draw_grab_strip(int win_w) {
    const float H = 32.0f;
    begin_panel("##grabstrip", 0, 0, (float)win_w, H, COL_SURFACE);
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    // Subtle top highlight edge
    fg->AddLine({0.0f, 0.5f}, {(float)win_w, 0.5f},
                to_u32({0.60f, 0.58f, 1.00f, 0.18f}), 1.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Bottom separator
    dl->AddLine({0.0f, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);
    // Shimmer
    dl->AddRectFilledMultiColor(
        {0.0f, 0.0f}, {(float)win_w, H},
        to_u32({0.40f, 0.38f, 0.88f, 0.04f}),
        to_u32({0.40f, 0.38f, 0.88f, 0.04f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));
    end_panel();
    return (int)H;
}

// ── Top (title+tabs + toolbar) ────────────────────────────────────────
// Rows drawn depend on AppState::settings visibility flags:
//   show_tabs=true   -> draw the tab strip (TAB_BAR_HEIGHT_PX)
//   show_tabs=false  -> if toolbar visible, add 30 px traffic-light pad;
//                       if toolbar also hidden, draw the grab strip (32 px)
//   show_toolbar=true  -> draw the URL bar + nav row (CHROME_HEIGHT_PX)
//
// NOTE: When native_chrome is active this function is NOT called.
// It is kept for fallback / testing only.
int chrome_draw_top(AppState* st,
                    int win_w, int /*win_h*/,
                    bool& new_tab_requested,
                    int&  close_tab_idx) {
    const bool want_tabs    = st->settings.show_tabs;
    const bool want_toolbar = st->settings.show_toolbar;

    int h = 0;

    if (want_tabs) {
        h += draw_title_tab_row(st, win_w, new_tab_requested, close_tab_idx);
    } else {
        // No tab strip -- reset out-params.
        close_tab_idx   = -1;
        new_tab_requested = false;

        if (!want_toolbar) {
            // Grab-bar-only: just the drag strip.
            h += draw_grab_strip(win_w);
        } else {
            // Toolbar at top -- reserve 30 px so the macOS traffic lights
            // don't overlap the back/forward buttons on the left edge.
            h += 30;
        }
    }

    if (want_toolbar) {
        h += draw_toolbar(st, win_w, h);
    }

    // panels now drawn via chrome_draw_panels()
    return h;
}

// ── Floating panels only (called from main.mm with native chrome) ─────
void chrome_draw_panels(AppState* st,
                        float anchor_bm_x, float anchor_hist_x,
                        int   panel_top) {
    draw_bookmarks_panel(st, anchor_bm_x,   (float)panel_top);
    draw_history_panel  (st, anchor_hist_x, (float)panel_top);
}
