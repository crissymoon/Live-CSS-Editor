#!/usr/bin/env python3
"""
bridge.py  -  Code Review TUI dispatcher
Located at:  dev-tools/code-review/bridge.py

Usage:
    python3 bridge.py <command> <directory>

Commands:
    security_scan   run security_ck.py
    god_funcs       run god_funcs.py
    lines_count     run lines_count.py
    py_audit        run py_audit.py
    code_smells     run code_smells.py
    run_all         run all five in sequence

Output protocol (read by modules/bridge.lua):
    LINE:<text>     display a result line
    ERROR:<text>    display an error line
    REPORT:<path>   path to the written markdown report
    DONE            end marker
"""

import sys
import os
import subprocess
import datetime

HERE = os.path.dirname(os.path.abspath(__file__))
REPORTS_DIR = os.path.join(HERE, "reports")


def out(prefix: str, text: str) -> None:
    """Write a protocol line to stdout immediately."""
    print(f"{prefix}:{text}", flush=True)


def out_line(text: str) -> None:
    out("LINE", text)


def out_error(text: str) -> None:
    out("ERROR", text)


def run_scanner(args: list[str], scan_dir: str, report_lines: list[str]) -> None:
    """
    Execute a scanner script as a subprocess.
    Stream its output as LINE: / ERROR: protocol lines.
    Also collect into report_lines for the final markdown file.
    """
    cmd = [sys.executable] + args
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=scan_dir,
            text=True,
            bufsize=1,
        )
        for line in proc.stdout:
            line = line.rstrip("\n")
            if line:
                out_line(line)
                report_lines.append(line)
        proc.wait()
    except Exception as exc:
        msg = f"Failed to run {args[0]}: {exc}"
        out_error(msg)
        report_lines.append(f"ERROR: {msg}")


def write_report(scan_dir: str, scanner_label: str, lines: list[str]) -> str:
    """Write a timestamped markdown report and return its path."""
    os.makedirs(REPORTS_DIR, exist_ok=True)
    ts   = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    name = scanner_label.lower().replace(" ", "-")
    path = os.path.join(REPORTS_DIR, f"report_{name}_{ts}.md")

    with open(path, "w") as f:
        f.write(f"# Code Review Report - {scanner_label}\n\n")
        f.write(f"**Directory:** `{scan_dir}`  \n")
        f.write(f"**Generated:** {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  \n\n")
        f.write("---\n\n")
        f.write("```\n")
        for ln in lines:
            f.write(ln + "\n")
        f.write("```\n")
    return path


# ------------------------------------------------------------------
# Scanner dispatcher functions
# ------------------------------------------------------------------

def scan_security(scan_dir: str) -> list[str]:
    out_line(f"[ Security Scan ]  {scan_dir}")
    script = os.path.join(HERE, "security_ck.py")
    report_lines: list[str] = []
    run_scanner([script, scan_dir, "--no-color"], scan_dir, report_lines)
    return report_lines


def scan_god_funcs(scan_dir: str) -> list[str]:
    out_line(f"[ God Functions ]  {scan_dir}")
    script = os.path.join(HERE, "god_funcs.py")
    report_lines: list[str] = []
    # god_funcs.py accepts a positional path argument
    run_scanner([script, scan_dir], scan_dir, report_lines)
    return report_lines


def scan_lines_count(scan_dir: str) -> list[str]:
    out_line(f"[ Line Count ]  {scan_dir}")
    script = os.path.join(HERE, "lines_count.py")
    report_lines: list[str] = []
    run_scanner([script, scan_dir], scan_dir, report_lines)
    return report_lines


def scan_py_audit(scan_dir: str) -> list[str]:
    out_line(f"[ Py Audit ]  {scan_dir}")
    script = os.path.join(HERE, "py_audit.py")
    report_lines: list[str] = []
    run_scanner([script, "--dir", scan_dir], scan_dir, report_lines)
    return report_lines


def scan_code_smells(scan_dir: str) -> list[str]:
    out_line(f"[ Code Smells ]  {scan_dir}")
    script = os.path.join(HERE, "code_smells.py")
    report_lines: list[str] = []
    run_scanner([script, scan_dir], scan_dir, report_lines)
    return report_lines


def run_all(scan_dir: str) -> list[str]:
    out_line(f"[ Run All Scans ]  {scan_dir}")
    all_lines: list[str] = []

    sections = [
        ("Security Scan",   scan_security),
        ("God Functions",   scan_god_funcs),
        ("Line Count",      scan_lines_count),
        ("Py Audit",        scan_py_audit),
        ("Code Smells",     scan_code_smells),
    ]
    for label, fn in sections:
        out_line("")
        out_line(f"=== {label} ===")
        section_lines = fn(scan_dir)
        all_lines += [f"", f"## {label}", ""] + section_lines

    return all_lines


# ------------------------------------------------------------------
# Entry point
# ------------------------------------------------------------------

COMMANDS = {
    "security_scan": (scan_security,    "Security Scan"),
    "god_funcs":     (scan_god_funcs,   "God Functions"),
    "lines_count":   (scan_lines_count, "Line Count"),
    "py_audit":      (scan_py_audit,    "Py Audit"),
    "code_smells":   (scan_code_smells, "Code Smells"),
    "run_all":       (run_all,          "Full Report"),
}


def main() -> None:
    if len(sys.argv) < 3:
        out_error("Usage: bridge.py <command> <directory>")
        print("DONE", flush=True)
        return

    command   = sys.argv[1]
    scan_dir  = os.path.abspath(sys.argv[2])

    if not os.path.isdir(scan_dir):
        out_error(f"Not a directory: {scan_dir}")
        print("DONE", flush=True)
        return

    if command not in COMMANDS:
        out_error(f"Unknown command: {command}  (valid: {', '.join(COMMANDS)})")
        print("DONE", flush=True)
        return

    fn, label = COMMANDS[command]
    try:
        report_lines = fn(scan_dir)
        report_path  = write_report(scan_dir, label, report_lines)
        out("REPORT", report_path)
    except Exception as exc:
        out_error(f"Bridge error: {exc}")

    print("DONE", flush=True)


if __name__ == "__main__":
    main()
