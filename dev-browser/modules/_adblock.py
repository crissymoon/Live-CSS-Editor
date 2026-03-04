"""
Ad-block domain list, compiled regex, and URL request interceptor.
"""
import re
from PyQt6.QtWebEngineCore import (
    QWebEngineUrlRequestInterceptor,
    QWebEngineUrlRequestInfo,
)

_AD_DOMAINS = {
    # Google advertising / tracking
    'pagead2.googlesyndication.com', 'adservice.google.com',
    'googletagservices.com', 'googletagmanager.com',
    'googleadservices.com', 'doubleclick.net', 'googlesyndication.com',
    'google-analytics.com', 'stats.g.doubleclick.net',
    # Facebook / Meta
    'connect.facebook.net', 'facebook.com/tr',
    'an.facebook.com', 'graph.facebook.com/tr',
    # Amazon advertising
    'aax.amazon-adsystem.com', 'amazon-adsystem.com',
    's.amazon-adsystem.com', 'fls-na.amazon.com',
    # Microsoft / Bing ads
    'bat.bing.com', 'ads.msn.com', 'adnxs.com', 'adsrvr.org',
    # Twitter / X ads
    'ads-twitter.com', 'ads.twitter.com', 't.co',
    # Major ad networks
    'outbrain.com', 'taboola.com', 'revcontent.com',
    'advertising.com', 'aol.com/ad', 'oath.com',
    'pubmatic.com', 'rubiconproject.com', 'openx.net', 'openx.com',
    'contextweb.com', 'casalemedia.com', 'indexexchange.com',
    'appnexus.com', 'criteo.com', 'criteo.net',
    'smartadserver.com', 'serving-sys.com', 'sizmek.com',
    'media.net', 'yieldmo.com', 'sharethrough.com',
    'lijit.com', 'sovrn.com', 'districtm.io', 'districtmtech.com',
    'spotxchange.com', 'spotx.tv', 'springserve.com',
    'triplelift.com', '33across.com', 'conversantmedia.com',
    'emxdgt.com', 'rhythmone.com', '1rx.io',
    'bidswitch.net', 'bidswitch.com',
    'tremorhub.com', 'telaria.com',
    'magnite.com', 'rubiconproject.com',
    'lkqd.net', 'lkqd.com',
    'yieldnexus.com', 'yieldlab.net', 'yieldlab.de',
    'adtelligent.com', 'between.digital',
    'admedo.com', 'admixer.net',
    'banner.solutions', 'synacormedia.com',
    'teads.tv', 'teads.com',
    'unruly.co', 'videologygroup.com',
    'springserve.net', 'admanmedia.com',
    'cedexis.com', 'cedexis-test.com',
    # Trackers / analytics
    'hotjar.com', 'mouseflow.com', 'fullstory.com',
    'heap.io', 'heapanalytics.com',
    'quantserve.com', 'quantcast.com',
    'scorecardresearch.com', 'comscore.com',
    'chartbeat.com', 'chartbeat.net',
    'parsely.com', 'pixel.parsely.com',
    'mxpnl.com', 'mixpanel.com',
    'segment.io', 'segment.com',
    'amplitude.com', 'cdn.amplitude.com',
    'kissmetrics.com', 'kissmetrics.io',
    'crazyegg.com', 'heatmap.com',
    'clicky.com', 'statcounter.com',
    'woopra.com', 'intercom.io', 'intercom.com',
    'luckyorange.com', 'luckyorange.net',
    # Consent management platforms (the cookie popup providers)
    'cookielaw.org', 'cookiebot.com',
    'onetrust.com', 'cdn.cookielaw.org',
    'consent.cookiebot.com',
    'trustarc.com', 'truste.com',
    'evidon.com', 'crownpeak.com',
    'usercentrics.eu', 'usercentrics.com',
    'consentmanager.net', 'consentmanager.de',
    'quantcast.mgr.consensu.org',
    # Misc trackers
    'tiqcdn.com', 'tealiumiq.com', 'tealium.com',
    'bluekai.com', 'demdex.net',
    'lijit.com', 'nexac.com',
    'rlcdn.com', 'addthis.com', 'sharethis.com',
    'semasio.net', 'semasio.com',
    'adition.com', 'intelliad.de',
    'weborama.fr', 'weborama.com',
    'omtrdc.net', 'adobedtm.com', '2o7.net',
    'mookie1.com', 'turn.com',
    'rfihub.com', 'rfihub.net',
    'eyeota.net', 'krxd.net',
    'adsymptotic.com', 'adform.net',
    'adform.com', 'adzerk.com', 'adzerk.net',
    'zedo.com', 'undertone.com',
    'buysellads.com', 'buysellads.net',
    'carbonads.com', 'carbonads.net',
}

# Build a compiled regex from the domain list for fast matching.
_AD_RE = re.compile(
    r'(?:^|\.)(?:' +
    '|'.join(re.escape(d) for d in sorted(_AD_DOMAINS, key=len, reverse=True)) +
    r')(?:\.|/|$)',
    re.IGNORECASE,
)




class AdBlockInterceptor(QWebEngineUrlRequestInterceptor):
    """Block third-party ad/tracker requests at the network level.

    Uses a _HostSet from adblock_lists for O(depth) matching against
    50k+ EasyList/EasyPrivacy domains, updated in a background thread.
    """

    def __init__(self, block_set, parent=None):
        super().__init__(parent)
        self._block_set = block_set

    def interceptRequest(self, info: QWebEngineUrlRequestInfo):
        url  = info.requestUrl()
        host = url.host().lower()
        if not host:
            return
        # Only block third-party requests -- never the page's own origin.
        first_party = info.firstPartyUrl().host().lower()
        if first_party and (host == first_party or host.endswith('.' + first_party)):
            return
        if self._block_set.matches(host):
            info.block(True)


# ── Scroll jank fix ──────────────────────────────────────────────────────────
# Injected at DocumentCreation -- before ANY page script -- so scroll API
# patches and the anti-snap CSS are in place before the page can undo them.
#
# Problem: pages call window.scrollTo({top: x, behavior: 'smooth'}) or set
# scroll-behavior/scroll-snap-type in CSS.  On macOS, this stacks a CSS
# animation on top of the trackpad's own inertia delivery, causing the visible
# snap/jump: JS moves the page to position X, then macOS inertia drifts it
# away, then JS corrects again -- a rapid position fight that looks like
# flicker and snapping.  The CSS scroll-snap-type variant snaps the scroll
# container to the nearest snap-point on every inertia tick, which looks
# like the page is jumping between sections erratically.
