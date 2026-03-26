// platform_win.cpp -- Windows platform implementation.
// GLFW handles the event loop and window creation.
// WebView2 is wired through webview_win.cpp.
// ImGui chrome (chrome.cpp) is used in place of native NSView chrome.

#include "platform.h"
#include "../app_state.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>     // PathRemoveFileSpecA
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// ── App lifecycle ─────────────────────────────────────────────────────────

void platform_app_preinit() {
    // Tell Windows this process is DPI-aware so GLFW window sizes are in
    // physical pixels and the webview content is not blurry on HiDPI displays.
#if defined(WINVER) && WINVER >= 0x0605
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#else
    SetProcessDPIAware();
#endif
}

void platform_app_postinit(void* glfw_window) {
    (void)glfw_window;
    // GLFW raises and focuses the window; nothing extra needed on Win32.
}

void platform_app_cleanup() {
    // WebView2 COM teardown is handled by webview_win.cpp.
}

// ── Executable path ───────────────────────────────────────────────────────

bool platform_exe_path(char* out, size_t size) {
    DWORD written = GetModuleFileNameA(nullptr, out, (DWORD)size);
    return written > 0 && written < (DWORD)size;
}

// ── Native menu bar ───────────────────────────────────────────────────────

void platform_menu_init() {
    // No system-level menu bar on Windows.
    // The ImGui toolbar in chrome.cpp provides all user-facing controls.
    // A Win32 HMENU could be installed here in the future if needed.
}

// ── Native chrome ─────────────────────────────────────────────────────────
// Windows uses the ImGui chrome layer (chrome.cpp) rather than a native toolkit.
// These functions are no-ops; chrome.cpp draws directly into the GLFW window.

void platform_chrome_create(void* glfw_window, AppState* state, int php_port) {
    (void)glfw_window; (void)state; (void)php_port;
}

int platform_chrome_update(AppState* state) {
    (void)state;
    return TOTAL_CHROME_TOP;
}

int platform_chrome_status_h() {
    return STATUS_HEIGHT_PX;
}

bool platform_chrome_has_hover() {
    return false;
}

void platform_chrome_resize(int win_w, int win_h) {
    (void)win_w; (void)win_h;
}

void platform_chrome_focus_url() {
    // TODO: set ImGui keyboard focus to the URL input widget in chrome.cpp.
}

void platform_chrome_destroy() {}

// ── Export bridge ─────────────────────────────────────────────────────────

void platform_export_file(const std::string& filename, const std::string& content) {
    char exe[MAX_PATH] = {};
    if (!platform_exe_path(exe, sizeof(exe))) {
        fprintf(stderr, "[platform_win] could not resolve exe path\n");
        return;
    }
    // Strip the filename from the path to get the directory.
    PathRemoveFileSpecA(exe);
    std::string path = std::string(exe) + "\\" + filename;

    std::ofstream f(path);
    if (f) {
        f << content;
        fprintf(stderr, "[platform_win] exported: %s\n", path.c_str());
    } else {
        fprintf(stderr, "[platform_win] export failed: %s\n", path.c_str());
    }
}
