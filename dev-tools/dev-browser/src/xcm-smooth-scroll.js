// xcm-smooth-scroll.js -- injected at document-start by imgui-browser
// Enables native CSS smooth scrolling for all in-page anchor jumps and
// programmatic scroll calls that do not already specify a behavior.
(function () {
    'use strict';
    if (window.__xcmSmoothScrollInstalled) return;
    window.__xcmSmoothScrollInstalled = true;

    var s = document.createElement('style');
    s.id = '__xcm-smooth-scroll';
    s.textContent = 'html { scroll-behavior: smooth !important; }';
    // document.head may not exist yet at document-start; use documentElement.
    (document.head || document.documentElement).appendChild(s);
})();
