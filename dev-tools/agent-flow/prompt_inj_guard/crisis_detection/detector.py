"""
crisis_detection/detector.py

Core crisis detection engine.

Uses regex pattern matching with category-based severity scoring.
Does NOT require any ML models or GPU. Pure CPU, no external dependencies
beyond the Python standard library.

Hot-reloads patterns.json and safe_resources.json when their mtime changes
so pattern updates take effect without restarting the process.

Result schema:
  {
    "flagged": bool,
    "category": str | None,
    "category_display": str | None,
    "severity": int,           # 0 (no match) ... 10 (most severe)
    "confidence": float,       # 0.0 ... 1.0
    "matched_patterns": [
      {"id": str, "category": str, "severity": int, "description": str,
       "matched_text": str, "negated": bool}
    ],
    "context_boost_detected": bool,
    "safe_resources": { ... } | None,   # from safe_resources.json
    "error": str | None
  }
"""

import json
import os
import re
import sys
import traceback
from pathlib import Path
from typing import Any

_DIR = Path(__file__).parent

DEFAULT_PATTERNS_PATH   = _DIR / "patterns.json"
DEFAULT_RESOURCES_PATH  = _DIR / "safe_resources.json"


class CrisisDetector:
    """Regex-based crisis signal detector with hot-reload support."""

    def __init__(
        self,
        patterns_path: str | os.PathLike = DEFAULT_PATTERNS_PATH,
        resources_path: str | os.PathLike = DEFAULT_RESOURCES_PATH,
    ) -> None:
        self._pat_path = Path(patterns_path)
        self._res_path = Path(resources_path)

        self._pat_mtime: float = 0.0
        self._res_mtime: float = 0.0

        self._patterns_raw:    list[dict]  = []
        self._resources_raw:   dict        = {}
        self._compiled:        list[Any]   = []   # [{meta, regexes: [re.Pattern]}]
        self._neg_phrases:     list[str]   = []
        self._ctx_boosters:    list[str]   = []
        self._categories:      dict        = {}

        self._load_patterns()
        self._load_resources()

    # ------------------------------------------------------------------
    # Internal loading

    def _load_patterns(self) -> None:
        """Load or reload patterns.json if the file has changed."""
        try:
            mtime = self._pat_path.stat().st_mtime
        except OSError as exc:
            print(f"[crisis_detector] Cannot stat patterns file: {exc}", file=sys.stderr)
            return

        if mtime == self._pat_mtime:
            return

        try:
            with open(self._pat_path, encoding="utf-8") as fh:
                data = json.load(fh)

            self._categories     = data.get("categories", {})
            self._patterns_raw   = data.get("patterns", [])
            self._neg_phrases    = [p.lower() for p in data.get("negation_phrases", [])]
            self._ctx_boosters   = [p.lower() for p in data.get("context_boosters", [])]
            self._compiled       = self._compile_all(self._patterns_raw)
            self._pat_mtime      = mtime

            print(
                f"[crisis_detector] Loaded {len(self._compiled)} pattern groups "
                f"from {self._pat_path}",
                file=sys.stderr,
            )
        except Exception:
            print(
                f"[crisis_detector] Failed to load patterns -- keeping previous set:\n"
                + traceback.format_exc(),
                file=sys.stderr,
            )

    def _load_resources(self) -> None:
        """Load or reload safe_resources.json if the file has changed."""
        try:
            mtime = self._res_path.stat().st_mtime
        except OSError as exc:
            print(f"[crisis_detector] Cannot stat resources file: {exc}", file=sys.stderr)
            return

        if mtime == self._res_mtime:
            return

        try:
            with open(self._res_path, encoding="utf-8") as fh:
                self._resources_raw = json.load(fh)
            self._res_mtime = mtime
            print(
                f"[crisis_detector] Loaded safe resources from {self._res_path}",
                file=sys.stderr,
            )
        except Exception:
            print(
                f"[crisis_detector] Failed to load resources -- keeping previous set:\n"
                + traceback.format_exc(),
                file=sys.stderr,
            )

    def _compile_all(self, pattern_groups: list[dict]) -> list[dict]:
        """Compile all regex patterns. Skips (and logs) any that fail to compile."""
        compiled = []
        for entry in pattern_groups:
            regexes = []
            for raw in entry.get("patterns", []):
                try:
                    regexes.append(re.compile(raw, re.IGNORECASE | re.UNICODE))
                except re.error as exc:
                    print(
                        f"[crisis_detector] Pattern compile error in group "
                        f"'{entry.get('id', '?')}': {exc!r} -- pattern='{raw}'",
                        file=sys.stderr,
                    )
            if regexes:
                compiled.append({
                    "id":          entry.get("id", "unknown"),
                    "category":    entry.get("category", "unknown"),
                    "severity":    int(entry.get("severity", 5)),
                    "description": entry.get("description", ""),
                    "weight":      float(entry.get("weight", 1.0)),
                    "regexes":     regexes,
                })
        return compiled

    # ------------------------------------------------------------------
    # Internal helpers

    def _reload_if_stale(self) -> None:
        self._load_patterns()
        self._load_resources()

    def _is_negated(self, text: str, match_start: int) -> bool:
        """
        Return True when the text around the match contains a negation phrase
        (within a 80-character look-behind window).
        """
        window_start = max(0, match_start - 80)
        window = text[window_start:match_start].lower()
        for phrase in self._neg_phrases:
            if phrase in window:
                return True
        return False

    def _has_context_booster(self, text: str) -> bool:
        low = text.lower()
        for booster in self._ctx_boosters:
            if booster in low:
                return True
        return False

    def _score_to_confidence(self, total_weight: float, max_severity: int) -> float:
        """
        Convert accumulated match weight + top severity into a 0..1 confidence.
        This is a heuristic, not a calibrated probability.
        """
        sev_factor  = max_severity / 10.0
        weight_norm = min(total_weight, 3.0) / 3.0    # cap at 3 signals
        return round(min(0.5 * sev_factor + 0.5 * weight_norm, 1.0), 4)

    def _get_resources(self, resource_key: str | None) -> dict | None:
        if not resource_key:
            return None
        return self._resources_raw.get(resource_key) or None

    # ------------------------------------------------------------------
    # Public API

    def detect(self, text: str) -> dict:
        """
        Run crisis detection on a single text string.

        Returns the result dict described in this module's docstring.
        On unexpected error returns a safe neutral result with error set.
        """
        try:
            return self._detect_inner(text)
        except Exception:
            err = traceback.format_exc()
            print(f"[crisis_detector] Unexpected error in detect():\n{err}", file=sys.stderr)
            return self._neutral_result(error=err)

    def detect_bulk(self, texts: list[str]) -> list[dict]:
        """Run detect() on every item in texts. Never raises."""
        results = []
        for t in texts:
            results.append(self.detect(t))
        return results

    # ------------------------------------------------------------------
    # Core logic

    def _detect_inner(self, text: str) -> dict:
        self._reload_if_stale()

        if not isinstance(text, str):
            text = str(text)

        matched_patterns: list[dict] = []
        category_hits:    dict[str, dict] = {}  # category -> best hit data

        has_booster = self._has_context_booster(text)

        for group in self._compiled:
            for regex in group["regexes"]:
                m = regex.search(text)
                if m is None:
                    continue

                negated    = self._is_negated(text, m.start())
                severity   = group["severity"]
                weight     = group["weight"]
                category   = group["category"]

                entry = {
                    "id":           group["id"],
                    "category":     category,
                    "severity":     severity,
                    "description":  group["description"],
                    "matched_text": m.group(0),
                    "negated":      negated,
                }
                matched_patterns.append(entry)

                if not negated:
                    prev = category_hits.get(category)
                    if prev is None or severity > prev["max_severity"]:
                        category_hits[category] = {
                            "max_severity": severity,
                            "total_weight": weight,
                            "category":     category,
                        }
                    else:
                        category_hits[category]["total_weight"] += weight

                break  # one match per group is enough

        # Determine dominant category (highest severity, then most weight)
        top_category_data: dict | None = None
        for cat_data in category_hits.values():
            if top_category_data is None:
                top_category_data = cat_data
                continue
            if cat_data["max_severity"] > top_category_data["max_severity"]:
                top_category_data = cat_data
            elif (
                cat_data["max_severity"] == top_category_data["max_severity"]
                and cat_data["total_weight"] > top_category_data["total_weight"]
            ):
                top_category_data = cat_data

        if top_category_data is None:
            return self._neutral_result()

        top_cat      = top_category_data["category"]
        max_sev      = top_category_data["max_severity"]
        total_weight = top_category_data["total_weight"]

        # Apply booster
        effective_severity = max_sev
        if has_booster and max_sev < 10:
            effective_severity = min(max_sev + 1, 10)

        confidence = self._score_to_confidence(total_weight, max_sev)
        if has_booster:
            confidence = min(confidence + 0.10, 1.0)

        cat_meta     = self._categories.get(top_cat, {})
        resource_key = cat_meta.get("resource_key")
        resources    = self._get_resources(resource_key)

        return {
            "flagged":               True,
            "category":              top_cat,
            "category_display":      cat_meta.get("display", top_cat),
            "severity":              effective_severity,
            "confidence":            round(confidence, 4),
            "matched_patterns":      matched_patterns,
            "context_boost_detected": has_booster,
            "safe_resources":        resources,
            "error":                 None,
        }

    @staticmethod
    def _neutral_result(error: str | None = None) -> dict:
        return {
            "flagged":                False,
            "category":               None,
            "category_display":       None,
            "severity":               0,
            "confidence":             0.0,
            "matched_patterns":       [],
            "context_boost_detected": False,
            "safe_resources":         None,
            "error":                  error,
        }

    # ------------------------------------------------------------------
    # Introspection helpers (useful for testing and admin endpoints)

    def loaded_pattern_count(self) -> int:
        return len(self._compiled)

    def loaded_categories(self) -> list[str]:
        return list(self._categories.keys())

    def pattern_db_path(self) -> str:
        return str(self._pat_path)

    def resources_db_path(self) -> str:
        return str(self._res_path)
