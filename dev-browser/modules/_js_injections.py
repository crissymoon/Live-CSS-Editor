"""
Injected JavaScript payloads used by browser_profile and CustomWebEnginePage.

  _SCROLL_FIX_JS        -- strips smooth-scroll behavior, injects anti-snap CSS
  _SCROLL_SPEED_JS      -- halves trackpad scroll speed on macOS
  _CONSENT_KILL_JS      -- removes cookie-consent banners
  _STEALTH_JS           -- hides webdriver / automation signals
  _VIDEO_FIX_JS         -- MSE H.264 shim, routes to native macOS VideoToolbox
  _SPECULATION_RULES_JS -- prefetch on hover for same-origin links
"""

_SCROLL_FIX_JS = r"""
(function () {
    'use strict';

    // ── 1. Patch all programmatic scroll APIs to force instant ────────────
    // Strip behavior:'smooth' from every scroll call so page JS cannot
    // stack an additional animation on top of macOS trackpad inertia.
    function _forceInstant(opts) {
        if (opts !== null && typeof opts === 'object') {
            var copy = {};
            for (var k in opts) { if (Object.prototype.hasOwnProperty.call(opts, k)) copy[k] = opts[k]; }
            copy.behavior = 'instant';
            return copy;
        }
        return opts;
    }

    // window.scroll / scrollTo / scrollBy
    ['scroll', 'scrollTo', 'scrollBy'].forEach(function (fn) {
        var orig = window[fn];
        if (typeof orig !== 'function') return;
        window[fn] = function () {
            var args = Array.prototype.slice.call(arguments);
            if (args.length === 1 && args[0] !== null && typeof args[0] === 'object') {
                args[0] = _forceInstant(args[0]);
            }
            return orig.apply(this, args);
        };
    });

    // Element.prototype.scroll / scrollTo / scrollBy / scrollIntoView
    ['scroll', 'scrollTo', 'scrollBy'].forEach(function (fn) {
        var orig = Element.prototype[fn];
        if (typeof orig !== 'function') return;
        Element.prototype[fn] = function () {
            var args = Array.prototype.slice.call(arguments);
            if (args.length === 1 && args[0] !== null && typeof args[0] === 'object') {
                args[0] = _forceInstant(args[0]);
            }
            return orig.apply(this, args);
        };
    });

    // scrollIntoView is intentionally NOT patched.
    // React Router, Next.js App Router, Turbo (Rails), and Stimulus all
    // call scrollIntoView({behavior:'smooth'}) for in-page navigation and
    // focus management.  Forcing instant breaks their intentional UX.
    // The CSS rule 'scroll-behavior: auto !important' already suppresses
    // the visual fight with macOS trackpad inertia.

    // ── 2. Inject anti-snap / anti-flash CSS as early as possible ─────────
    // scroll-snap-type: causes the viewport to snap to section boundaries.
    // scroll-behavior: causes the browser to animate even instant scrolls.
    // overscroll-behavior: set to 'contain' on html/body to prevent scroll
    //   momentum from chaining to the browser chrome (the rubber-band bounce
    //   that shows a white flash beyond the page edge). 'contain' keeps
    //   elastic overscroll within the page without leaking to the parent.
    // will-change: scroll-position on html promotes the document to its own
    //   compositing layer so the compositor thread can scroll the content
    //   independently of the main thread (eliminates main-thread repaint
    //   flashes during fast scrolling).
    function _injectScrollCss() {
        if (document.getElementById('__xcmScrollCss__')) return;
        var s = document.createElement('style');
        s.id = '__xcmScrollCss__';
        s.textContent = [
            '*, *::before, *::after {',
            '  scroll-snap-type: none !important;',
            '  scroll-snap-align: none !important;',
            '  scroll-behavior: auto !important;',
            '}',
            'html, body {',
            '  overscroll-behavior: contain !important;',
            '  will-change: scroll-position;',
            '}',
            // Stop CSS transitions/animations on transform and scroll-related
            // properties from producing mid-frame jumps during scroll.
            // Only suppress the specific properties that cause scroll fights;
            // leave opacity/color/etc. alone so the page still looks alive.
            '@keyframes __xcm_noop__ { from {} to {} }',
        ].join('\n');
        var root = document.head || document.documentElement;
        if (root) {
            root.appendChild(s);
        } else {
            // No root yet (DocumentCreation on a fresh frame) -- retry.
            document.addEventListener('DOMContentLoaded', _injectScrollCss, { once: true });
        }
    }
    _injectScrollCss();

    // Re-run on DOMContentLoaded so CSS is in place even if head was
    // recreated by a framework (Next.js, Remix, etc.).
    document.addEventListener('DOMContentLoaded', _injectScrollCss, { once: true });

    // Watch for the style being removed and re-inject immediately.
    var _cssObs = new MutationObserver(function () { _injectScrollCss(); });
    function _startCssObs() {
        var root = document.head || document.documentElement;
        if (root) {
            _cssObs.observe(root, { childList: true });
        } else {
            setTimeout(_startCssObs, 0);
        }
    }
    _startCssObs();

    // ── 3. Passively intercept wheel events at the document level ─────────
    // Some pages add non-passive wheel listeners that call preventDefault(),
    // which disables the compositor-thread fast-path for scrolling and forces
    // every wheel event through the slow main-thread round-trip.  By adding
    // our own capturing passive listener first, we ensure the compositor
    // always gets the hint that scrolling should proceed immediately.
    try {
        document.addEventListener('wheel', function(){}, { capture: true, passive: true });
        document.addEventListener('touchmove', function(){}, { capture: true, passive: true });
    } catch(e) {}
})();
"""


# ── Trackpad scroll speed reducer ───────────────────────────────────────────
# The QtWebEngine / Chromium compositor reports very large pixelDelta values
# for macOS high-res trackpads, making pages scroll far too fast.  This script
# registers a non-passive capture-phase wheel listener that cancels the native
# Chromium compositor scroll and replaces it with a scaled-down scrollBy call.
# SPEED < 1.0 = slower.  0.5 halves the native speed; adjust to taste.
_SCROLL_SPEED_JS = r"""
(function () {
    'use strict';
    var SPEED = 0.5;

    // Walk up the tree to the closest element that can actually scroll in
    // the given direction so inner overflow containers work naturally.
    function _scrollTarget(el, dx, dy) {
        while (el && el !== document.documentElement) {
            var cs = window.getComputedStyle(el);
            var oy = cs.overflowY;
            var ox = cs.overflowX;
            var mayScroll = (oy === 'auto' || oy === 'scroll' ||
                             ox === 'auto' || ox === 'scroll');
            if (mayScroll) {
                if ((Math.abs(dy) > 0 && el.scrollHeight > el.clientHeight) ||
                    (Math.abs(dx) > 0 && el.scrollWidth  > el.clientWidth)) {
                    return el;
                }
            }
            el = el.parentElement;
        }
        return window;
    }

    document.addEventListener('wheel', function (e) {
        // Only intercept trusted, pixel-mode, non-zoom wheel events.
        if (!e.isTrusted)       return;
        if (e.ctrlKey)          return;  // pinch-to-zoom -- let it pass
        if (e.deltaMode !== 0)  return;  // non-pixel mode (unexpected on macOS)

        e.preventDefault();

        var dx  = e.deltaX * SPEED;
        var dy  = e.deltaY * SPEED;
        var tgt = _scrollTarget(e.target, dx, dy);
        // scrollBy with two numeric args -- behavior forced to 'instant' by
        // the CSS overide in _SCROLL_FIX_JS so no extra animation is added.
        if (tgt === window) {
            window.scrollBy(dx, dy);
        } else {
            tgt.scrollBy(dx, dy);
        }
    }, { passive: false, capture: true });
})();
"""


# ── Cookie consent banner killer ─────────────────────────────────────────────
# Injected at DocumentReady so the DOM exists but before the user sees the page.
_CONSENT_KILL_JS = r"""
(function(){
    // CSS selectors for the most common consent overlay containers.
    var HIDE_SELECTORS = [
        // OneTrust
        '#onetrust-consent-sdk', '#onetrust-banner-sdk',
        '#onetrust-pc-sdk', '.onetrust-pc-dark-filter',
        // CookieBot
        '#CybotCookiebotDialog', '#CybotCookiebotDialogBodyUnderlay',
        // Generic cookie banners
        '#cookie-banner', '#cookie-notice', '#cookie-bar',
        '#cookie-consent', '#cookie-policy-banner',
        '#cookiebanner', '#cookieConsent', '#cookieNotice',
        '.cookie-banner', '.cookie-notice', '.cookie-bar',
        '.cookie-consent', '.cookie-popup', '.cookie-modal',
        '.cookiebanner', '.cookieConsent', '.cookieNotice',
        // GDPR generic
        '#gdpr-banner', '#gdpr-notice', '#gdpr-consent',
        '.gdpr-banner', '.gdpr-notice', '.gdpr-consent',
        // TrustArc
        '#truste-consent-track', '#truste-show-consent',
        '.truste_overlay', '.truste_box_overlay',
        // Quantcast
        '#qc-cmp2-container', '#qc-cmp2-ui',
        // Didomi
        '#didomi-host', '#didomi-notice',
        // Cookieyes
        '#cookie-law-info-bar', '.cli-modal-backdrop',
        // Complianz
        '#cmplz-cookiebanner', '.cmplz-overlay',
        // Termly
        '#termly-code-snippet-support',
        // Consentmanager
        '#cmp-container', '#cmpbox',
        // Iubenda
        '#iubenda-cs-banner',
        // Cookieinfo
        '#cookieinfo',
        // Cookieconsent (open source lib)
        '.cc-window', '.cc-banner', '.cc-overlay',
        // Borlabs
        '#BorlabsCookieBox',
        // Osano
        '.osano-cm-window',
        // Pippins
        '#cookie_notice',
        // WP Cookie Notice
        '#cookie-notice',
        // General overlays that dim the page
        '.cookie-overlay', '.consent-overlay',
        '.privacy-overlay', '.privacy-popup',
        // Usercentrics
        '#usercentrics-root',
    ];

    function removeNodes() {
        HIDE_SELECTORS.forEach(function(sel) {
            try {
                document.querySelectorAll(sel).forEach(function(el) {
                    el.remove();
                });
            } catch(e) {}
        });
        // Restore scroll lock that banners often add to <body> / <html>.
        try {
            document.body.style.removeProperty('overflow');
            document.body.style.removeProperty('position');
            document.documentElement.style.removeProperty('overflow');
        } catch(e) {}
        // Remove scroll-snap and jarring overscroll on all elements.
        try {
            var _snapStyle = document.getElementById('__xcmScrollFix__');
            if (!_snapStyle) {
                _snapStyle = document.createElement('style');
                _snapStyle.id = '__xcmScrollFix__';
                // scroll-snap-type: page/site CSS causes the jumpy "snap to
                //   section" behaviour when you scroll past a snap point.
                // overscroll-behavior: set to 'contain' so scroll momentum
                //   stays inside the page and does not chain to browser
                //   chrome (which causes the rubber-band white flash).
                // scroll-behavior: forces instant positioning even when page
                //   JS calls window.scrollTo({ behavior: 'smooth' }), which
                //   on top of macOS inertia causes a double-glide stutter.
                _snapStyle.textContent = [
                    '*, *::before, *::after {',
                    '  scroll-snap-type: none !important;',
                    '  scroll-snap-align: none !important;',
                    '  scroll-behavior: auto !important;',
                    '}',
                    'html, body {',
                    '  overscroll-behavior: contain !important;',
                    '}'
                ].join('\n');
                (document.head || document.documentElement).appendChild(_snapStyle);
            }
        } catch(e) {}
    }

    // Run immediately on DOMContentLoaded context.
    removeNodes();

    // Watch for dynamically inserted banners (React/Vue SPAs).
    var _obs = new MutationObserver(function(mutations) {
        var hit = false;
        mutations.forEach(function(m) {
            m.addedNodes.forEach(function(n) {
                if (n.nodeType !== 1) return;
                var id  = (n.id  || '').toLowerCase();
                var cls = (n.className && typeof n.className === 'string'
                           ? n.className : '').toLowerCase();
                if (/cookie|consent|gdpr|onetrust|cybot|truste|qc-cmp|didomi|borlabs|osano|cookieyes|usercentrics|iubenda|termly|cmp/.test(id + ' ' + cls)) {
                    hit = true;
                }
            });
        });
        if (hit) { setTimeout(removeNodes, 50); }
    });
    var _consentRoot = document.documentElement || document.body;
    if (_consentRoot) {
        _obs.observe(_consentRoot, {childList: true, subtree: true});
    }
})();
"""


# JavaScript injected before any page script runs to hide automation signals.
# Covers: webdriver flag, chrome object, plugins, languages, permissions,
# canvas fingerprint stability, WebGL renderer, connection info,
# screen / window metrics, iframe contentWindow consistency,
# Notification / Battery API, hardware concurrency, device memory,
# platform string, media codecs, and general iframe sandboxing.
_STEALTH_JS = r"""
(function(){
    // ── 1. navigator.webdriver ──────────────────────────────────
    Object.defineProperty(navigator, 'webdriver', {get: () => undefined});
    // Also delete it from the prototype in case anything walks the chain.
    try { delete Object.getPrototypeOf(navigator).webdriver; } catch(e){}

    // ── 2. window.chrome ────────────────────────────────────────
    if (!window.chrome) {
        window.chrome = {};
    }
    if (!window.chrome.runtime) {
        window.chrome.runtime = {
            connect: function(){},
            sendMessage: function(){},
            onMessage: {addListener: function(){}, removeListener: function(){}},
            PlatformOs: {MAC: 'mac', WIN: 'win', ANDROID: 'android',
                         CROS: 'cros', LINUX: 'linux', OPENBSD: 'openbsd'},
        };
    }
    window.chrome.loadTimes  = window.chrome.loadTimes  || function(){return {}};
    window.chrome.csi        = window.chrome.csi        || function(){return {}};
    window.chrome.app        = window.chrome.app        || {isInstalled: false};

    // ── 3. Plugins / Mime types ─────────────────────────────────
    var fakePlugins = [
        {name:'Chrome PDF Plugin',  filename:'internal-pdf-viewer',
         description:'Portable Document Format', length:1,
         0:{type:'application/x-google-chrome-pdf',suffixes:'pdf',
            description:'Portable Document Format',enabledPlugin:null}},
        {name:'Chrome PDF Viewer',  filename:'mhjfbmdgcfjbbpaeojofohoefgiehjai',
         description:'', length:1,
         0:{type:'application/pdf',suffixes:'pdf',
            description:'',enabledPlugin:null}},
        {name:'Native Client',      filename:'internal-nacl-plugin',
         description:'', length:2,
         0:{type:'application/x-nacl',suffixes:'',description:'',enabledPlugin:null},
         1:{type:'application/x-pnacl',suffixes:'',description:'',enabledPlugin:null}},
    ];
    fakePlugins.item = function(i){return this[i] || null;};
    fakePlugins.namedItem = function(n){
        for(var j=0;j<this.length;j++){if(this[j].name===n)return this[j];}return null;
    };
    fakePlugins.refresh = function(){};
    Object.defineProperty(navigator, 'plugins', {get: () => fakePlugins});
    Object.defineProperty(navigator, 'mimeTypes', {
        get: () => {
            var m = [{type:'application/pdf',suffixes:'pdf',description:'',enabledPlugin:fakePlugins[1]},
                     {type:'application/x-google-chrome-pdf',suffixes:'pdf',description:'Portable Document Format',enabledPlugin:fakePlugins[0]},
                     {type:'application/x-nacl',suffixes:'',description:'',enabledPlugin:fakePlugins[2]},
                     {type:'application/x-pnacl',suffixes:'',description:'',enabledPlugin:fakePlugins[2]}];
            m.item = function(i){return this[i]||null;};
            m.namedItem = function(n){for(var j=0;j<this.length;j++){if(this[j].type===n)return this[j];}return null;};
            return m;
        }
    });

    // ── 4. Languages ────────────────────────────────────────────
    Object.defineProperty(navigator, 'languages', {get: () => ['en-US','en']});
    Object.defineProperty(navigator, 'language',  {get: () => 'en-US'});

    // ── 5. Platform / hardware ──────────────────────────────────
    Object.defineProperty(navigator, 'platform',          {get: () => 'MacIntel'});
    Object.defineProperty(navigator, 'hardwareConcurrency',{get: () => 8});
    Object.defineProperty(navigator, 'deviceMemory',      {get: () => 8});
    Object.defineProperty(navigator, 'maxTouchPoints',    {get: () => 0});
    Object.defineProperty(navigator, 'vendor',            {get: () => 'Google Inc.'});

    // ── 6. Permissions ──────────────────────────────────────────
    if (navigator.permissions) {
        var origQuery = navigator.permissions.query.bind(navigator.permissions);
        navigator.permissions.query = function(desc) {
            if (desc.name === 'notifications') {
                return Promise.resolve({state: Notification.permission, onchange: null});
            }
            return origQuery(desc).catch(function(){
                return {state:'prompt', onchange:null};
            });
        };
    }

    // ── 7. Connection / Network ─────────────────────────────────
    if (!navigator.connection) {
        Object.defineProperty(navigator, 'connection', {
            get: () => ({effectiveType:'4g', rtt:50, downlink:10, saveData:false,
                         onchange:null, addEventListener:function(){},
                         removeEventListener:function(){}})
        });
    }

    // ── 8. Battery API ──────────────────────────────────────────
    if (!navigator.getBattery) {
        navigator.getBattery = function(){
            return Promise.resolve({
                charging:true, chargingTime:0, dischargingTime:Infinity, level:1,
                addEventListener:function(){}, removeEventListener:function(){}
            });
        };
    }

    // ── 9. WebGL renderer / vendor ──────────────────────────────
    (function(){
        var g = HTMLCanvasElement.prototype.getContext;
        HTMLCanvasElement.prototype.getContext = function(type, attrs){
            var ctx = g.call(this, type, attrs);
            if (ctx && (type === 'webgl' || type === 'webgl2' || type === 'experimental-webgl')) {
                var origGetParam = ctx.getParameter.bind(ctx);
                var ext = ctx.getExtension('WEBGL_debug_renderer_info');
                ctx.getParameter = function(p){
                    if (ext) {
                        if (p === ext.UNMASKED_VENDOR_WEBGL)   return 'Google Inc. (Apple)';
                        if (p === ext.UNMASKED_RENDERER_WEBGL) return 'ANGLE (Apple, Apple M1, OpenGL 4.1)';
                    }
                    return origGetParam(p);
                };
            }
            return ctx;
        };
    })();

    // ── 10. Screen metrics ──────────────────────────────────────
    // Only override if values look abnormal (headless or tiny windows).
    if (screen.width === 0 || screen.height === 0) {
        Object.defineProperty(screen, 'width',      {get: () => 1920});
        Object.defineProperty(screen, 'height',     {get: () => 1080});
        Object.defineProperty(screen, 'availWidth',  {get: () => 1920});
        Object.defineProperty(screen, 'availHeight', {get: () => 1080});
        Object.defineProperty(screen, 'colorDepth',  {get: () => 24});
        Object.defineProperty(screen, 'pixelDepth',  {get: () => 24});
    }

    // ── 11. Iframe contentWindow leak protection ────────────────
    // Some detectors create a temp iframe and check that
    // contentWindow.chrome exists.  Ensure it does.
    var origCreateElement = document.createElement.bind(document);
    document.createElement = function(tag){
        var el = origCreateElement(tag);
        if (tag.toLowerCase() === 'iframe') {
            var origAppend = el.__proto__.__lookupSetter__
                ? undefined : null;
            // After the iframe loads, patch its contentWindow.
            el.addEventListener('load', function(){
                try {
                    if (el.contentWindow && !el.contentWindow.chrome) {
                        el.contentWindow.chrome = window.chrome;
                    }
                } catch(e){}
            });
        }
        return el;
    };

    // ── 12. Notification permission ─────────────────────────────
    // Override Notification.permission to return 'default' instead of
    // 'denied' which is common in automated browsers.
    try {
        if (typeof Notification !== 'undefined' && Notification.permission === 'denied') {
            Object.defineProperty(Notification, 'permission', {get: () => 'default'});
        }
    } catch(e){}

    // ── 13. Canvas toDataURL / toBlob consistency ───────────────
    // Some fingerprinters detect automation by running canvas ops and
    // comparing a hash across frames.  We do not alter pixel data
    // (that would break sites), but we make sure the functions exist
    // with the expected signatures.
    if (typeof OffscreenCanvas !== 'undefined') {
        // ensure OffscreenCanvas.convertToBlob exists (missing in some engines)
        if (!OffscreenCanvas.prototype.convertToBlob) {
            OffscreenCanvas.prototype.convertToBlob = function(opts){
                return Promise.resolve(new Blob([], {type: opts && opts.type || 'image/png'}));
            };
        }
    }

    // ── 14. Speech synthesis voices ─────────────────────────────
    // Empty voice list is a headless giveaway.
    if (window.speechSynthesis && window.speechSynthesis.getVoices().length === 0) {
        var _origGetVoices = window.speechSynthesis.getVoices.bind(window.speechSynthesis);
        window.speechSynthesis.getVoices = function(){
            var v = _origGetVoices();
            if (v.length === 0) {
                return [{voiceURI:'Samantha', name:'Samantha', lang:'en-US',
                         localService:true, default:true}];
            }
            return v;
        };
    }

    // ── 15. Prevent Error.stack leaking qt / chromium paths ─────
    (function(){
        var origCaptureStack = Error.captureStackTrace;
        if (origCaptureStack) {
            Error.captureStackTrace = function(obj, fn){
                origCaptureStack(obj, fn);
                if (obj.stack) {
                    obj.stack = obj.stack.replace(/qtwebengine/gi, 'chromium');
                }
            };
        }
    })();

    // ── 16. Media codec fingerprint ─────────────────────────────
    // Leave isTypeSupported alone here.  The _VIDEO_FIX_JS script
    // (injected at DocumentCreation) overrides it with full H.264
    // and AAC support and handles everything via a FakeSourceBuffer
    // shim that routes decoded data through macOS VideoToolbox.
})();
"""


# ── Video fix: full MSE H.264 shim for macOS VideoToolbox ────────────────────
#
# QtWebEngine's internal FFmpeg was built WITHOUT proprietary H.264/AAC
# decoders.  MediaSource.isTypeSupported('video/mp4; codecs="avc1..."')
# returns false.  However, the native <video src="..."> element CAN play
# H.264 because macOS routes it through VideoToolbox.
#
# This script creates a complete MSE shim:
#   A. Override isTypeSupported to claim H.264/AAC support.
#   B. When addSourceBuffer() throws for an unsupported codec, return a
#      FakeSourceBuffer that collects all appendBuffer data.
#   C. On endOfStream or error, concatenate collected chunks into a Blob
#      URL and assign it to the <video> element's src for native playback.
#   D. Sync separate audio track via a hidden <audio> element.
#   E. Capture video URLs from fetch/XHR as a fallback.
#   F. DOM scanner + MutationObserver as a last-resort safety net.
#
# Injected at DocumentCreation so it runs before any page script.
_VIDEO_FIX_JS = r"""
(function(){
    'use strict';

    // Do not run on pages that are handled by the WKWebView engine
    // (LinkedIn, Bitmovin DRM).  Those pages have native H.264 via
    // VideoToolbox and the proxy intercept causes CSP crashes.
    var _HOST = location.hostname.toLowerCase();
    var _WK_HOSTS = [
        'linkedin.com', 'www.linkedin.com', 'm.linkedin.com',
        'lnkd.in', 'media.licdn.com', 'dms.licdn.com',
        'bitmovin.com', 'www.bitmovin.com', 'cdn.bitmovin.com',
        'player.bitmovin.com',
    ];
    for (var _i = 0; _i < _WK_HOSTS.length; _i++) {
        if (_HOST === _WK_HOSTS[_i] || _HOST.endsWith('.' + _WK_HOSTS[_i])) {
            return;  // let the WK engine handle this page natively
        }
    }
    // ════════════════════════════════════════════════════════════
    // Video.js has its own codec/source-selection pipeline that
    // operates independently of HTMLMediaElement.canPlayType.
    // When it decides a source is unsupported it fires
    // MEDIA_ERR_SRC_NOT_SUPPORTED and shows an error overlay
    // BEFORE our fallback code ever runs.
    //
    // Fix: when Video.js loads, we
    //   a) mute its WARN + ERROR console spam,
    //   b) override the Html5 tech's canPlayType to accept H.264,
    //   c) register a source handler on the Html5 tech that
    //      rewrites MP4/H.264 source URLs through our local
    //      transcoding proxy (H.264 -> VP8/WebM),
    //   d) hook every new player instance to retry through the
    //      proxy on MEDIA_ERR_SRC_NOT_SUPPORTED.
    //
    // The proxy URL and availability flag are baked in by Python
    // at profile-setup time via %%PROXY_URL%% / %%PROXY_OK%%.
    var _PROXY   = '%%PROXY_URL%%';
    var _proxyOk = %%PROXY_OK%%;

    // Remote HTTPS proxy on user's server.  Used as a CORS/HTTPS
    // pass-through for video resources on any HTTPS page.  Does not
    // transcode; just relays bytes over HTTPS so there is no
    // mixed-content block.
    var _REMOTE_PROXY = '%%REMOTE_PROXY%%';

    // The transcoding proxy runs on plain HTTP (http://127.0.0.1:9878).
    // Chromium's --unsafely-treat-insecure-origin-as-secure flag marks
    // that origin as secure, so HTTPS pages can load media from it
    // without mixed-content blocks.

    // -- Site whitelist: only route through the transcoding proxy on
    //    specific domains.  DRM sites (Bitmovin, Netflix, etc.) must
    //    NOT be proxied because the Widevine CDM handles playback.
    var _PROXY_DOMAINS = [
        'linkedin.com',
        'licdn.com',
        'facebook.com',
        'fbcdn.net',
        'twitter.com',
        'x.com',
        'twimg.com',
    ];

    // Detect EME (Encrypted Media Extensions) usage.  When a page
    // calls requestMediaKeySystemAccess the CDM will handle decryption
    // inside the browser.  The transcoding proxy cannot handle
    // encrypted streams, so disable it when DRM is active.
    var _emeActive = false;
    if (navigator.requestMediaKeySystemAccess) {
        var _origRMKSA = navigator.requestMediaKeySystemAccess.bind(navigator);
        navigator.requestMediaKeySystemAccess = function(keySystem, configs) {
            _emeActive = true;
            return _origRMKSA(keySystem, configs);
        };
    }

    // Guard against "play() request was interrupted by pause()" AbortError.
    // Many players (Bitmovin, Video.js) call play() then pause() before
    // the play promise resolves, producing an uncaught promise rejection.
    // Wrap HTMLMediaElement.prototype.play to silently absorb AbortErrors.
    (function() {
        var _origPlay = HTMLMediaElement.prototype.play;
        HTMLMediaElement.prototype.play = function() {
            var p = _origPlay.apply(this, arguments);
            if (p && typeof p.catch === 'function') {
                p = p.catch(function(err) {
                    if (err && err.name === 'AbortError') return;
                    throw err;
                });
            }
            return p;
        };
    })();

    // Safety net: swallow any remaining AbortError promise rejections
    // from third-party code that calls play() without catching.
    window.addEventListener('unhandledrejection', function(e) {
        if (e.reason && e.reason.name === 'AbortError') {
            e.preventDefault();
        }
    });

    // DASH/HLS segment URLs cannot be transcoded individually.
    var _SEGMENT_RE = /\/dash\/|\/hls\/|\.m4s(\?|$)|\.m3u8|\.mpd|init\.mp4|segment.*\.mp4|\.ts(\?|$)/i;
    function _isDashHlsSegment(url) { return _SEGMENT_RE.test(url); }

    function _proxyAllowed() {
        if (!_proxyOk) return false;
        if (_emeActive) return false;
        var host = window.location.hostname || '';
        for (var i = 0; i < _PROXY_DOMAINS.length; i++) {
            var d = _PROXY_DOMAINS[i];
            if (host === d || host.endsWith('.' + d)) return true;
        }
        return false;
    }

    // Build a proxied URL.  On whitelisted domains, use the local
    // transcoding proxy (H.264 -> VP8).  On all other HTTPS pages,
    // use the remote HTTPS pass-through proxy if configured.
    function _makeProxyUrl(origUrl) {
        if (!origUrl || !/^https?:\/\//i.test(origUrl)) return origUrl;
        if (_isDashHlsSegment(origUrl)) return origUrl;
        if (_emeActive) return origUrl;
        if (_proxyAllowed()) {
            // Use cbproxy: custom scheme.  The Python scheme handler
            // forwards to http://127.0.0.1:9878, bypassing CSP and
            // mixed-content restrictions entirely.
            return 'cbproxy:///transcode?url=' + encodeURIComponent(origUrl);
        }
        if (_REMOTE_PROXY && window.location.protocol === 'https:') {
            return _REMOTE_PROXY + '?url=' + encodeURIComponent(origUrl);
        }
        return origUrl;
    }

    // ── Proactive video src rewriting ────────────────────────
    // On whitelisted domains, intercept video.src assignment and
    // rewrite the URL through the transcoding proxy BEFORE the
    // browser attempts to decode the H.264 stream.  This avoids
    // the error-then-retry cycle entirely.
    (function _interceptVideoSrc() {
        var srcDesc = Object.getOwnPropertyDescriptor(
            HTMLMediaElement.prototype, 'src');
        if (!srcDesc || !srcDesc.set) return;
        var _origSrcSet = srcDesc.set;
        var _origSrcGet = srcDesc.get;

        Object.defineProperty(HTMLMediaElement.prototype, 'src', {
            get: function() { return _origSrcGet.call(this); },
            set: function(val) {
                if (this.tagName === 'VIDEO'
                    && val && typeof val === 'string'
                    && /^https?:\/\//i.test(val)
                    && val.indexOf('cbproxy:') !== 0
                    && val.indexOf('/transcode?') === -1
                    && !_isDashHlsSegment(val)
                    && _proxyAllowed()) {
                    this._cbOriginalSrc = val;
                    var proxied = _makeProxyUrl(val);
                    if (proxied !== val) {
                        console.info('[CB] Rewriting video.src:', val.substring(0, 120));
                        _origSrcSet.call(this, proxied);
                        return;
                    }
                }
                _origSrcSet.call(this, val);
            },
            configurable: true,
            enumerable: true
        });

        // Also intercept setAttribute('src', ...) for <video> elements.
        var _origSetAttr = Element.prototype.setAttribute;
        Element.prototype.setAttribute = function(name, value) {
            if (name === 'src' && this.tagName === 'VIDEO'
                && value && typeof value === 'string'
                && /^https?:\/\//i.test(value)
                && value.indexOf('cbproxy:') !== 0
                && value.indexOf('/transcode?') === -1
                && !_isDashHlsSegment(value)
                && _proxyAllowed()) {
                this._cbOriginalSrc = value;
                var proxied = _makeProxyUrl(value);
                if (proxied !== value) {
                    console.info('[CB] Rewriting setAttribute src:', value.substring(0, 120));
                    return _origSetAttr.call(this, name, proxied);
                }
            }
            return _origSetAttr.call(this, name, value);
        };
    })();

    (function _hookVideojs() {
        function _patchVideojs(vjs) {
            if (!vjs || vjs._cbFullyPatched) return;
            vjs._cbFullyPatched = true;

            // (a) Mute WARN + ERROR log spam.
            if (vjs.log) {
                var _origWarn = vjs.log.warn;
                var _origError = vjs.log.error;
                if (typeof _origWarn === 'function') {
                    vjs.log.warn = function() {
                        var msg = Array.prototype.join.call(arguments, ' ');
                        if (msg.indexOf('Using the tech directly') !== -1) return;
                        return _origWarn.apply(this, arguments);
                    };
                }
                if (typeof _origError === 'function') {
                    vjs.log.error = function() {
                        var msg = Array.prototype.join.call(arguments, ' ');
                        if (msg.indexOf('MEDIA_ERR_SRC_NOT_SUPPORTED') !== -1) return;
                        return _origError.apply(this, arguments);
                    };
                }
            }

            // (b) Override Html5 tech's canPlayType.
            var Html5 = vjs.getTech && vjs.getTech('Html5');
            if (Html5) {
                var _origCanPlay = Html5.canPlayType;
                Html5.canPlayType = function(type) {
                    var r = '';
                    try { r = _origCanPlay.call(this, type); } catch(e) {}
                    if (r) return r;
                    if (/avc1|avc3|hev1|hvc1|mp4a/i.test(type)) return 'probably';
                    if (/video\/mp4|audio\/mp4|video\/x-m4v/i.test(type)) return 'probably';
                    return r;
                };
                // Also patch the static isSupported
                if (Html5.isSupported) {
                    Html5.isSupported = function() { return true; };
                }
                // Patch prototype canPlayType too.
                if (Html5.prototype && Html5.prototype.canPlayType) {
                    Html5.prototype.canPlayType = Html5.canPlayType;
                }

                // (c) Register a source handler that rewrites MP4 URLs.
                //     Uses the local transcoding proxy on whitelisted
                //     domains, or the remote HTTPS pass-through proxy
                //     on other HTTPS pages.
                if ((_proxyAllowed() || _REMOTE_PROXY) && Html5.registerSourceHandler) {
                    Html5.registerSourceHandler({
                        canHandleSource: function(srcObj, options) {
                            if (_emeActive) return '';
                            if (!srcObj || !srcObj.type) return '';
                            if (/video\/mp4|audio\/mp4|video\/x-m4v/i.test(srcObj.type))
                                return 'probably';
                            if (/avc1|avc3|mp4a/i.test(srcObj.type))
                                return 'probably';
                            return '';
                        },
                        handleSource: function(srcObj, tech, options) {
                            var origUrl = srcObj.src || '';
                            var proxied = _makeProxyUrl(origUrl);
                            if (proxied !== origUrl && origUrl.indexOf(_PROXY) === -1
                                && (!_REMOTE_PROXY || origUrl.indexOf(_REMOTE_PROXY) === -1)) {
                                tech.setSrc(proxied);
                            } else {
                                tech.setSrc(origUrl);
                            }
                            return { dispose: function(){} };
                        },
                        canPlayType: function(type) {
                            if (/video\/mp4|audio\/mp4|video\/x-m4v/i.test(type))
                                return 'probably';
                            if (/avc1|avc3|mp4a/i.test(type))
                                return 'probably';
                            return '';
                        }
                    }, 0); // priority 0 = highest
                }
            }

            // (d) Hook every new player to retry on error.
            if (_proxyAllowed() || _REMOTE_PROXY) {
                var _origVjs = vjs;
                // Wrap vjs() calls to get access to player instances.
                try {
                    vjs.hook('setup', function(player) {
                        if (!player || player._cbProxyHooked) return;
                        player._cbProxyHooked = true;
                        player.on('error', function() {
                            var err = player.error();
                            if (!err || err.code !== 4) return;
                            if (player._cbProxyRetried) return;
                            player._cbProxyRetried = true;

                            // Get the original source.
                            var srcs = player.currentSources
                                ? player.currentSources()
                                : [player.currentSource()];
                            var origUrl = '';
                            for (var i = 0; i < srcs.length; i++) {
                                if (srcs[i] && srcs[i].src) {
                                    origUrl = srcs[i].src;
                                    break;
                                }
                            }
                            if (!origUrl || origUrl.indexOf(_PROXY) !== -1
                                || origUrl.indexOf('cbproxy:') === 0
                                || (_REMOTE_PROXY && origUrl.indexOf(_REMOTE_PROXY) !== -1)) {
                                // Already a proxy URL or no URL; try direct
                                // fallback from captured page URLs.
                                var el = player.el();
                                if (el) {
                                    var vid = el.querySelector('video');
                                    if (vid) _tryDirectUrl_vjs(vid);
                                }
                                return;
                            }

                            // Skip DASH/HLS segments and DRM streams.
                            if (_isDashHlsSegment(origUrl) || _emeActive) return;

                            // Clear the error and retry via proxy.
                            var retryUrl = _makeProxyUrl(origUrl);
                            if (retryUrl === origUrl) return;
                            player.error(null);
                            player.src({
                                src: retryUrl,
                                type: retryUrl.indexOf('cbproxy:') === 0
                                    || retryUrl.indexOf('/transcode?') !== -1
                                    ? 'video/webm' : 'video/mp4'
                            });
                            player.play().catch(function(){});
                        });
                    });
                } catch(e) {}
            }
        }

        // Patch immediately if already present.
        if (typeof window.videojs === 'function') _patchVideojs(window.videojs);

        // Watch for lazy-loaded Video.js.
        var _desc = Object.getOwnPropertyDescriptor(window, 'videojs');
        if (!_desc || _desc.configurable) {
            var _storedVjs = window.videojs;
            Object.defineProperty(window, 'videojs', {
                configurable: true,
                enumerable: true,
                get: function() { return _storedVjs; },
                set: function(v) {
                    _storedVjs = v;
                    _patchVideojs(v);
                }
            });
        }
    })();

    // Helper for the Video.js error hook: try extracted page URLs.
    function _tryDirectUrl_vjs(video) {
        if (video._directFixed) return;
        if (typeof _extractPageUrls !== 'function') return;
        var urls = _extractPageUrls();
        if (!urls.length) return;
        video._directFixed = true;
        var proxied = _makeProxyUrl(urls[0]);
        if (proxied !== urls[0]) {
            video.src = proxied;
        } else {
            video.src = urls[0];
        }
        video.controls = true;
        video.play().catch(function(){});
    }

    // ════════════════════════════════════════════════════════════
    // -0. Inject @font-face fallback for JetBrains Mono
    // ════════════════════════════════════════════════════════════
    // Pages that request JetBrains Mono trigger an expensive system
    // font lookup if the font is not installed.  Provide a CSS
    // @font-face that maps it to the system monospace stack so the
    // lookup resolves instantly.
    (function _fontFallback() {
        try {
            var style = document.createElement('style');
            style.textContent = '@font-face { '
                + 'font-family: "JetBrains Mono"; '
                + 'src: local("Menlo"), local("SF Mono"), '
                + 'local("Monaco"), local("Courier New"); '
                + 'font-weight: 100 900; '
                + 'font-style: normal; }';
            (document.head || document.documentElement).appendChild(style);
        } catch(e) {}
    })();

    // ════════════════════════════════════════════════════════════
    // A. Override canPlayType (works even without MSE)
    // ════════════════════════════════════════════════════════════
    // This must run BEFORE the MSE guard below because <video> elements
    // use canPlayType for <source> selection regardless of whether
    // MediaSource exists.
    var _H264_RE = /avc1|avc3|hev1|hvc1|mp4a|flac|ac-3|ec-3/i;
    var _MEDIA_RE = /^(video|audio)\//i;

    var _realCanPlay = HTMLMediaElement.prototype.canPlayType;
    HTMLMediaElement.prototype.canPlayType = function(mime) {
        var result = _realCanPlay.call(this, mime);
        if (result) return result;
        if (_H264_RE.test(mime) && _MEDIA_RE.test(mime)) return 'probably';
        if (/video\/mp4|video\/x-m4v|audio\/mp4|audio\/x-m4a|audio\/aac/i.test(mime))
            return 'probably';
        return result;
    };

    if (!window.MediaSource) return;

    // ════════════════════════════════════════════════════════════
    // A-2. Override isTypeSupported (MSE)
    // ════════════════════════════════════════════════════════════
    // _H264_RE and _MEDIA_RE are defined above (before MSE guard).
    var _realIsType = MediaSource.isTypeSupported.bind(MediaSource);
    MediaSource.isTypeSupported = function(mime) {
        if (_realIsType(mime)) return true;
        if (_H264_RE.test(mime) && _MEDIA_RE.test(mime)) return true;
        return false;
    };

    // ════════════════════════════════════════════════════════════
    // B. FakeSourceBuffer
    // ════════════════════════════════════════════════════════════
    function FakeSourceBuffer(ms, mime) {
        this._ms       = ms;
        this._mime     = mime;
        this._chunks   = [];
        this._updating = false;
        this._removed  = false;
        this._evts     = {};
        this._totalBytes = 0;
        this._firstAppendTime = 0;
        this._flushTimer = null;
        this.mode       = 'segments';
        this.timestampOffset   = 0;
        this.appendWindowStart = 0;
        this.appendWindowEnd   = Infinity;
        this.onupdatestart = null;
        this.onupdate      = null;
        this.onupdateend   = null;
        this.onerror       = null;
        this.onabort       = null;
        this.audioTracks = {length:0};
        this.videoTracks = {length:0};
        this.textTracks  = {length:0};
        this.buffered = _makeTR(0, 0);
    }
    function _makeTR(s, e) {
        return {
            length: (e > s) ? 1 : 0,
            start:  function(i) { return s; },
            end:    function(i) { return e; }
        };
    }
    var FSBp = FakeSourceBuffer.prototype;
    FSBp.addEventListener    = function(t,fn){ if(!this._evts[t]) this._evts[t]=[]; this._evts[t].push(fn); };
    FSBp.removeEventListener = function(t,fn){ var a=this._evts[t]; if(a) this._evts[t]=a.filter(function(f){return f!==fn;}); };
    FSBp.dispatchEvent = function(ev){
        var a = this._evts[ev.type] || [];
        for (var i=0; i<a.length; i++) try{a[i].call(this,ev);}catch(e){}
        var h = this['on'+ev.type];
        if (typeof h === 'function') try{h.call(this,ev);}catch(e){}
    };
    Object.defineProperty(FSBp, 'updating', {
        get: function(){ return this._updating; }
    });
    FSBp.appendBuffer = function(data) {
        var self = this;
        if (self._updating) throw new DOMException('SourceBuffer is updating','InvalidStateError');
        self._updating = true;
        // Save raw bytes
        try {
            var u8;
            if (data instanceof ArrayBuffer) { u8 = new Uint8Array(data); }
            else if (ArrayBuffer.isView(data)) { u8 = new Uint8Array(data.buffer, data.byteOffset, data.byteLength); }
            else { u8 = new Uint8Array(0); }
            self._chunks.push(u8);
            self._totalBytes += u8.byteLength;
            if (!self._firstAppendTime) self._firstAppendTime = Date.now();
        } catch(e) {}
        // Estimate buffered duration from total bytes
        var total = self._totalBytes;
        var dur = self._ms._fakeDuration || (total / 500000);
        var frac = total / Math.max(1, self._ms._expectedBytes || total);
        self.buffered = _makeTR(0, Math.min(dur, dur * frac));

        // Auto-flush: if we have collected enough data and no
        // endOfStream has been called, schedule a flush.  This
        // handles players (LinkedIn) that never call endOfStream.
        if (!self._ms._flushedToNative && !self._ms._autoFlushScheduled
            && self._totalBytes > 50000) {
            self._ms._autoFlushScheduled = true;
            console.info('[CB] FakeSourceBuffer: auto-flush scheduled ('
                + self._totalBytes + ' bytes collected)');
            // Wait 3 seconds for more data to arrive, then flush.
            setTimeout(function() {
                if (!self._ms._flushedToNative) {
                    console.info('[CB] FakeSourceBuffer: auto-flushing '
                        + self._totalBytes + ' bytes');
                    _flushToNative(self._ms);
                }
            }, 3000);
        }

        // Async event sequence (matches real SourceBuffer behaviour)
        setTimeout(function(){
            self.dispatchEvent(new Event('updatestart'));
            setTimeout(function(){
                self._updating = false;
                self.dispatchEvent(new Event('update'));
                self.dispatchEvent(new Event('updateend'));
            }, 0);
        }, 0);
    };
    FSBp.remove = function() {
        var self = this;
        self._updating = true;
        setTimeout(function(){
            self._updating = false;
            self.dispatchEvent(new Event('updateend'));
        }, 0);
    };
    FSBp.abort = function() { this._updating = false; };
    FSBp.changeType = function(t) { this._mime = t; };

    // ════════════════════════════════════════════════════════════
    // B2. Patch addSourceBuffer to return FakeSourceBuffer on failure
    // ════════════════════════════════════════════════════════════
    var _realAddSB = MediaSource.prototype.addSourceBuffer;
    MediaSource.prototype.addSourceBuffer = function(mime) {
        try {
            return _realAddSB.call(this, mime);
        } catch(e) {
            // Native addSourceBuffer rejected the codec - shim it.
            var fake = new FakeSourceBuffer(this, mime);
            if (!this._fakeSBs) this._fakeSBs = [];
            this._fakeSBs.push(fake);
            // Also add to the real sourceBuffers list if possible.
            // (Some players iterate ms.sourceBuffers.)
            return fake;
        }
    };

    // Patch removeSourceBuffer to handle fakes gracefully.
    var _realRemoveSB = MediaSource.prototype.removeSourceBuffer;
    MediaSource.prototype.removeSourceBuffer = function(sb) {
        if (sb instanceof FakeSourceBuffer) {
            sb._removed = true;
            return;
        }
        return _realRemoveSB.call(this, sb);
    };

    // ════════════════════════════════════════════════════════════
    // B3. Patch duration property (players set this; throws if
    //     readyState is not 'open' or there are no real SBs)
    // ════════════════════════════════════════════════════════════
    var _durDesc = Object.getOwnPropertyDescriptor(MediaSource.prototype, 'duration');
    if (_durDesc) {
        Object.defineProperty(MediaSource.prototype, 'duration', {
            get: function(){ try{return _durDesc.get.call(this);}catch(e){return this._fakeDuration||NaN;} },
            set: function(v){ this._fakeDuration = v; try{_durDesc.set.call(this, v);}catch(e){} },
            configurable: true
        });
    }

    // ════════════════════════════════════════════════════════════
    // C. Flush fake SourceBuffers -> blob -> native <video>
    // ════════════════════════════════════════════════════════════
    var _realEndOfStream = MediaSource.prototype.endOfStream;
    MediaSource.prototype.endOfStream = function(error) {
        if (this._fakeSBs && this._fakeSBs.length) {
            _flushToNative(this);
        }
        try { return _realEndOfStream.call(this, error); } catch(e) {}
    };

    function _flushToNative(ms) {
        if (ms._flushedToNative) return;
        ms._flushedToNative = true;

        var video = _findVideoForMS(ms);
        if (!video) return;

        var vChunks = [], aChunks = [];
        ms._fakeSBs.forEach(function(sb) {
            if (!sb._chunks.length) return;
            if (/audio/i.test(sb._mime)) {
                aChunks = aChunks.concat(sb._chunks);
            } else {
                vChunks = vChunks.concat(sb._chunks);
            }
        });

        // If we only have audio chunks, treat them as the primary source.
        if (!vChunks.length && aChunks.length) {
            vChunks = aChunks; aChunks = [];
        }
        if (!vChunks.length) return;

        var wasPlaying = !video.paused;
        var curTime    = video.currentTime || 0;

        // Dispose any Video.js player before replacing the source.
        _disposeVideoJS(video);
        video.pause();

        // First try: route the original video URL through a proxy.
        // Uses local transcoding proxy on whitelisted domains, or the
        // remote HTTPS pass-through on other pages.
        // Skip when DRM is active or the URL is a DASH/HLS segment.
        var urls = _extractPageUrls().filter(function(u) {
            return !_isDashHlsSegment(u);
        });
        // Also check captured network URLs for LinkedIn CDN.
        _capturedUrls.forEach(function(c) {
            var cu = c.url;
            if (urls.indexOf(cu) === -1
                && /dms\.licdn\.com|media\.licdn\.com/i.test(cu)
                && !_isDashHlsSegment(cu)) {
                urls.push(cu);
            }
        });
        console.info('[CB] _flushToNative: found', urls.length,
            'candidate URL(s) for proxy routing');
        if (urls.length) {
            var proxied = _makeProxyUrl(urls[0]);
            if (proxied !== urls[0]) {
                try { video.removeAttribute('src'); video.load(); } catch(e) {}
                video.querySelectorAll('source').forEach(function(s){ s.remove(); });
                video.src = proxied;
                video.controls = true;
                video.setAttribute('playsinline', '');
                // On error, fall back to old blob approach.
                video.addEventListener('error', function _mse_pe() {
                    video.removeEventListener('error', _mse_pe);
                    if (!video._blobFallbackUsed) {
                        video._blobFallbackUsed = true;
                        _blobFallback(video, vChunks, aChunks, wasPlaying, curTime);
                    }
                });
                if (wasPlaying) video.play().catch(function(){});
                _hideOverlays(video);
                return;
            }
        }

        // Second try: blob fallback.  Concatenate collected fMP4 chunks
        // into a blob URL.  This works for content whose codec Chromium
        // supports natively (VP8, VP9, etc.) but NOT for H.264.
        _blobFallback(video, vChunks, aChunks, wasPlaying, curTime);
    }

    function _blobFallback(video, vChunks, aChunks, wasPlaying, curTime) {
        // Primary path: POST raw H.264 fMP4 chunks to the transcoding
        // proxy via cbproxy: scheme.  The proxy transcodes to VP8/WebM
        // and we play the result as a blob URL.
        if (_proxyOk && vChunks.length) {
            var blob = new Blob(vChunks, { type: 'video/mp4' });
            console.info('[CB] _blobFallback: posting', blob.size,
                'bytes of H.264 data for transcoding');

            // Try 1: POST via cbproxy custom scheme.
            var _postViaScheme = function() {
                return fetch('cbproxy:///transcode-data', {
                    method: 'POST',
                    headers: { 'Content-Type': 'video/mp4' },
                    body: blob
                }).then(function(resp) {
                    if (!resp.ok) throw new Error('cbproxy HTTP ' + resp.status);
                    return resp.blob();
                });
            };

            // Try 2: POST directly to local proxy (may be blocked by CSP).
            var _postDirect = function() {
                return new Promise(function(resolve, reject) {
                    var xhr = new XMLHttpRequest();
                    xhr.open('POST', 'http://127.0.0.1:' + _PROXY.replace(/.*:(\d+).*/, '$1') + '/transcode-data', true);
                    xhr.responseType = 'blob';
                    xhr.setRequestHeader('Content-Type', 'video/mp4');
                    xhr.onload = function() {
                        if (xhr.status === 200) resolve(xhr.response);
                        else reject(new Error('XHR HTTP ' + xhr.status));
                    };
                    xhr.onerror = function() { reject(new Error('XHR network error')); };
                    xhr.send(blob);
                });
            };

            // Try 3: Read blob as base64 and GET via cbproxy.
            var _postViaBase64 = function() {
                return new Promise(function(resolve, reject) {
                    var reader = new FileReader();
                    reader.onload = function() {
                        var b64 = reader.result.split(',')[1];
                        // Send base64 data as a query parameter.
                        var url = 'cbproxy:///transcode-b64?data=' + encodeURIComponent(b64);
                        fetch(url).then(function(resp) {
                            if (!resp.ok) throw new Error('b64 HTTP ' + resp.status);
                            return resp.blob();
                        }).then(resolve).catch(reject);
                    };
                    reader.onerror = function() { reject(new Error('FileReader error')); };
                    reader.readAsDataURL(blob);
                });
            };

            _postViaScheme()
                .catch(function(e) {
                    console.warn('[CB] cbproxy POST failed:', e.message, '-- trying XHR');
                    return _postDirect();
                })
                .catch(function(e) {
                    console.warn('[CB] XHR POST failed:', e.message, '-- trying base64');
                    return _postViaBase64();
                })
                .then(function(webmBlob) {
                    console.info('[CB] Transcoded:', webmBlob.size, 'bytes of WebM');
                    _applyVideoBlob(video, webmBlob, 'video/webm',
                        aChunks, wasPlaying, curTime);
                })
                .catch(function(err) {
                    console.warn('[CB] All transcode paths failed:', err,
                        '-- falling back to raw blob');
                    var vBlob2 = new Blob(vChunks, { type: 'video/mp4' });
                    _applyVideoBlob(video, vBlob2, 'video/mp4',
                        aChunks, wasPlaying, curTime);
                });
            return;
        }
        // Fallback: direct blob (works for VP8/VP9 but not H.264).
        var vBlob = new Blob(vChunks, { type: 'video/mp4' });
        _applyVideoBlob(video, vBlob, 'video/mp4',
            aChunks, wasPlaying, curTime);
    }

    function _applyVideoBlob(video, vBlob, mimeType, aChunks, wasPlaying, curTime) {
        var vUrl = URL.createObjectURL(vBlob);

        // Remove any existing sources to avoid confusion.
        try { video.removeAttribute('src'); video.load(); } catch(e) {}
        video.querySelectorAll('source').forEach(function(s){ s.remove(); });

        video.src      = vUrl;
        video.controls = true;
        video.setAttribute('playsinline', '');

        // Audio sync: separate hidden <audio> element.
        if (aChunks && aChunks.length) {
            var aBlob = new Blob(aChunks, { type: 'audio/mp4' });
            var aUrl  = URL.createObjectURL(aBlob);
            var aud   = document.createElement('audio');
            aud.src   = aUrl;
            aud.style.cssText = 'position:fixed;left:-9999px;top:-9999px;width:0;height:0;';
            document.body.appendChild(aud);
            video._syncAudio = aud;

            video.addEventListener('play',       function(){ aud.play().catch(function(){}); });
            video.addEventListener('pause',      function(){ aud.pause(); });
            video.addEventListener('seeked',     function(){ aud.currentTime = video.currentTime; });
            video.addEventListener('ratechange', function(){ aud.playbackRate = video.playbackRate; });
            video.addEventListener('volumechange', function(){
                aud.volume = video.volume;
                aud.muted  = video.muted;
            });
            video.addEventListener('timeupdate', function(){
                if (Math.abs(aud.currentTime - video.currentTime) > 0.3) {
                    aud.currentTime = video.currentTime;
                }
            });
        }

        // Restore playback state.
        video.addEventListener('loadeddata', function _ld() {
            video.removeEventListener('loadeddata', _ld);
            if (curTime > 0.5) video.currentTime = curTime;
            if (wasPlaying) video.play().catch(function(){});
        });

        _hideOverlays(video);
    }

    // ════════════════════════════════════════════════════════════
    // D. Map blob URLs to MediaSource objects
    // ════════════════════════════════════════════════════════════
    var _blobMS = new Map();
    var _realCreateObjURL = URL.createObjectURL.bind(URL);
    URL.createObjectURL = function(obj) {
        var url = _realCreateObjURL(obj);
        if (obj instanceof MediaSource) _blobMS.set(url, obj);
        return url;
    };

    function _findVideoForMS(ms) {
        // First pass: match via blob URL mapping.
        var vids = document.querySelectorAll('video');
        for (var i = 0; i < vids.length; i++) {
            var s = vids[i].src || vids[i].currentSrc || '';
            if (_blobMS.get(s) === ms) return vids[i];
        }
        // Second pass: any video with a blob: src.
        for (var j = 0; j < vids.length; j++) {
            if ((vids[j].src||'').indexOf('blob:') === 0) return vids[j];
        }
        return vids[0] || null;
    }

    // ════════════════════════════════════════════════════════════
    // E. Capture video URLs from fetch / XHR as fallback data
    // ════════════════════════════════════════════════════════════
    var _capturedUrls = [];

    // -- fetch hook --
    if (window.fetch) {
        var _realFetch = window.fetch.bind(window);
        window.fetch = function(input, init) {
            var url = (typeof input === 'string') ? input :
                      (input && input.url) ? input.url : '';
            // Silently reject chrome-extension:// URLs.  LinkedIn
            // probes chrome-extension://invalid/ to detect Widevine
            // CDM.  Letting these through floods the console with
            // ERR_FAILED.  Return a synthetic 404 response instead.
            if (/^chrome-extension:\/\//i.test(url)) {
                return Promise.resolve(new Response('', {
                    status: 404,
                    statusText: 'Not Found'
                }));
            }
            return _realFetch(input, init).then(function(resp) {
                try {
                    var ct = (resp.headers.get('content-type') || '').toLowerCase();
                    if (/video\/mp4|video\/webm|application\/dash|mpd/i.test(ct) ||
                        /\.mp4|\.m4s|\.m4v|\.mpd|dms\.licdn|\/dash\/|\/hls\//i.test(url)) {
                        _capturedUrls.push({ url: url, ct: ct, ts: Date.now() });
                    }
                } catch(e) {}
                return resp;
            });
        };
    }

    // -- XHR hook --
    var _realXHROpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url) {
        this._vfUrl = url;
        return _realXHROpen.apply(this, arguments);
    };
    var _realXHRSend = XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send = function(body) {
        var xhr = this;
        var url = xhr._vfUrl || '';
        if (/\.mp4|\.m4s|\.m4v|\.mpd|dms\.licdn|\/dash\/|\/hls\//i.test(url)) {
            _capturedUrls.push({ url: url, ct: '', ts: Date.now() });
        }
        xhr.addEventListener('load', function() {
            try {
                var ct = (xhr.getResponseHeader('content-type') || '').toLowerCase();
                if (/video\/mp4|application\/dash|mpd/i.test(ct)) {
                    _capturedUrls.push({ url: url, ct: ct, ts: Date.now() });
                }
            } catch(e) {}
        });
        return _realXHRSend.apply(this, arguments);
    };

    // ════════════════════════════════════════════════════════════
    // F. DOM fallback: extract progressive MP4 URLs from page data
    // ════════════════════════════════════════════════════════════
    function _extractPageUrls() {
        var urls = [];
        // <source> tags
        document.querySelectorAll('source[src]').forEach(function(s) {
            var src = s.getAttribute('src') || '';
            var t   = (s.getAttribute('type') || '').toLowerCase();
            if (t.indexOf('mp4') !== -1 || /\.mp4/i.test(src)) urls.push(src);
        });
        // data attributes
        document.querySelectorAll('[data-src],[data-video-url],[data-media-url],[data-sources]').forEach(function(el) {
            ['data-src','data-video-url','data-media-url'].forEach(function(a) {
                var v = el.getAttribute(a);
                if (v && /\.mp4|video/i.test(v)) urls.push(v);
            });
            var ds = el.getAttribute('data-sources');
            if (ds) try { var arr = JSON.parse(ds); if (Array.isArray(arr)) arr.forEach(function(i){ if(i.src) urls.push(i.src); }); } catch(e){}
        });
        // JSON blobs in <script> and <code>
        var jsonEls = document.querySelectorAll('script[type*="json"], code');
        jsonEls.forEach(function(el) {
            try {
                var text = el.textContent || '';
                if (text.length > 5000000) return; // skip huge blobs
                var re = /https?:\/\/[^\s"'<>]+?\.mp4[^\s"'<>]*/g;
                var m;
                while ((m = re.exec(text)) !== null) {
                    var u = m[0].replace(/\\u002F/g, '/').replace(/\\/g, '');
                    if (urls.indexOf(u) === -1) urls.push(u);
                }
                // LinkedIn dms.licdn.com playlist URLs (no .mp4 extension)
                var re2 = /https?:\/\/dms\.licdn\.com\/playlist\/[^\s"'<>]+/g;
                while ((m = re2.exec(text)) !== null) {
                    var u2 = m[0].replace(/\\u002F/g, '/').replace(/\\/g, '');
                    if (urls.indexOf(u2) === -1) urls.push(u2);
                }
            } catch(e) {}
        });
        // Check existing video elements' src / currentSrc.
        document.querySelectorAll('video').forEach(function(v) {
            var vs = v._cbOriginalSrc || v.getAttribute('src') || '';
            if (vs && /^https?:\/\//i.test(vs) && urls.indexOf(vs) === -1) urls.push(vs);
            var vcs = v.currentSrc || '';
            if (vcs && /^https?:\/\//i.test(vcs) && urls.indexOf(vcs) === -1) urls.push(vcs);
        });
        // Captured from network
        _capturedUrls.forEach(function(c) {
            var cu = c.url;
            if (urls.indexOf(cu) !== -1) return;
            // Include .mp4 URLs and LinkedIn CDN URLs (dms.licdn.com
            // playlist URLs often lack a .mp4 extension).
            if (/\.mp4/i.test(cu)
                || /dms\.licdn\.com|media\.licdn\.com/i.test(cu)) {
                urls.push(cu);
            }
        });
        // Sort: prefer higher resolution.
        urls.sort(function(a, b) {
            var ra = (/(\d{3,4})p/.exec(a)||[])[1]||0;
            var rb = (/(\d{3,4})p/.exec(b)||[])[1]||0;
            return Number(rb) - Number(ra);
        });
        return urls;
    }

    // ════════════════════════════════════════════════════════════
    // G. Monitor all <video> elements for errors and fix them
    // ════════════════════════════════════════════════════════════
    // Dispose a Video.js player instance if one owns this <video> element.
    // This prevents the "Using the tech directly can be dangerous" warning.
    function _disposeVideoJS(video) {
        try {
            // Video.js stores the player ID on the element or its wrapper.
            var playerId = video.id || (video.parentElement && video.parentElement.id);
            if (playerId && typeof window.videojs === 'function') {
                var player = window.videojs.getPlayer(playerId);
                if (player) {
                    player.dispose();
                    return true;
                }
            }
            // Fallback: check the Video.js player cache attached to the element.
            if (video.player && typeof video.player.dispose === 'function') {
                video.player.dispose();
                return true;
            }
        } catch(e) {}
        return false;
    }

    function _hideOverlays(video) {
        var wrap = video.closest('.video-js, .video-player, .media-player, [class*="video-player"], [class*="VideoPlayer"]');
        if (!wrap) return;
        wrap.querySelectorAll(
            '.vjs-error-display, .vjs-error, .vjs-modal-dialog, ' +
            '.vjs-loading-spinner, [class*="error-display"], [class*="ErrorDisplay"]'
        ).forEach(function(el) {
            if (!el.contains(video)) el.style.display = 'none';
        });
        // Remove 'vjs-error' class from the wrapper so controls show.
        wrap.classList.remove('vjs-error');
    }

    function _tryDirectUrl(video) {
        if (video._directFixed) return;
        var urls = _extractPageUrls();
        // Also check the video's own src / currentSrc.  LinkedIn
        // may set video.src to a direct URL that _extractPageUrls
        // does not find in the DOM.
        var ownSrc = video._cbOriginalSrc
            || video.getAttribute('src') || video.currentSrc || '';
        if (ownSrc && /^https?:\/\//i.test(ownSrc)
            && ownSrc.indexOf('cbproxy:') !== 0
            && urls.indexOf(ownSrc) === -1) {
            urls.unshift(ownSrc);
        }
        if (!urls.length) return;
        console.info('[CB] _tryDirectUrl: found', urls.length, 'candidate(s),',
            'first:', (urls[0] || '').substring(0, 120));
        video._directFixed = true;
        _disposeVideoJS(video);
        video.pause();
        try { video.removeAttribute('src'); video.load(); } catch(e) {}
        video.querySelectorAll('source').forEach(function(s){ s.remove(); });

        // Use local transcoding proxy on whitelisted domains, or the
        // remote HTTPS pass-through proxy on other HTTPS pages.
        var proxied = _makeProxyUrl(urls[0]);
        if (proxied !== urls[0]) {
            video.src = proxied;
            // If the proxy URL also gets blocked by a strict media-src,
            // fall back to the direct URL on error.
            video.addEventListener('error', function _pe() {
                video.removeEventListener('error', _pe);
                if (!video._directFallback) {
                    video._directFallback = true;
                    video.src = urls[0];
                    video.play().catch(function(){});
                }
            });
        } else {
            video.src = urls[0];
        }
        video.controls = true;
        video.setAttribute('playsinline', '');
        video.play().catch(function(){});
        _hideOverlays(video);
    }

    function _monitorVideo(video) {
        if (video._vfMonitored) return;
        video._vfMonitored = true;

        // If after 4 seconds the video has an error or never loaded, try
        // the direct-URL fallback.
        setTimeout(function() {
            if (video.error || video.networkState === 3 ||
                (video.readyState === 0 && !video.paused)) {
                _tryDirectUrl(video);
            }
        }, 4000);

        // Stuck-video detector: if after 6 seconds the video has not
        // advanced past 0.5s of playback, it is likely a black/spinning
        // screen caused by our FakeSourceBuffer collecting H.264 data
        // that cannot be decoded.  Force a transcode flush.
        var _stuckCheckStart = Date.now();
        var _stuckTimer = setInterval(function() {
            var elapsed = Date.now() - _stuckCheckStart;
            // Stop checking after 30 seconds.
            if (elapsed > 30000) { clearInterval(_stuckTimer); return; }
            // Only act after 6 seconds.
            if (elapsed < 6000) return;
            // If the video is actually playing fine, stop.
            if (video.currentTime > 0.5 && !video.error) {
                clearInterval(_stuckTimer);
                return;
            }
            // If the video has a blob: src (MSE) and currentTime is
            // near zero, it is stuck.  Try the direct-URL fallback.
            var src = video.src || video.currentSrc || '';
            if ((src.indexOf('blob:') === 0 || video.readyState < 3)
                && !video._directFixed) {
                console.info('[CB] Stuck video detected: currentTime='
                    + video.currentTime + ' readyState=' + video.readyState
                    + ' src=' + src.substring(0, 80));
                clearInterval(_stuckTimer);
                _tryDirectUrl(video);
            }
        }, 2000);

        video.addEventListener('error', function() {
            setTimeout(function(){ _tryDirectUrl(video); }, 200);
        });
    }

    // Capture phase error listener for <video> and <source> elements.
    document.addEventListener('error', function(e) {
        var el = e.target;
        if (!el) return;
        if (el.tagName === 'VIDEO') {
            setTimeout(function(){ _tryDirectUrl(el); }, 200);
        }
        if (el.tagName === 'SOURCE' && el.parentElement && el.parentElement.tagName === 'VIDEO') {
            setTimeout(function(){ _tryDirectUrl(el.parentElement); }, 200);
        }
    }, true);

    // Watch for dynamically added <video> elements (SPAs like LinkedIn).
    var _obs = new MutationObserver(function(muts) {
        muts.forEach(function(m) {
            m.addedNodes.forEach(function(n) {
                if (n.nodeType !== 1) return;
                if (n.tagName === 'VIDEO') _monitorVideo(n);
                if (n.querySelectorAll) n.querySelectorAll('video').forEach(_monitorVideo);
            });
        });
    });
    // At DocumentCreation time, document.documentElement may not exist yet.
    // Defer the observe call until a root node is available.
    function _startObserving() {
        var root = document.documentElement || document.body;
        if (root) {
            _obs.observe(root, { childList: true, subtree: true });
            document.querySelectorAll('video').forEach(_monitorVideo);
        } else {
            // No DOM root yet; retry on the next microtask.
            setTimeout(_startObserving, 0);
        }
    }
    _startObserving();
})();
"""


_SPECULATION_RULES_JS = r"""
(function() {
    'use strict';
    if (!HTMLScriptElement.supports || !HTMLScriptElement.supports('speculationrules')) return;
    if (window.__xcmSpecRulesLoaded__) return;
    window.__xcmSpecRulesLoaded__ = true;

    function _injectRules() {
        var links = document.querySelectorAll('a[href]');
        var urls = [];
        var origin = location.origin;
        var current = location.href.split('#')[0];
        for (var i = 0; i < links.length; i++) {
            var href = links[i].href;
            if (!href || href.indexOf(origin) !== 0) continue;
            if (href.split('#')[0] === current) continue;
            if (urls.indexOf(href) === -1 && urls.length < 12) urls.push(href);
        }
        if (!urls.length) return;
        var old = document.getElementById('__xcmSpecRules__');
        if (old) old.remove();
        var s = document.createElement('script');
        s.id = '__xcmSpecRules__';
        s.type = 'speculationrules';
        s.textContent = JSON.stringify({
            prefetch: [{ source: 'list', eagerness: 'moderate', urls: urls }]
        });
        document.head.appendChild(s);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', _injectRules, { once: true });
    } else {
        setTimeout(_injectRules, 500);
    }

    // Re-scan after SPA route changes swap the link list.
    var _specTimer = null;
    var _specObs = new MutationObserver(function() {
        if (_specTimer) clearTimeout(_specTimer);
        _specTimer = setTimeout(_injectRules, 1200);
    });
    function _observe() {
        if (document.body) {
            _specObs.observe(document.body, { childList: true, subtree: true });
        } else {
            setTimeout(_observe, 100);
        }
    }
    _observe();
})();
"""
