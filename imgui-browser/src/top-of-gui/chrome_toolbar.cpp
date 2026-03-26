// chrome_toolbar.cpp -- URL bar, navigation buttons, security indicator,
// info-slide overlay, and the three-dot menu button.

#include "chrome_toolbar.h"
#include "chrome_shared.h"
#include "chrome_drawer.h"
#include "imgui.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <regex>
#include <cctype>
#include <algorithm>

// ── URL resolution (mirrors chrome.js resolveUrl) ─────────────────────
static std::string resolve_url(const std::string& raw) {
    size_t start = raw.find_first_not_of(" \t\r\n");
    size_t end   = raw.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    std::string v = raw.substr(start, end - start + 1);
    if (v.empty()) return {};

    if (v.find("://") != std::string::npos) return v;
    if (v.size() >= 6 && v.substr(0, 6) == "about:") return v;
    if (v.substr(0, 9) == "localhost") return "http://" + v;

    bool has_space = v.find(' ') != std::string::npos;
    bool has_dot   = v.find('.') != std::string::npos;

    static const std::regex re_ip(R"(^\d{1,3}(\.\d{1,3}){3}(:\d+)?(/.*)?$)");
    if (std::regex_match(v, re_ip)) return "https://" + v;

    static const std::regex re_tld(R"(\.[a-zA-Z]{2,}(/.*)?$)");
    if (!has_space && has_dot && std::regex_search(v, re_tld))
        return "https://" + v;

    if (v.substr(0, 4) == "127." || v.substr(0, 8) == "192.168.")
        return "http://" + v;

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

// ── Toolbar ───────────────────────────────────────────────────────────

int chrome_draw_toolbar(AppState* st, int win_w, int y_offset) {
    const float fh  = ImGui::GetFontSize();
    const float btn = std::max(28.0f, fh + 10.0f);
    const float H   = std::max((float)CHROME_HEIGHT_PX, btn + 16.0f);
    const float pad = 8.0f;

    begin_panel("##toolbar", 0, (float)y_offset, (float)win_w, H, COL_SURFACE);
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2      wpos = ImGui::GetWindowPos();

    // Bottom hairline + shimmer
    dl->AddLine({0, wpos.y + H - 1}, {(float)win_w, wpos.y + H - 1},
                to_u32(COL_SEP), 1.0f);
    dl->AddRectFilledMultiColor(
        {0.0f, wpos.y}, {(float)win_w, wpos.y + H},
        to_u32({0.38f, 0.36f, 0.88f, 0.025f}),
        to_u32({0.38f, 0.36f, 0.88f, 0.025f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));

    Tab* tab = st->current_tab();
    float cy = (H - btn) * 0.5f;
    ImGui::SetCursorPos({pad, cy});

    // ── Back ──────────────────────────────────────────────────────────
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

    // ── URL bar layout ────────────────────────────────────────────────
    float url_area_x = ImGui::GetCursorPosX();
    float lock_w     = 22.0f;
    float info_tog_w = 18.0f;
    float more_btn_w = btn + 2.0f;
    float url_bar_w  = (float)win_w - url_area_x - more_btn_w - pad * 2.0f;
    float url_inp_w  = url_bar_w - lock_w - 4.0f - info_tog_w - 2.0f;

    ImVec2 bar_sp = ImGui::GetCursorScreenPos();
    float  bar_fh = btn;

    // URL bar pill background
    dl->AddRectFilled(bar_sp,
                      {bar_sp.x + url_bar_w, bar_sp.y + bar_fh},
                      to_u32(COL_BASE), 6.0f);

    // Progress bar at bottom of URL pill
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
        float  dcx = sp2.x + lock_w * 0.5f;
        float  dcy = sp2.y + bar_fh * 0.5f;
        if (https) {
            dl->AddCircleFilled({dcx, dcy}, 4.5f, to_u32(sec_col), 12);
        } else {
            dl->AddCircle({dcx, dcy}, 4.5f, to_u32(sec_col), 12, 1.5f);
            if (http) dl->AddCircleFilled({dcx, dcy}, 1.5f, to_u32(sec_col), 8);
        }
        ImGui::InvisibleButton("##sec_ind", {lock_w, bar_fh});
        const char* sec_tip = https ? "Secure connection (HTTPS)"
                             : http  ? "Not secure (HTTP)" : nullptr;
        if (sec_tip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", sec_tip);
        ImGui::SameLine(0, 4);
    }

    // ── URL input ─────────────────────────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  {0,0,0,0});
        float url_pad_y = (btn - ImGui::GetFontSize()) * 0.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {8.0f, url_pad_y});
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

    // ── Info toggle (< / >) ───────────────────────────────────────────
    {
        bool slide_open = st->show_info_slide;
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, to_u32(COL_RAISED));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  to_u32(COL_ACCENT_LO));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0,0});
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

    // ── Info slide overlay ────────────────────────────────────────────
    if (st->show_info_slide) {
        float url_left  = bar_sp.x + lock_w + 4.0f;
        float url_right = url_left + url_inp_w;

        float dot_r  = 3.5f;
        float lbl_gap = 9.0f;
        float sep_gap = 8.0f;
        float php_tw  = ImGui::CalcTextSize("php").x;
        float js_tw   = ImGui::CalcTextSize("js").x;
        int content_h = st->win_h - TOTAL_CHROME_TOP
                        - (st->show_more_panel ? DRAWER_HEIGHT_PX : 0)
                        - STATUS_HEIGHT_PX;
        char vp_str[32];
        snprintf(vp_str, sizeof(vp_str), "%d x %d", st->win_w, content_h);
        float vp_tw   = ImGui::CalcTextSize(vp_str).x;
        float info_cw = 8.0f + (dot_r*2+lbl_gap+php_tw) + sep_gap*2
                              + (dot_r*2+lbl_gap+js_tw)  + sep_gap*2 + vp_tw + 8.0f;
        float info_w  = std::min(info_cw, url_inp_w);
        float info_x  = url_right - info_w;
        float shw     = 16.0f;

        dl->AddRectFilledMultiColor(
            {info_x - shw, bar_sp.y + 1.0f}, {info_x, bar_sp.y + bar_fh - 1.0f},
            to_u32({COL_BASE.x, COL_BASE.y, COL_BASE.z, 0.0f}),
            to_u32(COL_BASE), to_u32(COL_BASE),
            to_u32({COL_BASE.x, COL_BASE.y, COL_BASE.z, 0.0f}));
        dl->AddRectFilled({info_x, bar_sp.y + 1.0f},
                          {url_right, bar_sp.y + bar_fh - 1.0f},
                          to_u32(COL_BASE));

        float ic_cy = bar_sp.y + bar_fh * 0.5f;
        float icx   = info_x + 8.0f;
        float th    = ImGui::GetTextLineHeight();

        dl->AddCircleFilled({icx + dot_r, ic_cy}, dot_r,
                            to_u32(st->php_server_ok ? COL_OK : COL_BAD), 8);
        dl->AddText({icx + lbl_gap, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), "php");
        icx += lbl_gap + php_tw + sep_gap;

        dl->AddLine({icx, bar_sp.y + 6.0f}, {icx, bar_sp.y + bar_fh - 6.0f},
                    to_u32(COL_SEP), 1.0f);
        icx += sep_gap;

        bool js_ok = !tab || tab->js_enabled;
        dl->AddCircleFilled({icx + dot_r, ic_cy}, dot_r,
                            to_u32(js_ok ? COL_OK : COL_WARN), 8);
        dl->AddText({icx + lbl_gap, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), "js");
        icx += lbl_gap + js_tw + sep_gap;

        dl->AddLine({icx, bar_sp.y + 6.0f}, {icx, bar_sp.y + bar_fh - 6.0f},
                    to_u32(COL_SEP), 1.0f);
        icx += sep_gap;

        dl->AddText({icx, ic_cy - th * 0.5f}, to_u32(COL_TEXT_DIM), vp_str);
    }

    // URL bar border drawn on top of all URL bar widgets
    dl->AddRect(bar_sp,
                {bar_sp.x + url_bar_w, bar_sp.y + bar_fh},
                to_u32({0.388f, 0.400f, 0.941f, 0.28f}), 6.0f, 0, 1.0f);

    ImGui::SameLine(0, 4);

    // ── Three-dot menu button ─────────────────────────────────────────
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
                chrome_drawer_set_open_frame(ImGui::GetFrameCount());
        }
        {
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            ImU32  gc   = to_u32(more_open ? COL_ACCENT : COL_TEXT_DIM);
            float  my2  = (bmin.y + bmax.y) * 0.5f;
            float  bcx  = (bmin.x + bmax.x) * 0.5f;
            dl->AddCircleFilled({bcx - 5.5f, my2}, 1.9f, gc, 8);
            dl->AddCircleFilled({bcx,         my2}, 1.9f, gc, 8);
            dl->AddCircleFilled({bcx + 5.5f,  my2}, 1.9f, gc, 8);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Tools and settings");
    }

    end_panel();
    return (int)H;
}
