/*
 * wf-debug.js -- Client-side dev event logger
 * Crissy's Style Tool
 *
 * Hooks window.onerror, unhandledrejection, console.error/warn, and all
 * button click events. Posts each event as JSON to /dev-log on the server
 * so everything appears in server.log alongside HTTP request logs.
 *
 * Loaded only in dev mode (wireframe.php includes it unconditionally for now;
 * remove the <script> tag for production builds).
 *
 * This file has zero dependencies and is a plain IIFE -- no imports.
 */
(function () {
    'use strict';

    /* POST a log entry to the server's /dev-log endpoint */
    function send(level, msg) {
        var payload = JSON.stringify({ level: level, msg: '[client] ' + msg });
        /* sendBeacon is fire-and-forget and survives page unload */
        if (navigator.sendBeacon) {
            navigator.sendBeacon('/dev-log', new Blob([payload], { type: 'application/json' }));
        } else {
            try {
                var x = new XMLHttpRequest();
                x.open('POST', '/dev-log', true);
                x.setRequestHeader('Content-Type', 'application/json');
                x.send(payload);
            } catch (e) { /* ignore */ }
        }
    }

    /* Also mirror to the original console so DevTools still shows everything */
    var _errOrig  = console.error.bind(console);
    var _warnOrig = console.warn.bind(console);
    var _logOrig  = console.log.bind(console);

    console.error = function () {
        var msg = Array.prototype.slice.call(arguments).join(' ');
        send('JS-ERROR', msg);
        _errOrig.apply(console, arguments);
    };

    console.warn = function () {
        var msg = Array.prototype.slice.call(arguments).join(' ');
        send('JS-WARN', msg);
        _warnOrig.apply(console, arguments);
    };

    /* window.onerror -- catches uncaught exceptions */
    var _prevOnerror = window.onerror;
    window.onerror = function (msg, src, line, col, err) {
        send('JS-UNCAUGHT', msg + ' at ' + src + ':' + line + ':' + col + (err ? ' ' + err.stack : ''));
        if (typeof _prevOnerror === 'function') _prevOnerror.apply(this, arguments);
        return false;
    };

    /* Unhandled promise rejections (module import failures land here) */
    window.addEventListener('unhandledrejection', function (ev) {
        var reason = ev.reason ? (ev.reason.stack || String(ev.reason)) : 'unknown';
        send('JS-REJECT', 'UnhandledRejection: ' + reason);
    });

    /* Button click watcher -- logs any button/a click with its id and text */
    document.addEventListener('click', function (ev) {
        var t = ev.target;
        /* Walk up to find a button or anchor */
        while (t && t !== document.body) {
            if (t.tagName === 'BUTTON' || t.tagName === 'A') {
                var label = (t.id ? '#' + t.id + ' ' : '') + '"' + (t.textContent || '').trim().slice(0, 60) + '"';
                send('CLICK', t.tagName + ' ' + label);
                break;
            }
            t = t.parentElement;
        }
    }, true /* capture -- fires before any onclick handler */);

    /* Module load diagnostic: log which files were requested */
    var _perfObs;
    try {
        if (window.PerformanceObserver) {
            _perfObs = new PerformanceObserver(function (list) {
                list.getEntries().forEach(function (e) {
                    if (e.initiatorType === 'script' || e.initiatorType === 'fetch') {
                        send('RESOURCE', e.initiatorType + ' ' + e.name + ' ' + Math.round(e.duration) + 'ms');
                    }
                });
            });
            _perfObs.observe({ type: 'resource', buffered: true });
        }
    } catch (e) { /* PerformanceObserver not supported */ }

    send('INFO', 'wf-debug.js loaded on ' + window.location.href);
}());
