param(
    [string]$Message = "Update Live CSS Editor"
)

$ErrorActionPreference = "Stop"
$RepoUrl = "https://github.com/crissymoon/Live-CSS-Editor.git"

Set-Location $PSScriptRoot

function Invoke-PythonScript {
    param(
        [Parameter(Mandatory = $true)][string]$ScriptPath,
        [string[]]$Arguments = @(),
        [switch]$AllowFailure
    )

    if (-not (Test-Path $ScriptPath)) {
        Write-Host "--- Skipping missing script: $ScriptPath"
        return $false
    }

    if (Get-Command py -ErrorAction SilentlyContinue) {
        & py -3 $ScriptPath @Arguments | Out-Host
    } elseif (Get-Command python -ErrorAction SilentlyContinue) {
        & python $ScriptPath @Arguments | Out-Host
    } else {
        Write-Host "WARNING: Python not found; skipping $ScriptPath"
        return $false
    }

    if ($LASTEXITCODE -ne 0) {
        if ($AllowFailure) {
            Write-Host "WARNING: script failed (continuing): $ScriptPath"
            return $false
        }
        throw "Script failed: $ScriptPath"
    }

    return $true
}

Write-Host "--- push-win.ps1: working in $PWD"
Write-Host "--- push-win.ps1: remote $RepoUrl"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is not installed or not on PATH."
}
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw "GitHub CLI (gh) is not installed or not on PATH. Install from https://cli.github.com/."
}

if (-not (Test-Path .gitignore)) {
@"
# Environment and secrets
.env
.env.*
*.env
secrets.*
config.local.*

# macOS
.DS_Store
.AppleDouble
.LSOverride

# Editor/IDE directories
.vscode/
.idea/
*.suo
*.ntvs*
*.njsproj
*.sln

# Logs and temp files
*.log
*.tmp
*.swp
*.swo
*~

# PHP cache
/vendor/

# Build output
/dist/
/build/

# Python virtual environments
dev-browser/venv/
dev-browser/.venv/
.venv/

# Python compiled files
__pycache__/
*.pyc
*.pyo
*.pyd

# Tauri build output
**/src-tauri/target/
**/src-tauri/www/
**/src-tauri/gen/
"@ | Set-Content -Encoding utf8 .gitignore
    Write-Host ".gitignore created."
}

if (-not (Test-Path .git)) {
    git init | Out-Host
    Write-Host "Git repository initialized."
}

$hasOrigin = $true
try {
    git remote get-url origin *> $null
} catch {
    $hasOrigin = $false
}

if ($hasOrigin) {
    git remote set-url origin $RepoUrl | Out-Host
    Write-Host "Remote 'origin' updated to $RepoUrl"
} else {
    git remote add origin $RepoUrl | Out-Host
    Write-Host "Remote 'origin' added: $RepoUrl"
}

Write-Host "--- git remote -v:"
git remote -v | Out-Host

if (Test-Path make_readme.py) {
    Write-Host "--- Updating README.md..."
    Invoke-PythonScript -ScriptPath "make_readme.py" | Out-Null
}

if (Test-Path "dev-tools/zyx_planning_and_visuals/make_report.py") {
    Write-Host "--- Generating workspace reports..."
    Invoke-PythonScript -ScriptPath "dev-tools/zyx_planning_and_visuals/make_report.py" -AllowFailure | Out-Null
}

Write-Host "--- Running: git add ."
git add . | Out-Host
Write-Host "--- git add complete"

Write-Host "--- Staged files:"
git diff --cached --name-status | Out-Host

$hasStagedChanges = $true
git diff --cached --quiet
if ($LASTEXITCODE -eq 0) {
    $hasStagedChanges = $false
}

if ($hasStagedChanges) {
    Write-Host "--- Running: git commit"
    git commit -m $Message | Out-Host
} else {
    Write-Host "--- Nothing new to commit."
}

Write-Host "--- Checking GitHub CLI auth"
$authOk = $true
try {
    gh auth status | Out-Host
} catch {
    $authOk = $false
}
if (-not $authOk) {
    throw "gh is not authenticated. Run: gh auth login"
}

Write-Host "--- Running: gh auth setup-git"
gh auth setup-git | Out-Host

$branch = (git rev-parse --abbrev-ref HEAD).Trim()
Write-Host "--- Running: git push -v -u origin $branch"
git push -v -u origin $branch | Out-Host

Write-Host "--- Done: pushed to $RepoUrl ($branch)."
