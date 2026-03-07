// webview_nav.mm -- Navigation, JS evaluation, inspector, clipboard,
//   system-browser open, and JS-enabled toggle.

#include "webview_priv.h"

void webview_load_url(void* handle, const std::string& url) {
    if (!handle || url.empty()) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    NSString*  u  = [NSString stringWithUTF8String:url.c_str()];

    // Prepend https:// if the user typed a bare hostname with no scheme.
    NSString* lowered = u.lowercaseString;
    if (![lowered hasPrefix:@"http://"] && ![lowered hasPrefix:@"https://"]
        && ![lowered hasPrefix:@"about:"] && ![lowered hasPrefix:@"data:"]
        && ![lowered hasPrefix:@"blob:"] && ![lowered hasPrefix:@"javascript:"]
        && ![lowered hasPrefix:@"file://"]) {
        u = [@"https://" stringByAppendingString:u];
    }

    NSURL* ns = [NSURL URLWithString:u];
    if (!ns) {
        ns = [NSURL URLWithString:
              [u stringByAddingPercentEncodingWithAllowedCharacters:
               NSCharacterSet.URLQueryAllowedCharacterSet]];
    }
    if (!ns) {
        fprintf(stderr, "[nav] bad url: %s\n", url.c_str());
        return;
    }
    fprintf(stderr, "[nav] load: %s\n", ns.absoluteString.UTF8String);
    // ReloadIgnoringLocalCacheData prevents stale cached 301 redirects from
    // a previous server config from hijacking explicit user navigations.
    NSURLRequest* req = [NSURLRequest requestWithURL:ns
                                         cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                     timeoutInterval:60.0];
    [wv loadRequest:req];
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
        if (!err && res) result = [NSString stringWithFormat:@"%@", res].UTF8String;
        if (cb) cb(result);
    }];
}

void webview_open_inspector(void* handle) {
    if (!handle) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    @try {
        id inspector = [wv valueForKey:@"_inspector"];
        if (inspector) {
            SEL sel = NSSelectorFromString(@"show:");
            if ([inspector respondsToSelector:sel]) {
                NSMethodSignature* sig = [inspector methodSignatureForSelector:sel];
                NSInvocation* inv = [NSInvocation invocationWithMethodSignature:sig];
                [inv setTarget:inspector];
                [inv setSelector:sel];
                id arg = inspector;
                [inv setArgument:&arg atIndex:2];
                [inv invoke];
            }
        }
    } @catch (NSException*) {}
}

void webview_clipboard_action(void* handle, const char* action) {
    if (!handle || !action) return;
    WKWebView* wv = ((WVHandle*)handle)->wv;
    if (!wv) return;

    if (strcmp(action, "copy") == 0) {
        [NSApp sendAction:@selector(copy:) to:wv from:nil];
        fprintf(stderr, "[wv] clipboard action: copy\n");
        return;
    }
    if (strcmp(action, "cut") == 0) {
        [NSApp sendAction:@selector(cut:) to:wv from:nil];
        fprintf(stderr, "[wv] clipboard action: cut\n");
        return;
    }
    if (strcmp(action, "selectAll") == 0) {
        [NSApp sendAction:@selector(selectAll:) to:wv from:nil];
        fprintf(stderr, "[wv] clipboard action: selectAll\n");
        return;
    }

    if (strcmp(action, "paste") == 0) {
        // WKWebView has an internal clipboard that shadows NSPasteboard when
        // paste: is sent via the responder chain.  Text copied from external
        // apps ends up in NSPasteboard only, so naive sendAction:paste: always
        // inserts whatever was last copied INSIDE the browser.
        // Fix: read NSPasteboard directly and call window.__xcmPaste(text)
        // (provided by xcm-clip-watcher.js).
        NSPasteboard* pb   = NSPasteboard.generalPasteboard;
        NSString*     text = [pb stringForType:NSPasteboardTypeString];
        if (!text) {
            [NSApp sendAction:@selector(paste:) to:wv from:nil];
            fprintf(stderr, "[wv] clipboard action: paste (native fallback)\n");
            return;
        }
        NSError* err  = nil;
        NSData*  jraw = [NSJSONSerialization dataWithJSONObject:text
                                                       options:NSJSONWritingFragmentsAllowed
                                                         error:&err];
        if (!jraw || err) {
            [NSApp sendAction:@selector(paste:) to:wv from:nil];
            fprintf(stderr, "[wv] clipboard action: paste (json err, native fallback)\n");
            return;
        }
        NSString* j  = [[NSString alloc] initWithData:jraw encoding:NSUTF8StringEncoding];
        NSString* js = [NSString stringWithFormat:@"window.__xcmPaste(%@)", j];
        [wv evaluateJavaScript:js completionHandler:^(id res, NSError* e) {
            if (e) fprintf(stderr, "[wv] paste js error: %s\n",
                           e.localizedDescription.UTF8String);
        }];
        fprintf(stderr, "[wv] clipboard action: paste (%lu chars)\n",
                (unsigned long)text.length);
        return;
    }

    fprintf(stderr, "[wv] clipboard action: unknown: %s\n", action);
}

// Open a URL in the user's default browser (real Safari or Chrome).
// USE THIS FOR: Google sign-in, sites that block embedded WebView flows.
// Google's OAuth policy blocks sign-in from any embedded WKWebView regardless
// of UA, because it is a security policy (confused-deputy attacks), not a
// fingerprint check.
void webview_open_in_system_browser(const std::string& url) {
    NSString* ns_url = [NSString stringWithUTF8String:url.c_str()];
    NSURL* ns = [NSURL URLWithString:ns_url];
    if (!ns) {
        fprintf(stderr, "[wv] webview_open_in_system_browser: invalid URL: %s\n", url.c_str());
        return;
    }
    [[NSWorkspace sharedWorkspace] openURL:ns];
    fprintf(stderr, "[wv] opened in system browser: %s\n", url.c_str());
}
