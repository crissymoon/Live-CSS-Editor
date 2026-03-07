#!/usr/bin/env bash
# Run Crissy's Browser using the dev-browser venv Python.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/venv/bin/python3" "$SCRIPT_DIR/webbrowse.py" "$@"
