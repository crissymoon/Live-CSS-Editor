#!/usr/bin/env python3
"""
mood_check.py
Analyse the emotional mood of a text prompt using the EMI bio-inspired engine.
Returns a one-sentence prompt addition that can be appended to any AI API request.

The text is scored in overlapping word-chunks so arbitrarily long inputs are handled
without memory pressure. Scores from all chunks are aggregated (weighted by each
chunk's lexicon match rate) before the final mood sentence is built.

Usage
-----
  # score text passed as an argument
  python mood_check.py "I am really excited about this project"

  # score text from stdin
  echo "I feel so tired and overwhelmed today" | python mood_check.py

  # score a file
  python mood_check.py --file /path/to/prompt.txt

  # print only the one-sentence addon (useful for scripting)
  python mood_check.py --addon "some text here"

  # see full score breakdown as JSON
  python mood_check.py --json "some text here"

  # tune chunk size (words per chunk, default 250)
  python mood_check.py --chunk-words 150 "some very long prompt..."

Exit codes
----------
  0  success
  1  engine load failure
  2  bad arguments / no input
"""

import json
import os
import sys
import textwrap
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Path setup -- resolve the emi_model symlink so we can import engine.py
# ---------------------------------------------------------------------------

_HERE     = os.path.dirname(os.path.abspath(__file__))
_EMI_DIR  = os.path.join(_HERE, "emi_model")

if not os.path.isdir(_EMI_DIR):
    print(
        f"[mood_check] ERROR: symlink 'emi_model' not found in {_HERE}\n"
        f"  Run:  ln -s /Users/mac/Documents/deploy_emi_model {_HERE}/emi_model",
        file=sys.stderr,
    )
    sys.exit(1)

sys.path.insert(0, _EMI_DIR)

try:
    from engine import EMIEngine  # type: ignore
except Exception as _engine_import_err:
    print(
        f"[mood_check] ERROR: could not import EMIEngine from {_EMI_DIR}\n"
        f"  Detail: {_engine_import_err}",
        file=sys.stderr,
    )
    sys.exit(1)

# ---------------------------------------------------------------------------
# Mood sentence templates
# One sentence per mood that forms a natural prompt addition.
# The sentence is designed to inform an AI about the user's emotional state.
# ---------------------------------------------------------------------------

_MOOD_SENTENCES: Dict[str, str] = {
    "joy":      "The user is writing with enthusiasm and positivity.",
    "love":     "The user's message carries warmth and genuine affection.",
    "care":     "The user's tone is thoughtful and nurturing in this request.",
    "anger":    "The user's writing carries noticeable frustration or irritation.",
    "sadness":  "The user appears to be writing from a place of sadness or low energy.",
    "fear":     "The user's message conveys anxiety or apprehension about the topic.",
    "stress":   "The user's writing suggests they may be under pressure or stress.",
    "neutral":  "The user's message is calm and emotionally neutral.",
}

# Secondary-signal qualifiers appended when a runner-up emotion is significant
_SECONDARY_PHRASES: Dict[str, str] = {
    "joy":     "with moments of optimism",
    "love":    "with warmth",
    "care":    "with care",
    "anger":   "with underlying tension",
    "sadness": "with a low or tired undercurrent",
    "fear":    "with hints of apprehension",
    "stress":  "with signs of stress",
    "neutral": "",
}

# Colour helpers (no external deps)
_RESET   = "\033[0m"
_BOLD    = "\033[1m"
_DIM     = "\033[2m"
_CYAN    = "\033[36m"
_YELLOW  = "\033[33m"
_GREEN   = "\033[32m"
_RED     = "\033[31m"
_MAGENTA = "\033[35m"
_BLUE    = "\033[34m"
_WHITE   = "\033[37m"

_EMOTION_COLORS = {
    "joy":     _YELLOW,
    "love":    _MAGENTA,
    "care":    _CYAN,
    "anger":   _RED,
    "sadness": _BLUE,
    "fear":    _MAGENTA,
    "stress":  _RED,
    "neutral": _WHITE,
}


def _col(text: str, c: str, bold: bool = False) -> str:
    b = _BOLD if bold else ""
    return f"{b}{c}{text}{_RESET}"


def _emotion_col(emotion: str, bold: bool = False) -> str:
    return _col(emotion.upper(), _EMOTION_COLORS.get(emotion, _WHITE), bold=bold)


# ---------------------------------------------------------------------------
# Chunking
# ---------------------------------------------------------------------------

def _chunk_words(text: str, chunk_size: int, overlap: int) -> List[str]:
    """
    Split text into overlapping word-windows.

    chunk_size  -- max words per chunk
    overlap     -- words shared between consecutive chunks
    """
    words = text.split()
    if not words:
        return []

    chunks = []
    step   = max(1, chunk_size - overlap)
    i      = 0
    while i < len(words):
        chunk_words_list = words[i: i + chunk_size]
        chunks.append(" ".join(chunk_words_list))
        if i + chunk_size >= len(words):
            break
        i += step

    return chunks


# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------

def _aggregate_scores(chunk_results: List[Dict]) -> Dict[str, float]:
    """
    Weighted average of bio_scores across chunks.
    Each chunk is weighted by its match_rate so low-signal chunks
    (mostly unrecognised words) contribute less.
    """
    weight_sum = 0.0
    agg: Dict[str, float] = {}

    for res in chunk_results:
        w = res.get("match_rate", 0.0)
        if w <= 0:
            continue
        weight_sum += w
        for emotion, score in res.get("bio_scores", {}).items():
            agg[emotion] = agg.get(emotion, 0.0) + score * w

    if weight_sum <= 0:
        return {}

    return {em: v / weight_sum for em, v in agg.items()}


def _confidence(agg: Dict[str, float]) -> Tuple[str, float, str, float]:
    """
    Return (dominant, dom_confidence, runner_up, runner_up_confidence).
    Confidence is each emotion's fraction of the total signal.
    """
    if not agg:
        return "neutral", 0.0, "neutral", 0.0

    total = sum(agg.values())
    if total <= 0:
        return "neutral", 0.0, "neutral", 0.0

    ranked = sorted(agg.items(), key=lambda x: x[1], reverse=True)
    dominant     = ranked[0][0]
    dom_conf     = ranked[0][1] / total

    runner_up    = ranked[1][0] if len(ranked) > 1 else "neutral"
    runner_conf  = (ranked[1][1] / total) if len(ranked) > 1 else 0.0

    return dominant, dom_conf, runner_up, runner_conf


# ---------------------------------------------------------------------------
# Sentence builder
# ---------------------------------------------------------------------------

def build_mood_sentence(
    dominant: str,
    dom_confidence: float,
    runner_up: str,
    runner_conf: float,
    total_words: int,
    matched_words: int,
    n_chunks: int,
) -> str:
    """
    Build a one-sentence prompt addition from the aggregated mood signal.
    A secondary clause is appended when the runner-up reaches 20%+ of signal.
    When lexicon coverage is very low (<10%) the sentence notes low certainty.
    """
    base = _MOOD_SENTENCES.get(dominant, _MOOD_SENTENCES["neutral"])

    # strip the period so we can optionally extend it
    base = base.rstrip(".")

    # add secondary signal if meaningful and different from dominant
    if runner_up and runner_up != dominant and runner_conf >= 0.20:
        secondary = _SECONDARY_PHRASES.get(runner_up, "")
        if secondary:
            base = base + " " + secondary

    # coverage qualifier
    coverage = (matched_words / total_words) if total_words > 0 else 0.0
    if coverage < 0.10:
        base = base + " (low lexicon coverage -- signal may be limited)"

    return base + "."


# ---------------------------------------------------------------------------
# Core analysis function
# ---------------------------------------------------------------------------

def analyse(
    text: str,
    engine: EMIEngine,
    chunk_size: int = 250,
    overlap:    int = 30,
) -> Dict:
    """
    Analyse text in chunks and return:
      {
        "mood_sentence": str,           -- one-sentence prompt addition
        "dominant":      str,           -- dominant emotion name
        "dom_confidence":float,         -- 0-1 fraction of total signal
        "runner_up":     str,
        "runner_conf":   float,
        "agg_scores":    dict,          -- weighted-avg bio scores per emotion
        "n_chunks":      int,
        "total_words":   int,
        "matched_words": int,           -- sum across all chunks (de-dup not applied)
        "chunk_results": list[dict],    -- raw score dicts from EMIEngine.score()
      }
    """
    text = (text or "").strip()
    if not text:
        return {
            "mood_sentence": _MOOD_SENTENCES["neutral"],
            "dominant":      "neutral",
            "dom_confidence": 0.0,
            "runner_up":     "neutral",
            "runner_conf":   0.0,
            "agg_scores":    {},
            "n_chunks":      0,
            "total_words":   0,
            "matched_words": 0,
            "chunk_results": [],
        }

    chunks = _chunk_words(text, chunk_size=chunk_size, overlap=overlap)
    if not chunks:
        chunks = [text]

    chunk_results = []
    total_words_sum   = 0
    matched_words_sum = 0

    for i, chunk in enumerate(chunks):
        try:
            result = engine.score(chunk)
            chunk_results.append(result)
            total_words_sum   += result.get("total_words",   0)
            matched_words_sum += result.get("matched_words", 0)
        except Exception as exc:
            print(
                f"[mood_check] WARNING: chunk {i+1}/{len(chunks)} scoring failed: {exc}",
                file=sys.stderr,
            )

    agg = _aggregate_scores(chunk_results)
    dominant, dom_conf, runner_up, runner_conf = _confidence(agg)

    sentence = build_mood_sentence(
        dominant     = dominant,
        dom_confidence = dom_conf,
        runner_up    = runner_up,
        runner_conf  = runner_conf,
        total_words  = total_words_sum,
        matched_words = matched_words_sum,
        n_chunks     = len(chunk_results),
    )

    return {
        "mood_sentence":  sentence,
        "dominant":       dominant,
        "dom_confidence": round(dom_conf, 4),
        "runner_up":      runner_up,
        "runner_conf":    round(runner_conf, 4),
        "agg_scores":     {k: round(v, 6) for k, v in agg.items()},
        "n_chunks":       len(chunk_results),
        "total_words":    total_words_sum,
        "matched_words":  matched_words_sum,
        "chunk_results":  chunk_results,
    }


# ---------------------------------------------------------------------------
# Pretty-print helpers
# ---------------------------------------------------------------------------

_W = 60

def _hr(c: str = "-") -> str:
    return _col(c * _W, _DIM)


def _bar(value: float, max_val: float, width: int = 28, emotion: str = "") -> str:
    if max_val <= 0:
        filled = 0
    else:
        filled = int((value / max_val) * width)
    filled = max(0, min(filled, width))
    col_c  = _EMOTION_COLORS.get(emotion, _GREEN)
    return "[" + _col("#" * filled, col_c, bold=True) + _col("." * (width - filled), _DIM) + "]"


def print_report(result: Dict, text_preview: str = ""):
    dominant   = result["dominant"]
    dom_conf   = result["dom_confidence"]
    runner_up  = result["runner_up"]
    runner_conf= result["runner_conf"]
    agg        = result["agg_scores"]
    n_chunks   = result["n_chunks"]
    total_w    = result["total_words"]
    matched_w  = result["matched_words"]
    sentence   = result["mood_sentence"]

    print()
    print(_hr("="))
    print(_col("  MOOD CHECK", _WHITE, bold=True))
    print(_hr("="))

    if text_preview:
        preview = text_preview[:80] + ("..." if len(text_preview) > 80 else "")
        print(f"  {_col('Input:', _DIM + _WHITE)}  {preview}")
        print()

    print(
        f"  {_col('Chunks:', _DIM + _WHITE)}  {n_chunks}   "
        f"{_col('Words:', _DIM + _WHITE)}  {total_w}   "
        f"{_col('Matched:', _DIM + _WHITE)}  {matched_w}"
    )

    coverage = (matched_w / total_w * 100) if total_w > 0 else 0.0
    cov_col  = _GREEN if coverage > 30 else (_YELLOW if coverage > 10 else _RED)
    print(f"  {_col('Lexicon coverage:', _DIM + _WHITE)}  {_col(f'{coverage:.1f}%', cov_col)}")
    print()

    # dominant + runner-up
    max_agg = max(agg.values()) if agg else 1.0
    print(f"  {_col('Dominant:', _DIM + _WHITE)}  {_emotion_col(dominant, bold=True)}  "
          f"  {_bar(dom_conf, 1.0, emotion=dominant)}  {_col(f'{dom_conf*100:.1f}% of signal', _DIM + _WHITE)}")

    if runner_up and runner_up != dominant:
        print(f"  {_col('Runner-up:', _DIM + _WHITE)} {_emotion_col(runner_up)}  "
              f"  {_bar(runner_conf, 1.0, emotion=runner_up)}  "
              f"{_col(f'{runner_conf*100:.1f}% of signal', _DIM + _WHITE)}")

    # sorted emotion bars
    if agg:
        print()
        print(f"  {_col('All emotion scores (aggregated):', _DIM + _WHITE)}")
        for emotion, score in sorted(agg.items(), key=lambda x: x[1], reverse=True):
            marker = _col(" <", _DIM + _YELLOW) if emotion == dominant else ""
            print(
                f"    {_emotion_col(emotion, bold=(emotion==dominant)):<24}"
                f"  {_bar(score, max_agg, width=24, emotion=emotion)}"
                f"  {_col(f'{score:.5f}', _WHITE)}{marker}"
            )

    print()
    print(_hr())
    print(f"  {_col('Mood sentence (prompt addition):', _WHITE, bold=True)}")
    print()
    for line in textwrap.wrap(f'    "{sentence}"', width=_W + 4):
        print(_col(line, _CYAN))
    print()
    print(_hr("="))
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _read_stdin_if_available() -> Optional[str]:
    try:
        if not sys.stdin.isatty():
            data = sys.stdin.read()
            return data.strip() or None
    except Exception as exc:
        print(f"[mood_check] stdin read error: {exc}", file=sys.stderr)
    return None


def main():
    import argparse

    parser = argparse.ArgumentParser(
        prog="mood_check",
        description="Analyse the mood of a text prompt and return a one-sentence AI prompt addition.",
    )
    parser.add_argument(
        "text",
        nargs="?",
        default=None,
        help="Text to analyse (pass as argument or pipe via stdin)",
    )
    parser.add_argument(
        "--file",
        default=None,
        metavar="PATH",
        help="Read input text from a file",
    )
    parser.add_argument(
        "--addon",
        action="store_true",
        help="Print only the one-sentence mood addition (no report)",
    )
    parser.add_argument(
        "--json",
        dest="as_json",
        action="store_true",
        help="Print full result as JSON (no colour output)",
    )
    parser.add_argument(
        "--chunk-words",
        type=int,
        default=250,
        metavar="N",
        help="Max words per scoring chunk (default: 250)",
    )
    parser.add_argument(
        "--overlap",
        type=int,
        default=30,
        metavar="N",
        help="Word overlap between consecutive chunks (default: 30)",
    )
    parser.add_argument(
        "--lexicon",
        default=None,
        metavar="PATH",
        help="Override path to lexicon.json",
    )

    args = parser.parse_args()

    # --- resolve input text ---
    text: Optional[str] = None

    if args.file:
        try:
            with open(args.file, "r", encoding="utf-8", errors="replace") as fh:
                text = fh.read().strip()
        except Exception as exc:
            print(f"[mood_check] ERROR: could not read file {args.file!r}: {exc}", file=sys.stderr)
            sys.exit(2)

    if not text and args.text:
        text = args.text.strip()

    if not text:
        text = _read_stdin_if_available()

    if not text:
        parser.print_help()
        print("\n[mood_check] ERROR: no input text supplied.", file=sys.stderr)
        sys.exit(2)

    # --- load engine ---
    lexicon_path = args.lexicon
    if not lexicon_path:
        # default: lexicon.json inside the emi_model symlink
        lexicon_path = os.path.join(_EMI_DIR, "lexicon.json")

    try:
        engine = EMIEngine(lexicon_path)
    except Exception as exc:
        print(f"[mood_check] ERROR: EMIEngine failed to load: {exc}", file=sys.stderr)
        sys.exit(1)

    # --- analyse ---
    result = analyse(
        text,
        engine,
        chunk_size = args.chunk_words,
        overlap    = args.overlap,
    )

    # --- output ---
    if args.as_json:
        # strip large chunk_results to keep output manageable
        out = {k: v for k, v in result.items() if k != "chunk_results"}
        out["n_chunks"] = result["n_chunks"]
        print(json.dumps(out, indent=2, ensure_ascii=False))
        return

    if args.addon:
        print(result["mood_sentence"])
        return

    print_report(result, text_preview=text)


if __name__ == "__main__":
    main()
