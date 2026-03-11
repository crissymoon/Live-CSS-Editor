#!/usr/bin/env python3
"""Minimal MCP stdio smoke test for prompt_inj_guard server.

Validates:
  1) initialize
  2) tools/list contains expected tools
  3) tools/call guard_health returns status payload
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


def _send(proc: subprocess.Popen[bytes], payload: dict[str, Any]) -> None:
    raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    header = f"Content-Length: {len(raw)}\r\n\r\n".encode("ascii")
    assert proc.stdin is not None
    proc.stdin.write(header)
    proc.stdin.write(raw)
    proc.stdin.flush()


def _read(proc: subprocess.Popen[bytes]) -> dict[str, Any]:
    assert proc.stdout is not None
    headers: dict[str, str] = {}
    while True:
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError("No response from MCP server")
        if line in (b"\r\n", b"\n"):
            break
        key, value = line.decode("ascii", errors="replace").split(":", 1)
        headers[key.strip().lower()] = value.strip()

    length = int(headers.get("content-length", "0"))
    if length <= 0:
        raise RuntimeError("Invalid MCP response Content-Length")
    body = proc.stdout.read(length)
    return json.loads(body.decode("utf-8"))


def _rpc(proc: subprocess.Popen[bytes], request_id: int, method: str, params: dict[str, Any]) -> dict[str, Any]:
    _send(
        proc,
        {
            "jsonrpc": "2.0",
            "id": request_id,
            "method": method,
            "params": params,
        },
    )
    msg = _read(proc)
    if "error" in msg:
        raise RuntimeError(f"RPC {method} failed: {msg['error']}")
    return msg


def run(command: list[str]) -> int:
    proc = subprocess.Popen(
        command,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        init = _rpc(
            proc,
            1,
            "initialize",
            {
                "protocolVersion": "2024-11-05",
                "capabilities": {},
                "clientInfo": {"name": "mcp-smoke-test", "version": "1.0.0"},
            },
        )
        server_name = init.get("result", {}).get("serverInfo", {}).get("name", "?")
        print(f"initialize: ok (server={server_name})")

        # Optional notification; server may ignore it.
        _send(proc, {"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}})

        tools = _rpc(proc, 2, "tools/list", {})
        listed = {tool.get("name") for tool in tools.get("result", {}).get("tools", [])}
        required = {"classify_text", "classify_bulk", "guard_health"}
        missing = sorted(required - listed)
        if missing:
            raise RuntimeError(f"tools/list missing: {missing}")
        print("tools/list: ok")

        health = _rpc(
            proc,
            3,
            "tools/call",
            {"name": "guard_health", "arguments": {}},
        )
        result = health.get("result", {})
        structured = result.get("structuredContent")
        if not isinstance(structured, dict):
            raise RuntimeError("guard_health returned no structuredContent")
        if "rule_guard_loaded" not in structured:
            raise RuntimeError("guard_health missing expected keys")
        print("tools/call guard_health: ok")
        print(json.dumps(structured, indent=2, ensure_ascii=False))
        return 0

    finally:
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except Exception:
            proc.kill()


def main() -> int:
    parser = argparse.ArgumentParser(description="MCP smoke test for prompt_inj_guard")
    parser.add_argument(
        "--command",
        nargs="+",
        help="Server command to run, for example: ./dist/prompt-inj-guard-mcp",
    )
    args = parser.parse_args()

    if args.command:
        command = args.command
    else:
        root = Path(__file__).resolve().parent
        binary = root / "dist" / "prompt-inj-guard-mcp"
        if binary.exists():
            command = [str(binary)]
        else:
            command = [sys.executable, str(root / "mcp_guard_server.py")]

    print("running:", " ".join(command))
    return run(command)


if __name__ == "__main__":
    raise SystemExit(main())
