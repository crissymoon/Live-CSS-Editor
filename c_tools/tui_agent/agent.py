"""
agent.py
Calls Claude claude-haiku-4-5-20251001 via the Anthropic SDK.
Reads the API key from /Users/mac/Documents/live-css/ai/config.json
which points key files at ~/Desktop/my_keys/.

The agent receives a file's content + user instruction and returns
the fully rewritten file content (no explanations, no markdown fences).

Streaming is used so callers can receive live token counts via on_chunk(chars_so_far).
"""

import os
import json
import sys
from typing import Callable, Optional, Tuple

from log_util import get_logger
from emoji_clean import clean

HAIKU_MODEL       = "claude-haiku-4-5-20251001"
CONFIG_PATH       = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "../../ai/config.json")
)

SYSTEM_PROMPT = """\
You are a precise code editing agent operating inside a sandboxed tool.
Rules you must follow without exception:
1. You receive the FULL content of ONE existing file plus a user instruction.
2. You return ONLY the complete updated content of THAT file, with the instruction applied.
3. Do NOT create, reference, or produce content for any other file path.
4. Do NOT include markdown code fences, explanations, or commentary of any kind.
5. Do NOT prepend or append any text outside the file content itself.
6. Preserve indentation, encoding, and line endings exactly as in the original.
7. If no changes are needed, return the original content exactly as given.
8. Never add new file headers that suggest a different filename.
The output you produce will be written verbatim back to the same file that was given to you.
""".strip()


def _load_api_key() -> str:
    log = get_logger()
    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            config = json.load(f)
        keys_path = config.get("keys_path", "")
        key_files = config.get("key_files", {})
        key_file  = key_files.get("anthropic", "anthropic_key.txt")
        key_path  = os.path.join(keys_path, key_file)
        with open(key_path, "r", encoding="utf-8") as f:
            key = f.read().strip()
        if not key:
            raise ValueError("anthropic key file is empty")
        log.info("AGENT", f"loaded API key from {key_path}")
        return key
    except Exception as exc:
        log.error("AGENT", f"_load_api_key failed: {exc}")
        raise


def call_haiku(
    file_path: str,
    file_content: str,
    instruction: str,
    on_chunk: Optional[Callable[[int, str], None]] = None,
) -> Tuple[Optional[str], Optional[str]]:
    """
    Send file_content + instruction to Haiku using the streaming API.

    on_chunk(chars_received, latest_snippet) is called on every text delta
    so callers can show a live byte counter without polling.

    Returns (new_content, None) on success or (None, error_message) on failure.
    emoji_clean is applied to the response before returning.
    """
    log = get_logger()
    try:
        import anthropic as _anthropic
    except ImportError as exc:
        err = f"anthropic SDK not installed: {exc}"
        log.error("AGENT", err)
        return None, err

    try:
        api_key = _load_api_key()
    except Exception as exc:
        return None, str(exc)

    user_message = (
        f"File: {os.path.basename(file_path)}\n\n"
        f"Instruction: {instruction}\n\n"
        f"Current file content:\n"
        f"{file_content}"
    )

    log.info("AGENT", f"calling {HAIKU_MODEL} for {os.path.basename(file_path)}")
    log.debug("AGENT", f"instruction: {instruction[:120]}")

    try:
        client = _anthropic.Anthropic(api_key=api_key)
        chunks: list[str] = []
        total_chars = 0

        with client.messages.stream(
            model=HAIKU_MODEL,
            max_tokens=4096,
            system=SYSTEM_PROMPT,
            messages=[{"role": "user", "content": user_message}],
        ) as stream:
            for text_delta in stream.text_stream:
                chunks.append(text_delta)
                total_chars += len(text_delta)
                if on_chunk:
                    try:
                        on_chunk(total_chars, text_delta)
                    except Exception as cb_exc:
                        log.debug("AGENT", f"on_chunk callback error: {cb_exc}")

        raw     = "".join(chunks)
        cleaned = clean(raw)
        log.info("AGENT", f"stream complete: {len(raw)} chars raw, {len(cleaned)} after emoji_clean")
        return cleaned, None

    except Exception as exc:
        err = f"API call failed: {exc}"
        log.error("AGENT", err)
        return None, err
