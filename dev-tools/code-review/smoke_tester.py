#!/usr/bin/env python3
"""Run smoke tools listed in the root smoke-tools manifest.

This is intended for future code-review automation and agent workflows.
"""

from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


@dataclass
class SmokeResult:
    tool_id: str
    ok: bool
    exit_code: int
    elapsed_s: float
    command: list[str]
    cwd: str
    note: str


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def load_manifest(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or "tools" not in data:
        raise ValueError("manifest must be an object with a tools array")
    if not isinstance(data["tools"], list):
        raise ValueError("manifest.tools must be an array")
    return data


def host_platform_key() -> str:
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    return "linux"


def tool_matches_platform(tool: dict[str, Any], key: str) -> bool:
    targets = tool.get("platforms", ["windows", "macos", "linux"])
    return isinstance(targets, list) and key in [str(x).lower() for x in targets]


def select_tools(manifest: dict[str, Any], tool_ids: list[str], run_all: bool) -> list[dict[str, Any]]:
    tools = [t for t in manifest["tools"] if isinstance(t, dict)]
    if run_all:
        return tools

    if not tool_ids:
        # Safe default: run the new auth smoke that validates login + security probes.
        tool_ids = ["xcm_auth_guard_login_smoke"]

    by_id = {str(t.get("id")): t for t in tools}
    selected = []
    for tid in tool_ids:
        t = by_id.get(tid)
        if t is None:
            raise ValueError(f"tool id not found in manifest: {tid}")
        selected.append(t)
    return selected


def run_tool(tool: dict[str, Any], root: Path, timeout_s: int, dry_run: bool) -> SmokeResult:
    import time

    tool_id = str(tool.get("id", "unknown"))
    command = [str(x) for x in tool.get("command", [])]
    if not command:
        return SmokeResult(tool_id, False, 2, 0.0, [], str(root), "missing command array")

    cwd_rel = str(tool.get("cwd", "."))
    cwd = (root / cwd_rel).resolve()
    note = str(tool.get("description", ""))

    if dry_run:
        return SmokeResult(tool_id, True, 0, 0.0, command, str(cwd), f"dry-run: {note}")

    t0 = time.monotonic()
    try:
        proc = subprocess.run(
            command,
            cwd=str(cwd),
            timeout=timeout_s,
            check=False,
            text=True,
            capture_output=True,
        )
        elapsed = round(time.monotonic() - t0, 3)
        ok = proc.returncode == 0
        tail = "\n".join((proc.stdout + "\n" + proc.stderr).splitlines()[-12:]).strip()
        return SmokeResult(tool_id, ok, proc.returncode, elapsed, command, str(cwd), tail)
    except subprocess.TimeoutExpired:
        elapsed = round(time.monotonic() - t0, 3)
        return SmokeResult(tool_id, False, 124, elapsed, command, str(cwd), "timeout")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run smoke tools listed in smoke-tools.json")
    parser.add_argument("--manifest", default="smoke-tools.json", help="manifest path from repo root")
    parser.add_argument("--tool", action="append", default=[], help="tool id to run (repeatable)")
    parser.add_argument("--all", action="store_true", help="run all tools in manifest")
    parser.add_argument("--timeout", type=int, default=180, help="timeout seconds per tool")
    parser.add_argument("--dry-run", action="store_true", help="show selected tools without executing")
    parser.add_argument("--report", default="dev-tools/code-review/reports/smoke_report.json", help="report output path from repo root")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()
    manifest_path = (root / args.manifest).resolve()

    if not manifest_path.is_file():
        print(f"[smoke_tester] manifest not found: {manifest_path}", file=sys.stderr)
        return 2

    manifest = load_manifest(manifest_path)
    selected = select_tools(manifest, args.tool, args.all)

    platform_key = host_platform_key()
    selected = [t for t in selected if tool_matches_platform(t, platform_key)]

    if not selected:
        print("[smoke_tester] no tools selected for this platform")
        return 0

    results: list[SmokeResult] = []
    for tool in selected:
        result = run_tool(tool, root, args.timeout, args.dry_run)
        results.append(result)
        status = "PASS" if result.ok else "FAIL"
        print(f"[smoke_tester] {status} {result.tool_id} ({result.elapsed_s}s)")

    report_path = (root / args.report).resolve()
    report_path.parent.mkdir(parents=True, exist_ok=True)

    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "python": platform.python_version(),
        },
        "manifest": str(manifest_path),
        "results": [
            {
                "tool_id": r.tool_id,
                "ok": r.ok,
                "exit_code": r.exit_code,
                "elapsed_s": r.elapsed_s,
                "command": r.command,
                "cwd": r.cwd,
                "note": r.note,
            }
            for r in results
        ],
    }
    report_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(f"[smoke_tester] report: {report_path}")

    return 0 if all(r.ok for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
