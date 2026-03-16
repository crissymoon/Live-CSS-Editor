const vscode = require('vscode');
const http = require('http');
const https = require('https');

async function tryCommands(ids) {
  for (const id of ids) {
    try {
      await vscode.commands.executeCommand(id);
      return true;
    } catch (_) {
      // Ignore unsupported command IDs across VS Code versions.
    }
  }
  return false;
}

function getWorkspaceFolder() {
  const folders = vscode.workspace.workspaceFolders || [];
  return folders.length > 0 ? folders[0].uri.fsPath : '';
}

function resolveWorkspaceTokens(text, workspaceFolder) {
  return String(text || '').replace(/\$\{workspaceFolder\}/g, workspaceFolder || '');
}

function getStartupCommand(cfg, workspaceFolder) {
  const isWindows = process.platform === 'win32';
  const key = isWindows ? 'startupCommandWindows' : 'startupCommandUnix';
  const raw = cfg.get(key, '');
  return resolveWorkspaceTokens(raw, workspaceFolder);
}

function getProfileCommand(cfg, workspaceFolder, keyWindows, keyUnix) {
  const isWindows = process.platform === 'win32';
  const key = isWindows ? keyWindows : keyUnix;
  const raw = cfg.get(key, '');
  return resolveWorkspaceTokens(raw, workspaceFolder);
}

function probeUrl(url, timeoutMs) {
  return new Promise((resolve) => {
    let parsed;
    try {
      parsed = new URL(url);
    } catch (_) {
      resolve(false);
      return;
    }

    const mod = parsed.protocol === 'https:' ? https : http;
    const req = mod.request(
      {
        method: 'GET',
        hostname: parsed.hostname,
        port: parsed.port,
        path: parsed.pathname || '/',
        timeout: timeoutMs,
      },
      (res) => {
        res.resume();
        resolve(Boolean(res && res.statusCode));
      }
    );

    req.on('timeout', () => {
      req.destroy();
      resolve(false);
    });

    req.on('error', () => resolve(false));
    req.end();
  });
}

async function waitForUrl(url, totalWaitMs) {
  if (totalWaitMs <= 0) {
    return false;
  }

  const deadline = Date.now() + totalWaitMs;
  while (Date.now() < deadline) {
    const ok = await probeUrl(url, 1200);
    if (ok) {
      return true;
    }
    await new Promise((resolve) => setTimeout(resolve, 500));
  }
  return false;
}

async function openWorkflowUI(workflowUrl, focusChatAfterOpen) {
  try {
    await vscode.commands.executeCommand('workbench.view.explorer');
  } catch (_) {}

  try {
    await vscode.commands.executeCommand('simpleBrowser.show', workflowUrl);
  } catch (err) {
    vscode.window.showErrorMessage('Could not open Simple Browser: ' + (err && err.message ? err.message : String(err)));
    return false;
  }

  if (focusChatAfterOpen) {
    await tryCommands([
      'workbench.action.chat.open',
      'workbench.panel.chat.view.copilot.focus',
      'workbench.view.chat'
    ]);
  }

  vscode.window.setStatusBarMessage('Agent Flow workspace opened in Simple Browser', 4000);
  return true;
}

async function runProfileAndOpen({
  cfg,
  terminalName,
  command,
  url,
  waitMs,
  focusChatAfterOpen,
  workspaceFolder,
}) {
  if (command.trim().length > 0) {
    const terminal = vscode.window.createTerminal({
      name: terminalName,
      cwd: workspaceFolder || undefined,
    });
    terminal.show(false);
    terminal.sendText(command, true);
  }

  if (waitMs > 0) {
    await waitForUrl(url, waitMs);
  }

  await openWorkflowUI(url, focusChatAfterOpen);
}

class QuickOptionItem extends vscode.TreeItem {
  constructor({ label, description, tooltip, command, iconId }) {
    super(label, vscode.TreeItemCollapsibleState.None);
    this.description = description;
    this.tooltip = tooltip;
    this.command = command;
    this.iconPath = new vscode.ThemeIcon(iconId);
  }
}

class AgentFlowQuickOptionsProvider {
  getTreeItem(item) {
    return item;
  }

  getChildren() {
    return [
      new QuickOptionItem({
        label: 'Open Launch Profile',
        description: 'Choose a startup target',
        tooltip: 'Pick between local server, full stack, or PHP-WASM launch profiles.',
        command: {
          command: 'agentFlow.openLaunchProfile',
          title: 'Open Launch Profile',
        },
        iconId: 'list-selection',
      }),
      new QuickOptionItem({
        label: 'Start Local Workflow',
        description: 'Open Agent Flow on :9090',
        tooltip: 'Start the local Agent Flow server and open it in Simple Browser.',
        command: {
          command: 'agentFlow.startAndOpenWorkflow',
          title: 'Start Local Workflow',
        },
        iconId: 'play-circle',
      }),
      new QuickOptionItem({
        label: 'Open Workflow Only',
        description: 'Skip server startup',
        tooltip: 'Open the configured Agent Flow URL without starting a server.',
        command: {
          command: 'agentFlow.openWorkflowWorkspace',
          title: 'Open Workflow Only',
        },
        iconId: 'globe',
      }),
      new QuickOptionItem({
        label: 'Start Full Server Stack',
        description: 'Open HTTPS workflow',
        tooltip: 'Run the full server stack profile and open the HTTPS Agent Flow URL.',
        command: {
          command: 'agentFlow.startFullServerAndOpenWorkflow',
          title: 'Start Full Server Stack',
        },
        iconId: 'server-process',
      }),
      new QuickOptionItem({
        label: 'Start PHP-WASM Demo',
        description: 'Launch browser demo server',
        tooltip: 'Start the PHP-WASM demo server and open its configured URL.',
        command: {
          command: 'agentFlow.startPhpWasmAndOpen',
          title: 'Start PHP-WASM Demo',
        },
        iconId: 'browser',
      }),
      new QuickOptionItem({
        label: 'Open Settings',
        description: 'Review Agent Flow config',
        tooltip: 'Open VS Code Settings filtered to Agent Flow extension settings.',
        command: {
          command: 'agentFlow.openSettings',
          title: 'Open Settings',
        },
        iconId: 'settings-gear',
      }),
    ];
  }
}

function activate(context) {
  const quickOptionsProvider = new AgentFlowQuickOptionsProvider();
  const quickOptionsView = vscode.window.createTreeView('agentFlow.quickOptions', {
    treeDataProvider: quickOptionsProvider,
    showCollapseAll: false,
  });

  const openLaunchProfileCmd = vscode.commands.registerCommand('agentFlow.openLaunchProfile', async () => {
    const pick = await vscode.window.showQuickPick(
      [
        {
          label: 'Agent Flow local server',
          description: 'Start Server and Open Workflow',
          targetCommand: 'agentFlow.startAndOpenWorkflow',
        },
        {
          label: 'Full HTTPS server stack',
          description: 'Start Full Server Stack and Open Workflow',
          targetCommand: 'agentFlow.startFullServerAndOpenWorkflow',
        },
        {
          label: 'PHP-WASM demo server',
          description: 'Start PHP-WASM and Open Demo',
          targetCommand: 'agentFlow.startPhpWasmAndOpen',
        },
      ],
      {
        placeHolder: 'Choose an Agent Flow launch profile',
      }
    );

    if (!pick || !pick.targetCommand) {
      return;
    }

    await vscode.commands.executeCommand(pick.targetCommand);
  });

  const openSettingsCmd = vscode.commands.registerCommand('agentFlow.openSettings', async () => {
    await vscode.commands.executeCommand('workbench.action.openSettings', 'agentFlow');
  });

  const openOnlyCmd = vscode.commands.registerCommand('agentFlow.openWorkflowWorkspace', async () => {
    const cfg = vscode.workspace.getConfiguration('agentFlow');
    const workflowUrl = cfg.get('workflowUrl', 'http://127.0.0.1:9090');
    const focusChatAfterOpen = cfg.get('focusChatAfterOpen', true);

    await openWorkflowUI(workflowUrl, focusChatAfterOpen);
  });

  const startAndOpenCmd = vscode.commands.registerCommand('agentFlow.startAndOpenWorkflow', async () => {
    const cfg = vscode.workspace.getConfiguration('agentFlow');
    const workflowUrl = cfg.get('workflowUrl', 'http://127.0.0.1:9090');
    const focusChatAfterOpen = cfg.get('focusChatAfterOpen', true);
    const startServerOnOpen = cfg.get('startServerOnOpen', true);
    const startupWaitMs = Math.max(0, Number(cfg.get('startupWaitMs', 12000)) || 0);
    const workspaceFolder = getWorkspaceFolder();

    if (startServerOnOpen) {
      const startupCommand = getStartupCommand(cfg, workspaceFolder);
      if (startupCommand.trim().length > 0 || startupWaitMs > 0) {
        await runProfileAndOpen({
          cfg,
          terminalName: 'Agent Flow Server',
          command: startupCommand,
          url: workflowUrl,
          waitMs: startupWaitMs,
          focusChatAfterOpen,
          workspaceFolder,
        });
        return;
      }
    }

    await openWorkflowUI(workflowUrl, focusChatAfterOpen);
  });

  const startFullServerAndOpenCmd = vscode.commands.registerCommand('agentFlow.startFullServerAndOpenWorkflow', async () => {
    const cfg = vscode.workspace.getConfiguration('agentFlow');
    const workspaceFolder = getWorkspaceFolder();
    const focusChatAfterOpen = cfg.get('focusChatAfterOpen', true);
    const fullServerUrl = cfg.get('fullServerUrl', 'https://localhost:8443/dev-tools/agent-flow/');
    const fullServerWaitMs = Math.max(0, Number(cfg.get('fullServerStartupWaitMs', 22000)) || 0);
    const fullServerCommand = getProfileCommand(
      cfg,
      workspaceFolder,
      'fullServerCommandWindows',
      'fullServerCommandUnix'
    );

    await runProfileAndOpen({
      cfg,
      terminalName: 'Agent Flow Full Stack',
      command: fullServerCommand,
      url: fullServerUrl,
      waitMs: fullServerWaitMs,
      focusChatAfterOpen,
      workspaceFolder,
    });
  });

  const startPhpWasmAndOpenCmd = vscode.commands.registerCommand('agentFlow.startPhpWasmAndOpen', async () => {
    const cfg = vscode.workspace.getConfiguration('agentFlow');
    const workspaceFolder = getWorkspaceFolder();
    const focusChatAfterOpen = cfg.get('focusChatAfterOpen', true);
    const phpWasmUrl = cfg.get('phpWasmUrl', 'http://127.0.0.1:8080');
    const phpWasmWaitMs = Math.max(0, Number(cfg.get('phpWasmStartupWaitMs', 12000)) || 0);
    const phpWasmCommand = getProfileCommand(
      cfg,
      workspaceFolder,
      'phpWasmCommandWindows',
      'phpWasmCommandUnix'
    );

    await runProfileAndOpen({
      cfg,
      terminalName: 'Agent Flow PHP-WASM',
      command: phpWasmCommand,
      url: phpWasmUrl,
      waitMs: phpWasmWaitMs,
      focusChatAfterOpen,
      workspaceFolder,
    });
  });

  context.subscriptions.push(
    quickOptionsView,
    openLaunchProfileCmd,
    openSettingsCmd,
    openOnlyCmd,
    startAndOpenCmd,
    startFullServerAndOpenCmd,
    startPhpWasmAndOpenCmd
  );
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
