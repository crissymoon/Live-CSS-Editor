#!/usr/bin/env python3
"""
prompt_inj_guard/api/server.py - Two-layer guard API server.

Layer 1: DistilBERT ML classifier (guard_classifier.py)
Layer 2: Regex / heuristic rule guard (rule_guard.py + pattern_db.json)

Both layers run on every request. A combiner merges their signals:
  - Strong rule hit (>= 0.80) overrides model -- unless allowlisted
  - Allowlist match overrides a model false positive back to clean
  - Both agree non-clean: confidence is boosted
  - Low ML confidence + allowlist match: returned as clean

Endpoints
---------
GET  /health              liveness; model + rule guard status
POST /classify            single text, combined result
POST /classify/bulk       up to 64 texts
POST /feedback            human correction; logged to feedback_log.jsonl
                          feeds offline learn.py to grow pattern_db.json

Start
-----
    python server.py [--host 0.0.0.0] [--port 8765] [--model-dir PATH]

Environment variables
---------------------
    GUARD_MODEL_DIR   model weights dir  (default: ../model/spam_injection_model/final)
    GUARD_DB_PATH     pattern_db.json    (default: ./pattern_db.json)
    GUARD_FEEDBACK    feedback log       (default: ./feedback_log.jsonl)
    GUARD_HOST        bind host          (default: 0.0.0.0)
    GUARD_PORT        bind port          (default: 8765)

Fallback error handling: every route and every sub-call is wrapped in
try/except and returns a structured JSON body so callers always get context.
"""

import os
import sys
import time
import json
import argparse
import traceback
import threading
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Path constants
# ---------------------------------------------------------------------------

_HERE = os.path.dirname(os.path.abspath(__file__))
_DEFAULT_MODEL_DIR = os.path.realpath(
    os.path.join(_HERE, '..', 'model', 'spam_injection_model', 'final')
)
_DEFAULT_DB_PATH  = os.path.join(_HERE, 'pattern_db.json')
_DEFAULT_FEEDBACK = os.path.join(_HERE, 'feedback_log.jsonl')


def _resolve_model_dir(cli_arg):
    d = cli_arg or os.environ.get('GUARD_MODEL_DIR') or _DEFAULT_MODEL_DIR
    d = os.path.realpath(d)
    if not os.path.isdir(d):
        print(f'ERROR: model directory not found: {d}', file=sys.stderr)
        print('  Set --model-dir or GUARD_MODEL_DIR', file=sys.stderr)
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

_CLASSIFIER_DIR = os.path.join(_HERE, '..', 'model')
if _CLASSIFIER_DIR not in sys.path:
    sys.path.insert(0, _CLASSIFIER_DIR)

try:
    from guard_classifier import GuardClassifier
except ImportError as _e:
    print(f'ERROR: cannot import GuardClassifier: {_e}', file=sys.stderr)
    print('  pip install torch transformers numpy', file=sys.stderr)
    sys.exit(1)

try:
    from rule_guard import RuleGuard
except ImportError as _e:
    print(f'ERROR: cannot import RuleGuard: {_e}', file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------------------
# App + globals
# ---------------------------------------------------------------------------

app = Flask(__name__)

_guard         = None   # GuardClassifier instance
_rule_guard    = None   # RuleGuard instance
_model_dir     = ''
_db_path       = ''
_feedback_path = ''
_load_error    = ''
_load_time_s   = 0.0
_feedback_lock = threading.Lock()

BULK_LIMIT               = 64
RULES_OVERRIDE_THRESHOLD = 0.80   # rule confidence at which rules override model
MODEL_FLAG_THRESHOLD     = 0.70   # model confidence at which model flags non-clean
MODEL_LOW_CONF_THRESHOLD = 0.55   # lower bound -- allowlist still gets a veto

# ---------------------------------------------------------------------------
# Startup
# ---------------------------------------------------------------------------

def load_model(model_dir, db_path, feedback_path):
    global _guard, _rule_guard, _model_dir, _db_path, _feedback_path
    global _load_error, _load_time_s
    _model_dir     = model_dir
    _db_path       = db_path
    _feedback_path = feedback_path

    print(f'Loading ML model from:   {model_dir}', flush=True)
    print(f'Loading pattern DB from: {db_path}',   flush=True)
    t0 = time.time()
    try:
        _guard = GuardClassifier(model_dir)
        _load_time_s = round(time.time() - t0, 2)
        print(f'ML model loaded in {_load_time_s}s', flush=True)
    except Exception as e:
        _load_error = str(e)
        traceback.print_exc()
        print(f'ERROR: ML model failed to load: {_load_error}', file=sys.stderr)

    try:
        _rule_guard = RuleGuard(db_path)
        print('RuleGuard loaded ok', flush=True)
    except Exception as e:
        print(f'ERROR: RuleGuard failed to init: {e}', file=sys.stderr)
        _rule_guard = None   # continue without rule layer

# ---------------------------------------------------------------------------
# Combiner
# ---------------------------------------------------------------------------

def _combine(text):
    """
    Run ML + rule layers and merge decisions.

    Priority
    --------
    1. Strong rule hit + allowlisted       -> clean (suppress false positive)
    2. Strong rule hit, not allowlisted    -> rules win (boost if model agrees)
    3. Model high-conf non-clean           -> allowlist veto OR model wins
                                              (boost if rules agree)
    4. Model low-conf non-clean            -> allowlist veto OR low-conf flag
    5. Default                             -> clean
    """
    # Layer 1 -- ML
    try:
        ml = _guard.classify(text)
    except Exception as e:
        print(f'ERROR _combine ML classify: {e}', file=sys.stderr)
        ml = {'label': 'clean', 'confidence': 0.5, 'flagged': False, 'scores': {}}

    # Layer 2 -- rules
    rule = {
        'label': 'clean', 'confidence': 0.0, 'flagged': False,
        'matched': [], 'is_allowlisted': False,
        'allowlist_matches': [], 'allowlist_conf': 0.0,
    }
    if _rule_guard is not None:
        try:
            rule = _rule_guard.check(text)
        except Exception as e:
            print(f'ERROR _combine rule check: {e}', file=sys.stderr)

    ml_label    = ml['label']
    ml_conf     = float(ml['confidence'])
    rule_label  = rule['label']
    rule_conf   = float(rule['confidence'])
    allowlisted = rule['is_allowlisted']
    allow_conf  = float(rule['allowlist_conf'])

    # Decision
    final_label = 'clean'
    final_conf  = ml_conf if ml_label == 'clean' else 0.5
    source      = 'model'

    if rule['flagged'] and rule_conf >= RULES_OVERRIDE_THRESHOLD and allowlisted:
        final_label = 'clean'
        final_conf  = max(allow_conf, 0.75)
        source      = 'allowlist_override'

    elif rule['flagged'] and rule_conf >= RULES_OVERRIDE_THRESHOLD:
        final_label = rule_label
        if ml_label == rule_label:
            final_conf = min(0.99, (rule_conf + ml_conf) / 2.0 + 0.08)
            source     = 'rules+model_agree'
        else:
            final_conf = rule_conf
            source     = 'rules_override'

    elif ml_label != 'clean' and ml_conf >= MODEL_FLAG_THRESHOLD:
        if allowlisted:
            final_label = 'clean'
            final_conf  = max(allow_conf, 0.78)
            source      = 'allowlist_override'
        elif rule_label == ml_label:
            final_label = ml_label
            final_conf  = min(0.99, ml_conf + 0.07)
            source      = 'model+rules_agree'
        else:
            final_label = ml_label
            final_conf  = ml_conf
            source      = 'model'

    elif ml_label != 'clean' and ml_conf >= MODEL_LOW_CONF_THRESHOLD:
        if allowlisted:
            final_label = 'clean'
            final_conf  = max(allow_conf, 0.72)
            source      = 'allowlist_override'
        else:
            final_label = ml_label
            final_conf  = ml_conf
            source      = 'model_low_conf'

    else:
        final_label = 'clean'
        final_conf  = ml_conf if ml_label == 'clean' else 0.6
        source      = 'model'

    return {
        'label':             final_label,
        'confidence':        round(final_conf, 4),
        'flagged':           final_label != 'clean',
        'scores':            ml.get('scores', {}),
        'source':            source,
        'ml_label':          ml_label,
        'ml_confidence':     round(ml_conf, 4),
        'rule_label':        rule_label,
        'rule_confidence':   round(rule_conf, 4),
        'rule_matches':      rule['matched'],
        'allowlist_matches': rule['allowlist_matches'],
    }

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _err(msg, status=400):
    return jsonify({'ok': False, 'error': msg}), status


def _require_model():
    if _guard is None:
        reason = _load_error or 'model not loaded yet'
        return _err(f'Model unavailable: {reason}', 503)
    return None


def _write_feedback(entry):
    """Append a feedback record to the JSONL log. Thread-safe."""
    try:
        with _feedback_lock:
            with open(_feedback_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps(entry, ensure_ascii=False) + '\n')
    except Exception as e:
        print(f'ERROR writing feedback log: {e}', file=sys.stderr)


def _validate_text(text):
    """Return error string if text is invalid, else None."""
    if text is None:
        return 'Missing required field: text'
    if not isinstance(text, str):
        return '"text" must be a string'
    if not text.strip():
        return '"text" must not be blank'
    return None

# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.get('/health')
def health():
    rule_status = _rule_guard.status() if _rule_guard else {'error': 'not loaded'}
    return jsonify({
        'ok':           _guard is not None,
        'model_dir':    _model_dir,
        'load_time_s':  _load_time_s,
        'error':        _load_error or None,
        'rule_guard':   rule_status,
        'feedback_log': _feedback_path,
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

        err = _validate_text(body.get('text'))
        if err:
            return _err(err, 400)

        result = _combine(body['text'])
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
            err = _validate_text(text)
            if err:
                results.append({'index': i, 'ok': False, 'error': err})
                continue
            try:
                r = _combine(text)
                results.append({'index': i, 'ok': True, **r})
            except Exception as item_err:
                print(f'ERROR classify_bulk item {i}: {item_err}', file=sys.stderr)
                results.append({'index': i, 'ok': False, 'error': str(item_err)})

        any_flagged = any(r.get('flagged') for r in results if r.get('ok'))
        return jsonify({'ok': True, 'results': results, 'any_flagged': any_flagged})

    except Exception as e:
        traceback.print_exc()
        return _err(f'Internal error: {e}', 500)


@app.post('/feedback')
def feedback():
    """
    Accept human corrections. Appended to feedback_log.jsonl.
    Run learn.py offline to extract patterns from this log.

    Body
    ----
    text              string   required  the original input text
    predicted_label   string   required  what the system returned
    correct_label     string   required  clean | spam | prompt_injection
    session_id        string   optional  caller-supplied identifier
    notes             string   optional  free-text comment

    Returns
    -------
    {ok, logged, is_correction}
    """
    try:
        body = request.get_json(silent=True)
        if not body:
            return _err('Request body must be JSON', 400)

        valid_labels = {'clean', 'spam', 'prompt_injection'}

        err = _validate_text(body.get('text'))
        if err:
            return _err(err, 400)

        predicted_label = body.get('predicted_label', '')
        correct_label   = body.get('correct_label', '')

        if not predicted_label:
            return _err('Missing required field: predicted_label', 400)
        if not correct_label:
            return _err('Missing required field: correct_label', 400)
        if correct_label not in valid_labels:
            return _err(f'correct_label must be one of: {", ".join(sorted(valid_labels))}', 400)
        if predicted_label not in valid_labels:
            return _err(f'predicted_label must be one of: {", ".join(sorted(valid_labels))}', 400)

        is_correction = predicted_label != correct_label

        entry = {
            'ts':              datetime.now(timezone.utc).isoformat(),
            'text':            body['text'],
            'predicted_label': predicted_label,
            'correct_label':   correct_label,
            'is_correction':   is_correction,
            'session_id':      body.get('session_id') or None,
            'notes':           body.get('notes') or None,
        }
        _write_feedback(entry)

        if is_correction:
            print(
                f'feedback: correction logged  '
                f'predicted={predicted_label}  correct={correct_label}  '
                f'text="{body["text"][:60]}"',
                flush=True,
            )

        return jsonify({'ok': True, 'logged': True, 'is_correction': is_correction})

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
    parser = argparse.ArgumentParser(
        description='Prompt injection guard API server (two-layer: ML + rules)',
    )
    parser.add_argument('--host',      default=os.environ.get('GUARD_HOST', '0.0.0.0'))
    parser.add_argument('--port',      default=int(os.environ.get('GUARD_PORT', '8765')), type=int)
    parser.add_argument('--model-dir', default=None)
    parser.add_argument('--db-path',   default=os.environ.get('GUARD_DB_PATH',  _DEFAULT_DB_PATH))
    parser.add_argument('--feedback',  default=os.environ.get('GUARD_FEEDBACK', _DEFAULT_FEEDBACK))
    parser.add_argument('--debug',     action='store_true')
    args = parser.parse_args()

    model_dir = _resolve_model_dir(args.model_dir)
    load_model(model_dir, args.db_path, args.feedback)

    print(f'Starting server on {args.host}:{args.port}', flush=True)
    print(f'Feedback log: {args.feedback}', flush=True)
    app.run(host=args.host, port=args.port, debug=args.debug)
