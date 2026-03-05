#pragma once
// chrome.h -- Dear ImGui browser chrome (tabs / toolbar / status bar)
// All rendering is pure C++17.  No Obj-C types.

#include "app_state.h"
#include <cstdint>

// Must be called once after ImGui context is created.
void chrome_init(AppState* state);

// Set the logo texture (RGBA OpenGL texture ID, original pixel dimensions).
// Must be called after an OpenGL context is current.
// Safe to call multiple times; pass 0 to clear.
void chrome_set_logo(uint32_t tex_id, int src_w, int src_h);

// Draw the tab strip + toolbar.  Returns the pixel height consumed
// so the caller can position the WKWebView content area below it.
// Should be called every ImGui frame.
int chrome_draw_top(AppState* state,
                    int win_w, int win_h,
                    bool& new_tab_requested,
                    int&  close_tab_idx);

// Draw the bottom status bar.  Returns height consumed.
int chrome_draw_bottom(AppState* state, int win_w, int win_h);

// Apply the custom dark theme to ImGui::GetStyle().
void chrome_apply_theme();
