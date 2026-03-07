// platform_linux.cpp -- Linux platform implementation.
// GLFW handles the event loop and window creation.
// WebKitGTK is wired through webview_linux.cpp.
// ImGui chrome (chrome.cpp) is used in place of native NSView chrome.

#include "platform.h"
#include "../app_state.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>   // readlink
#include <limits.h>   // PATH_MAX

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// ── App lifecycle ─────────────────────────────────────────────────────────

void platform_app_preinit() {
    // GTK must be initialised before any WebKitGTK call.
    // Pass dummy argc/argv -- GTK only needs them to parse display arguments.
    int    argc = 0;
    char** argv = nullptr;
    // gtk_init is called in webview_linux.cpp when the first webview is created
    // via g_webview_gtk_init(). Calling it here a second time is safe (it is a
    // no-op after the first call).
}

void platform_app_postinit(void* glfw_window) {
    (void)glfw_window;
    // GLFW already raised and focused the window at glfwShowWindow time.
}

void platform_app_cleanup() {
    // GTK cleanup is handled by webview_linux.cpp on final webview_destroy.
}

// ── Executable path ───────────────────────────────────────────────────────

bool platform_exe_path(char* out, size_t size) {
    ssize_t len = readlink("/proc/self/exe", out, size - 1);
    if (len < 0) return false;
    out[len] = '\0';
    return true;
}

// ── Native menu bar ───────────────────────────────────────────────────────

void platform_menu_init() {
    // No system-level menu bar on Linux.
    // The ImGui toolbar in chrome.cpp provides all user-facing controls.
}

// ── Native chrome ─────────────────────────────────────────────────────────
// Linux uses the ImGui chrome layer (chrome.cpp) rather than a native toolkit.
// These functions are no-ops; chrome.cpp draws directly into the GLFW window.

void platform_chrome_create(void* glfw_window, AppState* state, int php_port) {
    (void)glfw_window; (void)state; (void)php_port;
}

int platform_chrome_update(AppState* state) {
    (void)state;
    // Return the same constant the macOS native chrome returns so callers
    // (reposition_webviews) get a correct content-area y-offset.
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
    // Resolve path relative to the executable directory.
    char exe[PATH_MAX] = {};
    if (!platform_exe_path(exe, sizeof(exe))) {
        fprintf(stderr, "[platform_linux] could not resolve exe path\n");
        return;
    }
    std::string ep(exe);
    auto sl = ep.rfind('/');
    std::string dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";

    std::string path = dir + "/" + filename;
    std::ofstream f(path);
    if (f) {
        f << content;
        fprintf(stderr, "[platform_linux] exported: %s\n", path.c_str());
    } else {
        fprintf(stderr, "[platform_linux] export failed: %s\n", path.c_str());
    }
}
