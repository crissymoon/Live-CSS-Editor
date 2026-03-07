/**
 * lru-worker.js
 *
 * Piscina worker -- runs maintenance sweeps for the in-memory LRU cache.
 * The main thread passes a lightweight metadata snapshot (no buffers) and
 * this worker returns keys to evict.  All CPU cost of the eviction decision
 * (sorting, scoring, percentage calculations) stays off the main event loop.
 *
 * Task shape:
 *   {
 *     entries: [
 *       { key: string, lastAccess: number, storedAt: number, ttlMs: number, size: number }
 *     ],
 *     maxEntries: number   -- hard cap (evict LRU entries if over limit)
 *     maxBytes:   number   -- soft byte cap (evict until under limit, 0 = disabled)
 *     nowMs:      number   -- current Date.now() from main thread
 *   }
 *
 * Return value:
 *   { evict: string[] }   -- keys the main thread should delete from LruStore
 *
 * Eviction policy (applied in order):
 *   1. Expired entries (storedAt + ttlMs < nowMs) -- always evict
 *   2. Over-count: evict by oldest lastAccess until count <= maxEntries
 *   3. Over-bytes: evict by oldest lastAccess until totalBytes <= maxBytes
 *
 * Stale scoring for informational purposes:
 *   staleRatio = (nowMs - storedAt) / ttlMs  -- 0=fresh, 1=expired
 */

'use strict';

module.exports = function lruWorkerTask(task) {
    const { entries, maxEntries, maxBytes, nowMs } = task;

    const toEvict = new Set();

    // ── Pass 1: expire ───────────────────────────────────────────────────────
    for (const e of entries) {
        if (nowMs >= e.storedAt + e.ttlMs) {
            toEvict.add(e.key);
        }
    }

    // Remaining live entries sorted by lastAccess ascending (LRU first).
    const live = entries
        .filter(function (e) { return !toEvict.has(e.key); })
        .sort(function (a, b) { return a.lastAccess - b.lastAccess; });

    // ── Pass 2: over-count ───────────────────────────────────────────────────
    let liveCount = live.length;
    let liveIdx   = 0;
    while (liveCount > maxEntries && liveIdx < live.length) {
        toEvict.add(live[liveIdx].key);
        liveCount--;
        liveIdx++;
    }

    // ── Pass 3: over-bytes ───────────────────────────────────────────────────
    if (maxBytes > 0) {
        let totalBytes = 0;
        for (let i = liveIdx; i < live.length; i++) {
            totalBytes += live[i].size || 0;
        }
        // Evict LRU entries until under the byte cap.
        let i = liveIdx;
        while (totalBytes > maxBytes && i < live.length) {
            if (!toEvict.has(live[i].key)) {
                totalBytes -= live[i].size || 0;
                toEvict.add(live[i].key);
            }
            i++;
        }
    }

    return { evict: Array.from(toEvict) };
};
