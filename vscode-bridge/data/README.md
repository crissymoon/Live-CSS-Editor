# vscode-bridge/data

This directory stores runtime sync files written by the PHP bridge.

These files are auto-created - do not edit them manually.

- `session.json`         Current active editor state pushed from the browser
- `pending-changes.json` Last stylesheet file changed by Copilot/MCP
- `changes-ack.json`     Acknowledgment record from the browser

Both files are transient and are not tracked in git.
