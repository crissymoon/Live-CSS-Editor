#pragma once
// chrome_traffic_lights.h -- macOS-style traffic light close/min/zoom buttons.
// Windows/Linux only (no-op on Apple).
//
// On Windows, uses a native GDI+ child HWND overlay so painting bypasses
// the OpenGL/ImGui pipeline entirely -- no z-order issues possible.

#include "app_state.h"

#ifndef __APPLE__

// Create the native overlay child window.  Call once after the main HWND
// exists (from platform_app_postinit or similar).
void chrome_tl_create(void* parent_hwnd, AppState* st);

// Invalidate and repaint the overlay.  Call once per frame (e.g. after
// glfwSwapBuffers) so hover animation stays smooth.
void chrome_tl_update();

// Destroy the overlay.  Call before the parent HWND is destroyed.
void chrome_tl_destroy();

#endif
