"""
fence_clean.py

Strips markdown code-fence wrappers from AI-generated file content.

The agent system prompt tells the model not to wrap output in backtick
fences, but some responses still arrive like:

    ```python
    <actual file content>
    ```

or with a bare opener:

    ```
    <actual file content>
    ```

This module detects and removes those wrappers WITHOUT touching any
fences that are part of the file content itself (e.g. a README.md that
legitimately contains fenced code blocks).

Rules
-----
- Only the OUTERMOST fence pair is stripped per pass.
- A valid wrapper means: first non-empty line is a fence opener
  (` ``` ` optionally followed by a language tag), AND the last
  non-empty line is a bare ` ``` `.
- The strip is repeated up to MAX_PASSES times to handle double-wrapping.
- If the opener and closer are the same ``` line (empty response wrapped
  in a single fence), return an empty string.
- All operations are logged for TUI debug visibility.
"""

import re
from typing import Tuple

from log_util import get_logger

MAX_PASSES = 3

# Matches a fence opener: optional whitespace, ```, optional language tag
_FENCE_OPEN_RE  = re.compile(r"^```(\w*)$")
# Matches a bare fence closer
_FENCE_CLOSE_RE = re.compile(r"^```$")


def strip_fences(text: str) -> Tuple[str, int]:
    """
    Remove outermost markdown code-fence wrapper(s) from text.

    Returns (cleaned_text, passes_applied).
    passes_applied == 0 means nothing was changed.

    Fallback: any exception during processing returns (original_text, 0)
    and logs the error so it is visible in the TUI log panel.
    """
    log = get_logger()
    original = text
    passes   = 0

    try:
        for _ in range(MAX_PASSES):
            result, stripped = _strip_one(text, log)
            if stripped:
                passes += 1
                text    = result
            else:
                break
    except Exception as exc:
        log.error("FENCE", f"strip_fences error (returning original): {exc}")
        return original, 0

    if passes:
        log.info(
            "FENCE",
            f"stripped {passes} fence wrapper(s): "
            f"{len(original)} -> {len(text)} chars",
        )
    return text, passes


def _strip_one(text: str, log) -> Tuple[str, bool]:
    """
    Attempt to strip a single outermost fence pair.
    Returns (result, was_stripped).
    """
    # split preserving lines; we work on stripped copies for detection only
    lines = text.split("\n")

    # find first non-empty line index
    first_idx = next((i for i, l in enumerate(lines) if l.strip()), None)
    # find last non-empty line index
    last_idx  = next((i for i, l in enumerate(reversed(lines)) if l.strip()), None)

    if first_idx is None or last_idx is None:
        return text, False

    last_idx = len(lines) - 1 - last_idx

    if first_idx >= last_idx:
        # only one non-empty line or same line -- not a valid pair
        return text, False

    first_line = lines[first_idx].strip()
    last_line  = lines[last_idx].strip()

    if not _FENCE_OPEN_RE.match(first_line):
        return text, False
    if not _FENCE_CLOSE_RE.match(last_line):
        return text, False

    lang = (_FENCE_OPEN_RE.match(first_line).group(1) or "(none)")
    log.debug(
        "FENCE",
        f"detected wrapper: opener=```{lang}  closer=``` "
        f"at lines {first_idx}/{last_idx} of {len(lines)}",
    )

    # strip the opener and closer lines
    inner_lines = lines[first_idx + 1 : last_idx]

    # remove a single leading blank line that the model commonly inserts
    # after the opener, and a single trailing blank line before the closer
    if inner_lines and not inner_lines[0].strip():
        inner_lines = inner_lines[1:]
    if inner_lines and not inner_lines[-1].strip():
        inner_lines = inner_lines[:-1]

    return "\n".join(inner_lines), True


# ---------------------------------------------------------------------------
# Convenience wrapper -- same signature as emoji_clean.clean()
# ---------------------------------------------------------------------------
def clean(text: str) -> str:
    """Strip fences and return cleaned text. Discards pass count."""
    result, _ = strip_fences(text)
    return result
