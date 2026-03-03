"""
page-speed/automation.py

Measures real page load performance using headless Chrome + the
Navigation Timing API and Resource Timing API.

Returns a dict with timing, resource stats, and page weight that maps
directly to the PHP frontend's renderResults().

params expected:  {"url": "https://...", "runs": 1}
  runs -- number of repeated loads to average (1-3, default 1)
"""
import os
import time
import json


def run(driver, params: dict, job) -> dict:
    url  = params.get('url', '').strip()
    runs = max(1, min(int(params.get('runs', 1)), 3))
    if not url:
        raise ValueError('params.url is required')
    if not url.startswith(('http://', 'https://')):
        url = 'https://' + url

    job.append_log(f'Target: {url}')
    job.append_log(f'Runs:   {runs}')

    all_timings = []

    for i in range(runs):
        job.append_log(f'--- Run {i + 1}/{runs} ---')

        t_start = time.perf_counter()
        driver.get(url)

        # Wait for window.load (max 30 s)
        deadline = time.time() + 30
        while time.time() < deadline:
            state = driver.execute_script('return document.readyState')
            if state == 'complete':
                break
            time.sleep(0.3)
        t_wall = (time.perf_counter() - t_start) * 1000  # ms wall clock

        extra_wait = 1.0 if runs == 1 else 0.5
        time.sleep(extra_wait)

        # Navigation Timing (Level 1 -- available everywhere)
        timing = driver.execute_script('return JSON.stringify(performance.timing)')
        t      = json.loads(timing)
        nav_start = t['navigationStart']

        def ms(key: str) -> float:
            v = t.get(key, 0)
            return max(0, v - nav_start) if v else 0

        ttfb       = ms('responseStart')           # Time to first byte
        dom_inter  = ms('domInteractive')          # DOM interactive
        dom_ready  = ms('domContentLoadedEventEnd')# DOMContentLoaded
        load_event = ms('loadEventEnd')            # window.onload end
        dns        = ms('domainLookupEnd') - ms('domainLookupStart')
        connect    = ms('connectEnd') - ms('connectStart')
        ttl_req    = ms('responseEnd') - ms('requestStart')  # transfer time

        # Resource Timing
        resources = driver.execute_script("""
            var entries = performance.getEntriesByType('resource');
            return entries.map(function(e) {
                return {
                    name:        e.name,
                    type:        e.initiatorType,
                    duration:    Math.round(e.duration),
                    size:        e.transferSize || 0,
                    encoded:     e.encodedBodySize || 0
                };
            });
        """) or []

        # Aggregate by type
        type_counts = {}
        type_sizes  = {}
        total_bytes = 0
        slowest     = []

        for r in resources:
            rtype = r.get('type', 'other') or 'other'
            type_counts[rtype] = type_counts.get(rtype, 0) + 1
            sz = r.get('size', 0) or 0
            type_sizes[rtype]  = type_sizes.get(rtype, 0) + sz
            total_bytes += sz
            slowest.append({'url': r['name'], 'ms': r.get('duration', 0), 'type': rtype})

        slowest.sort(key=lambda x: x['ms'], reverse=True)
        slowest = slowest[:10]

        run_data = {
            'ttfb':        round(ttfb, 1),
            'dom_ready':   round(dom_ready, 1),
            'load_event':  round(load_event or t_wall, 1),
            'dns':         round(dns, 1),
            'connect':     round(connect, 1),
            'transfer':    round(ttl_req, 1),
            'wall_ms':     round(t_wall, 1),
        }
        all_timings.append(run_data)
        job.append_log(
            f'  TTFB={run_data["ttfb"]}ms  DOMReady={run_data["dom_ready"]}ms'
            f'  Load={run_data["load_event"]}ms'
            f'  Resources={len(resources)}  Bytes={_fmt_bytes(total_bytes)}'
        )

    # Average if multiple runs
    avg = {k: round(sum(r[k] for r in all_timings) / runs, 1) for k in all_timings[0]}

    # Screenshot (after last run)
    shot_path = os.path.join(driver._shots_dir, 'latest.png')
    driver.save_screenshot(shot_path)
    job.append_log('Screenshot captured')

    # Page title / final URL
    title     = driver.title
    final_url = driver.current_url

    # Score (0-100, simple heuristic based on load time)
    load = avg['load_event']
    if load <= 800:        score = 95
    elif load <= 1500:     score = 85
    elif load <= 3000:     score = 70
    elif load <= 5000:     score = 50
    elif load <= 8000:     score = 30
    else:                  score = 10

    job.append_log(f'Score: {score}/100')

    return {
        'title':       title,
        'url':         final_url,
        'runs':        runs,
        'timing':      avg,
        'all_runs':    all_timings,
        'score':       score,
        'resources': {
            'total':      len(resources),
            'total_bytes': total_bytes,
            'by_type':    type_counts,
            'bytes_by_type': type_sizes,
        },
        'slowest':     slowest,
    }


def _fmt_bytes(n: int) -> str:
    if n < 1024:        return f'{n} B'
    if n < 1024 ** 2:   return f'{n / 1024:.1f} KB'
    return f'{n / 1024 ** 2:.1f} MB'
