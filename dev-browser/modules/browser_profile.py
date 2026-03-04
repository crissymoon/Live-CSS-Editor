"""
Browser profile, tab, and page classes.
Sets up the persistent QWebEngineProfile and handles downloads.

Submodules:
  _adblock.py        -- domain list, AdBlockInterceptor
  _js_injections.py  -- all injected JS payloads
  _page.py           -- CustomWebEnginePage
  _tab.py            -- BrowserTab
"""

import os

from PyQt6.QtWidgets import QFileDialog, QApplication
from PyQt6.QtWebEngineCore import (
    QWebEngineProfile,
    QWebEngineSettings,
    QWebEngineScript,
    QWebEngineDownloadRequest,
)

from ._adblock       import _AD_DOMAINS, AdBlockInterceptor          # noqa: F401
from ._js_injections import (                                         # noqa: F401
    _SCROLL_FIX_JS,
    _SCROLL_SPEED_JS,
    _CONSENT_KILL_JS,
    _VIDEO_FIX_JS,
    _SPECULATION_RULES_JS,
)
from ._page import CustomWebEnginePage                                # noqa: F401
from ._tab  import BrowserTab                                         # noqa: F401

class PersistentProfile:
    @staticmethod
    def setup_profile() -> QWebEngineProfile:
        profile_path = os.path.expanduser('~/.xcaliburmoon_profile')
        os.makedirs(profile_path, exist_ok=True)

        profile = QWebEngineProfile('xcaliburmoon_profile')

        # Set a real Chrome user-agent so sites like Google do not block us.
        _CHROME_UA = (
            'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) '
            'AppleWebKit/537.36 (KHTML, like Gecko) '
            'Chrome/131.0.0.0 Safari/537.36'
        )
        profile.setHttpUserAgent(_CHROME_UA)
        profile.setHttpAcceptLanguage('en-US,en;q=0.9')

        profile.setPersistentCookiesPolicy(
            QWebEngineProfile.PersistentCookiesPolicy.ForcePersistentCookies
        )
        profile.setCachePath(os.path.join(profile_path, 'cache'))
        profile.setPersistentStoragePath(os.path.join(profile_path, 'storage'))
        profile.setDownloadPath(os.path.expanduser('~/Downloads'))
        profile.setHttpCacheType(QWebEngineProfile.HttpCacheType.DiskHttpCache)
        # 1 GB disk cache -- speeds up repeat visits and media segment reuse
        profile.setHttpCacheMaximumSize(1024 * 1024 * 1024)

        settings = profile.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalStorageEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessFileUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.AllowRunningInsecureContent, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.PluginsEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanOpenWindows, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanAccessClipboard, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.WebGLEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.DnsPrefetchEnabled, True)
        # Ensure images always load and are cached (default True, but set
        # explicitly so a future Qt version regression cannot silently break it).
        settings.setAttribute(QWebEngineSettings.WebAttribute.AutoLoadImages, True)
        # ScrollAnimatorEnabled: keep True on macOS.
        # macOS trackpad delivers many small momentum events after finger lift.
        # With ScrollAnimatorEnabled=False, each event is an abrupt instant
        # jump rather than a smooth interpolation, which produces the
        # "snapping and jumping" effect.  The ScrollAnimator smoothly
        # integrates those events into a fluid motion.
        # The scroll APIs (window.scrollTo / scrollBy / Element.scroll*) are
        # patched in _SCROLL_FIX_JS to force behavior:'instant', so there is
        # no double-animation from the page-JS side.
        try:
            settings.setAttribute(QWebEngineSettings.WebAttribute.ScrollAnimatorEnabled, True)
        except AttributeError:
            pass
        # GPU-accelerated 2D canvas (reduces CPU load on canvas-heavy pages).
        try:
            settings.setAttribute(QWebEngineSettings.WebAttribute.Accelerated2dCanvasEnabled, True)
        except AttributeError:
            pass
        # Disable hyperlink auditing (reduces background pings, marginal speed gain)
        try:
            settings.setAttribute(QWebEngineSettings.WebAttribute.HyperlinkAuditingEnabled, False)
        except AttributeError:
            pass
        # Back/forward cache -- instant page restore on back/forward navigation
        try:
            settings.setAttribute(QWebEngineSettings.WebAttribute.BackForwardCacheEnabled, True)
        except AttributeError:
            pass
        # Allow video/audio to play without requiring a prior user gesture.
        # Without this, LinkedIn and other DASH/HLS players stall silently.
        settings.setAttribute(QWebEngineSettings.WebAttribute.PlaybackRequiresUserGesture, False)
        # Let pages request fullscreen for video players
        settings.setAttribute(QWebEngineSettings.WebAttribute.FullScreenSupportEnabled, True)
        # Allow screen capture (needed by some video conferencing embeds)
        settings.setAttribute(QWebEngineSettings.WebAttribute.ScreenCaptureEnabled, True)

        # ── Widevine CDM: copy to profile directory as a fallback ────
        # Chromium also searches its own user-data directory for CDMs.
        # Symlink the CDM into the profile so both search paths work.
        import shutil as _shutil
        _cdm_src = os.path.join(os.path.dirname(os.path.dirname(__file__)),
                                'WidevineCdm')
        if os.path.isdir(_cdm_src):
            _cdm_dst = os.path.join(profile_path, 'WidevineCdm')
            if not os.path.exists(_cdm_dst):
                try:
                    os.symlink(_cdm_src, _cdm_dst)
                except OSError:
                    try:
                        _shutil.copytree(_cdm_src, _cdm_dst)
                    except Exception:
                        pass

        # Handle downloads with a user-facing save dialog
        profile.downloadRequested.connect(PersistentProfile._on_download_requested)

        # Ad blocker: EasyList + EasyPrivacy + built-in domain list.
        # get_block_set() returns immediately (uses on-disk cache or fallback).
        # refresh_in_background() downloads updated lists without blocking startup.
        from .adblock_lists import get_block_set as _get_bs, refresh_in_background as _refresh_bg
        _block_set = _get_bs(_AD_DOMAINS)
        _refresh_bg(_block_set, _AD_DOMAINS)
        interceptor = AdBlockInterceptor(_block_set)
        profile.setUrlRequestInterceptor(interceptor)
        profile._ad_interceptor = interceptor    # prevent GC
        profile._block_set      = _block_set     # prevent GC
        profile._ad_interceptor = interceptor    # prevent GC
        profile._block_set      = _block_set     # prevent GC


        # Scroll jank fix - injected at DocumentCreation so it beats page JS.
        scroll_fix_script = QWebEngineScript()
        scroll_fix_script.setName('_scroll_fix')
        scroll_fix_script.setSourceCode(_SCROLL_FIX_JS)
        scroll_fix_script.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentCreation)
        scroll_fix_script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        # Subframes inherit the CSS cascade (scroll-snap-type: none, etc.) from
        # the top-level style injection, so running this in every subframe is
        # redundant and adds startup cost for React apps with many lazy iframes.
        scroll_fix_script.setRunsOnSubFrames(False)
        profile.scripts().insert(scroll_fix_script)

        # Trackpad scroll speed reducer - runs at DocumentCreation in every
        # frame so inner overflow containers are covered too.
        scroll_speed_script = QWebEngineScript()
        scroll_speed_script.setName('_scroll_speed')
        scroll_speed_script.setSourceCode(_SCROLL_SPEED_JS)
        scroll_speed_script.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentCreation)
        scroll_speed_script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        scroll_speed_script.setRunsOnSubFrames(True)
        profile.scripts().insert(scroll_speed_script)

        # Cookie consent banner killer - injected at DocumentReady on every page.
        consent_script = QWebEngineScript()
        consent_script.setName('_consent_kill')
        consent_script.setSourceCode(_CONSENT_KILL_JS)
        consent_script.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentReady)
        consent_script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        consent_script.setRunsOnSubFrames(False)
        profile.scripts().insert(consent_script)

        # Video fix - extracts direct MP4 URLs from failing video players
        # and replaces them with native <video src="..."> playback via
        # macOS VideoToolbox (bypasses broken MSE H.264 decode path).
        #
        # Proxy disabled (codec proxy removed).
        video_src = _VIDEO_FIX_JS.replace('%%PROXY_URL%%', '') \
                                  .replace('%%PROXY_OK%%',  'false') \
                                  .replace('%%REMOTE_PROXY%%', '')

        video_fix_script = QWebEngineScript()
        video_fix_script.setName('_video_fix')
        video_fix_script.setSourceCode(video_src)
        video_fix_script.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentCreation)
        video_fix_script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        video_fix_script.setRunsOnSubFrames(True)
        profile.scripts().insert(video_fix_script)

        # Speculation Rules prefetch -- injects <script type="speculationrules">
        # at DocumentReady so Chromium starts prefetching hovered same-origin
        # links before the user clicks.  Only runs in the main frame.
        spec_script = QWebEngineScript()
        spec_script.setName('_speculation_rules')
        spec_script.setSourceCode(_SPECULATION_RULES_JS)
        spec_script.setInjectionPoint(QWebEngineScript.InjectionPoint.DocumentReady)
        spec_script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        spec_script.setRunsOnSubFrames(False)
        profile.scripts().insert(spec_script)

        return profile

    @staticmethod
    def _on_download_requested(download: QWebEngineDownloadRequest):
        """Show a Save As dialog and start the download."""
        suggested = download.downloadFileName()
        save_dir  = download.downloadDirectory() or os.path.expanduser('~/Downloads')

        # Let the user choose where to save
        parent_widget = QApplication.activeWindow()
        dest, _ = QFileDialog.getSaveFileName(
            parent_widget,
            'Save File',
            os.path.join(save_dir, suggested),
        )
        if dest:
            dest_dir  = os.path.dirname(dest)
            dest_name = os.path.basename(dest)
            download.setDownloadDirectory(dest_dir)
            download.setDownloadFileName(dest_name)
            download.accept()
        else:
            download.cancel()
