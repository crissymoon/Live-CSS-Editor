#!/usr/bin/env python3
"""
pdf_convert.py
--------------
Convert a plain-text document to print-ready HTML.

The module infers document structure from text patterns --
no markup in the source file is required. It detects:

  h1  -- document title (first significant heading)
  h2  -- major section headings
  h3  -- sub-section headings / term labels
  p   -- body paragraphs
  ul  -- bullet lists (lines starting with - * + or bullet char)
  ol  -- numbered lists (lines starting with 1. 2. etc.)
  blockquote -- indented or quoted passages
  tagline -- short closing line at end of document
  hr  -- implied section break (blank line before a new h2 after body text)

Color themes:
  default       Clean white, dark charcoal text, blue-tinted headings
  professional  Navy/slate, serif headings, tight margins
  dark          Charcoal page, near-white text, amber headings
  minimal       Pure black on white, no decorative color
  xcm           XcaliburMoon brand (dark page, cyan/teal accent)

Usage (CLI):
    python3 pdf_convert.py doc.txt
    python3 pdf_convert.py doc.txt --theme xcm --out output.html
    python3 pdf_convert.py doc.txt --theme dark --title "My Report"

Usage (module):
    from c_tools.pdf_convert import convert
    html = convert(source_path="doc.txt", theme="professional")
    html = convert(text="raw string content", theme="default", title="Report")
"""

import os
import re
import sys
import html as html_lib
import traceback
import argparse
from dataclasses import dataclass, field
from typing import List, Optional, Tuple


# ---------------------------------------------------------------------------
# Block types
# ---------------------------------------------------------------------------

BLOCK_TYPES = ("h1", "h2", "h3", "p", "ul", "ol", "blockquote", "tagline", "hr")


@dataclass
class Block:
    kind: str
    text: str = ""
    items: List[str] = field(default_factory=list)  # for ul/ol


# ---------------------------------------------------------------------------
# Themes
# ---------------------------------------------------------------------------

THEMES = {
    "default": {
        "page_bg": "#ffffff",
        "text": "#1a1a2e",
        "h1_color": "#1a3a6e",
        "h2_color": "#1e4d91",
        "h3_color": "#2a6099",
        "accent": "#2a6099",
        "tagline_bg": "#eef3fb",
        "tagline_color": "#1a3a6e",
        "hr_color": "#c4d4e8",
        "link_color": "#2a6099",
        "body_font": "'Georgia', 'Times New Roman', serif",
        "heading_font": "'Helvetica Neue', 'Arial', sans-serif",
        "base_size": "10.5pt",
        "h1_size": "22pt",
        "h2_size": "15pt",
        "h3_size": "11.5pt",
    },
    "professional": {
        "page_bg": "#ffffff",
        "text": "#1c2333",
        "h1_color": "#0d1b3e",
        "h2_color": "#1a2f5e",
        "h3_color": "#263e6e",
        "accent": "#1a2f5e",
        "tagline_bg": "#e8ecf5",
        "tagline_color": "#0d1b3e",
        "hr_color": "#b0bccc",
        "link_color": "#1a2f5e",
        "body_font": "'Garamond', 'Georgia', serif",
        "heading_font": "'Palatino Linotype', 'Palatino', serif",
        "base_size": "10pt",
        "h1_size": "24pt",
        "h2_size": "16pt",
        "h3_size": "12pt",
    },
    "dark": {
        "page_bg": "#1e1e2e",
        "text": "#cdd6f4",
        "h1_color": "#cba6f7",
        "h2_color": "#89b4fa",
        "h3_color": "#74c7ec",
        "accent": "#89b4fa",
        "tagline_bg": "#313244",
        "tagline_color": "#cba6f7",
        "hr_color": "#45475a",
        "link_color": "#89b4fa",
        "body_font": "'Helvetica Neue', 'Arial', sans-serif",
        "heading_font": "'Helvetica Neue', 'Arial', sans-serif",
        "base_size": "10.5pt",
        "h1_size": "22pt",
        "h2_size": "15pt",
        "h3_size": "11.5pt",
    },
    "minimal": {
        "page_bg": "#ffffff",
        "text": "#000000",
        "h1_color": "#000000",
        "h2_color": "#111111",
        "h3_color": "#222222",
        "accent": "#000000",
        "tagline_bg": "#f5f5f5",
        "tagline_color": "#000000",
        "hr_color": "#999999",
        "link_color": "#000000",
        "body_font": "'Times New Roman', serif",
        "heading_font": "'Arial', sans-serif",
        "base_size": "10pt",
        "h1_size": "20pt",
        "h2_size": "14pt",
        "h3_size": "11pt",
    },
    "xcm": {
        "page_bg": "#0d0f1a",
        "text": "#d4deff",
        "h1_color": "#00e5ff",
        "h2_color": "#29b6f6",
        "h3_color": "#4fc3f7",
        "accent": "#00e5ff",
        "tagline_bg": "#131629",
        "tagline_color": "#00e5ff",
        "hr_color": "#1e2a45",
        "link_color": "#00e5ff",
        "body_font": "'Consolas', 'Menlo', 'monospace'",
        "heading_font": "'Helvetica Neue', 'Arial', sans-serif",
        "base_size": "10pt",
        "h1_size": "22pt",
        "h2_size": "15pt",
        "h3_size": "11.5pt",
    },
}


# ---------------------------------------------------------------------------
# Pattern helpers
# ---------------------------------------------------------------------------

_RE_BULLET = re.compile(r"^(\s*)[-*+\u2022\u2023\u25e6]\s+(.+)$")
_RE_ORDERED = re.compile(r"^(\s*)\d+[.)]\s+(.+)$")
_RE_INDENT = re.compile(r"^\s{3,}")
_RE_ENDS_SENTENCE = re.compile(r"[.!?\"]\s*$")
_RE_TITLE_CASE_WORD = re.compile(r"^[A-Z][a-z]")
_RE_ALL_CAPS_WORD = re.compile(r"^[A-Z][A-Z\s]+$")
_RE_ENDS_DASH_TAGLINE = re.compile(r".+\s+[-\u2013\u2014]+\s+.+$")


def _is_heading_candidate(line: str) -> bool:
    """True if a single stripped line looks like a heading rather than body text."""
    line = line.strip()
    if not line:
        return False
    word_count = len(line.split())
    if word_count > 18:
        return False
    if _RE_ENDS_SENTENCE.search(line) and word_count > 6:
        return False
    # All caps single word or short phrase
    if _RE_ALL_CAPS_WORD.match(line):
        return True
    # Title-cased (first word capitalized, relatively short)
    words = line.split()
    cap_words = sum(1 for w in words if _RE_TITLE_CASE_WORD.match(w))
    if cap_words >= max(1, len(words) - 1) and word_count <= 12:
        return True
    return False


def _heading_level_estimate(text: str, position: int, total_blocks: int) -> str:
    """
    Estimate whether a heading is h1, h2, or h3 based on text length,
    word count, and document position.
    """
    text = text.strip()
    words = text.split()
    word_count = len(words)
    char_count = len(text)

    # Very first heading is always h1
    if position == 0:
        return "h1"

    # Short, punchy, few words = higher level
    if word_count <= 4 and char_count <= 40:
        return "h2"
    if word_count <= 7 and char_count <= 60:
        return "h3"

    # Fallback based on char length
    if char_count <= 35:
        return "h2"
    return "h3"


# ---------------------------------------------------------------------------
# Parser: raw text -> Block list
# ---------------------------------------------------------------------------

def parse_text(text: str) -> List[Block]:
    """
    Parse raw plain text into a list of Block objects.

    Strategy:
    1. Split on blank lines to get raw paragraph chunks.
    2. Within each chunk check for bullet/ordered list lines.
    3. Classify each chunk as heading, paragraph, list, or blockquote.
    4. Assign heading levels (h1/h2/h3) using font-like heuristics.
    5. Detect tagline pattern at end of document.
    """
    try:
        blocks: List[Block] = []
        heading_count = 0
        last_was_body = False

        # Normalize line endings and split into raw chunks
        text = text.replace("\r\n", "\n").replace("\r", "\n")
        raw_chunks = re.split(r"\n{2,}", text.strip())

        total = len(raw_chunks)

        for chunk_idx, chunk in enumerate(raw_chunks):
            lines = [l for l in chunk.split("\n") if l.strip()]
            if not lines:
                continue

            # Check for bullet list
            if all(_RE_BULLET.match(l) for l in lines):
                items = [_RE_BULLET.match(l).group(2).strip() for l in lines]
                blocks.append(Block(kind="ul", items=items))
                last_was_body = True
                continue

            # Check for ordered list
            if all(_RE_ORDERED.match(l) for l in lines):
                items = [_RE_ORDERED.match(l).group(2).strip() for l in lines]
                blocks.append(Block(kind="ol", items=items))
                last_was_body = True
                continue

            # Mixed chunk: some bullet lines at start = treat as para + list
            bullet_lines = [l for l in lines if _RE_BULLET.match(l)]
            text_lines = [l for l in lines if not _RE_BULLET.match(l)]
            if bullet_lines and text_lines:
                # emit text lines first as a paragraph
                joined_text = " ".join(l.strip() for l in text_lines)
                blocks.append(Block(kind="p", text=joined_text))
                items = [_RE_BULLET.match(l).group(2).strip() for l in bullet_lines]
                blocks.append(Block(kind="ul", items=items))
                last_was_body = True
                continue

            # Indented block = blockquote
            if all(_RE_INDENT.match(l) for l in lines):
                joined = " ".join(l.strip() for l in lines)
                blocks.append(Block(kind="blockquote", text=joined))
                last_was_body = True
                continue

            # Single stripped line that is a heading candidate
            joined = " ".join(l.strip() for l in lines)

            # Tagline pattern: last or second-to-last chunk, ends with dash phrase
            is_last = chunk_idx >= total - 2
            if (is_last and _RE_ENDS_DASH_TAGLINE.match(joined)
                    and len(joined.split()) <= 12):
                blocks.append(Block(kind="tagline", text=joined))
                last_was_body = False
                continue

            # Multi-line chunk where first line is a heading candidate and
            # the rest is body text (common in plain-text docs with no blank
            # line between subheading and its paragraph).
            # Headings extracted this way are sub-level (h3) because they are
            # directly attached to their body text, unlike standalone section
            # headings which sit on their own blank-line-separated block.
            if len(lines) >= 2 and _is_heading_candidate(lines[0].strip()):
                first = lines[0].strip()
                rest = " ".join(l.strip() for l in lines[1:])
                # Force h3: a label glued to its paragraph is a sub-section
                level = "h3"
                blocks.append(Block(kind=level, text=first))
                heading_count += 1
                blocks.append(Block(kind="p", text=rest))
                last_was_body = True
                continue

            # Single line and looks like a heading
            if len(lines) == 1 and _is_heading_candidate(joined):
                level = _heading_level_estimate(joined, heading_count, total)
                # Insert implicit HR before h2 after body content
                if level == "h2" and last_was_body and blocks:
                    blocks.append(Block(kind="hr"))
                blocks.append(Block(kind=level, text=joined))
                heading_count += 1
                last_was_body = False
                continue

            # Default: paragraph
            blocks.append(Block(kind="p", text=joined))
            last_was_body = True

        return blocks

    except Exception as exc:
        print(f"[pdf_convert] parse_text error: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        # Fallback: return everything as paragraphs so output is never empty
        fallback = []
        for chunk in re.split(r"\n{2,}", text.strip()):
            stripped = chunk.strip()
            if stripped:
                fallback.append(Block(kind="p", text=stripped))
        return fallback


# ---------------------------------------------------------------------------
# CSS generation
# ---------------------------------------------------------------------------

def _build_css(t: dict) -> str:
    """Return the full embedded CSS string for a given theme dict."""
    return f"""
/* Base */
*, *::before, *::after {{ box-sizing: border-box; margin: 0; padding: 0; }}

body {{
  font-family: {t['body_font']};
  font-size: {t['base_size']};
  color: {t['text']};
  background: {t['page_bg']};
  line-height: 1.65;
  max-width: 720px;
  margin: 0 auto;
  padding: 2.4cm 2cm;
}}

/* Headings */
h1, h2, h3 {{
  font-family: {t['heading_font']};
  font-weight: 700;
  line-height: 1.2;
  margin-top: 0;
}}

h1 {{
  font-size: {t['h1_size']};
  color: {t['h1_color']};
  margin-bottom: 0.15em;
  letter-spacing: -0.02em;
  border-bottom: 2px solid {t['accent']};
  padding-bottom: 0.2em;
}}

h2 {{
  font-size: {t['h2_size']};
  color: {t['h2_color']};
  margin-bottom: 0.3em;
  margin-top: 1.4em;
  page-break-after: avoid;
}}

h3 {{
  font-size: {t['h3_size']};
  color: {t['h3_color']};
  margin-bottom: 0.15em;
  margin-top: 1em;
  page-break-after: avoid;
}}

/* Paragraphs */
p {{
  margin-bottom: 0.7em;
  orphans: 3;
  widows: 3;
}}

/* Lists */
ul, ol {{
  padding-left: 1.6em;
  margin-bottom: 0.7em;
}}

li {{
  margin-bottom: 0.25em;
  line-height: 1.55;
}}

/* Blockquote */
blockquote {{
  border-left: 3px solid {t['accent']};
  margin: 0.8em 0;
  padding: 0.4em 0.9em;
  font-style: italic;
  color: {t['h3_color']};
}}

/* Horizontal rule */
hr {{
  border: none;
  border-top: 1px solid {t['hr_color']};
  margin: 1.4em 0;
}}

/* Tagline */
.tagline {{
  margin-top: 1.8em;
  padding: 0.6em 1em;
  background: {t['tagline_bg']};
  color: {t['tagline_color']};
  font-weight: 600;
  font-family: {t['heading_font']};
  border-left: 3px solid {t['accent']};
  font-size: {t['h3_size']};
  page-break-inside: avoid;
}}

/* Links */
a {{
  color: {t['link_color']};
  text-decoration: underline;
}}

/* Print overrides */
@media print {{
  body {{
    padding: 0;
    max-width: 100%;
    -webkit-print-color-adjust: exact;
    print-color-adjust: exact;
  }}
  h1, h2, h3 {{
    page-break-after: avoid;
  }}
  p, li {{
    orphans: 3;
    widows: 3;
  }}
  .tagline {{
    page-break-inside: avoid;
  }}
}}
""".strip()


# ---------------------------------------------------------------------------
# HTML renderer
# ---------------------------------------------------------------------------

def blocks_to_html(blocks: List[Block], theme: str, title: str) -> str:
    """Convert a list of Block objects to a complete HTML document string."""
    try:
        t = THEMES.get(theme, THEMES["default"])
        css = _build_css(t)

        parts = [
            "<!DOCTYPE html>",
            '<html lang="en">',
            "<head>",
            '<meta charset="UTF-8">',
            '<meta name="viewport" content="width=device-width, initial-scale=1.0">',
            f"<title>{html_lib.escape(title)}</title>",
            "<style>",
            css,
            "</style>",
            "</head>",
            "<body>",
        ]

        for block in blocks:
            try:
                _render_block(block, parts)
            except Exception as exc:
                print(f"[pdf_convert] render error for block {block.kind!r}: {exc}",
                      file=sys.stderr)
                traceback.print_exc(file=sys.stderr)
                # Emit raw text so content is never lost
                parts.append(f"<p>{html_lib.escape(block.text)}</p>")

        parts.extend(["</body>", "</html>"])
        return "\n".join(parts)

    except Exception as exc:
        print(f"[pdf_convert] blocks_to_html fatal error: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        # Emergency fallback: plain HTML with all block text joined
        plain = "\n".join(
            html_lib.escape(b.text or " ".join(b.items)) for b in blocks
        )
        return f"<!DOCTYPE html><html><body><pre>{plain}</pre></body></html>"


def _render_block(block: Block, parts: List[str]) -> None:
    """Append HTML lines for a single block into parts."""
    e = html_lib.escape

    if block.kind == "h1":
        parts.append(f"<h1>{e(block.text)}</h1>")
    elif block.kind == "h2":
        parts.append(f"<h2>{e(block.text)}</h2>")
    elif block.kind == "h3":
        parts.append(f"<h3>{e(block.text)}</h3>")
    elif block.kind == "p":
        parts.append(f"<p>{e(block.text)}</p>")
    elif block.kind == "blockquote":
        parts.append(f"<blockquote>{e(block.text)}</blockquote>")
    elif block.kind == "tagline":
        parts.append(f'<div class="tagline">{e(block.text)}</div>')
    elif block.kind == "hr":
        parts.append("<hr>")
    elif block.kind == "ul":
        parts.append("<ul>")
        for item in block.items:
            parts.append(f"  <li>{e(item)}</li>")
        parts.append("</ul>")
    elif block.kind == "ol":
        parts.append("<ol>")
        for item in block.items:
            parts.append(f"  <li>{e(item)}</li>")
        parts.append("</ol>")
    else:
        # Unknown type -- fall back to paragraph
        content = block.text or " ".join(block.items)
        parts.append(f"<p>{e(content)}</p>")


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def convert(
    source_path: Optional[str] = None,
    text: Optional[str] = None,
    theme: str = "default",
    title: Optional[str] = None,
) -> str:
    """
    Convert a plain-text document to a print-ready HTML string.

    Args:
        source_path: Path to a .txt or text file. Mutually exclusive with text.
        text:        Raw document text. Mutually exclusive with source_path.
        theme:       Color theme key. One of: default, professional, dark, minimal, xcm.
        title:       HTML document title. If not given, inferred from first heading.

    Returns:
        Complete HTML string.

    Raises:
        ValueError: If neither source_path nor text is provided.
        OSError: If source_path cannot be read.
    """
    if source_path is None and text is None:
        raise ValueError("Provide either source_path or text.")

    if source_path is not None:
        try:
            with open(source_path, "r", encoding="utf-8", errors="replace") as fh:
                text = fh.read()
        except OSError as exc:
            print(f"[pdf_convert] cannot read {source_path}: {exc}", file=sys.stderr)
            raise

    if not text or not text.strip():
        print("[pdf_convert] warning: input text is empty", file=sys.stderr)
        return "<!DOCTYPE html><html><body><p>(empty document)</p></body></html>"

    try:
        if theme not in THEMES:
            print(
                f"[pdf_convert] unknown theme {theme!r}, falling back to 'default'",
                file=sys.stderr,
            )
            theme = "default"

        blocks = parse_text(text)

        if not blocks:
            print("[pdf_convert] warning: no blocks parsed from text", file=sys.stderr)
            blocks = [Block(kind="p", text=text.strip()[:2000])]

        # Infer title from first h1 block if not given
        if title is None:
            for b in blocks:
                if b.kind == "h1" and b.text:
                    title = b.text
                    break
            if title is None:
                title = (
                    os.path.splitext(os.path.basename(source_path))[0]
                    if source_path
                    else "Document"
                )

        return blocks_to_html(blocks, theme, title)

    except Exception as exc:
        print(f"[pdf_convert] convert error: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        # Absolute last resort: return wrapped raw text
        safe = html_lib.escape(text[:5000])
        ttl = html_lib.escape(title or "Document")
        return f"<!DOCTYPE html><html><head><title>{ttl}</title></head><body><pre>{safe}</pre></body></html>"


def list_themes() -> List[str]:
    """Return the list of available theme names."""
    return list(THEMES.keys())


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _cli() -> int:
    parser = argparse.ArgumentParser(
        description="Convert a plain-text document to print-ready HTML.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Themes: " + ", ".join(THEMES.keys()) + "\n\n"
            "Example:\n"
            "  python3 pdf_convert.py doc.txt --theme xcm --out report.html\n"
        ),
    )
    parser.add_argument("source", help="Input plain-text file")
    parser.add_argument(
        "--theme",
        default="default",
        choices=list(THEMES.keys()),
        help="Color theme (default: %(default)s)",
    )
    parser.add_argument(
        "--out",
        default=None,
        help="Output HTML file path. Defaults to <source>.html",
    )
    parser.add_argument(
        "--title",
        default=None,
        help="Override the HTML document title",
    )
    parser.add_argument(
        "--list-themes",
        action="store_true",
        help="Print available theme names and exit",
    )
    parser.add_argument(
        "--dump-blocks",
        action="store_true",
        help="Print the parsed block list and exit (useful for debugging structure detection)",
    )

    args = parser.parse_args()

    if args.list_themes:
        print("Available themes:")
        for name, t in THEMES.items():
            print(f"  {name:<14} bg={t['page_bg']}  text={t['text']}  accent={t['accent']}")
        return 0

    source = args.source
    if not os.path.isfile(source):
        print(f"[pdf_convert] file not found: {source}", file=sys.stderr)
        return 1

    try:
        with open(source, "r", encoding="utf-8", errors="replace") as fh:
            raw_text = fh.read()
    except OSError as exc:
        print(f"[pdf_convert] cannot read {source}: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        return 1

    if args.dump_blocks:
        try:
            blocks = parse_text(raw_text)
            print(f"Parsed {len(blocks)} block(s):\n")
            for i, b in enumerate(blocks):
                if b.items:
                    print(f"  [{i:02d}] {b.kind:<12}  {b.items[:2]} ...")
                else:
                    print(f"  [{i:02d}] {b.kind:<12}  {b.text[:80]!r}")
        except Exception as exc:
            print(f"[pdf_convert] dump-blocks error: {exc}", file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            return 1
        return 0

    try:
        html = convert(text=raw_text, theme=args.theme, title=args.title)
    except Exception as exc:
        print(f"[pdf_convert] conversion failed: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        return 1

    out_path = args.out or (os.path.splitext(source)[0] + ".html")

    try:
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write(html)
        print(f"[pdf_convert] wrote {out_path}  ({len(html):,} bytes)  theme={args.theme}")
    except OSError as exc:
        print(f"[pdf_convert] cannot write {out_path}: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    try:
        sys.exit(_cli())
    except KeyboardInterrupt:
        sys.stderr.write("\n[pdf_convert] interrupted\n")
        sys.exit(130)
    except Exception as exc:
        sys.stderr.write(f"[pdf_convert] fatal: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)
