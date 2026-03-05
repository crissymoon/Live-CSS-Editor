// webview.mm -- WKWebView bridge (Objective-C++)
// Embeds native WKWebView instances inside the GLFW/OpenGL NSWindow.
// ImGui draws the chrome; WKWebView fills the content area below it.
// ARC enabled via -fobjc-arc compiler flag (see CMakeLists.txt).

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "webview.h"
#include "app_state.h"
#include <string>
#include <unordered_map>
#include <functional>

// ── Internal state ────────────────────────────────────────────────────

static NSWindow*         s_window   = nil;
static AppState*         s_state    = nullptr;
static WebViewCallbacks  s_cbs;

// FPS probe: injected JS reads __xcmStats.fps() every 2 seconds.
// We piggyback on the existing stats-inject.js from dev-browser/src/.
static NSString* const JS_FPS_PROBE = @"(function(){"
    "var s=window.__xcmStats||window.__xcm&&window.__xcm.stats;"
    "var fps=s&&typeof s.fps==='function'?Math.round(+s.fps()||0):0;"
    "var hz=window.__xcmHz||0;"
    "return [fps,hz];"
    "})()";

// User script injected at document-start: disables the native WKWebView
// status bar (link-hover overlay) and fires performance hints.
static NSString* const JS_INIT = @"(function(){"
    // Disable the default status bar that would appear at the bottom.
    "Object.defineProperty(window,'status',{set:function(){},get:function(){return '';}});"
    // Tell stats-inject the display Hz (filled by the host after detection).
    "window.__xcmImguiHost=true;"
    "})();";

// JS to inject at document-start to kill rubber-band / focus zoom etc.
static NSString* const JS_SCROLL_KILL = @"(function(){"
    "var _orig=window.scrollTo;"
    "window.scrollTo=function(x,y){if(typeof x==='object'&&x.behavior==='smooth')"
    "{_orig.call(window,x);}else{_orig.call(window,x,y);}};"
    "})();";

// ── WKWebView delegate (Obj-C class) ─────────────────────────────────

@interface XCMNavDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@property (nonatomic, assign) int tabId;
@end

@implementation XCMNavDelegate

- (void)webView:(WKWebView*)wv didStartProvisionalNavigation:(WKNavigation*)nav {
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, true);
}
- (void)webView:(WKWebView*)wv didFinishNavigation:(WKNavigation*)nav {
    if (s_cbs.on_loading)   s_cbs.on_loading(self.tabId, false);
    if (s_cbs.on_nav_state) s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
}
- (void)webView:(WKWebView*)wv didFailNavigation:(WKNavigation*)nav withError:(NSError*)err {
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, false);
}
- (void)webView:(WKWebView*)wv didFailProvisionalNavigation:(WKNavigation*)nav withError:(NSError*)err {
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, false);
}

// Called on every committed navigation (including fragment changes).
- (void)webView:(WKWebView*)wv
    didCommitNavigation:(WKNavigation*)nav {
    NSString* u = wv.URL.absoluteString ?: @"";
    if (s_cbs.on_url_change) s_cbs.on_url_change(self.tabId, u.UTF8String);
    if (s_cbs.on_nav_state)  s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
}

// New tab requests (window.open, target=_blank) -- open in same view for now.
- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {
    [wv loadRequest:action.request];
    return nil;
}
@end

// ── KVO observer for title + estimatedProgress ───────────────────────

@interface XCMKvoObserver : NSObject
@property (nonatomic, assign) int tabId;
@property (nonatomic, weak)   WKWebView* webView;
@end

@implementation XCMKvoObserver
- (void)observeValueForKeyPath:(NSString*)kp
                      ofObject:(id)obj
                        change:(NSDictionary*)ch
                       context:(void*)ctx {
    if ([kp isEqualToString:@"title"]) {
        NSString* t = ((WKWebView*)obj).title ?: @"";
        if (s_cbs.on_title_change) s_cbs.on_title_change(self.tabId, t.UTF8String);
    } else if ([kp isEqualToString:@"estimatedProgress"]) {
        float p = (float)((WKWebView*)obj).estimatedProgress;
        if (s_cbs.on_progress) s_cbs.on_progress(self.tabId, p);
    } else if ([kp isEqualToString:@"URL"]) {
        NSString* u = ((WKWebView*)obj).URL.absoluteString ?: @"";
        if (s_cbs.on_url_change) s_cbs.on_url_change(self.tabId, u.UTF8String);
    }
}
@end

// ── Per-handle container ──────────────────────────────────────────────

struct WVHandle {
    int              tab_id   = 0;
    WKWebView*       wv       = nil;
    NSView*          host     = nil;  // transparent NSView parent
    XCMNavDelegate*  nav_del  = nil;
    XCMKvoObserver*  kvo      = nil;
    NSTimer*         fps_tmr  = nil;
};

static std::unordered_map<int, WVHandle*> s_handles;  // tab_id -> handle

// ── Public API ────────────────────────────────────────────────────────

void webview_init(void* ns_window, AppState* state, WebViewCallbacks cbs) {
    s_window = (__bridge NSWindow*)ns_window;
    s_state  = state;
    s_cbs    = std::move(cbs);
}

void* webview_create(int tab_id, const std::string& url) {
    NSCAssert(s_window != nil, @"webview_init() must be called first");

    WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
    cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore;

    // Preferences
    WKPreferences* prefs = cfg.preferences;
    // Allow devtools in debug builds
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    // World script: JS_INIT injected at doc-start in every frame
    WKUserScript* initScript = [[WKUserScript alloc]
        initWithSource:JS_INIT
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:NO];
    WKUserScript* scrollKill = [[WKUserScript alloc]
        initWithSource:JS_SCROLL_KILL
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:YES];
    [cfg.userContentController addUserScript:initScript];
    [cfg.userContentController addUserScript:scrollKill];

    // Frame: full content area (will be repositioned by webview_resize)
    NSRect frame = s_window.contentView.bounds;
    frame.origin.y    = STATUS_HEIGHT_PX;
    frame.size.height -= (TOTAL_CHROME_TOP + STATUS_HEIGHT_PX);

    WKWebView* wv = [[WKWebView alloc] initWithFrame:frame configuration:cfg];
    wv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Disable the link-preview status bar overlay
    if ([wv respondsToSelector:@selector(_setStatusBarEnabled:)])
        [wv performSelector:@selector(_setStatusBarEnabled:) withObject:@NO];
    if ([wv respondsToSelector:@selector(setAllowsBackForwardNavigationGestures:)])
        wv.allowsBackForwardNavigationGestures = YES;

    // Delegates
    XCMNavDelegate* nav_del  = [[XCMNavDelegate alloc] init];
    nav_del.tabId = tab_id;
    wv.navigationDelegate = nav_del;
    wv.UIDelegate         = nav_del;

    // KVO
    XCMKvoObserver* kvo = [[XCMKvoObserver alloc] init];
    kvo.tabId  = tab_id;
    kvo.webView = wv;
    [wv addObserver:kvo forKeyPath:@"title"             options:NSKeyValueObservingOptionNew context:nil];
    [wv addObserver:kvo forKeyPath:@"estimatedProgress" options:NSKeyValueObservingOptionNew context:nil];
    [wv addObserver:kvo forKeyPath:@"URL"               options:NSKeyValueObservingOptionNew context:nil];

    // Add as content-view subview
    [s_window.contentView addSubview:wv];

    // FPS probe: every 2 seconds read stats-inject metrics from page JS
    NSTimer* fps_tmr = [NSTimer scheduledTimerWithTimeInterval:2.0
        repeats:YES
        block:^(NSTimer* _) {
            [wv evaluateJavaScript:JS_FPS_PROBE completionHandler:^(id res, NSError* err) {
                if (err || !res) return;
                if ([res isKindOfClass:[NSArray class]]) {
                    NSArray* arr = (NSArray*)res;
                    double fps = arr.count > 0 ? [arr[0] doubleValue] : 0.0;
                    if (s_cbs.on_wkwv_fps) s_cbs.on_wkwv_fps(fps);
                }
            }];
        }];

    // Store handle
    WVHandle* h = new WVHandle{};
    h->tab_id  = tab_id;
    h->wv      = wv;
    h->nav_del = nav_del;
    h->kvo     = kvo;
    h->fps_tmr = fps_tmr;
    s_handles[tab_id] = h;

    if (!url.empty()) {
        webview_load_url(h, url);
    }

    return (void*)h;
}

void webview_destroy(void* handle) {
    if (!handle) return;
    WVHandle* h = (WVHandle*)handle;
    [h->fps_tmr invalidate];
    [h->wv removeObserver:h->kvo forKeyPath:@"title"];
    [h->wv removeObserver:h->kvo forKeyPath:@"estimatedProgress"];
    [h->wv removeObserver:h->kvo forKeyPath:@"URL"];
    [h->wv removeFromSuperview];
    s_handles.erase(h->tab_id);
    delete h;
}

void webview_show(void* handle) {
    if (!handle) return;
    ((WVHandle*)handle)->wv.hidden = NO;
}

void webview_hide(void* handle) {
    if (!handle) return;
    ((WVHandle*)handle)->wv.hidden = YES;
}

void webview_resize(void* handle, int x, int y, int w, int h) {
    if (!handle) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    // Convert from top-left coordinates (used by ImGui / GLFW) to
    // bottom-left coordinates (used by NSView/Cocoa).
    NSView* cv      = s_window.contentView;
    CGFloat cv_h    = cv.bounds.size.height;
    // Flip Y: NSView origin is bottom-left
    CGFloat ns_y    = cv_h - y - h;
    [wv setFrame:NSMakeRect(x, ns_y, w, h)];
}

void webview_load_url(void* handle, const std::string& url) {
    if (!handle || url.empty()) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    NSString*  u  = [NSString stringWithUTF8String:url.c_str()];
    NSURL*     ns = [NSURL URLWithString:u];
    if (!ns) {
        // Try percent-encoding
        ns = [NSURL URLWithString:
              [u stringByAddingPercentEncodingWithAllowedCharacters:
               NSCharacterSet.URLQueryAllowedCharacterSet]];
    }
    if (!ns) return;
    [wv loadRequest:[NSURLRequest requestWithURL:ns]];
}

void webview_go_back(void* handle) {
    if (!handle) return;
    [((WVHandle*)handle)->wv goBack];
}
void webview_go_forward(void* handle) {
    if (!handle) return;
    [((WVHandle*)handle)->wv goForward];
}
void webview_reload(void* handle) {
    if (!handle) return;
    [((WVHandle*)handle)->wv reload];
}
void webview_stop(void* handle) {
    if (!handle) return;
    [((WVHandle*)handle)->wv stopLoading];
}

void webview_eval_js(void* handle,
                     const std::string& script,
                     std::function<void(const std::string&)> cb) {
    if (!handle) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    NSString*  js = [NSString stringWithUTF8String:script.c_str()];
    [wv evaluateJavaScript:js completionHandler:^(id res, NSError* err) {
        std::string result;
        if (!err && res) {
            result = [NSString stringWithFormat:@"%@", res].UTF8String;
        }
        if (cb) cb(result);
    }];
}

void webview_shutdown() {
    for (auto& [id, h] : s_handles) {
        [h->fps_tmr invalidate];
        [h->wv removeFromSuperview];
        delete h;
    }
    s_handles.clear();
}
