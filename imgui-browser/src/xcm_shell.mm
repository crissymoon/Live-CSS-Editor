// xcm_shell.mm -- Unified action shell for XCM browser.
//
// Intercepts at every layer macOS and WKWebView restrict in an embedded
// browser: right-click context menus, blob/data URL downloads, image copy,
// image save, and http(s) a[download] navigation.
//
// One WKScriptMessageHandler (name: "xcmShell") receives all events from the
// injected JS.  The handler dispatches to typed action methods, logs every
// step, and shows consistently-themed UI panels (NSAppearanceNameDarkAqua
// with #00FFFF accent text).
//
// ARC enabled via -fobjc-arc.

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "xcm_shell.h"
#include <string>

// ── Theme colours ─────────────────────────────────────────────────────
#define XCM_COLOR_BG   [NSColor colorWithRed:0.047f green:0.047f blue:0.063f alpha:1.0f]
#define XCM_COLOR_CYAN [NSColor colorWithRed:0.0f   green:1.0f   blue:1.0f   alpha:1.0f]
#define XCM_COLOR_DIM  [NSColor colorWithRed:0.0f   green:0.7f   blue:0.7f   alpha:1.0f]
#define XCM_FONT_MONO  [NSFont monospacedSystemFontOfSize:12.0f weight:NSFontWeightRegular]
#define XCM_FONT_SMALL [NSFont monospacedSystemFontOfSize:10.0f weight:NSFontWeightMedium]

// ── Injected JavaScript ───────────────────────────────────────────────
// Runs at document-start in every frame.  Registers:
//   * window.addEventListener('contextmenu', ..., true)  -- replaces native menu
//   * window.addEventListener('click', ..., true)        -- intercepts a[download]
//   * window.xcmFetchImage(url, action, filename)         -- called by native
//     to fetch image data and post back via xcmShell
static NSString* const JS_SHELL_INIT =
// ── context-menu interceptor ─────────────────────────────────────────
@"(function(){"
"'use strict';"
// Register early in capture phase so page scripts cannot block us.
"window.addEventListener('contextmenu',function(e){"
"  try{"
"    var info={type:'page',"
"              pageUrl:window.location.href,"
"              pageTitle:document.title};"
"    var el=e.target,img=null,a=null;"
"    var cur=el;"
"    while(cur&&cur!==document.body){"
"      if(!img&&cur.tagName==='IMG') img=cur;"
"      if(!a&&cur.tagName==='A'&&cur.href) a=cur;"
"      cur=cur.parentElement;"
"    }"
"    if(img){"
"      info.type='image';"
"      info.src=img.currentSrc||img.src||'';"
"      info.alt=img.alt||'';"
"    }"
"    if(a){"
"      info.linkUrl=a.href||'';"
"      info.linkText=(a.textContent||'').trim().slice(0,200);"
"      if(!img) info.type='link';"
"    }"
"    var sel=window.getSelection&&window.getSelection();"
"    if(sel&&sel.toString().trim())"
"      info.selection=sel.toString().trim().slice(0,2000);"
"    window.webkit.messageHandlers.xcmShell.postMessage("
"      JSON.stringify({action:'contextmenu',data:info}));"
"  }catch(ex){"
"    try{window.webkit.messageHandlers.xcmShell.postMessage("
"      JSON.stringify({action:'contextmenu',data:"
"        {type:'page',pageUrl:window.location.href,pageTitle:document.title}}"
"    ));}catch(ex2){}"
"  }"
"  e.preventDefault();"
"  e.stopImmediatePropagation();"
"  return false;"
"},true);"

// ── blob/data download interceptor ───────────────────────────────────
"window.addEventListener('click',function(e){"
"  var el=e.target;"
"  while(el&&el.tagName!=='A') el=el.parentElement;"
"  if(!el||el.tagName!=='A') return;"
"  var href=el.href||'';"
"  var dl=el.getAttribute('download');"
"  if(dl===null) return;" // no download attribute
"  if(href.startsWith('blob:')){"
"    e.preventDefault();"
"    xcmHandleBlob(href,dl||'download');"
"  } else if(href.startsWith('data:')){"
"    e.preventDefault();"
"    try{"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:'dataDownload',data:{src:href,filename:dl||'download'}}));"
"    }catch(ex){}"
"  }"
"},true);"

// ── blob reader ───────────────────────────────────────────────────────
"function xcmHandleBlob(href,filename){"
"  fetch(href)"
"    .then(function(r){"
"      var ct=r.headers.get('content-type')||'';"
"      return r.arrayBuffer().then(function(b){return{buf:b,ct:ct};});"
"    })"
"    .then(function(res){"
"      var bytes=new Uint8Array(res.buf);"
"      var str='';"
"      var C=4096;"
"      for(var i=0;i<bytes.length;i+=C)"
"        str+=String.fromCharCode.apply(null,bytes.subarray(i,Math.min(i+C,bytes.length)));"
"      var b64=btoa(str);"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:'blobDownload',"
"          data:{b64:b64,mimeType:res.ct,filename:filename}}));"
"    })"
"    .catch(function(err){"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:'fetchError',"
"          data:{url:href,error:String(err)}}));"
"    });"
"}"

// ── image fetcher (called by native via evaluateJavaScript) ───────────
"window.xcmFetchImage=function(url,action,filename){"
"  fetch(url,{credentials:'include',mode:'cors'})"
"    .then(function(r){"
"      var ct=r.headers.get('content-type')||'image/jpeg';"
"      return r.arrayBuffer().then(function(b){return{buf:b,ct:ct};});"
"    })"
"    .then(function(res){"
"      var bytes=new Uint8Array(res.buf);"
"      var str='';"
"      var C=4096;"
"      for(var i=0;i<bytes.length;i+=C)"
"        str+=String.fromCharCode.apply(null,bytes.subarray(i,Math.min(i+C,bytes.length)));"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:action,"
"          data:{b64:btoa(str),mimeType:res.ct,filename:filename}}));"
"    })"
"    .catch(function(err){"
"      xcmFetchImageCanvas(url,action,filename);"
"    });"
"};"

// ── canvas fallback for cross-origin images ───────────────────────────
"function xcmFetchImageCanvas(url,action,filename){"
"  var img=new Image();"
"  img.crossOrigin='anonymous';"
"  img.onload=function(){"
"    try{"
"      var c=document.createElement('canvas');"
"      c.width=img.naturalWidth;c.height=img.naturalHeight;"
"      var ctx=c.getContext('2d');"
"      ctx.drawImage(img,0,0);"
"      var durl=c.toDataURL('image/png');"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:action,"
"          data:{dataURL:durl,filename:filename}}));"
"    }catch(ex){"
"      window.webkit.messageHandlers.xcmShell.postMessage("
"        JSON.stringify({action:'fetchError',"
"          data:{url:url,error:String(ex)}}));"
"    }"
"  };"
"  img.onerror=function(){"
"    window.webkit.messageHandlers.xcmShell.postMessage("
"      JSON.stringify({action:'fetchError',"
"        data:{url:url,error:'IMG_LOAD_FAILED'}}));"
"  };"
"  img.src=url;"
"}"
"})();";

// ── XCMShell ──────────────────────────────────────────────────────────

@interface XCMShell : NSObject <WKScriptMessageHandler>
@property (nonatomic, weak)   WKWebView*   activeWV;
@property (nonatomic, assign) int          activeTabId;
@property (nonatomic, strong) NSDictionary* lastCtx;      // latest contextmenu info dict
@property (nonatomic, strong) NSURL*        pendingSaveURL;// target for pending imageSave
@property (nonatomic, strong) NSPanel*      hudPanel;
@property (nonatomic, strong) NSTextField*  hudLabel;
+ (instancetype)shared;
@end

static XCMShell*  s_shell  = nil;
static NSWindow*  s_shell_window = nil;
static AppState*  s_shell_state  = nullptr;

@implementation XCMShell

+ (instancetype)shared {
    if (!s_shell) s_shell = [[XCMShell alloc] init];
    return s_shell;
}

// ── WKScriptMessageHandler ────────────────────────────────────────────

- (void)userContentController:(WKUserContentController*)ucc
      didReceiveScriptMessage:(WKScriptMessage*)msg {
    if (![msg.body isKindOfClass:[NSString class]]) return;
    NSString* body = (NSString*)msg.body;
    NSError* err = nil;
    NSDictionary* root = [NSJSONSerialization JSONObjectWithData:
        [body dataUsingEncoding:NSUTF8StringEncoding]
        options:0 error:&err];
    if (!root || err) {
        fprintf(stderr, "[xcm-shell] bad message JSON: %s\n",
                body.UTF8String ?: "(nil)");
        return;
    }
    NSString* action = root[@"action"] ?: @"";
    NSDictionary* data  = [root[@"data"] isKindOfClass:[NSDictionary class]]
                          ? (NSDictionary*)root[@"data"] : @{};
    fprintf(stderr, "[xcm-shell] action=%s\n", action.UTF8String);

    if ([action isEqualToString:@"contextmenu"])   { [self handleContextMenu:data]; return; }
    if ([action isEqualToString:@"blobDownload"])   { [self handleBlobDownload:data]; return; }
    if ([action isEqualToString:@"dataDownload"])   { [self handleDataDownload:data]; return; }
    if ([action isEqualToString:@"imageSave"])      { [self handleImageData:data save:YES]; return; }
    if ([action isEqualToString:@"imageCopy"])      { [self handleImageData:data save:NO];  return; }
    if ([action isEqualToString:@"fetchError"])     {
        NSString* u = data[@"url"] ?: @"?";
        NSString* e = data[@"error"] ?: @"?";
        fprintf(stderr, "[xcm-shell] fetch error: url=%s err=%s\n",
                u.UTF8String, e.UTF8String);
        [self hideHUD];
        return;
    }
    fprintf(stderr, "[xcm-shell] unknown action: %s\n", action.UTF8String);
}

// ── Context menu ──────────────────────────────────────────────────────

- (void)handleContextMenu:(NSDictionary*)info {
    self.lastCtx = info;
    NSString* type = info[@"type"] ?: @"page";

    NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];
    menu.autoenablesItems = NO;
    menu.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];

    if ([type isEqualToString:@"image"]) {
        [menu addItem:[self sectionItem:@"IMAGE"]];
        [menu addItem:[self item:@"Save Image As..."  sel:@selector(actionSaveImage:)]];
        [menu addItem:[self item:@"Copy Image"        sel:@selector(actionCopyImage:)]];
        [menu addItem:[self item:@"Copy Image URL"    sel:@selector(actionCopyImageURL:)]];
        [menu addItem:[self item:@"Open Image in New Tab" sel:@selector(actionOpenImageTab:)]];
        if (info[@"linkUrl"] && [info[@"linkUrl"] length] > 0) {
            [menu addItem:[NSMenuItem separatorItem]];
            [menu addItem:[self sectionItem:@"LINK"]];
            [menu addItem:[self item:@"Open Link"         sel:@selector(actionOpenLink:)]];
            [menu addItem:[self item:@"Open in New Tab"   sel:@selector(actionOpenLinkNewTab:)]];
            [menu addItem:[self item:@"Copy Link URL"     sel:@selector(actionCopyLinkURL:)]];
        }
    } else if ([type isEqualToString:@"link"]) {
        [menu addItem:[self sectionItem:@"LINK"]];
        [menu addItem:[self item:@"Open Link"              sel:@selector(actionOpenLink:)]];
        [menu addItem:[self item:@"Open in New Tab"        sel:@selector(actionOpenLinkNewTab:)]];
        [menu addItem:[self item:@"Download Linked File"   sel:@selector(actionDownloadLink:)]];
        [menu addItem:[NSMenuItem separatorItem]];
        [menu addItem:[self item:@"Copy Link URL"          sel:@selector(actionCopyLinkURL:)]];
        if (info[@"linkText"] && [info[@"linkText"] length]) {
            [menu addItem:[self item:@"Copy Link Text"     sel:@selector(actionCopyLinkText:)]];
        }
    } else {
        // page context
        if (info[@"selection"] && [info[@"selection"] length]) {
            [menu addItem:[self sectionItem:@"SELECTION"]];
            [menu addItem:[self item:@"Copy"               sel:@selector(actionCopySelection:)]];
            [menu addItem:[NSMenuItem separatorItem]];
        }
        [menu addItem:[self sectionItem:@"PAGE"]];
        [menu addItem:[self item:@"Select All"             sel:@selector(actionSelectAll:)]];
        [menu addItem:[self item:@"Save Page As..."        sel:@selector(actionSavePage:)]];
        [menu addItem:[self item:@"Reload"                 sel:@selector(actionReload:)]];
    }

    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:[self item:@"Inspect Element"            sel:@selector(actionInspect:)]];

    // Pop up at current mouse location (screen coords, Y=0 at bottom)
    NSPoint pt = [NSEvent mouseLocation];
    [menu popUpMenuPositioningItem:nil atLocation:pt inView:nil];
}

// ── Blob download ─────────────────────────────────────────────────────

- (void)handleBlobDownload:(NSDictionary*)data {
    NSString* b64      = data[@"b64"]      ?: @"";
    NSString* filename = data[@"filename"] ?: @"download";
    NSString* mimeType = data[@"mimeType"] ?: @"application/octet-stream";

    NSData* raw = [[NSData alloc] initWithBase64EncodedString:b64
                   options:NSDataBase64DecodingIgnoreUnknownCharacters];
    if (!raw) {
        fprintf(stderr, "[xcm-shell] blob: base64 decode failed\n");
        return;
    }
    [self saveData:raw suggestedName:filename mimeType:mimeType];
}

// ── Data URL download ─────────────────────────────────────────────────

- (void)handleDataDownload:(NSDictionary*)data {
    NSString* src      = data[@"src"]      ?: @"";
    NSString* filename = data[@"filename"] ?: @"download";
    if (!src.length) return;

    NSString* mimeType = @"application/octet-stream";
    NSData*   raw      = [self decodeDataURL:src mimeType:&mimeType];
    if (!raw) {
        fprintf(stderr, "[xcm-shell] data-url: decode failed\n");
        return;
    }
    // Ensure filename has an extension
    if (![filename containsString:@"."]) {
        filename = [filename stringByAppendingPathExtension:
                    [self extensionForMime:mimeType]];
    }
    [self saveData:raw suggestedName:filename mimeType:mimeType];
}

// ── Image data (from JS fetch or canvas fallback) ─────────────────────

- (void)handleImageData:(NSDictionary*)data save:(BOOL)saving {
    [self hideHUD];

    NSData* raw      = nil;
    NSString* mime   = data[@"mimeType"] ?: @"image/jpeg";
    NSString* fname  = data[@"filename"] ?: @"image";

    if (data[@"b64"]) {
        raw = [[NSData alloc] initWithBase64EncodedString:data[@"b64"]
               options:NSDataBase64DecodingIgnoreUnknownCharacters];
    } else if (data[@"dataURL"]) {
        NSString* m = nil;
        raw = [self decodeDataURL:data[@"dataURL"] mimeType:&m];
        if (m) mime = m;
    }
    if (!raw) {
        fprintf(stderr, "[xcm-shell] imageData: no usable payload\n");
        return;
    }

    if (saving) {
        // Use the previously chosen save destination if available,
        // otherwise show NSSavePanel now.
        NSURL* dest = self.pendingSaveURL;
        self.pendingSaveURL = nil;
        if (!dest) {
            NSString* ext = [self extensionForMime:mime];
            if (![fname containsString:@"."]) fname = [fname stringByAppendingPathExtension:ext];
            dest = [self runSavePanelForFilename:fname];
            if (!dest) return;
        }
        NSError* writeErr = nil;
        [raw writeToURL:dest options:NSDataWritingAtomic error:&writeErr];
        if (writeErr) {
            fprintf(stderr, "[xcm-shell] write error: %s\n",
                    writeErr.localizedDescription.UTF8String);
        } else {
            fprintf(stderr, "[xcm-shell] saved image: %s\n", dest.path.UTF8String);
            [[NSWorkspace sharedWorkspace] selectFile:dest.path
                          inFileViewerRootedAtPath:@""];
        }
    } else {
        // Copy to clipboard
        NSImage* img = [[NSImage alloc] initWithData:raw];
        if (!img) {
            fprintf(stderr, "[xcm-shell] imageCopy: could not create NSImage\n");
            return;
        }
        NSPasteboard* pb = NSPasteboard.generalPasteboard;
        [pb clearContents];
        [pb writeObjects:@[img]];
        fprintf(stderr, "[xcm-shell] image copied to clipboard (%lu bytes)\n",
                (unsigned long)raw.length);
        [self showHUDMessage:@"Image copied to clipboard"];
    }
}

// ── Menu actions ──────────────────────────────────────────────────────

- (void)actionSaveImage:(id)sender {
    NSString* src = self.lastCtx[@"src"] ?: @"";
    if (!src.length) return;
    // Derive suggested filename from the URL's last path component.
    NSString* fname = @"image";
    NSURL* srcURL = [NSURL URLWithString:src];
    if (srcURL.lastPathComponent.length) fname = srcURL.lastPathComponent;
    // Show the save panel BEFORE the async JS fetch so the user sees the dialog
    // immediately.  Store destination; write occurs in handleImageData:save:.
    NSString* ext = [self extensionForMime:[self guessMimeFromURL:src]];
    if (![fname containsString:@"."]) fname = [fname stringByAppendingPathExtension:ext];
    NSURL* dest = [self runSavePanelForFilename:fname];
    if (!dest) return;
    self.pendingSaveURL = dest;
    [self showHUDMessage:@"Downloading image..."];
    NSString* jsSrc  = [self jsonQuote:src];
    NSString* jsFname = [self jsonQuote:fname];
    NSString* js = [NSString stringWithFormat:
        @"typeof xcmFetchImage==='function'&&xcmFetchImage(%@,'imageSave',%@)",
        jsSrc, jsFname];
    [self evalJS:js];
}

- (void)actionCopyImage:(id)sender {
    NSString* src = self.lastCtx[@"src"] ?: @"";
    if (!src.length) return;
    [self showHUDMessage:@"Copying image..."];
    NSString* jsSrc  = [self jsonQuote:src];
    NSString* jsFname = [self jsonQuote:@"image"];
    NSString* js = [NSString stringWithFormat:
        @"typeof xcmFetchImage==='function'&&xcmFetchImage(%@,'imageCopy',%@)",
        jsSrc, jsFname];
    [self evalJS:js];
}

- (void)actionCopyImageURL:(id)sender {
    NSString* src = self.lastCtx[@"src"] ?: @"";
    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:(src ?: @"") forType:NSPasteboardTypeString];
    fprintf(stderr, "[xcm-shell] copied image URL: %s\n", src.UTF8String ?: "");
}

- (void)actionOpenImageTab:(id)sender {
    NSString* src = self.lastCtx[@"src"] ?: @"";
    if (src.length && s_shell_state)
        s_shell_state->push_nav(-2, src.UTF8String);
}

- (void)actionOpenLink:(id)sender {
    NSString* url = self.lastCtx[@"linkUrl"] ?: @"";
    if (url.length && s_shell_state)
        s_shell_state->push_nav(-1, url.UTF8String);
}

- (void)actionOpenLinkNewTab:(id)sender {
    NSString* url = self.lastCtx[@"linkUrl"] ?: @"";
    if (url.length && s_shell_state)
        s_shell_state->push_nav(-2, url.UTF8String);
}

- (void)actionDownloadLink:(id)sender {
    NSString* urlStr = self.lastCtx[@"linkUrl"] ?: @"";
    if (!urlStr.length) return;
    // Navigate the current tab to the URL; decidePolicyForNavigationResponse
    // will trigger WKNavigationResponsePolicyDownload for non-showable types.
    // For URLs that would render normally in WebKit, trigger via URLSession.
    if (s_shell_state) s_shell_state->push_nav(-1, urlStr.UTF8String);
}

- (void)actionCopyLinkURL:(id)sender {
    NSString* url = self.lastCtx[@"linkUrl"] ?: @"";
    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:(url ?: @"") forType:NSPasteboardTypeString];
}

- (void)actionCopyLinkText:(id)sender {
    NSString* text = self.lastCtx[@"linkText"] ?: @"";
    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:(text ?: @"") forType:NSPasteboardTypeString];
}

- (void)actionCopySelection:(id)sender {
    NSString* sel = self.lastCtx[@"selection"] ?: @"";
    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:(sel ?: @"") forType:NSPasteboardTypeString];
}

- (void)actionSelectAll:(id)sender {
    if (self.activeWV)
        [NSApp sendAction:@selector(selectAll:) to:self.activeWV from:nil];
}

- (void)actionSavePage:(id)sender {
    NSString* title = self.lastCtx[@"pageTitle"] ?: @"page";
    // Sanitise title for use as filename
    NSMutableString* safe = [title mutableCopy];
    for (NSString* bad in @[@"/",@"\\",@":",@"*",@"?",@"\"",@"<",@">",@"|"])
        [safe replaceOccurrencesOfString:bad withString:@"-" options:0
              range:NSMakeRange(0, safe.length)];
    NSString* fname = [safe stringByAppendingPathExtension:@"html"];
    NSURL* dest = [self runSavePanelForFilename:fname];
    if (!dest) return;

    if (!self.activeWV) return;
    [self.activeWV evaluateJavaScript:@"document.documentElement.outerHTML"
                    completionHandler:^(id res, NSError* e) {
        if (e || ![res isKindOfClass:[NSString class]]) {
            fprintf(stderr, "[xcm-shell] savePage JS error: %s\n",
                    e.localizedDescription.UTF8String ?: "(nil)");
            return;
        }
        NSString* html = (NSString*)res;
        NSError* we = nil;
        [html writeToURL:dest atomically:YES encoding:NSUTF8StringEncoding error:&we];
        if (we) {
            fprintf(stderr, "[xcm-shell] savePage write error: %s\n",
                    we.localizedDescription.UTF8String);
        } else {
            fprintf(stderr, "[xcm-shell] page saved: %s\n", dest.path.UTF8String);
            [[NSWorkspace sharedWorkspace] selectFile:dest.path
                          inFileViewerRootedAtPath:@""];
        }
    }];
}

- (void)actionReload:(id)sender {
    if (self.activeWV) [self.activeWV reload];
}

- (void)actionInspect:(id)sender {
    if (!self.activeWV) return;
    @try {
        id inspector = [self.activeWV valueForKey:@"_inspector"];
        if (inspector && [inspector respondsToSelector:NSSelectorFromString(@"show:")]) {
            NSMethodSignature* sig = [inspector methodSignatureForSelector:
                                      NSSelectorFromString(@"show:")];
            NSInvocation* inv = [NSInvocation invocationWithMethodSignature:sig];
            [inv setTarget:inspector];
            [inv setSelector:NSSelectorFromString(@"show:")];
            id arg = inspector;
            [inv setArgument:&arg atIndex:2];
            [inv invoke];
        }
    } @catch (NSException*) {}
}

// ── Save panel helper ─────────────────────────────────────────────────

- (NSURL*)runSavePanelForFilename:(NSString*)name {
    NSSavePanel* sp = [NSSavePanel savePanel];
    sp.appearance          = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    sp.nameFieldStringValue = name ?: @"download";
    sp.canCreateDirectories = YES;
    sp.title = @"XCM Shell -- Save File";
    NSURL* dlDir = [[NSFileManager defaultManager]
                    URLForDirectory:NSDownloadsDirectory
                           inDomain:NSUserDomainMask
                  appropriateForURL:nil
                             create:NO error:nil];
    if (dlDir) sp.directoryURL = dlDir;
    NSModalResponse r = [sp runModal];
    return (r == NSModalResponseOK) ? sp.URL : nil;
}

// ── Generic data saver (blob / data-URL downloads) ────────────────────

- (void)saveData:(NSData*)data
   suggestedName:(NSString*)name
        mimeType:(NSString*)mime {
    NSURL* dest = [self runSavePanelForFilename:name];
    if (!dest) return;
    NSError* e = nil;
    [data writeToURL:dest options:NSDataWritingAtomic error:&e];
    if (e) {
        fprintf(stderr, "[xcm-shell] saveData write error: %s\n",
                e.localizedDescription.UTF8String);
    } else {
        fprintf(stderr, "[xcm-shell] saved: %s (%lu bytes)\n",
                dest.path.UTF8String, (unsigned long)data.length);
        [[NSWorkspace sharedWorkspace] selectFile:dest.path inFileViewerRootedAtPath:@""];
    }
}

// ── HUD status panel (dark/cyan themed) ──────────────────────────────

- (void)showHUDMessage:(NSString*)msg {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!self.hudPanel) {
            NSPanel* p = [[NSPanel alloc]
                initWithContentRect:NSMakeRect(0, 0, 320, 52)
                          styleMask:NSWindowStyleMaskHUDWindow |
                                    NSWindowStyleMaskTitled   |
                                    NSWindowStyleMaskUtilityWindow
                            backing:NSBackingStoreBuffered
                              defer:NO];
            p.title = @"XCM Shell";
            p.level = NSFloatingWindowLevel;
            p.releasedWhenClosed = NO;
            p.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
            p.backgroundColor = XCM_COLOR_BG;

            NSProgressIndicator* spin = [[NSProgressIndicator alloc]
                initWithFrame:NSMakeRect(12, 16, 20, 20)];
            spin.style = NSProgressIndicatorStyleSpinning;
            spin.controlSize = NSControlSizeSmall;
            [spin startAnimation:nil];
            [p.contentView addSubview:spin];

            NSTextField* lbl = [[NSTextField alloc]
                initWithFrame:NSMakeRect(40, 16, 270, 20)];
            lbl.bezeled = NO;
            lbl.drawsBackground = NO;
            lbl.editable = NO;
            lbl.textColor = XCM_COLOR_CYAN;
            lbl.font = XCM_FONT_MONO;
            [p.contentView addSubview:lbl];

            self.hudPanel = p;
            self.hudLabel = lbl;
        }
        self.hudLabel.stringValue = msg ?: @"";
        if (s_shell_window) [self.hudPanel orderFront:s_shell_window];
        [self.hudPanel center];
    });
}

- (void)hideHUD {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.hudPanel orderOut:nil];
    });
}

// ── Menu builder helpers ──────────────────────────────────────────────

- (NSMenuItem*)sectionItem:(NSString*)text {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:text action:nil keyEquivalent:@""];
    NSMutableAttributedString* at = [[NSMutableAttributedString alloc] initWithString:text];
    [at addAttribute:NSForegroundColorAttributeName
               value:XCM_COLOR_DIM
               range:NSMakeRange(0, text.length)];
    [at addAttribute:NSFontAttributeName
               value:XCM_FONT_SMALL
               range:NSMakeRange(0, text.length)];
    item.attributedTitle = at;
    item.enabled = NO;
    return item;
}

- (NSMenuItem*)item:(NSString*)title sel:(SEL)action {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                   action:action
                                            keyEquivalent:@""];
    item.target  = self;
    item.enabled = YES;
    return item;
}

// ── Utilities ─────────────────────────────────────────────────────────

- (void)evalJS:(NSString*)js {
    if (!self.activeWV || !js.length) return;
    WKWebView* wv = self.activeWV;
    dispatch_async(dispatch_get_main_queue(), ^{
        [wv evaluateJavaScript:js completionHandler:^(id r, NSError* e) {
            if (e) fprintf(stderr, "[xcm-shell] evalJS error: %s\n",
                           e.localizedDescription.UTF8String);
        }];
    });
}

- (NSString*)jsonQuote:(NSString*)s {
    if (!s) return @"null";
    NSError* e = nil;
    NSData* d = [NSJSONSerialization dataWithJSONObject:s
                                               options:NSJSONWritingFragmentsAllowed
                                                 error:&e];
    if (!d || e) return @"null";
    return [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
}

- (NSString*)extensionForMime:(NSString*)mime {
    if (!mime.length) return @"bin";
    NSDictionary* m = @{
        @"image/jpeg":              @"jpg",
        @"image/png":               @"png",
        @"image/gif":               @"gif",
        @"image/webp":              @"webp",
        @"image/svg+xml":           @"svg",
        @"image/bmp":               @"bmp",
        @"image/tiff":              @"tiff",
        @"application/pdf":         @"pdf",
        @"application/zip":         @"zip",
        @"application/json":        @"json",
        @"text/html":               @"html",
        @"text/plain":              @"txt",
        @"text/css":                @"css",
        @"text/javascript":         @"js",
        @"video/mp4":               @"mp4",
        @"video/webm":              @"webm",
        @"audio/mpeg":              @"mp3",
        @"audio/ogg":               @"ogg",
        @"application/octet-stream":@"bin",
    };
    NSString* base = [mime componentsSeparatedByString:@";"].firstObject;
    return m[base.lowercaseString] ?: @"bin";
}

- (NSString*)guessMimeFromURL:(NSString*)url {
    NSString* ext = url.pathExtension.lowercaseString;
    NSDictionary* m = @{
        @"jpg":@"image/jpeg", @"jpeg":@"image/jpeg",
        @"png":@"image/png",  @"gif":@"image/gif",
        @"webp":@"image/webp",@"svg":@"image/svg+xml",
        @"bmp":@"image/bmp",  @"tiff":@"image/tiff",
    };
    return m[ext] ?: @"image/jpeg";
}

- (NSData*)decodeDataURL:(NSString*)dataURL mimeType:(NSString**)mimeOut {
    // Format: data:[<type>][;base64],<payload>
    if (![dataURL hasPrefix:@"data:"]) return nil;
    NSRange comma = [dataURL rangeOfString:@","];
    if (comma.location == NSNotFound) return nil;

    NSString* header  = [dataURL substringWithRange:NSMakeRange(5, comma.location - 5)];
    NSString* payload = [dataURL substringFromIndex:comma.location + 1];
    BOOL isBase64 = [header hasSuffix:@";base64"];
    if (isBase64) header = [header substringToIndex:header.length - 7];
    if (mimeOut) *mimeOut = header.length ? header : @"application/octet-stream";

    if (isBase64) {
        return [[NSData alloc] initWithBase64EncodedString:payload
                options:NSDataBase64DecodingIgnoreUnknownCharacters];
    }
    NSString* decoded = [payload stringByRemovingPercentEncoding] ?: payload;
    return [decoded dataUsingEncoding:NSUTF8StringEncoding];
}

@end

// ── Public C API ──────────────────────────────────────────────────────

void xcm_shell_install(WKUserContentController* ucc,
                       NSWindow*               window,
                       AppState*               state) {
    // Record globals (last writer wins; all tabs share the same window/state).
    s_shell_window = window;
    s_shell_state  = state;

    XCMShell* shell = [XCMShell shared];

    // Inject the JS interceptor script.
    WKUserScript* script = [[WKUserScript alloc]
        initWithSource:JS_SHELL_INIT
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:NO];
    [ucc addUserScript:script];

    // Register the message handler.  Guard against duplicate registration on
    // the same UCC instance (which can happen if xcm_shell_install is called
    // more than once for the same controller).
    @try {
        [ucc addScriptMessageHandler:shell name:@"xcmShell"];
        fprintf(stderr, "[xcm-shell] installed on UCC %p\n", (__bridge void*)ucc);
    } @catch (NSException* e) {
        fprintf(stderr, "[xcm-shell] handler already installed on UCC %p -- skipped\n",
                (__bridge void*)ucc);
    }
}

void xcm_shell_set_webview(WKWebView* wv, int tabId) {
    XCMShell* shell = [XCMShell shared];
    if (shell.activeWV == wv && shell.activeTabId == tabId) return;
    shell.activeWV    = wv;
    shell.activeTabId = tabId;
    fprintf(stderr, "[xcm-shell] active webview set to tab %d\n", tabId);
}
