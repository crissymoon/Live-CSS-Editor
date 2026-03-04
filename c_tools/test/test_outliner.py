#!/usr/bin/env python3
"""
Smoke tests for c_tools/outliner.py

Covers:
  - _BlockCommentState (single-line open/close, multi-line, include_comments=False)
  - outline_js   (named func, arrow, method, block comment)
  - outline_php  (function, class, block comment)
  - outline_c    (function, macro, struct, line comment)
  - outline_css  (selector, at-rule, variable, block comment)
  - outline_python (function, class, method, constant, module docstring)
  - outline() dispatcher (language override, unknown language fallback)
  - include_comments=False suppression across all languages

Run:
    python3 c_tools/test/test_outliner.py
"""

import sys
import os

# Allow running from repo root or from c_tools/test/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from c_tools.outliner import (
    Node,
    _BlockCommentState,
    outline_js,
    outline_php,
    outline_c,
    outline_css,
    outline_python,
    outline,
)

PASS = 0
FAIL = 0


def _check(label: str, condition: bool, detail: str = ""):
    global PASS, FAIL
    if condition:
        print(f"  PASS  {label}")
        PASS += 1
    else:
        print(f"  FAIL  {label}" + (f"  ({detail})" if detail else ""))
        FAIL += 1


def _kinds(nodes):
    return [n.kind for n in nodes]


def _names(nodes):
    return [n.name for n in nodes]


def _find(nodes, kind):
    return [n for n in nodes if n.kind == kind]


# ---------------------------------------------------------------------------
# _BlockCommentState
# ---------------------------------------------------------------------------

def test_block_comment_state():
    print("\n-- _BlockCommentState --")

    bc = _BlockCommentState()

    # Single-line open and close on the same line
    hit, node = bc.feed("/* hello world */", 1, 0, True)
    _check("single-line open+close returns hit=True", hit is True)
    _check("single-line close returns a node", node is not None)
    _check("single-line node kind is block_comment", node and node.kind == "block_comment")
    _check("single-line node text is captured", node and "hello world" in node.name)
    _check("state resets after single-line close", not bc.active)

    # Multi-line block comment
    bc2 = _BlockCommentState()
    hit, node = bc2.feed("/** Opens here", 5, 0, True)
    _check("multi-line open: hit=True", hit is True)
    _check("multi-line open: no node yet", node is None)
    _check("active after open", bc2.active)

    hit, node = bc2.feed(" * middle line", 6, 0, True)
    _check("middle line: hit=True", hit is True)
    _check("middle line: no node yet", node is None)

    hit, node = bc2.feed(" * ends here */", 7, 0, True)
    _check("close line: hit=True", hit is True)
    _check("close line: node returned", node is not None)
    _check("close line: node line is block start", node and node.line == 5)
    _check("close line: text captured", node and "Opens here" in node.name and "ends here" in node.name)
    _check("state resets after close", not bc2.active)

    # include_comments=False suppresses the node but still consumes the line
    bc3 = _BlockCommentState()
    hit, node = bc3.feed("/* suppressed */", 1, 0, False)
    _check("include_comments=False: hit=True", hit is True)
    _check("include_comments=False: node is None", node is None)

    # Normal line passes through
    bc4 = _BlockCommentState()
    hit, node = bc4.feed("int x = 1;", 1, 0, True)
    _check("normal line: hit=False", hit is False)
    _check("normal line: node is None", node is None)
    _check("normal line: not active", not bc4.active)


# ---------------------------------------------------------------------------
# outline_js
# ---------------------------------------------------------------------------

JS_SRC = """\
/* Module header
 * description
 */

// single comment

function greet(name) {
    return name;
}

const add = (a, b) => a + b;

class Calc {
    multiply(x, y) {
        return x * y;
    }
}
"""


def test_outline_js():
    print("\n-- outline_js --")
    nodes = outline_js(JS_SRC, include_comments=True)
    kinds = _kinds(nodes)

    _check("block_comment present", "block_comment" in kinds)
    _check("comment present", "comment" in kinds)
    _check("function greet found", any(n.kind == "function" and "greet" in n.name for n in nodes))
    _check("const add found", any(n.kind == "function" and "add" in n.name for n in nodes))
    _check("class Calc found", any(n.kind == "class" and "Calc" in n.name for n in nodes))
    _check("method multiply found", any(n.kind == "method" and "multiply" in n.name for n in nodes))

    # include_comments=False
    nodes_nc = outline_js(JS_SRC, include_comments=False)
    kinds_nc = _kinds(nodes_nc)
    _check("no block_comment when include_comments=False",
           "block_comment" not in kinds_nc and "comment" not in kinds_nc)


# ---------------------------------------------------------------------------
# outline_php
# ---------------------------------------------------------------------------

PHP_SRC = """\
<?php
/**
 * A doc block
 */

class UserService {
    public function getUser($id) {
        return null;
    }

    private function _hash($pw) {
        return md5($pw);
    }
}

function standalone($x, $y) {
    return $x + $y;
}

const VERSION = '1.0';
"""


def test_outline_php():
    print("\n-- outline_php --")
    nodes = outline_php(PHP_SRC, include_comments=True)
    kinds = _kinds(nodes)

    _check("block_comment present", "block_comment" in kinds)
    _check("class UserService found", any(n.kind == "class" and "UserService" in n.name for n in nodes))
    _check("method getUser found", any(n.kind == "function" and "getUser" in n.name for n in nodes))
    _check("method _hash found", any(n.kind == "function" and "_hash" in n.name for n in nodes))
    _check("standalone function found", any(n.kind == "function" and "standalone" in n.name for n in nodes))
    _check("constant VERSION found", any(n.kind == "constant" and "VERSION" in n.name for n in nodes))

    nodes_nc = outline_php(PHP_SRC, include_comments=False)
    _check("no block_comment when include_comments=False",
           "block_comment" not in _kinds(nodes_nc))


# ---------------------------------------------------------------------------
# outline_c
# ---------------------------------------------------------------------------

C_SRC = """\
/* File header */

#define MAX_SIZE 128
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

typedef struct Point {
    int x;
    int y;
} Point;

// Computes the sum
int sum(int a, int b) {
    return a + b;
}

static void reset(Point *p) {
    p->x = 0;
    p->y = 0;
}
"""


def test_outline_c():
    print("\n-- outline_c --")
    nodes = outline_c(C_SRC, include_comments=True)
    kinds = _kinds(nodes)

    _check("block_comment present", "block_comment" in kinds)
    _check("macro MAX_SIZE found", any(n.kind == "macro" and "MAX_SIZE" in n.name for n in nodes))
    _check("macro CLAMP found", any(n.kind == "macro" and "CLAMP" in n.name for n in nodes))
    _check("struct Point found", any(n.kind == "struct" and "Point" in n.name for n in nodes))
    _check("line comment found", "comment" in kinds)
    _check("function sum found", any(n.kind == "function" and "sum" in n.name for n in nodes))
    _check("function reset found", any(n.kind == "function" and "reset" in n.name for n in nodes))

    nodes_nc = outline_c(C_SRC, include_comments=False)
    kinds_nc = _kinds(nodes_nc)
    _check("no comments when include_comments=False",
           "block_comment" not in kinds_nc and "comment" not in kinds_nc)


# ---------------------------------------------------------------------------
# outline_css
# ---------------------------------------------------------------------------

CSS_SRC = """\
/* Base styles */

@import url('reset.css');

@media (max-width: 768px) {
    body {
        font-size: 14px;
    }
}

:root {
    --primary: #005fcc;
    --gap: 1rem;
}

.container {
    display: flex;
}

h1 {
    font-weight: bold;
}

h2 {
    font-weight: bold;
}
"""


def test_outline_css():
    print("\n-- outline_css --")
    nodes = outline_css(CSS_SRC, include_comments=True)
    kinds = _kinds(nodes)

    _check("block_comment present", "block_comment" in kinds)
    _check("at_rule @import found", any(n.kind == "at_rule" and "@import" in n.name for n in nodes))
    _check("at_rule @media found", any(n.kind == "at_rule" and "@media" in n.name for n in nodes))
    _check("selector .container found", any(n.kind == "selector" and ".container" in n.name for n in nodes))
    _check("selector h1 found", any(n.kind == "selector" and "h1" in n.name for n in nodes))
    _check("selector h2 found", any(n.kind == "selector" and "h2" in n.name for n in nodes))
    _check("variable --primary found", any(n.kind == "variable" and "--primary" in n.name for n in nodes))
    _check("variable --gap found", any(n.kind == "variable" and "--gap" in n.name for n in nodes))

    nodes_nc = outline_css(CSS_SRC, include_comments=False)
    _check("no block_comment when include_comments=False",
           "block_comment" not in _kinds(nodes_nc))


# ---------------------------------------------------------------------------
# outline_python
# ---------------------------------------------------------------------------

PY_SRC = '''\
"""Module docstring."""

MAX = 100
Singleton = None

class Animal:
    """An animal."""

    def speak(self) -> str:
        return ""

    @property
    def name(self) -> str:
        return self._name

async def fetch(url: str) -> bytes:
    pass

def _helper(x, y, z):
    pass
'''


def test_outline_python():
    print("\n-- outline_python --")
    nodes = outline_python(PY_SRC, include_comments=True)
    kinds = _kinds(nodes)

    _check("module docstring present", "docstring" in kinds)
    _check("constant MAX found", any(n.kind == "constant" and "MAX" in n.name for n in nodes))
    _check("class Animal found", any(n.kind == "class" and "Animal" in n.name for n in nodes))
    _check("method speak found", any(n.kind == "method" and "speak" in n.name for n in nodes))
    _check("async_function fetch found", any(n.kind == "async_function" and "fetch" in n.name for n in nodes))
    _check("function _helper found", any(n.kind == "function" and "_helper" in n.name for n in nodes))

    nodes_nc = outline_python(PY_SRC, include_comments=False)
    _check("no comment/docstring when include_comments=False",
           all(n.kind not in ("comment", "block_comment") for n in nodes_nc))


# ---------------------------------------------------------------------------
# outline() dispatcher
# ---------------------------------------------------------------------------

def test_outline_dispatcher():
    print("\n-- outline() dispatcher --")

    nodes = outline(text="function foo() {}", language="javascript")
    _check("JS dispatch via language=", any(n.kind == "function" for n in nodes))

    nodes = outline(text="def bar(): pass", language="python")
    _check("Python dispatch via language=", any(n.kind == "function" for n in nodes))

    nodes = outline(text=".x { color: red; }", language="css")
    _check("CSS dispatch via language=", any(n.kind == "selector" for n in nodes))

    nodes = outline(text="function baz() {}", language="unsupported_xyz")
    _check("unknown language returns empty list", nodes == [])


# ---------------------------------------------------------------------------
# Integration: sample file in this directory
# ---------------------------------------------------------------------------

def test_sample_js():
    print("\n-- integration: c_tools/test/sample.js --")
    sample = os.path.join(os.path.dirname(__file__), "sample.js")
    if not os.path.isfile(sample):
        print("  SKIP  sample.js not found")
        return
    nodes = outline(source_path=sample)
    _check("sample.js returns at least one node", len(nodes) > 0)
    _check("sample.js nodes have valid line numbers", all(n.line > 0 for n in nodes))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    test_block_comment_state()
    test_outline_js()
    test_outline_php()
    test_outline_c()
    test_outline_css()
    test_outline_python()
    test_outline_dispatcher()
    test_sample_js()

    total = PASS + FAIL
    print(f"\n{'=' * 40}")
    print(f"Results: {PASS}/{total} passed" + (f"  ({FAIL} FAILED)" if FAIL else ""))
    print(f"{'=' * 40}")
    return 0 if FAIL == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
