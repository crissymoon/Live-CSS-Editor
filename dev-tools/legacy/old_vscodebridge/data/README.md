# vscode-bridge/data

This directory stores runtime sync files written by the PHP bridge.
All files here are auto-created. Do not edit them manually.

## Signal / Ack Files (transient, git-ignored)

- `session.json`              Current active editor state pushed from the browser
- `pending-changes.json`      Last stylesheet file changed by Copilot/MCP
- `changes-ack.json`          Browser acknowledged the CSS change
- `html-content.json`         Last HTML push from Copilot/MCP
- `html-ack.json`             Browser acknowledged the HTML push
- `refresh-signal.json`       Refresh preview signal
- `wireframe.json`            Wireframe canvas state
- `wireframe-changes.json`    Last wireframe push signal
- `wireframe-ack.json`        Browser acknowledged the wireframe push
- `project-update-signal.json` A project was saved (by CLI, MCP, or bridge)
- `project-update-ack.json`   Browser acknowledged the project save

## Database (git-ignored)

- `projects.db`   SQLite database storing saved projects and backups
