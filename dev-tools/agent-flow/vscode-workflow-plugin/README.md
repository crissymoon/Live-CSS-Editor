# Agent Flow Workflow Plugin

Lightweight VS Code extension to open Agent Flow inside the editor area via Simple Browser and optionally focus chat.

## Command

- Agent Flow: Open Workflow Workspace
  - Command ID: agentFlow.openWorkflowWorkspace
- Agent Flow: Start Server and Open Workflow
  - Command ID: agentFlow.startAndOpenWorkflow
  - Starts the local workflow server command in a VS Code terminal, waits briefly, then opens the Simple Browser.

## Settings

- agentFlow.workflowUrl
  - Default: http://127.0.0.1:9090
- agentFlow.focusChatAfterOpen
  - Default: true
- agentFlow.startServerOnOpen
  - Default: true
- agentFlow.startupWaitMs
  - Default: 12000
- agentFlow.startupCommandWindows
  - Default uses PowerShell + PHP built-in server rooted at `dev-tools/agent-flow`
- agentFlow.startupCommandUnix
  - Default uses shell + PHP built-in server rooted at `dev-tools/agent-flow`

Both startup commands support `${workspaceFolder}` token replacement.

## Simple install

Install as an unpacked local extension (no marketplace publish required):

1. Windows:
   - Run `pwsh -NoProfile -ExecutionPolicy Bypass -File ./install-local.ps1` from this plugin folder.
2. macOS/Linux:
   - Run `chmod +x ./install-local.sh && ./install-local.sh` from this plugin folder.

Then restart VS Code (or run `Developer: Reload Window`).

## Local usage

1. Open this plugin folder in VS Code.
2. Press F5 to run an Extension Development Host.
3. Run command: Agent Flow: Start Server and Open Workflow.

## Notes

- This extension uses built-in Simple Browser command simpleBrowser.show.
- Chat focus command IDs vary by VS Code and Copilot versions, so it tries multiple IDs.
- The extension icon is sourced from `my_project/ava-moon.png` and copied into `images/ava-moon.png` for packaging.
