#pragma once
// chrome_toolbar.h -- URL bar, navigation buttons, three-dot menu button.

#include "app_state.h"

// Draw the toolbar row.  Returns the pixel height consumed.
int chrome_draw_toolbar(AppState* st, int win_w, int y_offset);
