#pragma once
// chrome_drawer.h -- inline more-panel drawer, bookmarks, history sub-panels.

#include "app_state.h"

// Must be called from the toolbar module when the more button opens the drawer.
// Prevents the drawer from closing on the same frame it opens.
void chrome_drawer_set_open_frame(int frame_count);

// Draw the inline more-panel drawer (pinned above WebView2).
// Returns height consumed; 0 when the panel is closed.
int chrome_draw_more_drawer(AppState* st, int win_w, int y_offset);

// Floating bookmark / history panels.
// On Windows/Linux these are no-ops (they would be occluded by WebView2 HWND).
void chrome_draw_bookmarks_panel(AppState* st, float anchor_x, float panel_top);
void chrome_draw_history_panel  (AppState* st, float anchor_x, float panel_top);

// Screen-X (right edge) of the bookmark / history toolbar buttons.
float chrome_drawer_bm_btn_x();
float chrome_drawer_hist_btn_x();
