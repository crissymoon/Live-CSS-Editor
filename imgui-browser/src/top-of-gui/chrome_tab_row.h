#pragma once
// chrome_tab_row.h -- single-row tab strip with drag-to-reorder.

#include "app_state.h"
#include <cstdint>

// Set the logo texture shown in the far-right corner of the tab strip.
void chrome_tab_set_logo(uint32_t tex_id, int src_w, int src_h);

// Draw the tab strip.  Returns the pixel height consumed.
int chrome_draw_tab_row(AppState* st, int win_w, bool& new_tab, int& close_idx);

// Returns the x-coordinate (in window pixels) just past the last interactive
// element in the tab row (last tab + "+" button).  Empty space to the right
// of this value is available for window dragging.
float chrome_tab_row_end_x();
