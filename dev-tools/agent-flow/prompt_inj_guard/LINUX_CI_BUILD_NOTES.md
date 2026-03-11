# Linux CI Build Notes for One-File MCP Binary

This document explains how to produce reproducible Linux artifacts for
`prompt-inj-guard-mcp` in CI.

## Build Target

- Artifact: `prompt-inj-guard-mcp`
- Packaging: PyInstaller one-file
- Required runtime: glibc-based Linux

## Reproducibility Controls

1. Pin Python version in CI (for example `3.12.8`).
2. Pin dependency versions in your requirements files where possible.
3. Build inside a fixed Linux runner image.
4. Run from a clean checkout with no uncommitted files.
5. Emit SHA256 checksums and build metadata.

## CI Build Steps

From `dev-tools/agent-flow/prompt_inj_guard`:

```bash
./release_onefile_mcp.sh v1.0.0
python3 mcp_smoke_test.py --command ./release/prompt-inj-guard-mcp-v1.0.0-linux-x86_64
```

## Example GitHub Actions Job

```yaml
name: Build MCP Binary (Linux)

on:
  workflow_dispatch:
  push:
    tags:
      - "v*"

jobs:
  build-linux:
    runs-on: ubuntu-22.04
    defaults:
      run:
        working-directory: dev-tools/agent-flow/prompt_inj_guard

    steps:
      - uses: actions/checkout@v4

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12.8"

      - name: Build release artifact
        run: |
          chmod +x build_onefile_mcp.sh release_onefile_mcp.sh
          ./release_onefile_mcp.sh ${GITHUB_REF_NAME}

      - name: Smoke test binary
        run: |
          python3 mcp_smoke_test.py --command ./release/prompt-inj-guard-mcp-${GITHUB_REF_NAME}-linux-x86_64

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: prompt-inj-guard-mcp-${{ github.ref_name }}-linux-x86_64
          path: |
            dev-tools/agent-flow/prompt_inj_guard/release/prompt-inj-guard-mcp-${{ github.ref_name }}-linux-x86_64
            dev-tools/agent-flow/prompt_inj_guard/release/prompt-inj-guard-mcp-${{ github.ref_name }}-linux-x86_64.sha256
            dev-tools/agent-flow/prompt_inj_guard/release/prompt-inj-guard-mcp-${{ github.ref_name }}-linux-x86_64.build-info.txt
```

## Native MCP Project Use

Use the built Linux binary in your MCP client config as the `command` for the
server. See `MCP_NATIVE_USAGE.md` for configuration examples and exposed tools.
