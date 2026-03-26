// main_globals.h -- Extern declarations for all file-scope globals.
// Include this (or main_funcs.h) in any main/ file that reads/writes them.
#pragma once
#include "main_priv.h"

extern AppState      g_state;
extern GLFWwindow*   g_win;
extern int           g_prev_top;
extern int           g_prev_bot;
extern int           g_prev_tab_count;
extern int           g_php_port;
extern int           g_fb_w;
extern int           g_fb_h;
extern bool          g_resize_dirty;
extern bool          g_win_iconified;
extern bool          g_chrome_has_hover;
