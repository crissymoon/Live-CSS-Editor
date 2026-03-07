// webview_cookies.mm -- Cookie and website-data management:
//   webview_clear_cache, webview_clear_data, webview_dump_cookies_json,
//   webview_inject_cookies, webview_set_cf_user_agent.

#include "webview_priv.h"

void webview_clear_cache() {
    // Clear only HTTP disk/memory cache.  Preserves cookies, localStorage,
    // IndexedDB, and auth state.  Use this to flush stale 301 redirects or
    // other cached responses without logging the user out.
    fprintf(stderr, "[wv] clearing http cache...\n");
    WKWebsiteDataStore* store = WKWebsiteDataStore.defaultDataStore;
    NSSet* types = [NSSet setWithObjects:
        WKWebsiteDataTypeDiskCache,
        WKWebsiteDataTypeMemoryCache,
        nil];
    NSDate* epoch = [NSDate dateWithTimeIntervalSince1970:0];
    [store removeDataOfTypes:types
               modifiedSince:epoch
           completionHandler:^{
        fprintf(stderr, "[wv] http cache cleared.\n");
    }];
}

void webview_clear_data() {
    // Clear ALL website data from the persistent store: cookies, localStorage,
    // IndexedDB, cache, service worker registrations, etc.
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

void webview_dump_cookies_json(std::function<void(const std::string&)> callback) {
    WKHTTPCookieStore* store = [WKWebsiteDataStore defaultDataStore].httpCookieStore;
    [store getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
        NSMutableArray* arr = [NSMutableArray array];
        for (NSHTTPCookie* c in cookies) {
            NSMutableDictionary* d = [NSMutableDictionary dictionary];
            d[@"name"]     = c.name     ?: @"";
            d[@"value"]    = c.value    ?: @"";
            d[@"domain"]   = c.domain   ?: @"";
            d[@"path"]     = c.path     ?: @"/";
            d[@"secure"]   = @(c.isSecure);
            d[@"httpOnly"] = @(c.isHTTPOnly);
            if (c.expiresDate) {
                d[@"expiresAt"] = @(c.expiresDate.timeIntervalSince1970);
            }
            [arr addObject:d];
        }
        NSError* je  = nil;
        NSData*  data = [NSJSONSerialization dataWithJSONObject:arr options:0 error:&je];
        std::string result = "[]";
        if (data && !je) {
            NSString* s = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            if (s) result = s.UTF8String;
        }
        fprintf(stderr, "[wv] dump_cookies: %zu cookies serialised\n", (size_t)cookies.count);
        if (callback) callback(result);
    }];
}

// Inject cookies harvested by the cf_bridge Chromium module into the
// default WKWebsiteDataStore so every WKWebView tab benefits immediately.
//
// json_arr: JSON array with objects having at minimum "name", "value",
//   "domain".  Optional keys: "path", "secure", "httpOnly",
//   "expiresAt" (Unix timestamp as number).
//
// Called from cmd_server.cpp via POST /inject-cookies.
void webview_inject_cookies(const std::string& json_arr) {
    NSString* ns_json = [NSString stringWithUTF8String:json_arr.c_str()];
    NSData*   data    = [ns_json dataUsingEncoding:NSUTF8StringEncoding];
    if (!data) {
        fprintf(stderr, "[wv] inject_cookies: empty input\n");
        return;
    }

    NSError* err = nil;
    id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];
    if (!parsed || err) {
        fprintf(stderr, "[wv] inject_cookies: JSON parse error: %s\n",
                [[err localizedDescription] UTF8String]);
        return;
    }
    if (![parsed isKindOfClass:[NSArray class]]) {
        fprintf(stderr, "[wv] inject_cookies: expected JSON array\n");
        return;
    }

    NSArray* arr = (NSArray*)parsed;
    WKHTTPCookieStore* store =
        [WKWebsiteDataStore defaultDataStore].httpCookieStore;

    for (id obj in arr) {
        if (![obj isKindOfClass:[NSDictionary class]]) continue;
        NSDictionary* d = (NSDictionary*)obj;

        NSString* name   = d[@"name"];
        NSString* value  = d[@"value"];
        NSString* domain = d[@"domain"];
        if (!name || !value || !domain) continue;

        NSMutableDictionary* props = [NSMutableDictionary dictionary];
        props[NSHTTPCookieName]   = name;
        props[NSHTTPCookieValue]  = value;
        props[NSHTTPCookieDomain] = domain;
        props[NSHTTPCookiePath]   = d[@"path"] ?: @"/";

        if ([d[@"secure"] boolValue])
            props[NSHTTPCookieSecure] = @"TRUE";

        id expiresAt = d[@"expiresAt"];
        if (expiresAt && [expiresAt isKindOfClass:[NSNumber class]]) {
            double ts = [expiresAt doubleValue];
            if (ts > 0)
                props[NSHTTPCookieExpires] = [NSDate dateWithTimeIntervalSince1970:ts];
        }

        NSHTTPCookie* cookie = [NSHTTPCookie cookieWithProperties:props];
        if (!cookie) {
            fprintf(stderr, "[wv] inject_cookies: failed to build cookie '%s'\n",
                    [name UTF8String]);
            continue;
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            [store setCookie:cookie completionHandler:^{
                fprintf(stderr, "[wv] inject_cookies: injected '%s' '%s'\n",
                        [name UTF8String], [domain UTF8String]);
            }];
        });
    }
    fprintf(stderr, "[wv] inject_cookies: queued %lu cookies for injection\n",
            (unsigned long)[arr count]);
}

// Set the customUserAgent on every open WKWebView tab to the Chrome UA string
// that cf_bridge reported.  CF binds cf_clearance to the UA that solved the
// challenge; WKWebView must send that exact UA when making requests carrying
// the cookie or CF will re-challenge.
//
// Sets the customUserAgent on all open WKWebView tabs.
void webview_set_cf_user_agent(const std::string& ua) {
    if (ua.empty()) return;
    NSString* ns_ua = [NSString stringWithUTF8String:ua.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
        for (auto& [tab_id, h] : s_handles) {
            if (h && h->wv) {
                h->wv.customUserAgent = ns_ua;
                fprintf(stderr, "[wv] set_cf_ua: tab %d\n", tab_id);
            }
        }
    });
}
