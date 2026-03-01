/**
 * theme_randomizer/context.js
 *
 * Locale / cultural context inference from browser APIs.
 * Requires: constants.js, state.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:context] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    function resolveTimeOfDay(hour) {
        if (hour >= 5  && hour < 12) { return 'Morning'; }
        if (hour >= 12 && hour < 17) { return 'Afternoon'; }
        if (hour >= 17 && hour < 21) { return 'Evening'; }
        return 'Night';
    }

    function resolveDirection(lang) {
        var rtl = ['ar', 'he', 'fa', 'ur', 'ps', 'dv', 'yi'];
        return rtl.indexOf(lang) !== -1 ? 'rtl' : 'ltr';
    }

    function resolveCulturalArea(lang, region) {
        var eastAsian  = ['zh', 'ja', 'ko'];
        var southAsian = ['hi', 'ur', 'bn', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa', 'si'];
        var rtlLangs   = ['ar', 'he', 'fa', 'ps', 'dv', 'yi'];
        var nordic     = ['sv', 'no', 'nb', 'nn', 'da', 'fi', 'is', 'et', 'lv', 'lt'];
        var slavic     = ['ru', 'pl', 'cs', 'sk', 'uk', 'bg', 'sr', 'hr', 'sl', 'mk', 'bs', 'be'];
        var latin      = ['es', 'pt', 'fr', 'it', 'ro', 'ca', 'gl', 'oc'];
        var germanic   = ['de', 'nl', 'af', 'lb'];
        var latamCarib = ['MX','GT','BZ','HN','SV','NI','CR','PA','CU','DO','PR','JM','TT','HT'];
        var latamSouth = ['AR','BO','BR','CL','CO','EC','PE','PY','UY','VE'];
        var australas  = ['AU','NZ'];

        if (eastAsian.indexOf(lang)  !== -1) { return 'East Asia'; }
        if (southAsian.indexOf(lang) !== -1) { return 'South Asia'; }
        if (rtlLangs.indexOf(lang)   !== -1) { return 'Middle East / North Africa'; }
        if (nordic.indexOf(lang)     !== -1) { return 'Northern Europe'; }
        if (slavic.indexOf(lang)     !== -1) { return 'Eastern Europe'; }
        if (germanic.indexOf(lang)   !== -1) { return 'Western Europe / Germanic'; }
        if (latin.indexOf(lang) !== -1) {
            if (latamCarib.indexOf(region) !== -1) { return 'Latin America / Caribbean'; }
            if (latamSouth.indexOf(region) !== -1) { return 'Latin America / South'; }
            return 'Western Europe / Latin';
        }
        if (australas.indexOf(region) !== -1) { return 'Australasia'; }
        if (region === 'IN')                  { return 'South Asia'; }
        return 'North America / English';
    }

    // -----------------------------------------------------------------------
    // Exported function
    // -----------------------------------------------------------------------

    /**
     * Build a context descriptor entirely from browser APIs.
     * Works offline -- the server endpoint enriches this but is not required.
     * @returns {object}
     */
    _i.fn.inferContext = function inferContext() {
        try {
            var locale  = navigator.language || 'en-US';
            var parts   = locale.split('-');
            var lang    = parts[0].toLowerCase();
            var region  = (parts[1] || '').toUpperCase();
            var tz      = 'UTC';
            try { tz = Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC'; } catch (e) {
                console.warn('[themeRand:context] Intl timezone unavailable, using UTC:', e);
            }

            var now       = new Date();
            var hour      = now.getHours();

            return {
                locale:        locale,
                lang:          lang,
                region:        region,
                timezone:      tz,
                time_of_day:   resolveTimeOfDay(hour),
                cultural_area: resolveCulturalArea(lang, region),
                direction:     resolveDirection(lang),
                date_long:     now.toLocaleDateString(locale, { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' }),
                time_24:       now.toLocaleTimeString(locale, { hour12: false, hour: '2-digit', minute: '2-digit' }),
            };
        } catch (e) {
            console.error('[themeRand:context] inferContext() failed:', e);
            return { locale: 'en-US', lang: 'en', region: '', timezone: 'UTC', time_of_day: 'Day', cultural_area: 'North America / English', direction: 'ltr', date_long: '', time_24: '' };
        }
    };

    console.log('[themeRand:context] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
