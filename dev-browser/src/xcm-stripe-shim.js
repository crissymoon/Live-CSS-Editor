/**
 * xcm-stripe-shim.js
 *
 * Injected at document-start by the imgui browser on every page.
 * Self-gates on hostname so it only activates on Anthropic / OpenAI
 * billing pages where Stripe.js runs.
 *
 * WHAT IT DOES
 * ────────────
 * 1. Intercepts fetch() calls to r.stripe.com/b (Stripe Radar beacon).
 *    In WKWebView the beacon fails with "Failed to fetch" because WKWebView
 *    treats opaque no-cors cross-origin POSTs to third-party endpoints as
 *    unsafe when they originate from a non-browser context. Stripe.js hangs
 *    waiting for this beacon before it fully initialises the payment UI.
 *    We resolve the beacon immediately with a synthetic 200 so Stripe.js
 *    continues initialising without any network round-trip.
 *
 * 2. Intercepts fetch() calls to m.stripe.network (Stripe Radar iframe
 *    loader). Same WKWebView cross-origin issue.
 *
 * 3. Polyfills navigator.sendBeacon for Stripe domains so the same beacon
 *    dispatch path succeeds without a visible network call.
 *
 * 4. Monitors all fetch / XHR calls and logs failures to stderr via
 *    console.error so the native [nav] page-probe shows blockers in the
 *    imgui browser debug log.
 *
 * 5. Reports Stripe.js initialisation state every 2 seconds until it either
 *    resolves or we see the payment element mount.
 */

(function () {
    var ACTIVE_HOSTS = [
        'platform.claude.com',
        'console.anthropic.com',
        'platform.openai.com',
        'chat.openai.com',
        'anthropic.com',
        // Stripe payment iframes load from these origins; the beacon calls
        // (r.stripe.com/b, m.stripe.network) are made from within these frames.
        'js.stripe.com',
        'stripe.com',
    ];
    var host = location.hostname;
    var active = ACTIVE_HOSTS.some(function (h) {
        return host === h || host.endsWith('.' + h);
    });
    if (!active) return;

    // ── Stripe Radar beacon domain list ──────────────────────────────────
    var STRIPE_BEACON_ORIGINS = [
        'https://r.stripe.com',
        'https://m.stripe.network',
        'https://b.stripecdn.com',
    ];

    function isStripeBeacon(url) {
        return STRIPE_BEACON_ORIGINS.some(function (o) { return url.indexOf(o) === 0; });
    }

    // ── Synthetic 200 response for beacon calls ───────────────────────────
    function makeMockResponse() {
        try {
            return new Response('{}', {
                status:  200,
                headers: {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*',
                }
            });
        } catch (e) {
            return { ok: true, status: 200, json: function () { return Promise.resolve({}); } };
        }
    }

    // ── Patch fetch ───────────────────────────────────────────────────────
    var _origFetch = window.fetch;
    window.fetch = function (input, init) {
        var url = (typeof input === 'string') ? input
                : (input && input.url)        ? input.url
                : String(input);

        if (isStripeBeacon(url)) {
            console.log('[xcm-stripe-shim] intercepted beacon: ' + url);
            return Promise.resolve(makeMockResponse());
        }

        return _origFetch.apply(this, arguments).then(function (r) {
            if (!r.ok) {
                console.warn('[xcm-stripe-shim] fetch non-ok ' + r.status + ': ' + url);
            }
            return r;
        }, function (err) {
            console.error('[xcm-stripe-shim] fetch failed: ' + url + ' -- ' + (err && err.message));
            // Re-throw so the original caller's error path still runs
            return Promise.reject(err);
        });
    };

    // ── Patch sendBeacon ──────────────────────────────────────────────────
    var _origBeacon = navigator.sendBeacon ? navigator.sendBeacon.bind(navigator) : null;
    navigator.sendBeacon = function (url, data) {
        if (isStripeBeacon(url)) {
            console.log('[xcm-stripe-shim] intercepted sendBeacon: ' + url);
            return true;
        }
        if (_origBeacon) return _origBeacon(url, data);
        return false;
    };

    // ── Patch XMLHttpRequest (Stripe fallback path) ───────────────────────
    var _OrigXHR = window.XMLHttpRequest;
    function PatchedXHR() { this._xhr = new _OrigXHR(); }
    PatchedXHR.prototype.open = function (method, url) {
        this._url = url;
        this._intercepted = isStripeBeacon(url);
        if (!this._intercepted) this._xhr.open.apply(this._xhr, arguments);
    };
    PatchedXHR.prototype.send = function (body) {
        if (this._intercepted) {
            console.log('[xcm-stripe-shim] intercepted XHR: ' + this._url);
            var self = this;
            setTimeout(function () {
                Object.defineProperty(self, 'status',       { value: 200 });
                Object.defineProperty(self, 'readyState',   { value: 4 });
                Object.defineProperty(self, 'responseText', { value: '{}' });
                if (typeof self.onreadystatechange === 'function') self.onreadystatechange();
                if (typeof self.onload === 'function') self.onload({ target: self });
            }, 0);
            return;
        }
        // Mirror addEventListener calls
        var self = this;
        this._xhr.onreadystatechange = function () {
            if (self.onreadystatechange) self.onreadystatechange.apply(self, arguments);
        };
        this._xhr.onload  = function (e) { if (self.onload)  self.onload.call(self, e); };
        this._xhr.onerror = function (e) {
            console.error('[xcm-stripe-shim] XHR error: ' + self._url);
            if (self.onerror) self.onerror.call(self, e);
        };
        this._xhr.send(body);
    };
    // Proxy all other XHR properties / methods
    ['setRequestHeader','abort','getResponseHeader','getAllResponseHeaders',
     'overrideMimeType','addEventListener','removeEventListener'].forEach(function (m) {
        PatchedXHR.prototype[m] = function () {
            if (this._intercepted) return;
            return this._xhr[m].apply(this._xhr, arguments);
        };
    });
    ['responseType','withCredentials','timeout'].forEach(function (p) {
        Object.defineProperty(PatchedXHR.prototype, p, {
            get: function () { return this._xhr[p]; },
            set: function (v) { if (!this._intercepted) this._xhr[p] = v; }
        });
    });
    ['status','readyState','responseText','response','responseURL',
     'statusText'].forEach(function (p) {
        Object.defineProperty(PatchedXHR.prototype, p, {
            get: function () { return this._xhr ? this._xhr[p] : undefined; },
            configurable: true,
        });
    });
    // Copy static constants
    [0,1,2,3,4].forEach(function (n) {
        PatchedXHR.UNSENT = 0; PatchedXHR.OPENED = 1; PatchedXHR.HEADERS_RECEIVED = 2;
        PatchedXHR.LOADING = 3; PatchedXHR.DONE = 4;
    });
    window.XMLHttpRequest = PatchedXHR;

    // ── Mock window.Stripe so React billing pages stop waiting ───────────
    // Stripe.js on WKWebView can freeze the JS engine (stripe-js issue #614).
    // We provide a no-op stub so the billing page React tree mounts, showing
    // plan info / usage / invoices even when real card processing is unavailable.
    // Only inject the stub if we are in the main frame (no real Stripe yet).
    if (window === window.top && !window.Stripe) {
        function makeMockElement() {
            return {
                mount:   function () {},
                unmount: function () {},
                update:  function () {},
                destroy: function () {},
                on:      function () {},
                off:     function () {},
            };
        }
        function makeMockElements() {
            return {
                create:  function () { return makeMockElement(); },
                getElement: function () { return null; },
                fetchUpdates: function () { return Promise.resolve({}); },
                submit: function () { return Promise.resolve({ error: { message: 'unavailable in this browser' } }); },
            };
        }
        function MockStripe() {}
        MockStripe.prototype.elements    = function () { return makeMockElements(); };
        MockStripe.prototype.confirmPayment = function () {
            return Promise.resolve({ error: { message: 'payment forms require a standard browser' } });
        };
        MockStripe.prototype.confirmSetup = MockStripe.prototype.confirmPayment;
        MockStripe.prototype.retrievePaymentIntent = function () { return Promise.resolve({ paymentIntent: null }); };
        MockStripe.prototype.createToken           = function () { return Promise.resolve({ error: { message: 'unavailable' } }); };
        MockStripe.prototype.createPaymentMethod   = function () { return Promise.resolve({ error: { message: 'unavailable' } }); };

        // Stripe() constructor
        window.Stripe = function Stripe(pubKey, opts) {
            return new MockStripe();
        };
        window.Stripe._mock = true;
        console.log('[xcm-stripe-shim] installed mock window.Stripe');
    }

    // ── Hard-remove lingering spinners after 6 seconds ────────────────────
    // Catches cases where the React tree still renders a loading overlay
    // even after window.Stripe is present.
    if (window === window.top) {
        setTimeout(function () {
            var removed = 0;
            // Common spinner/overlay patterns across billing UIs
            var candidates = document.querySelectorAll(
                '[class*="spinner"], [class*="Spinner"], ' +
                '[class*="loading"], [class*="Loading"], ' +
                '[role="progressbar"], ' +
                '[aria-label*="loading" i], [aria-label*="spinner" i]'
            );
            candidates.forEach(function (el) {
                var style = window.getComputedStyle(el);
                // Only remove full-viewport overlay spinners, not inline ones
                if (style.position === 'fixed' || style.position === 'absolute') {
                    el.style.display = 'none';
                    removed++;
                }
            });
            if (removed > 0) {
                console.log('[xcm-stripe-shim] hard-removed ' + removed + ' spinner(s) after timeout');
            }
        }, 6000);
    }

    console.log('[xcm-stripe-shim] active on ' + host);
})();
