"""
Shared JavaScript constants used by both the QtWebEngine and WKWebView backends.
Extracted here to avoid circular imports with QtWebEngineWidgets.
"""

# Cookie consent banner killer -- injected at document end.
CONSENT_KILL_JS = r"""
(function(){
    var HIDE_SELECTORS = [
        '#onetrust-consent-sdk', '#onetrust-banner-sdk',
        '#onetrust-pc-sdk', '.onetrust-pc-dark-filter',
        '#CybotCookiebotDialog', '#CybotCookiebotDialogBodyUnderlay',
        '#cookie-banner', '#cookie-notice', '#cookie-bar',
        '#cookie-consent', '#cookie-policy-banner',
        '#cookiebanner', '#cookieConsent', '#cookieNotice',
        '.cookie-banner', '.cookie-notice', '.cookie-bar',
        '.cookie-consent', '.cookie-popup', '.cookie-modal',
        '.cookiebanner', '.cookieConsent', '.cookieNotice',
        '#gdpr-banner', '#gdpr-notice', '#gdpr-consent',
        '.gdpr-banner', '.gdpr-notice', '.gdpr-consent',
        '#truste-consent-track', '#truste-show-consent',
        '.truste_overlay', '.truste_box_overlay',
        '#qc-cmp2-container', '#qc-cmp2-ui',
        '#didomi-host', '#didomi-notice',
        '#cookie-law-info-bar', '.cli-modal-backdrop',
        '#cmplz-cookiebanner', '.cmplz-overlay',
        '#termly-code-snippet-support',
        '#cmp-container', '#cmpbox',
        '#iubenda-cs-banner',
        '#cookieinfo',
        '.cc-window', '.cc-banner', '.cc-overlay',
        '#BorlabsCookieBox',
        '.osano-cm-window',
        '#cookie_notice',
        '#cookie-notice',
        '.cookie-overlay', '.consent-overlay',
        '.privacy-overlay', '.privacy-popup',
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
                _snapStyle.textContent = '* { scroll-snap-type: none !important; overscroll-behavior: auto !important; scroll-behavior: auto !important; }';
                (document.head || document.documentElement).appendChild(_snapStyle);
            }
        } catch(e) {}
    }

    removeNodes();

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


# Stealth JS -- injected before any page script.
STEALTH_JS = r"""
(function(){
    Object.defineProperty(navigator, 'webdriver', {get: () => undefined});
    try { delete Object.getPrototypeOf(navigator).webdriver; } catch(e){}

    if (!window.chrome) { window.chrome = {}; }
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

    Object.defineProperty(navigator, 'languages', {get: () => ['en-US','en']});
    Object.defineProperty(navigator, 'language',  {get: () => 'en-US'});

    Object.defineProperty(navigator, 'platform',          {get: () => 'MacIntel'});
    Object.defineProperty(navigator, 'hardwareConcurrency',{get: () => 8});
    Object.defineProperty(navigator, 'deviceMemory',      {get: () => 8});
    Object.defineProperty(navigator, 'maxTouchPoints',    {get: () => 0});
    Object.defineProperty(navigator, 'vendor',            {get: () => 'Google Inc.'});

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

    if (!navigator.connection) {
        Object.defineProperty(navigator, 'connection', {
            get: () => ({effectiveType:'4g', rtt:50, downlink:10, saveData:false,
                         onchange:null, addEventListener:function(){},
                         removeEventListener:function(){}})
        });
    }

    if (!navigator.getBattery) {
        navigator.getBattery = function(){
            return Promise.resolve({
                charging:true, chargingTime:0, dischargingTime:Infinity, level:1,
                addEventListener:function(){}, removeEventListener:function(){}
            });
        };
    }

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

    if (screen.width === 0 || screen.height === 0) {
        Object.defineProperty(screen, 'width',      {get: () => 1920});
        Object.defineProperty(screen, 'height',     {get: () => 1080});
        Object.defineProperty(screen, 'availWidth',  {get: () => 1920});
        Object.defineProperty(screen, 'availHeight', {get: () => 1080});
        Object.defineProperty(screen, 'colorDepth',  {get: () => 24});
        Object.defineProperty(screen, 'pixelDepth',  {get: () => 24});
    }

    var origCreateElement = document.createElement.bind(document);
    document.createElement = function(tag){
        var el = origCreateElement(tag);
        if (tag.toLowerCase() === 'iframe') {
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

    try {
        if (typeof Notification !== 'undefined' && Notification.permission === 'denied') {
            Object.defineProperty(Notification, 'permission', {get: () => 'default'});
        }
    } catch(e){}

    if (typeof OffscreenCanvas !== 'undefined') {
        if (!OffscreenCanvas.prototype.convertToBlob) {
            OffscreenCanvas.prototype.convertToBlob = function(opts){
                return Promise.resolve(new Blob([], {type: opts && opts.type || 'image/png'}));
            };
        }
    }

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
})();
"""
