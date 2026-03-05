// webview.mm -- WKWebView bridge (Objective-C++)
// Embeds native WKWebView instances inside the GLFW/OpenGL NSWindow.
// ImGui draws the chrome; WKWebView fills the content area below it.
// ARC enabled via -fobjc-arc compiler flag (see CMakeLists.txt).

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <Network/Network.h>
#import <CFNetwork/CFNetwork.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <Security/SecTrust.h>
#include "webview.h"
#include "app_state.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

// ── Internal state ────────────────────────────────────────────────────

static NSWindow*         s_window      = nil;
static AppState*         s_state       = nullptr;
static WebViewCallbacks  s_cbs;
// Shared process pool ensures all WKWebViews (tabs + OAuth popups) share
// the same in-memory cookie/token cache. Without this each WKWebView spawns
// a separate WebContent process and the popup's session is invisible to the
// main window, causing auth loops even when the persistent data store is shared.
static WKProcessPool*    s_process_pool = nil;
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

// JS_SCROLL_KILL removed -- it broke SPA routers on LinkedIn and similar
// single-page apps that rely on scrollTo with behaviour options.

// Mask WKWebView-specific globals that Google and LinkedIn use to detect
// and block embedded webview sign-in flows. LinkedIn runs a stricter
// fingerprint check than Google so we must cover all Chrome-specific APIs
// that are absent from WKWebView: vendor, plugins, mimeTypes, loadTimes,
// csi, app, and the full userAgentData shape.
static NSString* const JS_MASK_WEBVIEW = @"(function(){"

    // Remove window.webkit -- Google and LinkedIn detect WKWebView via this.
    "try{"
    "  Object.defineProperty(window,'webkit',{"
    "    get:function(){return undefined;},"
    "    configurable:true"
    "  });"
    "}catch(e){}"

    // navigator.webdriver must be false (LinkedIn treats true as a bot).
    "try{"
    "  Object.defineProperty(navigator,'webdriver',{"
    "    get:function(){return false;},"
    "    configurable:true"
    "  });"
    "}catch(e){}"

    // navigator.vendor -- Firefox returns "" (empty string).
    // WKWebView returns "Apple Computer, Inc." which is a strong signal.
    "try{"
    "  Object.defineProperty(navigator,'vendor',{"
    "    get:function(){return '';},"
    "    configurable:true"
    "  });"
    "}catch(e){}"

    // navigator.userAgentData -- Firefox 134 does not expose this API to web
    // pages by default; leave it undefined so the fingerprint matches Firefox.
    // Do not inject a Chrome-shaped object -- that would be inconsistent with
    // the Firefox UA and could trigger detection.
    "try{"
    "  if(navigator.userAgentData){"
    "    Object.defineProperty(navigator,'userAgentData',{"
    "      get:function(){return undefined;},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // navigator.plugins / navigator.mimeTypes -- Firefox 89+ returns empty
    // arrays for both; WKWebView also returns empty arrays natively, so the
    // fingerprint already matches. No injection needed.

    // window.chrome -- explicitly absent in Firefox. If WKWebView somehow
    // exposes it, clear it to match the Firefox fingerprint.
    "try{"
    "  if(window.chrome){"
    "    Object.defineProperty(window,'chrome',{"
    "      get:function(){return undefined;},"
    "      configurable:true,writable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // navigator.languages -- WKWebView may expose an empty array.
    "try{"
    "  if(!navigator.languages||navigator.languages.length===0){"
    "    Object.defineProperty(navigator,'languages',{"
    "      get:function(){return['en-US','en'];},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // Permissions API -- LinkedIn calls permissions.query({name:'notifications'})
    // and checks for a working Promise response. WKWebView returns undefined.
    "try{"
    "  if(!navigator.permissions){"
    "    Object.defineProperty(navigator,'permissions',{"
    "      value:{"
    "        query:function(desc){"
    "          return Promise.resolve({"
    "            name:desc.name,state:'prompt',"
    "            onchange:null,addEventListener:function(){},removeEventListener:function(){}"
    "          });"
    "        }"
    "      },"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // window.trustedTypes (Trusted Types API) -- Chrome 83+ exposes this.
    // Google's sign-in front-end (gapi.js / accounts.google.com) checks
    // typeof trustedTypes !== 'undefined' and attempts to create a policy
    // with it. If the object is absent the page detects a non-Chrome env
    // and falls through to the 'unsupported browser' error path.
    "try{"
    "  if(!window.trustedTypes){"
    "    var _ttPolicies={};"
    "    Object.defineProperty(window,'trustedTypes',{"
    "      value:{"
    "        createPolicy:function(name,rules){"
    "          var p={"
    "            createHTML:rules&&rules.createHTML?function(s,x,y){return rules.createHTML(s,x,y);}:function(s){return s;},"
    "            createScript:rules&&rules.createScript?function(s,x,y){return rules.createScript(s,x,y);}:function(s){return s;},"
    "            createScriptURL:rules&&rules.createScriptURL?function(s,x,y){return rules.createScriptURL(s,x,y);}:function(s){return s;}"
    "          };"
    "          _ttPolicies[name]=p;"
    "          return p;"
    "        },"
    "        isHTML:function(v){return false;},"
    "        isScript:function(v){return false;},"
    "        isScriptURL:function(v){return false;},"
    "        getPolicyNames:function(){return Object.keys(_ttPolicies);},"
    "        defaultPolicy:null,"
    "        emptyHTML:'',"
    "        emptyScript:''"
    "      },"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // performance.memory -- Chrome exposes this; WKWebView does not.
    // LinkedIn's module bundler checks for it to decide between the full
    // and 'lite' page templates. Without it certain route-based bundles
    // (including the global nav) are never fetched.
    "try{"
    "  if(typeof performance!=='undefined'&&!performance.memory){"
    "    Object.defineProperty(performance,'memory',{"
    "      get:function(){"
    "        return{"
    "          jsHeapSizeLimit:4294705152,"
    "          totalJSHeapSize:60000000,"
    "          usedJSHeapSize:30000000"
    "        };"
    "      },"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // navigator.connection (Network Information API) -- present in Chrome,
    // absent in WKWebView. LinkedIn uses it for network-aware lazy loading;
    // if undefined the voyager SPA may skip loading the global-nav bundle.
    "try{"
    "  if(!navigator.connection){"
    "    Object.defineProperty(navigator,'connection',{"
    "      get:function(){"
    "        return{"
    "          effectiveType:'4g',"
    "          downlink:10,"
    "          rtt:50,"
    "          saveData:false,"
    "          onchange:null,"
    "          addEventListener:function(){},"
    "          removeEventListener:function(){}"
    "        };"
    "      },"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // window.outerWidth / window.outerHeight -- WKWebView returns 0 for both.
    // LinkedIn's global-nav component reads outerWidth to determine whether
    // to render the desktop nav bar. A value of 0 causes it to fall through
    // to the mobile/lite path and skip rendering the nav entirely.
    "try{"
    "  if(!window.outerWidth||window.outerWidth===0){"
    "    Object.defineProperty(window,'outerWidth',{"
    "      get:function(){return window.innerWidth||1280;},"
    "      configurable:true"
    "    });"
    "  }"
    "  if(!window.outerHeight||window.outerHeight===0){"
    "    Object.defineProperty(window,'outerHeight',{"
    "      get:function(){return (window.innerHeight||800)+100;},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    "})();";

// JS_FINGERPRINT_NOISE: add human-like hardware variation so automated
// fingerprinting scripts cannot identify this as a headless / bot client.
// Runs at document-start before any page JS executes.
//
// Covered vectors:
//  1. Canvas 2D getImageData -- LSB noise on a few pixels per read
//  2. WebGL getParameter -- mask UNMASKED_VENDOR/RENDERER to neutral Intel strings
//  3. AudioBuffer.getChannelData -- sub-10^-7 amplitude noise on two samples
//  4. navigator.hardwareConcurrency -- stable 8 (common desktop value)
//  5. navigator.platform -- "MacIntel" (correct for Firefox/macOS)
//
// The noise uses a session-unique seed derived at injection time so the
// same page gets consistent values within its lifetime but different values
// across tabs and page loads.
static NSString* const JS_FINGERPRINT_NOISE = @"(function(){"

    // Session-stable Xorshift32 PRNG seeded at injection time.
    "var _s=(Date.now()^(Math.random()*0x7FFFFFFF))|1;"
    "function _r(){"
    "  _s=(_s^(_s<<13)|0)>>>0;"
    "  _s=(_s^(_s>>>17))>>>0;"
    "  _s=(_s^(_s<<5)|0)>>>0;"
    "  return _s;"
    "}"
    "function _r2(){return _r()&1?1:-1;}"

    // 1. Canvas 2D -- getImageData LSB noise
    "try{"
    "  var _ogc=HTMLCanvasElement.prototype.getContext;"
    "  HTMLCanvasElement.prototype.getContext=function(t,o){"
    "    var ctx=_ogc.call(this,t,o);"
    "    if(ctx&&t==='2d'&&ctx.getImageData){"
    "      var _og=ctx.getImageData.bind(ctx);"
    "      ctx.getImageData=function(x,y,w,h){"
    "        var d=_og(x,y,w,h);"
    "        if(d&&d.data&&d.data.length>16){"
    "          for(var i=0;i<3;i++){"
    "            var idx=(_r()%(d.data.length>>>2))<<2;"
    "            d.data[idx]=(d.data[idx]+_r2()+256)&255;"
    "          }"
    "        }"
    "        return d;"
    "      };"
    "    }"
    "    return ctx;"
    "  };"
    "}catch(e){}"

    // 2. WebGL / WebGL2 -- mask hardware identity strings
    "var _WGLV=0x9245,_WGLR=0x9246;"
    "function _patchWGL(P){"
    "  try{"
    "    var og=P.getParameter.bind(P.__proto__||P);"
    "    P.constructor.prototype.getParameter=function(param){"
    "      if(param===_WGLV)return'Intel Inc.';"
    "      if(param===_WGLR)return'Intel Iris OpenGL Engine';"
    "      return og.call(this,param);"
    "    };"
    "  }catch(e){}"
    "}"
    "try{"
    "  var _c2=document.createElement('canvas');"
    "  var _g=_c2.getContext('webgl')||_c2.getContext('experimental-webgl');"
    "  if(_g)_patchWGL(_g);"
    "  var _g2=_c2.getContext('webgl2');"
    "  if(_g2)_patchWGL(_g2);"
    "}catch(e){}"

    // 3. AudioBuffer.getChannelData -- tiny sub-perception amplitude noise
    "try{"
    "  var _oab=AudioBuffer.prototype.getChannelData;"
    "  AudioBuffer.prototype.getChannelData=function(ch){"
    "    var d=_oab.call(this,ch);"
    "    if(d&&d.length>4){"
    "      var i1=_r()%d.length,i2=_r()%d.length;"
    "      d[i1]+=_r2()*5e-8;"
    "      d[i2]+=_r2()*5e-8;"
    "    }"
    "    return d;"
    "  };"
    "}catch(e){}"

    // 4. navigator.hardwareConcurrency -- common desktop value, stable
    "try{"
    "  if(!navigator.hardwareConcurrency||navigator.hardwareConcurrency<2){"
    "    Object.defineProperty(navigator,'hardwareConcurrency',{"
    "      get:function(){return 8;},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    // 5. navigator.platform -- ensure correct macOS string
    "try{"
    "  if(navigator.platform!=='MacIntel'){"
    "    Object.defineProperty(navigator,'platform',{"
    "      get:function(){return'MacIntel';},"
    "      configurable:true"
    "    });"
    "  }"
    "}catch(e){}"

    "})();";

// Firefox 134 on macOS -- current as of March 2026.
// Firefox UA bypasses Google's embedded-WebView sign-in block more reliably
// than a Chrome UA because Google's detection targets Chrome-in-WebView
// specifically. The macOS version token is always "10.15" in Firefox
// regardless of the actual OS version.
static NSString* const kUserAgent =
    @"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:134.0) "
     "Gecko/20100101 Firefox/134.0";

// ── Popup delegate (OAuth / window.open windows) ─────────────────────
// When a page calls window.open() (e.g. "Sign in with Google") WKWebKit
// calls createWebViewWithConfiguration:. We return a real WKWebView so
// the opener relationship is maintained, and we show it in a floating
// NSPanel. When the popup calls window.close() or finishes auth,
// WKUIDelegate fires webViewDidClose: and we tear down the panel.

// Per-tab JavaScript enabled state. Keyed by tab_id. Defined here (before
// the Obj-C delegates) so XCMNavDelegate can read it in its navigation
// policy method without depending on the WVHandle struct.
// webview_set_js_enabled keeps this map in sync with WVHandle::js_enabled.
static std::unordered_map<int, bool> s_js_enabled;  // tab_id -> js enabled

// Forward declaration
@class XCMPopupDelegate;

// Static set that keeps popup delegates alive until the panel closes.
static NSMutableArray<XCMPopupDelegate*>* s_popup_delegates = nil;

@interface XCMPopupDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@property (nonatomic, strong) NSPanel*    panel;
@property (nonatomic, strong) WKWebView*  popupWV;
@property (nonatomic, assign) int         parentTabId;  // tab to refresh when OAuth completes
@end

@implementation XCMPopupDelegate

- (void)webViewDidClose:(WKWebView*)wv {
    // If the popup was on a linkedin.com page when it closed (user clicked X,
    // or the page called window.close()), navigate the main tab to the same
    // URL so it inherits whatever session state was just established.
    NSString* currentUrl = wv.URL.absoluteString ?: @"";
    NSString* host       = wv.URL.host.lowercaseString ?: @"";
    BOOL isLinkedIn = [host hasSuffix:@"linkedin.com"];
    BOOL isAuthPage = [currentUrl containsString:@"/login"] ||
                      [currentUrl containsString:@"/signup"] ||
                      [currentUrl containsString:@"authwall"];
    if (isLinkedIn && !isAuthPage && currentUrl.length > 0) {
        int parentId = self.parentTabId;
        if (s_state) s_state->push_nav(parentId, currentUrl.UTF8String);
        fprintf(stderr, "[popup] closed on %s, navigating main tab %d\n",
                currentUrl.UTF8String, parentId);
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

    // When the popup lands on any linkedin.com page that is not the sign-in
    // or signup page, consider auth done: navigate the main tab to that same
    // URL so it inherits the session cookies the popup just wrote.
    NSString* host = wv.URL.host.lowercaseString ?: @"";
    BOOL isLinkedIn = [host hasSuffix:@"linkedin.com"];
    BOOL isAuthPage = [urlStr containsString:@"/login"] ||
                      [urlStr containsString:@"/signup"] ||
                      [urlStr containsString:@"/uas/login"] ||
                      [urlStr containsString:@"authwall"];
    if (isLinkedIn && !isAuthPage && urlStr.length > 0) {
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

// Same scheme policy as the main delegate -- but always use
// AllowWithoutTryingAppLink so the popup's Google auth redirects are not
// hijacked by any registered universal link handler.
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
        // WKNavigationActionPolicyAllowWithoutTryingAppLink = 3
        decisionHandler((WKNavigationActionPolicy)3);
    } else {
        fprintf(stderr, "[popup] custom scheme -> OS: %s\n",
                url.absoluteString.UTF8String);
        [[NSWorkspace sharedWorkspace] openURL:url];
        decisionHandler(WKNavigationActionPolicyCancel);
    }
}

- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
    decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler {
    decisionHandler(WKNavigationResponsePolicyAllow);
}

// Allow nested popups (e.g. Google account chooser opening another page).
- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {
    [wv loadRequest:action.request];
    return nil;
}

// Grant media in popups too (Meet inside a Google auth flow, etc.).
- (void)webView:(WKWebView*)wv
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
    initiatedByFrame:(WKFrameInfo*)frame
    type:(WKMediaCaptureType)type
    decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
    decisionHandler(WKPermissionDecisionGrant);
}

// TLS validation (same policy as main delegate).
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

// Forward declaration -- defined after WVHandle in the Public API section.
static void xcm_inject_masks(WKUserContentController* ucc);

// ── WKWebView delegate (Obj-C class) ─────────────────────────────────

@interface XCMNavDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@property (nonatomic, assign) int tabId;
@end

@implementation XCMNavDelegate

- (void)webView:(WKWebView*)wv didStartProvisionalNavigation:(WKNavigation*)nav {
    fprintf(stderr, "[nav] start: %s\n", wv.URL.absoluteString.UTF8String ?: "(nil)");
    if (s_cbs.on_loading) s_cbs.on_loading(self.tabId, true);
}
- (void)webView:(WKWebView*)wv didFinishNavigation:(WKNavigation*)nav {
    NSString* u = wv.URL.absoluteString ?: @"(nil)";
    fprintf(stderr, "[nav] finish: %s\n", u.UTF8String);
    // Probe whether the page has real content or is blank.
    [wv evaluateJavaScript:@"(document.title||'(no title)')+' | body.children='+document.body.children.length"
         completionHandler:^(id res, NSError* jsErr) {
        if (jsErr) fprintf(stderr, "[nav] page-probe err: %s\n", jsErr.localizedDescription.UTF8String);
        else       fprintf(stderr, "[nav] page-probe: %s\n", [res description].UTF8String ?: "(nil)");
    }];
    if (s_cbs.on_loading)   s_cbs.on_loading(self.tabId, false);
    if (s_cbs.on_nav_state) s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
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
}
- (void)webView:(WKWebView*)wv
    didReceiveServerRedirectForProvisionalNavigation:(WKNavigation*)nav {
    fprintf(stderr, "[nav] redirect -> %s\n", wv.URL.absoluteString.UTF8String ?: "(nil)");
}

// Called on every committed navigation (including fragment changes).
- (void)webView:(WKWebView*)wv
    didCommitNavigation:(WKNavigation*)nav {
    NSString* u = wv.URL.absoluteString ?: @"";
    if (s_cbs.on_url_change) s_cbs.on_url_change(self.tabId, u.UTF8String);
    if (s_cbs.on_nav_state)  s_cbs.on_nav_state(self.tabId, wv.canGoBack, wv.canGoForward);
}

// window.open() / target=_blank -- create a real popup NSPanel so that
// the WKWebKit opener relationship is preserved. This is required for
// OAuth flows (Google, GitHub, etc.) that use postMessage or navigation
// callbacks to communicate the auth result to the parent window.
- (WKWebView*)webView:(WKWebView*)wv
    createWebViewWithConfiguration:(WKWebViewConfiguration*)cfg
    forNavigationAction:(WKNavigationAction*)action
    windowFeatures:(WKWindowFeatures*)features {

    if (!s_popup_delegates) s_popup_delegates = [NSMutableArray array];

    // Force the popup to use the same persistent data store as the main
    // window. The WKWebViewConfiguration WebKit passes here may point to
    // a non-persistent (ephemeral) store, which means any cookies the
    // popup writes (e.g. the LinkedIn session token after Google OAuth)
    // are invisible to the main WKWebView and the login never completes.
    @try { cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore; }
    @catch (NSException*) {}
    // Same shared process pool so in-memory cookie writes are immediately
    // visible to all other WKWebViews in this session.
    @try { cfg.processPool = s_process_pool; }
    @catch (NSException*) {}

    // Inject Chrome-masking scripts into the popup's userContentController.
    // WebKit provides a fresh (empty) controller here -- our scripts from the
    // parent WKWebView are NOT inherited. Without this, accounts.google.com
    // detects WKWebView in the popup and shows "This browser or app may not
    // be secure."
    xcm_inject_masks(cfg.userContentController);

    // Apply the same Chrome UA and media settings to the popup WKWebView.
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

    WKWebView* popup = [[WKWebView alloc] initWithFrame:panelRect
                                          configuration:cfg];
    popup.customUserAgent = kUserAgent;

    XCMPopupDelegate* popupDel = [[XCMPopupDelegate alloc] init];
    popupDel.panel        = panel;
    popupDel.popupWV      = popup;
    popupDel.parentTabId  = self.tabId;
    popup.navigationDelegate = popupDel;
    popup.UIDelegate         = popupDel;

    // Keep the delegate alive until the panel closes.
    [s_popup_delegates addObject:popupDel];

    panel.contentView = popup;
    [panel center];
    [panel makeKeyAndOrderFront:nil];

    return popup;
}

// Camera / microphone permission (macOS 12+).
// Grant all media capture requests so video calls (LinkedIn, Meet, etc.) work.
- (void)webView:(WKWebView*)wv
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
    initiatedByFrame:(WKFrameInfo*)frame
    type:(WKMediaCaptureType)type
    decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
    decisionHandler(WKPermissionDecisionGrant);
}

// Use the WKWebpagePreferences-aware navigation policy delegate so we can
// apply per-tab allowsContentJavaScript on every navigation. This method
// supersedes decidePolicyForNavigationAction:decisionHandler: when both
// exist; WebKit calls this one on macOS 10.15+.
- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationAction:(WKNavigationAction*)action
    preferences:(WKWebpagePreferences*)preferences
    decisionHandler:(void (^)(WKNavigationActionPolicy, WKWebpagePreferences*))decisionHandler {
    // Apply per-tab JavaScript enabled state. Default YES so tabs that
    // have not been toggled behave like a normal browser.
    auto it = s_js_enabled.find(self.tabId);
    bool js = (it != s_js_enabled.end()) ? it->second : true;
    preferences.allowsContentJavaScript = js ? YES : NO;

    NSURL* url = action.request.URL;
    NSString* scheme = url.scheme.lowercaseString ?: @"";
    if (scheme.length == 0
        || [scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]
        || [scheme isEqualToString:@"about"] || [scheme isEqualToString:@"data"]
        || [scheme isEqualToString:@"blob"]  || [scheme isEqualToString:@"javascript"]
        || [scheme isEqualToString:@"file"]) {
        fprintf(stderr, "[nav] allow: %s\n", url.absoluteString.UTF8String);
        // WKNavigationActionPolicyAllowWithoutTryingAppLink = 3 (macOS 11.3+)
        decisionHandler((WKNavigationActionPolicy)3, preferences);
    } else {
        fprintf(stderr, "[nav] custom scheme, handing to OS: %s\n",
                url.absoluteString.UTF8String);
        [[NSWorkspace sharedWorkspace] openURL:url];
        decisionHandler(WKNavigationActionPolicyCancel, preferences);
    }
}

// Allow all response types (including application/pdf, downloads, etc.).
// Without this WKWebView silently cancels responses with unrecognised MIME types.
- (void)webView:(WKWebView*)wv
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
    decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler {
    decisionHandler(WKNavigationResponsePolicyAllow);
}

// TLS validation.
// Use the OS default evaluator which enforces:
//   - Valid certificate chain anchored to a trusted root
//   - Certificate not expired
//   - Hostname match
//   - TLS 1.2 minimum (macOS 12+ system policy)
//   - TLS 1.3 negotiated automatically when server supports it
// We log the host/port so TLS errors surface in the console.
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
    int              tab_id    = 0;
    WKWebView*       wv        = nil;
    NSView*          host      = nil;  // transparent NSView parent
    XCMNavDelegate*  nav_del   = nil;
    XCMKvoObserver*  kvo       = nil;
    NSTimer*         fps_tmr   = nil;
    bool             js_enabled = true;  // per-tab JavaScript toggle
};

static std::unordered_map<int, WVHandle*> s_handles;  // tab_id -> handle

// ── Mask injection helper ─────────────────────────────────────────────
// Adds JS_INIT and JS_MASK_WEBVIEW to any WKUserContentController.
// Called for the main WKWebView on creation AND for every popup window
// because WebKit supplies a fresh (empty) userContentController in
// createWebViewWithConfiguration:, so the scripts do not propagate
// automatically from the parent page.
static void xcm_inject_masks(WKUserContentController* ucc) {
    WKUserScript* init = [[WKUserScript alloc]
        initWithSource:JS_INIT
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:NO];
    WKUserScript* mask = [[WKUserScript alloc]
        initWithSource:JS_MASK_WEBVIEW
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:NO];
    WKUserScript* noise = [[WKUserScript alloc]
        initWithSource:JS_FINGERPRINT_NOISE
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:NO];
    [ucc addUserScript:init];
    [ucc addUserScript:mask];
    [ucc addUserScript:noise];

    // Inject any app-supplied extra scripts (e.g. input-watcher.js,
    // chrome-gl-compositor.js) in the order they were registered.
    for (const auto& src : s_cbs.extra_scripts) {
        if (src.empty()) continue;
        NSString* nsSrc = [NSString stringWithUTF8String:src.c_str()];
        WKUserScript* extra = [[WKUserScript alloc]
            initWithSource:nsSrc
            injectionTime:WKUserScriptInjectionTimeAtDocumentStart
            forMainFrameOnly:NO];
        [ucc addUserScript:extra];
    }
}

// ── Public API ────────────────────────────────────────────────────────

// ── Network diagnostics ───────────────────────────────────────────────
// Called once at startup. Prints the system proxy configuration and warns
// if iCloud Private Relay is likely active. Private Relay rewrites the
// source IP on every connection; when it changes between the OAuth popup
// request and the LinkedIn session validation, the server sees a new IP
// and invalidates the session token.
static void xcm_check_network(void) {
    NSDictionary* ps = (__bridge_transfer NSDictionary*)CFNetworkCopySystemProxySettings();
    if (!ps || ps.count == 0) {
        fprintf(stderr, "[net] no system proxy settings found\n");
        return;
    }
    // Flatten settings to a human-readable form.
    fprintf(stderr, "[net] proxy settings:\n");
    for (NSString* k in ps) {
        fprintf(stderr, "  %s = %s\n",
                k.UTF8String, [ps[k] description].UTF8String);
    }
    // Heuristic: iCloud Private Relay proxies HTTPS traffic via a MASQUE/QUIC
    // or CONNECT proxy whose host ends in .apple-relay.apple.com or similar.
    // It also sets HTTPSEnable or ProxyAutoConfigEnable with an Apple relay URL.
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

    // Check for any active VPN that might cause similar IP inconsistency.
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

void webview_init(void* ns_window, AppState* state, WebViewCallbacks cbs) {
    s_window      = (__bridge NSWindow*)ns_window;
    s_state       = state;
    s_cbs         = std::move(cbs);
    s_process_pool = [[WKProcessPool alloc] init];
    xcm_check_network();
}

void* webview_create(int tab_id, const std::string& url) {
    NSCAssert(s_window != nil, @"webview_init() must be called first");

    WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
    // Persistent data store -- keeps cookies, localStorage, IndexedDB, and
    // service worker registrations across launches under the stable bundle ID.
    cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore;
    // Shared process pool -- all tabs and popups share one WebContent process
    // so in-memory cookies written by an OAuth popup are immediately visible
    // to the main WKWebView without waiting for the persistent store flush.
    cfg.processPool = s_process_pool;

    // Allow all media (audio and video) to play without a user gesture.
    // Required for autoplay on pages like LinkedIn video feed, Meet, etc.
    cfg.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    // Allow AirPlay from any page.
    cfg.allowsAirPlayForMediaPlayback = YES;

    // Preferences
    WKPreferences* prefs = cfg.preferences;
    // DevTools.
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    // Service workers -- LinkedIn and Google use SW for offline support and
    // for caching auth tokens in the Cache API / IndexedDB. The private key
    // _serviceWorkerEnabled only exists on macOS 14+ (Sonoma); guard with
    // @try so older OS versions do not crash.
    @try { [prefs setValue:@YES forKey:@"_serviceWorkerEnabled"]; }
    @catch (NSException*) {
        fprintf(stderr, "[wv] _serviceWorkerEnabled not available on this OS\n");
    }
    // Full-screen support (macOS 12.3+).
    if (@available(macOS 12.3, *)) prefs.elementFullscreenEnabled = YES;

    // Inject Chrome-masking scripts into every frame (including cross-origin
    // iframes such as accounts.google.com) via the shared helper.
    xcm_inject_masks(cfg.userContentController);

    // Frame: full content area (will be repositioned by webview_resize)
    NSRect frame = s_window.contentView.bounds;
    frame.origin.y    = STATUS_HEIGHT_PX;
    frame.size.height -= (TOTAL_CHROME_TOP + STATUS_HEIGHT_PX);

    WKWebView* wv = [[WKWebView alloc] initWithFrame:frame configuration:cfg];
    wv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    // Present as Chrome so sites serve full desktop pages and do not
    // block or degrade based on an unrecognised WebKit user agent.
    wv.customUserAgent = kUserAgent;

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
    s_handles[tab_id]    = h;
    s_js_enabled[tab_id] = true;  // default on

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
    s_js_enabled.erase(h->tab_id);
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

    // If the user typed a bare hostname (e.g. "linkedin.com") with no scheme,
    // NSURL will parse it as a path rather than a host, resulting in a blank
    // page when WKWebView tries to load it. Prepend https:// in that case.
    NSString* lowered = u.lowercaseString;
    if (![lowered hasPrefix:@"http://"] && ![lowered hasPrefix:@"https://"]
        && ![lowered hasPrefix:@"about:"] && ![lowered hasPrefix:@"data:"]
        && ![lowered hasPrefix:@"blob:"] && ![lowered hasPrefix:@"javascript:"
        ] && ![lowered hasPrefix:@"file://"]) {
        u = [@"https://" stringByAppendingString:u];
    }

    NSURL* ns = [NSURL URLWithString:u];
    if (!ns) {
        // Try percent-encoding the path/query portions.
        ns = [NSURL URLWithString:
              [u stringByAddingPercentEncodingWithAllowedCharacters:
               NSCharacterSet.URLQueryAllowedCharacterSet]];
    }
    if (!ns) {
        fprintf(stderr, "[nav] bad url: %s\n", url.c_str());
        return;
    }
    fprintf(stderr, "[nav] load: %s\n", ns.absoluteString.UTF8String);
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

void webview_set_js_enabled(void* handle, bool enabled) {
    if (!handle) return;
    WVHandle* h = (WVHandle*)handle;
    if (h->js_enabled == enabled) return;
    h->js_enabled = enabled;
    s_js_enabled[h->tab_id] = enabled;
    fprintf(stderr, "[wv] JS %s for tab %d\n", enabled ? "enabled" : "disabled", h->tab_id);
    // Reload so the navigation delegate immediately applies the new
    // allowsContentJavaScript setting via WKWebpagePreferences.
    [h->wv reload];
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

void webview_open_inspector(void* handle) {
    if (!handle) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    // Use KVC to reach the private _inspector object and call show:.
    // This is equivalent to right-click -> Inspect Element and works on
    // macOS 11+ when developerExtrasEnabled = YES.
    @try {
        id inspector = [wv valueForKey:@"_inspector"];
        if (inspector) {
            SEL sel = NSSelectorFromString(@"show:");
            if ([inspector respondsToSelector:sel]) {
                // performSelector:withObject: triggers a retain-cycle warning
                // on newer clang; use NSInvocation to silence it.
                NSMethodSignature* sig = [inspector methodSignatureForSelector:sel];
                NSInvocation* inv = [NSInvocation invocationWithMethodSignature:sig];
                [inv setTarget:inspector];
                [inv setSelector:sel];
                id arg = inspector;
                [inv setArgument:&arg atIndex:2];
                [inv invoke];
            }
        }
    } @catch (NSException*) {
        // Private API may not be available on all OS versions; fail silently.
    }
}

void webview_clear_data() {
    // Clear ALL website data from the persistent store: cookies, localStorage,
    // IndexedDB, cache, service worker registrations, etc. This is the nuclear
    // option for flushing a stuck auth state.
    fprintf(stderr, "[wv] clearing all website data...\n");
    WKWebsiteDataStore* store = WKWebsiteDataStore.defaultDataStore;
    NSSet* types = WKWebsiteDataStore.allWebsiteDataTypes;
    NSDate* epoch = [NSDate dateWithTimeIntervalSince1970:0];
    [store removeDataOfTypes:types
               modifiedSince:epoch
           completionHandler:^{
        fprintf(stderr, "[wv] website data cleared.\n");
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
