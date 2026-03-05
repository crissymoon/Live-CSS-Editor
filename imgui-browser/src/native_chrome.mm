// native_chrome.mm -- AppKit-native browser chrome
// Tab bar and status bar are drawn NSViews.
// Toolbar (URL bar + nav buttons) is a WKWebView child panel
// (src/toolbar.html) with a JS bridge so the HTML page drives
// navigation and receives live tab state every frame.

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <QuartzCore/QuartzCore.h>
#include "native_chrome.h"
#include "app_state.h"
#include <cstdio>
#include <algorithm>
#include <string>
#include <sstream>

// ── Static shared state ───────────────────────────────────────────────

static AppState* s_state    = nullptr;
static NSWindow* s_window   = nil;
static int       s_php_port = 0;

// forward declarations
@class XCMTabBarView;
@class XCMStatusView;
@class XCMBridgeHandler;

static XCMTabBarView*    s_tabs          = nil;
static NSPanel*          s_toolbar_panel = nil;
static WKWebView*        s_toolbar_wv    = nil;
static XCMBridgeHandler* s_bridge        = nil;
static XCMStatusView*    s_status        = nil;
static bool              s_tb_ready      = false;

static float   s_bm_btn_x    = 0.0f;
static float   s_hist_btn_x  = 0.0f;
static CGFloat s_extra_h     = 0.0f;   // extra panel height when dropdown open
static int     s_panel_win_w = 0;
static int     s_panel_win_h = 0;

// ── Color palette ─────────────────────────────────────────────────────

static NSColor* cSurface() { return [NSColor colorWithRed:.063 green:.063 blue:.094 alpha:1]; }
static NSColor* cAccent()  { return [NSColor colorWithRed:.388 green:.400 blue:.941 alpha:1]; }
static NSColor* cAccentLo(){ return [NSColor colorWithRed:.388 green:.400 blue:.941 alpha:.18]; }
static NSColor* cTabAct()  { return [NSColor colorWithRed:.185 green:.185 blue:.285 alpha:1]; }
static NSColor* cTabHov()  { return [NSColor colorWithRed:.155 green:.155 blue:.235 alpha:1]; }
static NSColor* xcmTxt()    { return [NSColor colorWithRed:.900 green:.914 blue:.961 alpha:1]; }
static NSColor* xcmTxtDim() { return [NSColor colorWithRed:.400 green:.427 blue:.502 alpha:1]; }
static NSColor* cSep()     { return [NSColor colorWithRed:.388 green:.400 blue:.941 alpha:.12]; }
static NSColor* cStatusBg(){ return [NSColor colorWithRed:.165 green:.130 blue:.270 alpha:1]; }
static NSColor* cOK()      { return [NSColor colorWithRed:.204 green:.827 blue:.600 alpha:1]; }
static NSColor* cBad()     { return [NSColor colorWithRed:.973 green:.529 blue:.451 alpha:1]; }

// ── XCMTabBarView ─────────────────────────────────────────────────────

@interface XCMTabBarView : NSView
@property (nonatomic) NSInteger hoverIdx;
@property (nonatomic) NSInteger dragIdx;
@property (nonatomic) CGFloat   dragStartX;
@property (nonatomic) CGFloat   dragCurrentX;
@property (nonatomic) NSInteger dropTarget;
@property (nonatomic, strong) NSTrackingArea* trackArea;
@end

@implementation XCMTabBarView
- (BOOL)acceptsFirstMouse:(NSEvent*)ev { return YES; }

- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque  { return YES; }

- (instancetype)initWithFrame:(NSRect)r {
    if (!(self = [super initWithFrame:r])) return nil;
    self.wantsLayer = YES;
    self.layer.geometryFlipped = YES;  // match CALayer coords to flipped NSView coords
    _hoverIdx    = -1;
    _dragIdx     = -1;
    _dropTarget  = -1;
    [self updateTracks];
    return self;
}

- (void)updateTracks {
    if (_trackArea) [self removeTrackingArea:_trackArea];
    _trackArea = [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited |
                     NSTrackingActiveAlways
               owner:self userInfo:nil];
    [self addTrackingArea:_trackArea];
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    [self updateTracks];
}

// ── Geometry ──────────────────────────────────────────────────────────

- (CGFloat)tabW {
    NSInteger n = s_state ? (NSInteger)s_state->tabs.size() : 1;
    CGFloat avail = self.bounds.size.width - (CGFloat)TRAFFIC_LIGHT_W - 34.0;
    return std::min(180.0, std::max(64.0, avail / std::max((NSInteger)1, n)));
}

- (NSRect)tabRectAt:(NSInteger)i {
    CGFloat tw = [self tabW];
    CGFloat cx = (CGFloat)TRAFFIC_LIGHT_W + tw * i;
    return NSMakeRect(cx + 1.0, 32.0, tw - 2.0, 34.0);
}

- (NSRect)closeRectAt:(NSInteger)i {
    NSRect tr = [self tabRectAt:i];
    return NSMakeRect(NSMaxX(tr) - 19.0,
                      NSMinY(tr) + (tr.size.height - 14.0) * 0.5,
                      14.0, 14.0);
}

- (NSRect)newTabRect {
    NSInteger n = s_state ? (NSInteger)s_state->tabs.size() : 0;
    CGFloat tw = [self tabW];
    CGFloat cx = (CGFloat)TRAFFIC_LIGHT_W + tw * n + 5.0;
    return NSMakeRect(cx, 33.0, 22.0, 22.0);
}

- (NSInteger)tabAtPoint:(NSPoint)p {
    if (!s_state) return -1;
    NSInteger n = (NSInteger)s_state->tabs.size();
    for (NSInteger i = 0; i < n; i++) {
        if (NSPointInRect(p, [self tabRectAt:i])) return i;
    }
    return -1;
}

// ── Drawing ───────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirty {
    NSInteger n    = s_state ? (NSInteger)s_state->tabs.size() : 0;
    NSInteger act  = s_state ? (NSInteger)s_state->active_tab  : 0;
    CGFloat   W    = self.bounds.size.width;
    CGFloat   H    = self.bounds.size.height;   // 70

    // Background
    [cSurface() set];
    NSRectFill(self.bounds);

    // Top highlight edge (1px)
    NSBezierPath* topLine = [NSBezierPath bezierPathWithRect:NSMakeRect(0, 0, W, 1.0)];
    [[NSColor colorWithRed:.60 green:.58 blue:1.0 alpha:.22] set];
    [topLine fill];

    // Gradient shimmer: purple tint fading out downward
    NSGradient* shimmer = [[NSGradient alloc]
        initWithStartingColor:[NSColor colorWithRed:.40 green:.38 blue:.88 alpha:.06]
                  endingColor:[NSColor colorWithRed:.40 green:.38 blue:.88 alpha:0]];
    [shimmer drawInRect:self.bounds angle:270];

    // Bottom separator
    [[cSep() colorWithAlphaComponent:.25] set];
    NSRectFill(NSMakeRect(0, H - 1.0, W, 1.0));

    // Tabs
    NSDictionary* dimAttrs = @{
        NSFontAttributeName:            [NSFont systemFontOfSize:12.0],
        NSForegroundColorAttributeName: xcmTxtDim(),
    };
    NSDictionary* actAttrs = @{
        NSFontAttributeName:            [NSFont systemFontOfSize:12.0 weight:NSFontWeightSemibold],
        NSForegroundColorAttributeName: xcmTxt(),
    };

    for (NSInteger i = 0; i < n; i++) {
        bool isActive  = (i == act);
        bool isHovered = (i == _hoverIdx);
        bool isDragged = (i == _dragIdx && _dragIdx != -1 &&
                          fabs(_dragCurrentX - _dragStartX) > 4.0);

        NSRect tr = [self tabRectAt:i];

        // During drag, draw ghost instead of original
        if (isDragged) {
            // Draw a ghost placeholder where the tab was
            [[cTabAct() colorWithAlphaComponent:.20] set];
            [[NSBezierPath bezierPathWithRoundedRect:tr xRadius:6 yRadius:6] fill];
            continue;
        }

        // Fill
        NSColor* bg = isActive ? cTabAct() : (isHovered ? cTabHov() : nil);
        if (bg) {
            [bg set];
            [[NSBezierPath bezierPathWithRoundedRect:tr xRadius:6 yRadius:6] fill];
        }

        // Border -- lighter on active, subtle on hover, barely visible at rest
        CGFloat borderAlpha = isActive ? .40 : (isHovered ? .20 : .10);
        [[cAccent() colorWithAlphaComponent:borderAlpha] set];
        NSBezierPath* border = [NSBezierPath bezierPathWithRoundedRect:tr xRadius:6 yRadius:6];
        border.lineWidth = 1.0;
        [border stroke];

        // Loading dot
        bool isLoading = s_state && i < (NSInteger)s_state->tabs.size() &&
                         s_state->tabs[i].loading;
        bool showClose = (n > 1) && (isActive || isHovered);
        CGFloat textRight = NSMaxX(tr) - (showClose ? 22.0 : 8.0) - (isLoading ? 14.0 : 0.0);

        if (isLoading) {
            CGFloat dx = textRight + 7.0;
            CGFloat dy = NSMinY(tr) + tr.size.height * 0.5;
            NSBezierPath* dot = [NSBezierPath bezierPathWithOvalInRect:
                NSMakeRect(dx - 3.0, dy - 3.0, 6.0, 6.0)];
            [cAccent() set];
            [dot fill];
        }

        // Title
        NSString* title = @"New Tab";
        if (s_state && i < (NSInteger)s_state->tabs.size()) {
            const auto& t = s_state->tabs[i];
            std::string disp = t.title.empty() ? t.url : t.title;
            if (disp.size() > 40) disp = disp.substr(0, 40);
            title = [NSString stringWithUTF8String:disp.c_str()];
        }

        NSDictionary* attrs = isActive ? actAttrs : dimAttrs;
        NSSize ts = [title sizeWithAttributes:attrs];
        CGFloat tx  = tr.origin.x + 10.0;
        CGFloat trx = textRight;
        CGFloat ty  = tr.origin.y + (tr.size.height - ts.height) * 0.5;

        if (ts.width > trx - tx) {
            // Clip to available rect with a fade
            [NSGraphicsContext saveGraphicsState];
            [[NSBezierPath bezierPathWithRect:NSMakeRect(tx, tr.origin.y, trx - tx, tr.size.height)] addClip];
        }
        [title drawAtPoint:NSMakePoint(tx, ty) withAttributes:attrs];
        if (ts.width > trx - tx) [NSGraphicsContext restoreGraphicsState];

        // Close X
        if (showClose) {
            NSRect cr = [self closeRectAt:i];
            NSPoint mp = [self convertPoint:[self.window mouseLocationOutsideOfEventStream]
                                   fromView:nil];
            bool closeHov = NSPointInRect(mp, cr);
            if (closeHov) {
                [[[NSColor redColor] colorWithAlphaComponent:.5] set];
                [[NSBezierPath bezierPathWithOvalInRect:
                    NSMakeRect(cr.origin.x - 1, cr.origin.y - 1, 16, 16)] fill];
            }
            NSColor* xc = closeHov ? [NSColor whiteColor] : xcmTxtDim();
            [xc set];
            CGFloat m = cr.origin.x + 3.5;
            CGFloat t2 = cr.origin.y + 3.5;
            CGFloat e = 7.0;
            NSBezierPath* x1 = [NSBezierPath bezierPath];
            [x1 moveToPoint:NSMakePoint(m, t2)];
            [x1 lineToPoint:NSMakePoint(m + e, t2 + e)];
            x1.lineWidth = 1.4;
            [x1 stroke];
            NSBezierPath* x2 = [NSBezierPath bezierPath];
            [x2 moveToPoint:NSMakePoint(m + e, t2)];
            [x2 lineToPoint:NSMakePoint(m, t2 + e)];
            x2.lineWidth = 1.4;
            [x2 stroke];
        }
    }

    // Ghost tab during drag
    if (_dragIdx != -1 && fabs(_dragCurrentX - _dragStartX) > 4.0 && s_state) {
        CGFloat tw = [self tabW];
        CGFloat gx = std::max((CGFloat)TRAFFIC_LIGHT_W,
                     std::min(_dragCurrentX - (_dragCurrentX - _dragStartX),
                              (CGFloat)TRAFFIC_LIGHT_W + tw * n - tw));
        // Center ghost on cursor X
        gx = _dragCurrentX - (tw * 0.5);
        gx = std::max((CGFloat)TRAFFIC_LIGHT_W, std::min(gx, (CGFloat)TRAFFIC_LIGHT_W + tw * (n-1)));
        NSRect ghost = NSMakeRect(gx + 1, 32.0, tw - 2.0, 34.0);
        [[cAccent() colorWithAlphaComponent:.20] set];
        [[NSBezierPath bezierPathWithRoundedRect:ghost xRadius:6 yRadius:6] fill];
        [[cAccent() colorWithAlphaComponent:.70] set];
        NSBezierPath* gb = [NSBezierPath bezierPathWithRoundedRect:ghost xRadius:6 yRadius:6];
        gb.lineWidth = 1.5;
        [gb stroke];

        // Drop indicator line
        if (_dropTarget >= 0 && s_state) {
            CGFloat lx = (CGFloat)TRAFFIC_LIGHT_W + tw * _dropTarget;
            if (_dropTarget > _dragIdx) lx += tw;
            [cAccent() set];
            NSBezierPath* dl = [NSBezierPath bezierPath];
            [dl moveToPoint:NSMakePoint(lx, 32)];
            [dl lineToPoint:NSMakePoint(lx, 66)];
            dl.lineWidth = 2.0;
            [dl stroke];
        }
    }

    // '+' new tab button
    NSRect nr = [self newTabRect];
    bool nhov = NSPointInRect([self convertPoint:[self.window mouseLocationOutsideOfEventStream]
                                        fromView:nil], nr);
    if (nhov) {
        [[cAccentLo() colorWithAlphaComponent:.22] set];
        [[NSBezierPath bezierPathWithRoundedRect:nr xRadius:5 yRadius:5] fill];
    }
    NSColor* pc = nhov ? cAccent() : xcmTxtDim();
    [pc set];
    CGFloat cx2 = NSMidX(nr), cy2 = NSMidY(nr);
    CGFloat arm  = 5.0;
    NSBezierPath* ph = [NSBezierPath bezierPath];
    [ph moveToPoint:NSMakePoint(cx2 - arm, cy2)];
    [ph lineToPoint:NSMakePoint(cx2 + arm, cy2)];
    ph.lineWidth = 1.6;
    [ph stroke];
    NSBezierPath* pv = [NSBezierPath bezierPath];
    [pv moveToPoint:NSMakePoint(cx2, cy2 - arm)];
    [pv lineToPoint:NSMakePoint(cx2, cy2 + arm)];
    pv.lineWidth = 1.6;
    [pv stroke];
}

// ── Mouse events ──────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)ev {
    // Ensure the main window is key before processing -- WKWebView content
    // focus can leave the GLFW window non-key so the first click was lost.
    if (![self.window isKeyWindow]) {
        [self.window makeKeyAndOrderFront:nil];
    }
    if (!s_state) return;
    NSPoint p = [self convertPoint:ev.locationInWindow fromView:nil];
    NSInteger i = [self tabAtPoint:p];
    if (i >= 0) {
        // Close button hit?
        bool showClose = ((NSInteger)s_state->tabs.size() > 1) &&
                         (i == s_state->active_tab || i == _hoverIdx);
        if (showClose && NSPointInRect(p, [self closeRectAt:i])) {
            // Close handled in mouseUp
            _dragIdx = -(i + 100);  // encode close intent
            return;
        }
        s_state->active_tab = (int)i;
        _dragIdx   = (int)i;
        _dragStartX   = p.x;
        _dragCurrentX = p.x;
        _dropTarget   = -1;
        [self setNeedsDisplay:YES];
        return;
    }
    // "+" button
    if (NSPointInRect(p, [self newTabRect])) {
        _dragIdx = -1;
        return;
    }
}

- (void)mouseUp:(NSEvent*)ev {
    if (!s_state) return;
    NSPoint p = [self convertPoint:ev.locationInWindow fromView:nil];

    // Close intent
    if (_dragIdx <= -100) {
        NSInteger ci = -(NSInteger)_dragIdx - 100;
        if (ci >= 0 && ci < (NSInteger)s_state->tabs.size()) {
            // tab_id -5 = close-tab request; url carries the tab's own .id as string
            s_state->push_nav(-5, std::to_string(s_state->tabs[ci].id));
        }
        _dragIdx = -1;
        [self setNeedsDisplay:YES];
        return;
    }

    // New tab button -- tab_id -2 signals dispatch_nav to open a new tab
    if (NSPointInRect(p, [self newTabRect]) && _dragIdx == -1) {
        std::string url = "http://127.0.0.1:" + std::to_string(s_php_port) + "/";
        s_state->push_nav(-2, url);
        [self setNeedsDisplay:YES];
        return;
    }

    // Drag complete
    if (_dragIdx >= 0) {
        bool dragged = fabs(_dragCurrentX - _dragStartX) > 4.0;
        if (dragged && _dropTarget >= 0 && _dropTarget != _dragIdx) {
            int from = (int)_dragIdx, to = (int)_dropTarget;
            int was  = s_state->active_tab;
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
        _dragIdx    = -1;
        _dropTarget = -1;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDragged:(NSEvent*)ev {
    if (_dragIdx < 0 || !s_state) return;
    NSPoint p = [self convertPoint:ev.locationInWindow fromView:nil];
    _dragCurrentX = p.x;
    CGFloat tw = [self tabW];
    NSInteger n = (NSInteger)s_state->tabs.size();
    _dropTarget = std::max((NSInteger)0, std::min(n - 1,
                  (NSInteger)((p.x - TRAFFIC_LIGHT_W) / tw)));
    [self setNeedsDisplay:YES];
}

- (void)mouseMoved:(NSEvent*)ev {
    NSPoint p = [self convertPoint:ev.locationInWindow fromView:nil];
    NSInteger prev = _hoverIdx;
    _hoverIdx = [self tabAtPoint:p];
    if (_hoverIdx != prev) [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)ev {
    _hoverIdx = -1;
    [self setNeedsDisplay:YES];
}

@end

// ── XCMBridgeHandler -- WKScriptMessageHandler for toolbar.html ───────
//
// toolbar.html calls:
//   window.webkit.messageHandlers.xcmBridge.postMessage({action, data})
//
// Supported actions:
//   navigate  -- data = URL string
//   back / fwd / reload
//   devt / js / bm / showbm / hist
//   urlfocus / urlblur  -- tell native side URL field has focus
//
// Native side calls xcmSetState(stateJSON) in the toolbar WKWebView every
// frame to push live tab state.

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
        if (!t) return;
        if (t->loading) { s_state->push_nav(t->id, "__stop__");   xcm_status("Stopped"); }
        else            { s_state->push_nav(t->id, "__reload__"); xcm_status("Reloading..."); }
    } else if ([action isEqualToString:@"devt"]) {
        s_state->dev_tools_open = !s_state->dev_tools_open;
        if (t) s_state->push_nav(t->id, "__devtools__");
        xcm_status(s_state->dev_tools_open ? "Dev tools opened" : "Dev tools closed");
    } else if ([action isEqualToString:@"js"]) {
        if (!t) return;
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
        // toolbar WKWebView stole key window for URL editing -- that is fine
    } else if ([action isEqualToString:@"urlblur"]) {
        // return key focus to the main browser window after URL editing
        [s_window makeKeyAndOrderFront:nil];
    } else if ([action isEqualToString:@"dropdownOpen"]) {
        CGFloat h = data ? (CGFloat)data.floatValue : 220.0f;
        s_extra_h = h;
        if (s_toolbar_panel) {
            int ww = s_panel_win_w > 0 ? s_panel_win_w : (int)s_window.frame.size.width;
            int wh = s_panel_win_h > 0 ? s_panel_win_h : (int)s_window.frame.size.height;
            CGFloat sx = s_window.frame.origin.x;
            CGFloat sy = s_window.frame.origin.y + wh - (CGFloat)TOTAL_CHROME_TOP - s_extra_h;
            [s_toolbar_panel setFrame:NSMakeRect(sx, sy, (CGFloat)ww,
                (CGFloat)CHROME_HEIGHT_PX + s_extra_h) display:YES];
        }
    } else if ([action isEqualToString:@"dropdownClose"]) {
        s_extra_h = 0.0f;
        if (s_toolbar_panel) {
            int ww = s_panel_win_w > 0 ? s_panel_win_w : (int)s_window.frame.size.width;
            int wh = s_panel_win_h > 0 ? s_panel_win_h : (int)s_window.frame.size.height;
            CGFloat sx = s_window.frame.origin.x;
            CGFloat sy = s_window.frame.origin.y + wh - (CGFloat)TOTAL_CHROME_TOP;
            [s_toolbar_panel setFrame:NSMakeRect(sx, sy, (CGFloat)ww,
                (CGFloat)CHROME_HEIGHT_PX) display:YES];
        }
    }
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
       << "isBm:"     << (isBm ? "true" : "false")
       << "});";

    NSString* jsStr = [NSString stringWithUTF8String:js.str().c_str()];
    [s_toolbar_wv evaluateJavaScript:jsStr completionHandler:nil];

    // Floating panel right-edge anchors.  Use the right edge of the panel.
    NSRect pf = s_toolbar_panel.frame;
    s_bm_btn_x   = (float)(pf.origin.x + pf.size.width - 8.0);
    s_hist_btn_x = s_bm_btn_x;
}


// ── XCMStatusView ─────────────────────────────────────────────────────

@interface XCMStatusView : NSView
@property (strong) NSTextField* leftLabel;
@property (strong) NSTextField* rightLabel;
@end

@implementation XCMStatusView

- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque  { return YES; }

- (instancetype)initWithFrame:(NSRect)r {
    if (!(self = [super initWithFrame:r])) return nil;

    auto makeLbl = [](NSView* parent) {
        NSTextField* f = [NSTextField labelWithString:@""];
        f.font             = [NSFont monospacedDigitSystemFontOfSize:10.5 weight:NSFontWeightRegular];
        f.textColor        = [NSColor colorWithRed:.400 green:.427 blue:.502 alpha:1];
        f.drawsBackground  = NO;
        f.backgroundColor  = NSColor.clearColor;
        [parent addSubview:f];
        return f;
    };
    _leftLabel  = makeLbl(self);
    _rightLabel = makeLbl(self);
    return self;
}

- (void)drawRect:(NSRect)dirty {
    [cStatusBg() set];
    NSRectFill(self.bounds);
    // Top separator
    [[cSep() colorWithAlphaComponent:.30] set];
    NSRectFill(NSMakeRect(0, 0, self.bounds.size.width, 1.0));

    // PHP + Node LED dots (drawn with CoreGraphics for pixel precision)
    CGFloat W = self.bounds.size.width;
    CGFloat midY = self.bounds.size.height * 0.5;
    CGFloat r    = 3.5;
    CGFloat lx   = W - 60.0;

    NSBezierPath* phpDot = [NSBezierPath bezierPathWithOvalInRect:
        NSMakeRect(lx - r, midY - r, r*2, r*2)];
    [(s_state && s_state->php_server_ok ? cOK() : cBad()) set];
    [phpDot fill];

    NSDictionary* la = @{NSFontAttributeName: [NSFont systemFontOfSize:9.5],
                         NSForegroundColorAttributeName: xcmTxtDim()};
    [@"php" drawAtPoint:NSMakePoint(lx + r + 2, midY - 5.5) withAttributes:la];

    NSBezierPath* jsDot = [NSBezierPath bezierPathWithOvalInRect:
        NSMakeRect(lx + 36.0 - r, midY - r, r*2, r*2)];
    [(s_state && s_state->node_server_ok ? cOK() : cBad()) set];
    [jsDot fill];

    [@"js" drawAtPoint:NSMakePoint(lx + 36.0 + r + 2, midY - 5.5) withAttributes:la];
}

- (void)syncState:(AppState*)st {
    // Left: hover URL or status text
    NSString* leftTxt = @"";
    if (!st->hover_url.empty())
        leftTxt = [NSString stringWithUTF8String:st->hover_url.c_str()];
    else if (!st->status_text.empty())
        leftTxt = [NSString stringWithUTF8String:st->status_text.c_str()];
    _leftLabel.stringValue = leftTxt;

    // Right: viewport size
    int ch = st->win_h - TAB_BAR_HEIGHT_PX - CHROME_HEIGHT_PX - STATUS_HEIGHT_PX;
    _rightLabel.stringValue = [NSString stringWithFormat:@"%d x %d", st->win_w, ch];

    // Layout labels
    CGFloat H = self.bounds.size.height;
    CGFloat W = self.bounds.size.width;
    CGFloat lh = 14.0;
    CGFloat ly = (H - lh) * 0.5;
    _leftLabel.frame  = NSMakeRect(10, ly, W * 0.5, lh);
    _rightLabel.frame = NSMakeRect(W - 160.0, ly, 100.0, lh);
    _rightLabel.alignment = NSTextAlignmentRight;

    [self setNeedsDisplay:YES];
}

@end

// ── XCMToolbarPanel ──────────────────────────────────────────────────
// Borderless child NSPanel holding the toolbar WKWebView (toolbar.html).
// Being a separate NSWindow from the GLFW GL view means it gets its own
// AppKit event pipeline -- no GL interception at all.

@interface XCMToolbarPanel : NSPanel
@end
@implementation XCMToolbarPanel
- (BOOL)canBecomeKeyWindow  { return YES; }
- (BOOL)canBecomeMainWindow { return NO;  }
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

// Locate toolbar.html relative to the running executable.
// Layout: imgui_browser.app/Contents/MacOS/imgui_browser  (4 levels above project root)
//         imgui_browser.app/Contents/Resources/toolbar.html
// For dev builds: MacOS/ -> Contents/ -> .app/ -> build/ -> project/ -> src/
static NSURL* toolbarHtmlURL() {
    // 1. Dev build: 4 levels up from MacOS/ to reach imgui-browser/, then src/
    NSString* binDir = [NSBundle mainBundle].executablePath.stringByDeletingLastPathComponent;
    NSString* dev1   = [binDir stringByAppendingPathComponent:@"../../../../src/toolbar.html"];
    NSString* dev1r  = dev1.stringByStandardizingPath;
    if ([[NSFileManager defaultManager] fileExistsAtPath:dev1r])
        return [NSURL fileURLWithPath:dev1r];
    // 2. App bundle Resources
    NSURL* res = [[NSBundle mainBundle] URLForResource:@"toolbar" withExtension:@"html"];
    if (res) return res;
    // 3. Same directory as binary
    NSString* same = [binDir stringByAppendingPathComponent:@"toolbar.html"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:same])
        return [NSURL fileURLWithPath:same];
    fprintf(stderr, "[toolbar] WARNING: toolbar.html not found\n");
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

    // Tab bar
    NSRect tbFrame = NSMakeRect(0,
                                win_h - (CGFloat)TAB_BAR_HEIGHT_PX,
                                win_w,
                                (CGFloat)TAB_BAR_HEIGHT_PX);
    s_tabs = [[XCMTabBarView alloc] initWithFrame:tbFrame];
    s_tabs.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [cv addSubview:s_tabs];

    // Toolbar panel (WKWebView)
    {
        CGFloat ph = (CGFloat)CHROME_HEIGHT_PX;
        CGFloat sx = s_window.frame.origin.x;
        CGFloat sy = s_window.frame.origin.y + win_h - (CGFloat)TOTAL_CHROME_TOP;
        NSRect panelFrame = NSMakeRect(sx, sy, win_w, ph);

        s_toolbar_panel = [[XCMToolbarPanel alloc]
            initWithContentRect:panelFrame
                      styleMask:NSWindowStyleMaskBorderless |
                                NSWindowStyleMaskNonactivatingPanel
                        backing:NSBackingStoreBuffered
                          defer:NO];
        s_toolbar_panel.opaque          = YES;
        s_toolbar_panel.backgroundColor = cSurface();
        s_toolbar_panel.hasShadow       = NO;
        [s_toolbar_panel setAppearance:
            [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        [s_toolbar_panel setReleasedWhenClosed:NO];

        // WKWebView configuration with JS bridge
        WKWebViewConfiguration* cfg = [[WKWebViewConfiguration alloc] init];
        cfg.suppressesIncrementalRendering = NO;

        s_bridge = [[XCMBridgeHandler alloc] init];
        [cfg.userContentController addScriptMessageHandler:s_bridge name:@"xcmBridge"];

        // Disable all context menus and selection in the toolbar
        NSString* lockdown =
            @"document.addEventListener('contextmenu',function(e){e.preventDefault();});"
             "document.addEventListener('selectstart',function(e){e.preventDefault();});";
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

        // Transparent background so cSurface() from the panel shows until first paint
        [s_toolbar_wv setValue:@NO forKey:@"drawsBackground"];

        s_tb_nav = [[XCMToolbarNavDelegate alloc] init];
        s_toolbar_wv.navigationDelegate = s_tb_nav;

        [s_toolbar_panel.contentView addSubview:s_toolbar_wv];
        [s_window addChildWindow:s_toolbar_panel ordered:NSWindowAbove];

        // Load toolbar.html
        NSURL* url = toolbarHtmlURL();
        if (url) {
            [s_toolbar_wv loadFileURL:url
              allowingReadAccessToURL:url.URLByDeletingLastPathComponent];
        } else {
            // Fallback: inline minimal HTML so something renders
            NSString* fallback =
                @"<html><body style='background:#101820;color:#e6e9f5;"
                 "font:14px -apple-system;display:flex;align-items:center;"
                 "height:44px;padding:0 8px'>toolbar.html not found</body></html>";
            [s_toolbar_wv loadHTMLString:fallback baseURL:nil];
        }
    }

    // Status bar
    NSRect stFrame = NSMakeRect(0, 0, win_w, (CGFloat)STATUS_HEIGHT_PX);
    s_status = [[XCMStatusView alloc] initWithFrame:stFrame];
    s_status.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
    [cv addSubview:s_status];

    fprintf(stderr, "[chrome] native chrome created w=%.0f h=%.0f\n", win_w, win_h);

    // ── Re-key guard ─────────────────────────────────────────────────────
    // WKWebView focus mechanics can leave the GLFW window non-key.  Intercept
    // every left-mouseDown inside the main window and restore key status before
    // AppKit routes the event to subviews.  This gives single-click behaviour
    // on the tab bar, toolbar panel, and drag bar.
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                          handler:^NSEvent*(NSEvent* ev) {
        if (ev.window == s_window && ![s_window isKeyWindow]) {
            [s_window makeKeyAndOrderFront:nil];
        }
        return ev;
    }];
}

int native_chrome_update(AppState* st) {
    if (!s_tabs || !s_toolbar_panel || !s_status) return TOTAL_CHROME_TOP;

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

    [s_status syncState:st];

    // Redraw tab bar when tab list changes
    static size_t s_last_n   = 0;
    static int    s_last_act = -1;
    bool changed = (st->tabs.size() != s_last_n || st->active_tab != s_last_act);
    if (changed) {
        s_last_n   = st->tabs.size();
        s_last_act = st->active_tab;
        [s_tabs setNeedsDisplay:YES];
    }
    [s_tabs setNeedsDisplay:YES];

    return TOTAL_CHROME_TOP;
}

int  native_chrome_status_h() { return STATUS_HEIGHT_PX; }

bool native_chrome_has_hover() {
    if (!s_window) return false;
    // Also check if the toolbar panel is key (URL field focused)
    if (s_toolbar_panel && [s_toolbar_panel isKeyWindow]) return true;
    NSPoint mp    = [s_window mouseLocationOutsideOfEventStream];
    CGFloat win_h = s_window.contentView.bounds.size.height;
    CGFloat chrome_bottom = win_h - (CGFloat)TOTAL_CHROME_TOP;
    bool in_top    = (mp.y >= chrome_bottom);
    bool in_status = (mp.y <= (CGFloat)STATUS_HEIGHT_PX);
    return in_top || in_status;
}

void native_chrome_resize(int win_w, int win_h) {
    if (!s_tabs || !s_toolbar_panel || !s_status) return;

    s_panel_win_w = win_w;
    s_panel_win_h = win_h;

    // Also close any open drawer when the window resizes
    if (s_extra_h > 0 && s_toolbar_wv)
        [s_toolbar_wv evaluateJavaScript:@"closeDrawerSilent&&closeDrawerSilent()" completionHandler:nil];
    s_extra_h = 0.0f;

    CGFloat sx = s_window.frame.origin.x;
    CGFloat sy = s_window.frame.origin.y + (CGFloat)win_h - (CGFloat)TOTAL_CHROME_TOP;
    NSRect pf  = NSMakeRect(sx, sy, (CGFloat)win_w, (CGFloat)CHROME_HEIGHT_PX);
    [s_toolbar_panel setFrame:pf display:NO];

    [s_tabs   setNeedsDisplay:YES];
    [s_status setNeedsDisplay:YES];
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
    [s_tabs removeFromSuperview]; s_tabs = nil;
    if (s_toolbar_panel) {
        [s_window removeChildWindow:s_toolbar_panel];
        [s_toolbar_panel close];
        s_toolbar_panel = nil;
    }
    s_toolbar_wv = nil;
    s_bridge     = nil;
    s_tb_nav     = nil;
    [s_status removeFromSuperview]; s_status = nil;
}

