#!/usr/bin/env python3
"""
lines_count.py
--------------
Recursively scans a directory for code files that exceed a line threshold.
JSON files, database files, and common binary/asset files are skipped.

Usage:
    python3 lines_count.py                          # scans the current directory
    python3 lines_count.py /path/to/project         # scans a specific directory
    python3 lines_count.py /path/to/project 500     # custom threshold (default: 1000)
"""

import os
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Add any folder names or relative paths here to exclude them from the scan.
# Folder names are matched against every directory encountered anywhere in the
# tree (e.g. "vendor" blocks ALL vendor folders, not just the top-level one).
# Relative paths (with a forward slash) are matched against the path relative
# to the root dir (e.g. "src-tauri/www" blocks that specific subtree only).
#
# Examples:
#   "src-tauri"            -- skip any folder named src-tauri at any depth
#   "src-tauri/www"        -- skip only that specific subtree
#   "style-sheets/themes"  -- skip only that specific subtree
USER_BLOCKED_FOLDERS = [
    "src-tauri",
    "src-tauri/www",
    "style-sheets",
]

# File extensions treated as code files worth counting.
CODE_EXTENSIONS = {
    ".py", ".js", ".ts", ".jsx", ".tsx",
    ".php", ".rb", ".go", ".java", ".c", ".cpp", ".h", ".cs",
    ".swift", ".kt", ".rs", ".scala", ".lua", ".sh", ".bash",
    ".zsh", ".fish", ".ps1", ".pl", ".r",
    ".html", ".htm", ".css", ".scss", ".sass", ".less",
    ".xml", ".yaml", ".yml", ".toml", ".ini", ".cfg", ".conf",
    ".sql", ".graphql", ".gql", ".vue", ".svelte",
    ".md", ".txt",
}

# Extensions to always skip regardless of anything else.
SKIP_EXTENSIONS = {
    # Data / serialised formats
    ".json", ".jsonl", ".ndjson",
    # Databases
    ".db", ".sqlite", ".sqlite3", ".mdb", ".accdb", ".ldb",
    # Compiled / binary
    ".pyc", ".pyo", ".class", ".o", ".obj", ".so", ".dylib", ".dll", ".exe",
    # Archives
    ".zip", ".tar", ".gz", ".bz2", ".xz", ".rar", ".7z",
    # Images
    ".png", ".jpg", ".jpeg", ".gif", ".ico", ".svg", ".webp", ".bmp", ".tiff",
    # Fonts
    ".woff", ".woff2", ".ttf", ".otf", ".eot",
    # Media
    ".mp4", ".mp3", ".wav", ".ogg", ".webm", ".mov",
    # Lock / generated dependency manifests
    ".lock",
}

# Directory names to skip entirely.
SKIP_DIRS = {
    ".git", ".svn", ".hg",
    "node_modules", "vendor", "__pycache__", ".cache",
    ".venv", "venv", "env",
    "dist", "build", "out", ".next", ".nuxt",
    "coverage", ".nyc_output",
    "target",           # Rust / Java build output
    "Pods",             # iOS
    "DerivedData",
}


def should_skip_dir(name):
    return name in SKIP_DIRS or name.startswith(".")


def is_user_blocked(dirpath, root_dir):
    """
    Return True if dirpath matches any entry in USER_BLOCKED_FOLDERS.
    Entries without a slash are matched against the directory's base name
    (blocks that name everywhere in the tree).
    Entries with a slash are matched against the path relative to root_dir
    (blocks only that specific subtree).
    """
    if not USER_BLOCKED_FOLDERS:
        return False
    rel = os.path.relpath(dirpath, root_dir).replace(os.sep, "/")
    base = os.path.basename(dirpath)
    for entry in USER_BLOCKED_FOLDERS:
        entry = entry.strip().strip("/")
        if not entry:
            continue
        if "/" in entry:
            # Relative-path match: exact subtree or any path starting with it.
            if rel == entry or rel.startswith(entry + "/"):
                return True
        else:
            # Name match: block this folder name anywhere in the tree.
            if base == entry:
                return True
    return False


def count_lines(filepath):
    """Return the number of lines in a file, or None if the file cannot be read."""
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as fh:
            return sum(1 for _ in fh)
    except (OSError, PermissionError) as exc:
        print(f"  [warn] could not read {filepath}: {exc}", file=sys.stderr)
        return None


def scan(root_dir, threshold):
    """
    Walk root_dir recursively and return a sorted list of
    (line_count, relative_path) tuples for files over threshold.
    """
    results = []

    for dirpath, dirnames, filenames in os.walk(root_dir):
        # Prune directories in-place so os.walk does not descend into them.
        dirnames[:] = [
            d for d in dirnames
            if not should_skip_dir(d)
            and not is_user_blocked(os.path.join(dirpath, d), root_dir)
        ]

        for filename in filenames:
            ext = os.path.splitext(filename)[1].lower()

            if ext in SKIP_EXTENSIONS:
                continue

            # Only count recognised code extensions; skip unknown binary blobs.
            if ext not in CODE_EXTENSIONS:
                continue

            full_path = os.path.join(dirpath, filename)
            line_count = count_lines(full_path)
            if line_count is None:
                continue

            if line_count >= threshold:
                rel_path = os.path.relpath(full_path, root_dir)
                results.append((line_count, rel_path))

    results.sort(key=lambda x: x[0], reverse=True)
    return results


def main():
    # Parse arguments
    root_dir  = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
    threshold = int(sys.argv[2]) if len(sys.argv) > 2 else 1000

    if not os.path.isdir(root_dir):
        print(f"Error: '{root_dir}' is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Scanning : {os.path.abspath(root_dir)}")
    print(f"Threshold: {threshold} lines")
    print(f"{'-' * 60}")

    results = scan(root_dir, threshold)

    if not results:
        print(f"No code files found with {threshold}+ lines.")
        return

    # Column widths
    max_lines_width = len(str(results[0][0]))  # widest line count (already sorted desc)
    col_width       = max(max_lines_width, 5)

    print(f"{'Lines':>{col_width}}  File")
    print(f"{'-' * col_width}  {'-' * 50}")

    for line_count, rel_path in results:
        print(f"{line_count:>{col_width}}  {rel_path}")

    print(f"{'-' * 60}")
    print(f"Total files over {threshold} lines: {len(results)}")


if __name__ == "__main__":
    main()
