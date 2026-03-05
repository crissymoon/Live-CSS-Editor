/**
 * auth-mask.js  --  Puppeteer automation signal removal
 *
 * Injected via Page.addScriptToEvaluateOnNewDocument (evaluateOnNewDocument)
 * before any page script runs.
 *
 * When Chrome launches under Puppeteer the automation stack leaves several
 * fingerprint signals that Google reads to block OAuth sign-in:
 *
 *   navigator.webdriver          -- set to true by ChromeDriver / Puppeteer
 *   window.cdc_*                 -- ChromeDriver content script globals
 *   navigator.permissions.query  -- returns "denied" for notifications in
 *                                    headless/automation mode
 *   document.$cdc_asdjflasutopfhvcZlioyqe61  -- CDP internal symbol
 *
 * None of these are present in a user-launched Chrome window.
 *
 * Security model
 * --------------
 * These overrides only affect the JavaScript environment of the loaded page;
 * they do not alter Chrome's actual security sandbox, certificate validation,
 * or network stack. The intent is to allow the user to authenticate with
 * their own accounts in a browser they control, not to bypass security.
 */
(function (global) {
    'use strict';

    // navigator.webdriver -- set true by Puppeteer/ChromeDriver.
    // This is the first thing Google reads. Overriding requires a descriptor
    // change because Puppeteer sets it as a non-configurable getter in
    // some Chrome versions; wrap in try/catch so older versions still work.
    try {
        Object.defineProperty(navigator, 'webdriver', {
            get: function () { return false; },
            configurable: true,
        });
    } catch (_) {}

    // ChromeDriver leaves several $cdc_ globals on window and on the
    // document object. Google's sign-in page scans for these.
    (function removeCdcGlobals() {
        var keys = Object.getOwnPropertyNames(global);
        for (var i = 0; i < keys.length; i++) {
            if (keys[i].startsWith('cdc_') || keys[i].startsWith('$cdc_')) {
                try { delete global[keys[i]]; } catch (_) {}
            }
        }
    })();

    // navigator.permissions -- Puppeteer automation mode returns "denied"
    // for notification permission queries. Google accounts checks this
    // because legitimate browsers return "prompt" for first-visit users.
    try {
        var _origQuery = global.navigator.permissions && global.navigator.permissions.query
            ? global.navigator.permissions.query.bind(global.navigator.permissions)
            : null;

        if (_origQuery) {
            Object.defineProperty(navigator.permissions, 'query', {
                configurable: true,
                enumerable:   true,
                writable:     true,
                value: function (parameters) {
                    // Return "prompt" instead of "denied" for notifications so
                    // the browser does not look like a headless bot.
                    if (parameters && parameters.name === 'notifications') {
                        return Promise.resolve({
                            name:               'notifications',
                            state:              'prompt',
                            onchange:           null,
                            addEventListener:   function () {},
                            removeEventListener: function () {},
                        });
                    }
                    return _origQuery(parameters);
                },
            });
        }
    } catch (_) {}

    // navigator.plugins -- already populated in headful Chrome but verify
    // it is not empty (some --disable-extensions paths clear it).
    // A real Chrome install always has at least the PDF plugin.
    try {
        if (navigator.plugins && navigator.plugins.length === 0) {
            // Cannot directly populate the PluginArray host object.
            // Define a getter that returns a length so fingerprint scripts
            // do not see an obviously empty automation profile.
            Object.defineProperty(navigator, 'plugins', {
                get: function () {
                    var p = { length: 1, item: function () { return null; }, refresh: function () {} };
                    return p;
                },
                configurable: true,
            });
        }
    } catch (_) {}

    // navigator.languages -- guarantee a non-empty array.
    try {
        if (!navigator.languages || navigator.languages.length === 0) {
            Object.defineProperty(navigator, 'languages', {
                get: function () { return ['en-US', 'en']; },
                configurable: true,
            });
        }
    } catch (_) {}

    // Remove any CDP/DevTools protocol tracing symbols that Chrome may
    // attach to the document object when launched with --remote-debugging-port.
    try {
        var docKeys = Object.getOwnPropertyNames(document);
        for (var k = 0; k < docKeys.length; k++) {
            if (docKeys[k].indexOf('cdc') !== -1 || docKeys[k].indexOf('__driver') !== -1) {
                try { delete document[docKeys[k]]; } catch (_) {}
            }
        }
    } catch (_) {}

    // Mark as loaded so re-injection is a no-op.
    global.__xcmAuthMaskLoaded = true;

})(window);
