// webview_popup.mm -- Auth/billing popup URL patterns +
//   xcm_is_auth_popup_url() + webview_open_virt_popup().
// All other popup logic lives in XCMPopupDelegate (webview_delegates.mm).

#include "webview_priv.h"

// ── Patterns that trigger the secure-window popup ─────────────────────

const char* const s_popup_patterns[] = {
    "platform.claude.com/settings/billing",
    "console.anthropic.com/settings/billing",
    "platform.openai.com/account/billing",
    "auth.openai.com/log-in",
    "pay.stripe.com/",
    nullptr
};

bool xcm_is_auth_popup_url(const char* url) {
    if (!url || !*url) return false;
    for (int i = 0; s_popup_patterns[i]; ++i) {
        if (strstr(url, s_popup_patterns[i])) return true;
    }
    return false;
}

// ── Secure floating panel (virt popup) ───────────────────────────────
// Opens a top-level NSPanel containing its own WKWebView driven by
// XCMVirtDelegate.  Used for billing and login pages that need a
// full, unfettered WebKit context isolated from the main tab.

void webview_open_virt_popup(const std::string& url) {
    std::string u = url;
    // Show the toolbar secure badge immediately.
    {
        std::string js = "xcmShowSecureBadge&&xcmShowSecureBadge('" + u + "')";
        native_chrome_eval_toolbar_js(js.c_str());
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!s_virt_delegates) s_virt_delegates = [NSMutableArray array];
        WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
        @try { cfg.websiteDataStore = WKWebsiteDataStore.defaultDataStore; } @catch (NSException*) {}
        @try { cfg.processPool = s_process_pool; } @catch (NSException*) {}
        xcm_inject_masks(cfg.userContentController);
        xcm_shell_install(cfg.userContentController, s_window, s_state);
        if (s_ad_rule_list) [cfg.userContentController addContentRuleList:s_ad_rule_list];
        NSRect rect = NSMakeRect(0, 0, 920, 760);
        NSPanel* panel = [[NSPanel alloc]
            initWithContentRect:rect
                      styleMask:NSWindowStyleMaskTitled |
                                NSWindowStyleMaskClosable |
                                NSWindowStyleMaskResizable
                        backing:NSBackingStoreBuffered
                          defer:NO];
        panel.title = @"Secure Window";
        panel.level = NSFloatingWindowLevel;
        panel.hidesOnDeactivate = NO;
        panel.releasedWhenClosed = NO;
        WKWebView* wv = [[WKWebView alloc] initWithFrame:rect configuration:cfg];
        XCMVirtDelegate* del = [[XCMVirtDelegate alloc] init];
        del.panel  = panel;
        del.secWV  = wv;
        wv.navigationDelegate = del;
        wv.UIDelegate         = del;
        [s_virt_delegates addObject:del];
        panel.delegate = del;
        panel.contentView = wv;
        [panel center];
        [panel makeKeyAndOrderFront:nil];
        NSURL* ns = [NSURL URLWithString:[NSString stringWithUTF8String:u.c_str()]];
        if (ns) [wv loadRequest:[NSURLRequest requestWithURL:ns]];
        fprintf(stderr, "[virt] opened secure panel for: %s\n", u.c_str());
    });
}
