#pragma once
// chrome.h -- Dear ImGui browser chrome (tabs / toolbar / status bar)
// All rendering is pure C++17.  No Obj-C types.

#include "app_state.h"

// Must be called once after ImGui context is created.
void chrome_init(AppState* state);

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
