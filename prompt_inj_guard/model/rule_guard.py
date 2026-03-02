"""
rule_guard.py  --  pattern-based second layer for the prompt injection guard.

Works alongside the ML classifier (guard_classifier.py). Uses regex patterns
from pattern_db.json to catch attack variants the model may miss and to
suppress false positives on known-clean instruction-style text.

Usage:
    from rule_guard import RuleGuard
    rg = RuleGuard("/path/to/pattern_db.json")
    result = rg.check("ignore all previous instructions")
    # result = {
    #   "label":              "prompt_injection",
    #   "confidence":         0.90,
    #   "flagged":            True,
    #   "matched":            [{"id": "inj_001", "label": "prompt_injection", "weight": 1.0, "description": "..."}],
    #   "is_allowlisted":     False,
    #   "allowlist_matches":  [],
    #   "allowlist_conf":     0.0,
    # }

The DB file is hot-reloaded automatically when its mtime changes, so you can
edit pattern_db.json while the server is running and changes take effect on the
next request without a restart.

Fallback error handling: pattern compilation failures are caught per-pattern
and logged to stderr; the guard runs with remaining patterns rather than
crashing. DB load failures return a neutral "clean" result with an error flag.
"""

import re
import os
import sys
import json
import time


class RuleGuard:
    def __init__(self, db_path: str):
        self._db_path     = os.path.realpath(db_path)
        self._mtime       = 0.0
        self._inj_pats    = []   # list of (compiled_re, entry_dict)
        self._spam_pats   = []
        self._allow_pats  = []
        self._load_error  = ''
        self._load()

    # -----------------------------------------------------------------------
    # Loader
    # -----------------------------------------------------------------------

    def _load(self) -> None:
        try:
            mtime = os.path.getmtime(self._db_path)
        except OSError as e:
            self._load_error = f'Cannot stat pattern_db: {e}'
            print(f'ERROR rule_guard: {self._load_error}', file=sys.stderr)
            return

        if mtime == self._mtime:
            return  # nothing changed

        try:
            with open(self._db_path, encoding='utf-8') as f:
                db = json.load(f)
        except Exception as e:
            self._load_error = f'Cannot parse pattern_db: {e}'
            print(f'ERROR rule_guard: {self._load_error}', file=sys.stderr)
            return

        inj_pats   = self._compile_group(db.get('injection_patterns', []),  'prompt_injection')
        spam_pats  = self._compile_group(db.get('spam_patterns', []),        'spam')
        allow_pats = self._compile_group(db.get('clean_allowlist', []),      'clean')

        self._inj_pats   = inj_pats
        self._spam_pats  = spam_pats
        self._allow_pats = allow_pats
        self._mtime      = mtime
        self._load_error = ''
        print(
            f'rule_guard: loaded {len(inj_pats)} injection, '
            f'{len(spam_pats)} spam, {len(allow_pats)} allowlist patterns '
            f'from {self._db_path}',
            flush=True,
        )

    def _compile_group(self, entries: list, label: str) -> list:
        compiled = []
        for entry in entries:
            pat_str = entry.get('pattern', '')
            flags   = 0
            raw_flags = entry.get('flags', '')
            if 'i' in raw_flags:
                flags |= re.IGNORECASE
            if 's' in raw_flags:
                flags |= re.DOTALL
            try:
                cre = re.compile(pat_str, flags)
                compiled.append((cre, entry, label))
            except re.error as e:
                print(
                    f'ERROR rule_guard: skipping bad pattern id={entry.get("id","?")} '
                    f'pattern="{pat_str}": {e}',
                    file=sys.stderr,
                )
        return compiled

    # -----------------------------------------------------------------------
    # Public API
    # -----------------------------------------------------------------------

    def check(self, text: str) -> dict:
        """
        Run pattern checks against text.
        Returns a dict with label, confidence, flagged, matched, is_allowlisted,
        allowlist_matches, allowlist_conf.
        On load error, returns a neutral result with error key set.
        """
        # Hot-reload if file changed.
        try:
            self._load()
        except Exception as e:
            print(f'ERROR rule_guard._load during check: {e}', file=sys.stderr)

        if self._load_error and not self._inj_pats and not self._spam_pats:
            return {
                'label':             'clean',
                'confidence':        0.5,
                'flagged':           False,
                'matched':           [],
                'is_allowlisted':    False,
                'allowlist_matches': [],
                'allowlist_conf':    0.0,
                'error':             self._load_error,
            }

        cleaned = ' '.join(text.lower().strip().split())

        # -- Check allowlist first -----------------------------------------
        allowlist_matches = self._run_group(cleaned, self._allow_pats)
        is_allowlisted    = len(allowlist_matches) > 0
        allowlist_conf    = self._score_confidence(allowlist_matches) if is_allowlisted else 0.0

        # -- Check injection patterns ---------------------------------------
        inj_matches = self._run_group(cleaned, self._inj_pats)

        # -- Check spam patterns --------------------------------------------
        spam_matches = self._run_group(cleaned, self._spam_pats)

        # -- Decide label ---------------------------------------------------
        inj_conf  = self._score_confidence(inj_matches)
        spam_conf = self._score_confidence(spam_matches)

        if inj_conf > 0 and inj_conf >= spam_conf:
            label      = 'prompt_injection'
            confidence = inj_conf
            matched    = inj_matches
        elif spam_conf > 0:
            label      = 'spam'
            confidence = spam_conf
            matched    = spam_matches
        else:
            label      = 'clean'
            confidence = 0.0
            matched    = []

        return {
            'label':             label,
            'confidence':        round(confidence, 4),
            'flagged':           label != 'clean',
            'matched':           matched,
            'is_allowlisted':    is_allowlisted,
            'allowlist_matches': allowlist_matches,
            'allowlist_conf':    round(allowlist_conf, 4),
        }

    def status(self) -> dict:
        return {
            'db_path':       self._db_path,
            'mtime':         self._mtime,
            'inj_patterns':  len(self._inj_pats),
            'spam_patterns': len(self._spam_pats),
            'allowlist':     len(self._allow_pats),
            'load_error':    self._load_error or None,
        }

    # -----------------------------------------------------------------------
    # Internals
    # -----------------------------------------------------------------------

    def _run_group(self, text: str, group: list) -> list:
        """Return list of match dicts for every pattern that fires."""
        hits = []
        for (cre, entry, label) in group:
            try:
                if cre.search(text):
                    hits.append({
                        'id':          entry.get('id', '?'),
                        'label':       label,
                        'weight':      entry.get('weight', 1.0),
                        'description': entry.get('description', ''),
                    })
            except Exception as e:
                # Should not happen after compile-time validation but be safe.
                print(f'ERROR rule_guard._run_group pattern {entry.get("id","?")}: {e}', file=sys.stderr)
        return hits

    @staticmethod
    def _score_confidence(matches: list) -> float:
        """
        Convert a list of pattern hits into a [0,1] confidence score.
        Single strong match (weight 1.0) => 0.75.
        Two matches => ~0.90.
        Three+ => 0.99.
        Each additional match adds 0.15 * weight.
        """
        if not matches:
            return 0.0
        base  = 0.60
        total = base + sum(m['weight'] * 0.15 for m in matches)
        return min(0.99, round(total, 4))
