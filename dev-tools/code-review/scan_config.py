#!/usr/bin/env python3
"""Shared scan configuration loader for the code-review tools."""

from __future__ import annotations

import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
CONFIG_PATH = HERE / "scan_config.json"

DEFAULT_CONFIG = {
    "default_scan_path": "../..",
    "skip_dir_names": [
        ".git",
        ".venv",
        "venv",
        "env",
        "__pycache__",
        "node_modules",
        "vendor",
        "dist",
        "build",
        "target",
        "coverage",
        "reports",
        ".idea",
        ".vscode",
    ],
    "skip_relative_paths": [
        "dev-tools/code-review/reports",
    ],
    "skip_file_names": [
        "package-lock.json",
        "composer.lock",
        "yarn.lock",
        "Cargo.lock",
    ],
}


def _string_list(value: object) -> list[str]:
    if not isinstance(value, list):
        return []
    return [str(item).strip() for item in value if str(item).strip()]


def load_scan_config() -> dict:
    config = {
        "default_scan_path": DEFAULT_CONFIG["default_scan_path"],
        "skip_dir_names": list(DEFAULT_CONFIG["skip_dir_names"]),
        "skip_relative_paths": list(DEFAULT_CONFIG["skip_relative_paths"]),
        "skip_file_names": list(DEFAULT_CONFIG["skip_file_names"]),
    }

    if not CONFIG_PATH.is_file():
        return config

    try:
        raw = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    except Exception:
        return config

    if not isinstance(raw, dict):
        return config

    if isinstance(raw.get("default_scan_path"), str) and raw["default_scan_path"].strip():
        config["default_scan_path"] = raw["default_scan_path"].strip()

    for key in ("skip_dir_names", "skip_relative_paths", "skip_file_names"):
        config[key] = _string_list(raw.get(key, config[key])) or list(config[key])

    return config


def resolve_config_path(path_text: str) -> Path:
    path = Path(path_text).expanduser()
    if not path.is_absolute():
        path = (CONFIG_PATH.parent / path).resolve()
    return path


def default_scan_path() -> str:
    config = load_scan_config()
    resolved = resolve_config_path(config["default_scan_path"])
    if resolved.is_dir():
        return str(resolved)
    fallback = HERE.parent.parent.resolve()
    return str(fallback)


def merge_skip_dir_names(*groups: set[str] | list[str] | tuple[str, ...]) -> set[str]:
    merged = set(load_scan_config()["skip_dir_names"])
    for group in groups:
        merged.update(group)
    return merged


def merge_skip_relative_paths(*groups: set[str] | list[str] | tuple[str, ...]) -> set[str]:
    merged = set(load_scan_config()["skip_relative_paths"])
    for group in groups:
        merged.update(group)
    return {item.strip().strip("/") for item in merged if item and item.strip()}


def merge_skip_file_names(*groups: set[str] | list[str] | tuple[str, ...]) -> set[str]:
    merged = set(load_scan_config()["skip_file_names"])
    for group in groups:
        merged.update(group)
    return merged


def should_skip_relative_path(path: Path, root: Path, skip_relative_paths: set[str]) -> bool:
    if not skip_relative_paths:
        return False
    try:
        rel = path.resolve().relative_to(root.resolve()).as_posix()
    except Exception:
        return False
    return any(rel == item or rel.startswith(item + "/") for item in skip_relative_paths)


def main() -> int:
    if len(sys.argv) == 2 and sys.argv[1] == "default-scan-path":
        print(default_scan_path())
        return 0

    print(CONFIG_PATH)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())