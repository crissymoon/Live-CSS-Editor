#!/usr/bin/env python3
"""
make_report.py
---------------
Cross-platform report runner for this repository.

It executes key code-review scanners and writes:
1) a timestamped markdown summary report
2) a timestamped JSON machine-readable report
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass
class CommandResult:
    name: str
    command: list[str]
    returncode: int
    duration_seconds: float
    stdout: str
    stderr: str

    @property
    def ok(self) -> bool:
        return self.returncode == 0


def parse_args() -> argparse.Namespace:
    script_dir = Path(__file__).resolve().parent
    default_root = script_dir.parents[2]
    default_out = script_dir / "reports"

    parser = argparse.ArgumentParser(description="Generate repository quality reports.")
    parser.add_argument("--root", default=str(default_root), help="Repository root path.")
    parser.add_argument("--outdir", default=str(default_out), help="Report output directory.")
    parser.add_argument(
        "--lines-threshold",
        type=int,
        default=1200,
        help="Function line threshold for lines_count.py.",
    )
    parser.add_argument(
        "--skip-create-report",
        action="store_true",
        help="Skip the optional create_report.py execution.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=240,
        help="Per-command timeout in seconds.",
    )
    return parser.parse_args()


def run_command(name: str, command: list[str], cwd: Path, timeout: int) -> CommandResult:
    start = time.perf_counter()
    try:
        proc = subprocess.run(
            command,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
        rc = proc.returncode
        stdout = proc.stdout or ""
        stderr = proc.stderr or ""
    except subprocess.TimeoutExpired as exc:
        rc = 124
        stdout = exc.stdout or ""
        stderr = (exc.stderr or "") + "\nCommand timed out."

    duration = time.perf_counter() - start
    return CommandResult(name, command, rc, duration, stdout, stderr)


def tail_text(text: str, max_lines: int = 60) -> str:
    lines = text.strip().splitlines()
    if not lines:
        return "(no output)"
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(lines[-max_lines:])


def build_commands(root: Path, lines_threshold: int, skip_create_report: bool) -> list[tuple[str, list[str]]]:
    code_review = root / "dev-tools" / "code-review"
    zyx_dir = root / "dev-tools" / "zyx_planning_and_visuals"
    python_exe = sys.executable

    commands: list[tuple[str, list[str]]] = [
        (
            "security_scan",
            [python_exe, str(code_review / "security_ck.py"), str(root), "--no-color"],
        ),
        (
            "lines_count",
            [python_exe, str(code_review / "lines_count.py"), str(root), str(lines_threshold)],
        ),
        (
            "python_audit",
            [python_exe, str(code_review / "py_audit.py"), "--dir", str(root)],
        ),
    ]

    if not skip_create_report:
        commands.append(
            (
                "create_report",
                [python_exe, str(zyx_dir / "create_report.py"), "--root", str(root)],
            )
        )

    return commands


def render_markdown(
    root: Path,
    generated_at: str,
    results: list[CommandResult],
    markdown_path: Path,
    json_path: Path,
) -> str:
    success_count = sum(1 for r in results if r.ok)
    fail_count = len(results) - success_count

    lines: list[str] = [
        f"# Workspace Report ({generated_at})",
        "",
        f"- Root: `{root}`",
        f"- Commands run: {len(results)}",
        f"- Successful: {success_count}",
        f"- Failed: {fail_count}",
        "",
        "## Repository Context",
        "",
        "This repo is an inspiration and rapid-start workspace for building apps and prototypes quickly.",
        "It intentionally mixes starter flows, experiments, and reusable tooling in one growing codebase.",
        "",
        "Some auth and database values in the workspace are for local dev preview and starter setups only, not production secrets.",
        "Sensitive assets should remain outside the repo root and be linked in locally when needed.",
        "",
        "## Artifacts",
        "",
        f"- Markdown: `{markdown_path}`",
        f"- JSON: `{json_path}`",
        "",
        "## Command Results",
        "",
    ]

    for result in results:
        status = "PASS" if result.ok else "FAIL"
        cmd_display = " ".join(result.command)
        lines.extend(
            [
                f"### {result.name} [{status}]",
                "",
                f"- Return code: `{result.returncode}`",
                f"- Duration: `{result.duration_seconds:.2f}s`",
                f"- Command: `{cmd_display}`",
                "",
                "#### stdout (tail)",
                "",
                "```text",
                tail_text(result.stdout),
                "```",
                "",
                "#### stderr (tail)",
                "",
                "```text",
                tail_text(result.stderr),
                "```",
                "",
            ]
        )

    return "\n".join(lines)


def write_reports(outdir: Path, root: Path, results: list[CommandResult]) -> tuple[Path, Path]:
    outdir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d_%H-%M-%S_UTC")

    md_path = outdir / f"workspace_report_{stamp}.md"
    json_path = outdir / f"workspace_report_{stamp}.json"

    md_content = render_markdown(root, stamp, results, md_path, json_path)
    md_path.write_text(md_content, encoding="utf-8")

    payload = {
        "generated_at": stamp,
        "root": str(root),
        "repository_context": {
            "purpose": "Inspiration and rapid-start workspace for quickly building apps and prototypes.",
            "notes": [
                "Starter auth and database values may exist for local dev preview flows.",
                "Sensitive assets are expected to stay outside the repo root and be linked locally when needed.",
            ],
        },
        "results": [
            {
                "name": r.name,
                "command": r.command,
                "returncode": r.returncode,
                "ok": r.ok,
                "duration_seconds": round(r.duration_seconds, 3),
                "stdout": r.stdout,
                "stderr": r.stderr,
            }
            for r in results
        ],
    }
    json_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    return md_path, json_path


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()
    outdir = Path(args.outdir).resolve()

    if not root.exists():
        print(f"ERROR: root path does not exist: {root}", file=sys.stderr)
        return 2

    commands = build_commands(root, args.lines_threshold, args.skip_create_report)

    print(f"Generating reports from root: {root}")
    results: list[CommandResult] = []
    for name, command in commands:
        print(f"- Running {name} ...")
        result = run_command(name, command, cwd=root, timeout=args.timeout)
        results.append(result)
        status = "PASS" if result.ok else "FAIL"
        print(f"  -> {status} ({result.duration_seconds:.2f}s, rc={result.returncode})")

    md_path, json_path = write_reports(outdir, root, results)
    print("Report artifacts:")
    print(f"- {md_path}")
    print(f"- {json_path}")

    failures = [r for r in results if not r.ok]
    if failures:
        print(f"Completed with {len(failures)} failed command(s).", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
