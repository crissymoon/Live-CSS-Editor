# -*- mode: python ; coding: utf-8 -*-
#
# PyInstaller spec for Crissy's Browser
#
# Build with:
#   cd dev-browser
#   ./build_app.sh
#
# Or manually (after running build_app.sh to copy PHP to bin/php):
#   pyinstaller dev-browser.spec
#
# Output: dist/CrissyBrowser.app  (macOS .app bundle)
#         dist/CrissyBrowser      (single-dir build, also produced)

import os
import sys

HERE = os.path.abspath(os.path.dirname(SPEC))  # noqa: F821  (SPEC is PyInstaller built-in)

# Path to the bundled PHP binary (placed by build_app.sh)
_php_bin = os.path.join(HERE, 'bin', 'php')

# Collect shared library dependencies of the PHP binary so the .app is
# self-contained.  We use macOS `otool -L` to find what PHP links to, then
# include any non-system libraries.
def _collect_php_dylibs(php_path):
    """Return a list of (src_path, dest_dir_in_app) tuples for PHP dylib deps."""
    if not os.path.isfile(php_path):
        return []
    import subprocess
    try:
        out = subprocess.check_output(['otool', '-L', php_path], text=True)
    except Exception:
        return []
    results = []
    for line in out.splitlines()[1:]:
        line = line.strip().split(' ')[0]
        # Skip system frameworks -- they are always present on macOS
        if (line.startswith('/usr/lib/') or
                line.startswith('/System/') or
                line.startswith('@rpath') or
                line.startswith('@executable_path') or
                line.startswith('@loader_path')):
            continue
        if os.path.isfile(line):
            results.append((line, '.'))
    return results


_php_binaries = []
if os.path.isfile(_php_bin):
    # Include the PHP binary itself
    _php_binaries.append((_php_bin, '.'))
    # Include its non-system dylib dependencies
    _php_binaries.extend(_collect_php_dylibs(_php_bin))


a = Analysis(  # noqa: F821
    [os.path.join(HERE, 'webbrowse.py')],
    pathex=[HERE],
    binaries=_php_binaries,
    datas=[
        # Python modules
        (os.path.join(HERE, 'modules'), 'modules'),
        # Apps directory (PHP mini-apps served at :9879)
        (os.path.join(HERE, 'apps'), 'apps'),
        # Tools manifest
        (os.path.join(HERE, 'tools.json'), '.'),
    ],
    hiddenimports=[
        # PyQt6
        'PyQt6', 'PyQt6.QtCore', 'PyQt6.QtGui', 'PyQt6.QtWidgets',
        'PyQt6.QtWebEngineWidgets', 'PyQt6.QtWebEngineCore',
        'PyQt6.QtNetwork',
        # macOS / pyobjc
        'objc', 'AppKit', 'Foundation', 'WebKit', 'Quartz',
        # Standard lib used at runtime
        'http.server', 'urllib.parse', 'json', 'sqlite3',
        'threading', 'subprocess', 'shutil', 'importlib.util',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)

pyz = PYZ(a.pure)  # noqa: F821

exe = EXE(  # noqa: F821
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='CrissyBrowser',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=False,   # no terminal window on launch
    disable_windowed_traceback=False,
    argv_emulation=True,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)

coll = COLLECT(  # noqa: F821
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=False,
    upx_exclude=[],
    name='CrissyBrowser',
)

app = BUNDLE(  # noqa: F821
    coll,
    name='CrissyBrowser.app',
    icon=None,
    bundle_identifier='net.xcaliburmoon.crissy-browser',
    info_plist={
        'CFBundleDisplayName': "Crissy's Browser",
        'CFBundleShortVersionString': '1.0.0',
        'NSHighResolutionCapable': True,
        'NSRequiresAquaSystemAppearance': False,
    },
)
