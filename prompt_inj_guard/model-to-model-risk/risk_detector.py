"""
model-to-model-risk/risk_detector.py

Model-to-model attack risk detector.

Attack scenario this defends against:
  A small fine-tuned manipulation model (client side) sends crafted prompts
  to a larger production model, iterating rapidly to find a bypass -- using
  jailbreak strings, encoded commands, persona overrides, token smuggling,
  or high-entropy noise to evade filters.

Detection layers (all pure Python, no ML required):
  1. Pattern layer    -- regex patterns for known attack families
  2. Entropy layer    -- Shannon entropy + symbol density flags
                        machine-generated adversarial inputs
  3. Session layer    -- sliding-window repetition tracker; escalates
                        to ALERT or SECURE_MODE on probe floods
  4. AI-detector hook -- optional P(AI) score from the DistilBERT model
                        in ai_detector_cli/ (lazy-loaded, gracefully skipped
                        if torch/transformers not available)

Result schema:
  {
    "flagged":          bool,
    "risk_level":       "none" | "low" | "medium" | "high" | "critical",
    "score":            float,          # 0.0 - 1.0
    "session_state":    str,            # "normal" | "alert" | "secure_mode"
    "attack_labels":    [str],          # matched attack categories
    "pattern_matches":  [{id, label, severity, description, matched_text}],
    "entropy_score":    float,          # Shannon entropy 0.0 - 8.0
    "symbol_ratio":     float,          # fraction of non-alphanumeric chars
    "high_entropy":     bool,
    "session_blocked":  bool,
    "session_detail":   {...},          # full session_guard result
    "ai_detector":      {...} | None,   # P(AI) result if model available
    "error":            str | None
  }

Hot-reloads patterns.json on mtime change.
All errors caught; public API never raises.
"""

import json
import math
import os
import re
import sys
import traceback
from pathlib import Path
from typing import Optional

_DIR = Path(__file__).parent

DEFAULT_PATTERNS_PATH   = _DIR / "patterns.json"
DEFAULT_AI_DETECTOR_DIR = _DIR / "ai_detector_cli" / "ai_detector_distilbert"

try:
    from .session_guard import SessionGuard, GuardState
except ImportError:
    # Fallback for direct execution or sys.path-based import
    sys.path.insert(0, str(_DIR))
    from session_guard import SessionGuard, GuardState  # type: ignore[no-redef]


# ------------------------------------------------------------------
# Entropy helpers

def _shannon_entropy(text: str) -> float:
    """Shannon entropy in bits."""
    if not text:
        return 0.0
    freq = {}
    for ch in text:
        freq[ch] = freq.get(ch, 0) + 1
    n = len(text)
    return -sum((c / n) * math.log2(c / n) for c in freq.values())


def _symbol_ratio(text: str) -> float:
    """Fraction of characters that are neither alphanumeric nor whitespace."""
    if not text:
        return 0.0
    non_alnum_ws = sum(1 for c in text if not c.isalnum() and not c.isspace())
    return non_alnum_ws / len(text)


def _repeat_ratio(text: str) -> float:
    """Fraction of 4-grams that are repeated (detects padded/looped input)."""
    if len(text) < 8:
        return 0.0
    grams = [text[i:i+4] for i in range(len(text) - 3)]
    if not grams:
        return 0.0
    unique = len(set(grams))
    return 1.0 - (unique / len(grams))


# ------------------------------------------------------------------
# Risk scoring

_RISK_BANDS = [
    (0.80, "critical"),
    (0.60, "high"),
    (0.40, "medium"),
    (0.15, "low"),
    (0.00, "none"),
]


def _score_to_risk(score: float) -> str:
    for threshold, label in _RISK_BANDS:
        if score >= threshold:
            return label
    return "none"


# ------------------------------------------------------------------
# Optional AI detector integration

def _try_ai_detect(text: str, model_dir: Path) -> Optional[dict]:
    """
    Attempt to call the DistilBERT AI detector.
    Returns its result dict or None if unavailable / error.
    Failure is non-fatal and logged to stderr.
    """
    try:
        import torch
        from transformers import DistilBertTokenizerFast, DistilBertForSequenceClassification

        if not model_dir.exists():
            return None

        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        tokenizer = DistilBertTokenizerFast.from_pretrained(str(model_dir))
        model     = DistilBertForSequenceClassification.from_pretrained(str(model_dir))
        model.to(device).eval()

        labels = model.config.id2label
        enc    = tokenizer(text, return_tensors="pt", truncation=True, max_length=128).to(device)

        with torch.no_grad():
            logits = model(**enc).logits
            probs  = torch.softmax(logits, dim=-1)[0].tolist()

        scored = {labels[i]: round(probs[i], 4) for i in range(len(probs))}
        top_label = max(scored, key=lambda k: scored[k])
        return {
            "label":      top_label,
            "confidence": scored[top_label],
            "scores":     scored,
            "flagged":    top_label == "AI",
        }
    except ImportError:
        return None
    except Exception:
        print(
            f"[risk_detector] ai_detector error (non-fatal):\n{traceback.format_exc()}",
            file=sys.stderr,
        )
        return None


# ------------------------------------------------------------------
# RiskDetector

class RiskDetector:
    """
    Model-to-model attack risk detector.

    One instance per application.  For per-session repetition tracking
    create one SessionGuard per user session and pass it via
    detect(text, session_guard=...).

    Parameters
    ----------
    patterns_path : path to patterns.json
    ai_detector_dir : path to ai_detector_distilbert model dir
                      (None = skip AI detection)
    use_ai_detector : bool, default False  -- set True to enable
    """

    def __init__(
        self,
        patterns_path: os.PathLike = DEFAULT_PATTERNS_PATH,
        ai_detector_dir: Optional[os.PathLike] = DEFAULT_AI_DETECTOR_DIR,
        use_ai_detector: bool = False,
    ) -> None:
        self._pat_path       = Path(patterns_path)
        self._ai_model_dir   = Path(ai_detector_dir) if ai_detector_dir else None
        self._use_ai         = use_ai_detector

        self._pat_mtime:  float = 0.0
        self._compiled:   list  = []
        self._ent_thresholds: dict = {}
        self._session_cfg:    dict = {}

        self._load_patterns()

    # ------------------------------------------------------------------
    # Hot-reload

    def _load_patterns(self) -> None:
        try:
            mtime = self._pat_path.stat().st_mtime
        except OSError as exc:
            print(f"[risk_detector] Cannot stat patterns file: {exc}", file=sys.stderr)
            return

        if mtime == self._pat_mtime:
            return

        try:
            with open(self._pat_path, encoding="utf-8") as fh:
                data = json.load(fh)

            self._compiled          = self._compile_all(data.get("attack_patterns", []))
            self._ent_thresholds    = data.get("high_entropy_thresholds", {})
            self._session_cfg       = data.get("session_thresholds", {})
            self._pat_mtime         = mtime

            print(
                f"[risk_detector] Loaded {len(self._compiled)} pattern groups "
                f"from {self._pat_path}",
                file=sys.stderr,
            )
        except Exception:
            print(
                f"[risk_detector] Failed to load patterns -- keeping previous:\n"
                + traceback.format_exc(),
                file=sys.stderr,
            )

    def _compile_all(self, groups: list) -> list:
        out = []
        for entry in groups:
            regexes = []
            for raw in entry.get("patterns", []):
                try:
                    regexes.append(re.compile(raw, re.IGNORECASE | re.DOTALL))
                except re.error as exc:
                    print(
                        f"[risk_detector] Compile error in '{entry.get('id', '?')}': "
                        f"{exc!r} -- pattern='{raw}'",
                        file=sys.stderr,
                    )
            if regexes:
                out.append({
                    "id":          entry.get("id", "unknown"),
                    "label":       entry.get("label", "unknown"),
                    "severity":    int(entry.get("severity", 5)),
                    "description": entry.get("description", ""),
                    "weight":      float(entry.get("weight", 1.0)),
                    "regexes":     regexes,
                })
        return out

    # ------------------------------------------------------------------
    # Public API

    def make_session_guard(self, session_id: Optional[str] = None) -> SessionGuard:
        """
        Create a pre-configured SessionGuard using thresholds from patterns.json.
        Pass the returned guard into detect() for per-session tracking.
        """
        cfg = self._session_cfg
        return SessionGuard(
            window_size                 = cfg.get("window_size", 20),
            window_seconds              = cfg.get("window_seconds", 300),
            similarity_block_threshold  = cfg.get("similarity_block_threshold", 0.72),
            hard_block_after_n_similar  = cfg.get("hard_block_after_n_similar", 3),
            secure_mode_trigger_count   = cfg.get("secure_mode_trigger_count", 5),
            secure_mode_cooldown_seconds= cfg.get("secure_mode_cooldown_seconds", 600),
            session_id                  = session_id,
        )

    def detect(
        self,
        text: str,
        session_guard: Optional[SessionGuard] = None,
    ) -> dict:
        """
        Run all detection layers on text.

        session_guard -- optional per-session guard for repetition tracking.
        Returns the result dict described in this module's docstring.
        Never raises.
        """
        try:
            return self._detect_inner(text, session_guard)
        except Exception:
            err = traceback.format_exc()
            print(f"[risk_detector] Unexpected error in detect():\n{err}", file=sys.stderr)
            return self._neutral_result(error=err)

    def detect_bulk(
        self,
        texts: list,
        session_guard: Optional[SessionGuard] = None,
    ) -> list:
        """Run detect() on each item. Never raises."""
        return [self.detect(t, session_guard) for t in texts]

    # ------------------------------------------------------------------
    # Core logic

    def _detect_inner(self, text: str, session_guard: Optional[SessionGuard]) -> dict:
        self._load_patterns()

        if not isinstance(text, str):
            text = str(text)

        # --- Layer 1: session gate (check before spending CPU on rest) ---
        session_result  = None
        session_blocked = False

        if session_guard is not None:
            session_result  = session_guard.check_and_record(text)
            session_blocked = not session_result["allowed"]

        # --- Layer 2: pattern matching ---
        pattern_matches: list[dict] = []
        labels: set[str]             = set()
        max_severity = 0
        total_weight = 0.0

        for group in self._compiled:
            for regex in group["regexes"]:
                m = regex.search(text)
                if m is None:
                    continue
                pattern_matches.append({
                    "id":           group["id"],
                    "label":        group["label"],
                    "severity":     group["severity"],
                    "description":  group["description"],
                    "matched_text": m.group(0)[:120],
                })
                labels.add(group["label"])
                if group["severity"] > max_severity:
                    max_severity = group["severity"]
                total_weight += group["weight"]
                break   # one hit per group is sufficient

        # --- Layer 3: entropy analysis ---
        ent     = round(_shannon_entropy(text), 4)
        sym_r   = round(_symbol_ratio(text), 4)
        rep_r   = round(_repeat_ratio(text), 4)

        ent_min      = self._ent_thresholds.get("shannon_entropy_min", 3.5)
        sym_min      = self._ent_thresholds.get("symbol_ratio_min", 0.30)
        sym_high     = self._ent_thresholds.get("symbol_ratio_high", 0.70)
        rep_max      = self._ent_thresholds.get("max_safe_repeat_ratio", 0.40)
        short_limit  = self._ent_thresholds.get("short_text_char_limit", 40)

        high_entropy = (
            len(text) > short_limit
            and (
                (ent >= ent_min and sym_r >= sym_min)
                or sym_r >= sym_high   # very high symbol density is flagged regardless of entropy
            )
        )
        high_repeat = rep_r > rep_max and len(text) > short_limit

        # --- Layer 4: optional AI content detector ---
        ai_result = None
        if self._use_ai and self._ai_model_dir:
            ai_result = _try_ai_detect(text, self._ai_model_dir)

        # --- Scoring ---
        pattern_score = 0.0
        if pattern_matches:
            sev_norm    = max_severity / 10.0
            weight_norm = min(total_weight, 3.0) / 3.0
            pattern_score = 0.6 * sev_norm + 0.4 * weight_norm

        entropy_score = 0.0
        if high_entropy:
            entropy_score = min((ent - ent_min) / (8.0 - ent_min), 1.0) * 0.5
        if high_repeat:
            entropy_score = max(entropy_score, 0.50)

        ai_score = 0.0
        if ai_result and ai_result.get("flagged"):
            ai_score = ai_result.get("confidence", 0.5) * 0.30  # weight: 30%

        session_score = 0.0
        if session_blocked:
            s_state = (session_result or {}).get("state", "normal")
            if s_state == "secure_mode":
                session_score = 1.0
            elif s_state == "alert":
                session_score = 0.70

        # Combine: take the maximum of pattern/entropy/session, then boost by AI
        base_score = max(pattern_score, entropy_score, session_score)
        final_score = round(min(base_score + ai_score, 1.0), 4)

        flagged    = (
            bool(pattern_matches)
            or high_entropy
            or high_repeat
            or session_blocked
            or bool(ai_result and ai_result.get("flagged"))
        )
        risk_level = _score_to_risk(final_score) if flagged else "none"

        return {
            "flagged":          flagged,
            "risk_level":       risk_level,
            "score":            final_score,
            "session_state":    (session_result or {}).get("state", "normal"),
            "attack_labels":    sorted(labels),
            "pattern_matches":  pattern_matches,
            "entropy_score":    ent,
            "symbol_ratio":     sym_r,
            "repeat_ratio":     rep_r,
            "high_entropy":     high_entropy,
            "high_repeat":      high_repeat,
            "session_blocked":  session_blocked,
            "session_detail":   session_result,
            "ai_detector":      ai_result,
            "error":            None,
        }

    # ------------------------------------------------------------------
    # Introspection

    def loaded_pattern_count(self) -> int:
        return len(self._compiled)

    def loaded_labels(self) -> list[str]:
        return [g["label"] for g in self._compiled]

    @staticmethod
    def _neutral_result(error: Optional[str] = None) -> dict:
        return {
            "flagged":          False,
            "risk_level":       "none",
            "score":            0.0,
            "session_state":    "normal",
            "attack_labels":    [],
            "pattern_matches":  [],
            "entropy_score":    0.0,
            "symbol_ratio":     0.0,
            "repeat_ratio":     0.0,
            "high_entropy":     False,
            "high_repeat":      False,
            "session_blocked":  False,
            "session_detail":   None,
            "ai_detector":      None,
            "error":            error,
        }
