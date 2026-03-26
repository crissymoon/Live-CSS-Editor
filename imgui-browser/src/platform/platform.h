#pragma once
// platform.h -- Thin platform abstraction layer for imgui-browser.
//
// Replaces all macOS-only API calls that live outside the webview/ and
// xcm_shell modules.  Each function has one implementation per platform:
//
//   platform_mac.mm     -- macOS (Cocoa / AppKit)
//   platform_linux.cpp  -- Linux (WebKitGTK, X11/Wayland via GLFW)
//   platform_win.cpp    -- Windows (WebView2, Win32)
//
// All functions are called from main.mm (macOS) or the equivalent
// main_entry_*.cpp on other platforms.

#include "../app_state.h"
#include <string>

// ── App lifecycle ─────────────────────────────────────────────────────────

// One-time init before the GLFW window is created.
// macOS: [NSApplication sharedApplication] + activation policy.
// Linux/Windows: no-op (GLFW handles the event loop).
void platform_app_preinit();

// One-time init after the GLFW window is created.
// macOS: NSApp activate, window ordering.
// Linux/Windows: no-op or DPI awareness call.
void platform_app_postinit(void* glfw_window);

// Clean up before exit.
void platform_app_cleanup();

// ── Executable path ───────────────────────────────────────────────────────

// Fill `out` with the absolute path to the running executable.
// Equivalent to _NSGetExecutablePath on macOS.
// Returns true on success.
bool platform_exe_path(char* out, size_t size);

// ── Native menu bar ───────────────────────────────────────────────────────

// Build and install a native application menu bar.
// macOS: NSMenu with Quit / Edit (Cmd+C/V/Z etc.) / Tools.
// Linux/Windows: no-op (no native menu bar; could be extended with a
//                native menu library later).
void platform_menu_init();

// ── Native chrome (toolbar / tab bar) ─────────────────────────────────────

// Create platform-native chrome views attached to the given window handle.
// glfw_window: the GLFWwindow* cast to void*.
// On macOS this delegates to native_chrome_create (NSView-based).
// On Linux/Windows this is a no-op -- the ImGui chrome layer in chrome.cpp
// is used as the fallback.
void platform_chrome_create(void* glfw_window, AppState* state, int php_port);

// Sync AppState -> chrome views each frame.
// Returns the total chrome height in logical pixels (same contract as
// native_chrome_update on macOS).
int  platform_chrome_update(AppState* state);

// Returns the height of the bottom status bar.
int  platform_chrome_status_h();

// True when the pointer is inside any chrome view.
bool platform_chrome_has_hover();

// Notify chrome of a window resize.
void platform_chrome_resize(int win_w, int win_h);

// Focus the URL input field (Cmd+L / Ctrl+L).
void platform_chrome_focus_url();

// Tear down all chrome views on shutdown.
void platform_chrome_destroy();

// ── Per-frame cursor hook ─────────────────────────────────────────────────

// Must be called BETWEEN glfwPollEvents() and ImGui_ImplGlfw_NewFrame().
// On Windows this sets ImGuiConfigFlags_NoMouseCursorChange when the pointer
// is over a resize edge, preventing GLFW/ImGui from overriding the resize
// cursor.  No-op on macOS and Linux.
void platform_pre_imgui_newframe();

// ── Resize inset ──────────────────────────────────────────────────────────

// Returns the number of device-pixels to inset embedded content (WebView2)
// from the left, right, and bottom window edges so that the parent HWND
// receives mouse messages for resize grips.  Returns 0 when no inset is
// needed (macOS, Linux, or when the window is maximized on Windows).
int platform_resize_inset();

// ── Export bridge ─────────────────────────────────────────────────────────

// Write `content` to a file at the platform's default user-facing save path.
// Called by the native webview message handler when the PHP layer requests
// an export (X-Export-File response header).
// On all platforms this writes to the project directory next to the binary.
void platform_export_file(const std::string& filename, const std::string& content);
