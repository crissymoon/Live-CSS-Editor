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

function activate(context) {
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
      if (startupCommand.trim().length > 0) {
        const terminal = vscode.window.createTerminal({
          name: 'Agent Flow Server',
          cwd: workspaceFolder || undefined,
        });
        terminal.show(false);
        terminal.sendText(startupCommand, true);
      }
    }

    if (startServerOnOpen && startupWaitMs > 0) {
      await waitForUrl(workflowUrl, startupWaitMs);
    }

    await openWorkflowUI(workflowUrl, focusChatAfterOpen);
  });

  context.subscriptions.push(openOnlyCmd, startAndOpenCmd);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
