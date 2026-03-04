/**
 * input-watcher.js -- Unified atomic input state for all pointer, keyboard,
 * wheel, touch, and scroll events.
 *
 * Race-condition root cause
 * -------------------------
 * The CDM SDK, the stats monitor, and the rAF ticker all previously owned
 * their own copy of "is the user scrolling?".  Each was a plain JS boolean
 * updated by a separate event listener and read by a different rAF callback.
 * Because WKWebView delivers input events as micro-tasks that can interleave
 * between any two JS statements, readings of those booleans were stale by the
 * time the reader acted on them.
 *
 * Solution: single-writer, multi-reader Int32Array atom
 * ------------------------------------------------------
 * One Int32Array(4) lives at window.__xcm.input.state.  All input events
 * (wheel, scroll, pointer, touch, keyboard) are funneled through this module
 * and write to it.  No other module ever writes these bits.
 *
 * Readers (CDM SDK drain loop, _rwTick, stats) snapshot state[0] ONCE at the
 * very top of their rAF callback.  Everything inside that callback uses the
 * snapshot.  This means a wheel event that fires between two JS tasks cannot
 * invalidate a decision that was already made in the current task.
 *
 * Int32Array semantics on ARM64 / x86_64
 * ---------------------------------------
 * A 32-bit read or write to an aligned TypedArray index is always a single
 * memory instruction on both ARM64 (WKWebView on Apple Silicon) and x86_64.
 * The JS engine will never split a 32-bit aligned store or load into two
 * operations, so there are no torn reads even without SharedArrayBuffer.
 *
 * State atom layout (window.__xcm.input.state -- Int32Array(4))
 * ---------------------------------------------------------------
 *   [0]  input flags bitmask
 *          bit 0  SCROLLING     -- wheel or native scroll in progress
 *          bit 1  POINTER_DOWN  -- at least one pointer button held
 *          bit 2  KEY_DOWN      -- at least one key held
 *          bit 3  WHEEL         -- wheel event fired this frame
 *          bit 4  TOUCH         -- touch contact active
 *   [1]  last event DOMHighResTimeStamp truncated to integer ms
 *   [2]  total event count (monotonic, wraps at 2^31-1)
 *   [3]  reserved (always 0)
 *
 * Delta atom layout (window.__xcm.input.delta -- Float32Array(4))
 * ---------------------------------------------------------------
 *   [0]  accumulated wheel deltaX this frame (pixels)
 *   [1]  accumulated wheel deltaY this frame (pixels)
 *   [2]  pointer deltaX since last pointerdown
 *   [3]  pointer deltaY since last pointerdown
 *   Reset to 0 on every tick via __xcmTick so callers always see per-frame
 *   deltas rather than a monotonically growing sum.
 *
 * Public API: window.__xcm.input
 *   .state       -- live Int32Array(4)
 *   .delta       -- live Float32Array(4)
 *   .flags       -- { SCROLLING, POINTER_DOWN, KEY_DOWN, WHEEL, TOUCH }
 *   .snapshot()  -- plain object copy of current state (safe to hold refs to)
 *   .isScrolling()
 *   .isPointerDown()
 *   .isKeyDown()
 *   .log(n)      -- last n log entries (default 64)
 *
 * Injected at DocumentStart, all frames, BEFORE the CDM SDK and ticker.
 */
(function (global) {
    'use strict';

    if (global.__xcmInputWatcher) return;
    global.__xcmInputWatcher = true;

    // ── Flag constants ────────────────────────────────────────────────────────
    var FLAGS = {
        SCROLLING:    1,   // bit 0
        POINTER_DOWN: 2,   // bit 1
        KEY_DOWN:     4,   // bit 2
        WHEEL:        8,   // bit 3
        TOUCH:        16,  // bit 4
    };

    // ── State atom ────────────────────────────────────────────────────────────
    // Int32Array over a plain ArrayBuffer.  All 4 fields are 32-bit aligned.
    var _stateBuf  = new ArrayBuffer(16);
    var _state     = new Int32Array(_stateBuf);   // [flags, lastTs, evCount, 0]
    var _deltaBuf  = new ArrayBuffer(16);
    var _delta     = new Float32Array(_deltaBuf); // [wDX, wDY, pDX, pDY]

    // ── Ring-buffer event log ────────────────────────────────────────────────
    var LOG_CAP   = 512;
    var _log      = new Array(LOG_CAP);
    var _logHead  = 0;  // next write index
    var _logCount = 0;  // total events ever recorded

    function _record(type, ts, x, y, dX, dY, extra) {
        _state[1] = (ts | 0);
        _state[2] = (_state[2] + 1) | 0;
        _log[_logHead] = {
            seq:   _logCount++,
            type:  type,
            ts:    ts,
            x:     x   || 0,
            y:     y   || 0,
            dX:    dX  || 0,
            dY:    dY  || 0,
            extra: extra || null,
            flags: _state[0],  // snapshot of flags at moment of event
        };
        _logHead = (_logHead + 1) % LOG_CAP;
    }

    // ── Atomic flag helpers ───────────────────────────────────────────────────
    // These are the ONLY functions that write to _state[0].
    // No other module touches this array.
    function _setFlags(bits)   { _state[0] = _state[0] | bits; }
    function _clearFlags(bits) { _state[0] = _state[0] & ~bits; }
    function _hasFlag(bit)     { return (_state[0] & bit) !== 0; }

    // ── Scroll idle timer ─────────────────────────────────────────────────────
    // One timer shared across wheel and native scroll events.
    var _scrollTimer = null;
    var _wheelTimer  = null;

    function _idleMs() {
        // Always re-read from ticker output so Hz changes are respected.
        return global.__xcmIdleThreshold || 68;
    }

    function _armScrollTimer() {
        if (_scrollTimer) clearTimeout(_scrollTimer);
        _scrollTimer = setTimeout(function () {
            _clearFlags(FLAGS.SCROLLING);
            _scrollTimer = null;
        }, _idleMs());
    }

    function _armWheelTimer() {
        if (_wheelTimer) clearTimeout(_wheelTimer);
        _wheelTimer = setTimeout(function () {
            _clearFlags(FLAGS.WHEEL);
            _delta[0] = 0;
            _delta[1] = 0;
            _wheelTimer = null;
        }, _idleMs());
    }

    // ── Event: wheel (trackpad + mouse wheel) ─────────────────────────────────
    // This is the primary trackpad signal on macOS WKWebView.
    // Wheel events carry deltaX/deltaY in CSS pixels.  The WKWebView
    // compositor normally handles them without JS involvement -- registering
    // a passive listener here is zero-cost for compositing.
    global.addEventListener('wheel', function (e) {
        _setFlags(FLAGS.SCROLLING | FLAGS.WHEEL);
        _delta[0] += e.deltaX;
        _delta[1] += e.deltaY;
        _record('wheel', e.timeStamp, e.clientX, e.clientY, e.deltaX, e.deltaY,
                { mode: e.deltaMode, phase: e.wheelDeltaY });
        _armScrollTimer();
        _armWheelTimer();
    }, { passive: true, capture: true });

    // ── Event: native scroll ──────────────────────────────────────────────────
    global.addEventListener('scroll', function (e) {
        _setFlags(FLAGS.SCROLLING);
        _record('scroll', e.timeStamp, 0, 0, 0, 0, null);
        _armScrollTimer();
    }, { passive: true, capture: true });

    // ── Event: pointer (mouse + pen + touch as pointer) ───────────────────────
    global.addEventListener('pointerdown', function (e) {
        _setFlags(FLAGS.POINTER_DOWN);
        _delta[2] = 0;  // reset pointer deltas on each new contact
        _delta[3] = 0;
        _record('pointerdown', e.timeStamp, e.clientX, e.clientY, 0, 0,
                { id: e.pointerId, type: e.pointerType, btn: e.button });
    }, { passive: true, capture: true });

    global.addEventListener('pointermove', function (e) {
        if (_hasFlag(FLAGS.POINTER_DOWN)) {
            _delta[2] += e.movementX;
            _delta[3] += e.movementY;
        }
        _record('pointermove', e.timeStamp, e.clientX, e.clientY,
                e.movementX, e.movementY,
                { id: e.pointerId, type: e.pointerType, pressure: e.pressure });
    }, { passive: true, capture: true });

    function _pointerUp(e) {
        _clearFlags(FLAGS.POINTER_DOWN);
        _record('pointerup', e.timeStamp, e.clientX, e.clientY, 0, 0,
                { id: e.pointerId, type: e.pointerType });
    }
    global.addEventListener('pointerup',     _pointerUp, { passive: true, capture: true });
    global.addEventListener('pointercancel', _pointerUp, { passive: true, capture: true });

    // ── Event: touch raw ─────────────────────────────────────────────────────
    global.addEventListener('touchstart', function (e) {
        _setFlags(FLAGS.TOUCH | FLAGS.SCROLLING);
        _record('touchstart', e.timeStamp, 0, 0, 0, 0,
                { count: e.touches.length });
        _armScrollTimer();
    }, { passive: true, capture: true });

    global.addEventListener('touchmove', function (e) {
        var t = e.touches[0];
        _record('touchmove', e.timeStamp,
                t ? t.clientX : 0, t ? t.clientY : 0, 0, 0,
                { count: e.touches.length });
    }, { passive: true, capture: true });

    global.addEventListener('touchend', function (e) {
        if (!e.touches.length) _clearFlags(FLAGS.TOUCH);
        _record('touchend', e.timeStamp, 0, 0, 0, 0,
                { count: e.touches.length });
    }, { passive: true, capture: true });

    global.addEventListener('touchcancel', function (e) {
        _clearFlags(FLAGS.TOUCH);
        _record('touchcancel', e.timeStamp, 0, 0, 0, 0, null);
    }, { passive: true, capture: true });

    // ── Event: keyboard ──────────────────────────────────────────────────────
    global.addEventListener('keydown', function (e) {
        _setFlags(FLAGS.KEY_DOWN);
        _record('keydown', e.timeStamp, 0, 0, 0, 0,
                { key: e.key, code: e.code, mod: ((e.ctrlKey ? 1 : 0) |
                    (e.altKey ? 2 : 0) | (e.shiftKey ? 4 : 0) |
                    (e.metaKey ? 8 : 0)) });
    }, { passive: true, capture: true });

    global.addEventListener('keyup', function (e) {
        _clearFlags(FLAGS.KEY_DOWN);
        _record('keyup', e.timeStamp, 0, 0, 0, 0,
                { key: e.key, code: e.code });
    }, { passive: true, capture: true });

    // ── Per-frame delta reset via ticker ─────────────────────────────────────
    // Subscribe to the shared rAF bus when it becomes available.
    // On each tick: (1) clear the WHEEL bit so callers know "did wheel fire
    // in THIS frame" vs "wheel fired at some point in the past", (2) reset
    // the delta accumulators so they represent per-frame motion.
    function _registerTick() {
        if (global.__xcmTick) {
            global.__xcmTick(function _inputTick() {
                // Clear per-frame transient flags.
                _clearFlags(FLAGS.WHEEL);
                // Reset accumulated deltas; downstream reads from _delta
                // between two ticks get the motion for exactly that frame.
                _delta[0] = 0;
                _delta[1] = 0;
                // Pointer deltas accumulate until pointerdown resets them;
                // only zero the per-frame pointer motion here.
                _delta[2] = 0;
                _delta[3] = 0;
            });
        } else {
            // Ticker not yet ready -- retry after one rAF.
            requestAnimationFrame(function () { _registerTick(); });
        }
    }
    _registerTick();

    // ── Public API ────────────────────────────────────────────────────────────
    var _inputAPI = {
        // Live typed arrays -- readers MUST snapshot state[0] once and
        // use the snapshot, never call state[0] twice in the same function.
        state: _state,
        delta: _delta,
        flags: FLAGS,

        // Safe snapshot: plain object, safe to hold a reference to.
        snapshot: function () {
            var f = _state[0];  // single 32-bit read
            return {
                flags:       f,
                scrolling:   (f & FLAGS.SCROLLING)    !== 0,
                pointerDown: (f & FLAGS.POINTER_DOWN)  !== 0,
                keyDown:     (f & FLAGS.KEY_DOWN)      !== 0,
                wheel:       (f & FLAGS.WHEEL)         !== 0,
                touch:       (f & FLAGS.TOUCH)         !== 0,
                lastTs:      _state[1],
                evCount:     _state[2],
                wDeltaX:     _delta[0],
                wDeltaY:     _delta[1],
                pDeltaX:     _delta[2],
                pDeltaY:     _delta[3],
            };
        },

        isScrolling:   function () { return (_state[0] & FLAGS.SCROLLING)    !== 0; },
        isPointerDown: function () { return (_state[0] & FLAGS.POINTER_DOWN)  !== 0; },
        isKeyDown:     function () { return (_state[0] & FLAGS.KEY_DOWN)      !== 0; },

        // Return the last n log entries in chronological order.
        log: function (n) {
            n = Math.min(n || 64, LOG_CAP);
            var out = [];
            var total = Math.min(_logCount, LOG_CAP);
            if (total < n) n = total;
            // Walk backwards from head.
            for (var i = 1; i <= n; i++) {
                var idx = (_logHead - i + LOG_CAP) % LOG_CAP;
                var e = _log[idx];
                if (e) out.unshift(e);
            }
            return out;
        },
    };

    // ── Attach to window.__xcm safely ────────────────────────────────────────
    // __xcm may not exist yet (ticker not run) -- set a setter trap so the
    // input API is merged in whenever __xcm is first written.
    if (global.__xcm) {
        global.__xcm.input = _inputAPI;
    } else {
        // Define a one-shot property setter that merges input in.
        var _pending = true;
        Object.defineProperty(global, '__xcm', {
            configurable: true,
            enumerable:   true,
            set: function (v) {
                // Replace the trap with a plain value once set.
                Object.defineProperty(global, '__xcm', {
                    configurable: true,
                    enumerable:   true,
                    writable:     true,
                    value:        v,
                });
                if (_pending && v && typeof v === 'object') {
                    _pending = false;
                    v.input = _inputAPI;
                }
            },
            get: function () { return undefined; },
        });
    }

    // Always expose a standalone global so CDM SDK can reach it even if
    // __xcm has not been assembled yet.
    global.__xcmInput = _inputAPI;

})(window);
