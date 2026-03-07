"""
convo.py
Rolling tech-slang commentary that runs alongside Haiku while it's processing.

OVERVIEW
  While the Haiku file-edit agent is streaming, a ConvoSession calls GPT-4o mini
  and asks it to produce a short running commentary about the pending task written
  in developer tech slang. Each generated message is delivered to the caller via
  a callback so the TUI can feed it into the log panel in real time.

  If the OpenAI API is unavailable (missing key, network error, quota), the session
  automatically falls back to randomly sampled phrases from phrases.db so the
  "ai-working" commentary still appears.

PUBLIC API
  session = ConvoSession()
  session.start(file_path, instruction, on_message)
      Starts a background daemon thread. on_message(text: str) is called once per
      generated message line. Returns immediately.

  session.stop()
      Signals the background thread to stop cleanly. Safe to call multiple times.

PHRASES DB
  Reads from: ../../ai/data/phrases.db  (relative to this file)
  Table: phrases
  Columns: id, text, category, use_count, created_at
  Categories used: "ai-working" (primary), "fallback" (secondary)

GPT MODEL
  Model: gpt-4o-mini
  Key file: /Users/mac/Desktop/my_keys/openai_key.txt
  Configured via: ../../ai/config.json  (same config.json as agent.py)
"""

import os
import json
import random
import sqlite3
import threading
import time
import traceback
from typing import Callable, Optional

from log_util import get_logger

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_HERE         = os.path.dirname(os.path.abspath(__file__))
_CONFIG_PATH  = os.path.normpath(os.path.join(_HERE, "../../ai/config.json"))
_PHRASES_DB   = os.path.normpath(os.path.join(_HERE, "../../ai/data/phrases.db"))

GPT_MODEL = "gpt-4o-mini"

# How many commentary lines to request from the model
_MSG_COUNT = 5

# Seconds to wait between drip-feeding fallback phrases when GPT is unavailable
_PHRASE_INTERVAL = 2.8

# ---------------------------------------------------------------------------
# System prompt for the model
# ---------------------------------------------------------------------------
_SYSTEM_PROMPT = """\
You are a developer giving running tech-slang commentary on an AI coding job.
Write exactly {count} short messages, one per line, no blank lines between them.
Each message must be 15-30 words. Use dev vernacular: spin up, diff, refactor,
ship the patch, hit the endpoint, parse the AST, stage the commit, LGTM, etc.
Tell the story progressively: set-up, analysis, implementation, verify, done.
No emojis. No bullets or numbers. No markdown. Output only the {count} lines.
""".format(count=_MSG_COUNT).strip()

# ---------------------------------------------------------------------------
# Phrase fallback -- reads from phrases.db
# ---------------------------------------------------------------------------
class _PhraseFallback:
    """
    Reads "ai-working" phrases from phrases.db.
    Thread-safe: each call opens its own short-lived connection.
    Falls back to a hardcoded list if the DB is missing.
    """

    _HARDCODED = [
        "Spinning up the diff engine, hold tight while we crunch the payload.",
        "Parsing the AST, mapping all the symbol refs before we refactor.",
        "Staging the mutations, making sure the change set stays clean.",
        "Running a quick sanity check on the patched output, looks nominal.",
        "Shipping the delta, committing the rewrite straight to the staging buffer.",
        "Scanning for edge cases in the new logic, tightening up the guards.",
        "Reconciling the token stream, verifying structural integrity post-patch.",
    ]

    def __init__(self, db_path: str):
        self._db_path = db_path
        self._cache: list[str] = []
        self._loaded = False

    def _load(self):
        log = get_logger()
        if not os.path.exists(self._db_path):
            log.warning("CONVO", f"phrases.db not found at {self._db_path} -- using hardcoded fallback")
            self._cache = list(self._HARDCODED)
            self._loaded = True
            return
        try:
            conn = sqlite3.connect(self._db_path, timeout=5)
            cur  = conn.execute(
                "SELECT text FROM phrases WHERE category = 'ai-working' ORDER BY id"
            )
            rows = [r[0] for r in cur.fetchall()]
            conn.close()
            if rows:
                self._cache = rows
                log.debug("CONVO", f"loaded {len(rows)} ai-working phrases from phrases.db")
            else:
                log.warning("CONVO", "phrases table empty -- using hardcoded fallback")
                self._cache = list(self._HARDCODED)
        except Exception as exc:
            log.error("CONVO", f"phrases.db read failed: {exc}")
            self._cache = list(self._HARDCODED)
        self._loaded = True

    def sample(self, n: int) -> list[str]:
        if not self._loaded:
            self._load()
        if not self._cache:
            return list(self._HARDCODED[:n])
        pool = self._cache[:]
        random.shuffle(pool)
        # repeat if we need more than the pool size
        while len(pool) < n:
            extra = self._cache[:]
            random.shuffle(extra)
            pool += extra
        return pool[:n]


# module-level singleton so we only load the DB once per process
_fallback = _PhraseFallback(_PHRASES_DB)


# ---------------------------------------------------------------------------
# API key loader  (mirrors agent.py pattern exactly)
# ---------------------------------------------------------------------------
def _load_openai_key() -> str:
    log = get_logger()
    try:
        with open(_CONFIG_PATH, "r", encoding="utf-8") as f:
            config = json.load(f)
        keys_path = config.get("keys_path", "")
        key_files = config.get("key_files", {})
        key_file  = key_files.get("openai", "openai_key.txt")
        key_path  = os.path.join(keys_path, key_file)
        with open(key_path, "r", encoding="utf-8") as f:
            key = f.read().strip()
        if not key:
            raise ValueError("openai key file is empty")
        log.debug("CONVO", f"loaded OpenAI key from {key_path}")
        return key
    except Exception as exc:
        log.warning("CONVO", f"_load_openai_key failed: {exc}  -- will use fallback phrases")
        raise


# ---------------------------------------------------------------------------
# ConvoSession
# ---------------------------------------------------------------------------
class ConvoSession:
    """
    Manages one GPT-4o mini commentary session.

    Usage:
        session = ConvoSession()
        session.start(file_path, instruction, on_message=lambda msg: ...)
        # ... haiku does its work ...
        session.stop()
    """

    def __init__(self):
        self._stop_event = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._log = get_logger()

    # ------------------------------------------------------------------
    # Public
    # ------------------------------------------------------------------
    def start(
        self,
        file_path:   str,
        instruction: str,
        on_message:  Callable[[str], None],
    ) -> None:
        """
        Kick off commentary in a background daemon thread.
        on_message(text) is called for each generated message.
        Returns immediately.
        """
        self._stop_event.clear()
        self._thread = threading.Thread(
            target=self._run,
            args=(file_path, instruction, on_message),
            daemon=True,
            name="convo-gpt",
        )
        self._thread.start()
        self._log.info("CONVO", f"session started for {os.path.basename(file_path)!r}")

    def stop(self) -> None:
        """Signal the session to stop. Does not block."""
        self._stop_event.set()
        self._log.debug("CONVO", "session stop signalled")

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------
    def _run(
        self,
        file_path:   str,
        instruction: str,
        on_message:  Callable[[str], None],
    ) -> None:
        try:
            self._try_gpt(file_path, instruction, on_message)
        except Exception as exc:
            self._log.error("CONVO", f"GPT session failed: {exc}")
            self._log.debug("CONVO", traceback.format_exc()[:400])
            self._drip_fallback(on_message)

    def _try_gpt(
        self,
        file_path:   str,
        instruction: str,
        on_message:  Callable[[str], None],
    ) -> None:
        """
        Call GPT-4o mini via the OpenAI streaming API.
        Streams the response and calls on_message for each completed line.
        Raises on any error so _run can fall back to phrases.
        """
        try:
            import openai as _openai
        except ImportError:
            raise RuntimeError("openai SDK not installed -- run: pip install openai")

        key = _load_openai_key()  # raises on failure

        basename = os.path.basename(file_path)
        ext      = os.path.splitext(basename)[1] or "(unknown)"

        user_msg = (
            f"Task: {instruction}\n"
            f"File: {basename}  (type: {ext})\n\n"
            f"Write {_MSG_COUNT} progressive tech-slang commentary messages, one per line."
        )

        self._log.info("CONVO", f"calling {GPT_MODEL} for tech-slang commentary")

        client = _openai.OpenAI(api_key=key)

        # accumulate streaming chars; deliver each complete line as a message
        buffer = ""
        # NOTE: once GPT starts streaming we let the whole response land.
        # We do NOT check stop_event inside the stream loop -- the response
        # is capped at 350 tokens so it is always fast. The stop event only
        # gates the inter-message pause so messages still roll in naturally
        # even if Haiku finishes before GPT does.

        stream = client.chat.completions.create(
            model=GPT_MODEL,
            max_tokens=350,
            stream=True,
            messages=[
                {"role": "system", "content": _SYSTEM_PROMPT},
                {"role": "user",   "content": user_msg},
            ],
        )

        try:
            for chunk in stream:
                delta = None
                try:
                    delta = chunk.choices[0].delta.content if chunk.choices else None
                except (IndexError, AttributeError) as _ce:
                    self._log.debug("CONVO", f"chunk access error: {_ce}")
                    continue
                if delta is None:
                    continue

                buffer += delta

                # deliver each complete line as soon as we have it
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if line:
                        self._deliver(line, on_message)
                        # brief pacing pause so messages roll in one at a time;
                        # stop_event shortens the wait but does NOT skip messages
                        self._stop_event.wait(timeout=0.35)
        finally:
            try:
                stream.close()
            except Exception:
                pass

        # flush any remaining text that did not end with a newline
        if buffer.strip():
            self._deliver(buffer.strip(), on_message)

        self._log.info("CONVO", "GPT stream finished")

    def _drip_fallback(self, on_message: Callable[[str], None]) -> None:
        """
        Deliver phrases from phrases.db at timed intervals when GPT is unavailable.
        Respects the stop event so it halts when Haiku finishes.
        """
        self._log.info("CONVO", f"using phrases.db fallback ({_MSG_COUNT} phrases)")
        phrases = _fallback.sample(_MSG_COUNT)
        for phrase in phrases:
            if self._stop_event.is_set():
                break
            self._deliver(phrase, on_message)
            self._stop_event.wait(timeout=_PHRASE_INTERVAL)

    def _deliver(self, text: str, on_message: Callable[[str], None]) -> None:
        """Call the on_message callback safely, logging any exception."""
        try:
            on_message(text)
        except Exception as exc:
            self._log.error("CONVO", f"on_message callback raised: {exc}")
