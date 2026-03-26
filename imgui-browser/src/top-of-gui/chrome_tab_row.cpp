// chrome_tab_row.cpp -- single-row tab strip with drag-to-reorder.
// On Windows, traffic lights are a native GDI+ child HWND overlay
// (chrome_traffic_lights.cpp) -- no ImGui rendering here.

#include "chrome_tab_row.h"
#include "chrome_shared.h"
#include "chrome_traffic_lights.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

static uint32_t s_logo_tex   = 0;
static int      s_logo_src_w = 0;
static int      s_logo_src_h = 0;

static int   s_drag_idx   = -1;
static float s_drag_off_x = 0.0f;
static int   s_drop_tgt   = -1;
static float s_end_x      = 0.0f;

void chrome_tab_set_logo(uint32_t tex_id, int src_w, int src_h) {
    s_logo_tex   = tex_id;
    s_logo_src_w = src_w;
    s_logo_src_h = src_h;
}

int chrome_draw_tab_row(AppState* st, int win_w, bool& new_tab, int& close_idx) {
    close_idx = -1;
    new_tab   = false;

    const float H       = (float)TAB_BAR_HEIGHT_PX;
    const float TOP_PAD = 30.0f;
    const float AVAIL_H = H - TOP_PAD;

    begin_panel("##titlerow", 0, 0, (float)win_w, H, COL_SURFACE);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background decoration
    dl->AddLine({0, H - 1}, {(float)win_w, H - 1}, to_u32(COL_SEP), 1.0f);
    dl->AddRectFilledMultiColor(
        {0.0f, 0.0f}, {(float)win_w, H},
        to_u32({0.40f, 0.38f, 0.88f, 0.030f}),
        to_u32({0.40f, 0.38f, 0.88f, 0.030f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}),
        to_u32({0.00f, 0.00f, 0.00f, 0.00f}));
    ImGui::GetForegroundDrawList()->AddLine(
        {0.0f, 0.5f}, {(float)win_w, 0.5f},
        to_u32({0.60f, 0.58f, 1.00f, 0.10f}), 1.0f);

#ifndef __APPLE__
    // Traffic lights are painted by a native GDI+ child HWND overlay
    // (chrome_tl_create / chrome_tl_update) -- no ImGui calls needed here.
    // The overlay sits at (0,0) and handles its own mouse input.
#endif

    const float TL_GAP = (float)TRAFFIC_LIGHT_W;
    const float PLUS_W = 28.0f;
    const float tabs_x = TL_GAP;
    const float tabs_w = (float)win_w - TL_GAP - PLUS_W - 4.0f;

    int   n     = (int)st->tabs.size();
    float tab_w = std::min(180.0f, std::max(64.0f, tabs_w / (float)std::max(n, 1)));
    float cx    = tabs_x;

    for (int i = 0; i < n; ++i) {
        auto& tab    = st->tabs[i];
        bool  active = (i == st->active_tab);
        bool  dragging_this = (s_drag_idx == i &&
                               ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f));

        ImGui::PushID(tab.id);

        ImVec2 tMin = {cx, TOP_PAD};
        ImVec2 tMax = {cx + tab_w, H};
        bool   hov_rect = ImGui::IsMouseHoveringRect(tMin, tMax) && s_drag_idx == -1;
        float  close_w  = (n > 1 && (active || hov_rect)) ? 18.0f : 0.0f;
        float  fill_alpha = dragging_this ? 0.25f : 1.0f;

        // Tab background
        if (active) {
            ImVec4 bg = COL_TAB_ACT; bg.w = fill_alpha;
            dl->AddRectFilled(tMin, tMax, to_u32(bg), 6.0f);
        } else if (hov_rect) {
            dl->AddRectFilled(tMin, tMax, to_u32(COL_TAB_HOV), 6.0f);
        }
        {
            float ba = active ? 0.40f : (hov_rect ? 0.20f : 0.10f);
            dl->AddRect(tMin, tMax,
                        to_u32(ImVec4{0.388f, 0.400f, 0.941f, ba}),
                        6.0f, ImDrawFlags_None, 1.0f);
        }

        // Label
        {
            char disp[36];
            snprintf(disp, sizeof(disp), "%.28s", tab.title.c_str());
            float ty = TOP_PAD + (AVAIL_H - ImGui::GetTextLineHeight()) * 0.5f;
            float tx = cx + 10.0f;
            float re = cx + tab_w - close_w - 6.0f;
            ImVec4 tc = active ? COL_TEXT : COL_TEXT_DIM;
            tc.w = fill_alpha;
            if (ImGui::CalcTextSize(disp).x > re - tx) {
                ImGui::PushClipRect({tx, ty},
                                    {re, ty + ImGui::GetTextLineHeight() + 2}, true);
                dl->AddText({tx, ty}, to_u32(tc), disp);
                ImGui::PopClipRect();
            } else {
                dl->AddText({tx, ty}, to_u32(tc), disp);
            }
        }

        // Loading dot
        if (tab.loading && !dragging_this) {
            dl->AddCircleFilled({cx + tab_w - close_w - 12.0f,
                                 TOP_PAD + AVAIL_H * 0.5f},
                                3.5f, to_u32(COL_ACCENT), 8);
        }

        // Invisible button for click / drag (excludes close area)
        ImGui::SetCursorPos({cx, TOP_PAD});
        char btn_id[16];
        snprintf(btn_id, sizeof(btn_id), "##tb%d", tab.id);
        float ib_w = (close_w > 0.0f) ? (tab_w - close_w - 6.0f) : tab_w;
        ImGui::InvisibleButton(btn_id, {ib_w, AVAIL_H});

        if (ImGui::IsItemActivated()) {
            s_drag_idx     = i;
            s_drag_off_x   = ImGui::GetMousePos().x - cx;
            st->active_tab = i;
        }
        if (s_drag_idx == i && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
            float mx   = ImGui::GetMousePos().x;
            s_drop_tgt = std::max(0, std::min(n - 1, (int)((mx - tabs_x) / tab_w)));
        }
        if (ImGui::IsItemDeactivated() && s_drag_idx == i) {
            if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
                st->active_tab = i;
            } else if (s_drop_tgt != -1 && s_drop_tgt != i) {
                int from = i, to = s_drop_tgt, was = st->active_tab;
                auto& tabs = st->tabs;
                if (from < to) {
                    std::rotate(tabs.begin()+from, tabs.begin()+from+1, tabs.begin()+to+1);
                    if      (was == from)             st->active_tab = to;
                    else if (was > from && was <= to) st->active_tab--;
                } else {
                    std::rotate(tabs.begin()+to, tabs.begin()+from, tabs.begin()+from+1);
                    if      (was == from)             st->active_tab = to;
                    else if (was >= to && was < from) st->active_tab++;
                }
            }
            s_drag_idx = -1;
            s_drop_tgt = -1;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && s_drag_idx == -1)
            ImGui::SetTooltip("%s", tab.url.c_str());

        // Close button
        if (close_w > 0.0f) {
            ImGui::SetCursorPos({cx + tab_w - close_w - 6.0f,
                                 TOP_PAD + (AVAIL_H - close_w) * 0.5f});
            ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f,0.3f,0.3f,0.30f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.9f,0.3f,0.3f,0.55f});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, close_w * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  {0, 0});
            char cid[16];
            snprintf(cid, sizeof(cid), "##cx%d", tab.id);
            if (ImGui::Button(cid, {close_w, close_w})) close_idx = i;
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

    // Ghost tab + drop indicator during drag
    if (s_drag_idx != -1 && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
        float mx      = ImGui::GetMousePos().x;
        float ghost_x = std::max(tabs_x,
                        std::min(mx - s_drag_off_x,
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
    if (ImGui::Button("##newtab", {22.0f, 22.0f})) new_tab = true;
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

    // Record where interactive elements end for drag-area calculation.
    s_end_x = cx + 2.0f + 22.0f + 4.0f;

    // Logo (Xcalibur The Cat) -- far-right corner, only if space allows
    if (s_logo_tex && s_logo_src_h > 0) {
        const float logo_h = AVAIL_H - 4.0f;
        const float logo_w = logo_h * (float)s_logo_src_w / (float)s_logo_src_h;
        const float right  = (float)win_w - 8.0f;
        const float lx     = right - logo_w;
        const float ly     = TOP_PAD + (AVAIL_H - logo_h) * 0.5f;
        float used_x       = cx + 24.0f + 8.0f;
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

float chrome_tab_row_end_x() { return s_end_x; }
