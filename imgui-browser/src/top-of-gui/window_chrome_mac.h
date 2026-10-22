#pragma once
// window_chrome_mac.h -- macOS window shape and title-bar behaviour.
//
// The browser chrome (tab bar + toolbar) lives in a borderless NSPanel laid
// over the top edge of the GLFW window.  That panel owns the window's top
// corners and the whole drag strip, so it has to reproduce three things
// AppKit would otherwise do for us:
//
//   1. the OS corner radius, so the window still looks like a Mac window;
//   2. the system title-bar double-click action (zoom / minimise / none);
//   3. reliable double-click detection, because the drag session started by
//      the first mouse down eats AppKit's click counter.
//
// All three are implemented here once and shared by the main window monitor
// (main.mm) and the chrome panel monitor (native_chrome.mm).

#ifdef __cplusplus
extern "C" {
#endif

// Corner radius macOS draws on the window frame, in points.  Read from the
// window itself so it follows the OS (10 pt on Big Sur .. Sonoma, 16 pt on
// Tahoe).  Falls back to 10 when the value is not exposed.
double xcm_window_corner_radius(void* ns_window);

// Round the top two corners of ns_view with the window's own radius and clip
// its subviews.  Call after the view has a superview.
void xcm_round_chrome_view(void* ns_view, void* ns_window);

// Run the action macOS has bound to a title-bar double click, from
// NSGlobalDomain AppleActionOnDoubleClick: "Maximize" (default), "Minimize"
// or "None".
void xcm_titlebar_double_click(void* ns_window);

// Feed one left mouse down.  Returns true when it completes a double click.
// click_count comes from NSEvent; the timestamp/coordinates are used for the
// fallback test that survives the swallowed first click.
bool xcm_is_double_click(double timestamp, double x, double y, int click_count);

#ifdef __cplusplus
}
#endif
