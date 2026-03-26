#pragma once
// chrome_traffic_lights.h -- macOS-style traffic light close/min/zoom buttons.
// Windows/Linux only (no-op on Apple).
//
// On Windows, uses GDI+ painted directly on the parent HWND's front buffer
// after each SwapBuffers call.  No child HWND (would flicker over OpenGL).

#include "app_state.h"

#ifndef __APPLE__

// Initialise GDI+ and store the parent HWND.  Call once after the main HWND
// exists (from platform_app_postinit or main entry).
void chrome_tl_create(void* parent_hwnd, AppState* st);

// Paint the traffic lights on the parent's front buffer and update hover
// state.  Call once per frame after glfwSwapBuffers.
void chrome_tl_update();

// Try to handle a left-button click at (client_x, client_y).
// Returns true if the click was on a traffic light button (and was handled).
// Call from the parent wndproc on WM_LBUTTONDOWN before passing to GLFW.
bool chrome_tl_try_click(int client_x, int client_y);

// Returns true if the given client point is over a traffic light circle.
// Useful for the parent wndproc to suppress drag / ImGui interaction there.
bool chrome_tl_hit(int client_x, int client_y);

// Shut down GDI+.  Call before the parent HWND is destroyed.
void chrome_tl_destroy();

#endif
