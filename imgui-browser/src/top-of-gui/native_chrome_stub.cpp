// native_chrome_stub.cpp -- No-op native chrome for non-Apple platforms.
//
// On macOS native_chrome.mm creates real NSView-based toolbar and tab bar
// widgets inside the window.  On Linux and Windows the ImGui chrome layer
// (chrome.cpp) is used instead.  The platform_*.cpp files return
// TOTAL_CHROME_TOP from platform_chrome_update() so the webview content
// area offset is still calculated correctly.

#include "chrome.h"     // TOTAL_CHROME_TOP constant pulled from app_state.h

// Stub bodies for the native_chrome_* symbols that main_funcs.h / main_render.mm
// may reference via the platform abstraction.  These are never called on
// non-Apple builds because platform_chrome_* delegates to these stubs.
// They exist only to satisfy the linker if any translation unit references
// the symbols by name rather than through the platform layer.

#include "app_state.h"   // for AppState type

void native_chrome_create(void*, AppState*, int)  {}
int  native_chrome_update(AppState*)              { return TOTAL_CHROME_TOP; }
int  native_chrome_status_h()                     { return STATUS_HEIGHT_PX; }
bool native_chrome_has_hover()                    { return false; }
void native_chrome_resize(int, int)               {}
float native_chrome_bm_btn_x()                    { return 0.0f; }
float native_chrome_hist_btn_x()                  { return 0.0f; }
void native_chrome_focus_url()                    {}
void native_chrome_destroy()                      {}
void native_chrome_eval_toolbar_js(const char*)   {}
