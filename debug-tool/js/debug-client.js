/**
 * debug-tool/js/debug-client.js
 * Lightweight JS client for reporting errors from the Live CSS app frontend.
 * Drop-in: include this file once, then call window.DebugTool.log(...)
 *
 * Always logs to console as a fallback even when the API is unreachable.
 */

(function (global) {
    'use strict';

    // Config - override before the script loads or via DebugTool.configure()
    const DEFAULT_CONFIG = {
        endpoint: '/debug-tool/api/',
        source:   'live-css-frontend',
        enabled:  true,
        // Set to a value if DEBUG_API_KEY is active on the server
        apiKey:   '',
        // Minimum level to actually POST (still console.logs everything)
        // Levels: info=0, low=1, medium=2, high=3, critical=4
        minPostLevel: 'info',
    };

    const LEVEL_WEIGHT = { info: 0, low: 1, medium: 2, high: 3, critical: 4 };

    let _config = Object.assign({}, DEFAULT_CONFIG);

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------
    function levelWeight(level) {
        return LEVEL_WEIGHT[level] ?? 0;
    }

    function consoleByLevel(level, title, message, extra) {
        const prefix = `[DebugTool][${level.toUpperCase()}] ${title}`;
        try {
            switch (level) {
                case 'critical':
                case 'high':
                    console.error(prefix, message, extra || '');
                    break;
                case 'medium':
                    console.warn(prefix, message, extra || '');
                    break;
                default:
                    console.log(prefix, message, extra || '');
            }
        } catch (e) {
            // Last-resort fallback - never let console calls throw
        }
    }

    /**
     * Capture a clean stack trace string from the current call site.
     */
    function captureStack() {
        try {
            throw new Error('__stack_capture__');
        } catch (e) {
            return (e.stack || '')
                .split('\n')
                .filter(l => !l.includes('__stack_capture__') && !l.includes('debug-client'))
                .slice(0, 10)
                .join('\n');
        }
    }

    /**
     * POST the error payload to the API endpoint.
     * Falls back to console.error on network failure.
     */
    async function postToAPI(payload) {
        if (!_config.enabled) return;
        if (levelWeight(payload.level) < levelWeight(_config.minPostLevel)) return;

        const headers = { 'Content-Type': 'application/json' };
        if (_config.apiKey) headers['X-Debug-Key'] = _config.apiKey;

        try {
            const res = await fetch(_config.endpoint, {
                method:  'POST',
                headers: headers,
                body:    JSON.stringify(payload),
            });

            if (!res.ok) {
                const text = await res.text().catch(() => '');
                console.error('[DebugTool] API returned non-200:', res.status, text);
                return null;
            }

            const json = await res.json().catch(e => {
                console.error('[DebugTool] Failed to parse API response:', e);
                return null;
            });

            if (json && json.ticket_id) {
                console.info(`[DebugTool] Ticket created: ${json.ticket_id}`);
            }
            return json;

        } catch (err) {
            // Network failure - always log to console so nothing is lost
            console.error('[DebugTool] Network error reporting to API:', err, '| Payload:', payload);
            return null;
        }
    }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------
    const DebugTool = {

        /**
         * Override default configuration.
         * @param {object} opts
         */
        configure(opts = {}) {
            _config = Object.assign(_config, opts);
            console.log('[DebugTool] Configured:', JSON.stringify(_config));
        },

        /**
         * Log an error ticket.
         * @param {object} opts
         *   level       - 'critical' | 'high' | 'medium' | 'low' | 'info'
         *   title       - Short description (required)
         *   message     - Full error message (required)
         *   source      - Component source (default: config.source)
         *   file        - Filename (optional)
         *   line        - Line number (optional)
         *   stack_trace - Stack trace string (optional, auto-captured if omitted)
         *   context     - Plain object of extra data (optional)
         */
        log(opts = {}) {
            const level   = opts.level   || 'info';
            const title   = opts.title   || 'Untitled error';
            const message = opts.message || '';
            const source  = opts.source  || _config.source;

            // Always hit the console first - synchronous and reliable
            consoleByLevel(level, title, message, opts.context);

            const payload = {
                level,
                title,
                message,
                source,
                file:        opts.file        || null,
                line:        opts.line        || null,
                stack_trace: opts.stack_trace || captureStack(),
                context:     opts.context     || null,
            };

            // Fire-and-forget POST
            postToAPI(payload).catch(e => {
                console.error('[DebugTool] Unhandled postToAPI rejection:', e);
            });
        },

        // Convenience wrappers
        critical(title, message, extra = {}) {
            this.log(Object.assign({ level: 'critical', title, message }, extra));
        },
        high(title, message, extra = {}) {
            this.log(Object.assign({ level: 'high', title, message }, extra));
        },
        medium(title, message, extra = {}) {
            this.log(Object.assign({ level: 'medium', title, message }, extra));
        },
        low(title, message, extra = {}) {
            this.log(Object.assign({ level: 'low', title, message }, extra));
        },
        info(title, message, extra = {}) {
            this.log(Object.assign({ level: 'info', title, message }, extra));
        },

        /**
         * Hook into window.onerror to auto-capture uncaught JS errors.
         * Call once during app init.
         */
        hookUncaughtErrors() {
            const prev = window.onerror;
            window.onerror = (msg, src, lineno, colno, err) => {
                DebugTool.high(
                    'Uncaught JS Error',
                    String(msg),
                    {
                        file:        src,
                        line:        lineno,
                        stack_trace: err?.stack || null,
                        context:     { colno },
                        source:      'window.onerror',
                    }
                );
                if (typeof prev === 'function') prev(msg, src, lineno, colno, err);
                return false;
            };

            window.addEventListener('unhandledrejection', (event) => {
                DebugTool.high(
                    'Unhandled Promise Rejection',
                    String(event.reason),
                    {
                        stack_trace: event.reason?.stack || null,
                        source:      'unhandledrejection',
                    }
                );
            });

            console.log('[DebugTool] Uncaught error hooks registered.');
        },
    };

    // Expose globally
    global.DebugTool = DebugTool;
    console.log('[DebugTool] Loaded. Endpoint:', _config.endpoint);

})(window);
