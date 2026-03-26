// main_globals.mm -- Single definition point for all shared file-scope globals.
// Every other main/ file accesses these via extern declarations in main_globals.h.

#include "main_globals.h"

AppState      g_state;
GLFWwindow*   g_win             = nullptr;
int           g_prev_top        = 0;
int           g_prev_bot        = 0;
int           g_prev_tab_count  = 0;
int           g_php_port        = 9879;
int           g_fb_w            = 0;
int           g_fb_h            = 0;
bool          g_resize_dirty    = false;
bool          g_win_iconified   = false;
bool          g_chrome_has_hover = false;
