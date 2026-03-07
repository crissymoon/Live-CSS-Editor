// xcm_shell_stub.cpp -- No-op xcm_shell implementation for non-Apple platforms.
//
// On macOS xcm_shell.mm provides real NSMenu / WKScriptMessageHandler-based
// context menus and download interception.  On Linux and Windows those are
// either not applicable or handled through the platform-specific webview
// implementation (webview_linux.cpp / webview_win.cpp).
//
// The export bridge on non-Apple platforms is implemented in platform_linux.cpp
// and platform_win.cpp via the webview's native postMessage mechanism.

#include "xcm_shell.h"

void xcm_shell_install_stub(void* /*ucc*/, void* /*window*/, AppState* /*state*/) {}
void xcm_shell_set_webview_stub(void* /*wv*/, int /*tabId*/) {}
