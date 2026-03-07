// main_menu.mm -- NSApplication menu bar + Tools menu action handler.
//
// build_app_menubar() must be called AFTER glfwInit/glfwCreateWindow because
// GLFW replaces the NSApp main menu during its Cocoa initialisation.  Calling
// it afterwards ensures our version is the last one installed.

#include "main_funcs.h"

// ── Tools menu action target ──────────────────────────────────────────────
// Lightweight ObjC object wired to Tools menu items.
// Lives in this file so it can reach g_state, g_php_port, etc.

@interface XCMMenuActions : NSObject
- (void)openPDFSign:(id)sender;
@end

@implementation XCMMenuActions
- (void)openPDFSign:(id)sender {
    dispatch_async(dispatch_get_main_queue(), ^{
        std::string url = "http://127.0.0.1:"
                        + std::to_string(g_php_port)
                        + "/pdf-sign/";
        int idx = g_state.new_tab(url);
        g_state.tabs[(size_t)idx].wv_handle =
            webview_create(g_state.tabs[(size_t)idx].id, url);
        reposition_webviews(g_prev_top, g_prev_bot,
                            g_state.win_w, g_state.win_h);
    });
}
@end

// ── Menu bar builder ──────────────────────────────────────────────────────

void build_app_menubar() {
    NSMenu* menubar = [[NSMenu alloc] init];

    // Application menu
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menubar addItem:appItem];
    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit"
                      action:@selector(terminate:)
               keyEquivalent:@"q"];
    [appItem setSubmenu:appMenu];

    // Edit menu -- gives WKWebView a proper responder-chain target for copy/
    // paste/cut/select-all so Cmd+C etc. work in web text inputs.
    NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit"
                                                      action:nil
                                               keyEquivalent:@""];
    [menubar addItem:editItem];
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Undo"  action:@selector(undo:)  keyEquivalent:@"z"];
    [editMenu addItemWithTitle:@"Redo"  action:@selector(redo:)  keyEquivalent:@"Z"];
    [editMenu addItem:[NSMenuItem separatorItem]];
    [editMenu addItemWithTitle:@"Cut"        action:@selector(cut:)       keyEquivalent:@"x"];
    [editMenu addItemWithTitle:@"Copy"       action:@selector(copy:)      keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Paste"      action:@selector(paste:)     keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
    [editItem setSubmenu:editMenu];

    // Tools menu
    static XCMMenuActions* xcmMenuActions = [[XCMMenuActions alloc] init];
    NSMenuItem* toolsItem = [[NSMenuItem alloc] initWithTitle:@"Tools"
                                                       action:nil
                                                keyEquivalent:@""];
    [menubar addItem:toolsItem];
    NSMenu* toolsMenu = [[NSMenu alloc] initWithTitle:@"Tools"];
    NSMenuItem* pdfSignItem = [toolsMenu
        addItemWithTitle:@"PDF Fill & Sign"
                 action:@selector(openPDFSign:)
          keyEquivalent:@"p"];
    pdfSignItem.target = xcmMenuActions;
    pdfSignItem.keyEquivalentModifierMask =
        NSEventModifierFlagCommand | NSEventModifierFlagShift;
    [toolsItem setSubmenu:toolsMenu];

    [NSApp setMainMenu:menubar];
}
