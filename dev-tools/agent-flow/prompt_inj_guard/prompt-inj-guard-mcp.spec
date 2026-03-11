# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['/Users/mac/Documents/live-css/dev-tools/agent-flow/prompt_inj_guard/mcp_guard_server.py'],
    pathex=['/Users/mac/Documents/live-css/dev-tools/agent-flow/prompt_inj_guard/model'],
    binaries=[],
    datas=[('/Users/mac/Documents/live-css/dev-tools/agent-flow/prompt_inj_guard/api/pattern_db.json', 'api')],
    hiddenimports=['guard_classifier', 'rule_guard'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='prompt-inj-guard-mcp',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
