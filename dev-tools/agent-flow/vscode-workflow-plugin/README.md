# Agent Flow Workflow Plugin

Lightweight VS Code extension to open Agent Flow inside the editor area via Simple Browser and optionally focus chat.

## Side panel

- Adds an Agent Flow icon to the VS Code Activity Bar.
- Opens a dedicated Quick Options view with one-click launch actions.

## Command

- Agent Flow: Open Launch Profile
  - Command ID: agentFlow.openLaunchProfile
  - Opens a quick picker with all supported launch profiles.
- Agent Flow: Open Workflow Workspace
  - Command ID: agentFlow.openWorkflowWorkspace
- Agent Flow: Start Server and Open Workflow
  - Command ID: agentFlow.startAndOpenWorkflow
  - Starts the local workflow server command in a VS Code terminal, waits briefly, then opens the Simple Browser.
- Agent Flow: Start Full Server Stack and Open Workflow
  - Command ID: agentFlow.startFullServerAndOpenWorkflow
  - Runs the `server/start.sh` profile command, waits for the configured HTTPS URL, then opens Simple Browser.
- Agent Flow: Start PHP-WASM and Open Demo
  - Command ID: agentFlow.startPhpWasmAndOpen
  - Runs the `imgui-browser/php-wasm-project/server.js` profile command, waits for the configured URL, then opens Simple Browser.
- Agent Flow: Open Settings
  - Command ID: agentFlow.openSettings
  - Opens VS Code settings filtered to Agent Flow configuration.

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
- agentFlow.fullServerUrl
  - Default: https://localhost:8443/dev-tools/agent-flow/
- agentFlow.fullServerStartupWaitMs
  - Default: 22000
- agentFlow.fullServerCommandWindows
  - Default runs `bash server/start.sh` via PowerShell in workspace root
- agentFlow.fullServerCommandUnix
  - Default runs `bash server/start.sh` in workspace root
- agentFlow.phpWasmUrl
  - Default: http://127.0.0.1:8080
- agentFlow.phpWasmStartupWaitMs
  - Default: 12000
- agentFlow.phpWasmCommandWindows
  - Default runs `node server.js 8080` in `imgui-browser/php-wasm-project`
- agentFlow.phpWasmCommandUnix
  - Default runs `node server.js 8080` in `imgui-browser/php-wasm-project`

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
3. Click the Agent Flow icon in the Activity Bar.
4. Use Quick Options in the side panel, or run command: Agent Flow: Open Launch Profile.

## Notes

- This extension uses built-in Simple Browser command simpleBrowser.show.
- Chat focus command IDs vary by VS Code and Copilot versions, so it tries multiple IDs.
- The extension icon is sourced from `my_project/ava-moon.png` and copied into `images/ava-moon.png` for packaging.
