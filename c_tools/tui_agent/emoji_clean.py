"""
emoji_clean.py
Python mirror of emoji_replacer.h / emoji_replacer.c

Strips or replaces emoji sequences from AI API responses so they
render safely in a terminal TUI without breaking layout.
"""

import re
import sys
from typing import Optional

from log_util import get_logger

# Mapping mirrors DEFAULT_MAPPINGS in emoji_replacer.c
DEFAULT_MAP = {
    "\U0001f4c1": "[FOLDER]",
    "\U0001f4c2": "[OPEN_FOLDER]",
    "\U0001f4c4": "[FILE]",
    "\U0001f4be": "[SAVE]",
    "\u270f\ufe0f": "[EDIT]",
    "\u2705":       "[CHECK]",
    "\u274c":       "[CLOSE]",
    "\U0001f50d": "[SEARCH]",
    "\u2699\ufe0f": "[SETTINGS]",
    "\U0001f3a8": "[THEME]",
    "\U0001f41b": "[DEBUG]",
    "\u26a0\ufe0f": "[WARNING]",
    "\U0001f525": "[HOT]",
    "\U0001f4a1": "[IDEA]",
    "\U0001f680": "[LAUNCH]",
    "\U0001f4dd": "[NOTE]",
    "\U0001f527": "[TOOL]",
    "\U0001f4ca": "[CHART]",
    "\U0001f4c8": "[TRENDING_UP]",
    "\U0001f4c9": "[TRENDING_DOWN]",
    "\U0001f512": "[LOCK]",
    "\U0001f513": "[UNLOCK]",
    "\U0001f310": "[WEB]",
    "\U0001f4f1": "[MOBILE]",
    "\U0001f4bb": "[COMPUTER]",
    "\U0001f5a5\ufe0f": "[DESKTOP]",
    "\u2328\ufe0f": "[KEYBOARD]",
    "\U0001f5b1\ufe0f": "[MOUSE]",
    "\U0001f3af": "[TARGET]",
    "\u2728":       "[SPARKLE]",
    "\U0001f3c1": "[FLAG]",
    "\U0001f514": "[NOTIFICATION]",
    "\U0001f4ee": "[MAILBOX]",
    "\U0001f4ec": "[MAIL]",
    "\U0001f4eb": "[INBOX]",
    "\U0001f5d1\ufe0f": "[TRASH]",
    "\U0001f4cc": "[PIN]",
    "\U0001f517": "[LINK]",
    "\u27a1\ufe0f": "->",
    "\u2b05\ufe0f": "<-",
    "\u2b06\ufe0f": "^",
    "\u2b07\ufe0f": "v",
}

# Broad Unicode emoji range pattern (covers most emoji blocks)
_EMOJI_RE = re.compile(
    "[\U0001f000-\U0001ffff"       # misc symbols and pictographs
    "\U00002600-\U000027bf"        # misc symbols
    "\U0000fe00-\U0000fe0f"        # variation selectors
    "\U0001f300-\U0001f9ff"        # more emoji
    "\u200d"                       # ZWJ
    "\u2640-\u2642"
    "]+",
    flags=re.UNICODE,
)


class EmojiCleaner:
    def __init__(self, custom_map: Optional[dict] = None):
        self._map       = dict(DEFAULT_MAP)
        self._stats     = {}        # emoji -> count
        self._total     = 0
        self._log       = get_logger()
        if custom_map:
            self._map.update(custom_map)
        self._log.debug("EMOJI", "EmojiCleaner initialized with %d mappings" % len(self._map))

    def add_mapping(self, emoji: str, placeholder: str):
        self._map[emoji] = placeholder
        self._log.debug("EMOJI", f"added mapping {repr(emoji)} -> {placeholder}")

    def process(self, text: str) -> str:
        """Replace known emojis with placeholders, strip unknown ones."""
        if not text:
            return text

        result = text

        # Pass 1: apply known mappings (longest-match-first)
        for emoji, placeholder in sorted(self._map.items(), key=lambda x: -len(x[0])):
            if emoji in result:
                count = result.count(emoji)
                result = result.replace(emoji, placeholder)
                self._stats[emoji] = self._stats.get(emoji, 0) + count
                self._total += count

        # Pass 2: strip any remaining emoji sequences
        def _strip(m):
            seq = m.group(0)
            self._stats[seq] = self._stats.get(seq, 0) + 1
            self._total += 1
            return ""

        result = _EMOJI_RE.sub(_strip, result)
        return result

    def process_lines(self, text: str) -> str:
        """Process each line independently (safer for multiline AI output)."""
        try:
            return "\n".join(self.process(line) for line in text.splitlines())
        except Exception as exc:
            self._log.error("EMOJI", f"process_lines failed: {exc}")
            return text

    def stats(self) -> dict:
        return {"total_replacements": self._total, "by_emoji": dict(self._stats)}

    def reset_stats(self):
        self._stats = {}
        self._total = 0


_cleaner: Optional[EmojiCleaner] = None

def get_cleaner() -> EmojiCleaner:
    global _cleaner
    if _cleaner is None:
        _cleaner = EmojiCleaner()
    return _cleaner

def clean(text: str) -> str:
    """Module-level convenience: clean emojis from text."""
    return get_cleaner().process_lines(text)
