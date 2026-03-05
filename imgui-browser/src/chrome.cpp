// chrome.cpp -- Dear ImGui browser chrome
// Modern dark design for Crissy's Style Tool.
// Single header row: title pill (left) + tabs (centre) + nav/URL (row 2).

#include "chrome.h"
#include "imgui.h"
#include <cstdio>
#include <string>
#include <algorithm>

static AppState* s_state = nullptr;

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
// Combines the draggable title bar (with app name pill on the left) and
// the tab strip in the same single row to save vertical space.

static int draw_title_tab_row(AppState* st, int win_w,
                               bool& new_tab_req, int& close_idx) {
    close_idx   = -1;
    new_tab_req = false;

    const float H = (float)TAB_BAR_HEIGHT_PX;

    // Full-width panel, sits at y=0
    begin_panel("##titlerow", 0, 0, (float)win_w, H, COL_SURFACE);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Bottom separator line
    dl->AddLine({0, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);

    // The NSWindow "traffic light" buttons occupy ~82px on the left.
    // We start our content after that gap so tab text is never obscured.
    const float TL_GAP  = (float)TRAFFIC_LIGHT_W;
    const float NEW_BTN = 28.0f;
    const float PLUS_W  = NEW_BTN;
    float       tabs_x  = TL_GAP;
    float       tabs_w  = (float)win_w - TL_GAP - PLUS_W - 4.0f;

    int   n        = (int)st->tabs.size();
    float max_tw   = 180.0f;
    float min_tw   = 64.0f;
    float tab_w    = std::min(max_tw, std::max(min_tw, tabs_w / n));

    // Title pill -- only visible when tabs are narrow enough that it fits
    // OR when there is only one tab (there is always room on the far right)
    {
        const char* name = "Crissy's Style Tool";
        ImVec2      ns   = ImGui::CalcTextSize(name);
        float       pill_w = ns.x + 20.0f;
        float       pill_h = 18.0f;
        float       pill_x = (float)win_w * 0.5f - pill_w * 0.5f;
        float       pill_y = (H - pill_h) * 0.5f;

        // Only draw the pill if it does not overlap any tab
        bool fits = (pill_x > tabs_x + n * tab_w + 8.0f) ||
                    (pill_x + pill_w < tabs_x - 8.0f);
        // Always draw it in the far-right zone if there is enough room
        float right_zone_x = tabs_x + n * tab_w + 16.0f;
        if (!fits && (float)win_w - right_zone_x > pill_w + 16.0f) {
            pill_x = right_zone_x;
            fits   = true;
        }
        if (fits) {
            // Pill background
            dl->AddRectFilled({pill_x, pill_y},
                              {pill_x + pill_w, pill_y + pill_h},
                              to_u32(COL_ACCENT_LO), 9.0f);
            // Pill border
            dl->AddRect({pill_x, pill_y},
                        {pill_x + pill_w, pill_y + pill_h},
                        to_u32(COL_SEP), 9.0f, 0, 1.0f);
            // Pill text
            dl->AddText({pill_x + 10.0f,
                         pill_y + (pill_h - ImGui::GetTextLineHeight()) * 0.5f},
                        to_u32(COL_ACCENT),
                        name);
        }
    }

    // Tabs
    float cx = tabs_x;
    ImGui::SetCursorPos({cx, 0});

    for (int i = 0; i < n; i++) {
        auto& tab    = st->tabs[i];
        bool  active = (i == st->active_tab);

        ImGui::PushID(tab.id);

        float close_w = active ? 18.0f : 0.0f;  // close only visible on active tab
        float label_w = tab_w - close_w - (active ? 4.0f : 0.0f);

        // Tab background (rounded top corners only via manual rect)
        ImVec2 tMin = {cx, 0};
        ImVec2 tMax = {cx + tab_w, H};
        if (active) {
            dl->AddRectFilled(tMin, tMax, to_u32(COL_TAB_ACT), 6.0f,
                              ImDrawFlags_RoundCornersTop);
            // Accent top edge line
            dl->AddLine({tMin.x + 4, tMin.y + 1},
                        {tMax.x - 4, tMin.y + 1},
                        to_u32(COL_ACCENT), 2.0f);
        }

        // Tab click area
        ImGui::SetCursorPos({cx + 4.0f, (H - ImGui::GetTextLineHeight()) * 0.5f - 1});
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? ImVec4{0,0,0,0} : COL_TAB_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0,0,0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});

        char label[52];
        snprintf(label, sizeof(label), "%.28s##tab%d", tab.title.c_str(), tab.id);
        // Strip the ##tab... from displayed text
        char disp[36];
        snprintf(disp, sizeof(disp), "%.28s", tab.title.c_str());

        // Invisible button for click detection
        ImGui::SetCursorPos({cx, 0});
        char btn_id[16];
        snprintf(btn_id, sizeof(btn_id), "##tb%d", tab.id);
        if (ImGui::InvisibleButton(btn_id, {tab_w - close_w, H}))
            st->active_tab = i;
        bool hov = ImGui::IsItemHovered();
        if (hov && !active)
            dl->AddRectFilled(tMin, tMax, to_u32(COL_TAB_HOV), 6.0f,
                              ImDrawFlags_RoundCornersTop);

        // Draw truncated tab title
        {
            ImVec2 ts = ImGui::CalcTextSize(disp);
            float ty = (float)(H - ImGui::GetTextLineHeight()) * 0.5f;
            float tx = cx + 10.0f;
            float max_x = cx + label_w;
            if (ts.x > label_w - 8.0f) {
                // Clip via AddText scissor
                ImGui::PushClipRect({tx, ty}, {max_x, ty + ImGui::GetTextLineHeight() + 2}, true);
                dl->AddText({tx, ty}, to_u32(active ? COL_TEXT : COL_TEXT_DIM), disp);
                ImGui::PopClipRect();
            } else {
                dl->AddText({tx, ty}, to_u32(active ? COL_TEXT : COL_TEXT_DIM), disp);
            }
        }

        // LoadingIndicator dot pulse (simple)
        if (tab.loading) {
            float dot_x = cx + tab_w - close_w - 12.0f;
            float dot_y = H * 0.5f;
            dl->AddCircleFilled({dot_x, dot_y}, 3.5f, to_u32(COL_ACCENT), 8);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        // Close button (only on active tab, on hover of any tab it appears)
        if (active && n > 1) {
            ImGui::SetCursorPos({cx + tab_w - close_w - 1.0f,
                                 (H - close_w) * 0.5f});
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f,0.3f,0.3f,0.30f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.9f,0.3f,0.3f,0.55f});
            ImGui::PushStyleColor(ImGuiCol_Text,          COL_TEXT_DIM);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, close_w * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0, 0});
            char cid[16];
            snprintf(cid, sizeof(cid), "##cx%d", tab.id);
            if (ImGui::Button(cid, {close_w, close_w}))
                close_idx = i;
            // Draw x glyph manually
            {
                ImVec2 bmin = ImGui::GetItemRectMin();
                ImVec2 bmax = ImGui::GetItemRectMax();
                float m = 5.0f;
                ImU32 xc = ImGui::IsItemHovered()
                    ? to_u32({1.0f,1.0f,1.0f,0.9f})
                    : to_u32(COL_TEXT_DIM);
                dl->AddLine({bmin.x+m, bmin.y+m}, {bmax.x-m, bmax.y-m}, xc, 1.5f);
                dl->AddLine({bmax.x-m, bmin.y+m}, {bmin.x+m, bmax.y-m}, xc, 1.5f);
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", tab.url.c_str());

        cx += tab_w;
        ImGui::PopID();
    }

    // "+" new tab button
    ImGui::SetCursorPos({cx + 2.0f, (H - 22.0f) * 0.5f});
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0, 0});
    if (ImGui::Button("##newtab", {22.0f, 22.0f})) new_tab_req = true;
    {
        ImVec2 c2 = {(ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
                     (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f};
        float arm = 5.0f;
        ImU32 pc  = to_u32(ImGui::IsItemHovered() ? COL_ACCENT : COL_TEXT_DIM);
        dl->AddLine({c2.x-arm, c2.y}, {c2.x+arm, c2.y}, pc, 1.5f);
        dl->AddLine({c2.x, c2.y-arm}, {c2.x, c2.y+arm}, pc, 1.5f);
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

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

    // Bottom hair line
    dl->AddLine({0, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);

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
    float devtool_w = btn + 4.0f;
    float url_w     = (float)win_w - url_x - devtool_w - pad * 2.0f;

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

    // Focus if requested (e.g. Cmd+L)
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

    end_panel();
    return (int)H;
}

// ── Status bar ────────────────────────────────────────────────────────

int chrome_draw_bottom(AppState* st, int win_w, int win_h) {
    const float H = (float)STATUS_HEIGHT_PX;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BASE);
    begin_panel("##status", 0, (float)(win_h - (int)H), (float)win_w, H, COL_BASE);
    ImGui::PopStyleColor();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine({0, 0}, {(float)win_w, 0}, to_u32(COL_SEP), 1.0f);

    float cy = (H - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPos({10.0f, cy});

    const std::string& disp = st->hover_url.empty() ? st->status_text : st->hover_url;
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
    ImGui::TextUnformatted(disp.c_str());
    ImGui::PopStyleColor();

    // Right side: fps + server LEDs
    char fps_str[80];
    snprintf(fps_str, sizeof(fps_str), "DEV %.0f  ENG %.0f  HOST %.0f",
             st->fps_wkwv, st->fps_engine, st->fps_host.fps_avg());
    float tw = ImGui::CalcTextSize(fps_str).x;

    ImVec4 fc = (st->fps_wkwv >= 50) ? COL_OK
              : (st->fps_wkwv >= 30) ? COL_WARN
              : (st->fps_wkwv >  0 ) ? COL_BAD
                                     : COL_TEXT_DIM;
    ImGui::SameLine((float)win_w - tw - 76.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, fc);
    ImGui::TextUnformatted(fps_str);
    ImGui::PopStyleColor();

    // Server LED dots
    float lx = (float)win_w - 58.0f;
    float ly = H * 0.5f;
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

// ── Top (title+tabs + toolbar) ────────────────────────────────────────

int chrome_draw_top(AppState* st,
                    int win_w, int win_h,
                    bool& new_tab_requested,
                    int&  close_tab_idx) {
    int h = 0;
    h += draw_title_tab_row(st, win_w, new_tab_requested, close_tab_idx);
    h += draw_toolbar(st, win_w, h);
    return h;
}
