#!/usr/bin/env python3
"""
Standalone launcher for cf_bridge.

Started automatically by the imgui browser via server_manager.cpp.
Listens on 127.0.0.1:9925 for GET /solve?url=X requests from cf_client.cpp.

Do NOT import this as a module -- run it directly.
"""
import sys
import os
import signal

# Make sure the dev-browser package root is on the path so
# "from modules.cf_bridge import ..." works regardless of cwd.
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)

# Set the activation policy to Accessory BEFORE QApplication is created.
# If this is done after QApplication(), macOS has already activated the app
# and shown the Dock icon / opened a window. Setting it first prevents that.
try:
    from AppKit import NSApplication, NSApplicationActivationPolicyAccessory  # type: ignore
    NSApplication.sharedApplication().setActivationPolicy_(NSApplicationActivationPolicyAccessory)
except Exception as _dock_err:
    print(f"[cf_bridge_runner] dock-hide skipped: {_dock_err}", flush=True)

from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import QTimer
from modules.cf_bridge import start_bridge

app = QApplication(sys.argv)

# QApplication.__init__ calls [NSApp setActivationPolicy:Regular] internally,
# which overwrites the Accessory policy set above and shows a Dock icon.
# Re-asserting here immediately after construction is the reliable fix.
try:
    from AppKit import NSApplication, NSApplicationActivationPolicyAccessory  # type: ignore
    NSApplication.sharedApplication().setActivationPolicy_(NSApplicationActivationPolicyAccessory)
except Exception as _dock_err2:
    print(f"[cf_bridge_runner] post-QApp dock-hide skipped: {_dock_err2}", flush=True)

# PyQt5/6 runs a C++ event loop that never yields to Python, so
# signal.signal(SIGTERM, ...) handlers never fire during app.exec().
# Fix: a short timer forces the event loop to tick Python every 200 ms,
# which is enough for SIGTERM to be delivered and app.quit() to run.
_sig_received = False
def _on_sigterm(_signum, _frame):
    global _sig_received
    _sig_received = True
    app.quit()

signal.signal(signal.SIGTERM, _on_sigterm)
signal.signal(signal.SIGINT,  _on_sigterm)

_py_tick = QTimer()
_py_tick.setInterval(200)
_py_tick.timeout.connect(lambda: None)  # forces return to Python every 200 ms
_py_tick.start()

start_bridge(app)

print("[cf_bridge_runner] ready on port 9925", flush=True)
sys.exit(app.exec())
