/**
 * auth-mask.js  --  Puppeteer automation signal removal
 *
 * Injected via Page.addScriptToEvaluateOnNewDocument (evaluateOnNewDocument)
 * before any page script runs.
 *
 * When Chrome launches under Puppeteer the automation stack leaves several
 * fingerprint signals that Google / Cloudflare / Stripe read to block OAuth:
 *
 *   navigator.webdriver          -- set to true by ChromeDriver / Puppeteer
 *   window.cdc_*                 -- ChromeDriver content script globals
 *   Function.prototype.toString  -- overridden functions leak source code
 *                                   instead of "[native code]"
 *   document.hasFocus()          -- returns false when window is minimized
 *   document.visibilityState     -- returns "hidden" when minimized
 *   window.outerWidth/Height     -- 0 when minimized, -32000 screenX
 *   navigator.permissions.query  -- returns "denied" in automation mode
 *   WebSocket to CDP port        -- page scripts can probe the debug port
 *   Error.stack                  -- contains puppeteer/node_modules paths
 *
 * None of these are present in a user-launched Chrome window.
 *
 * Security model
 * --------------
 * These overrides only affect the JavaScript environment of the loaded page;
 * they do not alter Chrome's actual security sandbox, certificate validation,
 * or network stack.
 */
(function (global) {
    'use strict';

    if (global.__xcmAuthMaskLoaded) return;

    // ========================================================================
    // SECTION 0:  _makeNative  --  Function.prototype.toString defense
    // ========================================================================
    // When we override navigator.permissions.query, getContext, etc., calling
    // .toString() on them returns JS source code instead of "[native code]".
    // Cloudflare's fingerprinting script does exactly this check on dozens of
    // functions.  This is the #1 detection vector for function overrides.
    //
    // _makeNative(fn, name) patches fn so that fn.toString() and
    // Function.prototype.toString.call(fn) both return
    //   "function <name>() { [native code] }"
    // matching what a real browser returns for built-in functions.
    //
    // The approach: store a WeakMap of patched functions -> fake toString
    // strings, then replace Function.prototype.toString once.

    var _nativeStrings = new WeakMap();
    var _origToString  = Function.prototype.toString;
    var _toStringPatched = false;

    function _makeNative(fn, name) {
        if (typeof fn !== 'function') return fn;
        var fakeName = name || fn.name || '';
        _nativeStrings.set(fn, 'function ' + fakeName + '() { [native code] }');
        if (!_toStringPatched) {
            _toStringPatched = true;
            // Replace toString once; the replacement itself must also look native.
            var _replacement = function toString() {
                var str = _nativeStrings.get(this);
                if (str) return str;
                return _origToString.call(this);
            };
            _nativeStrings.set(_replacement, 'function toString() { [native code] }');
            Function.prototype.toString = _replacement;
        }
        // Also set the length and name properties to match the original.
        try {
            Object.defineProperty(fn, 'length', { value: fn.length, configurable: true });
            Object.defineProperty(fn, 'name',   { value: fakeName, configurable: true });
        } catch (_) {}
        return fn;
    }

    // Helper: defineProperty with a getter that looks native.
    function _defProp(obj, prop, getter) {
        _makeNative(getter, 'get ' + prop);
        Object.defineProperty(obj, prop, { get: getter, configurable: true });
    }

    // ========================================================================
    // SECTION 1:  navigator.webdriver
    // ========================================================================
    try {
        Object.defineProperty(navigator, 'webdriver', {
            get: function () { return undefined; },
            configurable: true,
        });
    } catch (_) {}
    try { delete Object.getPrototypeOf(navigator).webdriver; } catch (_) {}

    // ========================================================================
    // SECTION 2:  window.chrome
    // ========================================================================
    if (!global.chrome) { global.chrome = {}; }
    if (!global.chrome.runtime) {
        var _rt = {
            connect: _makeNative(function () {}, 'connect'),
            sendMessage: _makeNative(function () {}, 'sendMessage'),
            onMessage:   { addListener: function () {}, removeListener: function () {} },
            onConnect:   { addListener: function () {}, removeListener: function () {} },
            onInstalled: { addListener: function () {} },
            getManifest: _makeNative(function () { return {}; }, 'getManifest'),
            getURL:      _makeNative(function (p) { return 'chrome-extension://' + p; }, 'getURL'),
            PlatformOs: { MAC: 'mac', WIN: 'win', ANDROID: 'android',
                          CROS: 'cros', LINUX: 'linux', OPENBSD: 'openbsd' },
        };
        global.chrome.runtime = _rt;
    }
    if (!global.chrome.loadTimes) {
        global.chrome.loadTimes = _makeNative(function () { return {}; }, 'loadTimes');
    }
    if (!global.chrome.csi) {
        global.chrome.csi = _makeNative(function () {
            return { startE: Date.now(), onloadT: Date.now(), pageT: 0, tran: 15 };
        }, 'csi');
    }
    if (!global.chrome.app) {
        global.chrome.app = {
            isInstalled: false,
            getDetails:     _makeNative(function () { return null; }, 'getDetails'),
            getIsInstalled: _makeNative(function () { return false; }, 'getIsInstalled'),
            installState:   _makeNative(function (cb) { cb && cb({ state: 'not_installed' }); }, 'installState'),
            runningState:   _makeNative(function () { return 'cannot_run'; }, 'runningState'),
        };
    }

    // ========================================================================
    // SECTION 3:  navigator.plugins / mimeTypes
    // ========================================================================
    try {
        var fakePlugins = [
            { name: 'Chrome PDF Plugin',  filename: 'internal-pdf-viewer',
              description: 'Portable Document Format', length: 1,
              0: { type: 'application/x-google-chrome-pdf', suffixes: 'pdf',
                   description: 'Portable Document Format', enabledPlugin: null } },
            { name: 'Chrome PDF Viewer',  filename: 'mhjfbmdgcfjbbpaeojofohoefgiehjai',
              description: '', length: 1,
              0: { type: 'application/pdf', suffixes: 'pdf',
                   description: '', enabledPlugin: null } },
            { name: 'Native Client', filename: 'internal-nacl-plugin',
              description: '', length: 2,
              0: { type: 'application/x-nacl',  suffixes: '', description: '', enabledPlugin: null },
              1: { type: 'application/x-pnacl', suffixes: '', description: '', enabledPlugin: null } },
        ];
        fakePlugins.item = _makeNative(function (i) { return this[i] || null; }, 'item');
        fakePlugins.namedItem = _makeNative(function (n) {
            for (var j = 0; j < 3; j++) { if (fakePlugins[j].name === n) return fakePlugins[j]; }
            return null;
        }, 'namedItem');
        fakePlugins.refresh = _makeNative(function () {}, 'refresh');
        fakePlugins.length  = 3;
        if (!navigator.plugins || navigator.plugins.length === 0) {
            _defProp(navigator, 'plugins', function () { return fakePlugins; });
        }
        var fakeMime = [
            { type: 'application/pdf', suffixes: 'pdf', description: '', enabledPlugin: fakePlugins[1] },
            { type: 'application/x-google-chrome-pdf', suffixes: 'pdf',
              description: 'Portable Document Format', enabledPlugin: fakePlugins[0] },
            { type: 'application/x-nacl',  suffixes: '', description: '', enabledPlugin: fakePlugins[2] },
            { type: 'application/x-pnacl', suffixes: '', description: '', enabledPlugin: fakePlugins[2] },
        ];
        fakeMime.item = _makeNative(function (i) { return this[i] || null; }, 'item');
        fakeMime.namedItem = _makeNative(function (n) {
            for (var j = 0; j < 4; j++) { if (fakeMime[j].type === n) return fakeMime[j]; }
            return null;
        }, 'namedItem');
        _defProp(navigator, 'mimeTypes', function () { return fakeMime; });
    } catch (_) {}

    // ========================================================================
    // SECTION 4:  Languages
    // ========================================================================
    try {
        _defProp(navigator, 'languages', function () { return ['en-US', 'en']; });
        _defProp(navigator, 'language',  function () { return 'en-US'; });
    } catch (_) {}

    // ========================================================================
    // SECTION 5:  Platform / hardware
    // ========================================================================
    try {
        _defProp(navigator, 'platform',             function () { return 'MacIntel'; });
        _defProp(navigator, 'vendor',               function () { return 'Google Inc.'; });
        _defProp(navigator, 'hardwareConcurrency',  function () { return 8; });
        _defProp(navigator, 'deviceMemory',         function () { return 8; });
        _defProp(navigator, 'maxTouchPoints',       function () { return 0; });
    } catch (_) {}

    // ========================================================================
    // SECTION 6:  navigator.permissions
    // ========================================================================
    try {
        if (navigator.permissions && navigator.permissions.query) {
            var _origQuery = navigator.permissions.query.bind(navigator.permissions);
            var _patchedQuery = _makeNative(function query(desc) {
                if (desc && desc.name === 'notifications') {
                    return Promise.resolve({ state: 'prompt', onchange: null });
                }
                return _origQuery(desc).catch(function () {
                    return { state: 'prompt', onchange: null };
                });
            }, 'query');
            navigator.permissions.query = _patchedQuery;
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 7:  Notification.permission
    // ========================================================================
    try {
        if (typeof Notification !== 'undefined' && Notification.permission === 'denied') {
            _defProp(Notification, 'permission', function () { return 'default'; });
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 8:  navigator.connection
    // ========================================================================
    try {
        if (!navigator.connection) {
            _defProp(navigator, 'connection', function () {
                return { effectiveType: '4g', rtt: 50, downlink: 10, saveData: false,
                         onchange: null,
                         addEventListener: _makeNative(function () {}, 'addEventListener'),
                         removeEventListener: _makeNative(function () {}, 'removeEventListener') };
            });
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 9:  navigator.getBattery
    // ========================================================================
    try {
        if (!navigator.getBattery) {
            navigator.getBattery = _makeNative(function getBattery() {
                return Promise.resolve({
                    charging: true, chargingTime: 0, dischargingTime: Infinity, level: 1,
                    addEventListener: _makeNative(function () {}, 'addEventListener'),
                    removeEventListener: _makeNative(function () {}, 'removeEventListener'),
                });
            }, 'getBattery');
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 10:  WebGL renderer / vendor
    // ========================================================================
    try {
        var _origGetContext = HTMLCanvasElement.prototype.getContext;
        HTMLCanvasElement.prototype.getContext = _makeNative(function getContext(type, attrs) {
            var ctx = _origGetContext.call(this, type, attrs);
            if (ctx && (type === 'webgl' || type === 'webgl2' ||
                        type === 'experimental-webgl')) {
                var ext = ctx.getExtension('WEBGL_debug_renderer_info');
                if (ext) {
                    var _origGetParam = ctx.getParameter.bind(ctx);
                    ctx.getParameter = _makeNative(function getParameter(p) {
                        if (p === ext.UNMASKED_VENDOR_WEBGL)   return 'Google Inc. (Apple)';
                        if (p === ext.UNMASKED_RENDERER_WEBGL) return 'ANGLE (Apple, Apple M1, OpenGL 4.1)';
                        return _origGetParam(p);
                    }, 'getParameter');
                }
            }
            return ctx;
        }, 'getContext');
    } catch (_) {}

    // ========================================================================
    // SECTION 11:  navigator.userAgentData
    // ========================================================================
    // Strip HeadlessChrome brands and provide a real getHighEntropyValues
    // response instead of an empty object (which is an instant detection).
    try {
        var _uaBrands = [
            { brand: 'Google Chrome',  version: '131' },
            { brand: 'Chromium',       version: '131' },
            { brand: 'Not_A Brand',    version: '24'  },
        ];
        var _fullUaBrands = [
            { brand: 'Google Chrome',  version: '131.0.6778.86' },
            { brand: 'Chromium',       version: '131.0.6778.86' },
            { brand: 'Not_A Brand',    version: '24.0.0.0'      },
        ];
        var _uaData = {
            brands: _uaBrands,
            mobile: false,
            platform: 'macOS',
            getHighEntropyValues: _makeNative(function getHighEntropyValues(hints) {
                return Promise.resolve({
                    brands:              _uaBrands,
                    fullVersionList:     _fullUaBrands,
                    mobile:              false,
                    platform:            'macOS',
                    platformVersion:     '14.4.0',
                    architecture:        'arm',
                    bitness:             '64',
                    model:               '',
                    uaFullVersion:       '131.0.6778.86',
                    wow64:               false,
                });
            }, 'getHighEntropyValues'),
            toJSON: _makeNative(function toJSON() {
                return { brands: _uaBrands, mobile: false, platform: 'macOS' };
            }, 'toJSON'),
        };
        _defProp(navigator, 'userAgentData', function () { return _uaData; });
    } catch (_) {}

    // ========================================================================
    // SECTION 12:  Screen metrics (unconditional)
    // ========================================================================
    // Always override, not just when width===0. When the window is minimized
    // or off-screen, macOS may report correct screen dimensions but
    // outerWidth/outerHeight/screenX/screenY become abnormal.
    try {
        _defProp(screen, 'width',       function () { return 1920; });
        _defProp(screen, 'height',      function () { return 1080; });
        _defProp(screen, 'availWidth',  function () { return 1920; });
        _defProp(screen, 'availHeight', function () { return 1055; });
        _defProp(screen, 'colorDepth',  function () { return 24; });
        _defProp(screen, 'pixelDepth',  function () { return 24; });
    } catch (_) {}

    // ========================================================================
    // SECTION 13:  document.hasFocus() / visibilityState / hidden
    // ========================================================================
    // When Chrome is minimized or off-screen:
    //   document.hasFocus()       -> false   (should be true)
    //   document.visibilityState  -> "hidden" (should be "visible")
    //   document.hidden           -> true    (should be false)
    // Cloudflare Turnstile and Google use all three as bot signals.
    try {
        document.hasFocus = _makeNative(function hasFocus() { return true; }, 'hasFocus');
        _defProp(document, 'visibilityState', function () { return 'visible'; });
        _defProp(document, 'hidden',          function () { return false; });
        _defProp(document, 'webkitHidden',    function () { return false; });
        // Suppress the visibilitychange event so page scripts never learn
        // the window was minimized.
        var _origAddEventListener = document.addEventListener.bind(document);
        document.addEventListener = _makeNative(function addEventListener(type, fn, opts) {
            if (type === 'visibilitychange' || type === 'webkitvisibilitychange') {
                return; // silently drop the listener
            }
            return _origAddEventListener(type, fn, opts);
        }, 'addEventListener');
    } catch (_) {}

    // ========================================================================
    // SECTION 14:  window.outerWidth / outerHeight / screenX / screenY
    // ========================================================================
    // When minimized: outerWidth and outerHeight can be 0 or very small.
    // When positioned at -32000,0: screenX exposes the off-screen position.
    // Both are checked by Cloudflare and Stripe fingerprinting.
    try {
        _defProp(global, 'outerWidth',  function () { return 1280; });
        _defProp(global, 'outerHeight', function () { return 900; });
        _defProp(global, 'screenX',     function () { return 0; });
        _defProp(global, 'screenY',     function () { return 25; });
        _defProp(global, 'screenLeft',  function () { return 0; });
        _defProp(global, 'screenTop',   function () { return 25; });
        // innerWidth/innerHeight should match viewport or close to outerWidth.
        // Only override if they look abnormal (0 or negative).
        if (global.innerWidth <= 0 || global.innerHeight <= 0) {
            _defProp(global, 'innerWidth',  function () { return 1280; });
            _defProp(global, 'innerHeight', function () { return 850; });
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 15:  Speech synthesis voices
    // ========================================================================
    try {
        if (global.speechSynthesis) {
            var _origGetVoices = global.speechSynthesis.getVoices.bind(global.speechSynthesis);
            global.speechSynthesis.getVoices = _makeNative(function getVoices() {
                var v = _origGetVoices();
                return v && v.length ? v : [{ voiceURI: 'Samantha', name: 'Samantha',
                    lang: 'en-US', localService: true, default: true }];
            }, 'getVoices');
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 16:  Iframe contentWindow chrome patching
    // ========================================================================
    try {
        var _origCreateElement = document.createElement.bind(document);
        document.createElement = _makeNative(function createElement(tag) {
            var el = _origCreateElement(tag);
            if (typeof tag === 'string' && tag.toLowerCase() === 'iframe') {
                el.addEventListener('load', function () {
                    try {
                        if (el.contentWindow && !el.contentWindow.chrome) {
                            el.contentWindow.chrome = global.chrome;
                        }
                    } catch (_) {}
                });
            }
            return el;
        }, 'createElement');
    } catch (_) {}

    // ========================================================================
    // SECTION 17:  ChromeDriver $cdc_ globals
    // ========================================================================
    (function removeCdcGlobals() {
        var keys = Object.getOwnPropertyNames(global);
        for (var i = 0; i < keys.length; i++) {
            if (keys[i].indexOf('cdc_') === 0 || keys[i].indexOf('$cdc_') === 0) {
                try { delete global[keys[i]]; } catch (_) {}
            }
        }
    })();

    // ========================================================================
    // SECTION 18:  CDP / DevTools tracing symbols on document
    // ========================================================================
    try {
        var docKeys = Object.getOwnPropertyNames(document);
        for (var k = 0; k < docKeys.length; k++) {
            if (docKeys[k].indexOf('cdc') !== -1 || docKeys[k].indexOf('__driver') !== -1) {
                try { delete document[docKeys[k]]; } catch (_) {}
            }
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 19:  Error.stack sanitization
    // ========================================================================
    // Puppeteer paths in stack traces (node_modules/puppeteer-core, pptr:evaluate)
    // are a dead giveaway.  Also sanitize qtwebengine paths for QWebEngine builds.
    try {
        var _origCaptureStack = Error.captureStackTrace;
        if (_origCaptureStack) {
            Error.captureStackTrace = _makeNative(function captureStackTrace(obj, fn) {
                _origCaptureStack(obj, fn);
                if (obj.stack) {
                    obj.stack = obj.stack
                        .replace(/pptr:/g, 'chrome-extension:')
                        .replace(/puppeteer/gi, 'chromium')
                        .replace(/qtwebengine/gi, 'chromium')
                        .replace(/node_modules\//g, '')
                        .replace(/__puppeteer_evaluation_script__/g, '<anonymous>');
                }
            }, 'captureStackTrace');
        }
        // Also patch Error.prepareStackTrace for V8-based engines.
        var _origPrepare = Error.prepareStackTrace;
        Error.prepareStackTrace = function (err, frames) {
            if (_origPrepare) {
                var result = _origPrepare(err, frames);
                if (typeof result === 'string') {
                    return result
                        .replace(/pptr:/g, 'chrome-extension:')
                        .replace(/puppeteer/gi, 'chromium')
                        .replace(/__puppeteer_evaluation_script__/g, '<anonymous>');
                }
                return result;
            }
            return err.toString();
        };
    } catch (_) {}

    // ========================================================================
    // SECTION 20:  Block WebSocket connections to CDP debugging port
    // ========================================================================
    // Page scripts can probe for the CDP WebSocket on localhost:9223 to
    // confirm Puppeteer is running.  Override WebSocket to reject connections
    // to the debugging port.
    try {
        var _OrigWebSocket = global.WebSocket;
        var _CDP_PORT_RE = /^wss?:\/\/(localhost|127\.0\.0\.1):(9222|9223|9224|9225)/;
        global.WebSocket = _makeNative(function WebSocket(url, protocols) {
            if (_CDP_PORT_RE.test(url)) {
                throw new DOMException(
                    "Failed to construct 'WebSocket': The URL '" + url + "' is invalid.",
                    'SyntaxError'
                );
            }
            if (protocols !== undefined) {
                return new _OrigWebSocket(url, protocols);
            }
            return new _OrigWebSocket(url);
        }, 'WebSocket');
        global.WebSocket.prototype = _OrigWebSocket.prototype;
        global.WebSocket.CONNECTING = 0;
        global.WebSocket.OPEN = 1;
        global.WebSocket.CLOSING = 2;
        global.WebSocket.CLOSED = 3;
    } catch (_) {}

    // ========================================================================
    // SECTION 21:  OffscreenCanvas.convertToBlob
    // ========================================================================
    // Missing convertToBlob is a headless detection signal.
    try {
        if (typeof OffscreenCanvas !== 'undefined' &&
            !OffscreenCanvas.prototype.convertToBlob) {
            OffscreenCanvas.prototype.convertToBlob = _makeNative(
                function convertToBlob(opts) {
                    return Promise.resolve(new Blob([], { type: (opts && opts.type) || 'image/png' }));
                }, 'convertToBlob'
            );
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 22:  performance.memory normalization
    // ========================================================================
    // In automation, jsHeapSizeLimit and other values can be abnormal.
    try {
        if (global.performance && global.performance.memory) {
            var _mem = global.performance.memory;
            _defProp(global.performance, 'memory', function () {
                return {
                    jsHeapSizeLimit:  _mem.jsHeapSizeLimit || 2330000000,
                    totalJSHeapSize:  _mem.totalJSHeapSize || 35000000,
                    usedJSHeapSize:   _mem.usedJSHeapSize  || 25000000,
                };
            });
        }
    } catch (_) {}

    // ========================================================================
    // SECTION 23:  Prevent CDP target discovery via fetch/XHR
    // ========================================================================
    // Some fingerprinters try fetch('http://localhost:9222/json') to discover
    // CDP targets. Block requests to common debugging ports.
    try {
        var _origFetch = global.fetch;
        global.fetch = _makeNative(function fetch(input, init) {
            var url = (typeof input === 'string') ? input : (input && input.url) || '';
            if (_CDP_PORT_RE.test(url) ||
                /^https?:\/\/(localhost|127\.0\.0\.1):(9222|9223|9224|9225)/.test(url)) {
                return Promise.reject(new TypeError('Failed to fetch'));
            }
            return _origFetch.call(global, input, init);
        }, 'fetch');
    } catch (_) {}

    // ========================================================================
    // SECTION 24:  Proxy/Reflect detection defense
    // ========================================================================
    // Some detectors check if Proxy is used by testing:
    //   Reflect.apply(Function.prototype.toString, fn, [])
    // Our _makeNative WeakMap approach already handles this because we
    // replace Function.prototype.toString at the prototype level, so
    // Reflect.apply uses our replacement.  Verify Reflect exists and
    // that its .apply is not tampered with:
    try {
        if (typeof Reflect !== 'undefined' && Reflect.apply) {
            // Ensure our toString works through Reflect.apply too.
            _makeNative(Reflect.apply, 'apply');
        }
    } catch (_) {}

    // ========================================================================
    // Mark as loaded so re-injection is a no-op.
    // ========================================================================
    Object.defineProperty(global, '__xcmAuthMaskLoaded', {
        value: true, writable: false, enumerable: false, configurable: false
    });

})(window);
