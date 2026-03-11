// webview_core.mm -- Core lifecycle functions:
//   xcm_inject_masks, xcm_check_network, webview_init, webview_load_adblock,
//   webview_create, webview_destroy, webview_show, webview_hide,
//   webview_resize, webview_shutdown.

#include "webview_priv.h"

// ── Script injection helper ───────────────────────────────────────────
// Installs xcm-authored user scripts into a WKUserContentController.
// All scripts are injected at document-start in the MAIN FRAME ONLY to
// keep them out of cross-origin subframes (especially Cloudflare Turnstile
// iframes) that scan for unexpected window properties or non-native
// property descriptors via Object.getOwnPropertyDescriptor.

void xcm_inject_masks(WKUserContentController* ucc) {
    WKUserScript* init = [[WKUserScript alloc]
        initWithSource:JS_INIT
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:YES];
    // Reporting API polyfill -- main frame only to avoid duplicate POSTs
    // from cross-origin child frames.
    WKUserScript* relay = [[WKUserScript alloc]
        initWithSource:JS_REPORT_TO_RELAY
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:YES];
    [ucc addUserScript:init];
    [ucc addUserScript:relay];

    // App-supplied extra scripts (e.g. input-watcher.js, chrome-gl-compositor.js).
    // Main frame only -- do not leak app globals into payment/auth iframes.
    for (const auto& src : s_cbs.extra_scripts) {
        if (src.empty()) continue;
        NSString* nsSrc = [NSString stringWithUTF8String:src.c_str()];
        WKUserScript* extra = [[WKUserScript alloc]
            initWithSource:nsSrc
            injectionTime:WKUserScriptInjectionTimeAtDocumentStart
            forMainFrameOnly:YES];
        [ucc addUserScript:extra];
    }

    // All-frames scripts: run in every subframe (forMainFrameOnly:NO).
    // Used for shims that need to intercept beacon calls from inside third-party
    // iframes (e.g. js.stripe.com).
    for (const auto& src : s_cbs.extra_scripts_all_frames) {
        if (src.empty()) continue;
        NSString* nsSrc = [NSString stringWithUTF8String:src.c_str()];
        WKUserScript* extra = [[WKUserScript alloc]
            initWithSource:nsSrc
            injectionTime:WKUserScriptInjectionTimeAtDocumentStart
            forMainFrameOnly:NO];
        [ucc addUserScript:extra];
    }
}

// ── Network diagnostics ───────────────────────────────────────────────
// Called once at startup.  Prints the system proxy configuration and warns
// if iCloud Private Relay is likely active (it randomises the source IP,
// which breaks IP-locked session tokens on sites like LinkedIn).

static void xcm_check_network(void) {
    NSDictionary* ps = (__bridge_transfer NSDictionary*)CFNetworkCopySystemProxySettings();
    if (!ps || ps.count == 0) {
        fprintf(stderr, "[net] no system proxy settings found\n");
        return;
    }
    fprintf(stderr, "[net] proxy settings:\n");
    for (NSString* k in ps) {
        fprintf(stderr, "  %s = %s\n",
                k.UTF8String, [ps[k] description].UTF8String);
    }
    NSString* pacUrl  = [ps[@"ProxyAutoConfigURLString"] description] ?: @"";
    NSString* httpsH  = [ps[@"HTTPSProxy"] description] ?: @"";
    NSString* socksH  = [ps[@"SOCKSProxy"] description]  ?: @"";
    BOOL relayHint = [pacUrl  containsString:@"apple"] ||
                     [pacUrl  containsString:@"relay"]  ||
                     [httpsH  containsString:@"relay"]  ||
                     [socksH  containsString:@"relay"];
    if (relayHint) {
        fprintf(stderr,
            "[net] WARNING: iCloud Private Relay appears active.\n"
            "[net]   Private Relay randomises the source IP on each connection.\n"
            "[net]   LinkedIn session tokens are IP-locked; the login will fail\n"
            "[net]   consistently until Private Relay is disabled for this\n"
            "[net]   network in System Settings > VPN & Network > iCloud Private Relay.\n");
    } else {
        fprintf(stderr, "[net] Private Relay not detected.\n");
    }

    NSDictionary* scProxies = (__bridge_transfer NSDictionary*)SCDynamicStoreCopyProxies(NULL);
    if (scProxies) {
        NSNumber* httpsEnabled = scProxies[(__bridge NSString*)kSCPropNetProxiesHTTPSEnable];
        if (httpsEnabled.intValue) {
            NSString* host = scProxies[(__bridge NSString*)kSCPropNetProxiesHTTPSProxy] ?: @"?";
            NSNumber* port = scProxies[(__bridge NSString*)kSCPropNetProxiesHTTPSPort]  ?: @0;
            fprintf(stderr, "[net] HTTPS proxy active: %s:%d\n",
                    host.UTF8String, port.intValue);
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────

void webview_init(void* ns_window, AppState* state, WebViewCallbacks cbs) {
    s_window       = (__bridge NSWindow*)ns_window;
    s_state        = state;
    s_cbs          = std::move(cbs);
    s_process_pool = [[WKProcessPool alloc] init];
    xcm_check_network();
}

void webview_load_adblock(const std::string& rules_json) {
    if (rules_json.empty()) return;
    NSString* json = [NSString stringWithUTF8String:rules_json.c_str()];
    // Remove stale cached version first, then compile fresh.
    [[WKContentRuleListStore defaultStore]
        removeContentRuleListForIdentifier:@"xcm-adblock"
        completionHandler:^(NSError* _) {
            [[WKContentRuleListStore defaultStore]
                compileContentRuleListForIdentifier:@"xcm-adblock"
                encodedContentRuleList:json
                completionHandler:^(WKContentRuleList* list, NSError* err) {
                    if (err || !list) {
                        fprintf(stderr, "[adblock] compile error: %s\n",
                                err ? err.localizedDescription.UTF8String : "nil list");
                        return;
                    }
                    s_ad_rule_list = list;
                    fprintf(stderr, "[adblock] compiled OK\n");
                    // Apply retroactively to tabs created before compilation finished.
                    dispatch_async(dispatch_get_main_queue(), ^{
                        for (auto& kv : s_handles) {
                            if (kv.second && kv.second->wv) {
                                [kv.second->wv.configuration.userContentController
                                    addContentRuleList:s_ad_rule_list];
                            }
                        }
                    });
                }];
        }];
}

void* webview_create(int tab_id, const std::string& url) {
    NSCAssert(s_window != nil, @"webview_init() must be called first");

    WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
    // Persistent data store -- keeps cookies, localStorage, IndexedDB, and
    // service worker registrations across launches.
    cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore;
    // Shared process pool -- all tabs and popups share one WebContent process
    // so in-memory cookies written by an OAuth popup are visible to main tabs.
    cfg.processPool = s_process_pool;
    // Allow all media to play without a user gesture.
    cfg.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    cfg.allowsAirPlayForMediaPlayback = YES;

    WKPreferences* prefs = cfg.preferences;
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    @try { [prefs setValue:@YES forKey:@"_serviceWorkerEnabled"]; }
    @catch (NSException*) {
        fprintf(stderr, "[wv] _serviceWorkerEnabled not available on this OS\n");
    }
    if (@available(macOS 12.3, *)) prefs.elementFullscreenEnabled = YES;

    xcm_inject_masks(cfg.userContentController);
    xcm_shell_install(cfg.userContentController, s_window, s_state);
    if (s_ad_rule_list) [cfg.userContentController addContentRuleList:s_ad_rule_list];

    NSRect frame = s_window.contentView.bounds;
    frame.origin.y    = STATUS_HEIGHT_PX;
    frame.size.height -= (TOTAL_CHROME_TOP + STATUS_HEIGHT_PX);

    WKWebView* wv = [[WKWebView alloc] initWithFrame:frame configuration:cfg];
    wv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    if ([wv respondsToSelector:@selector(_setStatusBarEnabled:)])
        [wv performSelector:@selector(_setStatusBarEnabled:) withObject:@NO];
    if ([wv respondsToSelector:@selector(setAllowsBackForwardNavigationGestures:)])
        wv.allowsBackForwardNavigationGestures = YES;

    XCMNavDelegate* nav_del = [[XCMNavDelegate alloc] init];
    nav_del.tabId             = tab_id;
    wv.navigationDelegate     = nav_del;
    wv.UIDelegate             = nav_del;

    XCMKvoObserver* kvo = [[XCMKvoObserver alloc] init];
    kvo.tabId  = tab_id;
    kvo.webView = wv;
    [wv addObserver:kvo forKeyPath:@"title"             options:NSKeyValueObservingOptionNew context:nil];
    [wv addObserver:kvo forKeyPath:@"estimatedProgress" options:NSKeyValueObservingOptionNew context:nil];
    [wv addObserver:kvo forKeyPath:@"URL"               options:NSKeyValueObservingOptionNew context:nil];

    [s_window.contentView addSubview:wv];

    // Metal / compositor performance settings.
    wv.layer.drawsAsynchronously = YES;
    if ([wv respondsToSelector:@selector(_setAllowsAcceleratedInteractionScrolling:)])
        [wv performSelector:@selector(_setAllowsAcceleratedInteractionScrolling:)
                 withObject:@YES];

    // FPS probe timer: every 2s read stats-inject metrics from page JS.
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

    WVHandle* h    = new WVHandle{};
    h->tab_id      = tab_id;
    h->wv          = wv;
    h->nav_del     = nav_del;
    h->kvo         = kvo;
    h->fps_tmr     = fps_tmr;
    s_handles[tab_id]    = h;
    s_js_enabled[tab_id] = true;

    if (!url.empty()) webview_load_url(h, url);

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
    s_js_enabled.erase(h->tab_id);
    delete h;
}

void webview_show(void* handle) {
    if (!handle) return;
    WVHandle* h = (WVHandle*)handle;
    h->wv.hidden = NO;
    xcm_shell_set_webview(h->wv, h->tab_id);
    // Keep the active WKWebView in the responder chain so Cmd shortcuts
    // (copy/cut/paste/select-all) target page inputs consistently.
    if (s_window) {
        [s_window makeFirstResponder:h->wv];
    }
}

void webview_hide(void* handle) {
    if (!handle) return;
    ((WVHandle*)handle)->wv.hidden = YES;
}

void webview_resize(void* handle, int x, int y, int w, int h) {
    if (!handle) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    NSView*   cv  = s_window.contentView;
    CGFloat  cv_h = cv.bounds.size.height;
    CGFloat  ns_y = cv_h - y - h;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [wv setFrame:NSMakeRect(x, ns_y, w, h)];
    [CATransaction commit];
}

void webview_shutdown() {
    for (auto& [id, h] : s_handles) {
        [h->fps_tmr invalidate];
        [h->wv removeFromSuperview];
        delete h;
    }
    s_handles.clear();
}
