// chrome_drawer.cpp -- inline more-panel drawer (three-dot menu contents),
// plus floating bookmarks and history panels (macOS only).

#include "chrome_drawer.h"
#include "chrome_shared.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cctype>
#include <algorithm>

static float s_bm_btn_screen_x   = 0.0f;
static float s_hist_btn_screen_x = 0.0f;
static int   s_bm_open_frame     = -1;
static int   s_hist_open_frame   = -1;
static int   s_more_open_frame   = -1;

float chrome_drawer_bm_btn_x()   { return s_bm_btn_screen_x; }
float chrome_drawer_hist_btn_x() { return s_hist_btn_screen_x; }

void chrome_drawer_set_open_frame(int frame_count) {
    s_more_open_frame = frame_count;
}

// ── Inline more-panel drawer ──────────────────────────────────────────

int chrome_draw_more_drawer(AppState* st, int win_w, int y_offset) {
    if (!st->show_more_panel) return 0;

    const float H   = (float)DRAWER_HEIGHT_PX;
    const float pad = 8.0f;

    begin_panel("##drawer_panel", 0, (float)y_offset, (float)win_w, H, COL_BASE);

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2      wpos = ImGui::GetWindowPos();
    dl->AddLine({0.0f, wpos.y}, {(float)win_w, wpos.y}, to_u32(COL_SEP), 1.0f);

    const float item_h = 28.0f;
    const float col_w  = ((float)win_w - pad * 2.0f - 4.0f) * 0.5f;

    ImGui::SetCursorPos({pad, 4.0f});
    ImGui::BeginChild("##drwcont", {(float)win_w - pad * 2.0f, H - 10.0f},
                      false, ImGuiWindowFlags_None);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {4.0f, 3.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Header,        COL_RAISED);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COL_TAB_HOV);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  COL_ACCENT_MID);

    Tab* tab = st->current_tab();

    auto DR_ITEM = [&](const char* label, float w, bool disabled) -> bool {
        ImGui::BeginDisabled(disabled);
        bool hit = ImGui::Selectable(label, false, 0, {w, item_h});
        ImGui::EndDisabled();
        return hit && !disabled;
    };

    if (st->more_subpanel == 0) {
        // ── Main grid ─────────────────────────────────────────────────

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

        // Developer Tools
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

        // Bookmarks / History
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

        // Tools
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

        // Viewport Size
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
                    st->more_subpanel   = 0;
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
                    t2.find(filt) == std::string::npos) continue;
            }
            const std::string& nm = he.title.empty() ? he.url : he.title;
            char lbl[104];
            snprintf(lbl, sizeof(lbl), "%.80s##histsub%d", nm.c_str(), i);
            if (ImGui::Selectable(lbl)) {
                if (tab) st->push_nav(tab->id, he.url);
                st->more_subpanel   = 0;
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

    // Close: Escape or click outside.
    // Use IsMouseReleased (not IsMouseClicked) so this fires on the same
    // frame as ImGui::Button -- otherwise mouse-down closes the drawer and
    // the subsequent mouse-up re-toggles it via the three-dot button.
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        st->show_more_panel = false;
    {
        ImVec2 mp = ImGui::GetMousePos();
        bool in_drawer = (mp.x >= 0 && mp.x < (float)win_w &&
                          mp.y >= (float)y_offset && mp.y < (float)y_offset + H);
        if (!in_drawer
                && ImGui::IsMouseReleased(ImGuiMouseButton_Left)
                && ImGui::GetFrameCount() != s_more_open_frame)
            st->show_more_panel = false;
    }

    end_panel();
    return (int)H;
}

// ── Floating panels (macOS only) ──────────────────────────────────────

void chrome_draw_bookmarks_panel(AppState* st, float anchor_x, float panel_top) {
#if defined(__APPLE__)
    if (!st->show_bookmarks_panel) return;

    const float W = 300.0f;
    ImGui::SetNextWindowPos({anchor_x - W + 2.0f, panel_top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({W, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {10.0f, 8.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    {6.0f, 4.0f});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.070f, 0.070f, 0.110f, 0.97f});

    if (ImGui::Begin("##bm_panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove)) {

        Tab* tab = st->current_tab();
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

        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
                && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetFrameCount() != s_bm_open_frame)
            st->show_bookmarks_panel = false;
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
#else
    (void)st; (void)anchor_x; (void)panel_top;
#endif
}

void chrome_draw_history_panel(AppState* st, float anchor_x, float panel_top) {
#if defined(__APPLE__)
    if (!st->show_history_panel) return;

    const float W  = 340.0f;
    const float MH = 380.0f;
    ImGui::SetNextWindowPos({anchor_x - W, panel_top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({W, MH});
    ImGui::SetNextWindowBgAlpha(0.97f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {10.0f, 8.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.070f, 0.070f, 0.110f, 0.97f});

    if (ImGui::Begin("##hist_panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {

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
            for (auto& c : filt) c = (char)tolower((unsigned char)c);
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
#else
    (void)st; (void)anchor_x; (void)panel_top;
#endif
}
