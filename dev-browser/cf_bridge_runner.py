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

from PyQt6.QtWidgets import QApplication
from modules.cf_bridge import start_bridge

app = QApplication(sys.argv)

# Respond to SIGTERM from server_manager cleanly.
signal.signal(signal.SIGTERM, lambda _s, _f: app.quit())

start_bridge(app)

print("[cf_bridge_runner] ready on port 9925", flush=True)
sys.exit(app.exec())
