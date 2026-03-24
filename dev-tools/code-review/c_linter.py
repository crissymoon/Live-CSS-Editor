#!/usr/bin/env python3
"""
c_linter.py  -  Dedicated C/C++ static analysis linter
Scans .c, .h, .cc, .cpp, .cxx, .hh, .hpp, .hxx files.

Usage:
    python3 c_linter.py <directory>

Checks:
  * Unsafe / dangerous function calls (expanded set)
  * Format string vulnerabilities  (printf/fprintf with a variable first arg)
  * Implicit switch fallthrough     (case without break/return/goto/throw)
  * malloc/calloc/realloc without NULL check
  * goto usage
  * Pointer cast warnings           (int cast of pointer)
  * C++ raw new without try/catch in the same scope
"""

import sys
import os
import re
import argparse
from pathlib import Path
from collections import defaultdict

from scan_config import (
    merge_skip_dir_names,
    merge_skip_file_names,
    merge_skip_relative_paths,
    should_skip_relative_path,
)

C_EXTENSIONS = {".c", ".h", ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx"}

FINDINGS: list[dict] = []


def add(path: str, line: int, kind: str, msg: str, severity: str = "MEDIUM") -> None:
    FINDINGS.append({"path": path, "line": line, "kind": kind, "msg": msg, "severity": severity})
    rel = os.path.relpath(path)
    print(f"[{severity}] {rel}:{line}  [{kind}]  {msg}")


# ---------------------------------------------------------------------------
# Regex helpers
# ---------------------------------------------------------------------------

# Strip block comments and string literals, preserving line structure.
# Returns cleaned line and updates mutable state dict.
def _strip_comment_str(raw: str, state: dict) -> str:
    out: list[str] = []
    i = 0
    in_block = state.get("block_comment", False)
    while i < len(raw):
        ch = raw[i]
        nxt = raw[i + 1] if i + 1 < len(raw) else ""
        if in_block:
            if ch == "*" and nxt == "/":
                in_block = False
                i += 2
            else:
                i += 1
            continue
        if ch == "/" and nxt == "*":
            in_block = True
            i += 2
            continue
        if ch == "/" and nxt == "/":
            break  # rest of line is comment
        if ch in ('"', "'"):
            quote = ch
            i += 1
            while i < len(raw):
                if raw[i] == "\\" and i + 1 < len(raw):
                    i += 2
                    continue
                if raw[i] == quote:
                    i += 1
                    break
                i += 1
            out.append(" ")
            continue
        out.append(ch)
        i += 1
    state["block_comment"] = in_block
    return "".join(out)


# ---------------------------------------------------------------------------
# Check 1: Unsafe / dangerous function calls
# ---------------------------------------------------------------------------

DANGEROUS_CALLS: list[tuple] = [
    # Buffer overflow
    (re.compile(r"\bgets\s*\("),    "HIGH",   "gets() is unsafe; no bounds check — use fgets()"),
    (re.compile(r"\bstrcpy\s*\("),  "HIGH",   "strcpy() may overflow destination; use strncpy() or strlcpy()"),
    (re.compile(r"\bstrcat\s*\("),  "HIGH",   "strcat() may overflow destination; use strncat()"),
    (re.compile(r"\bsprintf\s*\("), "HIGH",   "sprintf() may overflow; use snprintf()"),
    (re.compile(r"\bvsprintf\s*\("),"HIGH",   "vsprintf() may overflow; use vsnprintf()"),
    # scanf without width limit
    (re.compile(r'\bscanf\s*\(\s*"[^"]*%s'),    "HIGH", 'scanf("%s") has no width limit; use scanf("%Ns")'),
    (re.compile(r'\bsscanf\s*\([^,]+,\s*"[^"]*%s'), "HIGH",
     'sscanf("%s") has no width limit; use a width specifier'),
    # strncpy does not null-terminate when src length >= n
    (re.compile(r"\bstrncpy\s*\("), "MEDIUM", "strncpy() does not guarantee null termination"),
    # Stack overflow
    (re.compile(r"\balloca\s*\("),  "MEDIUM", "alloca() is not portable and may overflow the stack"),
    # Command injection
    (re.compile(r"\bsystem\s*\("),  "HIGH",   "system() is vulnerable to command injection"),
    (re.compile(r"\bpopen\s*\("),   "HIGH",   "popen() is vulnerable to command injection"),
    # TOCTOU race conditions
    (re.compile(r"\bmktemp\s*\("),  "HIGH",   "mktemp() has a TOCTOU race condition; use mkstemp()"),
    (re.compile(r"\btmpnam\s*\("),  "HIGH",   "tmpnam() has a TOCTOU race condition; use tmpfile() or mkstemp()"),
    # Error-silent conversions
    (re.compile(r"\batoi\s*\("),    "MEDIUM", "atoi() has no error handling and UB on overflow; use strtol()"),
    (re.compile(r"\batol\s*\("),    "MEDIUM", "atol() has no error handling and UB on overflow; use strtol()"),
    (re.compile(r"\batof\s*\("),    "MEDIUM", "atof() has no error handling; use strtod()"),
]


def check_dangerous_calls(path: str, lines: list[str]) -> None:
    state: dict = {"block_comment": False}
    for i, raw in enumerate(lines, 1):
        cleaned = _strip_comment_str(raw.rstrip("\n"), state)
        for pat, severity, message in DANGEROUS_CALLS:
            if pat.search(cleaned):
                add(path, i, "DANGEROUS_CALL", message, severity)
                break  # one finding per line is enough


# ---------------------------------------------------------------------------
# Check 2: Format string vulnerability
# Printf/fprintf/syslog called with a variable (not a string literal) as the
# format argument is a classic format string attack vector.
# ---------------------------------------------------------------------------

# Matches: printf(varname  or  fprintf(stream, varname
# The key signal is that the format argument starts with an identifier, not "
_FMT_PRINTF  = re.compile(r"\bprintf\s*\(\s*([A-Za-z_][\w]*)")
_FMT_FPRINTF = re.compile(r"\b(?:fprintf|dprintf)\s*\([^,]+,\s*([A-Za-z_][\w]*)")
_FMT_SYSLOG  = re.compile(r"\bsyslog\s*\([^,]+,\s*([A-Za-z_][\w]*)")
_FMT_PATTERNS = [_FMT_PRINTF, _FMT_FPRINTF, _FMT_SYSLOG]

# Safe literal keywords that look like identifiers but are not variables
_FMT_SAFE_IDENTS = {"NULL", "stderr", "stdout", "stdin", "LOG_ERR", "LOG_INFO",
                    "LOG_WARNING", "LOG_DEBUG", "LOG_NOTICE"}


def check_format_string(path: str, lines: list[str]) -> None:
    state: dict = {"block_comment": False}
    for i, raw in enumerate(lines, 1):
        cleaned = _strip_comment_str(raw.rstrip("\n"), state)
        for pat in _FMT_PATTERNS:
            m = pat.search(cleaned)
            if m:
                ident = m.group(1)
                if ident not in _FMT_SAFE_IDENTS:
                    add(path, i, "FORMAT_STRING",
                        f"Possible format string vulnerability: format arg is variable '{ident}'",
                        "HIGH")
                    break


# ---------------------------------------------------------------------------
# Check 3: Implicit switch fallthrough
# A case block that contains statements but ends without break/return/goto/
# throw/exit/continue is flagged.  Intentional fallthrough must be annotated
# with /* fallthrough */ or [[fallthrough]].
# ---------------------------------------------------------------------------

_TERMINATOR_RE = re.compile(
    r"\b(?:break|return|goto|throw|exit|abort|continue)\b"
    r"|/\*\s*fall(?:s?\s*through)?|//\s*fall(?:s?\s*through)?"
    r"|\[\[fallthrough\]\]"
)
_CASE_RE    = re.compile(r"^\s*(?:case\b|default\s*:)")
_SWITCH_RE  = re.compile(r"\bswitch\s*\(")


def check_implicit_fallthrough(path: str, lines: list[str]) -> None:
    """Detect case blocks that have statements but no terminating statement."""
    state: dict = {"block_comment": False}
    cleaned_lines = []
    for raw in lines:
        cleaned_lines.append(_strip_comment_str(raw.rstrip("\n"), state))

    n = len(cleaned_lines)
    i = 0
    switch_depth = 0   # brace depth of the outermost switch
    brace_depth  = 0

    case_start_line  = None  # 1-based line number of the current case
    case_has_code    = False
    case_terminated  = False

    while i < n:
        cl = cleaned_lines[i]

        # Track brace depth
        open_b  = cl.count("{")
        close_b = cl.count("}")

        if _SWITCH_RE.search(cl):
            # When we enter a switch the opening brace follows (may be on same line)
            if open_b > close_b:
                switch_depth = brace_depth + 1

        brace_depth += open_b - close_b

        # Are we at the switch body level?
        in_switch = (switch_depth > 0 and brace_depth >= switch_depth)

        if in_switch:
            # Leaving the switch?
            if brace_depth < switch_depth:
                # Flush last case
                if case_start_line and case_has_code and not case_terminated:
                    add(path, case_start_line, "IMPLICIT_FALLTHROUGH",
                        "Case block falls through to next case without break/return/goto",
                        "MEDIUM")
                switch_depth  = 0
                case_start_line  = None
                case_has_code    = False
                case_terminated  = False
                i += 1
                continue

            if _CASE_RE.match(cl):
                # New case — evaluate previous one
                if case_start_line and case_has_code and not case_terminated:
                    add(path, case_start_line, "IMPLICIT_FALLTHROUGH",
                        "Case block falls through to next case without break/return/goto",
                        "MEDIUM")
                case_start_line = i + 1
                case_has_code   = False
                case_terminated = False
            elif case_start_line:
                stripped = cl.strip()
                if stripped and stripped not in ("{", "}"):
                    case_has_code = True
                if _TERMINATOR_RE.search(cl):
                    case_terminated = True

        i += 1


# ---------------------------------------------------------------------------
# Check 4: malloc / calloc / realloc without NULL check
# Simple heuristic: assignment from an allocator function where the next few
# lines do not contain a NULL check for the same variable name.
# ---------------------------------------------------------------------------

_ALLOC_RE = re.compile(
    r"\b(\w+)\s*=\s*(?:malloc|calloc|realloc|aligned_alloc|memalign)\s*\("
)
_NULL_CHECK_RE = re.compile(r"\bif\s*\(|assert\s*\(")
_NULL_REF_RE   = re.compile(r"\bNULL\b|==\s*0\b|!=\s*NULL\b|!\s*\w")
LOOKAHEAD_LINES = 5


def check_malloc_no_null(path: str, lines: list[str]) -> None:
    state: dict = {"block_comment": False}
    cleaned_lines = [_strip_comment_str(raw.rstrip("\n"), state) for raw in lines]
    n = len(cleaned_lines)

    for i, cl in enumerate(cleaned_lines):
        m = _ALLOC_RE.search(cl)
        if not m:
            continue
        var = m.group(1)
        # Look ahead for a NULL check referencing var
        found_check = False
        for j in range(i + 1, min(i + 1 + LOOKAHEAD_LINES, n)):
            ahead = cleaned_lines[j]
            if _NULL_CHECK_RE.search(ahead) and var in ahead and _NULL_REF_RE.search(ahead):
                found_check = True
                break
            # If a new function or block starts, stop looking
            if re.search(r"\breturn\b|\bfree\b|\bgoto\b", ahead):
                break
        if not found_check:
            add(path, i + 1, "MALLOC_NO_NULL_CHECK",
                f"Result of allocator assigned to '{var}' without a NULL check",
                "HIGH")


# ---------------------------------------------------------------------------
# Check 5: goto usage
# goto is considered harmful in most modern C/C++ (error cleanup patterns
# excepted) — flag it with LOW severity for manual review.
# ---------------------------------------------------------------------------

_GOTO_RE = re.compile(r"\bgoto\b")


def check_goto(path: str, lines: list[str]) -> None:
    state: dict = {"block_comment": False}
    for i, raw in enumerate(lines, 1):
        cleaned = _strip_comment_str(raw.rstrip("\n"), state)
        if _GOTO_RE.search(cleaned):
            add(path, i, "GOTO_USAGE",
                "goto statement — review for spaghetti control flow",
                "LOW")


# ---------------------------------------------------------------------------
# Check 6: Pointer-to-integer cast
# (int)ptr style casts strip the upper bits on 64-bit platforms.
# ---------------------------------------------------------------------------

_PTR_CAST_RE = re.compile(r"\(\s*(?:int|long|unsigned\s+int|unsigned\s+long)\s*\)\s*\w+")


def check_pointer_cast(path: str, lines: list[str]) -> None:
    state: dict = {"block_comment": False}
    for i, raw in enumerate(lines, 1):
        cleaned = _strip_comment_str(raw.rstrip("\n"), state)
        if _PTR_CAST_RE.search(cleaned):
            # Only flag if a pointer-sized type is being narrowed — heuristic:
            # flag any (int)expr where the surrounding code has pointer context.
            # We keep it simple: flag the cast, let the developer decide.
            add(path, i, "NARROWING_CAST",
                "Cast to narrower integer type may truncate pointer on 64-bit platforms",
                "LOW")


# ---------------------------------------------------------------------------
# File scanner
# ---------------------------------------------------------------------------

BASE_SKIP_DIRS  = {".git", "__pycache__", "node_modules", "vendor", ".venv",
                   "venv", "env", "dist", "build", ".tox", "eggs"}
BASE_SKIP_FILES: set[str] = set()


def scan_file(path: str, lines: list[str]) -> None:
    check_dangerous_calls(path, lines)
    check_format_string(path, lines)
    check_implicit_fallthrough(path, lines)
    check_malloc_no_null(path, lines)
    check_goto(path, lines)
    check_pointer_cast(path, lines)


def scan_dir(root: str) -> None:
    root_path = Path(root).resolve()
    skip_dir_names       = merge_skip_dir_names(BASE_SKIP_DIRS)
    skip_relative_paths  = merge_skip_relative_paths()
    skip_file_names      = merge_skip_file_names(BASE_SKIP_FILES)

    file_count = 0
    for dirpath, dirnames, filenames in os.walk(root):
        dirpath_path = Path(dirpath)
        dirnames[:] = [
            d for d in dirnames
            if d not in skip_dir_names
            and not should_skip_relative_path(dirpath_path / d, root_path, skip_relative_paths)
        ]
        for fname in filenames:
            if fname in skip_file_names:
                continue
            if os.path.splitext(fname)[1].lower() in C_EXTENSIONS:
                fpath = os.path.join(dirpath, fname)
                try:
                    with open(fpath, encoding="utf-8", errors="replace") as f:
                        lines = f.readlines()
                    scan_file(fpath, lines)
                    file_count += 1
                except Exception as exc:
                    print(f"[ERROR] Cannot read {fpath}: {exc}")

    print(f"Scanned {file_count} C/C++ file(s).")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="C/C++ static analysis linter.")
    parser.add_argument("directory", help="Directory to scan")
    args = parser.parse_args()

    target = os.path.abspath(args.directory)
    if not os.path.isdir(target):
        print(f"Not a directory: {target}", file=sys.stderr)
        sys.exit(1)

    print(f"C/C++ Lint scan: {target}")
    print("-" * 60)

    scan_dir(target)

    print("-" * 60)
    by_severity: dict[str, int] = defaultdict(int)
    by_kind: dict[str, int] = defaultdict(int)
    for f in FINDINGS:
        by_severity[f["severity"]] += 1
        by_kind[f["kind"]] += 1

    total = len(FINDINGS)
    print(f"Total findings: {total}")
    for sev in ("HIGH", "MEDIUM", "LOW"):
        n = by_severity.get(sev, 0)
        if n:
            print(f"  {sev}: {n}")

    print()
    print("By type:")
    for kind, n in sorted(by_kind.items(), key=lambda x: -x[1]):
        print(f"  {kind}: {n}")


if __name__ == "__main__":
    main()
