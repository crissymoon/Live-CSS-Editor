#!/usr/bin/env python3
"""
Locate or download the Widevine CDM for use with QtWebEngine.

QtWebEngine (Chromium-based) supports loading an external Widevine CDM
as a "component CDM".  The CDM is a proprietary Google binary that
handles DRM decryption for EME (Encrypted Media Extensions).

This script:
  1. Searches common locations for an existing Widevine CDM.
  2. If not found, downloads the latest CDM from Google's component
     update server (the same way Chrome and Firefox fetch it).
  3. Reports the path and version for use by webbrowse.py.

The CDM is NOT open source.  Google distributes it freely but it is
governed by a separate license.  It is the same binary that Chrome,
Firefox, Opera, and every other browser downloads automatically.
"""

import glob
import json
import os
import platform
import shutil
import ssl
import struct
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

# SSL context that skips certificate verification (same as codec_proxy.py).
# Needed on macOS where Python's default cert bundle is often missing.
_SSL_CTX = ssl.create_default_context()
_SSL_CTX.check_hostname = False
_SSL_CTX.verify_mode = ssl.CERT_NONE

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
_HERE = os.path.dirname(os.path.abspath(__file__))
_CDM_DIR = os.path.join(os.path.dirname(_HERE), 'WidevineCdm')

# Google's component update endpoint for Widevine CDM.
# This is the same endpoint Chrome and Firefox use.
_UPDATE_URL = 'https://update.googleapis.com/service/update2/json'

# Widevine's extension/component ID used by Chromium's component updater.
_WIDEVINE_APP_ID = 'oimompecagnajdejgnnjijobebaeigek'

# Architecture
_ARCH = platform.machine()
if _ARCH == 'x86_64':
    _PLATFORM = 'mac-x64'
    _PLATFORM_DIR = 'mac_x64'
elif _ARCH == 'arm64':
    _PLATFORM = 'mac-arm64'
    _PLATFORM_DIR = 'mac_arm64'
else:
    _PLATFORM = 'mac-x64'
    _PLATFORM_DIR = 'mac_x64'


def _find_existing_cdm():
    """
    Search common locations for an existing Widevine CDM installation.
    Returns (path_to_dir, version) or (None, None).
    """
    candidates = [
        # Our own bundled copy
        _CDM_DIR,
        # Chrome
        os.path.expanduser(
            '~/Library/Application Support/Google/Chrome/WidevineCdm'),
        # Chrome Beta
        os.path.expanduser(
            '~/Library/Application Support/Google/Chrome Beta/WidevineCdm'),
        # Chrome Canary
        os.path.expanduser(
            '~/Library/Application Support/Google/Chrome Canary/WidevineCdm'),
        # Chromium
        os.path.expanduser(
            '~/Library/Application Support/Chromium/WidevineCdm'),
        # Brave
        os.path.expanduser(
            '~/Library/Application Support/BraveSoftware/Brave-Browser/WidevineCdm'),
        # Edge
        os.path.expanduser(
            '~/Library/Application Support/Microsoft Edge/WidevineCdm'),
        # Vivaldi
        os.path.expanduser(
            '~/Library/Application Support/Vivaldi/WidevineCdm'),
        # Opera
        os.path.expanduser(
            '~/Library/Application Support/com.operasoftware.Opera/WidevineCdm'),
        # Firefox (stores it differently)
        os.path.expanduser(
            '~/Library/Application Support/Mozilla/Firefox/Profiles'),
        # System-wide
        '/Library/Google/WidevineCdm',
    ]

    for base in candidates:
        if not os.path.isdir(base):
            continue

        # Chrome/Chromium structure: WidevineCdm/<version>/_platform_specific/<arch>/
        # Look for manifest.json first
        for manifest in glob.glob(
                os.path.join(base, '**/manifest.json'), recursive=True):
            mdir = os.path.dirname(manifest)
            try:
                with open(manifest) as f:
                    mdata = json.load(f)
                version = mdata.get('version', '')
                if not version:
                    continue
            except Exception:
                continue

            # Find the actual dylib
            dylib = None
            for pattern in [
                os.path.join(mdir, '_platform_specific', _PLATFORM_DIR,
                             'libwidevinecdm.dylib'),
                os.path.join(mdir, 'libwidevinecdm.dylib'),
            ]:
                if os.path.isfile(pattern):
                    dylib = pattern
                    break

            # Also search recursively from manifest dir
            if not dylib:
                for f in glob.glob(
                        os.path.join(mdir, '**', 'libwidevinecdm.dylib'),
                        recursive=True):
                    dylib = f
                    break

            if dylib:
                print(f'[widevine] Found CDM v{version} at {mdir}')
                return mdir, version

        # Firefox stores it as gmp-widevinecdm/<version>/
        for dylib in glob.glob(
                os.path.join(base, '**/libwidevinecdm.dylib'),
                recursive=True):
            mdir = os.path.dirname(dylib)
            # Try to read version from id file or directory name
            version = os.path.basename(
                os.path.dirname(mdir)) if mdir else 'unknown'
            print(f'[widevine] Found CDM (Firefox) at {mdir}')
            return mdir, version

    return None, None


def _download_cdm():
    """
    Download the Widevine CDM from Google's component update server.
    Returns (path_to_dir, version) or (None, None) on failure.

    This uses the same protocol that Chrome and Firefox use to fetch
    the CDM component.
    """
    print('[widevine] Downloading Widevine CDM from Google...')

    # Build the update check request (Omaha protocol v3 JSON).
    os_version = platform.mac_ver()[0] or '12.0'
    arch = 'x64' if _ARCH == 'x86_64' else 'arm64'

    request_body = json.dumps({
        'request': {
            '@os': 'mac',
            '@updater': 'chromiumcrx',
            'acceptformat': 'crx3',
            'app': [{
                'appid': _WIDEVINE_APP_ID,
                'version': '0.0.0.0',
                'updatecheck': {},
            }],
            'arch': arch,
            'hw': {},
            'os': {
                'arch': arch,
                'platform': 'Mac OS X',
                'version': os_version,
            },
            'prodversion': '120.0.0.0',
            'protocol': '3.1',
        }
    }).encode('utf-8')

    try:
        req = urllib.request.Request(
            _UPDATE_URL,
            data=request_body,
            headers={
                'Content-Type': 'application/json',
                'User-Agent': 'Mozilla/5.0',
            },
        )
        resp = urllib.request.urlopen(req, timeout=30, context=_SSL_CTX)
        # Response starts with )]}\n then JSON
        raw = resp.read().decode('utf-8')
        # Strip safety prefix if present
        if raw.startswith(')]}\n') or raw.startswith(")]}'\n"):
            raw = raw.split('\n', 1)[1]
        data = json.loads(raw)
    except Exception as e:
        print(f'[widevine] Update check failed: {e}')
        return None, None

    # Parse the response to find the download URL.
    try:
        app = data['response']['app'][0]
        uc = app['updatecheck']
        if uc.get('status') != 'ok':
            print(f'[widevine] No update available: {uc.get("status")}')
            return None, None
        manifest = uc['manifest']
        version = manifest['version']
        pkg = manifest['packages']['package'][0]
        pkg_name = pkg['name']
        # Build download URL from codebase
        urls = uc['urls']['url']
        base_url = urls[0]['codebase']
        download_url = base_url + pkg_name
    except (KeyError, IndexError) as e:
        print(f'[widevine] Failed to parse update response: {e}')
        return None, None

    print(f'[widevine] Found CDM v{version}, downloading...')

    # Download the CRX3 package.
    try:
        req = urllib.request.Request(download_url, headers={
            'User-Agent': 'Mozilla/5.0',
        })
        resp = urllib.request.urlopen(req, timeout=60, context=_SSL_CTX)
        crx_data = resp.read()
    except Exception as e:
        print(f'[widevine] Download failed: {e}')
        return None, None

    print(f'[widevine] Downloaded {len(crx_data)} bytes')

    # CRX3 format: magic(4) + version(4) + header_size(4) + header + ZIP
    # Magic: "Cr24", Version: 3
    if crx_data[:4] != b'Cr24':
        print('[widevine] Not a CRX3 file, trying as raw ZIP...')
        zip_data = crx_data
    else:
        crx_version = struct.unpack('<I', crx_data[4:8])[0]
        header_size = struct.unpack('<I', crx_data[8:12])[0]
        zip_offset = 12 + header_size
        zip_data = crx_data[zip_offset:]

    # Extract to our CDM directory.
    cdm_versioned = os.path.join(_CDM_DIR, version)
    os.makedirs(cdm_versioned, exist_ok=True)

    try:
        tmpzip = tempfile.NamedTemporaryFile(suffix='.zip', delete=False)
        tmpzip.write(zip_data)
        tmpzip.close()

        with zipfile.ZipFile(tmpzip.name) as zf:
            zf.extractall(cdm_versioned)

        os.unlink(tmpzip.name)
    except Exception as e:
        print(f'[widevine] Extraction failed: {e}')
        try:
            os.unlink(tmpzip.name)
        except OSError:
            pass
        return None, None

    # Verify the dylib exists.
    dylib = None
    for f in glob.glob(os.path.join(cdm_versioned, '**',
                                     'libwidevinecdm.dylib'),
                       recursive=True):
        dylib = f
        break

    if not dylib:
        # Check if it was extracted flat (no _platform_specific)
        if os.path.isfile(os.path.join(cdm_versioned, 'libwidevinecdm.dylib')):
            dylib = os.path.join(cdm_versioned, 'libwidevinecdm.dylib')

    if not dylib:
        print('[widevine] dylib not found in downloaded package.')
        print('  Contents:', os.listdir(cdm_versioned))
        return None, None

    # Ad-hoc sign the dylib so macOS allows loading it.
    try:
        subprocess.run(
            ['codesign', '--sign', '-', '--force', dylib],
            capture_output=True, timeout=10,
        )
    except Exception:
        pass

    print(f'[widevine] CDM v{version} installed at {cdm_versioned}')
    return cdm_versioned, version


def ensure_widevine():
    """
    Find or download the Widevine CDM.
    Returns (cdm_dir, version) or (None, None).
    """
    path, version = _find_existing_cdm()
    if path:
        return path, version

    print('[widevine] No existing CDM found, attempting download...')
    return _download_cdm()


if __name__ == '__main__':
    path, version = ensure_widevine()
    if path:
        print(f'\nWidevine CDM ready:')
        print(f'  Path:    {path}')
        print(f'  Version: {version}')

        # Show all files
        for root, dirs, files in os.walk(path):
            for f in files:
                fp = os.path.join(root, f)
                sz = os.path.getsize(fp)
                print(f'  {os.path.relpath(fp, path)}  ({sz} bytes)')
    else:
        print('\nFailed to obtain Widevine CDM.')
        sys.exit(1)
