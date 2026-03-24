# Crissy's DB Browser - Setup (Windows / MSYS2)
# Run this once to install dependencies and build.

$ErrorActionPreference = "Stop"

Write-Host "============================================================"
Write-Host "  Crissy's DB Browser - Setup (Windows)"
Write-Host "============================================================"
Write-Host ""

# --- Locate MSYS2 ----------------------------------------------------------
$msys2Candidates = @(
    "C:\msys64",
    "C:\msys2",
    "$env:LOCALAPPDATA\msys2\msys64",
    "$env:LOCALAPPDATA\Programs\msys2"
)
$MSYS2_ROOT = $msys2Candidates | Where-Object { Test-Path "$_\usr\bin\bash.exe" } | Select-Object -First 1

if (-not $MSYS2_ROOT) {
    Write-Host "[WARN] MSYS2 not found in common locations."
    Write-Host ""
    Write-Host "  Install it with WinGet:"
    Write-Host "    winget install MSYS2.MSYS2"
    Write-Host ""
    Write-Host "  Then re-run this script."
    exit 1
}

$bash = "$MSYS2_ROOT\usr\bin\bash.exe"
Write-Host "[OK] MSYS2 found: $MSYS2_ROOT"
Write-Host ""

# Helper: run a command inside the MSYS2 MINGW64 shell
function Invoke-MSYS2 {
    param([string]$Cmd, [switch]$PassThru)
    $env:MSYSTEM = "MINGW64"
    if ($PassThru) {
        & $bash --login -c $Cmd
        return $LASTEXITCODE
    }
    & $bash --login -c $Cmd
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[ERROR] MSYS2 command failed: $Cmd"
        exit 1
    }
}

# --- Install required packages via pacman ----------------------------------
Write-Host "[STEP 1] Installing build dependencies via pacman..."
$packages = @(
    "mingw-w64-x86_64-gcc",
    "mingw-w64-x86_64-make",
    "mingw-w64-x86_64-pkg-config",
    "mingw-w64-x86_64-gtk3",
    "mingw-w64-x86_64-sqlite3",
    "mingw-w64-x86_64-openssl"
)
$pkgList = $packages -join " "
Invoke-MSYS2 "pacman -S --noconfirm --needed $pkgList"
Write-Host "[OK] Dependencies installed"
Write-Host ""

# --- Convert project path to MSYS2 Unix path -------------------------------
$ProjectDir = $PSScriptRoot
$drive    = ($ProjectDir -replace '^([A-Za-z]):.*', '$1').ToLower()
$rest     = $ProjectDir -replace '^[A-Za-z]:', '' -replace '\\', '/'
$unixPath = "/$drive$rest"

# --- Build -----------------------------------------------------------------
Write-Host "[STEP 2] Building..."
Invoke-MSYS2 "cd '$unixPath' && mingw32-make clean && mingw32-make"
Write-Host ""

# --- Verify ----------------------------------------------------------------
$binary = Join-Path $ProjectDir "build\bin\db-browser.exe"
if (Test-Path $binary) {
    Write-Host "============================================================"
    Write-Host "  Setup Complete - Crissy's DB Browser"
    Write-Host "============================================================"
    Write-Host ""
    Write-Host "Binary: build\bin\db-browser.exe"
    Write-Host "Themes: build\bin\css\theme.css        (dark)"
    Write-Host "        build\bin\css\theme-simple.css  (light)"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\quick-launch.ps1"
    Write-Host "  .\build\bin\db-browser.exe <database.db>"
    Write-Host ""
    Write-Host "Note: GTK3 DLLs are in $MSYS2_ROOT\mingw64\bin"
    Write-Host "      That directory must be in PATH when running db-browser.exe"
    Write-Host "      quick-launch.ps1 handles this automatically."
    Write-Host "============================================================"
} else {
    Write-Error "[ERROR] Build failed - binary not found. Check output above."
    exit 1
}
