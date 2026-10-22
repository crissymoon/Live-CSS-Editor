// window_chrome_mac.mm -- see window_chrome_mac.h.

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include "window_chrome_mac.h"

// Corner radius of a window that AppKit does not expose publicly.  Queried
// once per window and cached, because it never changes.
static double xcm_query_corner_radius(NSWindow* win) {
    SEL sel = NSSelectorFromString(@"_cornerRadius");
    if (!win || ![win respondsToSelector:sel]) return 0.0;

    NSMethodSignature* sig = [win methodSignatureForSelector:sel];
    if (!sig) return 0.0;

    NSInvocation* inv = [NSInvocation invocationWithMethodSignature:sig];
    [inv setTarget:win];
    [inv setSelector:sel];
    [inv invoke];

    CGFloat radius = 0.0;
    if (sig.methodReturnLength == sizeof(CGFloat))
        [inv getReturnValue:&radius];
    else if (sig.methodReturnLength == sizeof(double))
        [inv getReturnValue:&radius];

    return (double)radius;
}

double xcm_window_corner_radius(void* ns_window) {
    static void* cached_window = nullptr;
    static double cached_radius = 0.0;

    if (ns_window && ns_window == cached_window && cached_radius > 0.0)
        return cached_radius;

    double radius = xcm_query_corner_radius((__bridge NSWindow*)ns_window);
    if (radius <= 0.0) {
        // Pre-Tahoe window shape.  Only used when the OS hides the value.
        radius = 10.0;
    }
    cached_window = ns_window;
    cached_radius = radius;
    return radius;
}

void xcm_round_chrome_view(void* ns_view, void* ns_window) {
    NSView* view = (__bridge NSView*)ns_view;
    if (!view) return;

    CGFloat radius = (CGFloat)xcm_window_corner_radius(ns_window);

    view.wantsLayer            = YES;
    view.layer.cornerRadius    = radius;
    view.layer.maskedCorners   = kCALayerMinXMaxYCorner | kCALayerMaxXMaxYCorner;
    view.layer.masksToBounds   = YES;
}

void xcm_titlebar_double_click(void* ns_window) {
    NSWindow* win = (__bridge NSWindow*)ns_window;
    if (!win) return;

    NSString* action = [[NSUserDefaults standardUserDefaults]
                           stringForKey:@"AppleActionOnDoubleClick"];
    if ([action isEqualToString:@"Minimize"]) {
        [win performMiniaturize:nil];
        return;
    }
    if ([action isEqualToString:@"None"]) return;

    // "Maximize" and anything unknown: zoom, which also restores a zoomed
    // window and respects the saved frame, exactly like a title-bar click.
    if ([win isZoomable]) [win zoom:nil];
}

bool xcm_is_double_click(double timestamp, double x, double y, int click_count) {
    static double last_t = 0.0;
    static double last_x = 0.0;
    static double last_y = 0.0;

    // 3 pt of slop: the pointer always moves a hair between two clicks.
    const double max_dist_sq = 9.0;
    const double dx = x - last_x;
    const double dy = y - last_y;
    const bool   same_spot = (dx * dx + dy * dy) <= max_dist_sq;
    const bool   in_time   = (last_t > 0.0) &&
                             ((timestamp - last_t) <= [NSEvent doubleClickInterval]);

    const bool is_double = (click_count >= 2) || (in_time && same_spot);

    // A fresh registration after a double click, so a third click does not
    // chain into another zoom.
    last_t = is_double ? 0.0 : timestamp;
    last_x = x;
    last_y = y;
    return is_double;
}
