#pragma once
// chrome_shared.h -- palette, panel helpers, and common types shared by
// all chrome modules.  All functions are static inline to avoid ODR issues.

#include "imgui.h"
#include "app_state.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

// ── Palette (inline const = one ODR-safe definition per program) ──────
inline const ImVec4 COL_BASE       = {0.035f, 0.035f, 0.051f, 1.0f};
inline const ImVec4 COL_SURFACE    = {0.063f, 0.063f, 0.094f, 1.0f};
inline const ImVec4 COL_RAISED     = {0.090f, 0.090f, 0.133f, 1.0f};
inline const ImVec4 COL_ACCENT     = {0.388f, 0.400f, 0.941f, 1.0f};
inline const ImVec4 COL_ACCENT_LO  = {0.388f, 0.400f, 0.941f, 0.18f};
inline const ImVec4 COL_ACCENT_MID = {0.388f, 0.400f, 0.941f, 0.35f};
inline const ImVec4 COL_TAB_ACT    = {0.110f, 0.110f, 0.176f, 1.0f};
inline const ImVec4 COL_TAB_IDLE   = {0.000f, 0.000f, 0.000f, 0.0f};
inline const ImVec4 COL_TAB_HOV    = {0.388f, 0.400f, 0.941f, 0.10f};
inline const ImVec4 COL_TEXT       = {0.900f, 0.914f, 0.961f, 1.0f};
inline const ImVec4 COL_TEXT_DIM   = {0.400f, 0.427f, 0.502f, 1.0f};
inline const ImVec4 COL_OK         = {0.204f, 0.827f, 0.600f, 1.0f};
inline const ImVec4 COL_WARN       = {0.984f, 0.753f, 0.259f, 1.0f};
inline const ImVec4 COL_BAD        = {0.973f, 0.529f, 0.451f, 1.0f};
inline const ImVec4 COL_SEP        = {0.388f, 0.400f, 0.941f, 0.12f};
inline const ImVec4 COL_STATUS_BG  = {0.165f, 0.130f, 0.270f, 1.0f};

// ── Color convert ─────────────────────────────────────────────────────
static inline ImU32 to_u32(ImVec4 v) {
    return ImGui::ColorConvertFloat4ToU32(v);
}

// ── Frameless pinned panel ────────────────────────────────────────────
static inline bool begin_panel(const char* id,
                                float x, float y, float w, float h,
                                ImVec4 bg = {0.063f, 0.063f, 0.094f, 1.0f}) {
    ImGui::SetNextWindowPos({x, y});
    ImGui::SetNextWindowSize({w, h});
    ImGui::SetNextWindowBgAlpha(bg.w);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    return ImGui::Begin(id, nullptr,
        ImGuiWindowFlags_NoTitleBar       | ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings  | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove           | ImGuiWindowFlags_NoNav);
}

static inline void end_panel() {
    ImGui::End();
    ImGui::PopStyleColor();
}

// ── Ghost button (no background, hover ring) ──────────────────────────
static inline bool ghost_btn(const char* id, const char* text,
                               float w, float h,
                               const char* tooltip = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACCENT_LO);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  COL_ACCENT_MID);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    bool hit = ImGui::Button(id, {w, h});
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

// ── Genie animation state (owned by traffic lights module) ────────────
struct GenieAnimState {
    bool   active     = false;
    double start_time = 0.0;
    float  window_w   = 0.0f;
    float  window_h   = 0.0f;
};
