// platform_mac.mm -- macOS platform implementation.
// Delegates to existing Cocoa / AppKit / native_chrome code.
// This file is only compiled on Apple targets.

#import <Cocoa/Cocoa.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <fstream>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "platform.h"
#include "../top-of-gui/native_chrome.h"

// ── App lifecycle ─────────────────────────────────────────────────────────

void platform_app_preinit() {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

void platform_app_postinit(void* glfw_window) {
    (void)glfw_window;
    [NSApp activateIgnoringOtherApps:YES];
}

void platform_app_cleanup() {
    // ARC handles Obj-C object teardown; nothing explicit needed here.
}

// ── Executable path ───────────────────────────────────────────────────────

bool platform_exe_path(char* out, size_t size) {
    uint32_t sz = (uint32_t)size;
    return _NSGetExecutablePath(out, &sz) == 0;
}

// ── Native menu bar ───────────────────────────────────────────────────────

// build_app_menubar() is defined in main_menu.mm and calls NSMenu APIs.
void build_app_menubar();
void platform_menu_init() {
    build_app_menubar();
}

// ── Native chrome ─────────────────────────────────────────────────────────

void platform_chrome_create(void* glfw_window, AppState* state, int php_port) {
    // native_chrome_create expects the NSWindow*, which GLFW exposes via
    // glfwGetCocoaWindow when GLFW_EXPOSE_NATIVE_COCOA is defined.
    NSWindow* nswin = glfwGetCocoaWindow((GLFWwindow*)glfw_window);
    native_chrome_create(nswin, state, php_port);
}

int platform_chrome_update(AppState* state) {
    return native_chrome_update(state);
}

int platform_chrome_status_h() {
    return native_chrome_status_h();
}

bool platform_chrome_has_hover() {
    return native_chrome_has_hover();
}

void platform_chrome_resize(int win_w, int win_h) {
    native_chrome_resize(win_w, win_h);
}

void platform_chrome_focus_url() {
    native_chrome_focus_url();
}

void platform_chrome_destroy() {
    native_chrome_destroy();
}

// ── Export bridge ─────────────────────────────────────────────────────────

void platform_export_file(const std::string& filename, const std::string& content) {
    // On macOS the webview bridge (WKScriptMessageHandler in xcm_shell.mm /
    // webview_delegates.mm) receives the xcm_export postMessage call from JS
    // and calls this function to persist the file next to the project directory.
    //
    // Determine the project root from the executable path and write there.
    char exe[1024] = {};
    uint32_t sz = sizeof(exe);
    _NSGetExecutablePath(exe, &sz);

    std::string ep(exe);
    auto sl = ep.rfind('/');
    std::string dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";

    // Walk up out of any .app bundle path.
    while (dir.find(".app") != std::string::npos || dir.find("Contents") != std::string::npos) {
        auto s = dir.rfind('/');
        if (s == std::string::npos) break;
        dir = dir.substr(0, s);
    }

    std::string path = dir + "/" + filename;
    std::ofstream f(path);
    if (f) {
        f << content;
        fprintf(stderr, "[platform_mac] exported: %s\n", path.c_str());
    } else {
        fprintf(stderr, "[platform_mac] export failed: %s\n", path.c_str());
    }
}
