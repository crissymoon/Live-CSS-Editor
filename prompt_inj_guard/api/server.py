#!/usr/bin/env python3
"""
prompt_inj_guard/api/server.py

Flask REST API wrapping the GuardClassifier.
Loads the DistilBERT model once at startup and serves inference requests.

Endpoints
---------
GET  /health        -- liveness check; returns model load status
POST /classify      -- classify one text string
POST /classify/bulk -- classify a list of text strings (max 64 per call)

Start
-----
    python server.py [--host 0.0.0.0] [--port 8765] [--model-dir PATH]

Environment variables (override CLI defaults)
---------------------------------------------
    GUARD_MODEL_DIR   path to the model directory (default: ../model/spam_injection_model/final)
    GUARD_HOST        bind host   (default: 0.0.0.0)
    GUARD_PORT        bind port   (default: 8765)

Fallback error handling: every route wraps its logic in try/except and
returns a structured JSON error so callers always get a usable response.
Critical startup errors are printed to stderr and cause a non-zero exit.
"""

import os
import sys
import time
import argparse
import traceback

# ---------------------------------------------------------------------------
# Resolve model directory before importing heavy deps so startup errors are
# clear even if torch is missing.
# ---------------------------------------------------------------------------

_DEFAULT_MODEL_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    '..', 'model', 'spam_injection_model', 'final'
)

def _resolve_model_dir(cli_arg: str | None) -> str:
    d = cli_arg or os.environ.get('GUARD_MODEL_DIR') or _DEFAULT_MODEL_DIR
    d = os.path.realpath(d)
    if not os.path.isdir(d):
        print(f'ERROR: model directory not found: {d}', file=sys.stderr)
        print('  Set --model-dir or GUARD_MODEL_DIR to the path containing', file=sys.stderr)
        print('  model.safetensors, tokenizer.json, config.json', file=sys.stderr)
        sys.exit(1)
    return d

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

try:
    from flask import Flask, request, jsonify
except ImportError:
    print('ERROR: Flask not installed. Run: pip install flask', file=sys.stderr)
    sys.exit(1)

# Guard classifier lives one level up in model/
_CLASSIFIER_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    '..', 'model'
)
if _CLASSIFIER_DIR not in sys.path:
    sys.path.insert(0, _CLASSIFIER_DIR)

try:
    from guard_classifier import GuardClassifier
except ImportError as _e:
    print(f'ERROR: cannot import GuardClassifier: {_e}', file=sys.stderr)
    print('  Make sure torch and transformers are installed:', file=sys.stderr)
    print('  pip install torch transformers numpy', file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

app = Flask(__name__)

_guard: GuardClassifier | None = None
_model_dir: str = ''
_load_error: str = ''
_load_time_s: float = 0.0

BULK_LIMIT = 64

# ---------------------------------------------------------------------------
# Startup loader (called after arg parsing)
# ---------------------------------------------------------------------------

def load_model(model_dir: str) -> None:
    global _guard, _model_dir, _load_error, _load_time_s
    _model_dir = model_dir
    print(f'Loading model from: {model_dir}', flush=True)
    t0 = time.time()
    try:
        _guard = GuardClassifier(model_dir)
        _load_time_s = round(time.time() - t0, 2)
        print(f'Model loaded in {_load_time_s}s', flush=True)
    except Exception as e:
        _load_error = str(e)
        traceback.print_exc()
        print(f'ERROR: model failed to load: {_load_error}', file=sys.stderr)
        # Do not exit -- health endpoint will report the error so callers can
        # detect it without a TCP connection refused.

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _err(msg: str, status: int = 400):
    return jsonify({'ok': False, 'error': msg}), status

def _require_model():
    """Return an error response if model is not loaded, else None."""
    if _guard is None:
        reason = _load_error or 'model not loaded yet'
        return _err(f'Model unavailable: {reason}', 503)
    return None

# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.get('/health')
def health():
    return jsonify({
        'ok':          _guard is not None,
        'model_dir':   _model_dir,
        'load_time_s': _load_time_s,
        'error':       _load_error or None,
    })


@app.post('/classify')
def classify():
    try:
        guard_err = _require_model()
        if guard_err:
            return guard_err

        body = request.get_json(silent=True)
        if not body:
            return _err('Request body must be JSON with a "text" field', 400)

        text = body.get('text')
        if text is None:
            return _err('Missing required field: text', 400)
        if not isinstance(text, str):
            return _err('"text" must be a string', 400)
        if not text.strip():
            return _err('"text" must not be blank', 400)

        result = _guard.classify(text)
        return jsonify({'ok': True, **result})

    except Exception as e:
        traceback.print_exc()
        return _err(f'Internal error: {e}', 500)


@app.post('/classify/bulk')
def classify_bulk():
    try:
        guard_err = _require_model()
        if guard_err:
            return guard_err

        body = request.get_json(silent=True)
        if not body:
            return _err('Request body must be JSON with a "texts" array', 400)

        texts = body.get('texts')
        if texts is None:
            return _err('Missing required field: texts', 400)
        if not isinstance(texts, list):
            return _err('"texts" must be an array of strings', 400)
        if len(texts) == 0:
            return _err('"texts" array is empty', 400)
        if len(texts) > BULK_LIMIT:
            return _err(f'"texts" exceeds bulk limit of {BULK_LIMIT}', 400)

        results = []
        for i, text in enumerate(texts):
            if not isinstance(text, str) or not text.strip():
                results.append({
                    'index': i,
                    'ok':    False,
                    'error': 'item is not a non-empty string',
                })
                continue
            try:
                r = _guard.classify(text)
                results.append({'index': i, 'ok': True, **r})
            except Exception as item_err:
                print(f'ERROR classify_bulk item {i}: {item_err}', file=sys.stderr)
                results.append({'index': i, 'ok': False, 'error': str(item_err)})

        any_flagged = any(r.get('flagged') for r in results if r.get('ok'))
        return jsonify({'ok': True, 'results': results, 'any_flagged': any_flagged})

    except Exception as e:
        traceback.print_exc()
        return _err(f'Internal error: {e}', 500)


@app.errorhandler(404)
def not_found(e):
    return _err('Endpoint not found', 404)


@app.errorhandler(405)
def method_not_allowed(e):
    return _err('Method not allowed', 405)


@app.errorhandler(Exception)
def unhandled(e):
    traceback.print_exc()
    return _err(f'Unhandled error: {e}', 500)

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Prompt injection guard API server')
    parser.add_argument('--host',      default=os.environ.get('GUARD_HOST', '0.0.0.0'))
    parser.add_argument('--port',      default=int(os.environ.get('GUARD_PORT', '8765')), type=int)
    parser.add_argument('--model-dir', default=None,
                        help='Path to model directory (default: ../model/spam_injection_model/final)')
    parser.add_argument('--debug',     action='store_true', help='Enable Flask debug mode')
    args = parser.parse_args()

    model_dir = _resolve_model_dir(args.model_dir)
    load_model(model_dir)

    print(f'Starting server on {args.host}:{args.port}', flush=True)
    app.run(host=args.host, port=args.port, debug=args.debug)
