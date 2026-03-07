#!/usr/bin/env python3
"""
outliner.py
-----------
Print the structural outline and comments of a source file.

Supported languages (auto-detected from extension):
  .py           Python  -- AST-based (classes, functions, constants, decorators)
  .js .ts .mjs  JavaScript / TypeScript -- regex-based
  .php          PHP     -- regex-based
  .c .h         C / C++ -- regex-based
  .css .scss    CSS     -- regex-based (selectors, @-rules)

Each outline item shows:
  LINE   KIND        NAME / SUMMARY

Comment lines from the source are included inline so you can see
doc-strings, JSDoc, block comments, and inline annotations.

Usage:
    python3 c_tools/outliner.py path/to/file.py
    python3 c_tools/outliner.py path/to/file.js --json
    python3 c_tools/outliner.py path/to/file.php --no-comments
    python3 c_tools/outliner.py path/to/file.c  --max-comment 80

Output columns (plain text):
    L<line>   <kind padded>   <indent><name>
              ^comment text

JSON schema per item:
    { "line": int, "kind": str, "name": str, "depth": int, "comment": str }
"""

import os
import re
import sys
import ast
import json
import argparse
import tokenize
import io
import traceback
from typing import List, Optional, Dict, Any


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

KINDS = (
    "module", "class", "function", "method", "property",
    "async_function", "async_method",
    "constant", "import", "decorator",
    "selector", "at_rule", "variable",
    "struct", "typedef", "macro",
    "comment", "docstring", "block_comment",
)


class Node:
    __slots__ = ("line", "kind", "name", "depth", "comment")

    def __init__(
        self,
        line: int,
        kind: str,
        name: str,
        depth: int = 0,
        comment: str = "",
    ) -> None:
        self.line = line
        self.kind = kind
        self.name = name
        self.depth = depth
        self.comment = comment

    def to_dict(self) -> Dict[str, Any]:
        return {
            "line": self.line,
            "kind": self.kind,
            "name": self.name,
            "depth": self.depth,
            "comment": self.comment,
        }


# ---------------------------------------------------------------------------
# Python outliner (AST + tokenize)
# ---------------------------------------------------------------------------

def _py_get_comments(source: str) -> Dict[int, str]:
    """Return {line_number: comment_text} for all # comments in source."""
    comments: Dict[int, str] = {}
    try:
        tokens = tokenize.generate_tokens(io.StringIO(source).readline)
        for tok_type, tok_string, tok_start, _, _ in tokens:
            if tok_type == tokenize.COMMENT:
                line_no = tok_start[0]
                comments[line_no] = tok_string.lstrip("#").strip()
    except tokenize.TokenError as exc:
        sys.stderr.write(f"[outliner] tokenize warning: {exc}\n")
    except Exception as exc:
        sys.stderr.write(f"[outliner] comment extraction error: {exc}\n")
        traceback.print_exc(file=sys.stderr)
    return comments


def _py_docstring(node: ast.AST) -> str:
    """Extract the docstring text from a function/class/module node."""
    try:
        doc = ast.get_docstring(node, clean=True)
        if doc:
            first_line = doc.splitlines()[0].strip()
            return first_line[:160]
    except Exception:
        pass
    return ""


def _py_decorator_names(node: ast.FunctionDef) -> List[str]:
    """Return decorator names for a function/method."""
    names = []
    try:
        for d in node.decorator_list:
            if isinstance(d, ast.Name):
                names.append(d.id)
            elif isinstance(d, ast.Attribute):
                names.append(f"{ast.unparse(d)}" if hasattr(ast, "unparse") else d.attr)
            elif isinstance(d, ast.Call):
                func = d.func
                if isinstance(func, ast.Name):
                    names.append(f"@{func.id}()")
                elif isinstance(func, ast.Attribute):
                    names.append(f"@{func.attr}()")
    except Exception as exc:
        sys.stderr.write(f"[outliner] decorator extraction warning: {exc}\n")
    return names


def _py_walk(
    node: ast.AST,
    source: str,
    comments: Dict[int, str],
    parent_kind: str = "module",
    depth: int = 0,
) -> List[Node]:
    nodes: List[Node] = []

    try:
        for child in ast.iter_child_nodes(node):

            # Module-level docstring
            if (
                isinstance(child, ast.Expr)
                and depth == 0
                and isinstance(child.value, ast.Constant)
                and isinstance(child.value.value, str)
            ):
                text = child.value.value.splitlines()[0].strip()[:160]
                nodes.append(Node(child.lineno, "docstring", text, depth))
                continue

            # Class definition
            if isinstance(child, ast.ClassDef):
                bases = []
                try:
                    bases = [
                        ast.unparse(b) if hasattr(ast, "unparse") else getattr(b, "id", "?")
                        for b in child.bases
                    ]
                except Exception:
                    pass
                base_str = f"({', '.join(bases)})" if bases else ""
                doc = _py_docstring(child)
                nodes.append(Node(child.lineno, "class", f"{child.name}{base_str}", depth, doc))
                nodes.extend(_py_walk(child, source, comments, "class", depth + 1))
                continue

            # Function / async function
            if isinstance(child, (ast.FunctionDef, ast.AsyncFunctionDef)):
                is_async = isinstance(child, ast.AsyncFunctionDef)
                if parent_kind == "class":
                    kind = "async_method" if is_async else "method"
                else:
                    kind = "async_function" if is_async else "function"

                # Argument summary
                args = child.args
                arg_names = [a.arg for a in args.args]
                if args.vararg:
                    arg_names.append(f"*{args.vararg.arg}")
                if args.kwarg:
                    arg_names.append(f"**{args.kwarg.arg}")
                # skip 'self' / 'cls' in display
                display_args = [a for a in arg_names if a not in ("self", "cls")]
                sig = f"({', '.join(display_args)})"

                # Return annotation
                ret = ""
                try:
                    if child.returns:
                        ret = " -> " + (
                            ast.unparse(child.returns)
                            if hasattr(ast, "unparse")
                            else "..."
                        )
                except Exception:
                    pass

                doc = _py_docstring(child)

                # Decorators as comment annotation
                decs = _py_decorator_names(child)
                dec_str = "  @" + ", @".join(decs) if decs else ""

                nodes.append(Node(
                    child.lineno, kind,
                    f"{child.name}{sig}{ret}{dec_str}",
                    depth, doc
                ))
                nodes.extend(_py_walk(child, source, comments, kind, depth + 1))
                continue

            # Top-level assignments that look like constants (ALL_CAPS or titled)
            if (
                isinstance(child, ast.Assign)
                and parent_kind == "module"
                and depth == 0
            ):
                try:
                    for target in child.targets:
                        if isinstance(target, ast.Name):
                            name = target.id
                            # Only show if it looks like a constant or notable binding
                            if name.isupper() or (name[0].isupper() and "_" not in name):
                                val_str = ""
                                if hasattr(ast, "unparse"):
                                    try:
                                        val_str = " = " + ast.unparse(child.value)[:60]
                                    except Exception:
                                        pass
                                comment = comments.get(child.lineno, "")
                                nodes.append(Node(child.lineno, "constant", name + val_str, depth, comment))
                except Exception as exc:
                    sys.stderr.write(f"[outliner] assignment node warning: {exc}\n")
                continue

    except Exception as exc:
        sys.stderr.write(f"[outliner] _py_walk error at depth {depth}: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


def outline_python(source: str, include_comments: bool = True) -> List[Node]:
    nodes: List[Node] = []
    try:
        comments = _py_get_comments(source)
        tree = ast.parse(source)
        nodes = _py_walk(tree, source, comments, "module", 0)

        # Standalone top-level # comments (not attached to a node)
        if include_comments:
            node_lines = {n.line for n in nodes}
            for line_no in sorted(comments):
                if line_no not in node_lines:
                    nodes.append(Node(line_no, "comment", comments[line_no], 0))

        nodes.sort(key=lambda n: n.line)

    except SyntaxError as exc:
        sys.stderr.write(f"[outliner] Python syntax error line {exc.lineno}: {exc.msg}\n")
        nodes.append(Node(exc.lineno or 0, "comment",
                          f"SYNTAX ERROR: {exc.msg}", 0))
    except Exception as exc:
        sys.stderr.write(f"[outliner] Python outline error: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


# ---------------------------------------------------------------------------
# Shared block-comment accumulator (used by all regex-based outliners)
# ---------------------------------------------------------------------------

class _BlockCommentState:
    """
    Tracks whether we are currently inside a /* ... */ block comment and
    accumulates the text.

    Each line-loop body calls feed() once.  The return value tells the caller
    what to do:
        (True,  node_or_None) -- line was consumed by block-comment logic;
                                 append node if not None, then continue.
        (False, None)         -- normal line; proceed to pattern matching.
    """

    __slots__ = ("_active", "_buf", "_start")

    def __init__(self) -> None:
        self._active = False
        self._buf: List[str] = []
        self._start = 0

    @property
    def active(self) -> bool:
        return self._active

    def feed(
        self, line: str, lineno: int, depth: int, include_comments: bool
    ):
        if not self._active and "/*" in line:
            self._active = True
            self._buf = []
            self._start = lineno
            content = re.sub(r"^.*?/\*+\s*", "", line).rstrip()
            if content and not content.startswith("*"):
                self._buf.append(content)
            if "*/" in line:
                return self._close(depth, include_comments)
            return True, None

        if self._active:
            stripped = re.sub(r"^\s*\*+\s*", "", line).rstrip("*/").strip()
            if stripped:
                self._buf.append(stripped)
            if "*/" in line:
                return self._close(depth, include_comments)
            return True, None

        return False, None

    def _close(self, depth: int, include_comments: bool):
        self._active = False
        node = None
        if include_comments and self._buf:
            node = Node(self._start, "block_comment",
                        " ".join(self._buf)[:200], depth)
        self._buf = []
        return True, node


# ---------------------------------------------------------------------------
# JavaScript / TypeScript outliner
# ---------------------------------------------------------------------------

_JS_PATTERNS = [
    # JSDoc / block comment
    ("block_comment", re.compile(r"^\s*/\*+\s*(.*?)\s*(?:\*/)?$")),
    # single-line comment
    ("comment",       re.compile(r"^\s*//+\s*(.+)$")),
    # class declaration
    ("class",         re.compile(r"^\s*(?:export\s+)?(?:default\s+)?class\s+(\w+)")),
    # async function declaration
    ("async_function", re.compile(
        r"^\s*(?:export\s+)?(?:default\s+)?async\s+function\s*\*?\s*(\w+)\s*\(")),
    # function declaration
    ("function",      re.compile(
        r"^\s*(?:export\s+)?(?:default\s+)?function\s*\*?\s*(\w+)\s*\(")),
    # const/let/var arrow or function expression
    ("function",      re.compile(
        r"^\s*(?:export\s+)?(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s+)?(?:function|\([^)]*\)\s*=>|\w+\s*=>)")),
    # method shorthand: word followed by ( ... ) { but NOT a keyword
    ("method",        re.compile(
        r"^\s*(?:async\s+)?"
        r"(?!(?:if|else|for|while|switch|do|try|catch|finally|return|new|typeof|instanceof|void|delete)\b)"
        r"([a-zA-Z_$][\w$]*)\s*\([^)]*\)\s*\{")),
]

_JS_INDENT_RE = re.compile(r"^(\s*)")


def outline_js(source: str, include_comments: bool = True) -> List[Node]:
    nodes: List[Node] = []
    _bc = _BlockCommentState()

    try:
        lines = source.splitlines()
        for i, raw in enumerate(lines, start=1):
            line = raw.rstrip()
            indent = len(_JS_INDENT_RE.match(line).group(1))
            depth = indent // 2

            hit, bc_node = _bc.feed(line, i, depth, include_comments)
            if hit:
                if bc_node:
                    nodes.append(bc_node)
                continue

            for kind, pat in _JS_PATTERNS:
                m = pat.match(line)
                if m:
                    if kind in ("comment", "block_comment") and not include_comments:
                        break
                    name = m.group(1).strip() if m.lastindex else ""
                    nodes.append(Node(i, kind, name, depth))
                    break

    except Exception as exc:
        sys.stderr.write(f"[outliner] JS outline error: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


# ---------------------------------------------------------------------------
# PHP outliner
# ---------------------------------------------------------------------------

_PHP_PATTERNS = [
    ("block_comment", re.compile(r"^\s*/\*+\s*(.*?)\s*(?:\*/)?$")),
    ("comment",       re.compile(r"^\s*//+\s*(.+)$|^\s*#\s*(.+)$")),
    ("class",         re.compile(
        r"^\s*(?:abstract\s+|final\s+)?class\s+(\w+)")),
    ("class",         re.compile(r"^\s*(?:interface|trait)\s+(\w+)")),
    ("async_function", re.compile(
        r"^\s*(?:public|protected|private|static|\s)*function\s+(\w+)\s*\("
        r".*async", re.IGNORECASE)),
    ("function",      re.compile(
        r"^\s*(?:public\s+|protected\s+|private\s+|static\s+|abstract\s+|final\s+)*"
        r"function\s+(\w+)\s*\(")),
    ("constant",      re.compile(r"^\s*(?:const|define\s*\()\s*['\"]?(\w+)")),
]


def outline_php(source: str, include_comments: bool = True) -> List[Node]:
    nodes: List[Node] = []
    _bc = _BlockCommentState()
    brace_depth = 0

    try:
        lines = source.splitlines()
        for i, raw in enumerate(lines, start=1):
            line = raw.rstrip()
            depth = max(0, brace_depth)
            brace_depth += line.count("{") - line.count("}")

            hit, bc_node = _bc.feed(line, i, depth, include_comments)
            if hit:
                if bc_node:
                    nodes.append(bc_node)
                continue

            for kind, pat in _PHP_PATTERNS:
                m = pat.match(line)
                if m:
                    if kind in ("comment", "block_comment") and not include_comments:
                        break
                    name = next((g for g in m.groups() if g), "") if m.lastindex else ""
                    nodes.append(Node(i, kind, name.strip(), depth))
                    break

    except Exception as exc:
        sys.stderr.write(f"[outliner] PHP outline error: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


# ---------------------------------------------------------------------------
# C / C++ outliner
# ---------------------------------------------------------------------------

_C_FUNC_RE = re.compile(
    r"^(?!//|#|/\*|typedef|struct\b|enum\b)"
    r"(?:(?:static|extern|inline|const|unsigned|signed|void|int|long|short|char|"
    r"float|double|size_t|uint\w*|int\w*|bool|auto|struct\s+\w+|[A-Z_]\w+)\s+)+"
    r"(?:\*+\s*)?(\w+)\s*\([^;]*\)\s*(?:\{|$)"
)
_C_STRUCT_RE = re.compile(r"^\s*(?:typedef\s+)?struct\s+(\w+)?\s*\{")
_C_TYPEDEF_RE = re.compile(r"^\s*typedef\s+(?:struct\s+)?\w+[^(]*\b(\w+)\s*;")
_C_MACRO_RE = re.compile(r"^\s*#define\s+(\w+)")
_C_COMMENT_RE = re.compile(r"^\s*//+\s*(.+)$")
_C_BLOCK_RE = re.compile(r"^\s*/\*+\s*(.*?)\s*(?:\*/)?$")


def outline_c(source: str, include_comments: bool = True) -> List[Node]:
    nodes: List[Node] = []
    _bc = _BlockCommentState()
    brace_depth = 0

    try:
        lines = source.splitlines()
        for i, raw in enumerate(lines, start=1):
            line = raw.rstrip()
            depth = min(brace_depth, 4)
            brace_depth = max(0, brace_depth + line.count("{") - line.count("}"))

            hit, bc_node = _bc.feed(line, i, depth, include_comments)
            if hit:
                if bc_node:
                    nodes.append(bc_node)
                continue

            m = _C_COMMENT_RE.match(line)
            if m:
                if include_comments:
                    nodes.append(Node(i, "comment", m.group(1), depth))
                continue

            m = _C_MACRO_RE.match(line)
            if m:
                nodes.append(Node(i, "macro", m.group(1), depth))
                continue

            m = _C_STRUCT_RE.match(line)
            if m:
                nodes.append(Node(i, "struct", m.group(1) or "(anonymous)", depth))
                continue

            m = _C_TYPEDEF_RE.match(line)
            if m:
                nodes.append(Node(i, "typedef", m.group(1), depth))
                continue

            if brace_depth <= 1:
                m = _C_FUNC_RE.match(line)
                if m:
                    nodes.append(Node(i, "function", m.group(1), 0))

    except Exception as exc:
        sys.stderr.write(f"[outliner] C outline error: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


# ---------------------------------------------------------------------------
# CSS outliner
# ---------------------------------------------------------------------------

_CSS_AT_RE   = re.compile(r"^\s*(@[\w-]+)[^{;]*")
_CSS_VAR_RE  = re.compile(r"^\s*(--[\w-]+)\s*:")
_CSS_SEL_RE  = re.compile(r"^\s*([^{}@/\n,]+?)\s*\{")
_CSS_COMM_RE = re.compile(r"^\s*/\*+\s*(.*?)\s*(?:\*/)?$")


def outline_css(source: str, include_comments: bool = True) -> List[Node]:
    nodes: List[Node] = []
    _bc = _BlockCommentState()
    brace_depth = 0

    try:
        lines = source.splitlines()
        for i, raw in enumerate(lines, start=1):
            line = raw.rstrip()
            depth = min(brace_depth, 3)
            brace_depth = max(0, brace_depth + line.count("{") - line.count("}"))

            hit, bc_node = _bc.feed(line, i, depth, include_comments)
            if hit:
                if bc_node:
                    nodes.append(bc_node)
                continue

            m = _CSS_VAR_RE.match(line)
            if m and brace_depth > 0:
                nodes.append(Node(i, "variable", m.group(1), depth))
                continue

            m = _CSS_AT_RE.match(line)
            if m:
                rule_text = line.strip().rstrip("{").strip()[:100]
                nodes.append(Node(i, "at_rule", rule_text, depth))
                continue

            m = _CSS_SEL_RE.match(line)
            if m and brace_depth <= 1:
                sel = m.group(1).strip()[:120]
                if sel and not sel.startswith(("//", "/*")):
                    nodes.append(Node(i, "selector", sel, depth))

    except Exception as exc:
        sys.stderr.write(f"[outliner] CSS outline error: {exc}\n")
        traceback.print_exc(file=sys.stderr)

    return nodes


# ---------------------------------------------------------------------------
# Language dispatch
# ---------------------------------------------------------------------------

_EXT_MAP: Dict[str, str] = {
    ".py":   "python",
    ".js":   "javascript",
    ".ts":   "javascript",
    ".mjs":  "javascript",
    ".cjs":  "javascript",
    ".jsx":  "javascript",
    ".tsx":  "javascript",
    ".php":  "php",
    ".c":    "c",
    ".h":    "c",
    ".cpp":  "c",
    ".cc":   "c",
    ".cxx":  "c",
    ".css":  "css",
    ".scss": "css",
    ".sass": "css",
}


def detect_language(path: str) -> Optional[str]:
    ext = os.path.splitext(path)[1].lower()
    return _EXT_MAP.get(ext)


def outline(
    source_path: Optional[str] = None,
    text: Optional[str] = None,
    language: Optional[str] = None,
    include_comments: bool = True,
) -> List[Node]:
    """
    Parse and return the outline of a source file.

    Args:
        source_path:      Path to the file. Used for language detection if language is None.
        text:             Raw source text. If omitted, read from source_path.
        language:         Override language detection. One of: python, javascript, php, c, css.
        include_comments: Whether to include comment nodes in the output.

    Returns:
        List of Node objects sorted by line number.
    """
    if source_path is None and text is None:
        raise ValueError("Provide either source_path or text.")

    if text is None:
        try:
            with open(source_path, "r", encoding="utf-8", errors="replace") as fh:
                text = fh.read()
        except OSError as exc:
            sys.stderr.write(f"[outliner] cannot read {source_path}: {exc}\n")
            raise

    if language is None and source_path:
        language = detect_language(source_path)

    if not language:
        sys.stderr.write("[outliner] warning: unknown language, attempting Python parser\n")
        language = "python"

    try:
        if language == "python":
            return outline_python(text, include_comments)
        if language == "javascript":
            return outline_js(text, include_comments)
        if language == "php":
            return outline_php(text, include_comments)
        if language == "c":
            return outline_c(text, include_comments)
        if language == "css":
            return outline_css(text, include_comments)

        sys.stderr.write(f"[outliner] unsupported language: {language!r}\n")
        return []

    except Exception as exc:
        sys.stderr.write(f"[outliner] outline() error for language={language}: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        return []


# ---------------------------------------------------------------------------
# Plain-text renderer
# ---------------------------------------------------------------------------

_KIND_LABELS: Dict[str, str] = {
    "module":          "MODULE   ",
    "class":           "CLASS    ",
    "function":        "FUNC     ",
    "async_function":  "ASYNC    ",
    "method":          "METHOD   ",
    "async_method":    "ASYNC-M  ",
    "property":        "PROP     ",
    "constant":        "CONST    ",
    "import":          "IMPORT   ",
    "decorator":       "DECOR    ",
    "selector":        "SELECTOR ",
    "at_rule":         "AT-RULE  ",
    "variable":        "VAR      ",
    "struct":          "STRUCT   ",
    "typedef":         "TYPEDEF  ",
    "macro":           "MACRO    ",
    "comment":         "//       ",
    "docstring":       '"""      ',
    "block_comment":   "/*       ",
}

_KIND_COLORS: Dict[str, str] = {
    "class":           "\033[1;33m",
    "function":        "\033[1;36m",
    "async_function":  "\033[1;96m",
    "method":          "\033[36m",
    "async_method":    "\033[96m",
    "comment":         "\033[90m",
    "docstring":       "\033[90m",
    "block_comment":   "\033[90m",
    "constant":        "\033[35m",
    "macro":           "\033[35m",
    "selector":        "\033[32m",
    "at_rule":         "\033[1;32m",
    "variable":        "\033[34m",
    "struct":          "\033[1;33m",
    "typedef":         "\033[33m",
    "property":        "\033[34m",
}
_RESET = "\033[0m"
_DIM   = "\033[2m"


def render_plain(
    nodes: List[Node],
    source_path: str,
    use_color: bool = True,
    max_comment: int = 120,
    include_comments: bool = True,
) -> str:
    try:
        lines = [f"  {source_path}", "  " + "-" * max(len(source_path), 40), ""]

        for n in nodes:
            if not include_comments and n.kind in ("comment", "docstring", "block_comment"):
                continue

            label = _KIND_LABELS.get(n.kind, f"{n.kind:<9}")
            indent = "  " * n.depth
            name = n.name

            if use_color:
                color = _KIND_COLORS.get(n.kind, "")
                line = f"  L{n.line:<5}  {color}{label}{_RESET}  {indent}{name}"
            else:
                line = f"  L{n.line:<5}  {label}  {indent}{name}"

            lines.append(line)

            if n.comment:
                comment_text = n.comment[:max_comment]
                if use_color:
                    lines.append(f"          {_DIM}  {comment_text}{_RESET}")
                else:
                    lines.append(f"             {comment_text}")

        lines.append("")
        lines.append(f"  {len(nodes)} item(s)  in  {source_path}")
        return "\n".join(lines)

    except Exception as exc:
        sys.stderr.write(f"[outliner] render_plain error: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        return "\n".join(f"L{n.line} {n.kind} {n.name}" for n in nodes)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _cli() -> int:
    parser = argparse.ArgumentParser(
        description="Print the structural outline and comments of a source file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Supported: .py  .js .ts .mjs  .php  .c .h .cpp  .css .scss\n\n"
            "Examples:\n"
            "  python3 c_tools/outliner.py js/agent.js\n"
            "  python3 c_tools/outliner.py ai/anthropic.php --no-comments\n"
            "  python3 c_tools/outliner.py db-browser/main.c --json\n"
        ),
    )
    parser.add_argument("file", help="Source file to outline")
    parser.add_argument(
        "--language", "-l",
        default=None,
        choices=["python", "javascript", "php", "c", "css"],
        help="Override language detection",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output as JSON array",
    )
    parser.add_argument(
        "--no-comments",
        action="store_true",
        help="Omit comment lines from output",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI color codes",
    )
    parser.add_argument(
        "--max-comment",
        type=int,
        default=120,
        help="Maximum characters shown per comment line (default 120)",
    )

    args = parser.parse_args()
    include_comments = not args.no_comments

    if not os.path.isfile(args.file):
        sys.stderr.write(f"[outliner] file not found: {args.file}\n")
        return 1

    try:
        nodes = outline(
            source_path=args.file,
            language=args.language,
            include_comments=include_comments,
        )
    except OSError:
        return 1
    except Exception as exc:
        sys.stderr.write(f"[outliner] failed: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        return 1

    if not nodes:
        sys.stderr.write("[outliner] no outline items found\n")
        return 0

    if args.json:
        try:
            print(json.dumps([n.to_dict() for n in nodes], indent=2))
        except Exception as exc:
            sys.stderr.write(f"[outliner] JSON serialization error: {exc}\n")
            traceback.print_exc(file=sys.stderr)
            return 1
        return 0

    try:
        use_color = not args.no_color and sys.stdout.isatty()
        report = render_plain(
            nodes,
            source_path=args.file,
            use_color=use_color,
            max_comment=args.max_comment,
            include_comments=include_comments,
        )
        print(report)
    except Exception as exc:
        sys.stderr.write(f"[outliner] render error: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        # Fallback: plain dump so output is never lost
        for n in nodes:
            print(f"L{n.line}\t{n.kind}\t{n.name}")

    return 0


if __name__ == "__main__":
    try:
        sys.exit(_cli())
    except KeyboardInterrupt:
        sys.stderr.write("\n[outliner] interrupted\n")
        sys.exit(130)
    except Exception as exc:
        sys.stderr.write(f"[outliner] fatal: {exc}\n")
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)
