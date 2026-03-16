#!/usr/bin/env python3
"""
make_readme.py
---------------
Generates the repository root README.md from a small curated map and live
filesystem checks so it stays accurate across refactors.
"""

from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "README.md"

PROJECT_TITLE = "Dev Tools Workspace"

DIR_MAP = [
    ("my_project", "Live HTML/CSS/JS editor with AI agent and VSCode bridge."),
    ("page-builder", "JSON-driven page composer, visual editor, and admin panel."),
    ("dev-tools", "Utilities, scanners, automation, and planning/reporting tools."),
    ("imgui-browser", "Cross-platform native browser shell (C++ / Dear ImGui)."),
    ("server", "Nginx/PHP-FPM startup scripts and local runtime controls."),
    ("db_bridge", "Shared SQLite bridge API (PHP + JS client)."),
]

ROOT_FILE_CANDIDATES = [
    "README.md",
    "make_readme.py",
    "push.sh",
    "push-win.ps1",
    "smoke-tools.json",
    "launcher.json",
]


def _exists(rel: str) -> bool:
    return (ROOT / rel).exists()


def build_header() -> str:
    return "\n".join(
        [
            f"# {PROJECT_TITLE}",
            "",
            "**Author:** Crissy Deutsch",
            "**Company:** XcaliburMoon Web Development",
            "**Website:** https://xcaliburmoon.net/",
            "**License:** MIT",
            "",
            "Monorepo for the style tool, page builder, auth/runtime services, native browser",
            "targets, and internal automation utilities.",
            "",
        ]
    )


def build_quick_start() -> str:
    return "\n".join(
        [
            "## Quick Start",
            "",
            "```bash",
            "# Start style tool",
            "cd my_project",
            "php -S 127.0.0.1:9879 index.php",
            "",
            "# Start full local stack",
            "bash server/start.sh",
            "bash page-builder/pb_admin/start-auth.sh",
            "",
            "# Generate docs/reports and push",
            "bash push.sh",
            "powershell -ExecutionPolicy Bypass -File .\\push-win.ps1",
            "```",
            "",
        ]
    )


def build_project_notes() -> str:
    return "\n".join(
        [
            "## Project Notes",
            "",
            "This repository is meant to be an inspiration and rapid-start workspace for building apps quickly.",
            "It combines experiments, starter flows, and reusable tooling in one growing repo so ideas can move into working prototypes with minimal setup.",
            "",
            "Local auth/database values that appear in some parts of the repo are intended for dev preview and starter workflows only, not for production secrets.",
            "Sensitive material is expected to stay outside the repo root and be linked in locally when needed.",
            "",
        ]
    )


def build_directory_map() -> str:
    rows = [
        "## Directory Map",
        "",
        "| Folder | Description | README |",
        "|--------|-------------|--------|",
    ]
    for folder, desc in DIR_MAP:
        readme_rel = f"{folder}/README.md"
        readme_cell = f"[README]({readme_rel})" if _exists(readme_rel) else "--"
        rows.append(f"| `{folder}/` | {desc} | {readme_cell} |")
    return "\n".join(rows) + "\n"


def build_project_structure() -> str:
    lines = ["## Project Structure", "", "```", f"{ROOT.name}/"]
    for folder, desc in DIR_MAP:
        if _exists(folder):
            lines.append(f"  {folder}/  -- {desc}")
    for filename in ROOT_FILE_CANDIDATES:
        if _exists(filename) and filename != "README.md":
            lines.append(f"  {filename}")
    lines.append("```")
    lines.append("")
    return "\n".join(lines)


def build_automation_section() -> str:
    report_cmd = "python dev-tools/zyx_planning_and_visuals/make_report.py"
    return "\n".join(
        [
            "## Automation",
            "",
            "Push scripts run generation steps before staging commits:",
            "",
            "1. Regenerate root README via `make_readme.py`.",
            "2. Generate report artifacts via `dev-tools/zyx_planning_and_visuals/make_report.py`.",
            "",
            "Manual report run:",
            "",
            "```bash",
            report_cmd,
            "```",
            "",
        ]
    )


def build_license() -> str:
    return "\n".join(
        [
            "## License",
            "",
            "MIT License. See [LICENSE](LICENSE).",
            "",
        ]
    )


def build_timestamp() -> str:
    ts = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    return f"---\n\n*README last generated: {ts}*\n"


def generate() -> str:
    sections = [
        build_header(),
        build_quick_start(),
        build_project_notes(),
        build_directory_map(),
        build_project_structure(),
        build_automation_section(),
        build_license(),
        build_timestamp(),
    ]
    return "\n".join(sections)


def main() -> int:
    content = generate()
    OUT.write_text(content, encoding="utf-8")
    print(f"README.md written ({len(content)} chars) -- {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
