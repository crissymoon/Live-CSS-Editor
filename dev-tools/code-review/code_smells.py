#!/usr/bin/env python3
"""
code_smells.py  -  Code Smell Detector
Scans .py, .php, .js, .jsx, .ts, .tsx, .c, .h, .cc, .cpp, .cxx, .hh, .hpp,
.hxx, and .go files.

Usage:
    python3 code_smells.py <directory>

Detected smells:
  * Long parameter lists   (> 5 params in function signature)
  * Deep nesting           (> 4 levels of indent)
  * Duplicate code blocks  (>= 6 identical consecutive lines appear 2+ times)
    * Orphaned code          (top-level symbols that appear unreferenced in the scanned tree)
    * C memory safety        (unsafe C/C++ APIs like gets/strcpy/sprintf)
  * Primitive obsession    (> 4 primitive params in one function)
  * Dead imports           (imported name never used in rest of file)
  * Long methods           (function body > 60 lines)
  * Magic numbers          (bare numeric literals in code, not in declarations)
  * Empty catch blocks     (catch/except with only pass or comment)
"""

import sys
import os
import re
import argparse
from pathlib import Path
from collections import defaultdict

from scan_config import merge_skip_dir_names, merge_skip_file_names, merge_skip_relative_paths, should_skip_relative_path

EXTENSIONS = {
    ".py", ".php", ".js", ".jsx", ".ts", ".tsx", ".c", ".h",
    ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx", ".go",
}
MAX_PARAMS          = 5
MAX_NESTING         = 4
DUP_BLOCK_SIZE      = 6      # lines in a block to consider duplicates
MAX_PRIMITIVE_PARAMS= 4
MAX_METHOD_LINES    = 60

PRIMITIVES = {"int", "str", "float", "bool", "string", "number", "char",
              "double", "long", "short", "unsigned"}
C_EXTENSIONS = {".c", ".h", ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx"}

FINDINGS: list[dict] = []
ACTIVE_KIND_FILTER: str | None = None

ORPHAN_EXEMPT_NAMES = {
    "main", "init", "setup", "setUp", "teardown", "tearDown",
    "bootstrap", "boot", "index",
}


def add(path: str, line: int, kind: str, msg: str, severity: str = "MEDIUM") -> None:
    if ACTIVE_KIND_FILTER and kind != ACTIVE_KIND_FILTER:
        return
    FINDINGS.append({"path": path, "line": line, "kind": kind, "msg": msg, "severity": severity})
    rel = os.path.relpath(path)
    print(f"[{severity}] {rel}:{line}  [{kind}]  {msg}")


# ------------------------------------------------------------------
# Smell detectors
# ------------------------------------------------------------------

# Regex patterns
PAT_PY_DEF   = re.compile(r"^\s*def\s+\w+\s*\(([^)]*)\)")
PAT_JS_FUNC  = re.compile(r"(?:function\s+\w+\s*\(|(?:const|let|var)\s+\w+\s*=\s*(?:async\s*)?\()\s*([^)]*)\)")
PAT_PHP_FUNC = re.compile(r"function\s+\w+\s*\(([^)]*)\)")
PAT_C_FUNC   = re.compile(r"\w[\w\s\*]+\s+\w+\s*\(([^)]*)\)\s*\{")
PAT_IMPORT_PY= re.compile(r"^(?:import|from)\s+(\.{0,3}[\w.]*)")
PAT_MAGIC    = re.compile(r"(?<![.\w'\"])(\d{2,})(?![.\w'\"])")
PAT_TOKEN    = re.compile(r"[A-Za-z_$][A-Za-z0-9_$]*")
# Matches the entire content of a string literal (single or double quoted, with escapes)
# Used to blank out string contents before the magic-number scan.
_STR_CONTENT_RE = re.compile(r'"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\'')

PAT_ORPHAN_PY = re.compile(r"^(def|class)\s+([A-Za-z_]\w*)\b")
PAT_ORPHAN_PHP_FUNC = re.compile(r"^(?:final\s+|abstract\s+)?function\s+([A-Za-z_]\w*)\s*\(")
PAT_ORPHAN_PHP_TYPE = re.compile(r"^(?:final\s+|abstract\s+)?(?:class|interface|trait)\s+([A-Za-z_]\w*)\b")
PAT_ORPHAN_JS_FUNC = re.compile(r"^(?:export\s+default\s+|export\s+)?(?:async\s+)?function\s+([A-Za-z_$][\w$]*)\s*\(")
PAT_ORPHAN_JS_VAR = re.compile(
    r"^(?:export\s+)?(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*"
    r"(?:async\s*)?(?:function\b|\([^)]*\)\s*=>|[A-Za-z_$][\w$]*\s*=>)"
)
PAT_ORPHAN_JS_CLASS = re.compile(r"^(?:export\s+default\s+|export\s+)?(?:abstract\s+)?class\s+([A-Za-z_$][\w$]*)\b")
PAT_ORPHAN_GO_FUNC = re.compile(r"^func\s+([A-Za-z_]\w*)\s*\(")
PAT_ORPHAN_GO_METHOD = re.compile(r"^func\s*\([^)]*\)\s*([A-Za-z_]\w*)\s*\(")
PAT_ORPHAN_GO_TYPE = re.compile(r"^type\s+([A-Za-z_]\w*)\b")
PAT_ORPHAN_C_STATIC_FUNC = re.compile(
    r"^(?:static\s+)(?:inline\s+)?[\w:\<\>\~\*&\s]+\b([A-Za-z_]\w*)\s*\([^;]*\)\s*(?:const\s*)?\{"
)

UNSAFE_C_CALLS = [
    (re.compile(r"\bgets\s*\("), "HIGH", "Use of gets() is unsafe and can cause buffer overflows"),
    (re.compile(r"\bstrcpy\s*\("), "MEDIUM", "Use of strcpy() may overflow destination buffers"),
    (re.compile(r"\bstrcat\s*\("), "MEDIUM", "Use of strcat() may overflow destination buffers"),
    (re.compile(r"\bsprintf\s*\("), "HIGH", "Use of sprintf() is unsafe; prefer snprintf()"),
    (re.compile(r"\bvsprintf\s*\("), "HIGH", "Use of vsprintf() is unsafe; prefer vsnprintf()"),
]


def _split_params(sig: str) -> list[str]:
    """Split a parameter string by top-level commas, respecting nested brackets.

    Prevents generic type annotations like Dict[str, int] from being counted
    as two separate parameters.
    """
    result: list[str] = []
    depth = 0
    current: list[str] = []
    for ch in sig:
        if ch in "([{":
            depth += 1
            current.append(ch)
        elif ch in ")]}":
            depth -= 1
            current.append(ch)
        elif ch == "," and depth == 0:
            part = "".join(current).strip()
            if part:
                result.append(part)
            current = []
        else:
            current.append(ch)
    part = "".join(current).strip()
    if part:
        result.append(part)
    return result


def count_params(sig: str) -> tuple[int, int]:
    """Return (total_params, primitive_params)."""
    if not sig.strip():
        return 0, 0
    params = _split_params(sig)
    # Strip type annotations / defaults
    primitives = 0
    for p in params:
        p_clean = p.split("=")[0].strip()
        # Split on ":" for typed params (Python/TS "name: type") and on
        # whitespace for positional type-before-name style (C/PHP "type name").
        # Checking every token in the cleaned param string is sufficient;
        # the old separate "hint" check caused double-counting for typed params.
        words = p_clean.split()
        for w in words:
            if w.lower().rstrip(":") in PRIMITIVES:
                primitives += 1
                break
    return len(params), primitives


def check_params_and_nesting(path: str, lines: list[str]) -> None:
    # Choose patterns based on extension
    ext = os.path.splitext(path)[1].lower()
    if ext == ".py":
        func_pat = PAT_PY_DEF
    elif ext == ".php":
        func_pat = PAT_PHP_FUNC
    elif ext in (".js", ".jsx", ".ts", ".tsx"):
        func_pat = PAT_JS_FUNC
    else:
        func_pat = PAT_C_FUNC

    func_start = None
    func_line_count = 0

    # Track triple-quoted string state (Python only) to avoid flagging
    # indented string content as DEEP_NESTING.
    triple_quote_delim: str | None = None

    for i, raw in enumerate(lines, 1):
        # Expand tabs to spaces before measuring indent so tab-indented
        # files are treated the same as space-indented files.
        expanded = raw.expandtabs(4)
        indent = len(expanded) - len(expanded.lstrip())
        indent_level = indent // 4  # assume 4-space / tab=4 indent

        # For Python, skip lines that are inside a triple-quoted string.
        if ext == ".py":
            stripped_raw = raw.strip()
            for delim in ('"""', "'''"):
                # Odd number of delimiter occurrences on this line toggles state.
                if stripped_raw.count(delim) % 2 == 1:
                    if triple_quote_delim == delim:
                        triple_quote_delim = None   # closing delimiter
                    elif triple_quote_delim is None:
                        triple_quote_delim = delim  # opening delimiter
                    break
            if triple_quote_delim is not None and not any(
                    raw.strip().startswith(d) for d in ('"""', "'''")):
                # Inside a multi-line string body — skip structural checks.
                if ext == ".py" and func_start is not None:
                    func_line_count += 1
                continue

        if indent_level > MAX_NESTING:
            add(path, i, "DEEP_NESTING",
                f"Nesting depth ~{indent_level} (max {MAX_NESTING})", "HIGH")

        # Long method tracking (Python only - indentation-based)
        if ext == ".py":
            if PAT_PY_DEF.match(raw):
                if func_start is not None and func_line_count > MAX_METHOD_LINES:
                    add(path, func_start, "LONG_METHOD",
                        f"Method body ~{func_line_count} lines (max {MAX_METHOD_LINES})", "MEDIUM")
                func_start = i
                func_line_count = 0
            elif func_start is not None:
                func_line_count += 1

        # Param count
        m = func_pat.search(raw)
        if m:
            sig = m.group(1)
            total, prims = count_params(sig)
            if total > MAX_PARAMS:
                add(path, i, "LONG_PARAM_LIST",
                    f"{total} parameters (max {MAX_PARAMS})", "MEDIUM")
            if prims > MAX_PRIMITIVE_PARAMS:
                add(path, i, "PRIMITIVE_OBSESSION",
                    f"{prims} primitive-type parameters (max {MAX_PRIMITIVE_PARAMS})", "LOW")

    # Flush last method
    if func_start is not None and func_line_count > MAX_METHOD_LINES:
        add(path, func_start, "LONG_METHOD",
            f"Method body ~{func_line_count} lines (max {MAX_METHOD_LINES})", "MEDIUM")


def _blank_strings(line: str) -> str:
    """Replace the *contents* of string literals with spaces.

    This prevents numbers that appear inside strings from being flagged as
    magic numbers.  The surrounding quotes are preserved so the character
    offsets stay correct for other checks.
    """
    return _STR_CONTENT_RE.sub(lambda m: m.group()[0] + " " * (len(m.group()) - 2) + m.group()[-1], line)


def check_magic_numbers(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    for i, raw in enumerate(lines, 1):
        stripped = raw.strip()
        # Skip comments, strings, declarations/constants
        if stripped.startswith(("#", "//", "*", "/*", "'")):
            continue
        if "=" in stripped:
            # Strip optional type annotation from LHS: "MAX_VAL: int = 5" -> "MAX_VAL"
            lhs = stripped.split("=")[0].strip()
            lhs_name = lhs.split(":")[0].strip()
            # Skip any simple identifier assignment (UPPER_CASE constant OR
            # lowercase named variable). Magic numbers in logic expressions
            # like "if x > 30:" are still caught because they have no "name = "
            # LHS that is a plain identifier.
            if lhs_name and re.match(r'^[A-Za-z_]\w*$', lhs_name):
                continue
        for m in PAT_MAGIC.finditer(_blank_strings(raw)):
            val = int(m.group(1))
            if val in (0, 1, 2, 100, 200, 404, 500):  # common acceptable literals
                continue
            add(path, i, "MAGIC_NUMBER",
                f"Bare numeric literal {val}", "LOW")
            break  # one per line is enough


def check_empty_catch(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    i = 0
    while i < len(lines):
        raw = lines[i].strip()
        # Python except
        if re.match(r"except[\s(:]", raw):
            # Inline body: "except E as x: statement" — not empty
            colon_pos = raw.find(":")
            if colon_pos != -1:
                inline = raw[colon_pos + 1:].strip()
                if inline and not inline.startswith("#"):
                    i += 1
                    continue
            # look ahead for body
            j = i + 1
            body_lines = []
            while j < len(lines) and (not lines[j].strip() or
                  len(lines[j]) - len(lines[j].lstrip()) > len(lines[i]) - len(lines[i].lstrip())):
                s = lines[j].strip()
                if s:
                    body_lines.append(s)
                j += 1
            if not body_lines or all(b in ("pass", "...") or b.startswith("#") for b in body_lines):
                add(path, i + 1, "EMPTY_CATCH", "Empty except block (swallowed exception)", "HIGH")
        # JS/PHP catch
        if re.match(r"}\s*catch\s*\(|catch\s*\(", raw):
            # Inline body: "} catch (e) { code; }" — not empty
            brace_open = raw.find("{")
            if brace_open != -1:
                inline = raw[brace_open + 1:].rstrip("}").strip()
                if inline and not inline.startswith("//"):
                    i += 1
                    continue
            j = i + 1
            brace_depth = 0
            found_code = False
            while j < len(lines):
                s = lines[j].strip()
                brace_depth += s.count("{") - s.count("}")
                if s and s not in ("{", "}") and not s.startswith("//"):
                    found_code = True
                    break
                if brace_depth <= 0:
                    break
                j += 1
            if not found_code:
                add(path, i + 1, "EMPTY_CATCH", "Empty catch block (swallowed exception)", "HIGH")
        i += 1


def _collect_import_names(tail: str, lineno: int, lines: list[str], idx: int,
                          imports: dict[str, int]) -> int:
    """Parse the name list from a 'from X import ...' statement.

    Handles parenthesised single-line `(A, B)`, multi-line `(\\n  A,\\n  B,\\n)`,
    and inline comments `A, B  # noqa`.  Returns the updated line index (0-based).
    """
    # Strip trailing inline comment from the tail portion
    tail = re.sub(r'\s*#.*$', '', tail).strip()

    if tail.startswith("(") and ")" not in tail:
        # Multi-line parenthesised import: accumulate continuation lines
        tail = tail.lstrip("(")
        idx += 1
        while idx < len(lines):
            cont = re.sub(r'\s*#.*$', '', lines[idx].strip())
            if ")" in cont:
                tail += "," + cont[:cont.index(")")]
                break
            tail += "," + cont
            idx += 1
    else:
        # Strip outer parens for single-line `(A, B)` style
        tail = tail.strip("()")

    for part in tail.split(","):
        n = part.strip().split(" as ")[-1].strip("() ")
        if n and n != "*":
            imports[n] = lineno

    return idx


def check_dead_imports(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    if ext != ".py":
        return
    imports: dict[str, int] = {}
    idx = 0
    while idx < len(lines):
        stripped = lines[idx].strip()
        lineno = idx + 1
        m = PAT_IMPORT_PY.match(stripped)
        if m:
            if stripped.startswith("from"):
                rest = re.search(r"import\s+(.+)", stripped)
                if rest:
                    idx = _collect_import_names(rest.group(1), lineno, lines, idx, imports)
            else:
                # "import X as alias" → the usable name is the alias, not X.
                as_m = re.match(r"import\s+[\w.]+\s+as\s+(\w+)", stripped)
                name = as_m.group(1) if as_m else m.group(1).split(".")[-1]
                imports[name] = lineno
        idx += 1

    # Check usages in rest of file
    full_text = "\n".join(lines)
    for name, lineno in imports.items():
        # Count occurrences beyond the import line itself
        uses = len(re.findall(r'\b' + re.escape(name) + r'\b', full_text))
        if uses <= 1:  # only the import line itself
            add(path, lineno, "DEAD_IMPORT", f"'{name}' imported but never used", "LOW")


def check_duplicate_blocks(path: str, lines: list[str]) -> None:
    """Find duplicate blocks of DUP_BLOCK_SIZE or more non-blank lines."""
    # Build normalised line list (strip whitespace for comparison)
    norm = [ln.strip() for ln in lines]
    # Create all blocks of size DUP_BLOCK_SIZE
    seen: dict[tuple, int] = {}
    # Track the end of the last reported duplicate range so overlapping
    # sliding-window positions for the same duplication are not reported again.
    next_report_i = 0
    for i in range(len(norm) - DUP_BLOCK_SIZE + 1):
        block = tuple(norm[i : i + DUP_BLOCK_SIZE])
        if all(b == "" for b in block):
            continue
        if block in seen:
            first_line = seen[block]
            if i >= next_report_i:
                add(path, i + 1, "DUPLICATE_CODE",
                    f"Block of {DUP_BLOCK_SIZE} lines duplicates lines {first_line}-{first_line + DUP_BLOCK_SIZE - 1}",
                    "MEDIUM")
                next_report_i = i + DUP_BLOCK_SIZE
        else:
            seen[block] = i + 1


def check_c_memory_safety(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    if ext not in C_EXTENSIONS:
        return

    state = {"block_comment": False}
    for i, raw in enumerate(lines, 1):
        cleaned = strip_comments_and_strings(raw.rstrip("\n"), ext, state)
        if not cleaned.strip():
            continue
        for pat, severity, message in UNSAFE_C_CALLS:
            if pat.search(cleaned):
                add(path, i, "C_MEMORY_UNSAFE", message, severity)
                break


def strip_comments_and_strings(raw: str, ext: str, state: dict[str, bool]) -> str:
    """Strip comments/strings enough to track brace depth and definitions."""
    if ext == ".py":
        return raw

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
            break
        if ext == ".php" and ch == "#":
            break

        if ch in ('"', "'", "`"):
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


def extract_symbol_defs(path: str, lines: list[str]) -> list[dict]:
    """Extract conservative orphan-code candidates from one file."""
    ext = os.path.splitext(path)[1].lower()
    defs: list[dict] = []

    if ext == ".py":
        for i, raw in enumerate(lines, 1):
            if raw[:1].isspace():
                continue
            stripped = raw.strip()
            if stripped.startswith("@"):
                continue
            match = PAT_ORPHAN_PY.match(stripped)
            if match:
                kind, name = match.groups()
                defs.append({
                    "path": path,
                    "line": i,
                    "name": name,
                    "kind": "function" if kind == "def" else "class",
                    "ext": ext,
                })
        return defs

    brace_depth = 0
    state = {"block_comment": False}

    for i, raw in enumerate(lines, 1):
        cleaned = strip_comments_and_strings(raw.rstrip("\n"), ext, state)
        stripped = cleaned.strip()

        if brace_depth == 0 and stripped:
            if ext == ".php":
                match = PAT_ORPHAN_PHP_FUNC.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "function", "ext": ext})
                match = PAT_ORPHAN_PHP_TYPE.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "type", "ext": ext})

            elif ext in (".js", ".jsx", ".ts", ".tsx"):
                match = PAT_ORPHAN_JS_FUNC.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "function", "ext": ext})
                match = PAT_ORPHAN_JS_VAR.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "function", "ext": ext})
                match = PAT_ORPHAN_JS_CLASS.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "class", "ext": ext})

            elif ext == ".go":
                if PAT_ORPHAN_GO_METHOD.match(stripped):
                    pass
                else:
                    match = PAT_ORPHAN_GO_FUNC.match(stripped)
                    if match:
                        defs.append({"path": path, "line": i, "name": match.group(1), "kind": "function", "ext": ext})
                match = PAT_ORPHAN_GO_TYPE.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "type", "ext": ext})

            elif ext in (".c", ".h", ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx"):
                match = PAT_ORPHAN_C_STATIC_FUNC.match(stripped)
                if match:
                    defs.append({"path": path, "line": i, "name": match.group(1), "kind": "static function", "ext": ext})

        brace_depth += cleaned.count("{") - cleaned.count("}")
        if brace_depth < 0:
            brace_depth = 0

    return defs


def should_skip_orphan(defn: dict) -> bool:
    name = defn["name"]
    ext = defn["ext"]

    if name in ORPHAN_EXEMPT_NAMES:
        return True
    if name.startswith("__") and name.endswith("__"):
        return True
    if ext == ".go" and name[:1].isupper():
        return True
    return False


def check_orphaned_code(source_files: list[dict]) -> None:
    """Flag top-level symbols that appear only at their definition site."""
    token_counts: dict[str, int] = defaultdict(int)
    defs_by_name: dict[str, list[dict]] = defaultdict(list)

    for source in source_files:
        for token in PAT_TOKEN.findall(source["text"]):
            token_counts[token] += 1
        for defn in extract_symbol_defs(source["path"], source["lines"]):
            defs_by_name[defn["name"]].append(defn)

    for name, defs in defs_by_name.items():
        if len(defs) != 1:
            continue
        if token_counts.get(name, 0) > 1:
            continue

        defn = defs[0]
        if should_skip_orphan(defn):
            continue

        add(
            defn["path"],
            defn["line"],
            "ORPHANED_CODE",
            f"{defn['kind'].capitalize()} '{name}' appears unreferenced in the scanned source tree",
            "LOW",
        )


# ------------------------------------------------------------------
# File scanner
# ------------------------------------------------------------------

BASE_SKIP_DIRS = {".git", "__pycache__", "node_modules", "vendor", ".venv",
                  "venv", "env", "dist", "build", ".tox", "eggs"}
BASE_SKIP_FILES = set()


def load_source_file(path: str) -> dict | None:
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
    except Exception as exc:
        print(f"[ERROR] Cannot read {path}: {exc}")
        return None

    return {"path": path, "lines": lines, "text": "".join(lines)}


def scan_file(path: str, lines: list[str]) -> None:

    check_params_and_nesting(path, lines)
    check_magic_numbers(path, lines)
    check_empty_catch(path, lines)
    check_c_memory_safety(path, lines)
    check_dead_imports(path, lines)
    check_duplicate_blocks(path, lines)


def scan_dir(root: str) -> None:
    source_files: list[dict] = []
    root_path = Path(root).resolve()
    skip_dir_names = merge_skip_dir_names(BASE_SKIP_DIRS)
    skip_relative_paths = merge_skip_relative_paths()
    skip_file_names = merge_skip_file_names(BASE_SKIP_FILES)

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
            if os.path.splitext(fname)[1].lower() in EXTENSIONS:
                source = load_source_file(os.path.join(dirpath, fname))
                if source is not None:
                    source_files.append(source)

    for source in source_files:
        scan_file(source["path"], source["lines"])

    check_orphaned_code(source_files)


# ------------------------------------------------------------------
# Main
# ------------------------------------------------------------------

def main() -> None:
    global ACTIVE_KIND_FILTER

    parser = argparse.ArgumentParser(description="Detect code smells and orphaned code.")
    parser.add_argument("directory", help="Directory to scan")
    parser.add_argument("--only-kind", choices=["ORPHANED_CODE", "C_MEMORY_UNSAFE"], help="Only emit one finding kind")
    args = parser.parse_args()

    target = os.path.abspath(args.directory)
    if not os.path.isdir(target):
        print(f"Not a directory: {target}", file=sys.stderr)
        sys.exit(1)

    ACTIVE_KIND_FILTER = args.only_kind

    print(f"Code Smells scan: {target}")
    print("-" * 60)

    scan_dir(target)

    # Summary
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
