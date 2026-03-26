[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$LoveArgs
)

$ErrorActionPreference = "Stop"

$Dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$env:CODE_REVIEW_DIR = $Dir

function Resolve-Python {
    $py = Get-Command py -ErrorAction SilentlyContinue
    if ($py) {
        return @{ Cmd = $py.Source; PrefixArgs = @("-3") }
    }

    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return @{ Cmd = $python.Source; PrefixArgs = @() }
    }

    $python3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($python3) {
        return @{ Cmd = $python3.Source; PrefixArgs = @() }
    }

    return $null
}

$loveCmd = Get-Command love -ErrorAction SilentlyContinue
$loveExe = if ($loveCmd) { $loveCmd.Source } else { $null }

if (-not $loveExe) {
    $loveCandidates = @(
        "C:\Program Files\LOVE\love.exe",
        "C:\Program Files (x86)\LOVE\love.exe",
        (Join-Path $env:LOCALAPPDATA "Programs\LOVE\love.exe")
    )
    foreach ($candidate in $loveCandidates) {
        if (Test-Path $candidate) {
            $loveExe = $candidate
            break
        }
    }
}

if (-not $loveExe) {
    Write-Error "Error: love2d not found. Install Love2D from https://love2d.org and ensure 'love.exe' is available."
    exit 1
}

$pythonInfo = Resolve-Python
if (-not $pythonInfo) {
    Write-Error "Error: Python 3 not found (tried: py -3, python, python3)."
    exit 1
}

# Export the resolved Python command so Love2D/Lua can use it via os.getenv("CODE_REVIEW_PYTHON")
$pythonPrefix = if ($pythonInfo.PrefixArgs.Count -gt 0) { " " + ($pythonInfo.PrefixArgs -join " ") } else { "" }
$env:CODE_REVIEW_PYTHON = $pythonInfo.Cmd + $pythonPrefix

# Export the Python sqlite3.dll path so the Lua history module can load it via FFI.
$pythonDllsDir = Join-Path (Split-Path $pythonInfo.Cmd -Parent) "DLLs"
$sqliteDll = Join-Path $pythonDllsDir "sqlite3.dll"
if (Test-Path $sqliteDll) {
    $env:CODE_REVIEW_SQLITE3 = $sqliteDll
}

$reportsDir = Join-Path $Dir "reports"
if (-not (Test-Path $reportsDir)) {
    New-Item -ItemType Directory -Path $reportsDir | Out-Null
}

$scanConfigPath = Join-Path $Dir "scan_config.py"
$fallbackScanDir = (Resolve-Path (Join-Path $Dir "..\..")).Path
$defaultScanDir = $fallbackScanDir

try {
    $pyArgs = $pythonInfo.PrefixArgs + @($scanConfigPath, "default-scan-path")
    $result = & $pythonInfo.Cmd @pyArgs 2>$null
    if ($LASTEXITCODE -eq 0 -and $result) {
        $line = ($result | Select-Object -First 1).ToString().Trim()
        if ($line) {
            $defaultScanDir = $line
        }
    }
} catch {
    $defaultScanDir = $fallbackScanDir
}

# SDL2 hints: pass focus-acquiring clicks through to the app (fixes first-click
# not registering on Windows), and force logical DPI coordinates so hit areas
# match rendering at any display scale factor.
$env:SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH = "1"
$env:SDL_HINT_VIDEO_HIGHDPI_DISABLED   = "1"

Write-Host "Starting Code Review TUI..."
Write-Host "  Scan dir default: $defaultScanDir"
Write-Host "  Reports dir:      $reportsDir"
Write-Host ""

$reviewTuiDir = Join-Path $Dir "review_tui"
& $loveExe $reviewTuiDir @LoveArgs
exit $LASTEXITCODE
