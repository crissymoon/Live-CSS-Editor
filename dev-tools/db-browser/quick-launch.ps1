# Crissy's DB Browser - Quick Launch (Windows)
# Builds if needed, then launches the browser.

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Definition
$DbPath      = Join-Path $ScriptDir "..\..\page-builder\xcm-editor.db"
$BinDir      = Join-Path $ScriptDir "build\bin"
$BrowserBin  = Join-Path $BinDir "db-browser.exe"
$CssSrc      = Join-Path $ScriptDir "css"
$CssDst      = Join-Path $BinDir "css"

# Locate MSYS2 for GTK3 DLLs
$msys2Candidates = @(
    "C:\msys64",
    "C:\msys2",
    "$env:LOCALAPPDATA\msys2\msys64",
    "$env:LOCALAPPDATA\Programs\msys2"
)
$MSYS2_ROOT = $msys2Candidates | Where-Object { Test-Path "$_\usr\bin\bash.exe" } | Select-Object -First 1
$MINGW64_BIN = if ($MSYS2_ROOT) { "$MSYS2_ROOT\mingw64\bin" } else { $null }

# Add MSYS2 MinGW64 DLLs to PATH so GTK3 can be found at runtime
if ($MINGW64_BIN -and ($env:PATH -notlike "*$MINGW64_BIN*")) {
    $env:PATH = "$MINGW64_BIN;$env:PATH"
}

# Build if the binary is missing
if (-not (Test-Path $BrowserBin)) {
    Write-Host "[INFO] Browser not built yet. Running setup..."
    & (Join-Path $ScriptDir "setup.ps1")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[ERROR] Build failed."
        exit 1
    }
    Write-Host ""
}

New-Item -ItemType Directory -Force -Path $CssDst | Out-Null
foreach ($f in @("theme.css", "theme-simple.css")) {
    $src = Join-Path $CssSrc $f
    if (Test-Path $src) {
        Copy-Item -Force $src (Join-Path $CssDst $f)
    }
}

& $BrowserBin $DbPath
if ($LASTEXITCODE -ne 0) {
    Write-Error "[ERROR] Browser exited with error"
    exit 1
}
