// xcm_shell.h -- Unified action shell for XCM browser.
//
// All download, copy, save, and context-menu actions that macOS or WKWebView
// restrict in an embedded webview are funnelled through this module so there
// is exactly one place to intercept, log, theme, and extend them.
//
// Architecture
// ============
//   - One shared WKScriptMessageHandler (message name: "xcmShell")
//   - JS injected at document-start intercepts:
//       * contextmenu events (suppresses native menu, shows our themed NSMenu)
//       * blob: / data: URL download anchor clicks
//   - decidePolicyForNavigationAction respects shouldPerformDownload (macOS 11.3+)
//   - NSURLSession (seeded with WKWebsiteDataStore cookies) handles http/https
//     image saves and copies, so authentication is preserved
//   - All panels use NSAppearanceNameDarkAqua + cyan (#00FFFF) accent text
//
// Usage
// =====
//   // Once, after the WKWebView config is set up:
//   xcm_shell_install(cfg.userContentController, window, state);
//
//   // Each time the active/focused tab changes:
//   xcm_shell_set_webview(handle->wv, handle->tab_id);

#pragma once
#include "app_state.h"

#ifdef __APPLE__
#  import <WebKit/WebKit.h>
#  import <Cocoa/Cocoa.h>

// Install the shell message handler and inject the JS interceptor into a
// WKUserContentController.  Call once per WKWebView (including popup windows).
// window: the NSWindow that owns the WKWebView (used for panel positioning).
// state:  AppState for push_nav() (tab open / navigation commands).
void xcm_shell_install(WKUserContentController* ucc,
                       NSWindow*               window,
                       AppState*               state);

// Call whenever the visible/focused tab changes so that "Save Image As" /
// "Copy Image" JS evaluation targets the correct WKWebView.
void xcm_shell_set_webview(WKWebView* wv, int tabId);

#else
// Non-Apple platforms: xcm_shell is a no-op (implemented in xcm_shell_stub.cpp).
// The native bridge postMessage export path is handled in platform_*.cpp instead.
void xcm_shell_install_stub(void* ucc, void* window, AppState* state);
void xcm_shell_set_webview_stub(void* wv, int tabId);
#endif
