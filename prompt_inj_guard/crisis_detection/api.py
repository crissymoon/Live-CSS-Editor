"""
crisis_detection/api.py

Standalone Flask API for the crisis detection module.

Environment variables:
  CRISIS_PATTERNS_PATH   -- path to patterns.json  (default: patterns.json next to this file)
  CRISIS_RESOURCES_PATH  -- path to safe_resources.json (default: sibling file)
  CRISIS_HOST            -- bind host                (default: 127.0.0.1)
  CRISIS_PORT            -- bind port                (default: 5100)

Routes:
  GET  /health           -- liveness check
  POST /detect           -- single text
  POST /detect/bulk      -- list of texts

Usage:
  python api.py
  python api.py --port 5200
  CRISIS_PORT=5200 python api.py
"""

import argparse
import json
import os
import sys
import traceback
from pathlib import Path

# Allow running as a script without installing the package
_DIR = Path(__file__).parent
sys.path.insert(0, str(_DIR.parent.parent))   # project root
sys.path.insert(0, str(_DIR.parent))          # prompt_inj_guard root

try:
    from flask import Flask, jsonify, request
except ImportError:
    print(
        "[crisis_api] Flask is not installed. Run: pip install flask",
        file=sys.stderr,
    )
    sys.exit(1)

from crisis_detection.detector import CrisisDetector, DEFAULT_PATTERNS_PATH, DEFAULT_RESOURCES_PATH


# ------------------------------------------------------------------
# Config helpers

def _resolve_path(env_key: str, fallback: Path) -> Path:
    val = os.environ.get(env_key, "").strip()
    return Path(val) if val else fallback


def _resolve_str(env_key: str, fallback: str) -> str:
    return os.environ.get(env_key, "").strip() or fallback


def _resolve_int(env_key: str, fallback: int) -> int:
    raw = os.environ.get(env_key, "").strip()
    if raw:
        try:
            return int(raw)
        except ValueError:
            print(
                f"[crisis_api] Invalid int for {env_key}={raw!r}, using {fallback}",
                file=sys.stderr,
            )
    return fallback


# ------------------------------------------------------------------
# App factory

def create_app(
    patterns_path: Path | None = None,
    resources_path: Path | None = None,
) -> Flask:
    """Create and configure the Flask app.  Accepts optional path overrides."""

    pat_path = patterns_path or _resolve_path("CRISIS_PATTERNS_PATH",  DEFAULT_PATTERNS_PATH)
    res_path = resources_path or _resolve_path("CRISIS_RESOURCES_PATH", DEFAULT_RESOURCES_PATH)

    detector = CrisisDetector(patterns_path=pat_path, resources_path=res_path)

    app = Flask(__name__)

    # ------------------------------------------------------------------
    # Routes

    @app.get("/health")
    def health():
        try:
            return jsonify({
                "status":          "ok",
                "module":          "crisis_detection",
                "pattern_count":   detector.loaded_pattern_count(),
                "categories":      detector.loaded_categories(),
                "patterns_path":   detector.pattern_db_path(),
                "resources_path":  detector.resources_db_path(),
            })
        except Exception:
            err = traceback.format_exc()
            print(f"[crisis_api] /health error:\n{err}", file=sys.stderr)
            return jsonify({"error": "health check failed", "traceback": err}), 500

    @app.post("/detect")
    def detect():
        try:
            body = request.get_json(silent=True) or {}
            text = body.get("text", "")

            if not isinstance(text, str):
                return jsonify({"error": "'text' field must be a string"}), 400
            if not text.strip():
                return jsonify({"error": "'text' field is required and must not be empty"}), 400

            result = detector.detect(text)
            return jsonify(result)
        except Exception:
            err = traceback.format_exc()
            print(f"[crisis_api] /detect error:\n{err}", file=sys.stderr)
            return jsonify({"error": "detection failed", "traceback": err}), 500

    @app.post("/detect/bulk")
    def detect_bulk():
        try:
            body = request.get_json(silent=True) or {}
            texts = body.get("texts", [])

            if not isinstance(texts, list):
                return jsonify({"error": "'texts' field must be a list"}), 400
            if not texts:
                return jsonify({"error": "'texts' list must not be empty"}), 400

            limit = 200
            if len(texts) > limit:
                return jsonify({
                    "error": f"'texts' list exceeds maximum of {limit} items per request"
                }), 400

            results = detector.detect_bulk(texts)
            return jsonify({
                "count":   len(results),
                "results": results,
            })
        except Exception:
            err = traceback.format_exc()
            print(f"[crisis_api] /detect/bulk error:\n{err}", file=sys.stderr)
            return jsonify({"error": "bulk detection failed", "traceback": err}), 500

    return app


# ------------------------------------------------------------------
# Entry point

def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Crisis Detection API server")
    parser.add_argument("--host", default=None, help="Bind host (overrides CRISIS_HOST)")
    parser.add_argument("--port", type=int, default=None, help="Bind port (overrides CRISIS_PORT)")
    parser.add_argument("--debug", action="store_true", help="Enable Flask debug mode")
    return parser.parse_args()


if __name__ == "__main__":
    args = _parse_args()

    host = args.host or _resolve_str("CRISIS_HOST", "127.0.0.1")
    port = args.port or _resolve_int("CRISIS_PORT", 5100)

    app = create_app()

    print(f"[crisis_api] Starting on http://{host}:{port}", file=sys.stderr)
    app.run(host=host, port=port, debug=args.debug)
