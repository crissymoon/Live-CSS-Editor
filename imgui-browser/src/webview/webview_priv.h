// webview_priv.h -- Internal shared state for the split webview module.
// Included by every webview_*.mm file.  NOT part of the public API.
// Do NOT include this header from outside the webview module.
#pragma once

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Network/Network.h>
#import <CFNetwork/CFNetwork.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <Security/SecTrust.h>

#include "../webview.h"
#include "../app_state.h"
#include "../xcm_shell.h"
#include "../top-of-gui/native_chrome.h"
#include "webview_js.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <cstring>

// ── ObjC delegate interfaces ──────────────────────────────────────────
// Full @interface declarations are placed here so every webview_*.mm file
// can allocate and configure these objects without needing to see the
// @implementation in webview_delegates.mm.

@interface XCMDownloadDelegate : NSObject <WKDownloadDelegate>
@end

@interface XCMVirtDelegate : NSObject <WKNavigationDelegate, WKUIDelegate, NSWindowDelegate>
@property (nonatomic, strong) NSPanel*   panel;
@property (nonatomic, strong) WKWebView* secWV;
@end

@interface XCMPopupDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@property (nonatomic, strong) NSPanel*    panel;
@property (nonatomic, strong) WKWebView*  popupWV;
@property (nonatomic, assign) int         parentTabId;
@end

@interface XCMNavDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@property (nonatomic, assign) int tabId;
@property (nonatomic, assign) int retryConnect;
@end

@interface XCMKvoObserver : NSObject
@property (nonatomic, assign) int tabId;
@property (nonatomic, weak)   WKWebView* webView;
@end

// ── Per-tab handle ─────────────────────────────────────────────────────
struct WVHandle {
    int              tab_id     = 0;
    WKWebView*       wv         = nil;
    NSView*          host       = nil;
    XCMNavDelegate*  nav_del    = nil;
    XCMKvoObserver*  kvo        = nil;
    NSTimer*         fps_tmr    = nil;
    bool             js_enabled = true;
};

// ── Shared mutable state (defined in webview_state.mm) ────────────────
extern NSWindow*          s_window;
extern AppState*          s_state;
extern WebViewCallbacks   s_cbs;
extern WKProcessPool*     s_process_pool;
extern WKContentRuleList* s_ad_rule_list;
extern std::unordered_map<std::string, std::string> s_report_to;
extern std::unordered_map<int, bool>                s_js_enabled;
extern std::unordered_map<int, WVHandle*>           s_handles;

// ── Delegate container state (defined in webview_delegates.mm) ────────
extern XCMDownloadDelegate*               s_dl_delegate;
extern NSMutableArray<XCMPopupDelegate*>* s_popup_delegates;
extern NSMutableArray*                    s_virt_delegates;

// ── Auth popup patterns (defined in webview_popup.mm) ─────────────────
extern const char* const s_popup_patterns[];
bool xcm_is_auth_popup_url(const char* url);

// ── Internal helpers ──────────────────────────────────────────────────
// Inject document-start user scripts into a WKUserContentController.
// Called for every new WKWebView (main tabs and popup windows).
// Defined in webview_core.mm.
void xcm_inject_masks(WKUserContentController* ucc);
