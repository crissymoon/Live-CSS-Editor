<?php
/**
 * dom-inspector/proxy.php
 *
 * Fetches a remote URL, strips restrictive security headers, rewrites
 * all internal links/assets to route through this proxy, and injects
 * the live DOM inspector script so the parent frame (index.php) can
 * receive element data via postMessage.
 *
 * Usage: proxy.php?url=https%3A%2F%2Fexample.com%2F&inspect=1
 */
$raw_url = isset($_GET['url']) ? trim($_GET['url']) : '';
if (!$raw_url) {
    http_response_code(400);
    echo 'Missing url parameter.';
    exit;
}
if (!preg_match('#^https?://#i', $raw_url)) {
    $raw_url = 'https://' . $raw_url;
}

/* ── Fetch the remote page ───────────────────────────────────── */
$ch = curl_init($raw_url);
curl_setopt_array($ch, [
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_HEADER         => true,
    CURLOPT_FOLLOWLOCATION => true,
    CURLOPT_MAXREDIRS      => 5,
    CURLOPT_TIMEOUT        => 20,
    CURLOPT_SSL_VERIFYPEER => false,
    CURLOPT_SSL_VERIFYHOST => false,
    CURLOPT_USERAGENT      => 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) '
                             .'AppleWebKit/537.36 (KHTML, like Gecko) '
                             .'Chrome/120.0.0.0 Safari/537.36',
    CURLOPT_ENCODING       => '',                // Accept-Encoding: all
    CURLOPT_HTTPHEADER     => [
        'Accept: text/html,application/xhtml+xml,*/*;q=0.9',
        'Accept-Language: en-US,en;q=0.9',
        'Cache-Control: no-cache',
    ],
]);
$response     = curl_exec($ch);
$header_size  = curl_getinfo($ch, CURLINFO_HEADER_SIZE);
$effective_url= curl_getinfo($ch, CURLINFO_EFFECTIVE_URL);
$content_type = curl_getinfo($ch, CURLINFO_CONTENT_TYPE) ?: 'text/html';
$http_code    = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

if ($response === false || $http_code >= 400) {
    http_response_code(502);
    echo "Proxy error: could not fetch $raw_url (HTTP $http_code)";
    exit;
}

$resp_headers = substr($response, 0, $header_size);
$body         = substr($response, $header_size);

/* ── Only process HTML ───────────────────────────────────────── */
if (!preg_match('#text/html#i', $content_type)) {
    // For non-HTML resources (images, CSS, JS) pass through directly.
    $clean_ct = preg_replace('/;.*$/', '', $content_type);
    header('Content-Type: ' . $clean_ct);
    header('Access-Control-Allow-Origin: *');
    // Strip some headers that break embedding
    foreach (explode("\r\n", $resp_headers) as $line) {
        $lc = strtolower($line);
        if (str_starts_with($lc, 'x-frame-options:'))           continue;
        if (str_starts_with($lc, 'content-security-policy:'))   continue;
        if (str_starts_with($lc, 'content-security-policy-report-only:')) continue;
    }
    echo $body;
    exit;
}

/* ── Determine base URL for rewriting ───────────────────────── */
$parsed      = parse_url($effective_url);
$scheme      = $parsed['scheme'] ?? 'https';
$host        = $parsed['host']   ?? '';
$port_part   = isset($parsed['port']) ? ':' . $parsed['port'] : '';
$base_origin = $scheme . '://' . $host . $port_part;
$base_path   = isset($parsed['path'])
    ? rtrim(dirname($parsed['path']), '/') . '/'
    : '/';

$self_proxy  = 'http://127.0.0.1:9879/dom-inspector/proxy.php?url=';

$rewrite_url = function(string $url) use ($base_origin, $base_path, $self_proxy, $scheme): string {
    $url = trim($url);
    if ($url === '' || str_starts_with($url, '#') || str_starts_with($url, 'javascript:')
        || str_starts_with($url, 'data:') || str_starts_with($url, 'mailto:')) {
        return $url;
    }
    if (preg_match('#^https?://#i', $url)) {
        return $self_proxy . urlencode($url);
    }
    if (str_starts_with($url, '//')) {
        return $self_proxy . urlencode($scheme . ':' . $url);
    }
    if (str_starts_with($url, '/')) {
        return $self_proxy . urlencode($base_origin . $url);
    }
    return $self_proxy . urlencode($base_origin . $base_path . $url);
};

/* ── Rewrite HTML ────────────────────────────────────────────── */
// Remove <base> tags
$body = preg_replace('#<base[^>]*>#i', '', $body);

// Rewrite href, src, action, srcset attributes
$body = preg_replace_callback(
    '#\b(href|src|action|data-src|data-href)\s*=\s*(["\'])([^"\']*)\2#i',
    function ($m) use ($rewrite_url) {
        $attr  = $m[1];
        $quote = $m[2];
        $url   = $m[3];
        $rw    = $rewrite_url($url);
        return $attr . '=' . $quote . $rw . $quote;
    },
    $body
);

// Rewrite url(...) inside inline CSS
$body = preg_replace_callback(
    '#url\(\s*(["\']?)([^)\'"]+)\1\s*\)#i',
    function ($m) use ($rewrite_url) {
        $q  = $m[1];
        $url = $m[2];
        return 'url(' . $q . $rewrite_url($url) . $q . ')';
    },
    $body
);

/* ── Inspector script to inject ─────────────────────────────── */
$inspector_js = <<<'INSPECTOR_JS'
<script>
(function() {
'use strict';

let _active    = false;
let _overlay   = null;
let _lastEl    = null;

/* ── overlay div ─────────────────────────────────────── */
function makeOverlay() {
    var d = document.createElement('div');
    d.id  = '__xcm_inspector_overlay__';
    d.style.cssText = [
        'position:fixed','top:0','left:0','pointer-events:none',
        'z-index:2147483647','box-sizing:border-box',
        'outline:2px solid #6366f1','background:rgba(99,102,241,0.10)',
        'transition:all 0.05s','display:none'
    ].join(';');
    document.documentElement.appendChild(d);
    return d;
}

function positionOverlay(el) {
    if (!_overlay) _overlay = makeOverlay();
    var r = el.getBoundingClientRect();
    _overlay.style.display  = 'block';
    _overlay.style.top      = r.top  + 'px';
    _overlay.style.left     = r.left + 'px';
    _overlay.style.width    = r.width  + 'px';
    _overlay.style.height   = r.height + 'px';
}

/* ── CSS selector builder ────────────────────────────── */
function getCssSelector(el) {
    if (!el || el.nodeType !== 1) return '';
    var parts = [];
    while (el && el.nodeType === 1 && el !== document.documentElement) {
        var tag = el.tagName.toLowerCase();
        if (el.id) {
            parts.unshift('#' + el.id);
            break;
        }
        var cls = Array.from(el.classList).join('.');
        var sel = tag + (cls ? '.' + cls : '');
        /* nth-child disambiguation */
        var sib = el; var idx = 1;
        while ((sib = sib.previousElementSibling)) {
            if (sib.tagName === el.tagName) idx++;
        }
        var totalSibs = el.parentElement
            ? Array.from(el.parentElement.children).filter(c => c.tagName === el.tagName).length
            : 1;
        if (totalSibs > 1) sel += ':nth-of-type(' + idx + ')';
        parts.unshift(sel);
        el = el.parentElement;
    }
    return parts.join(' > ');
}

/* ── XPath builder ───────────────────────────────────── */
function getXPath(el) {
    if (!el || el.nodeType !== 1) return '';
    if (el.id) return '//*[@id="' + el.id + '"]';
    var parts = [];
    while (el && el.nodeType === 1) {
        var tag = el.tagName.toLowerCase();
        var sib = el; var idx = 1;
        while ((sib = sib.previousElementSibling)) {
            if (sib.tagName === el.tagName) idx++;
        }
        parts.unshift(tag + '[' + idx + ']');
        el = el.parentElement;
    }
    return '/' + parts.join('/');
}

/* ── data collector ──────────────────────────────────── */
function collectData(el) {
    var attrs = {};
    for (var i = 0; i < el.attributes.length; i++) {
        var a = el.attributes[i];
        attrs[a.name] = a.value;
    }

    var computed = {};
    var KEYS = [
        'display','position','width','height','margin','padding',
        'font-size','font-family','font-weight','color','background-color',
        'border','border-radius','flex','grid-template-columns',
        'z-index','opacity','overflow','visibility'
    ];
    try {
        var cs = window.getComputedStyle(el);
        KEYS.forEach(function(k) { computed[k] = cs.getPropertyValue(k); });
    } catch(e) {}

    var children = [];
    for (var c = el.firstElementChild; c; c = c.nextElementSibling) {
        var ci = c.id ? '#'+c.id : (c.className ? '.'+String(c.className).trim().split(/\s+/).join('.') : '');
        children.push(c.tagName.toLowerCase() + (ci ? ci : ''));
    }

    var text = (el.textContent || '').replace(/\s+/g,' ').trim().slice(0, 200);

    return {
        tagName:     el.tagName.toLowerCase(),
        id:          el.id || '',
        className:   el.className || '',
        classes:     Array.from(el.classList),
        attrs:       attrs,
        cssSelector: getCssSelector(el),
        xpath:       getXPath(el),
        children:    children,
        childCount:  el.childElementCount,
        text:        text,
        computed:    computed,
        outerHtml:   el.outerHTML.slice(0, 500),
        rect:        (function(r){return{top:Math.round(r.top),left:Math.round(r.left),width:Math.round(r.width),height:Math.round(r.height)};})(el.getBoundingClientRect()),
    };
}

/* ── event handlers ──────────────────────────────────── */
document.addEventListener('mousemove', function(e) {
    if (!_active) return;
    var el = document.elementFromPoint(e.clientX, e.clientY);
    if (!el || el.id === '__xcm_inspector_overlay__') return;
    _lastEl = el;
    positionOverlay(el);
}, true);

document.addEventListener('click', function(e) {
    if (!_active) return;
    e.preventDefault();
    e.stopPropagation();
    var el = _lastEl || e.target;
    if (!el || el.id === '__xcm_inspector_overlay__') return;
    var data = collectData(el);
    try {
        window.parent.postMessage({type:'xcm_inspect', data: data}, '*');
    } catch(ex) {}
}, true);

document.addEventListener('contextmenu', function(e) {
    if (!_active) return;
    e.preventDefault();
    e.stopPropagation();
    var el = _lastEl || e.target;
    if (!el || el.id === '__xcm_inspector_overlay__') return;
    var data = collectData(el);
    try {
        window.parent.postMessage({type:'xcm_inspect', data: data}, '*');
    } catch(ex) {}
}, true);

/* ── listen for activate/deactivate from parent ──────── */
window.addEventListener('message', function(e) {
    if (e.data && e.data.type === 'xcm_set_active') {
        _active = e.data.active;
        if (!_active && _overlay) _overlay.style.display = 'none';
    }
});

/* ── ready signal to parent ──────────────────────────── */
window.addEventListener('load', function() {
    try { window.parent.postMessage({type:'xcm_ready'}, '*'); } catch(ex) {}
});
try { window.parent.postMessage({type:'xcm_ready'}, '*'); } catch(ex) {}

})();
</script>
INSPECTOR_JS;

/* ── Inject before </body> or at end ───────────────────────── */
if (stripos($body, '</body>') !== false) {
    $body = preg_replace('#</body>#i', $inspector_js . '</body>', $body, 1);
} else {
    $body .= $inspector_js;
}

/* ── Send the response ──────────────────────────────────────── */
header('Content-Type: text/html; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('X-Frame-Options: ALLOWALL');
// Explicitly do NOT send CSP or X-Content-Type-Options that would block the iframe
echo $body;
