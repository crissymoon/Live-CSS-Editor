#!/usr/bin/env python3
"""
security_ck.py
--------------
Recursively scans a directory for hardcoded API keys, passwords, tokens,
and other secrets that should be moved to environment variables or a
secrets manager outside of the project directory.

Usage:
    python3 security_ck.py                       # scans the current directory
    python3 security_ck.py /path/to/project      # scans a specific directory
    python3 security_ck.py /path/to/project --json  # output results as JSON

The script prints each finding with file path, line number, pattern label,
and a redacted preview of the matched value. A summary is printed at the end.
"""

import os
import re
import sys
import json
import argparse
import traceback
from pathlib import Path

from scan_config import merge_skip_dir_names, merge_skip_file_names, merge_skip_relative_paths, should_skip_relative_path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Directory names to skip entirely (matched at any depth).
BASE_SKIP_DIRS = {
    ".git",
    ".venv",
    "venv",
    "node_modules",
    "__pycache__",
    "vendor",
    "target",      # Rust build output
    "build",
    "dist",
    ".idea",
    ".vscode",
}

# Specific relative sub-paths to skip (relative to the root scan dir).
# E.g. "src-tauri/target" skips only that subtree.
BASE_SKIP_REL_PATHS = {
    "src-tauri/target",
    "src-tauri/www",
    "db-browser/build",
}

# File extensions to scan (text-based only).
SCAN_EXTENSIONS = {
    ".php", ".js", ".ts", ".py", ".json", ".env", ".sh", ".bash",
    ".yaml", ".yml", ".toml", ".ini", ".cfg", ".conf", ".xml",
    ".html", ".htm", ".txt", ".md", ".rb", ".go", ".java", ".cs",
    ".cpp", ".c", ".h", ".rs", ".sql", ".graphql", ".tf", ".hcl",
}

# Filenames (no extension filter) that are always scanned.
SCAN_FILENAMES = {
    ".env", ".env.local", ".env.production", ".env.staging",
    "Dockerfile", "docker-compose.yml", "docker-compose.yaml",
    "Makefile", ".htaccess",
}

# Filenames to always skip regardless of extension.
BASE_SKIP_FILENAMES = {
    "package-lock.json",
    "composer.lock",
    "yarn.lock",
    "Cargo.lock",
}

# How many characters of a matched secret to show in the preview (then redact).
PREVIEW_CHARS = 6

# ---------------------------------------------------------------------------
# Secret patterns
# Each entry: (label, compiled regex)
# The regex must have at least one capture group for the secret value itself.
# ---------------------------------------------------------------------------

RAW_PATTERNS = [
    # Generic assignment patterns - covers most cases first
    ("Password assignment",
     r"""(?i)(?:password|passwd|pwd|pass)\s*[=:]\s*['"]([^'"]{4,})['"]"""),

    ("Secret key assignment",
     r"""(?i)(?:secret[_-]?key|app[_-]?secret|client[_-]?secret|auth[_-]?secret|signing[_-]?secret)\s*[=:>]+\s*['"]([^'"]{6,})['"]"""),

    ("API key assignment",
     r"""(?i)(?:api[_-]?key|apikey|access[_-]?key|app[_-]?key)\s*[=:>]+\s*['"]([^'"]{8,})['"]"""),

    ("Token assignment",
     r"""(?i)(?:token|auth[_-]?token|bearer[_-]?token|access[_-]?token|refresh[_-]?token|id[_-]?token)\s*[=:>]+\s*['"]([^'"]{8,})['"]"""),

    ("Private key / cert header",
     r"""(-----BEGIN (?:RSA |EC |DSA |OPENSSH )?PRIVATE KEY-----)"""),

    ("AWS access key id",
     r"""\b(AKIA[0-9A-Z]{16})\b"""),

    ("AWS secret access key",
     r"""(?i)aws[_-]?secret[_-]?access[_-]?key\s*[=:]\s*['"]?([A-Za-z0-9+/]{40})['"]?"""),

    ("Stripe secret key",
     r"""\b(sk_(?:live|test)_[0-9a-zA-Z]{24,})\b"""),

    ("Stripe publishable key",
     r"""\b(pk_(?:live|test)_[0-9a-zA-Z]{24,})\b"""),

    ("SendGrid API key",
     r"""\b(SG\.[A-Za-z0-9_\-]{22,}\.[A-Za-z0-9_\-]{43,})\b"""),

    ("Twilio account SID",
     r"""\b(AC[a-fA-F0-9]{32})\b"""),

    ("Twilio auth token",
     r"""(?i)twilio[_-]?auth[_-]?token\s*[=:]\s*['"]?([a-fA-F0-9]{32})['"]?"""),

    ("GitHub token",
     r"""\b(gh[pousr]_[A-Za-z0-9]{36,})\b"""),

    ("Slack token",
     r"""\b(xox[baprs]-[0-9A-Za-z\-]{10,})\b"""),

    ("Slack webhook",
     r"""(https://hooks\.slack\.com/services/T[A-Z0-9]+/B[A-Z0-9]+/[A-Za-z0-9]+)"""),

    ("Google API key",
     r"""\b(AIza[0-9A-Za-z_\-]{35})\b"""),

    ("Firebase URL",
     r"""(https://[a-z0-9-]+\.firebaseio\.com)"""),

    ("Firebase API key",
     r"""(?i)firebase[_-]?api[_-]?key\s*[=:]\s*['"]?(AIza[0-9A-Za-z_\-]{35})['"]?"""),

    ("Anthropic API key",
     r"""\b(sk-ant-[A-Za-z0-9\-_]{30,})\b"""),

    ("OpenAI API key",
     r"""\b(sk-[A-Za-z0-9]{32,})\b"""),

    ("DeepSeek API key",
     r"""(?i)deepseek[_-]?(?:api[_-]?)?key\s*[=:]\s*['"]([^'"]{10,})['"]"""),

    ("Database URL / DSN",
     r"""(?i)(?:database[_-]?url|db[_-]?url|dsn|connection[_-]?string)\s*[=:>]+\s*['"]([^'"]{10,})['"]"""),

    ("Database password",
     r"""(?i)(?:db[_-]?pass(?:word)?|database[_-]?pass(?:word)?|mysql[_-]?pass(?:word)?|pg[_-]?pass(?:word)?)\s*[=:>]+\s*['"]([^'"]{4,})['"]"""),

    ("SMTP / mail password",
     r"""(?i)(?:smtp[_-]?pass(?:word)?|mail[_-]?pass(?:word)?|email[_-]?pass(?:word)?)\s*[=:>]+\s*['"]([^'"]{4,})['"]"""),

    ("JWT secret",
     r"""(?i)jwt[_-]?secret\s*[=:>]+\s*['"]([^'"]{6,})['"]"""),

    ("OAuth client secret",
     r"""(?i)(?:oauth[_-]?client[_-]?secret|client[_-]?secret)\s*[=:>]+\s*['"]([^'"]{6,})['"]"""),

    ("Hardcoded basic auth URL",
     r"""https?://[A-Za-z0-9_\-]+:([^@\s'"]{4,})@[A-Za-z0-9._\-]+"""),

    ("Private key variable",
     r"""(?i)(?:private[_-]?key|rsa[_-]?key|pem[_-]?key)\s*[=:>]+\s*['"]([^'"]{8,})['"]"""),

    ("Generic secret variable",
     r"""(?i)\b(?:secret|credential)\b\s*[=:>]+\s*['"]([^'"]{6,})['"]"""),
]

try:
    PATTERNS = [(label, re.compile(pattern, re.IGNORECASE | re.MULTILINE))
                for label, pattern in RAW_PATTERNS]
except re.error as exc:
    print(f"[security_ck] ERROR compiling patterns: {exc}", file=sys.stderr, flush=True)
    traceback.print_exc()
    sys.exit(1)

# ---------------------------------------------------------------------------
# False-positive filters
# Lines containing these strings are skipped (informational /example values).
# ---------------------------------------------------------------------------

FALSE_POSITIVE_HINTS = [
    "example.com",
    "your_api_key",
    "your_key_here",
    "insert_key",
    "placeholder",
    "changeme",
    "xxxx",
    "****",
    "todo",
    "<api_key>",
    "{api_key}",
    "${",          # env var substitution placeholders like ${SECRET}
    "%(secret)s",
    "process.env",
    "getenv(",
    "os.environ",
    "env('",
    "env(\"",
    "config('",
    "config(\"",
    "__placeholder__",
    "your-secret",
    "your_secret",
    "my_password",
    "my_api_key",
    "test_key",
    "test_pass",
    "null",
    "undefined",
    "empty",
    "replace_me",
    "insert_here",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def redact(value: str) -> str:
    """Show only the first PREVIEW_CHARS characters then mask the rest."""
    if not value:
        return "<empty>"
    if len(value) <= PREVIEW_CHARS:
        return "*" * len(value)
    return value[:PREVIEW_CHARS] + "*" * (len(value) - PREVIEW_CHARS)


def is_false_positive(line: str) -> bool:
    """Return True if the line looks like an example or placeholder."""
    lower = line.lower()
    return any(hint.lower() in lower for hint in FALSE_POSITIVE_HINTS)


def should_skip_dir(dir_path: Path, root: Path) -> bool:
    """Return True if this directory should be excluded from the scan."""
    try:
        name = dir_path.name
        skip_dir_names = merge_skip_dir_names(BASE_SKIP_DIRS)
        skip_relative_paths = merge_skip_relative_paths(BASE_SKIP_REL_PATHS)
        if name in skip_dir_names:
            return True
        if should_skip_relative_path(dir_path, root, skip_relative_paths):
            return True
    except Exception as exc:
        print(f"[security_ck] WARNING: could not evaluate dir skip for {dir_path}: {exc}",
              file=sys.stderr, flush=True)
    return False


def should_scan_file(file_path: Path) -> bool:
    """Return True if this file should be scanned."""
    try:
        if file_path.name in merge_skip_file_names(BASE_SKIP_FILENAMES):
            return False
        if file_path.name in SCAN_FILENAMES:
            return True
        return file_path.suffix.lower() in SCAN_EXTENSIONS
    except Exception as exc:
        print(f"[security_ck] WARNING: could not evaluate file scan flag for {file_path}: {exc}",
              file=sys.stderr, flush=True)
        return False


def scan_file(file_path: Path):
    """
    Scan a single file for secrets.
    Returns a list of dicts: {file, line_no, label, preview, raw_match}.
    """
    findings = []
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except PermissionError as exc:
        print(f"[security_ck] PERMISSION DENIED: {file_path} -- {exc}", file=sys.stderr, flush=True)
        return findings
    except OSError as exc:
        print(f"[security_ck] OS ERROR reading {file_path}: {exc}", file=sys.stderr, flush=True)
        return findings
    except Exception as exc:
        print(f"[security_ck] UNEXPECTED ERROR reading {file_path}: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        return findings

    lines = content.splitlines()
    for line_no, line in enumerate(lines, start=1):
        try:
            if is_false_positive(line):
                continue
            for label, pattern in PATTERNS:
                try:
                    for match in pattern.finditer(line):
                        # Use the first capture group as the secret value.
                        secret_value = match.group(1) if match.lastindex and match.lastindex >= 1 else match.group(0)
                        findings.append({
                            "file": str(file_path),
                            "line_no": line_no,
                            "label": label,
                            "preview": redact(secret_value),
                            "raw_match": match.group(0)[:80],  # cap length for display
                        })
                except re.error as exc:
                    print(f"[security_ck] REGEX ERROR on pattern '{label}' in {file_path}:{line_no}: {exc}",
                          file=sys.stderr, flush=True)
                except Exception as exc:
                    print(f"[security_ck] ERROR matching pattern '{label}' in {file_path}:{line_no}: {exc}",
                          file=sys.stderr, flush=True)
                    traceback.print_exc()
        except Exception as exc:
            print(f"[security_ck] ERROR processing line {line_no} of {file_path}: {exc}", file=sys.stderr, flush=True)
            traceback.print_exc()

    return findings


def walk_directory(root: Path):
    """
    Yield all file paths under root that should be scanned,
    respecting SKIP_DIRS and SKIP_REL_PATHS.
    """
    try:
        for dirpath_str, dirnames, filenames in os.walk(root, topdown=True):
            dirpath = Path(dirpath_str)
            try:
                # Prune skip dirs in-place so os.walk does not descend into them.
                dirnames[:] = [
                    d for d in dirnames
                    if not should_skip_dir(dirpath / d, root)
                ]
            except Exception as exc:
                print(f"[security_ck] WARNING: error pruning dirs under {dirpath}: {exc}",
                      file=sys.stderr, flush=True)
                traceback.print_exc()

            for filename in filenames:
                file_path = dirpath / filename
                try:
                    if should_scan_file(file_path):
                        yield file_path
                except Exception as exc:
                    print(f"[security_ck] WARNING: error checking file {file_path}: {exc}",
                          file=sys.stderr, flush=True)
    except Exception as exc:
        print(f"[security_ck] FATAL: error walking directory {root}: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

SEVERITY_ORDER = {
    "Private key / cert header": "CRITICAL",
    "AWS access key id": "CRITICAL",
    "AWS secret access key": "CRITICAL",
    "Anthropic API key": "HIGH",
    "OpenAI API key": "HIGH",
    "Stripe secret key": "HIGH",
    "Stripe publishable key": "MEDIUM",
    "SendGrid API key": "HIGH",
    "Twilio auth token": "HIGH",
    "Twilio account SID": "MEDIUM",
    "GitHub token": "HIGH",
    "Slack token": "HIGH",
    "Slack webhook": "MEDIUM",
    "Google API key": "HIGH",
    "Firebase API key": "HIGH",
    "Firebase URL": "LOW",
    "DeepSeek API key": "HIGH",
    "JWT secret": "HIGH",
    "OAuth client secret": "HIGH",
    "Secret key assignment": "HIGH",
    "API key assignment": "HIGH",
    "Token assignment": "HIGH",
    "Private key variable": "HIGH",
    "Password assignment": "MEDIUM",
    "Database URL / DSN": "HIGH",
    "Database password": "HIGH",
    "SMTP / mail password": "MEDIUM",
    "Hardcoded basic auth URL": "HIGH",
    "Generic secret variable": "LOW",
}

SEVERITY_COLOR = {
    "CRITICAL": "\033[1;31m",  # bold red
    "HIGH":     "\033[0;31m",  # red
    "MEDIUM":   "\033[0;33m",  # yellow
    "LOW":      "\033[0;36m",  # cyan
}
RESET = "\033[0m"


def severity_of(label: str) -> str:
    return SEVERITY_ORDER.get(label, "LOW")


def print_findings(all_findings: list, use_color: bool = True) -> None:
    if not all_findings:
        print("\n[security_ck] No secrets detected.", flush=True)
        return

    # Group by severity for the summary
    severity_counts = {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0, "LOW": 0}

    print("\n" + "=" * 70, flush=True)
    print(" SECURITY SCAN FINDINGS", flush=True)
    print("=" * 70, flush=True)

    current_file = None
    for finding in all_findings:
        sev = severity_of(finding["label"])
        severity_counts[sev] += 1
        color = SEVERITY_COLOR.get(sev, "") if use_color else ""
        reset = RESET if use_color else ""

        if finding["file"] != current_file:
            current_file = finding["file"]
            print(f"\n  FILE: {current_file}", flush=True)

        print(
            f"    Line {finding['line_no']:>5}  "
            f"{color}[{sev}]{reset}  "
            f"{finding['label']}"
            f"  ->  {finding['preview']}",
            flush=True,
        )

    print("\n" + "-" * 70, flush=True)
    print(" SUMMARY", flush=True)
    print("-" * 70, flush=True)
    total = sum(severity_counts.values())
    print(f"  Total findings : {total}", flush=True)
    for sev in ("CRITICAL", "HIGH", "MEDIUM", "LOW"):
        color = SEVERITY_COLOR.get(sev, "") if use_color else ""
        reset = RESET if use_color else ""
        count = severity_counts[sev]
        if count:
            print(f"  {color}{sev:<10}{reset}: {count}", flush=True)

    print("=" * 70, flush=True)
    print("\nRECOMMENDATIONS:", flush=True)
    print("  - Move secrets to environment variables or a .env file.", flush=True)
    print("  - Add .env to .gitignore so it is never committed.", flush=True)
    print("  - Use a secrets manager (e.g. AWS Secrets Manager, Vault, Doppler).", flush=True)
    print("  - Rotate any keys that may have already been exposed.", flush=True)
    print("  - Review each finding manually; some may be intentional test values.", flush=True)
    print(flush=True)


def output_json(all_findings: list, root: Path) -> None:
    try:
        enriched = []
        for f in all_findings:
            enriched.append({**f, "severity": severity_of(f["label"])})
        result = {
            "scan_root": str(root),
            "total": len(enriched),
            "findings": enriched,
        }
        print(json.dumps(result, indent=2), flush=True)
    except Exception as exc:
        print(f"[security_ck] ERROR serializing JSON output: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Scan a project directory for hardcoded API keys and secrets.",
    )
    parser.add_argument(
        "directory",
        nargs="?",
        default=".",
        help="Root directory to scan (default: current directory).",
    )
    parser.add_argument(
        "--json",
        dest="output_json",
        action="store_true",
        default=False,
        help="Output results as JSON instead of human-readable text.",
    )
    parser.add_argument(
        "--no-color",
        dest="no_color",
        action="store_true",
        default=False,
        help="Disable ANSI color codes in output.",
    )

    try:
        args = parser.parse_args()
    except SystemExit:
        raise
    except Exception as exc:
        print(f"[security_ck] ERROR parsing arguments: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    try:
        root = Path(args.directory).resolve()
        if not root.exists():
            print(f"[security_ck] ERROR: directory does not exist: {root}", file=sys.stderr, flush=True)
            sys.exit(1)
        if not root.is_dir():
            print(f"[security_ck] ERROR: path is not a directory: {root}", file=sys.stderr, flush=True)
            sys.exit(1)
    except Exception as exc:
        print(f"[security_ck] ERROR resolving directory path: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    print(f"[security_ck] Loaded {len(PATTERNS)} detection patterns.", file=sys.stderr, flush=True)
    if not args.output_json:
        print(f"[security_ck] Scanning: {root}", flush=True)
    else:
        print(f"[security_ck] Scanning: {root}", file=sys.stderr, flush=True)

    all_findings = []
    files_scanned = 0
    files_errored = 0

    try:
        for file_path in walk_directory(root):
            try:
                findings = scan_file(file_path)
                all_findings.extend(findings)
                files_scanned += 1
                if files_scanned % 50 == 0:
                    print(f"[security_ck] ...scanned {files_scanned} files so far",
                          file=sys.stderr, flush=True)
            except Exception as exc:
                files_errored += 1
                print(f"[security_ck] ERROR scanning file {file_path}: {exc}", file=sys.stderr, flush=True)
                traceback.print_exc()
    except Exception as exc:
        print(f"[security_ck] FATAL error during directory walk: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    print(f"[security_ck] Scan complete. Files scanned: {files_scanned}, errors: {files_errored}",
          file=sys.stderr, flush=True)

    # Sort findings by severity then file then line
    severity_rank = {"CRITICAL": 0, "HIGH": 1, "MEDIUM": 2, "LOW": 3}
    try:
        all_findings.sort(
            key=lambda f: (
                severity_rank.get(severity_of(f["label"]), 99),
                f["file"],
                f["line_no"],
            )
        )
    except Exception as exc:
        print(f"[security_ck] WARNING: could not sort findings: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()

    if args.output_json:
        output_json(all_findings, root)
    else:
        use_color = not args.no_color and sys.stdout.isatty()
        print_findings(all_findings, use_color=use_color)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[security_ck] Interrupted by user.", file=sys.stderr, flush=True)
        sys.exit(130)
    except Exception as exc:
        print(f"[security_ck] UNHANDLED EXCEPTION: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)
