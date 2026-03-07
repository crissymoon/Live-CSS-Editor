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
// Launches webbrowse_no_controls.py as a fully detached process via
// /bin/sh + nohup + & so it is reparented to launchd and shares no
// process group, file descriptors, or resource constraints with this app.

// JS injected into the active tab to show a full-page "opening secure window"
// animation.  Uses only inline styles -- no external dependencies.
// Auto-fades and removes itself after 3 seconds.
static const char* const kSecureOverlayJS =
"(function(){"
"if(document.getElementById('__xcm_sw_overlay'))return;"
"var s=document.createElement('style');"
"s.id='__xcm_sw_style';"
"s.textContent='@keyframes __xcm_spin{to{transform:rotate(360deg)}}'"
" +'@keyframes __xcm_pulse{0%,80%,100%{transform:scale(.5);opacity:.25}40%{transform:scale(1);opacity:1}}';"
"document.head.appendChild(s);"
"var d=document.createElement('div');"
"d.id='__xcm_sw_overlay';"
"d.style.cssText='position:fixed;inset:0;z-index:2147483647;"
"background:rgba(8,12,22,0.9);"
"display:flex;align-items:center;justify-content:center;"
"backdrop-filter:blur(6px);-webkit-backdrop-filter:blur(6px)';"
"d.innerHTML="
"'<div style=\"display:flex;flex-direction:column;align-items:center;gap:22px;"
"font-family:-apple-system,BlinkMacSystemFont,sans-serif;\">'"
"+'<div style=\"position:relative;width:68px;height:68px;\">'"
"+'<div style=\"position:absolute;top:-14px;left:-14px;width:96px;height:96px;"
"border-radius:50%;border:2px solid rgba(56,189,248,.12);"
"border-top-color:rgba(56,189,248,.9);"
"animation:__xcm_spin 1.1s linear infinite;\"></div>'"
"+'<svg width=\"68\" height=\"68\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#38bdf8\"'"
"+'stroke-width=\"1.5\" stroke-linecap=\"round\" stroke-linejoin=\"round\">'"
"+'<path d=\"M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z\"/>'"
"+'<polyline points=\"9 12 11 14 15 10\"/>'"
"+'</svg></div>'"
"+'<div style=\"color:#94a3b8;font-size:12px;letter-spacing:.1em;text-transform:uppercase;\">'"
"+'Opening secure window</div>'"
"+'<div>'"
"+'<span style=\"display:inline-block;width:7px;height:7px;border-radius:50%;background:#38bdf8;margin:0 4px;animation:__xcm_pulse 1.4s ease-in-out infinite;\"></span>'"
"+'<span style=\"display:inline-block;width:7px;height:7px;border-radius:50%;background:#38bdf8;margin:0 4px;animation:__xcm_pulse 1.4s ease-in-out .2s infinite;\"></span>'"
"+'<span style=\"display:inline-block;width:7px;height:7px;border-radius:50%;background:#38bdf8;margin:0 4px;animation:__xcm_pulse 1.4s ease-in-out .4s infinite;\"></span>'"
"+'</div></div>';"
"document.documentElement.appendChild(d);"
"setTimeout(function(){"
"d.style.transition='opacity .5s ease';"
"d.style.opacity='0';"
"setTimeout(function(){"
"if(d.parentNode)d.parentNode.removeChild(d);"
"var ss=document.getElementById('__xcm_sw_style');"
"if(ss&&ss.parentNode)ss.parentNode.removeChild(ss);"
"},600);"
"},3000);"
"})();";

void webview_open_virt_popup(const std::string& url) {
    fprintf(stderr, "[popup] launching no-controls browser for: %s\n", url.c_str());

    // ── Full-page overlay animation in the active tab ────────────────
    // Inject into the active tab's WKWebView so the user sees a visual
    // indicator while the Python process starts up.  Auto-removes after 3s.
    if (s_state) {
        Tab* tab = s_state->current_tab();
        if (tab) {
            auto it = s_handles.find(tab->id);
            if (it != s_handles.end() && it->second && it->second->wv) {
                WKWebView* wv = it->second->wv;
                NSString* animJS = [NSString stringWithUTF8String:kSecureOverlayJS];
                [wv evaluateJavaScript:animJS completionHandler:nil];
            }
        }
    }

    // Show the toolbar secure badge.
    {
        std::string js = "xcmShowSecureBadge&&xcmShowSecureBadge('" + url + "')";
        native_chrome_eval_toolbar_js(js.c_str());
    }

    // Shell-escape the URL so single quotes inside it cannot break the command.
    std::string escaped_url;
    for (char c : url) {
        if (c == '\'') escaped_url += "'\\''";
        else           escaped_url += c;
    }

    // nohup + & reparents the child to launchd -- fully detached from this app.
    std::string cmd =
        "nohup "
        "/Users/mac/Documents/live-css/dev-browser/venv/bin/python3 "
        "/Users/mac/Documents/live-css/dev-browser/webbrowse_no_controls.py "
        "--url '" + escaped_url + "' "
        "</dev/null >/dev/null 2>&1 &";

    dispatch_async(dispatch_get_main_queue(), ^{
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = @"/bin/sh";
        task.arguments  = @[@"-c", [NSString stringWithUTF8String:cmd.c_str()]];
        task.currentDirectoryPath = @"/Users/mac/Documents/live-css/dev-browser";

        NSError* err = nil;
        if (@available(macOS 10.13, *)) {
            [task launchAndReturnError:&err];
        } else {
            [task launch];
        }

        if (err) {
            fprintf(stderr, "[popup] launch error: %s\n",
                    err.localizedDescription.UTF8String);
        } else {
            fprintf(stderr, "[popup] shell launched (detached via nohup &)\n");
        }
    });
}
