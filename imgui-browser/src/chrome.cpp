// chrome.cpp -- Dear ImGui browser chrome
// Tab bar, address bar with back/forward/reload/stop, progress bar, status bar.
// All immediate-mode -- no retained state beyond AppState.

#include "chrome.h"
#include "imgui.h"
#include <cstdio>
#include <string>
#include <algorithm>

static AppState* s_state = nullptr;

// ── Palette ───────────────────────────────────────────────────────────
static const ImVec4 COL_BG         = {0.047f, 0.047f, 0.063f, 1.0f};  // #0C0C10
static const ImVec4 COL_CHROME     = {0.075f, 0.075f, 0.102f, 1.0f};  // #13131A
static const ImVec4 COL_TAB_ACTIVE = {0.118f, 0.118f, 0.165f, 1.0f};  // #1E1E2A
static const ImVec4 COL_TAB_HOVER  = {0.098f, 0.098f, 0.137f, 1.0f};
static const ImVec4 COL_TAB_IDLE   = {0.059f, 0.059f, 0.082f, 1.0f};
static const ImVec4 COL_ACCENT     = {0.388f, 0.400f, 0.941f, 1.0f};  // #6366F1
static const ImVec4 COL_TEXT       = {0.886f, 0.910f, 0.941f, 1.0f};  // #E2E8F0
static const ImVec4 COL_DIM        = {0.392f, 0.447f, 0.514f, 1.0f};  // #64748B
static const ImVec4 COL_OK         = {0.204f, 0.827f, 0.600f, 1.0f};  // #34D399
static const ImVec4 COL_WARN       = {0.984f, 0.753f, 0.259f, 1.0f};  // #FBBF24
static const ImVec4 COL_BAD        = {0.973f, 0.529f, 0.451f, 1.0f};  // #F87171
static const ImVec4 COL_BORDER     = {0.118f, 0.118f, 0.220f, 1.0f};  // #1E1E38

void chrome_apply_theme() {
    ImGuiStyle& s   = ImGui::GetStyle();
    s.WindowPadding = {4, 4};
    s.FramePadding  = {6, 4};
    s.ItemSpacing   = {6, 4};
    s.ScrollbarSize = 10;
    s.GrabMinSize   = 8;
    s.WindowRounding   = 0;
    s.FrameRounding    = 4;
    s.ScrollbarRounding= 4;
    s.TabRounding      = 4;
    s.WindowBorderSize = 0;
    s.FrameBorderSize  = 0;

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]           = COL_CHROME;
    c[ImGuiCol_ChildBg]            = COL_BG;
    c[ImGuiCol_PopupBg]            = COL_CHROME;
    c[ImGuiCol_Border]             = COL_BORDER;
    c[ImGuiCol_FrameBg]            = {0.059f, 0.059f, 0.082f, 1.0f};
    c[ImGuiCol_FrameBgHovered]     = {0.098f, 0.098f, 0.137f, 1.0f};
    c[ImGuiCol_FrameBgActive]      = {0.118f, 0.118f, 0.165f, 1.0f};
    c[ImGuiCol_TitleBg]            = COL_CHROME;
    c[ImGuiCol_TitleBgActive]      = COL_CHROME;
    c[ImGuiCol_MenuBarBg]          = COL_CHROME;
    c[ImGuiCol_ScrollbarBg]        = COL_BG;
    c[ImGuiCol_ScrollbarGrab]      = COL_DIM;
    c[ImGuiCol_Button]             = {0.118f, 0.118f, 0.165f, 1.0f};
    c[ImGuiCol_ButtonHovered]      = COL_ACCENT;
    c[ImGuiCol_ButtonActive]       = {0.310f, 0.322f, 0.878f, 1.0f};
    c[ImGuiCol_Header]             = COL_TAB_ACTIVE;
    c[ImGuiCol_HeaderHovered]      = COL_TAB_HOVER;
    c[ImGuiCol_HeaderActive]       = COL_ACCENT;
    c[ImGuiCol_Tab]                = COL_TAB_IDLE;
    c[ImGuiCol_TabHovered]         = COL_TAB_HOVER;
    c[ImGuiCol_TabActive]          = COL_TAB_ACTIVE;
    c[ImGuiCol_TabUnfocused]       = COL_TAB_IDLE;
    c[ImGuiCol_TabUnfocusedActive] = COL_TAB_IDLE;
    c[ImGuiCol_Text]               = COL_TEXT;
    c[ImGuiCol_TextDisabled]       = COL_DIM;
    c[ImGuiCol_CheckMark]          = COL_ACCENT;
    c[ImGuiCol_SliderGrab]         = COL_ACCENT;
    c[ImGuiCol_SliderGrabActive]   = COL_ACCENT;
    c[ImGuiCol_PlotHistogram]      = COL_ACCENT;
    c[ImGuiCol_SeparatorHovered]   = COL_ACCENT;
    c[ImGuiCol_SeparatorActive]    = COL_ACCENT;
}

void chrome_init(AppState* state) {
    s_state = state;
    chrome_apply_theme();
}

// ── Helpers ───────────────────────────────────────────────────────────

static void push_no_padding() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,   {4, 3});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    {4, 0});
}
static void pop_no_padding() { ImGui::PopStyleVar(3); }

// Small icon-like button.  Returns true if clicked.
static bool icon_btn(const char* label, const char* tooltip = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_Button,        {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.f,1.f,1.f, 0.08f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {1.f,1.f,1.f, 0.15f});
    bool clicked = ImGui::SmallButton(label);
    ImGui::PopStyleColor(3);
    if (tooltip && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

// ── Tab strip ─────────────────────────────────────────────────────────

static int draw_tab_bar(AppState* st, int win_w,
                        bool& new_tab_req, int& close_idx) {
    close_idx    = -1;
    new_tab_req  = false;

    const float bar_h = (float)TAB_BAR_HEIGHT_PX * st->dpi_scale;
    const float max_tab_w = 200.0f * st->dpi_scale;
    const float min_tab_w = 80.0f  * st->dpi_scale;
    int   n   = (int)st->tabs.size();
    float avail_w = win_w - 32.0f;   // reserve space for "+" button
    float tab_w   = std::min(max_tab_w, std::max(min_tab_w, avail_w / n));

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({(float)win_w, bar_h});
    ImGui::SetNextWindowBgAlpha(1.0f);
    push_no_padding();
    ImGui::Begin("##tabs", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    pop_no_padding();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {1, 0});

    for (int i = 0; i < n; i++) {
        auto& tab = st->tabs[i];
        bool  active = (i == st->active_tab);

        ImGui::PushID(tab.id);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, COL_TAB_ACTIVE);
        else        ImGui::PushStyleColor(ImGuiCol_Button, COL_TAB_IDLE);

        // Tab button (title truncated)
        float close_w = 16.0f * st->dpi_scale;
        float label_w = tab_w - close_w - 8.0f;

        // Trim title to fit
        char  label[64];
        snprintf(label, sizeof(label), "%.32s", tab.title.c_str());

        if (ImGui::Button(label, {tab_w - close_w - 2.0f, bar_h - 2.0f})) {
            st->active_tab = i;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tab.url.c_str());

        ImGui::PopStyleColor();

        // Close button
        ImGui::SameLine(0, 1);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.f,0.3f,0.3f,0.4f});
        ImGui::PushStyleColor(ImGuiCol_Text, COL_DIM);
        if (n > 1) {
            if (ImGui::Button("x", {close_w, bar_h - 2.0f}))
                close_idx = i;
        } else {
            ImGui::Button(" ", {close_w, bar_h - 2.0f});  // placeholder
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 1);
        ImGui::PopID();
    }

    // New tab "+" button
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Button, {0,0,0,0});
    if (ImGui::Button("+", {28.0f, bar_h - 2.0f}))
        new_tab_req = true;
    ImGui::PopStyleColor();

    ImGui::PopStyleVar(2);
    ImGui::End();

    return (int)bar_h;
}

// ── Toolbar (URL bar + nav buttons) ──────────────────────────────────

static int draw_toolbar(AppState* st, int win_w, int y_offset) {
    const float bar_h = (float)CHROME_HEIGHT_PX * st->dpi_scale;

    ImGui::SetNextWindowPos({0, (float)y_offset});
    ImGui::SetNextWindowSize({(float)win_w, bar_h});
    ImGui::SetNextWindowBgAlpha(1.0f);

    push_no_padding();
    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    pop_no_padding();

    Tab* tab = st->current_tab();
    const float btn_w  = 28.0f * st->dpi_scale;
    const float pad    = 4.0f  * st->dpi_scale;
    const float url_w  = win_w - btn_w * 5.0f - pad * 8.0f;

    ImGui::SetCursorPosY((bar_h - ImGui::GetFrameHeight()) * 0.5f);
    ImGui::SetCursorPosX(pad);

    // Back
    ImGui::BeginDisabled(!tab || !tab->can_back);
    if (icon_btn("<", "Back") && tab) {
        // request back navigation -- resolved by main loop
        st->push_nav(tab->id, "__back__");
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, pad);

    // Forward
    ImGui::BeginDisabled(!tab || !tab->can_forward);
    if (icon_btn(">", "Forward") && tab) {
        st->push_nav(tab->id, "__forward__");
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, pad);

    // Reload / Stop
    if (tab && tab->loading) {
        if (icon_btn("X", "Stop")) st->push_nav(tab->id, "__stop__");
    } else {
        if (icon_btn("R", "Reload") && tab) st->push_nav(tab->id, "__reload__");
    }
    ImGui::SameLine(0, pad);

    // Home
    if (icon_btn("H", "Home"))
        st->push_nav(tab ? tab->id : 1, "http://127.0.0.1:9879/");
    ImGui::SameLine(0, pad);

    // Progress fill inside URL box when loading
    if (tab && tab->loading && tab->progress > 0.0f) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float  fh  = ImGui::GetFrameHeight();
        ImVec4 pg  = COL_ACCENT;
        pg.w = 0.25f;
        dl->AddRectFilled(pos, {pos.x + url_w * tab->progress, pos.y + fh},
                          ImGui::ColorConvertFloat4ToU32(pg), 3.0f);
    }

    // URL input
    ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.039f, 0.039f, 0.055f, 1.0f});
    ImGui::SetNextItemWidth(url_w);
    if (tab) {
        if (ImGui::InputText("##url", tab->url_buf, URL_BUF_SIZE,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            tab->url = tab->url_buf;
            st->push_nav(tab->id, tab->url);
        }
        // Keep url_buf in sync when URL changes from navigation
        if (s_state && !ImGui::IsItemActive()) {
            if (tab->url != tab->url_buf) {
                snprintf(tab->url_buf, URL_BUF_SIZE, "%s", tab->url.c_str());
            }
        }
    } else {
        char empty[1] = {};
        ImGui::InputText("##url", empty, 1);
    }
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad);

    // Dev tools toggle
    ImGui::PushStyleColor(ImGuiCol_Button,
        st->dev_tools_open ? COL_ACCENT : ImVec4{0,0,0,0});
    if (icon_btn("{}", "Dev Tools"))
        st->dev_tools_open = !st->dev_tools_open;
    ImGui::PopStyleColor();

    ImGui::End();
    return (int)bar_h;
}

// ── Status bar ────────────────────────────────────────────────────────

int chrome_draw_bottom(AppState* st, int win_w, int win_h) {
    const float bar_h = (float)STATUS_HEIGHT_PX * st->dpi_scale;
    ImGui::SetNextWindowPos({0, win_h - bar_h});
    ImGui::SetNextWindowSize({(float)win_w, bar_h});
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG);

    push_no_padding();
    ImGui::Begin("##status", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    pop_no_padding();
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY((bar_h - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::SetCursorPosX(8.0f);

    // Hover URL / status text
    const std::string& disp = st->hover_url.empty() ? st->status_text : st->hover_url;
    ImGui::PushStyleColor(ImGuiCol_Text, COL_DIM);
    ImGui::TextUnformatted(disp.c_str());
    ImGui::PopStyleColor();

    // Right-aligned: DEV fps / ENG fps / server indicators
    char fps_str[64];
    double dev_fps = st->fps_wkwv;
    double eng_fps = st->fps_engine;

    snprintf(fps_str, sizeof(fps_str),
             "DEV: %.0f  ENG: %.0f  HOST: %.0f",
             dev_fps, eng_fps, st->fps_host.fps_avg());

    float text_w = ImGui::CalcTextSize(fps_str).x;
    ImGui::SameLine(win_w - text_w - 80.0f);

    ImVec4 fps_col = (dev_fps >= 50) ? COL_OK : (dev_fps >= 30) ? COL_WARN : COL_BAD;
    if (dev_fps <= 0) fps_col = COL_DIM;
    ImGui::PushStyleColor(ImGuiCol_Text, fps_col);
    ImGui::TextUnformatted(fps_str);
    ImGui::PopStyleColor();

    // Server LEDs
    ImGui::SameLine(win_w - 70.0f);
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2      cur = ImGui::GetCursorScreenPos();
    float       r   = 4.0f;
    cur.y += (bar_h - r * 2) * 0.5f;
    // PHP server
    dl->AddCircleFilled({cur.x + r, cur.y + r}, r,
        ImGui::ColorConvertFloat4ToU32(st->php_server_ok ? COL_OK : COL_BAD));
    ImGui::SameLine(0, r * 3 + 4);
    cur = ImGui::GetCursorScreenPos();
    cur.y += (bar_h - r * 2) * 0.5f;
    // Node server
    dl->AddCircleFilled({cur.x + r, cur.y + r}, r,
        ImGui::ColorConvertFloat4ToU32(st->node_server_ok ? COL_OK : COL_BAD));
    ImGui::Dummy({r * 2, 1});

    ImGui::End();
    return (int)bar_h;
}

// ── Top (tab bar + toolbar) ───────────────────────────────────────────

int chrome_draw_top(AppState* st,
                    int win_w, int win_h,
                    bool& new_tab_requested,
                    int&  close_tab_idx) {
    int h = 0;
    h += draw_tab_bar(st, win_w, new_tab_requested, close_tab_idx);
    h += draw_toolbar(st, win_w, h);
    return h;
}
