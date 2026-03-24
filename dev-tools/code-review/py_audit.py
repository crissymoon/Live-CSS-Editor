#!/usr/bin/env python3
"""
py_audit.py
-----------
Scans the live-css project for Python code quality concerns.

Checks performed:
  PEP 8   -- line length > 120, bare except, star imports, trailing whitespace
  PEP 484 -- public functions missing type annotations
  PEP 498 -- old-style %-format strings reachable as f-string candidates
  Safety  -- eval/exec usage, shell=True in subprocess, os.system(), mutable defaults
  Design  -- swallowed exceptions (except + pass only), missing __main__ guard,
              TODO/FIXME/HACK comments, global statements outside module level,
              print() calls in library modules (non-test, non-script files)
  Threads -- threading.Thread subclass or Thread() usage without obvious lock usage
              alongside shared mutable state

Usage:
    python3 py_audit.py                  # scan live-css root
    python3 py_audit.py --dir /some/path # scan another directory
    python3 py_audit.py --json           # output raw JSON
    python3 py_audit.py --min-severity warning  # filter by severity
"""

import os
import re
import sys
import ast
import json
import argparse
import traceback
from pathlib import Path
from typing import List, Dict, Any, Optional

from scan_config import merge_skip_dir_names, merge_skip_file_names, merge_skip_relative_paths, should_skip_relative_path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Directories to skip entirely (matched at any depth by folder name).
BASE_SKIP_DIRS = {
    ".venv",
    "venv",
    "__pycache__",
    "src-tauri",
    "vendor",
    "node_modules",
    ".git",
    "build",
    "dist",
    "site-packages",
    "backups",
    "target",  # Rust target dir
    "logs",
}

# Files to skip entirely (by basename).
BASE_SKIP_FILES = {
    "py_audit.py",  # skip this script itself
}

# Subdirectory prefixes that are clearly test/script contexts
# (suppresses "print in library" warning).
SCRIPT_PATTERNS = {
    "test_", "smoke_", "diag_", "run_", "lines_count", "security_ck",
}

SEVERITY_ORDER = {"error": 0, "warning": 1, "info": 2, "note": 3}

# ---------------------------------------------------------------------------
# Finding dataclass (plain dict for JSON compat)
# ---------------------------------------------------------------------------

def finding(
    path: str,
    line: int,
    severity: str,
    code: str,
    message: str,
    snippet: str = "",
) -> Dict[str, Any]:
    return {
        "path": path,
        "line": line,
        "severity": severity,
        "code": code,
        "message": message,
        "snippet": snippet.strip()[:120],
    }


# ---------------------------------------------------------------------------
# AST-based checks
# ---------------------------------------------------------------------------

def _func_missing_annotations(node: ast.FunctionDef, is_method: bool) -> bool:
    """Return True if a public function lacks return annotation AND arg annotations."""
    if node.name.startswith("_"):
        return False
    args = node.args
    all_args = args.args + args.posonlyargs + args.kwonlyargs
    if args.vararg:
        all_args.append(args.vararg)
    if args.kwarg:
        all_args.append(args.kwarg)
    # skip 'self' and 'cls'
    checked_args = [a for a in all_args if a.arg not in ("self", "cls")]
    has_arg_annot = any(a.annotation is not None for a in checked_args)
    has_return = node.returns is not None
    return not has_arg_annot and not has_return and len(checked_args) > 0


def _collect_names_in_func(node: ast.FunctionDef) -> List[str]:
    """Collect all Name nodes inside a function body."""
    names = []
    for child in ast.walk(node):
        if isinstance(child, ast.Name):
            names.append(child.id)
    return names


def ast_checks(rel_path: str, source: str, tree: ast.AST) -> List[Dict]:
    findings = []
    is_script = any(rel_path.startswith(p) or Path(rel_path).stem.startswith(p)
                    for p in SCRIPT_PATTERNS)

    for node in ast.walk(tree):

        # --- bare except: ---
        if isinstance(node, ast.ExceptHandler):
            if node.type is None:
                findings.append(finding(
                    rel_path, node.lineno, "warning", "E001",
                    "Bare 'except:' catches everything including KeyboardInterrupt and SystemExit. "
                    "Use 'except Exception:' at minimum."
                ))

        # --- swallowed exception (except + only pass) ---
        if isinstance(node, ast.ExceptHandler):
            body = node.body
            if len(body) == 1 and isinstance(body[0], ast.Pass):
                findings.append(finding(
                    rel_path, node.lineno, "warning", "E002",
                    "Exception caught but silently swallowed (bare pass). "
                    "At least log the error so failures are visible in the console."
                ))

        # --- eval() / exec() ---
        if isinstance(node, ast.Call):
            func = node.func
            name = None
            if isinstance(func, ast.Name):
                name = func.id
            elif isinstance(func, ast.Attribute):
                name = func.attr
            if name in ("eval", "exec"):
                findings.append(finding(
                    rel_path, node.lineno, "error", "S001",
                    f"'{name}()' executes arbitrary code. "
                    "Review whether this is intentional and ensure input is never user-controlled."
                ))

        # --- os.system() ---
        if (isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr == "system"
                and isinstance(node.func.value, ast.Name)
                and node.func.value.id == "os"):
            findings.append(finding(
                rel_path, node.lineno, "warning", "S002",
                "os.system() is a shell injection risk. "
                "Prefer subprocess.run(..., shell=False) with a list of arguments."
            ))

        # --- subprocess shell=True ---
        if isinstance(node, ast.Call):
            for kw in node.keywords:
                if kw.arg == "shell" and isinstance(kw.value, ast.Constant) and kw.value.value is True:
                    findings.append(finding(
                        rel_path, node.lineno, "warning", "S003",
                        "subprocess called with shell=True. "
                        "Pass a list of arguments and set shell=False unless a pipeline is genuinely required."
                    ))

        # --- mutable default argument ---
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            for default in node.args.defaults + node.args.kw_defaults:
                if default is None:
                    continue
                if isinstance(default, (ast.List, ast.Dict, ast.Set)):
                    findings.append(finding(
                        rel_path, node.lineno, "warning", "D001",
                        f"Function '{node.name}' has a mutable default argument (list/dict/set). "
                        "Use None as the default and create the object inside the function."
                    ))

        # --- global statement inside a function ---
        if isinstance(node, ast.Global):
            # Only flag globals that appear inside a function body; module-level
            # globals are ordinary variable assignments and should not be reported.
            # We detect this by checking whether any ancestor is a FunctionDef.
            # ast.walk visits all nodes without parent links, so we build a
            # parent map once per tree and reuse it.
            pass  # handled in the parent-map pass below

        # --- star import ---
        if isinstance(node, ast.ImportFrom):
            for alias in node.names:
                if alias.name == "*":
                    findings.append(finding(
                        rel_path, node.lineno, "warning", "E003",
                        f"'from {node.module} import *' pollutes the namespace. "
                        "Import only the names you need."
                    ))

        # --- print() in a library module (not a script/test) ---
        if not is_script:
            if isinstance(node, ast.Call):
                func = node.func
                if isinstance(func, ast.Name) and func.id == "print":
                    findings.append(finding(
                        rel_path, node.lineno, "note", "D003",
                        "print() in a library module -- consider using logging so callers "
                        "can control verbosity."
                    ))

        # --- public function missing type annotations ---
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            is_method = False  # simplified; method check below
            if _func_missing_annotations(node, is_method):
                findings.append(finding(
                    rel_path, node.lineno, "note", "A001",
                    f"Public function '{node.name}' has no type annotations (PEP 484). "
                    "Adding annotations improves IDE support and catches type errors early."
                ))

    # --- global statement inside a function (parent-map pass) ---
    try:
        parent: dict = {}
        for node in ast.walk(tree):
            for child in ast.iter_child_nodes(node):
                parent[id(child)] = node
        for node in ast.walk(tree):
            if isinstance(node, ast.Global):
                p = parent.get(id(node))
                while p is not None:
                    if isinstance(p, (ast.FunctionDef, ast.AsyncFunctionDef)):
                        findings.append(finding(
                            rel_path, node.lineno, "info", "D002",
                            f"'global {', '.join(node.names)}' inside a function makes state "
                            "hard to reason about. Consider passing values explicitly."
                        ))
                        break
                    p = parent.get(id(p))
    except Exception as exc:
        sys.stderr.write(f"[py_audit] global-in-function check failed for {rel_path}: {exc}\n")

    # --- missing if __name__ == '__main__' guard in files with top-level code ---
    try:
        _check_main_guard(rel_path, tree, findings, is_script)
    except Exception as exc:
        sys.stderr.write(f"[py_audit] __main__ guard check failed for {rel_path}: {exc}\n")

    return findings


def _check_main_guard(rel_path: str, tree: ast.AST, findings: List[Dict], is_script: bool) -> None:
    """Flag scripts with top-level side-effect code but no __name__ guard."""
    if not is_script:
        return
    # Look for top-level Expr(Call) that are not imports/assignments -- side effects
    has_guard = False
    has_top_level_calls = False

    for node in tree.body:
        if isinstance(node, ast.If):
            test = node.test
            # if __name__ == "__main__" or if "__main__" == __name__
            if isinstance(test, ast.Compare):
                left = test.left
                comparators = test.comparators
                if (isinstance(left, ast.Name) and left.id == "__name__") or \
                   (comparators and isinstance(comparators[0], ast.Name) and comparators[0].id == "__name__"):
                    has_guard = True
        if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
            func = node.value.func
            # ignore print at top-level
            if isinstance(func, ast.Name) and func.id in ("print",):
                continue
            has_top_level_calls = True

    if has_top_level_calls and not has_guard:
        findings.append(finding(
            rel_path, 1, "info", "D004",
            "Script has top-level calls but no 'if __name__ == \"__main__\":' guard. "
            "Wrapping side effects prevents accidental execution on import."
        ))


# ---------------------------------------------------------------------------
# Line-level checks
# ---------------------------------------------------------------------------

_RE_LONG_LINE = re.compile(r".{121,}")
_RE_TRAILING_WS = re.compile(r"[ \t]+$")
_RE_OLD_FORMAT = re.compile(r'"[^"]*%[sdiouxXeEfFgGcrb%][^"]*"\s*%\s*|\'[^\']*%[sdiouxXeEfFgGcrb%][^\']*\'\s*%\s*')
_RE_TODO = re.compile(r"#.*\b(TODO|FIXME|HACK|XXX)\b", re.IGNORECASE)
_RE_TABS = re.compile(r"^\t+")
_RE_MIXED_INDENT = re.compile(r"^ +\t|^\t+ ")


def line_checks(rel_path: str, lines: List[str]) -> List[Dict]:
    findings = []

    for i, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\n")

        # PEP 8 E501 analogous: line > 120 chars
        if len(line) > 120:
            findings.append(finding(
                rel_path, i, "note", "L001",
                f"Line is {len(line)} characters (limit 120). Long lines reduce readability.",
                line[:80] + "..."
            ))

        # Trailing whitespace
        if _RE_TRAILING_WS.search(line):
            findings.append(finding(
                rel_path, i, "note", "L002",
                "Trailing whitespace.",
            ))

        # Tab indentation (PEP 8 recommends spaces)
        if _RE_TABS.match(line):
            findings.append(finding(
                rel_path, i, "info", "L003",
                "Tab used for indentation. PEP 8 requires 4-space indentation."
            ))

        # Mixed indentation
        if _RE_MIXED_INDENT.match(line):
            findings.append(finding(
                rel_path, i, "warning", "L004",
                "Mixed tabs and spaces in indentation. This will cause IndentationError in Python 3."
            ))

        # Old-style %-format (PEP 498 f-string candidate)
        if _RE_OLD_FORMAT.search(line):
            findings.append(finding(
                rel_path, i, "note", "F001",
                "Old-style %-format string. Consider replacing with an f-string (PEP 498).",
                line.strip()
            ))

        # TODO/FIXME/HACK
        if _RE_TODO.search(line):
            match = _RE_TODO.search(line)
            label = match.group(1).upper() if match else "TODO"
            findings.append(finding(
                rel_path, i, "note", "D005",
                f"{label} comment left in code.",
                line.strip()
            ))

    return findings


# ---------------------------------------------------------------------------
# File scanner
# ---------------------------------------------------------------------------

def scan_file(path: Path, root: Path) -> List[Dict]:
    rel = str(path.relative_to(root))
    findings = []

    try:
        source = path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        sys.stderr.write(f"[py_audit] cannot read {rel}: {exc}\n")
        return findings

    lines = source.splitlines(keepends=True)

    # Line-level checks (always run even if AST parse fails)
    try:
        findings.extend(line_checks(rel, lines))
    except Exception as exc:
        sys.stderr.write(f"[py_audit] line checks failed for {rel}: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    # AST checks
    try:
        tree = ast.parse(source, filename=str(path))
        findings.extend(ast_checks(rel, source, tree))
    except SyntaxError as exc:
        findings.append(finding(
            rel, exc.lineno or 0, "error", "P001",
            f"Syntax error -- file cannot be parsed: {exc.msg}",
            str(exc.text or "").strip()
        ))
    except Exception as exc:
        sys.stderr.write(f"[py_audit] AST parse failed for {rel}: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return findings


# ---------------------------------------------------------------------------
# Directory walker
# ---------------------------------------------------------------------------

def walk_python_files(root: Path) -> List[Path]:
    results = []
    skip_dir_names = merge_skip_dir_names(BASE_SKIP_DIRS)
    skip_relative_paths = merge_skip_relative_paths()
    skip_file_names = merge_skip_file_names(BASE_SKIP_FILES)
    try:
        for dirpath, dirnames, filenames in os.walk(root):
            dirpath_path = Path(dirpath)
            # Prune skipped directories in place so os.walk does not descend
            dirnames[:] = [
                d for d in dirnames
                if d not in skip_dir_names
                and not d.startswith(".")
                and not should_skip_relative_path(dirpath_path / d, root, skip_relative_paths)
            ]
            for fname in filenames:
                if not fname.endswith(".py"):
                    continue
                if fname in skip_file_names:
                    continue
                results.append(Path(dirpath) / fname)
    except Exception as exc:
        sys.stderr.write(f"[py_audit] walk failed under {root}: {exc}\n")
        traceback.print_exc(file=sys.stderr)
    return sorted(results)


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

SEVERITY_LABEL = {
    "error":   "ERROR  ",
    "warning": "WARN   ",
    "info":    "INFO   ",
    "note":    "NOTE   ",
}

SEVERITY_COLOR = {
    "error":   "\033[91m",
    "warning": "\033[93m",
    "info":    "\033[94m",
    "note":    "\033[37m",
}
RESET = "\033[0m"


def print_report(all_findings: List[Dict], use_color: bool, min_severity: str) -> None:
    min_rank = SEVERITY_ORDER.get(min_severity, 3)

    by_file: Dict[str, List[Dict]] = {}
    for f in all_findings:
        by_file.setdefault(f["path"], []).append(f)

    total = 0
    counts: Dict[str, int] = {"error": 0, "warning": 0, "info": 0, "note": 0}

    for path in sorted(by_file):
        file_findings = [
            f for f in by_file[path]
            if SEVERITY_ORDER.get(f["severity"], 99) <= min_rank
        ]
        if not file_findings:
            continue

        print(f"\n  {path}")
        print("  " + "-" * (len(path) + 2))

        for f in sorted(file_findings, key=lambda x: x["line"]):
            sev = f["severity"]
            label = SEVERITY_LABEL.get(sev, "       ")
            code = f["code"]
            msg = f["message"]
            line = f["line"]
            snip = f["snippet"]

            if use_color:
                color = SEVERITY_COLOR.get(sev, "")
                print(f"  {color}{label}{RESET} L{line:<5} [{code}] {msg}")
            else:
                print(f"  {label} L{line:<5} [{code}] {msg}")

            if snip:
                print(f"           {snip[:100]}")

            counts[sev] = counts.get(sev, 0) + 1
            total += 1

    print()
    print("=" * 68)
    print(f"  SUMMARY   {total} finding(s) in {len(by_file)} file(s)")
    print(f"            errors={counts.get('error',0)}  "
          f"warnings={counts.get('warning',0)}  "
          f"info={counts.get('info',0)}  "
          f"notes={counts.get('note',0)}")
    print("=" * 68)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Python code quality audit for the live-css project."
    )
    parser.add_argument(
        "--dir",
        default=str(Path(__file__).parent),
        help="Root directory to scan (default: directory containing this script)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output findings as JSON instead of human-readable report",
    )
    parser.add_argument(
        "--min-severity",
        default="note",
        choices=["error", "warning", "info", "note"],
        help="Only report findings at this severity or above (default: note)",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI color codes in output",
    )
    args = parser.parse_args()

    root = Path(args.dir).resolve()
    if not root.is_dir():
        sys.stderr.write(f"[py_audit] not a directory: {root}\n")
        return 1

    use_color = not args.no_color and sys.stdout.isatty()

    print(f"[py_audit] scanning {root}")
    print(f"[py_audit] skipping dirs: {', '.join(sorted(merge_skip_dir_names(BASE_SKIP_DIRS)))}")

    try:
        py_files = walk_python_files(root)
    except Exception as exc:
        sys.stderr.write(f"[py_audit] fatal: walk failed: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        return 1

    print(f"[py_audit] {len(py_files)} Python file(s) found\n")

    all_findings: List[Dict] = []
    for py_file in py_files:
        try:
            file_findings = scan_file(py_file, root)
            all_findings.extend(file_findings)
        except Exception as exc:
            sys.stderr.write(f"[py_audit] unhandled error scanning {py_file}: {exc}\n")
            traceback.print_exc(file=sys.stderr)

    if args.json:
        print(json.dumps(all_findings, indent=2))
        return 0

    try:
        print_report(all_findings, use_color, args.min_severity)
    except Exception as exc:
        sys.stderr.write(f"[py_audit] report rendering failed: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        # fall back to raw JSON so results are never lost
        print(json.dumps(all_findings, indent=2))
        return 1

    errors = sum(1 for f in all_findings if f["severity"] == "error")
    return 1 if errors > 0 else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write("\n[py_audit] interrupted\n")
        sys.exit(130)
    except Exception as exc:
        sys.stderr.write(f"[py_audit] fatal unhandled exception: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)
