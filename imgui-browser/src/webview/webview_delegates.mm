// webview_delegates.mm -- Objective-C delegate implementations.
// Contains: XCMDownloadDelegate, XCMVirtDelegate, XCMPopupDelegate,
//           XCMNavDelegate, XCMKvoObserver.
// See webview_priv.h for shared state declarations.

#include "webview_priv.h"

// ── Download delegate ─────────────────────────────────────────────────
// Handles WKDownload callbacks: asks the user where to save the file,
// then writes it to disk.  One shared instance reused across all tabs.

@implementation XCMDownloadDelegate

- (void)download:(WKDownload*)download
    decideDestinationUsingResponse:(NSURLResponse*)response
    suggestedFilename:(NSString*)filename
    completionHandler:(void (^)(NSURL* _Nullable))completionHandler {
    void (^cb)(NSURL*) = [completionHandler copy];
    __strong WKDownload* dl_ref = download;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.nameFieldStringValue = filename.length ? filename : @"download";
        panel.canCreateDirectories = YES;
        panel.title = @"Save Downloaded File";
        NSURL* dlDir = [[NSFileManager defaultManager]
                            URLForDirectory:NSDownloadsDirectory
                                   inDomain:NSUserDomainMask
                          appropriateForURL:nil
                                     create:NO
                                      error:nil];
        if (dlDir) panel.directoryURL = dlDir;
        NSModalResponse r = [panel runModal];
        if (r == NSModalResponseOK && panel.URL) {
            fprintf(stderr, "[dl] saving to: %s\n", panel.URL.path.UTF8String);
            cb(panel.URL);
        } else {
            fprintf(stderr, "[dl] cancelled\n");
            cb(nil);
            [dl_ref cancel:nil];
        }
    });
}

- (void)downloadDidFinish:(WKDownload*)download {
    fprintf(stderr, "[dl] finished\n");
    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            NSURL* dst = [download valueForKey:@"_destination"];
            if (dst) [[NSWorkspace sharedWorkspace] selectFile:dst.path
                              inFileViewerRootedAtPath:@""];
        } @catch (NSException*) {}
    });
}

- (void)download:(WKDownload*)download
    didFailWithError:(NSError*)error
    resumeData:(NSData*)resumeData {
    fprintf(stderr, "[dl] error: %s\n", error.localizedDescription.UTF8String);
}
@end

// ── Virt secure-window delegate ───────────────────────────────────────
// Minimal delegate for directly-opened secure panels (billing, login).
// The user closes the window manually when done.

@implementation XCMVirtDelegate
- (void)webViewDidClose:(WKWebView*)wv { [self cleanup]; }
- (void)windowWillClose:(NSNotification*)note { [self cleanup]; }
- (void)cleanup {
    if (!self.panel) return;
    [self.panel orderOut:nil];
    self.panel = nil;
    self.secWV = nil;
    native_chrome_eval_toolbar_js("xcmHideSecureBadge&&xcmHideSecureBadge()");
    if (s_virt_delegates) [s_virt_delegates removeObject:self];
}
- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {
    if (action.request.URL) [self.secWV loadRequest:action.request];
    return nil;
}
- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationAction:(WKNavigationAction*)action
    preferences:(WKWebpagePreferences*)preferences
    decisionHandler:(void (^)(WKNavigationActionPolicy, WKWebpagePreferences*))decisionHandler {
    NSString* sc = action.request.URL.scheme.lowercaseString ?: @"";
    if ([sc isEqualToString:@"http"] || [sc isEqualToString:@"https"]
        || [sc hasPrefix:@"about"] || [sc hasPrefix:@"blob"] || [sc hasPrefix:@"data"]) {
        decisionHandler((WKNavigationActionPolicy)3, preferences);
    } else {
        [[NSWorkspace sharedWorkspace] openURL:action.request.URL];
        decisionHandler(WKNavigationActionPolicyCancel, preferences);
    }
}
@end

// ── OAuth popup delegate ──────────────────────────────────────────────
// Manages floating NSPanel popups for window.open() OAuth flows.
// Detects auth completion and reloads the parent tab.

@implementation XCMPopupDelegate

- (void)webViewDidClose:(WKWebView*)wv {
    NSString* currentUrl = wv.URL.absoluteString ?: @"";
    NSString* host       = wv.URL.host.lowercaseString ?: @"";

    BOOL isOAuthProvider = [host hasSuffix:@"google.com"]    ||
                            [host hasSuffix:@"github.com"]    ||
                            [host hasSuffix:@"apple.com"]     ||
                            [host hasSuffix:@"microsoft.com"];
    BOOL isAuthPage = [currentUrl containsString:@"/login"]   ||
                      [currentUrl containsString:@"/signin"]  ||
                      [currentUrl containsString:@"/signup"]  ||
                      [currentUrl containsString:@"/oauth"]   ||
                      [currentUrl containsString:@"/auth/"]   ||
                      [currentUrl containsString:@"authwall"];

    if (!isOAuthProvider && !isAuthPage && currentUrl.length > 0) {
        int parentId = self.parentTabId;
        fprintf(stderr, "[popup] closed on app page %s, reloading main tab %d\n",
                currentUrl.UTF8String, parentId);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (s_state) s_state->push_nav(parentId, "__reload__");
        });
    }
    [self.panel orderOut:nil];
    self.panel   = nil;
    self.popupWV = nil;
    [s_popup_delegates removeObject:self];
}

- (void)webView:(WKWebView*)wv didStartProvisionalNavigation:(WKNavigation*)nav {
    fprintf(stderr, "[popup] start: %s\n", wv.URL.absoluteString.UTF8String ?: "(nil)");
}

- (void)webView:(WKWebView*)wv didFinishNavigation:(WKNavigation*)nav {
    NSString* urlStr = wv.URL.absoluteString ?: @"";
    fprintf(stderr, "[popup] finish: %s\n", urlStr.UTF8String);
    if (self.panel) [self.panel center];

    NSString* host = wv.URL.host.lowercaseString ?: @"";
    BOOL isOAuthProvider = [host hasSuffix:@"google.com"]    ||
                            [host hasSuffix:@"github.com"]    ||
                            [host hasSuffix:@"apple.com"]     ||
                            [host hasSuffix:@"microsoft.com"] ||
                            [host hasSuffix:@"facebook.com"];
    BOOL isAuthPage = [urlStr containsString:@"/login"]     ||
                      [urlStr containsString:@"/signin"]    ||
                      [urlStr containsString:@"/signup"]    ||
                      [urlStr containsString:@"/uas/login"] ||
                      [urlStr containsString:@"/oauth"]     ||
                      [urlStr containsString:@"/auth/"]     ||
                      [urlStr containsString:@"authwall"];

    if (!isOAuthProvider && !isAuthPage && urlStr.length > 0) {
        fprintf(stderr, "[popup] oauth done on %s, navigating main tab %d\n",
                urlStr.UTF8String, self.parentTabId);
        NSString* destUrl = urlStr;
        int parentId = self.parentTabId;
        dispatch_async(dispatch_get_main_queue(), ^{
            if (s_state) s_state->push_nav(parentId, destUrl.UTF8String);
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.6 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            [self.panel orderOut:nil];
            self.panel   = nil;
            self.popupWV = nil;
            [s_popup_delegates removeObject:self];
        });
    }
}

- (void)webView:(WKWebView*)wv didFailNavigation:(WKNavigation*)nav withError:(NSError*)err {
    fprintf(stderr, "[popup] fail: %s\n", err.localizedDescription.UTF8String);
}
- (void)webView:(WKWebView*)wv didFailProvisionalNavigation:(WKNavigation*)nav withError:(NSError*)err {
    fprintf(stderr, "[popup] provisional fail: %s\n", err.localizedDescription.UTF8String);
}

- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationAction:(WKNavigationAction*)action
    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    NSURL* url = action.request.URL;
    NSString* scheme = url.scheme.lowercaseString ?: @"";
    if (scheme.length == 0
        || [scheme isEqualToString:@"http"]  || [scheme isEqualToString:@"https"]
        || [scheme isEqualToString:@"about"] || [scheme isEqualToString:@"data"]
        || [scheme isEqualToString:@"blob"]  || [scheme isEqualToString:@"javascript"]
        || [scheme isEqualToString:@"file"]) {
        fprintf(stderr, "[popup] allow: %s\n", url.absoluteString.UTF8String);
        decisionHandler((WKNavigationActionPolicy)3);
    } else {
        fprintf(stderr, "[popup] custom scheme -> OS: %s\n", url.absoluteString.UTF8String);
        [[NSWorkspace sharedWorkspace] openURL:url];
        decisionHandler(WKNavigationActionPolicyCancel);
    }
}

- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
    decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler {
    BOOL triggerDownload = !response.canShowMIMEType;
    if (!triggerDownload && [response.response isKindOfClass:[NSHTTPURLResponse class]]) {
        NSString* cd = ((NSHTTPURLResponse*)response.response).allHeaderFields[@"Content-Disposition"];
        if (cd && [cd.lowercaseString hasPrefix:@"attachment"]) triggerDownload = YES;
    }
    if (triggerDownload) {
        fprintf(stderr, "[dl/popup] triggering download for: %s\n",
                response.response.URL.absoluteString.UTF8String ?: "?");
        decisionHandler(WKNavigationResponsePolicyDownload);
        return;
    }
    decisionHandler(WKNavigationResponsePolicyAllow);
}

- (void)webView:(WKWebView*)wv
    navigationResponse:(WKNavigationResponse*)response
    didBecomeDownload:(WKDownload*)download {
    if (!s_dl_delegate) s_dl_delegate = [[XCMDownloadDelegate alloc] init];
    download.delegate = s_dl_delegate;
    fprintf(stderr, "[dl/popup] download started from response\n");
}

- (void)webView:(WKWebView*)wv
    navigationAction:(WKNavigationAction*)action
    didBecomeDownload:(WKDownload*)download {
    if (!s_dl_delegate) s_dl_delegate = [[XCMDownloadDelegate alloc] init];
    download.delegate = s_dl_delegate;
    fprintf(stderr, "[dl/popup] download started from action\n");
}

- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {
    [wv loadRequest:action.request];
    return nil;
}

- (void)webView:(WKWebView*)wv
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
    initiatedByFrame:(WKFrameInfo*)frame
    type:(WKMediaCaptureType)type
    decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
    decisionHandler(WKPermissionDecisionGrant);
}

- (void)webView:(WKWebView*)wv
    didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
    completionHandler:(void(^)(NSURLSessionAuthChallengeDisposition, NSURLCredential*))completionHandler {
    NSURLProtectionSpace* ps = challenge.protectionSpace;
    if ([ps.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        fprintf(stderr, "[tls/popup] %s:%ld\n", ps.host.UTF8String, (long)ps.port);
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
    } else {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
    }
}
@end

// ── Main navigation delegate ──────────────────────────────────────────
// One instance per tab.  Handles all WKNavigationDelegate + WKUIDelegate
// callbacks for the primary content WKWebView.

@implementation XCMNavDelegate

- (void)webView:(WKWebView*)wv didStartProvisionalNavigation:(WKNavigation*)nav {
    fprintf(stderr, "[nav] start: %s\n", wv.URL.absoluteString.UTF8String ?: "(nil)");
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, true);
}

- (void)webView:(WKWebView*)wv didFinishNavigation:(WKNavigation*)nav {
    NSString* u = wv.URL.absoluteString ?: @"(nil)";
    fprintf(stderr, "[nav] finish: %s\n", u.UTF8String);
    [wv evaluateJavaScript:@"(document.title||'(no title)')+' | body.children='+document.body.children.length"
         completionHandler:^(id res, NSError* jsErr) {
        if (jsErr) {
            fprintf(stderr, "[nav] page-probe err: %s\n", jsErr.localizedDescription.UTF8String);
        } else {
            NSString* info = [res description] ?: @"";
            fprintf(stderr, "[nav] page-probe: %s\n", info.UTF8String);
        }
    }];
    if (s_cbs.on_loading)   s_cbs.on_loading(self.tabId, false);
    if (s_cbs.on_nav_state) s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
    if (s_cbs.on_favicon_change) {
        int tid = self.tabId;
        [wv evaluateJavaScript:
            @"(function(){"
            "  var ll=document.querySelectorAll(\"link[rel~='icon']\");"
            "  var b='',bs=0;"
            "  for(var i=0;i<ll.length;i++){"
            "    var h=ll[i].href||'';if(!h)continue;"
            "    var m=(ll[i].getAttribute('sizes')||'').match(/(\\d+)/);"
            "    var s=m?parseInt(m[1],10):1;"
            "    if(s>bs||!b){bs=s;b=h;}"
            "  }"
            "  return b||(location.protocol+'//'+location.host+'/favicon.ico');"
            "})()"
            completionHandler:^(id res, NSError* jsErr) {
                if (!jsErr && [res isKindOfClass:[NSString class]]) {
                    std::string fav = ((NSString*)res).UTF8String ?: "";
                    if (s_cbs.on_favicon_change) s_cbs.on_favicon_change(tid, fav);
                }
            }];
    }
    @try {
        if (wv.URL.scheme.length && wv.URL.host.length) {
            std::string origin = std::string(wv.URL.scheme.UTF8String)
                               + "://" + wv.URL.host.UTF8String;
            auto it = s_report_to.find(origin);
            if (it != s_report_to.end()) {
                NSString* epRaw = [NSString stringWithUTF8String:it->second.c_str()];
                NSString* esc = [epRaw stringByReplacingOccurrencesOfString:@"\\"
                                                                 withString:@"\\\\"];
                esc = [esc stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
                NSString* epStr = [NSString stringWithFormat:@"\"%@\"", esc];
                NSString* js = [NSString stringWithFormat:
                    @"typeof window.__xcmSetReportTo==='function'"
                    "&&window.__xcmSetReportTo(%@)", epStr];
                [wv evaluateJavaScript:js completionHandler:nil];
                fprintf(stderr, "[report-to] injected endpoint for %s\n", origin.c_str());
            }
        }
    } @catch (NSException* e) {
        fprintf(stderr, "[nav] report-to inject exception: %s -- %s\n",
                e.name.UTF8String, e.reason.UTF8String);
    }
    self.retryConnect = 0;
}

- (void)webView:(WKWebView*)wv didFailNavigation:(WKNavigation*)nav withError:(NSError*)err {
    fprintf(stderr, "[nav] fail %s -- %s\n",
            wv.URL.absoluteString.UTF8String ?: "(nil)",
            err.localizedDescription.UTF8String);
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, false);
}

- (void)webView:(WKWebView*)wv didFailProvisionalNavigation:(WKNavigation*)nav withError:(NSError*)err {
    fprintf(stderr, "[nav] provisional fail %s -- %s\n",
            wv.URL.absoluteString.UTF8String ?: "(nil)",
            err.localizedDescription.UTF8String);
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, false);

    NSInteger code = err.code;
    BOOL isConnectErr = (code == NSURLErrorCannotConnectToHost ||
                         code == NSURLErrorNetworkConnectionLost ||
                         code == NSURLErrorTimedOut ||
                         code == NSURLErrorCannotFindHost);
    NSString* failing = err.userInfo[NSURLErrorFailingURLStringErrorKey] ?:
                        wv.URL.absoluteString ?: @"";
    BOOL isLocal = [failing containsString:@"127.0.0.1"] ||
                   [failing containsString:@"localhost"];
    if (isConnectErr && isLocal && self.retryConnect < 10) {
        self.retryConnect++;
        fprintf(stderr, "[nav] retrying localhost (attempt %d/10) in 2s...\n",
                self.retryConnect);
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)),
            dispatch_get_main_queue(), ^{
                [wv reload];
            });
    }
}

- (void)webViewWebContentProcessDidTerminate:(WKWebView*)wv {
    fprintf(stderr, "[nav] WebContent process terminated for tab %d, url=%s -- reloading\n",
            self.tabId, wv.URL.absoluteString.UTF8String ?: "(nil)");
    [wv reload];
}

- (void)webView:(WKWebView*)wv
    didReceiveServerRedirectForProvisionalNavigation:(WKNavigation*)nav {
    fprintf(stderr, "[nav] redirect -> %s\n", wv.URL.absoluteString.UTF8String ?: "(nil)");
}

- (void)webView:(WKWebView*)wv didCommitNavigation:(WKNavigation*)nav {
    NSString* u = wv.URL.absoluteString ?: @"";
    if (s_cbs.on_url_change) s_cbs.on_url_change(self.tabId, u.UTF8String);
    if (s_cbs.on_nav_state)  s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
}

// window.open() / target=_blank handler.
- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {

    if (action.navigationType == WKNavigationTypeLinkActivated && action.request.URL) {
        NSString* urlStr = action.request.URL.absoluteString;
        if (urlStr.length > 0) {
            const char* rawUrl = urlStr.UTF8String;
            if (rawUrl && xcm_is_auth_popup_url(rawUrl)) {
                fprintf(stderr, "[popup] auth popup target=_blank: %s\n", rawUrl);
                webview_open_virt_popup(std::string(rawUrl));
                return nil;
            }
        }
        if (s_state && urlStr.length > 0) {
            std::string url = urlStr.UTF8String;
            dispatch_async(dispatch_get_main_queue(), ^{
                if (s_state) s_state->push_nav(-2, url);
            });
        }
        return nil;
    }

    if (!s_popup_delegates) s_popup_delegates = [NSMutableArray array];

    @try { cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore; } @catch (NSException*) {}
    @try { cfg.processPool = s_process_pool; } @catch (NSException*) {}

    xcm_inject_masks(cfg.userContentController);
    xcm_shell_install(cfg.userContentController, s_window, s_state);
    if (s_ad_rule_list) [cfg.userContentController addContentRuleList:s_ad_rule_list];

    NSRect panelRect = NSMakeRect(0, 0, 520, 640);
    NSPanel* panel = [[NSPanel alloc]
        initWithContentRect:panelRect
                  styleMask:NSWindowStyleMaskTitled |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable |
                            NSWindowStyleMaskMiniaturizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    panel.title = @"Sign In";
    panel.level = NSFloatingWindowLevel;

    WKWebView* popup = [[WKWebView alloc] initWithFrame:panelRect configuration:cfg];

    XCMPopupDelegate* popupDel = [[XCMPopupDelegate alloc] init];
    popupDel.panel       = panel;
    popupDel.popupWV     = popup;
    popupDel.parentTabId = self.tabId;
    popup.navigationDelegate = popupDel;
    popup.UIDelegate         = popupDel;

    [s_popup_delegates addObject:popupDel];

    panel.contentView = popup;
    [panel center];
    [panel makeKeyAndOrderFront:nil];

    return popup;
}

- (void)webView:(WKWebView*)wv
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
    initiatedByFrame:(WKFrameInfo*)frame
    type:(WKMediaCaptureType)type
    decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
    decisionHandler(WKPermissionDecisionGrant);
}

- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationAction:(WKNavigationAction*)action
    preferences:(WKWebpagePreferences*)preferences
    decisionHandler:(void (^)(WKNavigationActionPolicy, WKWebpagePreferences*))decisionHandler {
    auto it = s_js_enabled.find(self.tabId);
    bool js = (it != s_js_enabled.end()) ? it->second : true;
    preferences.allowsContentJavaScript = js ? YES : NO;

    if (@available(macOS 11.3, *)) {
        if (action.shouldPerformDownload) {
            fprintf(stderr, "[nav] shouldPerformDownload=YES -> .download\n");
            decisionHandler(WKNavigationActionPolicyDownload, preferences);
            return;
        }
    }

    NSURL* url = action.request.URL;
    const char* rawUrl = url.absoluteString.UTF8String ?: "";
    if (rawUrl[0] && xcm_is_auth_popup_url(rawUrl)) {
        fprintf(stderr, "[nav] auth popup intercept: %s\n", rawUrl);
        webview_open_virt_popup(std::string(rawUrl));
        decisionHandler(WKNavigationActionPolicyCancel, preferences);
        return;
    }
    NSString* scheme = url.scheme.lowercaseString ?: @"";
    if (scheme.length == 0
        || [scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]
        || [scheme isEqualToString:@"about"] || [scheme isEqualToString:@"data"]
        || [scheme isEqualToString:@"blob"]  || [scheme isEqualToString:@"javascript"]
        || [scheme isEqualToString:@"file"]) {
        fprintf(stderr, "[nav] allow: %s\n", url.absoluteString.UTF8String);
        decisionHandler((WKNavigationActionPolicy)3, preferences);
    } else {
        fprintf(stderr, "[nav] custom scheme, handing to OS: %s\n",
                url.absoluteString.UTF8String);
        [[NSWorkspace sharedWorkspace] openURL:url];
        decisionHandler(WKNavigationActionPolicyCancel, preferences);
    }
}

- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
    decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler {
    if ([response.response isKindOfClass:[NSHTTPURLResponse class]]) {
        NSHTTPURLResponse* http = (NSHTTPURLResponse*)response.response;
        NSURL* ru = response.response.URL;
        if (ru.scheme.length && ru.host.length) {
            std::string origin = std::string(ru.scheme.UTF8String)
                               + "://" + ru.host.UTF8String;
            NSString* repEP = http.allHeaderFields[@"Reporting-Endpoints"];
            if (repEP) {
                NSRange q1 = [repEP rangeOfString:@"\""];
                if (q1.location != NSNotFound) {
                    NSRange q2 = [repEP rangeOfString:@"\""
                                             options:0
                                               range:NSMakeRange(q1.location+1,
                                                     repEP.length-q1.location-1)];
                    if (q2.location != NSNotFound && q2.location > q1.location) {
                        NSString* ep = [repEP substringWithRange:
                            NSMakeRange(q1.location+1, q2.location-q1.location-1)];
                        if (ep.length > 4) s_report_to[origin] = ep.UTF8String;
                    }
                }
            }
            if (s_report_to.find(origin) == s_report_to.end()) {
                NSString* rtHdr = http.allHeaderFields[@"Report-To"];
                if (rtHdr) {
                    NSData* d = [rtHdr dataUsingEncoding:NSUTF8StringEncoding];
                    id parsed = [NSJSONSerialization JSONObjectWithData:d
                                                               options:0 error:nil];
                    NSDictionary* obj = nil;
                    if ([parsed isKindOfClass:[NSArray class]])
                        obj = [(NSArray*)parsed firstObject];
                    else if ([parsed isKindOfClass:[NSDictionary class]])
                        obj = (NSDictionary*)parsed;
                    id epRaw = [[obj[@"endpoints"] firstObject] objectForKey:@"url"];
                    NSString* ep = [epRaw isKindOfClass:[NSString class]]
                                   ? (NSString*)epRaw : nil;
                    if (ep.length > 4) s_report_to[origin] = ep.UTF8String;
                }
            }
        }
    }
    BOOL triggerDownload = !response.canShowMIMEType;
    if (!triggerDownload && [response.response isKindOfClass:[NSHTTPURLResponse class]]) {
        NSString* cd = ((NSHTTPURLResponse*)response.response).allHeaderFields[@"Content-Disposition"];
        if (cd && [cd.lowercaseString hasPrefix:@"attachment"]) triggerDownload = YES;
    }
    if (triggerDownload) {
        fprintf(stderr, "[dl] triggering download for: %s\n",
                response.response.URL.absoluteString.UTF8String ?: "?");
        decisionHandler(WKNavigationResponsePolicyDownload);
        return;
    }
    decisionHandler(WKNavigationResponsePolicyAllow);
}

- (void)webView:(WKWebView*)wv
    didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
    completionHandler:(void(^)(NSURLSessionAuthChallengeDisposition, NSURLCredential*))completionHandler {
    NSURLProtectionSpace* ps = challenge.protectionSpace;
    if ([ps.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        SecTrustRef trust = ps.serverTrust;
        if (trust) {
            CFErrorRef tls_err = nil;
            bool ok = SecTrustEvaluateWithError(trust, &tls_err);
            fprintf(stderr, "[tls] %s:%ld eval=%s\n",
                    ps.host.UTF8String, (long)ps.port, ok ? "ok" : "FAIL");
            if (tls_err) CFRelease(tls_err);
        }
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
    } else {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
    }
}

// ── File upload (<input type="file">) ─────────────────────────────────
- (void)webView:(WKWebView*)wv
    runOpenPanelWithParameters:(WKOpenPanelParameters*)params
    initiatedByFrame:(WKFrameInfo*)frame
    completionHandler:(void (^)(NSArray<NSURL*>* _Nullable))completionHandler {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = NO;
    panel.allowsMultipleSelection = params.allowsMultipleSelection;
    [panel beginSheetModalForWindow:s_window
                  completionHandler:^(NSModalResponse r) {
        completionHandler(r == NSModalResponseOK ? panel.URLs : nil);
    }];
}

// ── JavaScript dialogs ────────────────────────────────────────────────
- (void)webView:(WKWebView*)wv
    runJavaScriptAlertPanelWithMessage:(NSString*)message
    initiatedByFrame:(WKFrameInfo*)frame
    completionHandler:(void (^)(void))completionHandler {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert* alert    = [[NSAlert alloc] init];
        alert.messageText = message ?: @"";
        alert.alertStyle  = NSAlertStyleInformational;
        [alert addButtonWithTitle:@"OK"];
        [alert beginSheetModalForWindow:s_window
                      completionHandler:^(NSModalResponse __unused r) {
            completionHandler();
        }];
    });
}

- (void)webView:(WKWebView*)wv
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
    initiatedByFrame:(WKFrameInfo*)frame
    completionHandler:(void (^)(BOOL))completionHandler {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert* alert    = [[NSAlert alloc] init];
        alert.messageText = message ?: @"";
        alert.alertStyle  = NSAlertStyleInformational;
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert beginSheetModalForWindow:s_window
                      completionHandler:^(NSModalResponse r) {
            completionHandler(r == NSAlertFirstButtonReturn);
        }];
    });
}

- (void)webView:(WKWebView*)wv
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
    defaultText:(NSString*)defaultText
    initiatedByFrame:(WKFrameInfo*)frame
    completionHandler:(void (^)(NSString* _Nullable))completionHandler {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert* alert    = [[NSAlert alloc] init];
        alert.messageText = prompt ?: @"";
        alert.alertStyle  = NSAlertStyleInformational;
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        NSTextField* tf = [[NSTextField alloc] initWithFrame:NSMakeRect(0,0,260,22)];
        tf.stringValue  = defaultText ?: @"";
        alert.accessoryView = tf;
        [alert beginSheetModalForWindow:s_window
                      completionHandler:^(NSModalResponse r) {
            completionHandler(r == NSAlertFirstButtonReturn ? tf.stringValue : nil);
        }];
    });
}

// ── Download handoff ──────────────────────────────────────────────────
- (void)webView:(WKWebView*)wv
    navigationResponse:(WKNavigationResponse*)response
    didBecomeDownload:(WKDownload*)download {
    if (!s_dl_delegate) s_dl_delegate = [[XCMDownloadDelegate alloc] init];
    download.delegate = s_dl_delegate;
    fprintf(stderr, "[dl] download started from response\n");
}

- (void)webView:(WKWebView*)wv
    navigationAction:(WKNavigationAction*)action
    didBecomeDownload:(WKDownload*)download {
    if (!s_dl_delegate) s_dl_delegate = [[XCMDownloadDelegate alloc] init];
    download.delegate = s_dl_delegate;
    fprintf(stderr, "[dl] download started from action\n");
}

@end

// ── KVO observer ──────────────────────────────────────────────────────

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
