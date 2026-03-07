#!/usr/bin/env python3
"""
webbrowse_no_controls.py -- Launcher for the virt-chrome-bridge Node server.

Starts virt-chrome-bridge.js (port 9928) which opens a native Chrome window
in --app mode (no nav bar, no tab bar, no grab bar) and positions it over the
imgui browser content area on specific URLs.

Usage:
    python webbrowse_no_controls.py [--dev-src DIR]
"""
import sys
import os
import signal
import subprocess
import time
import argparse
import urllib.request

_HERE = os.path.dirname(os.path.abspath(__file__))
_DEV_SRC = os.path.join(_HERE, '..', 'imgui-browser', 'src', 'dev-src')

def find_node():
    for p in ('/opt/homebrew/bin/node', '/usr/local/bin/node', '/usr/bin/node'):
        if os.access(p, os.X_OK):
            return p
    raise RuntimeError('node not found. Install Node.js via homebrew: brew install node')

def health_check(port=9928, retries=20, interval=0.5):
    url = f'http://127.0.0.1:{port}/health'
    for _ in range(retries):
        try:
            resp = urllib.request.urlopen(url, timeout=1)
            if resp.status == 200:
                return True
        except Exception:
            pass
        time.sleep(interval)
    return False

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dev-src', default=_DEV_SRC,
                        help='Path to dev-src directory containing virt-chrome-bridge.js')
    args = parser.parse_args()

    dev_src = os.path.realpath(args.dev_src)
    bridge  = os.path.join(dev_src, 'virt-chrome-bridge.js')

    if not os.path.exists(bridge):
        print(f'[webbrowse_no_controls] ERROR: bridge not found at {bridge}', flush=True)
        sys.exit(1)

    node = find_node()
    print(f'[webbrowse_no_controls] starting virt-chrome-bridge.js ...', flush=True)

    proc = subprocess.Popen(
        [node, bridge],
        cwd=dev_src,
        stdout=sys.stdout,
        stderr=sys.stderr,
    )

    def _shutdown(signum, frame):
        proc.terminate()
        sys.exit(0)

    signal.signal(signal.SIGTERM, _shutdown)
    signal.signal(signal.SIGINT,  _shutdown)

    if health_check():
        print('[webbrowse_no_controls] virt-chrome-bridge ready on :9928', flush=True)
    else:
        print('[webbrowse_no_controls] WARNING: bridge did not become healthy within 10s', flush=True)

    # Keep the launcher alive so server_manager can track this PID.
    proc.wait()
    print(f'[webbrowse_no_controls] bridge exited with code {proc.returncode}', flush=True)

if __name__ == '__main__':
    main()
