// webview_state.mm -- Definitions of all shared webview module state.
// Each variable is declared extern in webview_priv.h and defined exactly
// once here.  No logic lives in this file.

#include "webview_priv.h"

NSWindow*          s_window       = nil;
AppState*          s_state        = nullptr;
WebViewCallbacks   s_cbs;
WKProcessPool*     s_process_pool = nil;
WKContentRuleList* s_ad_rule_list = nil;

std::unordered_map<std::string, std::string> s_report_to;
std::unordered_map<int, bool>                s_js_enabled;
std::unordered_map<int, WVHandle*>           s_handles;

// Delegate containers -- owned/managed by webview_delegates.mm.
XCMDownloadDelegate*               s_dl_delegate    = nil;
NSMutableArray<XCMPopupDelegate*>* s_popup_delegates = nil;
NSMutableArray*                    s_virt_delegates  = nil;
