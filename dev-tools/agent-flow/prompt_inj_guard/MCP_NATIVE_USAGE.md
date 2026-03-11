# prompt_inj_guard as a Native MCP Binary

This project can be packaged as a single binary MCP server for native MCP clients.

## What Gets Bundled

- MCP stdio server entrypoint from `mcp_guard_server.py`
- Rule database `api/pattern_db.json`

## What Stays External

- DistilBERT model weights in `model/spam_injection_model/final`

The binary can still run in rules-only mode if weights are missing.

## Build One-File Binary

From `dev-tools/agent-flow/prompt_inj_guard`:

```bash
chmod +x build_onefile_mcp.sh
./build_onefile_mcp.sh
```

Output:

```bash
dist/prompt-inj-guard-mcp
```

## Native MCP Client Config

Example config snippet:

```json
{
  "mcpServers": {
    "prompt-inj-guard": {
      "command": "/absolute/path/to/prompt-inj-guard-mcp",
      "env": {
        "GUARD_MODEL_DIR": "/absolute/path/to/model/spam_injection_model/final",
        "GUARD_DB_PATH": "/absolute/path/to/pattern_db.json"
      }
    }
  }
}
```

If `GUARD_DB_PATH` is omitted, the bundled `pattern_db.json` is used.

## Exposed MCP Tools

- `classify_text`
  - input: `{ "text": "..." }`
  - returns: combined label, confidence, source, and diagnostics
- `classify_bulk`
  - input: `{ "texts": ["...", "..."] }` (max 64)
  - returns: per-item classification result list
- `guard_health`
  - input: `{}`
  - returns: model/rule load status and active paths

## Smoke Test

You can run the binary directly and let an MCP inspector client connect over stdio.
Then call `guard_health` first to verify model path and load status.

Automated smoke test script:

```bash
python3 mcp_smoke_test.py --command ./dist/prompt-inj-guard-mcp
```

## Release Build with Checksums

```bash
chmod +x release_onefile_mcp.sh
./release_onefile_mcp.sh v1.0.0
```

Output files are written to `release/`:

- `prompt-inj-guard-mcp-v1.0.0-<platform>`
- `prompt-inj-guard-mcp-v1.0.0-<platform>.sha256`
- `prompt-inj-guard-mcp-v1.0.0-<platform>.build-info.txt`

## Linux CI Notes

For reproducible Linux builds in CI, see `LINUX_CI_BUILD_NOTES.md`.
