"""
page-inspector/automation.py

Uses headless Chrome to inspect any URL:
  - Captures a full-page screenshot saved as screenshots/latest.png
  - Extracts title, meta description, all links, and image src URLs
  - Returns a dict that maps to the PHP frontend's renderResults()

params expected:  {"url": "https://..."}
"""
import os
import time


def run(driver, params: dict, job) -> dict:
    url = params.get('url', '').strip()
    if not url:
        raise ValueError('params.url is required')
    if not url.startswith(('http://', 'https://')):
        url = 'https://' + url

    job.append_log(f'Loading {url}')
    driver.get(url)

    # Wait for the page to settle (max 8 s)
    deadline = time.time() + 8
    while time.time() < deadline:
        state = driver.execute_script('return document.readyState')
        if state == 'complete':
            break
        time.sleep(0.4)
    time.sleep(0.8)   # let lazy-load images trigger

    job.append_log('Page loaded, capturing screenshot')
    shot_path = os.path.join(driver._shots_dir, 'latest.png')
    driver.save_screenshot(shot_path)
    job.append_log(f'Screenshot saved')

    # Title
    title = driver.title

    # Meta description
    description = driver.execute_script(
        "var m=document.querySelector('meta[name=\"description\"]');"
        "return m ? m.content : '';"
    ) or ''

    # All links (unique, absolute)
    raw_links = driver.execute_script(
        "return Array.from(document.querySelectorAll('a[href]'))"
        ".map(a=>a.href).filter(h=>h.startsWith('http'))"
    ) or []
    links = list(dict.fromkeys(raw_links))   # deduplicate preserving order

    # All image src (unique, absolute, skip data: URIs)
    raw_imgs = driver.execute_script(
        "return Array.from(document.querySelectorAll('img[src]'))"
        ".map(i=>i.src).filter(s=>s.startsWith('http'))"
    ) or []
    images = list(dict.fromkeys(raw_imgs))

    # HTTP status is not exposed by WebDriver; report as n/a
    status_code = 'n/a (WebDriver)'

    job.append_log(f'Found {len(links)} links, {len(images)} images')

    return {
        'title':       title,
        'description': description,
        'url':         driver.current_url,
        'status_code': status_code,
        'links':       links,
        'images':      images,
    }
