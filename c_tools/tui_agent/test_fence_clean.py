#!/usr/bin/env python3
"""Unit tests for fence_clean.strip_fences."""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fence_clean import strip_fences

CASES = [
    (
        "bare wrapper",
        "```\nfunction hello() {}\n```\n",
        "function hello() {}",
    ),
    (
        "python tag",
        "```python\nprint('hi')\n```",
        "print('hi')",
    ),
    (
        "js tag",
        "```js\nconst x = 1;\n```",
        "const x = 1;",
    ),
    (
        "javascript tag",
        "```javascript\nconst x = 1;\n```",
        "const x = 1;",
    ),
    (
        "no fence -- untouched",
        "function hello() {}\n",
        "function hello() {}",
    ),
    (
        "double wrap",
        "```python\n```js\nconst x = 1;\n```\n```",
        "const x = 1;",
    ),
    (
        "leading blank after opener",
        "```python\n\nprint('hi')\n```",
        "print('hi')",
    ),
    (
        "trailing blank before closer",
        "```python\nprint('hi')\n\n```",
        "print('hi')",
    ),
    (
        "internal fences preserved -- do not strip outer because last line is not bare ```",
        "# README\n```python\nhello()\n```\nmore text\n```js\nworld()\n```js-end",
        "# README\n```python\nhello()\n```\nmore text\n```js\nworld()\n```js-end",
    ),
    (
        "multi-line body preserved",
        "```python\ndef add(a, b):\n    return a + b\n\ndef sub(a, b):\n    return a - b\n```",
        "def add(a, b):\n    return a + b\n\ndef sub(a, b):\n    return a - b",
    ),
    (
        "empty inner becomes empty string",
        "```python\n```",
        "",
    ),
]

ok   = 0
fail = 0
for label, src, expected in CASES:
    try:
        result, passes = strip_fences(src)
        result_stripped   = result.strip()
        expected_stripped = expected.strip()
        if result_stripped == expected_stripped:
            print(f"  PASS  {label}  (passes={passes})")
            ok += 1
        else:
            print(f"  FAIL  {label}")
            print(f"         expected: {expected_stripped!r}")
            print(f"         got:      {result_stripped!r}")
            fail += 1
    except Exception as exc:
        print(f"  ERROR {label}: {exc}")
        fail += 1

print()
print(f"{ok} passed, {fail} failed")
sys.exit(0 if fail == 0 else 1)
