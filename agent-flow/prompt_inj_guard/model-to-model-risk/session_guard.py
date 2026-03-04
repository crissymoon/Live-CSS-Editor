"""
model-to-model-risk/session_guard.py

Session-level repetition tracker and secure-mode state machine.

Tracks a sliding window of prompt fingerprints for a single session.
When similar prompts are submitted repeatedly (the pattern of a small
fine-tuned model probing a larger model), the guard escalates:

  NORMAL       -> no repeated similarity events
  ALERT        -> repetition threshold crossed; warn and log
  SECURE_MODE  -> hard block threshold crossed; all prompts rejected
                  until cooldown expires or manual reset

Thread-safe.  No external dependencies beyond the standard library.

All errors are caught and logged to stderr; the public API never raises.
"""

import hashlib
import math
import sys
import threading
import time
import traceback
from collections import deque
from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


# ------------------------------------------------------------------
# State

class GuardState(str, Enum):
    NORMAL      = "normal"
    ALERT       = "alert"
    SECURE_MODE = "secure_mode"


@dataclass
class PromptRecord:
    fingerprint: str
    text_preview: str    # first 80 chars, for debug logging only
    timestamp:   float
    similarity_score: float = 0.0   # similarity to previous prompt at insert time


@dataclass
class SessionSummary:
    state:                   GuardState
    total_prompts:           int
    similar_event_count:     int
    secure_mode_entry_count: int
    cooldown_remaining_s:    float
    window_size:             int
    last_fingerprint:        Optional[str]


# ------------------------------------------------------------------
# Fingerprinting helpers

def _normalize(text: str) -> str:
    """Lowercase, collapse whitespace, strip punctuation runs."""
    import re
    text = text.lower().strip()
    text = re.sub(r'\s+', ' ', text)
    text = re.sub(r'[^\w\s]', '', text)
    return text


def _fingerprint(text: str) -> str:
    """Stable 16-char hex fingerprint based on sorted trigrams."""
    norm = _normalize(text)
    tokens = norm.split()
    if not tokens:
        return "0" * 16
    trigrams = set()
    for i in range(len(tokens) - 2):
        trigrams.add(tokens[i] + "_" + tokens[i+1] + "_" + tokens[i+2])
    # also include unigrams for short texts
    if len(tokens) < 6:
        trigrams.update(tokens)
    key = "|".join(sorted(trigrams))
    return hashlib.sha256(key.encode()).hexdigest()[:16]


def _jaccard_similarity(a: str, b: str) -> float:
    """
    Token-set Jaccard similarity between two normalized texts.
    Returns float in [0.0, 1.0].
    """
    try:
        sa = set(_normalize(a).split())
        sb = set(_normalize(b).split())
        if not sa and not sb:
            return 1.0
        if not sa or not sb:
            return 0.0
        return len(sa & sb) / len(sa | sb)
    except Exception:
        return 0.0


def _fingerprint_similarity(fp_a: str, fp_b: str) -> float:
    """Simple char overlap for quick fingerprint comparison."""
    if fp_a == fp_b:
        return 1.0
    matches = sum(c == d for c, d in zip(fp_a, fp_b))
    return matches / max(len(fp_a), len(fp_b), 1)


# ------------------------------------------------------------------
# SessionGuard

class SessionGuard:
    """
    Tracks a session window of prompts and escalates state
    when repetition patterns are detected.

    Parameters
    ----------
    window_size : int
        Maximum number of recent prompts tracked.
    window_seconds : int
        Prompts older than this are dropped from the window.
    similarity_block_threshold : float
        Jaccard similarity above which two prompts count as a
        'similar event'.
    hard_block_after_n_similar : int
        Number of consecutive similar events that trigger ALERT.
    secure_mode_trigger_count : int
        Total similar events in window that trigger SECURE_MODE.
    secure_mode_cooldown_seconds : int
        Seconds before secure mode auto-expires.
    session_id : str | None
        Optional identifier for log lines.
    """

    def __init__(
        self,
        window_size: int = 20,
        window_seconds: int = 300,
        similarity_block_threshold: float = 0.72,
        hard_block_after_n_similar: int = 3,
        secure_mode_trigger_count: int = 5,
        secure_mode_cooldown_seconds: int = 600,
        session_id: Optional[str] = None,
    ) -> None:
        self._window_size              = window_size
        self._window_seconds           = window_seconds
        self._sim_threshold            = similarity_block_threshold
        self._hard_block_n             = hard_block_after_n_similar
        self._secure_trigger           = secure_mode_trigger_count
        self._secure_cooldown          = secure_mode_cooldown_seconds
        self._session_id               = session_id or "default"

        self._lock                     = threading.Lock()
        self._records: deque[PromptRecord] = deque(maxlen=window_size)
        self._state                    = GuardState.NORMAL
        self._similar_event_count       = 0
        self._secure_mode_entry_count   = 0
        self._secure_mode_entered_at    = 0.0
        self._total_prompts             = 0

    # ------------------------------------------------------------------
    # Public API

    def check_and_record(self, text: str) -> dict:
        """
        Evaluate text against session history.

        Returns:
          {
            "allowed": bool,
            "state": str,                  # "normal" | "alert" | "secure_mode"
            "blocked_reason": str | None,
            "similarity_to_last": float,
            "similar_event_count": int,
            "secure_mode_cooldown_remaining": float,  # seconds; 0 if not in secure mode
          }
        """
        try:
            return self._check_inner(text)
        except Exception:
            err = traceback.format_exc()
            print(f"[session_guard:{self._session_id}] Unexpected error:\n{err}", file=sys.stderr)
            # fail open -- let risk_detector make the call
            return self._safe_result(allowed=True, reason=None, sim=0.0)

    def reset(self) -> None:
        """Manually reset the guard back to NORMAL and clear history."""
        try:
            with self._lock:
                self._records.clear()
                self._state                  = GuardState.NORMAL
                self._similar_event_count     = 0
                self._secure_mode_entered_at  = 0.0
                self._total_prompts           = 0
            print(
                f"[session_guard:{self._session_id}] Manual reset to NORMAL",
                file=sys.stderr,
            )
        except Exception:
            print(
                f"[session_guard:{self._session_id}] reset() error:\n{traceback.format_exc()}",
                file=sys.stderr,
            )

    def summary(self) -> SessionSummary:
        """Return a snapshot of the current session state."""
        try:
            with self._lock:
                cooldown = 0.0
                if self._state == GuardState.SECURE_MODE:
                    elapsed  = time.time() - self._secure_mode_entered_at
                    cooldown = max(0.0, self._secure_cooldown - elapsed)
                return SessionSummary(
                    state                   = self._state,
                    total_prompts           = self._total_prompts,
                    similar_event_count     = self._similar_event_count,
                    secure_mode_entry_count = self._secure_mode_entry_count,
                    cooldown_remaining_s    = round(cooldown, 1),
                    window_size             = len(self._records),
                    last_fingerprint        = self._records[-1].fingerprint if self._records else None,
                )
        except Exception:
            print(
                f"[session_guard:{self._session_id}] summary() error:\n{traceback.format_exc()}",
                file=sys.stderr,
            )
            return SessionSummary(
                state=GuardState.NORMAL, total_prompts=0, similar_event_count=0,
                secure_mode_entry_count=0, cooldown_remaining_s=0.0,
                window_size=0, last_fingerprint=None,
            )

    # ------------------------------------------------------------------
    # Internal logic

    def _handle_secure_mode_state(self, now: float) -> Optional[dict]:
        """If in SECURE_MODE, check for expiry. Returns a block result dict, or None if
        the guard has expired back to NORMAL or was never in secure mode."""
        if self._state != GuardState.SECURE_MODE:
            return None
        elapsed = now - self._secure_mode_entered_at
        if elapsed >= self._secure_cooldown:
            self._state = GuardState.NORMAL
            self._similar_event_count = 0
            print(
                f"[session_guard:{self._session_id}] Secure mode expired after "
                f"{elapsed:.0f}s -- returning to NORMAL",
                file=sys.stderr,
            )
            return None
        remaining = self._secure_cooldown - elapsed
        return self._safe_result(
            allowed=False,
            reason=f"session in secure_mode -- cooldown {remaining:.0f}s remaining",
            sim=0.0,
            state=GuardState.SECURE_MODE,
            cooldown=remaining,
        )

    def _record_prompt(self, text: str, fp: str, sim: float, is_sim: bool, now: float) -> None:
        """Append a prompt record and update event counters."""
        self._total_prompts += 1
        if is_sim:
            self._similar_event_count += 1
            print(
                f"[session_guard:{self._session_id}] Similar event #{self._similar_event_count} "
                f"sim={sim:.2f} text='{text[:60]}'",
                file=sys.stderr,
            )
        self._records.append(PromptRecord(
            fingerprint      = fp,
            text_preview     = text[:80],
            timestamp        = now,
            similarity_score = sim,
        ))

    def _evaluate_state_transition(self, sim: float, now: float) -> Optional[dict]:
        """Apply threshold rules and update state. Returns a block result or None if allowed."""
        if self._similar_event_count >= self._secure_trigger:
            if self._state != GuardState.SECURE_MODE:
                self._state                  = GuardState.SECURE_MODE
                self._secure_mode_entered_at = now
                self._secure_mode_entry_count += 1
                print(
                    f"[session_guard:{self._session_id}] SECURE MODE ACTIVATED "
                    f"after {self._similar_event_count} similar events",
                    file=sys.stderr,
                )
            remaining = float(self._secure_cooldown)
            return self._safe_result(
                allowed=False,
                reason=f"secure_mode triggered by {self._similar_event_count} similar events",
                sim=sim,
                state=GuardState.SECURE_MODE,
                cooldown=remaining,
            )
        if self._similar_event_count >= self._hard_block_n:
            self._state = GuardState.ALERT
            return self._safe_result(
                allowed=False,
                reason=f"alert: {self._similar_event_count} similar probing attempts detected",
                sim=sim,
                state=GuardState.ALERT,
            )
        return None

    def _check_inner(self, text: str) -> dict:
        now = time.time()
        with self._lock:
            block = self._handle_secure_mode_state(now)
            if block is not None:
                return block

            self._prune_stale(now)

            fp     = _fingerprint(text)
            sim    = self._max_similarity_in_window(text, fp)
            is_sim = sim >= self._sim_threshold

            self._record_prompt(text, fp, sim, is_sim, now)

            block = self._evaluate_state_transition(sim, now)
            if block is not None:
                return block

            return self._safe_result(allowed=True, reason=None, sim=sim, state=self._state)

    def _prune_stale(self, now: float) -> None:
        cutoff = now - self._window_seconds
        while self._records and self._records[0].timestamp < cutoff:
            self._records.popleft()

    def _max_similarity_in_window(self, text: str, fp: str) -> float:
        if not self._records:
            return 0.0
        max_sim = 0.0
        # check the most recent N records (cap at 10 for speed)
        recent = list(self._records)[-10:]
        for rec in recent:
            # fast fingerprint check first
            fp_sim = _fingerprint_similarity(fp, rec.fingerprint)
            if fp_sim < 0.4:
                continue
            # full Jaccard only if fingerprints are close
            jsim = _jaccard_similarity(text, rec.text_preview)
            if jsim > max_sim:
                max_sim = jsim
        return round(max_sim, 4)

    def _safe_result(
        self,
        allowed: bool,
        reason: Optional[str],
        sim: float,
        state: Optional[GuardState] = None,
        cooldown: float = 0.0,
    ) -> dict:
        return {
            "allowed":                       allowed,
            "state":                         (state or self._state).value,
            "blocked_reason":                reason,
            "similarity_to_last":            sim,
            "similar_event_count":           self._similar_event_count,
            "secure_mode_cooldown_remaining": round(cooldown, 1),
        }
