#!/usr/bin/env python3
"""Native MCP stdio server for prompt injection and spam guard.

This server exposes tools for MCP clients:
  - classify_text
  - classify_bulk
  - guard_health

It can run in two modes:
  1) ML + rules (if model weights are available)
  2) Rules-only fallback (if model is unavailable)
"""

from __future__ import annotations

import json
import os
import sys
import traceback
from pathlib import Path
from typing import Any


def _runtime_base() -> Path:
    # PyInstaller one-file extracts to sys._MEIPASS at runtime.
    if hasattr(sys, "_MEIPASS"):
        return Path(getattr(sys, "_MEIPASS"))
    return Path(__file__).resolve().parent


HERE = Path(__file__).resolve().parent
RUNTIME_BASE = _runtime_base()

def _default_model_dir() -> str:
    # In one-file binaries, prefer a path next to the executable so users can
    # ship model files externally without relying on temp extraction paths.
    if getattr(sys, "frozen", False):
        exe_dir = Path(sys.executable).resolve().parent
        return str((exe_dir / "model" / "spam_injection_model" / "final").resolve())
    return str((HERE / "model" / "spam_injection_model" / "final").resolve())


MODEL_DIR_DEFAULT = _default_model_dir()
DB_PATH_DEFAULT = str((RUNTIME_BASE / "api" / "pattern_db.json").resolve())

if str(HERE / "model") not in sys.path:
    sys.path.insert(0, str(HERE / "model"))

try:
    from guard_classifier import GuardClassifier
except Exception:  # pragma: no cover
    GuardClassifier = None

try:
    from rule_guard import RuleGuard
except Exception:  # pragma: no cover
    RuleGuard = None


RULES_OVERRIDE_THRESHOLD = 0.80
MODEL_FLAG_THRESHOLD = 0.70
MODEL_LOW_CONF_THRESHOLD = 0.55
BULK_LIMIT = 64


classifier = None
rule_guard = None
classifier_error = ""
rule_error = ""
active_model_dir = ""
active_db_path = ""


def _load_engines() -> None:
    global classifier, rule_guard, classifier_error, rule_error
    global active_model_dir, active_db_path

    model_dir = os.environ.get("GUARD_MODEL_DIR", MODEL_DIR_DEFAULT)
    db_path = os.environ.get("GUARD_DB_PATH", DB_PATH_DEFAULT)

    active_model_dir = str(Path(model_dir).resolve())
    active_db_path = str(Path(db_path).resolve())

    classifier = None
    rule_guard = None
    classifier_error = ""
    rule_error = ""

    if GuardClassifier is None:
        classifier_error = "GuardClassifier import failed"
    else:
        try:
            if Path(active_model_dir).is_dir():
                classifier = GuardClassifier(active_model_dir)
            else:
                classifier_error = f"model dir not found: {active_model_dir}"
        except Exception as exc:
            classifier_error = str(exc)

    if RuleGuard is None:
        rule_error = "RuleGuard import failed"
    else:
        try:
            rule_guard = RuleGuard(active_db_path)
        except Exception as exc:
            rule_error = str(exc)


def _ml_classify(text: str) -> dict[str, Any]:
    if classifier is None:
        return {"label": "clean", "confidence": 0.5, "flagged": False, "scores": {}}
    try:
        return classifier.classify(text)
    except Exception:
        return {"label": "clean", "confidence": 0.5, "flagged": False, "scores": {}}


def _rules_check(text: str) -> dict[str, Any]:
    if rule_guard is None:
        return {
            "label": "clean",
            "confidence": 0.0,
            "flagged": False,
            "matched": [],
            "is_allowlisted": False,
            "allowlist_matches": [],
            "allowlist_conf": 0.0,
        }
    try:
        return rule_guard.check(text)
    except Exception:
        return {
            "label": "clean",
            "confidence": 0.0,
            "flagged": False,
            "matched": [],
            "is_allowlisted": False,
            "allowlist_matches": [],
            "allowlist_conf": 0.0,
        }


def classify_text(text: str) -> dict[str, Any]:
    if not isinstance(text, str) or not text.strip():
        raise ValueError("text must be a non-empty string")

    ml = _ml_classify(text)
    rule = _rules_check(text)

    ml_label = ml["label"]
    ml_conf = float(ml["confidence"])
    rule_label = rule["label"]
    rule_conf = float(rule["confidence"])
    allowlisted = bool(rule["is_allowlisted"])
    allow_conf = float(rule["allowlist_conf"])

    final_label = "clean"
    final_conf = ml_conf if ml_label == "clean" else 0.5
    source = "model"

    if rule["flagged"] and rule_conf >= RULES_OVERRIDE_THRESHOLD and allowlisted:
        final_label = "clean"
        final_conf = max(allow_conf, 0.75)
        source = "allowlist_override"
    elif rule["flagged"] and rule_conf >= RULES_OVERRIDE_THRESHOLD:
        final_label = rule_label
        if ml_label == rule_label:
            final_conf = min(0.99, (rule_conf + ml_conf) / 2.0 + 0.08)
            source = "rules+model_agree"
        else:
            final_conf = rule_conf
            source = "rules_override"
    elif ml_label != "clean" and ml_conf >= MODEL_FLAG_THRESHOLD:
        if allowlisted:
            final_label = "clean"
            final_conf = max(allow_conf, 0.78)
            source = "allowlist_override"
        elif rule_label == ml_label:
            final_label = ml_label
            final_conf = min(0.99, ml_conf + 0.07)
            source = "model+rules_agree"
        else:
            final_label = ml_label
            final_conf = ml_conf
            source = "model"
    elif ml_label != "clean" and ml_conf >= MODEL_LOW_CONF_THRESHOLD:
        if allowlisted:
            final_label = "clean"
            final_conf = max(allow_conf, 0.72)
            source = "allowlist_override"
        else:
            final_label = ml_label
            final_conf = ml_conf
            source = "model_low_conf"

    result = {
        "label": final_label,
        "confidence": round(final_conf, 4),
        "flagged": final_label != "clean",
        "scores": ml.get("scores", {}),
        "source": source,
        "ml_label": ml_label,
        "ml_confidence": round(ml_conf, 4),
        "rule_label": rule_label,
        "rule_confidence": round(rule_conf, 4),
        "rule_matches": rule.get("matched", []),
        "allowlist_matches": rule.get("allowlist_matches", []),
        "model_loaded": classifier is not None,
        "rule_guard_loaded": rule_guard is not None,
    }
    return result


def _health() -> dict[str, Any]:
    return {
        "ok": classifier is not None or rule_guard is not None,
        "model_loaded": classifier is not None,
        "rule_guard_loaded": rule_guard is not None,
        "model_dir": active_model_dir,
        "db_path": active_db_path,
        "model_error": classifier_error or None,
        "rule_error": rule_error or None,
    }


def _write_msg(payload: dict[str, Any]) -> None:
    raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    header = f"Content-Length: {len(raw)}\r\n\r\n".encode("ascii")
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(raw)
    sys.stdout.buffer.flush()


def _read_msg() -> dict[str, Any] | None:
    headers: dict[str, str] = {}
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        text = line.decode("ascii", errors="replace").strip()
        if ":" in text:
            key, value = text.split(":", 1)
            headers[key.strip().lower()] = value.strip()

    length_text = headers.get("content-length")
    if not length_text:
        return None
    length = int(length_text)
    body = sys.stdin.buffer.read(length)
    if not body:
        return None
    return json.loads(body.decode("utf-8"))


def _rpc_ok(request_id: Any, result: dict[str, Any]) -> None:
    _write_msg({"jsonrpc": "2.0", "id": request_id, "result": result})


def _rpc_err(request_id: Any, code: int, message: str, data: Any = None) -> None:
    payload = {
        "jsonrpc": "2.0",
        "id": request_id,
        "error": {"code": code, "message": message},
    }
    if data is not None:
        payload["error"]["data"] = data
    _write_msg(payload)


def _tool_result(data: dict[str, Any], is_error: bool = False) -> dict[str, Any]:
    text = json.dumps(data, ensure_ascii=False)
    return {
        "content": [{"type": "text", "text": text}],
        "structuredContent": data,
        "isError": is_error,
    }


def _handle_request(msg: dict[str, Any]) -> None:
    method = msg.get("method")
    req_id = msg.get("id")
    params = msg.get("params") or {}

    # Notifications do not require responses.
    if req_id is None:
        return

    try:
        if method == "initialize":
            _rpc_ok(
                req_id,
                {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"tools": {}},
                    "serverInfo": {"name": "prompt-inj-guard-mcp", "version": "1.0.0"},
                },
            )
            return

        if method == "tools/list":
            _rpc_ok(
                req_id,
                {
                    "tools": [
                        {
                            "name": "classify_text",
                            "description": "Classify one input as clean, spam, or prompt_injection.",
                            "inputSchema": {
                                "type": "object",
                                "properties": {"text": {"type": "string"}},
                                "required": ["text"],
                                "additionalProperties": False,
                            },
                        },
                        {
                            "name": "classify_bulk",
                            "description": "Classify up to 64 texts in one call.",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "texts": {
                                        "type": "array",
                                        "items": {"type": "string"},
                                        "maxItems": BULK_LIMIT,
                                    }
                                },
                                "required": ["texts"],
                                "additionalProperties": False,
                            },
                        },
                        {
                            "name": "guard_health",
                            "description": "Return model and rule-engine load status.",
                            "inputSchema": {
                                "type": "object",
                                "properties": {},
                                "additionalProperties": False,
                            },
                        },
                    ]
                },
            )
            return

        if method == "tools/call":
            name = params.get("name")
            arguments = params.get("arguments") or {}

            if name == "classify_text":
                result = classify_text(arguments.get("text"))
                _rpc_ok(req_id, _tool_result(result))
                return

            if name == "classify_bulk":
                texts = arguments.get("texts")
                if not isinstance(texts, list):
                    raise ValueError("texts must be an array of strings")
                if len(texts) > BULK_LIMIT:
                    raise ValueError(f"texts exceeds bulk limit of {BULK_LIMIT}")

                items: list[dict[str, Any]] = []
                for idx, item in enumerate(texts):
                    try:
                        items.append({"index": idx, "ok": True, **classify_text(item)})
                    except Exception as exc:
                        items.append({"index": idx, "ok": False, "error": str(exc)})

                payload = {
                    "ok": True,
                    "count": len(items),
                    "any_flagged": any(i.get("flagged") for i in items if i.get("ok")),
                    "results": items,
                }
                _rpc_ok(req_id, _tool_result(payload))
                return

            if name == "guard_health":
                _rpc_ok(req_id, _tool_result(_health()))
                return

            _rpc_err(req_id, -32602, f"Unknown tool: {name}")
            return

        _rpc_err(req_id, -32601, f"Method not found: {method}")

    except Exception as exc:
        _rpc_err(
            req_id,
            -32000,
            str(exc),
            {"trace": traceback.format_exc().splitlines()[-8:]},
        )


def main() -> int:
    _load_engines()
    while True:
        msg = _read_msg()
        if msg is None:
            return 0
        _handle_request(msg)


if __name__ == "__main__":
    raise SystemExit(main())
