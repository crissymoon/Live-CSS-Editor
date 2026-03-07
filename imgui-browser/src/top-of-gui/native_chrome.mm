// native_chrome.mm -- AppKit-native browser chrome
// Chrome (tab bar, toolbar, traffic lights) is a WKWebView child panel
// (src/chrome.html) with a JS bridge so the HTML page drives
// navigation and receives live tab state every frame.

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <QuartzCore/QuartzCore.h>
#include "native_chrome.h"
#include "app_state.h"
#include "webview.h"
#include <cstdio>
#include <algorithm>
#include <string>
#include <sstream>

// ── Static shared state ───────────────────────────────────────────────

static AppState* s_state    = nullptr;
static NSWindow* s_window   = nil;
static int       s_php_port = 0;

// forward declarations
@class XCMBridgeHandler;

static NSPanel*          s_toolbar_panel = nil;
static WKWebView*        s_toolbar_wv    = nil;
static XCMBridgeHandler* s_bridge        = nil;
static bool              s_tb_ready      = false;

static float   s_bm_btn_x    = 0.0f;
static float   s_hist_btn_x  = 0.0f;
static CGFloat s_extra_h     = 0.0f;   // extra panel height when dropdown open
static int     s_panel_win_w = 0;
static int     s_panel_win_h = 0;

// ── Color palette ─────────────────────────────────────────────────────




// ── XCMBridgeHandler -- WKScriptMessageHandler for chrome.html ────────
//
// chrome.html calls:
//   window.webkit.messageHandlers.xcmBridge.postMessage({action, data})
//
// Supported actions:
//   navigate  -- data = URL string
//   back / fwd / reload
//   devt / js / bm / showbm / hist
//   copy / cut / paste / paste_url / select_all  -- clipboard actions
//   urlfocus / urlblur  -- tell native side URL field has focus
//   tab_switch -- data = tab index (string)
//   tab_close  -- data = tab index (string)
//   tab_new    -- open default page in new tab
//   tab_move   -- data = "from,to" indices (string)
//
// Native side calls xcmSetState(stateJSON) in the toolbar WKWebView every
// frame to push live tab + chrome state.

static void xcm_status(const char* msg);  // forward declared before use below

@interface XCMBridgeHandler : NSObject <WKScriptMessageHandler>
@end
@implementation XCMBridgeHandler
- (void)userContentController:(WKUserContentController*)ucc
      didReceiveScriptMessage:(WKScriptMessage*)msg {
    if (!s_state) return;
    NSDictionary* body = msg.body;
    if (![body isKindOfClass:[NSDictionary class]]) return;
    NSString* action = body[@"action"];
    NSString* data   = body[@"data"];
    if (!action) return;

    Tab* t = s_state->current_tab();

    // After most actions key focus should return to the main browser window.
    // Exceptions: urlfocus (URL bar needs key for typing), dropdownOpen (drawer
    // interaction still needs first-click response).
    // We use a local block so every early return path below also triggers it.
    __block BOOL _returnKey = YES;
    auto returnKeyIfNeeded = ^{ if (_returnKey && s_window) [s_window makeKeyAndOrderFront:nil]; };

    if ([action isEqualToString:@"navigate"]) {
        if (t && data.length) {
            std::string url = data.UTF8String;
            s_state->push_nav(t->id, url);
            xcm_status("Navigating...");
        }
    } else if ([action isEqualToString:@"back"]) {
        if (t) { s_state->push_nav(t->id, "__back__"); xcm_status("Back"); }
    } else if ([action isEqualToString:@"fwd"]) {
        if (t) { s_state->push_nav(t->id, "__forward__"); xcm_status("Forward"); }
    } else if ([action isEqualToString:@"reload"]) {
        if (!t) { returnKeyIfNeeded(); return; }
        if (t->loading) { s_state->push_nav(t->id, "__stop__");   xcm_status("Stopped"); }
        else            { s_state->push_nav(t->id, "__reload__"); xcm_status("Reloading..."); }
    } else if ([action isEqualToString:@"devt"]) {
        s_state->dev_tools_open = !s_state->dev_tools_open;
        if (t) s_state->push_nav(t->id, "__devtools__");
        xcm_status(s_state->dev_tools_open ? "Dev tools opened" : "Dev tools closed");
    } else if ([action isEqualToString:@"js"]) {
        if (!t) { returnKeyIfNeeded(); return; }
        t->js_enabled = !t->js_enabled;
        s_state->push_nav(t->id, t->js_enabled ? "__js_on__" : "__js_off__");
        xcm_status(t->js_enabled ? "JavaScript enabled" : "JavaScript disabled");
    } else if ([action isEqualToString:@"bm"]) {
        s_state->show_bookmarks_panel = !s_state->show_bookmarks_panel;
        s_state->show_history_panel   = false;
        xcm_status(s_state->show_bookmarks_panel ? "Bookmarks" : "");
    } else if ([action isEqualToString:@"showbm"]) {
        s_state->show_bookmarks_panel = true;
        s_state->show_history_panel   = false;
        xcm_status("Bookmarks");
    } else if ([action isEqualToString:@"hist"]) {
        s_state->show_history_panel   = !s_state->show_history_panel;
        s_state->show_bookmarks_panel = false;
        xcm_status(s_state->show_history_panel ? "History" : "");
    } else if ([action isEqualToString:@"urlfocus"]) {
        // URL bar needs the toolbar panel as key window for typing -- do not return key.
        _returnKey = NO;
    } else if ([action isEqualToString:@"urlblur"]) {
        // urlblur is handled by returnKeyIfNeeded at the end.
    } else if ([action isEqualToString:@"dropdownOpen"]) {
        // Drawer stays open -- keep panel as key so item clicks register first time.
        _returnKey = NO;
        CGFloat h = data ? (CGFloat)data.floatValue : 220.0f;
        s_extra_h = h;
        if (s_toolbar_panel) {
            int ww = s_panel_win_w > 0 ? s_panel_win_w : (int)s_window.frame.size.width;
            int wh = s_panel_win_h > 0 ? s_panel_win_h : (int)s_window.frame.size.height;
            CGFloat sx = s_window.frame.origin.x;
            CGFloat chh = (CGFloat)(s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP);
            CGFloat sy = s_window.frame.origin.y + wh - chh - s_extra_h;
            [s_toolbar_panel setFrame:NSMakeRect(sx, sy, (CGFloat)ww,
                chh + s_extra_h) display:YES];
        }
    } else if ([action isEqualToString:@"dropdownClose"]) {
        s_extra_h = 0.0f;
        if (s_toolbar_panel) {
            int ww = s_panel_win_w > 0 ? s_panel_win_w : (int)s_window.frame.size.width;
            int wh = s_panel_win_h > 0 ? s_panel_win_h : (int)s_window.frame.size.height;
            CGFloat sx = s_window.frame.origin.x;
            CGFloat chh = (CGFloat)(s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP);
            CGFloat sy = s_window.frame.origin.y + wh - chh;
            [s_toolbar_panel setFrame:NSMakeRect(sx, sy, (CGFloat)ww,
                chh) display:YES];
        }
    } else if ([action isEqualToString:@"tab_switch"]) {
        int idx = data ? (int)data.integerValue : 0;
        if (idx >= 0 && idx < (int)s_state->tabs.size())
            s_state->active_tab = idx;
    } else if ([action isEqualToString:@"tab_close"]) {
        int idx = data ? (int)data.integerValue : -1;
        if (idx >= 0 && idx < (int)s_state->tabs.size())
            s_state->push_nav(-5, std::to_string(s_state->tabs[idx].id));
    } else if ([action isEqualToString:@"clear_cache"]) {
        webview_clear_cache();
        xcm_status("Cache cleared");
    } else if ([action isEqualToString:@"open_url"]) {
        // Open a URL in a new tab (used by the app launcher in the drawer).
        std::string url = data ? data.UTF8String : "";
        if (!url.empty()) s_state->push_nav(-2, url);
    } else if ([action isEqualToString:@"tab_new"]) {
        std::string url = "https://lainc.tech/how_ai/";
        s_state->push_nav(-2, url);
    } else if ([action isEqualToString:@"tab_move"]) {
        if (data) {
            NSArray* parts = [data componentsSeparatedByString:@","];
            if (parts.count >= 2) {
                int from = (int)[parts[0] integerValue];
                int to   = (int)[parts[1] integerValue];
                int n    = (int)s_state->tabs.size();
                if (from >= 0 && from < n && to >= 0 && to < n && from != to) {
                    int was = s_state->active_tab;
                    auto& tabs = s_state->tabs;
                    if (from < to) {
                        std::rotate(tabs.begin()+from, tabs.begin()+from+1, tabs.begin()+to+1);
                        if      (was == from)             s_state->active_tab = to;
                        else if (was > from && was <= to) s_state->active_tab--;
                    } else {
                        std::rotate(tabs.begin()+to, tabs.begin()+from, tabs.begin()+from+1);
                        if      (was == from)             s_state->active_tab = to;
                        else if (was >= to && was < from) s_state->active_tab++;
                    }
                }
            }
        }
    } else if ([action isEqualToString:@"copy"]) {
        if (t && t->wv_handle) webview_clipboard_action(t->wv_handle, "copy");
    } else if ([action isEqualToString:@"cut"]) {
        if (t && t->wv_handle) webview_clipboard_action(t->wv_handle, "cut");
    } else if ([action isEqualToString:@"paste"]) {
        if (t && t->wv_handle) webview_clipboard_action(t->wv_handle, "paste");
    } else if ([action isEqualToString:@"paste_url"]) {
        // The toolbar paste button always targets the URL <input> in the
        // toolbar WKWebView -- never the page content.  After inserting,
        // focus and select the field so the user sees the pasted URL and
        // can press Enter immediately to navigate.
        // _returnKey stays NO so the toolbar panel keeps key focus.
        _returnKey = NO;
        NSString* text = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
        if (text.length && s_toolbar_wv) {
            NSData* d = [NSJSONSerialization dataWithJSONObject:text
                                                       options:NSJSONWritingFragmentsAllowed
                                                         error:nil];
            if (d) {
                NSString* jsonStr = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
                NSString* js = [NSString stringWithFormat:
                    @"(function(){"
                    "  var el=document.getElementById('url');"
                    "  if(!el)return;"
                    "  var t=%@;"
                    "  el.value=t;"
                    "  el.focus();"
                    "  el.select();"
                    "  el.dispatchEvent(new Event('input',{bubbles:true}));"
                    "})()", jsonStr];
                [s_toolbar_wv evaluateJavaScript:js completionHandler:nil];
            }
        }
    } else if ([action isEqualToString:@"select_all"]) {
        if (t && t->wv_handle) webview_clipboard_action(t->wv_handle, "selectAll");
    } else if ([action isEqualToString:@"win_close"]) {
        _returnKey = NO;
        dispatch_async(dispatch_get_main_queue(), ^{ [s_window performClose:nil]; });
    } else if ([action isEqualToString:@"win_minimize"]) {
        _returnKey = NO;
        dispatch_async(dispatch_get_main_queue(), ^{ [s_window performMiniaturize:nil]; });
    } else if ([action isEqualToString:@"win_zoom"]) {
        _returnKey = NO;
        dispatch_async(dispatch_get_main_queue(), ^{ [s_window zoom:nil]; });
    } else if ([action isEqualToString:@"resize_window"]) {
        // data = "WxH" where W and H are desired viewport (content area) dimensions
        // in logical points.  The window content height = H + TOTAL_CHROME_TOP.
        int rw = 0, rh = 0;
        if (data) sscanf(data.UTF8String, "%dx%d", &rw, &rh);
        if (rw > 0 && rh > 0) {
            dispatch_async(dispatch_get_main_queue(), ^{
                // computed chrome height (ui-mode-dependent) accounts for the chrome
                // that sits above the WKWebView content area.
                int chrome_top = s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP;
                int contentH = rh + chrome_top;
                // The NSWindow frame includes the native macOS title bar.
                // Keep the top-left corner of the window fixed by adjusting
                // origin.y (Cocoa coord origin is bottom-left).
                NSRect fr   = s_window.frame;
                CGFloat titleH = fr.size.height
                               - s_window.contentView.bounds.size.height;
                CGFloat newTotalH = (CGFloat)contentH + titleH;
                fr.origin.y += fr.size.height - newTotalH;
                fr.size      = NSMakeSize((CGFloat)rw, newTotalH);
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                [s_window setFrame:fr display:YES animate:NO];
                [CATransaction commit];
            });
        }
    }
    returnKeyIfNeeded();
}
@end

// ── Status feedback helper ────────────────────────────────────────────

static void xcm_status(const char* msg) {
    if (!s_state) return;
    s_state->status_text = msg;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.5 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        if (s_state) s_state->status_text.clear();
    });
}

// ── Toolbar state push ────────────────────────────────────────────────
// Called every frame from native_chrome_update to push current tab state
// into the toolbar WKWebView as a JSON object via evaluateJavaScript.

static void toolbar_sync_state(AppState* st) {
    if (!s_toolbar_wv || !s_tb_ready) return;
    Tab* tab = st->current_tab();

    bool loading  = tab && tab->loading;
    bool canBack  = tab && tab->can_back;
    bool canFwd   = tab && tab->can_forward;
    float prog    = tab ? tab->progress : 0.0f;
    bool https    = tab && tab->url.size() >= 8 && tab->url.substr(0,8) == "https://";
    bool http     = tab && tab->url.size() >= 7 && tab->url.substr(0,7) == "http://";

    bool isBm = false;
    if (tab && !tab->url.empty())
        for (auto& b : st->bookmarks) if (b.url == tab->url) { isBm = true; break; }

    // Escape the URL for safe JS string embedding
    std::string raw_url = tab ? tab->url : "";
    std::string url_esc;
    url_esc.reserve(raw_url.size() + 16);
    for (char c : raw_url) {
        if      (c == '\\') url_esc += "\\\\";
        else if (c == '"')  url_esc += "\\\"";
        else if (c == '\n') url_esc += "\\n";
        else                url_esc += c;
    }

    // Escape hover/status text for the info panel
    const std::string& raw_st = !st->hover_url.empty() ? st->hover_url : st->status_text;
    std::string st_esc;
    st_esc.reserve(raw_st.size() + 16);
    for (char c : raw_st) {
        if      (c == '\\') st_esc += "\\\\";
        else if (c == '"')  st_esc += "\\\"";        else if (c == '\n') st_esc += "\\n";
        else                st_esc += c;
    }
    int vp_h = s_panel_win_h - st->settings.computed_top_height();
    int vp_w = s_panel_win_w;

    std::ostringstream js;
    js << "xcmSetState({"
       << "url:\""   << url_esc << "\","
       << "loading:" << (loading ? "true" : "false") << ","
       << "progress:" << prog << ","
       << "canBack:"  << (canBack ? "true" : "false") << ","
       << "canFwd:"   << (canFwd  ? "true" : "false") << ","
       << "https:"    << (https   ? "true" : "false") << ","
       << "http:"     << (http && !https ? "true" : "false") << ","
       << "devtOpen:" << (st->dev_tools_open ? "true" : "false") << ","
       << "jsOn:"     << (tab && tab->js_enabled ? "true" : "false") << ","
       << "isBm:"     << (isBm ? "true" : "false") << ","
       << "phpOk:"    << (st->php_server_ok  ? "true" : "false") << ","
       << "nodeOk:"   << (st->node_server_ok ? "true" : "false") << ","
       << "statusTxt:\"" << st_esc << "\","
       << "vpW:"      << vp_w << ","
       << "vpH:"      << vp_h << ","
       << "phpPort:" << s_php_port
       << ",tabs:[";
    for (size_t i = 0; i < st->tabs.size(); i++) {
        const auto& tb = st->tabs[i];
        std::string raw = tb.title.empty() ? tb.url : tb.title;
        if (raw.size() > 40) raw = raw.substr(0, 40);
        std::string tesc;
        tesc.reserve(raw.size() + 8);
        for (char c : raw) {
            if      (c == '\\') tesc += "\\\\";
            else if (c == '"')  tesc += "\\\"";
            else if (c == '\n') tesc += "\\n";
            else                tesc += c;
        }
        std::string fesc;
        fesc.reserve(tb.favicon.size() + 8);
        for (char c : tb.favicon) {
            if      (c == '\\') fesc += "\\\\";
            else if (c == '"')  fesc += "\\\"";
            else if (c == '\n') fesc += "\\n";
            else                fesc += c;
        }
        js << (i > 0 ? "," : "")
           << "{title:\"" << tesc << "\",loading:"
           << (tb.loading ? "true" : "false")
           << ",favicon:\"" << fesc << "\"}";
    }
    js << "],activeTab:"   << st->active_tab
       << ",showTabs:"     << (st->settings.show_tabs    ? "true" : "false")
       << ",showToolbar:"  << (st->settings.show_toolbar ? "true" : "false")
       << "});";    

    NSString* jsStr = [NSString stringWithUTF8String:js.str().c_str()];
    [s_toolbar_wv evaluateJavaScript:jsStr completionHandler:nil];

    // Floating panel right-edge anchors.  Use the right edge of the panel.
    NSRect pf = s_toolbar_panel.frame;
    s_bm_btn_x   = (float)(pf.origin.x + pf.size.width - 8.0);
    s_hist_btn_x = s_bm_btn_x;
}


// ── XCMToolbarPanel ──────────────────────────────────────────────────
// Borderless child NSPanel holding the toolbar WKWebView (toolbar.html).
// Being a separate NSWindow from the GLFW GL view means it gets its own
// AppKit event pipeline -- no GL interception at all.

@interface XCMToolbarPanel : NSPanel
@end
@implementation XCMToolbarPanel
- (BOOL)canBecomeKeyWindow       { return YES; }
- (BOOL)canBecomeMainWindow      { return NO;  }
// Always receive mouse-moved events so CSS :hover in the WKWebView stays
// active even when the user has focus in the browser content area or URL field.
- (BOOL)acceptsMouseMovedEvents  { return YES; }
@end

// ── WKWebView navigation delegate for toolbar ─────────────────────────

@interface XCMToolbarNavDelegate : NSObject <WKNavigationDelegate>
@end
@implementation XCMToolbarNavDelegate
- (void)webView:(WKWebView*)wv didFinishNavigation:(WKNavigation*)nav {
    s_tb_ready = true;
}
- (void)webView:(WKWebView*)wv didFailNavigation:(WKNavigation*)nav
      withError:(NSError*)err {
    fprintf(stderr, "[toolbar] nav failed: %s\n", err.localizedDescription.UTF8String);
}
@end

// ── Public C API ──────────────────────────────────────────────────────

static XCMToolbarNavDelegate* s_tb_nav = nil;

// Locate chrome.html relative to the running executable.
// Layout: imgui_browser.app/Contents/MacOS/imgui_browser  (4 levels above project root)
//         imgui_browser.app/Contents/Resources/chrome.html
// For dev builds: MacOS/ -> Contents/ -> .app/ -> build/ -> project/ -> src/
static NSURL* toolbarHtmlURL() {
    // 1. Dev build: 4 levels up from MacOS/ to reach imgui-browser/, then src/
    NSString* binDir = [NSBundle mainBundle].executablePath.stringByDeletingLastPathComponent;
    NSString* dev1   = [binDir stringByAppendingPathComponent:@"../../../../src/top-of-gui/chrome.html"];
    NSString* dev1r  = dev1.stringByStandardizingPath;
    if ([[NSFileManager defaultManager] fileExistsAtPath:dev1r])
        return [NSURL fileURLWithPath:dev1r];
    // 2. App bundle Resources
    NSURL* res = [[NSBundle mainBundle] URLForResource:@"chrome" withExtension:@"html"];
    if (res) return res;
    // 3. Same directory as binary
    NSString* same = [binDir stringByAppendingPathComponent:@"chrome.html"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:same])
        return [NSURL fileURLWithPath:same];
    fprintf(stderr, "[chrome] WARNING: chrome.html not found\n");
    return nil;
}

void native_chrome_create(void* ns_window, AppState* state, int php_port) {
    s_state    = state;
    s_window   = (__bridge NSWindow*)ns_window;
    s_php_port = php_port;

    [s_window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];

    NSView* cv    = s_window.contentView;
    CGFloat win_w = cv.bounds.size.width;
    CGFloat win_h = cv.bounds.size.height;

    // Chrome panel (WKWebView) -- height determined by settings.computed_top_height() (ui mode)
    {
        CGFloat ph = (CGFloat)s_state->settings.computed_top_height();
        CGFloat sx = s_window.frame.origin.x;
        CGFloat sy = s_window.frame.origin.y + win_h - ph;
        NSRect panelFrame = NSMakeRect(sx, sy, win_w, ph);

        s_toolbar_panel = [[XCMToolbarPanel alloc]
            initWithContentRect:panelFrame
                      styleMask:NSWindowStyleMaskBorderless
                        backing:NSBackingStoreBuffered
                          defer:NO];
        s_toolbar_panel.opaque                  = NO;
        s_toolbar_panel.backgroundColor         = NSColor.clearColor;
        s_toolbar_panel.hasShadow               = NO;
        s_toolbar_panel.acceptsMouseMovedEvents = YES;
        [s_toolbar_panel setAppearance:
            [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        [s_toolbar_panel setReleasedWhenClosed:NO];

        // WKWebView configuration with JS bridge
        WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
        cfg.suppressesIncrementalRendering = NO;

        s_bridge = [[XCMBridgeHandler alloc] init];
        [cfg.userContentController addScriptMessageHandler:s_bridge name:@"xcmBridge"];

        // Disable text selection in the toolbar (keep contextmenu for custom right-click menu)
        NSString* lockdown =
            @"document.addEventListener('selectstart',function(e){"
               "var t=e.target;if(t.tagName==='INPUT'||t.tagName==='TEXTAREA')return;"
               "e.preventDefault();"
             "});";;
        WKUserScript* lk = [[WKUserScript alloc]
            initWithSource:lockdown
             injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
          forMainFrameOnly:YES];
        [cfg.userContentController addUserScript:lk];

        cfg.defaultWebpagePreferences.allowsContentJavaScript = YES;

        s_toolbar_wv = [[WKWebView alloc]
            initWithFrame:NSMakeRect(0, 0, win_w, ph)
            configuration:cfg];
        s_toolbar_wv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        // Seed panel win size so dropdownOpen works before the first resize call
        s_panel_win_w = (int)win_w;
        s_panel_win_h = (int)win_h;

        // Transparent background so corners show through until first paint
        [s_toolbar_wv setValue:@NO forKey:@"drawsBackground"];

        s_tb_nav = [[XCMToolbarNavDelegate alloc] init];
        s_toolbar_wv.navigationDelegate = s_tb_nav;

        [s_toolbar_panel.contentView addSubview:s_toolbar_wv];

        // Round the top two corners to match the macOS window frame.
        // We clip at the contentView level, not the WKWebView, because
        // WKWebView has internal sublayers that can render outside its own
        // top-level layer, making masksToBounds on WKWebView unreliable.
        // Clipping the plain NSView contentView is guaranteed to work.
        NSView* cv = s_toolbar_panel.contentView;
        cv.wantsLayer              = YES;
        cv.layer.backgroundColor   = NSColor.clearColor.CGColor;
        cv.layer.cornerRadius      = 10.0;
        cv.layer.maskedCorners     =
            kCALayerMinXMaxYCorner | kCALayerMaxXMaxYCorner; // top-left + top-right
        cv.layer.masksToBounds     = YES;

        [s_window addChildWindow:s_toolbar_panel ordered:NSWindowAbove];

        // Load chrome.html
        NSURL* url = toolbarHtmlURL();
        if (url) {
            [s_toolbar_wv loadFileURL:url
              allowingReadAccessToURL:url.URLByDeletingLastPathComponent];
        } else {
            NSString* fallback =
                @"<html><body style='background:#101820;color:#e6e9f5;"
                 "font:14px -apple-system;display:flex;align-items:center;"
                 "height:114px;padding:0 8px'>chrome.html not found</body></html>";
            [s_toolbar_wv loadHTMLString:fallback baseURL:nil];
        }
    }

    fprintf(stderr, "[chrome] native chrome created w=%.0f h=%.0f\n", win_w, win_h);

    // ── Mouse event monitor ──────────────────────────────────────────────────
    // Two concerns only:
    //  1. Chrome panel top 32px (excluding traffic-light column) = drag strip.
    //  2. Traffic lights are now HTML buttons in chrome.html -- do NOT intercept
    //     their zone; let WKWebView receive the click so the JS bridge fires.
    // Everything else is returned untouched -- do NOT call makeKeyAndOrderFront
    // here because that steals key window from the panel and kills WKWebView
    // onclick dispatch before the event reaches JavaScript.
    // Key restoration for the main window happens exclusively through bridge
    // actions (urlblur, dropdownClose, navigation, etc.).
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                          handler:^NSEvent*(NSEvent* ev) {
        if (s_toolbar_panel && ev.window == s_toolbar_panel) {
            NSPoint p      = ev.locationInWindow; // Cocoa: (0,0) = bottom-left
            CGFloat panelH = s_toolbar_panel.frame.size.height;

            // Drag strip: top 32px, but skip the traffic-light column so HTML
            // buttons (#tl-close/#tl-minimize/#tl-zoom) receive their clicks.
            if (p.y > panelH - 32.0f && p.x >= (CGFloat)TRAFFIC_LIGHT_W) {
                if (ev.clickCount == 2) {
                    // Double-click on drag bar: zoom (maximize) the window.
                    [s_window zoom:nil];
                } else {
                    [s_window performWindowDragWithEvent:ev];
                }
                return nil;
            }

            // All other panel clicks: make panel key now so WKWebView
            // receives this event without a second click.
            if (![s_toolbar_panel isKeyWindow]) {
                [s_toolbar_panel makeKeyAndOrderFront:nil];
            }
        }
        return ev;
    }];

    // ── JS-driven hover state ─────────────────────────────────────────
    // CSS :hover only fires when the panel is key. It almost never is.
    // We drive hover through JS instead: xcmMouseMove(x,y) is called by
    // this monitor and uses document.elementFromPoint to walk the DOM and
    // add/remove "js-hover" on every element + ancestor under the cursor.
    // All CSS :hover rules are duplicated as .js-hover equivalents below.
    //
    // Throttle: only evaluate JS when position changes by >= 1 screen-pt.
    // Without this the JS queue fills up faster than WKWebView drains it
    // and events are silently dropped -- which is why hover only worked
    // after a click (click flushed the queue).
    {
        __block CGFloat lastX = -9999.f;
        __block CGFloat lastY = -9999.f;

        [NSEvent addLocalMonitorForEventsMatchingMask:
            NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp
                                              handler:^NSEvent*(NSEvent* ev) {
            if (!s_toolbar_panel || !s_toolbar_wv || !s_tb_ready) return ev;

            if (ev.type == NSEventTypeLeftMouseUp) {
                lastX = -9999.f; lastY = -9999.f;
                [s_toolbar_wv evaluateJavaScript:
                    @"typeof xcmMouseLeave==='function'&&xcmMouseLeave()"
                               completionHandler:nil];
                return ev;
            }

            NSPoint screenPt = [NSEvent mouseLocation];

            if (!NSPointInRect(screenPt, s_toolbar_panel.frame)) {
                if (lastX != -9999.f) {
                    lastX = -9999.f; lastY = -9999.f;
                    [s_toolbar_wv evaluateJavaScript:
                        @"typeof xcmMouseLeave==='function'&&xcmMouseLeave()"
                                   completionHandler:nil];
                }
                return ev;
            }

            NSPoint p  = [s_toolbar_panel convertPointFromScreen:screenPt];
            CGFloat cx = p.x;
            CGFloat cy = s_toolbar_panel.frame.size.height - p.y; // flip to CSS Y

            CGFloat dx = cx - lastX;
            CGFloat dy = cy - lastY;
            if (dx*dx + dy*dy < 1.f) return ev;   // skip if barely moved
            lastX = cx; lastY = cy;

            NSString* js = [NSString stringWithFormat:
                @"typeof xcmMouseMove==='function'&&xcmMouseMove(%.1f,%.1f)", cx, cy];
            [s_toolbar_wv evaluateJavaScript:js completionHandler:nil];
            return ev;
        }];
    }

    // ── Minimize / Deminiaturize observers ────────────────────────────
    // When the parent window miniaturizes macOS detaches child panels.
    // Hide the panel before the animation starts so it doesn't float
    // detached, then re-attach and reposition it on restore.
    [[NSNotificationCenter defaultCenter]
        addObserverForName:NSWindowWillMiniaturizeNotification
                    object:s_window
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification*) {
        if (s_toolbar_panel) [s_toolbar_panel orderOut:nil];
    }];

    [[NSNotificationCenter defaultCenter]
        addObserverForName:NSWindowDidDeminiaturizeNotification
                    object:s_window
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification*) {
        if (!s_toolbar_panel || !s_window) return;

        // Re-add as child if macOS removed it during miniaturize.
        if (![s_window.childWindows containsObject:s_toolbar_panel]) {
            [s_window addChildWindow:s_toolbar_panel ordered:NSWindowAbove];
        }

        // Snap back to the correct position (frame may have drifted).
        int ww = s_panel_win_w > 0 ? s_panel_win_w : (int)s_window.frame.size.width;
        int wh = s_panel_win_h > 0 ? s_panel_win_h : (int)s_window.frame.size.height;
        CGFloat sx = s_window.frame.origin.x;
        int _dh = s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP;
        CGFloat sy = s_window.frame.origin.y + (CGFloat)wh - (CGFloat)_dh;
        [s_toolbar_panel setFrame:NSMakeRect(sx, sy, (CGFloat)ww,
                                             (CGFloat)_dh + s_extra_h)
                          display:YES];
        [s_toolbar_panel orderFront:nil];
    }];

    // When the parent regains key status after switching back from another app,
    // ensure the panel is ordered above the main window.
    [[NSNotificationCenter defaultCenter]
        addObserverForName:NSWindowDidBecomeKeyNotification
                    object:s_window
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification*) {
        if (s_toolbar_panel) [s_toolbar_panel orderFront:nil];
    }];
}

int native_chrome_update(AppState* st) {
    if (!s_toolbar_panel) return s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP;

    // Cmd+L: focus toolbar URL field
    if (st->focus_url_next_frame) {
        [s_toolbar_panel makeKeyAndOrderFront:nil];
        if (s_toolbar_wv)
            [s_toolbar_wv evaluateJavaScript:
                @"var u=document.getElementById('url');if(u){u.focus();u.select();}"
                          completionHandler:nil];
        st->focus_url_next_frame = false;
    }

    // Push state into toolbar WKWebView every frame
    toolbar_sync_state(st);

    return st->settings.computed_top_height();
}

int  native_chrome_status_h() { return 0; }

bool native_chrome_has_hover() {
    if (!s_window) return false;
    // Also check if the toolbar panel is key (URL field focused)
    if (s_toolbar_panel && [s_toolbar_panel isKeyWindow]) return true;
    NSPoint mp    = [s_window mouseLocationOutsideOfEventStream];
    CGFloat win_h = s_window.contentView.bounds.size.height;
    CGFloat chrome_bottom = win_h - (CGFloat)(s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP);
    bool in_top    = (mp.y >= chrome_bottom);
    return in_top;
}

void native_chrome_resize(int win_w, int win_h) {
    if (!s_toolbar_panel) return;

    s_panel_win_w = win_w;
    s_panel_win_h = win_h;

    // Also close any open drawer when the window resizes
    if (s_extra_h > 0 && s_toolbar_wv)
        [s_toolbar_wv evaluateJavaScript:@"closeDrawerSilent&&closeDrawerSilent()" completionHandler:nil];
    s_extra_h = 0.0f;

    CGFloat sx = s_window.frame.origin.x;
    int dh = s_state ? s_state->settings.computed_top_height() : TOTAL_CHROME_TOP;
    CGFloat sy = s_window.frame.origin.y + (CGFloat)win_h - (CGFloat)dh;
    NSRect pf  = NSMakeRect(sx, sy, (CGFloat)win_w, (CGFloat)dh);
    [s_toolbar_panel setFrame:pf display:NO];
}

float native_chrome_bm_btn_x()   { return s_bm_btn_x;   }
float native_chrome_hist_btn_x() { return s_hist_btn_x; }

void native_chrome_focus_url() {
    if (!s_toolbar_panel || !s_toolbar_wv) return;
    [s_toolbar_panel makeKeyAndOrderFront:nil];
    [s_toolbar_wv evaluateJavaScript:
        @"var u=document.getElementById('url');if(u){u.focus();u.select();}"
                  completionHandler:nil];
}

void native_chrome_destroy() {
    if (s_toolbar_panel) {
        [s_window removeChildWindow:s_toolbar_panel];
        [s_toolbar_panel close];
        s_toolbar_panel = nil;
    }
    s_toolbar_wv = nil;
    s_bridge     = nil;
    s_tb_nav     = nil;
}

void native_chrome_eval_toolbar_js(const char* js) {
    if (!s_toolbar_wv || !js) return;
    NSString* src = [NSString stringWithUTF8String:js];
    dispatch_async(dispatch_get_main_queue(), ^{
        [s_toolbar_wv evaluateJavaScript:src completionHandler:nil];
    });
}

