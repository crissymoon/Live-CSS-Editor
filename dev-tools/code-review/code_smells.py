#!/usr/bin/env python3
"""
code_smells.py  –  Code Smell Detector
Scans .py, .php, .js, .ts, .c, .h files.

Usage:
    python3 code_smells.py <directory>

Detected smells:
  • Long parameter lists   (> 5 params in function signature)
  • Deep nesting           (> 4 levels of indent)
  • Duplicate code blocks  (>= 6 identical consecutive lines appear 2+ times)
  • Primitive obsession    (> 4 primitive params in one function)
  • Dead imports           (imported name never used in rest of file)
  • Long methods           (function body > 60 lines)
  • Magic numbers          (bare numeric literals in code, not in declarations)
  • Empty catch blocks     (catch/except with only pass or comment)
"""

import sys
import os
import re
from collections import defaultdict

EXTENSIONS = {".py", ".php", ".js", ".ts", ".c", ".h"}
MAX_PARAMS          = 5
MAX_NESTING         = 4
DUP_BLOCK_SIZE      = 6      # lines in a block to consider duplicates
MAX_PRIMITIVE_PARAMS= 4
MAX_METHOD_LINES    = 60

PRIMITIVES = {"int", "str", "float", "bool", "string", "number", "char",
              "double", "long", "short", "unsigned"}

FINDINGS: list[dict] = []


def add(path: str, line: int, kind: str, msg: str, severity: str = "MEDIUM") -> None:
    FINDINGS.append({"path": path, "line": line, "kind": kind, "msg": msg, "severity": severity})
    rel = os.path.relpath(path)
    print(f"[{severity}] {rel}:{line}  [{kind}]  {msg}")


# ──────────────────────────────────────────────────────────────────
# Smell detectors
# ──────────────────────────────────────────────────────────────────

# Regex patterns
PAT_PY_DEF   = re.compile(r"^\s*def\s+\w+\s*\(([^)]*)\)")
PAT_JS_FUNC  = re.compile(r"(?:function\s+\w+\s*|(?:const|let|var)\s+\w+\s*=\s*(?:async\s*)?\()\s*([^)]*)\)")
PAT_PHP_FUNC = re.compile(r"function\s+\w+\s*\(([^)]*)\)")
PAT_C_FUNC   = re.compile(r"\w[\w\s\*]+\s+\w+\s*\(([^)]*)\)\s*\{")
PAT_IMPORT_PY= re.compile(r"^(?:import|from)\s+([\w.]+)")
PAT_MAGIC    = re.compile(r"(?<!\w)(?<![\'\"])(\d{2,})(?![\'\"])(?!\w)")


def count_params(sig: str) -> tuple[int, int]:
    """Return (total_params, primitive_params)."""
    if not sig.strip():
        return 0, 0
    params = [p.strip() for p in sig.split(",") if p.strip()]
    # Strip type annotations / defaults
    primitives = 0
    for p in params:
        p_clean = p.split("=")[0].strip()
        # Python/TS type hint: name: type
        hint = p_clean.split(":")[-1].strip().lower() if ":" in p_clean else ""
        # C/PHP: type name
        words = p_clean.split()
        for w in words:
            if w.lower() in PRIMITIVES:
                primitives += 1
                break
        if hint in PRIMITIVES:
            primitives += 1
    return len(params), primitives


def check_params_and_nesting(path: str, lines: list[str]) -> None:
    # Choose patterns based on extension
    ext = os.path.splitext(path)[1].lower()
    if ext == ".py":
        func_pat = PAT_PY_DEF
    elif ext == ".php":
        func_pat = PAT_PHP_FUNC
    elif ext in (".js", ".ts"):
        func_pat = PAT_JS_FUNC
    else:
        func_pat = PAT_C_FUNC

    func_start = None
    func_line_count = 0

    for i, raw in enumerate(lines, 1):
        # Nesting depth: count leading braces / indentation
        indent = len(raw) - len(raw.lstrip())
        indent_level = indent // 4  # assume 4-space / tab=4 indent

        if indent_level > MAX_NESTING:
            add(path, i, "DEEP_NESTING",
                f"Nesting depth ~{indent_level} (max {MAX_NESTING})", "HIGH")

        # Long method tracking (Python only – indentation-based)
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


def check_magic_numbers(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    for i, raw in enumerate(lines, 1):
        stripped = raw.strip()
        # Skip comments, strings, declarations/constants
        if stripped.startswith(("#", "//", "*", "/*", "'")):
            continue
        if "=" in stripped and stripped.split("=")[0].strip().upper() == stripped.split("=")[0].strip():
            continue  # UPPER_CASE = constant
        for m in PAT_MAGIC.finditer(raw):
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


def check_dead_imports(path: str, lines: list[str]) -> None:
    ext = os.path.splitext(path)[1].lower()
    if ext != ".py":
        return
    imports: dict[str, int] = {}
    for i, raw in enumerate(lines, 1):
        m = PAT_IMPORT_PY.match(raw.strip())
        if m:
            # from X import Y, Z  → Y and Z
            if raw.strip().startswith("from"):
                rest = re.search(r"import\s+(.+)", raw)
                if rest:
                    names = [n.strip().split(" as ")[-1] for n in rest.group(1).split(",")]
                    for n in names:
                        if n and n != "*":
                            imports[n] = i
            else:
                name = m.group(1).split(".")[-1]
                imports[name] = i

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
    for i in range(len(norm) - DUP_BLOCK_SIZE + 1):
        block = tuple(norm[i : i + DUP_BLOCK_SIZE])
        if all(b == "" for b in block):
            continue
        if block in seen:
            first_line = seen[block]
            add(path, i + 1, "DUPLICATE_CODE",
                f"Block of {DUP_BLOCK_SIZE} lines duplicates lines {first_line}–{first_line + DUP_BLOCK_SIZE - 1}",
                "MEDIUM")
        else:
            seen[block] = i + 1


# ──────────────────────────────────────────────────────────────────
# File scanner
# ──────────────────────────────────────────────────────────────────

SKIP_DIRS = {".git", "__pycache__", "node_modules", "vendor", ".venv",
             "venv", "env", "dist", "build", ".tox", "eggs"}


def scan_file(path: str) -> None:
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
    except Exception as exc:
        print(f"[ERROR] Cannot read {path}: {exc}")
        return

    check_params_and_nesting(path, lines)
    check_magic_numbers(path, lines)
    check_empty_catch(path, lines)
    check_dead_imports(path, lines)
    check_duplicate_blocks(path, lines)


def scan_dir(root: str) -> None:
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fname in filenames:
            if os.path.splitext(fname)[1].lower() in EXTENSIONS:
                scan_file(os.path.join(dirpath, fname))


# ──────────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────────

def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: code_smells.py <directory>", file=sys.stderr)
        sys.exit(1)

    target = os.path.abspath(sys.argv[1])
    if not os.path.isdir(target):
        print(f"Not a directory: {target}", file=sys.stderr)
        sys.exit(1)

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
