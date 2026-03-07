#!/usr/bin/env bash
# scan_deps.sh -- list files in this project that reference Python or JavaScript.
#
# Looks for:
#   - .py file extensions
#   - .js file extensions
#   - Python import patterns  (import X, from X import, require("...py"), exec/spawn of python)
#   - JS import patterns      (import X, require("..."), <script src=, loadScript, cdn-loader)
#
# Skips: .git/, build/, vendor/, node_modules/, __pycache__/, legacy/, symlinks

ROOT="$(cd "$(dirname "$0")" && pwd)"

# ── Patterns ──────────────────────────────────────────────────────────
PY_PATTERN='\.py["\x27 )]|[^a-z]python[23]?[^a-z]|import\s+\w|from\s+\w+\s+import|exec\s*\(|subprocess\.(run|Popen|call)|PyQt|AppKit|NSApp'
JS_PATTERN='\.js["\x27 )]|require\s*\(|import\s+.*from\s+|<script\s|loadScript|cdn[-_]loader|\.addEventListener|document\.|window\.'

EXCLUDE_DIRS="\.git|/build/|/vendor/|/node_modules/|/__pycache__/|\.dSYM|/legacy/|/debug/"

# ── Helpers ───────────────────────────────────────────────────────────
section() { printf "\n== %s ==\n" "$1"; }
match_files() {
    local label="$1"
    local pattern="$2"
    local results
    results=$(grep -rlE "$pattern" "$ROOT" \
        | grep -vE "$EXCLUDE_DIRS" \
        | while IFS= read -r f; do [ -L "$f" ] || printf '%s\n' "$f"; done \
        | sort)
    if [ -z "$results" ]; then
        printf "  (none)\n"
    else
        while IFS= read -r f; do
            printf "  %s\n" "${f#"$ROOT"/}"
        done <<< "$results"
    fi
}

echo "Scanning: $ROOT"
echo "Skipping: build/ vendor/ node_modules/ __pycache__ .git legacy/ debug/ symlinks"

section "Files with .py extension"
find "$ROOT" -not -type l -name "*.py" | grep -vE "$EXCLUDE_DIRS" | sort | while IFS= read -r f; do
    printf "  %s\n" "${f#"$ROOT"/}"
done

section "Files with .js extension"
find "$ROOT" -not -type l -name "*.js" | grep -vE "$EXCLUDE_DIRS" | sort | while IFS= read -r f; do
    printf "  %s\n" "${f#"$ROOT"/}"
done

section "Files referencing Python (imports, subprocess, python binary, PyQt, AppKit)"
match_files "python-refs" "$PY_PATTERN"

section "Files referencing JavaScript (import/require, script tags, DOM/window API)"
match_files "js-refs" "$JS_PATTERN"
