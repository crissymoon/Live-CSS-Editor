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
#include <regex>

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
static float s_more_btn_screen_x = 0.0f;
// Frame counters: prevent panels from closing on the same frame they open.
static int   s_bm_open_frame     = -1;
static int   s_hist_open_frame   = -1;
static int   s_more_open_frame   = -1;

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

struct GenieAnimState {
    bool   active = false;
    double start_time = 0.0;
    float  window_w = 0.0f;
    float  window_h = 0.0f;
};

static GenieAnimState s_genie_anim;

// Per-button animated hover value (0=cold, 1=hot). Kept outside
// draw_window_controls so it persists across frames and lerps smoothly.
static float s_tl_hover_t[3] = {0.0f, 0.0f, 0.0f};

static float ease_in_out_cubic(float t) {
    if (t < 0.5f) return 4.0f * t * t * t;
    float f = -2.0f * t + 2.0f;
    return 1.0f - (f * f * f) * 0.5f;
}

#ifndef __APPLE__
static void draw_window_controls(float y, float h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    GLFWwindow* win = glfwGetCurrentContext();
    const float r   = 6.5f;
    const float hit = 9.0f;
    const float d   = hit * 2.0f;
    const float x0  = 16.0f;
    const float gap = 8.0f;
    const float cy  = y + h * 0.5f;
    const float dt  = ImGui::GetIO().DeltaTime;

    struct ButtonSpec {
        const char* id;
        ImVec4 fill;
        const char* glyph;
    } buttons[] = {
        {"##tl_close", {0.973f, 0.529f, 0.451f, 1.0f}, "x"},
        {"##tl_min",   {0.984f, 0.753f, 0.259f, 1.0f}, "-"},
        {"##tl_zoom",  {0.204f, 0.827f, 0.600f, 1.0f}, "+"},
    };

    for (int i = 0; i < 3; ++i) {
        float cx = x0 + hit + (d + gap) * i;
        // Snap to half-pixel for crisp anti-aliased rendering on Windows
        float pcx = floorf(cx) + 0.5f;
        float pcy = floorf(cy) + 0.5f;
        ImGui::SetCursorPos({cx - hit, cy - hit});
        ImGui::InvisibleButton(buttons[i].id, {d, d});
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

        float tgt = hovered ? 1.0f : 0.0f;
        s_tl_hover_t[i] += (tgt - s_tl_hover_t[i]) * std::min(1.0f, dt * 14.0f);
        const float ht = s_tl_hover_t[i];

        ImVec4 fill = buttons[i].fill;
        fill.w = 0.88f + ht * 0.12f;
        dl->AddCircleFilled({pcx, pcy}, r, to_u32(fill), 24);
        dl->AddCircle({pcx, pcy}, r, to_u32({0.03f, 0.03f, 0.05f, 0.28f + ht * 0.27f}), 24, 1.0f);
        dl->AddCircleFilled({pcx, pcy}, r - 2.5f, to_u32({1.0f, 1.0f, 1.0f, 0.04f + ht * 0.06f}), 20);
        if (ht > 0.01f) {
            ImU32 gc = to_u32({0.10f, 0.10f, 0.12f, 0.95f * ht});
            const float gs = 2.8f;
            if (i == 0) {
                dl->AddLine({pcx - gs, pcy - gs}, {pcx + gs, pcy + gs}, gc, 1.5f);
                dl->AddLine({pcx + gs, pcy - gs}, {pcx - gs, pcy + gs}, gc, 1.5f);
            } else if (i == 1) {
                dl->AddLine({pcx - gs, pcy}, {pcx + gs, pcy}, gc, 1.5f);
            } else {
                dl->AddLine({pcx - gs, pcy}, {pcx + gs, pcy}, gc, 1.5f);
                dl->AddLine({pcx, pcy - gs}, {pcx, pcy + gs}, gc, 1.5f);
            }
        }

        if (clicked && win) {
            if (i == 0) {
                glfwSetWindowShouldClose(win, GLFW_TRUE);
            } else if (i == 1) {
                if (!s_genie_anim.active) {
                    s_genie_anim.active = true;
                    s_genie_anim.start_time = ImGui::GetTime();
                    s_genie_anim.window_w = (float)s_state->win_w;
                    s_genie_anim.window_h = (float)s_state->win_h;
                }
            } else {
                if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED)) glfwRestoreWindow(win);
                else glfwMaximizeWindow(win);
            }
        }
    }
}
#endif

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
    // Morphism: very subtle vertical gradient
    dl->AddRectFilledMultiColor(
        {0.0f, 0.0f}, {(float)win_w, H},
        to_u32({0.40f, 0.38f, 0.88f, 0.030f}),
        to_u32({0.40f, 0.38f, 0.88f, 0.030f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));
    // Single-pixel top edge
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    fg->AddLine({0.0f, 0.5f}, {(float)win_w, 0.5f},
                to_u32({0.60f, 0.58f, 1.00f, 0.10f}), 1.0f);

    draw_window_controls(0.0f, TOP_PAD);

    const float TL_GAP = (float)TRAFFIC_LIGHT_W;
    const float PLUS_W = 28.0f;
    float tabs_x = TL_GAP;
    float tabs_w = (float)win_w - TL_GAP - PLUS_W - 4.0f;


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

        // Tab background -- flat pill style matching macOS CSS
        float fill_alpha = dragging_this ? 0.25f : 1.0f;
        if (active) {
            ImVec4 bg = COL_TAB_ACT; bg.w = fill_alpha;
            dl->AddRectFilled(tMin, tMax, to_u32(bg), 6.0f);
        } else if (hov_rect) {
            dl->AddRectFilled(tMin, tMax, to_u32(COL_TAB_HOV), 6.0f);
        }
        // Subtle border (matches CSS: idle 10%, hover 20%, active 40%)
        {
            float ba = active ? 0.40f : (hov_rect ? 0.20f : 0.10f);
            ImU32 tb = to_u32(ImVec4{0.388f, 0.400f, 0.941f, ba});
            dl->AddRect(tMin, tMax, tb, 6.0f, ImDrawFlags_None, 1.0f);
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
        dl->AddRectFilled(gMin, gMax, to_u32({0.388f,0.400f,0.941f,0.20f}), 6.0f);
        dl->AddRect(gMin, gMax, to_u32(COL_ACCENT), 6.0f, ImDrawFlags_None, 1.5f);
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

// ── URL resolution (mirrors chrome.js resolveUrl) ─────────────────────
// If the user typed a bare hostname, IP, or search term, prepend the right
// scheme or redirect to a Google search -- just like the macOS chrome does.
static std::string resolve_url(const std::string& raw) {
    // Trim whitespace
    size_t start = raw.find_first_not_of(" \t\r\n");
    size_t end   = raw.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    std::string v = raw.substr(start, end - start + 1);
    if (v.empty()) return {};

    // Already has a scheme
    if (v.find("://") != std::string::npos) return v;
    // about: pages
    if (v.size() >= 6 && v.substr(0, 6) == "about:") return v;

    // localhost or localhost:port
    if (v.substr(0, 9) == "localhost") return "http://" + v;

    bool has_space = v.find(' ') != std::string::npos;
    bool has_dot   = v.find('.') != std::string::npos;

    // Plain IP address: 192.168.1.1 or 192.168.1.1:8080/path
    static const std::regex re_ip(R"(^\d{1,3}(\.\d{1,3}){3}(:\d+)?(/.*)?$)");
    if (std::regex_match(v, re_ip)) return "https://" + v;

    // Hostname-like: no spaces, has a dot, ends with a short alpha TLD
    static const std::regex re_tld(R"(\.[a-zA-Z]{2,}(/.*)?$)");
    if (!has_space && has_dot && std::regex_search(v, re_tld))
        return "https://" + v;

    // 127.0.0.1 with port
    if (v.substr(0, 4) == "127." || v.substr(0, 8) == "192.168.")
        return "http://" + v;

    // Everything else is a search query
    // URL-encode the query
    std::string encoded;
    for (char c : v) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return "https://www.google.com/search?q=" + encoded;
}

// ── Toolbar (URL bar + nav row) ───────────────────────────────────────
// Layout matches mac chrome: back/fwd/reload | [lock | url input | < info >] | ...

static int draw_toolbar(AppState* st, int win_w, int y_offset) {
    const float H   = (float)CHROME_HEIGHT_PX;
    const float pad = 8.0f;
    const float btn = 28.0f;

    begin_panel("##toolbar", 0, (float)y_offset, (float)win_w, H, COL_SURFACE);

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2      wpos = ImGui::GetWindowPos();

    // Bottom hair line
    dl->AddLine({0, wpos.y + H - 1}, {(float)win_w, wpos.y + H - 1},
                to_u32(COL_SEP), 1.0f);
    // Morphism shimmer
    dl->AddRectFilledMultiColor(
        {0.0f, wpos.y}, {(float)win_w, wpos.y + H},
        to_u32({0.38f, 0.36f, 0.88f, 0.025f}),
        to_u32({0.38f, 0.36f, 0.88f, 0.025f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));

    Tab* tab = st->current_tab();
    // Vertically center ALL toolbar items using the same btn height
    float cy = (H - btn) * 0.5f;
    ImGui::SetCursorPos({pad, cy});

    // ── Back ─────────────────────────────────────────────────────────
    ImGui::BeginDisabled(!tab || !tab->can_back);
    if (ghost_btn("##back", "<", btn, btn, "Back (Ctrl+[)") && tab)
        st->push_nav(tab->id, "__back__");
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    // ── Forward ───────────────────────────────────────────────────────
    ImGui::BeginDisabled(!tab || !tab->can_forward);
    if (ghost_btn("##fwd", ">", btn, btn, "Forward (Ctrl+])") && tab)
        st->push_nav(tab->id, "__forward__");
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    // ── Reload / Stop ─────────────────────────────────────────────────
    if (tab && tab->loading) {
        if (ghost_btn("##stop", "x", btn, btn, "Stop (Esc)"))
            st->push_nav(tab->id, "__stop__");
    } else {
        if (ghost_btn("##reload", "r", btn, btn, "Reload (Ctrl+R)") && tab)
            st->push_nav(tab->id, "__reload__");
    }
    ImGui::SameLine(0, 6);

    // ── URL bar area ──────────────────────────────────────────────────
    // The URL bar is a single visual container: [lock] [input or info] [</>]
    float url_area_x = ImGui::GetCursorPosX();   // local x after nav buttons
    float lock_w     = 22.0f;
    float info_tog_w = 18.0f;
    float more_btn_w = btn + 2.0f;
    // Full width of the URL bar background rect (lock + input + toggle)
    float url_bar_w  = (float)win_w - url_area_x - more_btn_w - pad * 2.0f;
    // Width available for the text input (URL bar minus lock and toggle)
    float url_inp_w  = url_bar_w - lock_w - 4.0f - info_tog_w - 2.0f;

    // Get screen coords for custom background before advancing cursor
    ImVec2 bar_sp = ImGui::GetCursorScreenPos();
    float  bar_fh = btn;  // match nav button height for alignment

    // Draw unified URL bar background (lock + input + info toggle share one pill)
    dl->AddRectFilled(bar_sp,
                      {bar_sp.x + url_bar_w, bar_sp.y + bar_fh},
                      to_u32(COL_BASE), 6.0f);

    // Progress bar at bottom of URL bar background (when loading)
    if (tab && tab->loading && tab->progress > 0.0f) {
        dl->AddRectFilled({bar_sp.x, bar_sp.y + bar_fh - 2},
                          {bar_sp.x + url_bar_w, bar_sp.y + bar_fh},
                          to_u32({0.10f, 0.10f, 0.16f, 1.0f}), 2.0f);
        ImVec4 pg = COL_ACCENT; pg.w = 0.60f;
        dl->AddRectFilled({bar_sp.x, bar_sp.y + bar_fh - 2},
                          {bar_sp.x + url_bar_w * tab->progress, bar_sp.y + bar_fh},
                          to_u32(pg), 2.0f);
    }

    // ── Security indicator ────────────────────────────────────────────
    {
        const std::string& turl = tab ? tab->url : std::string();
        bool https = turl.size() >= 8 && turl.substr(0, 8) == "https://";
        bool http  = turl.size() >= 7 && turl.substr(0, 7) == "http://";
        ImVec4 sec_col = https ? COL_OK : (http ? COL_WARN : COL_TEXT_DIM);
        ImVec2 sp2 = ImGui::GetCursorScreenPos();
        float  fh2 = bar_fh;
        float  dcx = sp2.x + lock_w * 0.5f;
        float  dcy = sp2.y + fh2 * 0.5f;
        if (https) {
            dl->AddCircleFilled({dcx, dcy}, 4.5f, to_u32(sec_col), 12);
        } else {
            dl->AddCircle({dcx, dcy}, 4.5f, to_u32(sec_col), 12, 1.5f);
            if (http)
                dl->AddCircleFilled({dcx, dcy}, 1.5f, to_u32(sec_col), 8);
        }
        ImGui::InvisibleButton("##sec_ind", {lock_w, fh2});
        const char* sec_tip = https ? "Secure connection (HTTPS)"
                             : http  ? "Not secure (HTTP)"
                                     : nullptr;
        if (sec_tip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", sec_tip);
        ImGui::SameLine(0, 4);
    }

    // ── URL input (always rendered) ──────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  {0.0f, 0.0f, 0.0f, 0.0f});
        // Increase vertical padding so InputText height matches the URL bar pill
        float url_pad_y = (btn - ImGui::GetFontSize()) * 0.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8.0f, url_pad_y});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::SetNextItemWidth(url_inp_w);

        if (st->focus_url_next_frame) {
            ImGui::SetKeyboardFocusHere();
            st->focus_url_next_frame = false;
        }

        if (tab) {
            bool activated = ImGui::InputText("##url", tab->url_buf, URL_BUF_SIZE,
                                             ImGuiInputTextFlags_EnterReturnsTrue |
                                             ImGuiInputTextFlags_AutoSelectAll);
            if (activated) {
                std::string resolved = resolve_url(tab->url_buf);
                if (!resolved.empty()) {
                    tab->url = resolved;
                    snprintf(tab->url_buf, URL_BUF_SIZE, "%s", resolved.c_str());
                    st->push_nav(tab->id, tab->url);
                    fprintf(stderr, "[chrome] URL navigate: %s\n", tab->url.c_str());
                }
            }
            if (!ImGui::IsItemActive() && tab->url != tab->url_buf)
                snprintf(tab->url_buf, URL_BUF_SIZE, "%s", tab->url.c_str());
        } else {
            char empty[2] = {};
            ImGui::InputText("##url", empty, sizeof(empty));
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 2);
    }

    // ── Info toggle button (< always at right edge of URL bar) ───────
    {
        bool slide_open = st->show_info_slide;
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_u32(COL_RAISED));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  to_u32(COL_ACCENT_LO));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, 0.0f});
        if (ImGui::Button("##info_tog", {info_tog_w, bar_fh}))
            st->show_info_slide = !st->show_info_slide;
        {
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            const char* lbl = slide_open ? ">" : "<";
            ImVec2 ts = ImGui::CalcTextSize(lbl);
            dl->AddText({(bmin.x + bmax.x - ts.x) * 0.5f,
                         (bmin.y + bmax.y - ts.y) * 0.5f},
                        to_u32(slide_open ? COL_ACCENT : COL_TEXT_DIM), lbl);
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(slide_open ? "Hide status info" : "Status & viewport");
    }

    // ── Info slide overlay (slides over URL from the right) ──────────
    // Like the macOS CSS: position:absolute; right:0; transform:translateX
    // Draws on top of the URL text with an opaque background + shadow fade.
    if (st->show_info_slide) {
        float url_left  = bar_sp.x + lock_w + 4.0f;
        float url_right = url_left + url_inp_w;

        // Measure info content width
        float dot_r   = 3.5f;
        float lbl_gap = 9.0f;
        float sep_gap = 8.0f;
        float php_tw  = ImGui::CalcTextSize("php").x;
        float js_tw   = ImGui::CalcTextSize("js").x;
        int content_h = st->win_h - TOTAL_CHROME_TOP
                        - (st->show_more_panel ? DRAWER_HEIGHT_PX : 0)
                        - STATUS_HEIGHT_PX;
        char vp_str[32];
        snprintf(vp_str, sizeof(vp_str), "%d x %d", st->win_w, content_h);
        float vp_tw = ImGui::CalcTextSize(vp_str).x;
        // Total: padding + php_dot + php_lbl + sep + js_dot + js_lbl + sep + vp + padding
        float info_cw = 8.0f + (dot_r*2+lbl_gap+php_tw) + sep_gap*2 + (dot_r*2+lbl_gap+js_tw) + sep_gap*2 + vp_tw + 8.0f;
        float info_w  = std::min(info_cw, url_inp_w);
        float info_x  = url_right - info_w;

        // Shadow gradient on left edge (fade from transparent to opaque)
        float shw = 16.0f;
        dl->AddRectFilledMultiColor(
            {info_x - shw, bar_sp.y + 1.0f}, {info_x, bar_sp.y + bar_fh - 1.0f},
            to_u32({COL_BASE.x, COL_BASE.y, COL_BASE.z, 0.0f}),
            to_u32(COL_BASE), to_u32(COL_BASE),
            to_u32({COL_BASE.x, COL_BASE.y, COL_BASE.z, 0.0f}));

        // Opaque background covering URL text
        dl->AddRectFilled({info_x, bar_sp.y + 1.0f},
                          {url_right, bar_sp.y + bar_fh - 1.0f},
                          to_u32(COL_BASE));

        // Draw status indicators
        float ic_cy = bar_sp.y + bar_fh * 0.5f;
        float cx    = info_x + 8.0f;
        float th    = ImGui::GetTextLineHeight();

        // PHP dot + label
        dl->AddCircleFilled({cx + dot_r, ic_cy}, dot_r,
                            to_u32(st->php_server_ok ? COL_OK : COL_BAD), 8);
        dl->AddText({cx + lbl_gap, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), "php");
        cx += lbl_gap + php_tw + sep_gap;

        // Separator
        dl->AddLine({cx, bar_sp.y + 6.0f}, {cx, bar_sp.y + bar_fh - 6.0f},
                    to_u32(COL_SEP), 1.0f);
        cx += sep_gap;

        // JS dot + label
        bool js_ok = !tab || tab->js_enabled;
        dl->AddCircleFilled({cx + dot_r, ic_cy}, dot_r,
                            to_u32(js_ok ? COL_OK : COL_WARN), 8);
        dl->AddText({cx + lbl_gap, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), "js");
        cx += lbl_gap + js_tw + sep_gap;

        // Separator
        dl->AddLine({cx, bar_sp.y + 6.0f}, {cx, bar_sp.y + bar_fh - 6.0f},
                    to_u32(COL_SEP), 1.0f);
        cx += sep_gap;

        // Viewport size
        dl->AddText({cx, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), vp_str);
    }

    // Draw URL bar border on top of all URL bar widgets
    dl->AddRect(bar_sp,
                {bar_sp.x + url_bar_w, bar_sp.y + bar_fh},
                to_u32({0.388f, 0.400f, 0.941f, 0.28f}), 6.0f, 0, 1.0f);

    ImGui::SameLine(0, 4);

    // ── Three-dot menu ────────────────────────────────────────────────
    {
        bool more_open = st->show_more_panel;
        if (more_open) {
            ImGui::PushStyleColor(ImGuiCol_Button,        COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_MID);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::Button("##more", {btn, btn})) {
            st->show_more_panel      = !st->show_more_panel;
            st->more_subpanel        = 0;
            st->show_bookmarks_panel = false;
            st->show_history_panel   = false;
            if (st->show_more_panel)
                s_more_open_frame = ImGui::GetFrameCount();
        }
        s_more_btn_screen_x = ImGui::GetItemRectMax().x;
        {
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            ImU32  gc   = to_u32(more_open ? COL_ACCENT : COL_TEXT_DIM);
            float  my2  = (bmin.y + bmax.y) * 0.5f;
            float  cx   = (bmin.x + bmax.x) * 0.5f;
            dl->AddCircleFilled({cx - 5.5f, my2}, 1.9f, gc, 8);
            dl->AddCircleFilled({cx,        my2}, 1.9f, gc, 8);
            dl->AddCircleFilled({cx + 5.5f, my2}, 1.9f, gc, 8);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Tools and settings");
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

// ── Inline more-panel drawer ──────────────────────────────────────────
// Drawn as a pinned panel WITHIN the chrome area (above WebView2).
// Returns the height added so chrome_draw_top can include it in chrome_top,
// which causes reposition_webviews to push WebView2 down accordingly.

static int draw_more_drawer(AppState* st, int win_w, int y_offset) {
    if (!st->show_more_panel) return 0;

    const float H   = (float)DRAWER_HEIGHT_PX;
    const float pad = 8.0f;

    begin_panel("##drawer_panel", 0, (float)y_offset, (float)win_w, H, COL_BASE);

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2      wpos = ImGui::GetWindowPos();

    // Top border
    dl->AddLine({0.0f, wpos.y}, {(float)win_w, wpos.y}, to_u32(COL_SEP), 1.0f);

    const float item_h = 28.0f;
    const float col_w  = ((float)win_w - pad * 2.0f - 4.0f) * 0.5f;

    ImGui::SetCursorPos({pad, 4.0f});
    ImGui::BeginChild("##drwcont", {(float)win_w - pad * 2.0f, H - 10.0f},
                      false, ImGuiWindowFlags_None);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4.0f, 3.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Header,        COL_RAISED);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COL_TAB_HOV);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  COL_ACCENT_MID);

    Tab* tab = st->current_tab();

    // Helper: selectable item that returns true when clicked and not disabled
    auto DR_ITEM = [&](const char* label, float w, bool disabled) -> bool {
        ImGui::BeginDisabled(disabled);
        bool hit = ImGui::Selectable(label, false, 0, {w, item_h});
        ImGui::EndDisabled();
        return hit && !disabled;
    };

    if (st->more_subpanel == 0) {
        // ── Main grid ─────────────────────────────────────────────────

        // Navigation: Back | Forward (2-col), then Reload/Stop (full)
        if (DR_ITEM("< Back##drn",    col_w - 2.0f, !tab || !tab->can_back)) {
            if (tab) st->push_nav(tab->id, "__back__");
            st->show_more_panel = false;
        }
        ImGui::SameLine(0, 4);
        if (DR_ITEM("> Forward##drn", col_w - 2.0f, !tab || !tab->can_forward)) {
            if (tab) st->push_nav(tab->id, "__forward__");
            st->show_more_panel = false;
        }
        if (tab && tab->loading) {
            if (DR_ITEM("Stop##drn", 0, false)) {
                st->push_nav(tab->id, "__stop__");
                st->show_more_panel = false;
            }
        } else {
            if (DR_ITEM("Reload##drn", 0, false)) {
                if (tab) st->push_nav(tab->id, "__reload__");
                st->show_more_panel = false;
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        // Developer Tools (collapsible accordion)
        if (ImGui::CollapsingHeader("Developer Tools##drh")) {
            ImGui::Indent(8.0f);
            if (DR_ITEM(st->dev_tools_open
                        ? "Hide DevTools##dr" : "Show DevTools##dr", 0, !tab)) {
                if (tab) st->push_nav(tab->id, "__devtools__");
                st->show_more_panel = false;
            }
            if (tab) {
                if (DR_ITEM(tab->js_enabled
                            ? "Disable JavaScript##dr"
                            : "Enable JavaScript##dr", 0, false)) {
                    tab->js_enabled = !tab->js_enabled;
                    st->push_nav(tab->id, tab->js_enabled ? "__js_on__" : "__js_off__");
                    st->show_more_panel = false;
                }
            }
            if (st->php_port > 0) {
                auto open_app = [&](const char* slug) {
                    if (tab) {
                        char url[256];
                        snprintf(url, sizeof(url), "http://127.0.0.1:%d/%s/",
                                 st->php_port, slug);
                        st->push_nav(tab->id, url);
                    }
                    st->show_more_panel = false;
                };
                if (DR_ITEM("DOM Inspector##dr",  col_w-2.0f, false)) open_app("dom-inspector");
                ImGui::SameLine(0, 4);
                if (DR_ITEM("Page Inspector##dr", col_w-2.0f, false)) open_app("page-inspector");
                if (DR_ITEM("Page Speed##dr",     col_w-2.0f, false)) open_app("page-speed");
                ImGui::SameLine(0, 4);
                if (DR_ITEM("Page Builder##dr",   col_w-2.0f, false)) open_app("page-builder-debug");
            }
            ImGui::Unindent(8.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        // Bookmarks: [Bookmark Page | Bookmarks] (2-col), History (full)
        {
            bool is_bm = false;
            if (tab && !tab->url.empty())
                for (auto& b : st->bookmarks)
                    if (b.url == tab->url) { is_bm = true; break; }
            if (DR_ITEM(is_bm ? "Remove Bookmark##dr" : "Bookmark Page##dr",
                        col_w - 2.0f, !tab)) {
                if (tab) st->push_nav(tab->id, "__bookmark_toggle__");
                st->show_more_panel = false;
            }
            ImGui::SameLine(0, 4);
            if (DR_ITEM("Bookmarks##dr", col_w - 2.0f, false))
                st->more_subpanel = 1;
        }
        if (DR_ITEM("History##dr", 0, false))
            st->more_subpanel = 2;

        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        // Tools: PDF | Image (2-col), Editz (full)
        if (st->php_port > 0) {
            auto open_app = [&](const char* slug) {
                if (tab) {
                    char url[256];
                    snprintf(url, sizeof(url), "http://127.0.0.1:%d/%s/",
                             st->php_port, slug);
                    st->push_nav(tab->id, url);
                }
                st->show_more_panel = false;
            };
            if (DR_ITEM("PDF Fill & Sign##dr", col_w-2.0f, false)) open_app("pdf-sign");
            ImGui::SameLine(0, 4);
            if (DR_ITEM("Image Generator##dr", col_w-2.0f, false)) open_app("image-gen");
            if (DR_ITEM("Editz##dr", 0, false)) open_app("editz");
        }

        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        // Viewport Size (collapsible accordion)
        if (ImGui::CollapsingHeader("Viewport Size##drh")) {
            struct VpPreset { int w, h; const char* label; };
            static const VpPreset presets[] = {
                {375,  667,  "iPhone SE  375 x 667##vp"},
                {390,  844,  "iPhone 14  390 x 844##vp"},
                {430,  932,  "iPhone Plus  430 x 932##vp"},
                {768,  1024, "iPad  768 x 1024##vp"},
                {1024, 768,  "iPad land.  1024 x 768##vp"},
                {1280, 800,  "Laptop  1280 x 800##vp"},
                {1366, 768,  "Laptop  1366 x 768##vp"},
                {1440, 900,  "Laptop HD  1440 x 900##vp"},
                {1920, 1080, "Desktop FHD  1920 x 1080##vp"},
                {2560, 1440, "Desktop QHD  2560 x 1440##vp"},
            };
            GLFWwindow* win = glfwGetCurrentContext();
            ImGui::Indent(8.0f);
            for (auto& p : presets) {
                if (DR_ITEM(p.label, 0, false)) {
                    if (win) glfwSetWindowSize(win, p.w, p.h);
                    st->show_more_panel = false;
                }
            }
            ImGui::Unindent(8.0f);
        }

    } else if (st->more_subpanel == 1) {
        // ── Bookmarks sub-panel ───────────────────────────────────────
        if (ImGui::Button("< Back##bmsub")) st->more_subpanel = 0;
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
        ImGui::TextUnformatted("Bookmarks");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        if (st->bookmarks.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
            ImGui::TextUnformatted("No bookmarks yet.");
            ImGui::PopStyleColor();
        } else {
            for (int i = (int)st->bookmarks.size() - 1; i >= 0; i--) {
                auto& bm = st->bookmarks[i];
                const std::string& nm = bm.title.empty() ? bm.url : bm.title;
                char lbl[104];
                snprintf(lbl, sizeof(lbl), "%.80s##bmsub%d", nm.c_str(), i);
                if (ImGui::Selectable(lbl)) {
                    if (tab) st->push_nav(tab->id, bm.url);
                    st->more_subpanel = 0;
                    st->show_more_panel = false;
                }
            }
        }

    } else {
        // ── History sub-panel ─────────────────────────────────────────
        if (ImGui::Button("< Back##histsub")) st->more_subpanel = 0;
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
        ImGui::TextUnformatted("History");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        ImGui::SetNextItemWidth((float)win_w - pad * 2.0f - 20.0f);
        ImGui::InputTextWithHint("##hfilt2", "Search history...",
                                 st->history_filter, sizeof(st->history_filter));
        ImGui::PushStyleColor(ImGuiCol_Separator, COL_SEP);
        ImGui::Separator();
        ImGui::PopStyleColor();

        std::string filt(st->history_filter);
        for (auto& c : filt) c = (char)tolower((unsigned char)c);

        int shown = 0;
        for (int i = (int)st->history.size() - 1; i >= 0 && shown < 100; i--) {
            auto& he = st->history[i];
            if (!filt.empty()) {
                std::string u = he.url, t2 = he.title;
                for (auto& c : u)  c = (char)tolower((unsigned char)c);
                for (auto& c : t2) c = (char)tolower((unsigned char)c);
                if (u.find(filt) == std::string::npos &&
                    t2.find(filt) == std::string::npos)
                    continue;
            }
            const std::string& nm = he.title.empty() ? he.url : he.title;
            char lbl[104];
            snprintf(lbl, sizeof(lbl), "%.80s##histsub%d", nm.c_str(), i);
            if (ImGui::Selectable(lbl)) {
                if (tab) st->push_nav(tab->id, he.url);
                st->more_subpanel = 0;
                st->show_more_panel = false;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("%s", he.url.c_str());
            shown++;
        }
        if (st->history.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
            ImGui::TextUnformatted("No history yet.");
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    // Close drawer on click outside (but not on the frame it was opened)
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && ImGui::GetFrameCount() != s_more_open_frame)
        st->show_more_panel = false;

    end_panel();
    return (int)H;
}

// ── Button screen-X getters (used by native_chrome_stub on non-Apple) ────

float chrome_bm_btn_x()   { return s_bm_btn_screen_x; }
float chrome_hist_btn_x()  { return s_hist_btn_screen_x; }

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

    // Inline more-panel drawer: extends chrome area so WebView2 is pushed down
    h += draw_more_drawer(st, win_w, h);

    // panels now drawn via chrome_draw_panels()
    return h;
}

// ── Floating panels only (called from main.mm with native chrome) ─────
void chrome_draw_panels(AppState* st,
                        float anchor_bm_x, float anchor_hist_x,
                        int   panel_top) {
#if defined(__APPLE__)
    draw_bookmarks_panel(st, anchor_bm_x,   (float)panel_top);
    draw_history_panel  (st, anchor_hist_x, (float)panel_top);
#else
    // On Windows/Linux, bookmarks and history live inside the inline drawer.
    // Floating ImGui panels would be hidden behind the WebView2 HWND.
    (void)st; (void)anchor_bm_x; (void)anchor_hist_x; (void)panel_top;
#endif
}
