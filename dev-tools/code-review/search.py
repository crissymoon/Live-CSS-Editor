"""
search.py -- project-wide search utility for live-css

Usage:
  python search.py <query> [options]

Options:
  -e, --exact           Literal substring search (default mode)
  -r, --regex           Treat query as a Python regex
  -f, --fuzzy           Fuzzy character-sequence match (difflib)
  -F, --file  <path>    Restrict search to one file (or WATCH_FILES index)
  -d, --folder <path>   Restrict to one folder (or WATCH_FOLDERS index)
      --ext   <.ext>    Filter by file extension, e.g. --ext .py
      --watch           Search only within WATCH_FILES list
      --models          Print AI_MODELS list and exit
      --list-files      Print WATCH_FILES list and exit
      --list-folders    Print WATCH_FOLDERS list and exit
  -m, --max-results <n> Cap total output lines (default: unlimited)
  -i, --ignore-case     Case-insensitive search (default: on)
      --case            Case-sensitive search
  -h, --help            Show this help
"""

# ---------------------------------------------------------------------------
# AI MODELS IN USE
# ---------------------------------------------------------------------------
AI_MODELS = [
    {
        "name": "claude-haiku-4-5-20251001",
        "provider": "Anthropic",
        "default": False,
        "used_in": [
            "dev-tools/c_tools/tui_agent/agent.py",
            "dev-tools/c_tools/tui_agent/project_mode.py",
        ],
    },
    {
        "name": "claude-sonnet-4-5-20250929",
        "provider": "Anthropic",
        "default": True,
        "used_in": [
            "ai/config.json (default_model)",
        ],
    },
    {
        "name": "claude-sonnet-4-6",
        "provider": "Anthropic",
        "default": False,
        "used_in": [
            "ai/config.json (models list)",
        ],
    },
    {
        "name": "claude-opus-4-6",
        "provider": "Anthropic",
        "default": False,
        "used_in": [
            "ai/config.json (models list)",
        ],
    },
    {
        "name": "gpt-4o-mini",
        "provider": "OpenAI",
        "default": False,
        "used_in": [
            "dev-tools/c_tools/tui_agent/convo.py",
            "dev-tools/c_tools/tui_agent/project_mode.py",
            "ai/phrases.php",
        ],
    },
    {
        "name": "gpt-4o",
        "provider": "OpenAI",
        "default": True,
        "used_in": [
            "ai/config.json (default_model)",
        ],
    },
    {
        "name": "gpt-4-turbo",
        "provider": "OpenAI",
        "default": False,
        "used_in": [
            "ai/config.json (models list)",
        ],
    },
    {
        "name": "gpt-3.5-turbo",
        "provider": "OpenAI",
        "default": False,
        "used_in": [
            "ai/config.json (models list)",
        ],
    },
    {
        "name": "deepseek-chat",
        "provider": "DeepSeek",
        "default": True,
        "used_in": [
            "ai/deepseek.php",
            "ai/test-providers.php",
            "ai/config.json (default_model)",
            "security_ck.py (key pattern detection)",
        ],
    },
    {
        "name": "deepseek-reasoner",
        "provider": "DeepSeek",
        "default": False,
        "used_in": [
            "ai/config.json (models list)",
        ],
    },
    {
        "name": "EMI bio-inspired (local)",
        "provider": "local",
        "default": False,
        "used_in": [
            "dev-tools/agent-flow/mood_ck/mood_check.py",
            "dev-tools/agent-flow/mood_ck/emi_model/engine.py",
        ],
    },
]

# ---------------------------------------------------------------------------
# HARDCODED FILE TARGETS (commonly searched files)
# ---------------------------------------------------------------------------
WATCH_FILES = [
    "security_ck.py",
    "py_audit.py",
    "lines_count.py",
    "search.py",
    "dev-tools/c_tools/tui_agent/main.py",
    "dev-tools/c_tools/tui_agent/agent.py",
    "dev-tools/c_tools/tui_agent/convo.py",
    "dev-tools/c_tools/tui_agent/project_mode.py",
    "dev-tools/c_tools/tui_agent/db.py",
    "dev-tools/c_tools/tui_agent/merger.py",
    "dev-tools/agent-flow/mood_ck/mood_check.py",
    "ai/config.json",
    "ai/config.php",
    "ai/phrases.php",
    "ai/deepseek.php",
    "ai/anthropic.php",
    "ai/openai.php",
    "dev-tools/agent-flow/prompt_inj_guard/model/rule_guard.py",
    "email_smoke/smoke_test.py",
    "style.css",
    "index.php",
]

# ---------------------------------------------------------------------------
# HARDCODED FOLDER TARGETS (commonly searched directories)
# ---------------------------------------------------------------------------
WATCH_FOLDERS = [
    "dev-tools/c_tools/tui_agent",
    "dev-tools/c_tools/test",
    "ai",
    "dev-tools/agent-flow/mood_ck",
    "email_smoke",
    "dev-tools/agent-flow/prompt_inj_guard",
    "db-browser",
    "scripts",
    "page-builder/xcm_auth",
    "vscode-bridge",
    "dev-tools/debug-tool",
    "data",
    "css",
    "js",
    "style-sheets",
    "page-builder",
]

# ---------------------------------------------------------------------------
# stdlib imports only -- no external deps required
# ---------------------------------------------------------------------------
import argparse
import difflib
import os
import re
import sys

# ---------------------------------------------------------------------------
# ANSI color helpers with console fallback
# ---------------------------------------------------------------------------
_USE_COLOR = sys.stdout.isatty()

def _c(code: str, text: str) -> str:
    if not _USE_COLOR:
        return text
    return f"\033[{code}m{text}\033[0m"

def red(t):    return _c("31", t)
def green(t):  return _c("32", t)
def yellow(t): return _c("33", t)
def cyan(t):   return _c("36", t)
def bold(t):   return _c("1",  t)
def dim(t):    return _c("2",  t)


def highlight_match(line: str, query: str, mode: str, ignore_case: bool) -> str:
    """Wrap each match in the line with red+bold ANSI codes."""
    if not _USE_COLOR:
        return line
    flags = re.IGNORECASE if ignore_case else 0
    try:
        if mode == "regex":
            pattern = query
        elif mode == "fuzzy":
            return line  # fuzzy is line-level, no intra-line highlight
        else:
            pattern = re.escape(query)
        return re.sub(pattern, lambda m: bold(red(m.group(0))), line, flags=flags)
    except re.error as exc:
        print(f"[warn] highlight regex error: {exc}", file=sys.stderr)
        return line


# ---------------------------------------------------------------------------
# Match logic
# ---------------------------------------------------------------------------
def match_exact(line: str, query: str, ignore_case: bool) -> bool:
    if ignore_case:
        return query.lower() in line.lower()
    return query in line


def match_regex(line: str, pattern: str, ignore_case: bool) -> bool:
    flags = re.IGNORECASE if ignore_case else 0
    try:
        return bool(re.search(pattern, line, flags=flags))
    except re.error as exc:
        print(f"[error] invalid regex: {exc}", file=sys.stderr)
        sys.exit(1)


def match_fuzzy(line: str, query: str, ignore_case: bool, threshold: float = 0.55) -> bool:
    """
    Fuzzy match using difflib SequenceMatcher.
    Checks the full line and also a sliding window of len(query)*2 characters
    to catch embedded matches.
    """
    target = line.lower() if ignore_case else line
    q = query.lower() if ignore_case else query

    # full-line ratio
    ratio = difflib.SequenceMatcher(None, q, target).ratio()
    if ratio >= threshold:
        return True

    # sliding window for short queries inside long lines
    w = max(len(q), 10) * 2
    for start in range(0, max(1, len(target) - w + 1), max(1, len(q) // 2)):
        chunk = target[start : start + w]
        r = difflib.SequenceMatcher(None, q, chunk).ratio()
        if r >= threshold:
            return True

    return False


# ---------------------------------------------------------------------------
# File walker
# ---------------------------------------------------------------------------
def collect_files(
    root: str,
    folder: str | None,
    single_file: str | None,
    watch_only: bool,
    ext_filter: str | None,
) -> list[str]:
    """Return list of absolute paths to search."""
    paths: list[str] = []

    if single_file:
        abs_f = os.path.join(root, single_file) if not os.path.isabs(single_file) else single_file
        if not os.path.isfile(abs_f):
            print(f"[error] file not found: {abs_f}", file=sys.stderr)
            sys.exit(1)
        return [abs_f]

    if watch_only:
        for rel in WATCH_FILES:
            abs_f = os.path.join(root, rel)
            if os.path.isfile(abs_f):
                paths.append(abs_f)
            else:
                print(f"[warn] watch file missing: {abs_f}", file=sys.stderr)
        return paths

    search_root = root
    if folder:
        search_root = os.path.join(root, folder) if not os.path.isabs(folder) else folder
        if not os.path.isdir(search_root):
            print(f"[error] folder not found: {search_root}", file=sys.stderr)
            sys.exit(1)

    # walk the directory tree, skip hidden and .venv
    SKIP_DIRS = {".git", ".venv", "__pycache__", "node_modules", "vendor", "target"}
    for dirpath, dirnames, filenames in os.walk(search_root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith(".")]
        for fname in filenames:
            if ext_filter and not fname.endswith(ext_filter):
                continue
            paths.append(os.path.join(dirpath, fname))

    return sorted(paths)


# ---------------------------------------------------------------------------
# Core search
# ---------------------------------------------------------------------------
def search_files(
    root: str,
    query: str,
    mode: str,
    folder: str | None,
    single_file: str | None,
    watch_only: bool,
    ext_filter: str | None,
    ignore_case: bool,
    max_results: int | None,
) -> int:
    """Run search and print results. Returns total match count."""
    files = collect_files(root, folder, single_file, watch_only, ext_filter)

    total_matches = 0
    total_files = 0
    printed_lines = 0

    for fpath in files:
        try:
            with open(fpath, encoding="utf-8", errors="replace") as fh:
                lines = fh.readlines()
        except OSError as exc:
            print(f"[warn] cannot read {fpath}: {exc}", file=sys.stderr)
            continue

        file_matches: list[tuple[int, str]] = []

        for lineno, raw_line in enumerate(lines, start=1):
            line = raw_line.rstrip("\n")

            matched = False
            if mode == "exact":
                matched = match_exact(line, query, ignore_case)
            elif mode == "regex":
                matched = match_regex(line, query, ignore_case)
            elif mode == "fuzzy":
                matched = match_fuzzy(line, query, ignore_case)

            if matched:
                file_matches.append((lineno, line))

        if file_matches:
            rel = os.path.relpath(fpath, root)
            print(f"\n{bold(cyan(rel))}  {dim(f'({len(file_matches)} match(es))')}")
            total_files += 1
            for lineno, line in file_matches:
                if max_results is not None and printed_lines >= max_results:
                    print(dim("  ... max results reached, stopping."))
                    return total_matches
                highlighted = highlight_match(line, query, mode, ignore_case)
                print(f"  {yellow(str(lineno).rjust(4))}  {highlighted}")
                printed_lines += 1
            total_matches += len(file_matches)

    return total_matches


# ---------------------------------------------------------------------------
# Print helpers for --models / --list-files / --list-folders
# ---------------------------------------------------------------------------
def print_models() -> None:
    print(bold("\nAI Models in use:"))
    prev_provider = None
    for m in AI_MODELS:
        if m["provider"] != prev_provider:
            print(f"\n  {bold(cyan(m['provider']))}")
            prev_provider = m["provider"]
        tag = green(" [default]") if m.get("default") else ""
        print(f"    {yellow(m['name'])}{tag}")
        for path in m["used_in"]:
            print(f"      {dim(path)}")
    print()


def print_watch_files() -> None:
    print(bold("\nWATCH_FILES:"))
    for i, f in enumerate(WATCH_FILES):
        print(f"  {dim(str(i).rjust(2))}  {f}")
    print()


def print_watch_folders() -> None:
    print(bold("\nWATCH_FOLDERS:"))
    for i, d in enumerate(WATCH_FOLDERS):
        print(f"  {dim(str(i).rjust(2))}  {d}")
    print()


# ---------------------------------------------------------------------------
# Argument resolution helpers
# ---------------------------------------------------------------------------
def resolve_watch_file(value: str) -> str:
    """If value is a decimal index, return WATCH_FILES[index], else return as-is."""
    try:
        idx = int(value)
        if 0 <= idx < len(WATCH_FILES):
            return WATCH_FILES[idx]
        print(f"[error] WATCH_FILES index {idx} out of range (0-{len(WATCH_FILES)-1})", file=sys.stderr)
        sys.exit(1)
    except ValueError:
        return value


def resolve_watch_folder(value: str) -> str:
    """If value is a decimal index, return WATCH_FOLDERS[index], else return as-is."""
    try:
        idx = int(value)
        if 0 <= idx < len(WATCH_FOLDERS):
            return WATCH_FOLDERS[idx]
        print(f"[error] WATCH_FOLDERS index {idx} out of range (0-{len(WATCH_FOLDERS)-1})", file=sys.stderr)
        sys.exit(1)
    except ValueError:
        return value


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="live-css project search utility",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("query", nargs="?", help="Search term or pattern")

    mode_group = p.add_mutually_exclusive_group()
    mode_group.add_argument("-e", "--exact",  action="store_true", help="Literal substring (default)")
    mode_group.add_argument("-r", "--regex",  action="store_true", help="Regex pattern")
    mode_group.add_argument("-f", "--fuzzy",  action="store_true", help="Fuzzy match (difflib)")

    p.add_argument("-F", "--file",   metavar="PATH|IDX", help="Restrict to one file (path or WATCH_FILES index)")
    p.add_argument("-d", "--folder", metavar="PATH|IDX", help="Restrict to folder (path or WATCH_FOLDERS index)")
    p.add_argument("--ext",          metavar=".EXT",     help="File extension filter, e.g. .py")
    p.add_argument("--watch",        action="store_true",help="Search only WATCH_FILES")
    p.add_argument("-i", "--ignore-case", action="store_true", default=True, dest="ignore_case",
                   help="Case-insensitive (default: on)")
    p.add_argument("--case",         action="store_false", dest="ignore_case",
                   help="Case-sensitive search")
    p.add_argument("-m", "--max-results", type=int, metavar="N", help="Cap output lines")

    p.add_argument("--models",       action="store_true", help="Print AI_MODELS and exit")
    p.add_argument("--list-files",   action="store_true", help="Print WATCH_FILES and exit")
    p.add_argument("--list-folders", action="store_true", help="Print WATCH_FOLDERS and exit")

    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    # info-only flags
    if args.models:
        print_models()
        return
    if args.list_files:
        print_watch_files()
        return
    if args.list_folders:
        print_watch_folders()
        return

    if not args.query:
        parser.print_help()
        sys.exit(0)

    # determine search mode
    if args.regex:
        mode = "regex"
    elif args.fuzzy:
        mode = "fuzzy"
    else:
        mode = "exact"

    # resolve file / folder args
    single_file = resolve_watch_file(args.file)   if args.file   else None
    folder      = resolve_watch_folder(args.folder) if args.folder else None

    root = os.path.dirname(os.path.abspath(__file__))

    print(
        f"\n{bold('search.py')}  query={yellow(repr(args.query))}"
        f"  mode={cyan(mode)}"
        f"  case={'off' if args.ignore_case else 'on'}"
        + (f"  ext={args.ext}" if args.ext else "")
        + (f"  file={single_file}" if single_file else "")
        + (f"  folder={folder}" if folder else "")
        + (f"  watch=yes" if args.watch else "")
    )

    try:
        total = search_files(
            root=root,
            query=args.query,
            mode=mode,
            folder=folder,
            single_file=single_file,
            watch_only=args.watch,
            ext_filter=args.ext,
            ignore_case=args.ignore_case,
            max_results=args.max_results,
        )
    except KeyboardInterrupt:
        print("\n[interrupted]", file=sys.stderr)
        sys.exit(130)
    except Exception as exc:
        print(f"[error] unexpected error: {exc}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)

    print(
        f"\n{bold('done.')}  {green(str(total))} match(es) found.\n"
    )


if __name__ == "__main__":
    main()
