# launch-browser.ps1 -- Launch Crissy's Style Tool (imgui-browser) on Windows.
#
# Usage:
#   .\launch-browser.ps1
#   .\launch-browser.ps1 --url https://example.com
#   .\launch-browser.ps1 --clean          # wipe build dir, reconfigure, rebuild
#
# Checks whether the source is newer than the built exe and prompts you to
# build before opening if so.  Requires Visual Studio 2022 or CMake in PATH.

param(
    [string]$Url   = "",
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root    = $PSScriptRoot
$Exe     = Join-Path $Root "imgui-browser\build\Release\imgui_browser.exe"
$SrcDir  = Join-Path $Root "imgui-browser\src"
$CmkList = Join-Path $Root "imgui-browser\CMakeLists.txt"
$BuildDir= Join-Path $Root "imgui-browser\build"

# ── Find a build tool ────────────────────────────────────────────────────────

function Find-MSBuild {
    # 1. vswhere (VS installer ships this)
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $found = & $vsWhere -latest -requires Microsoft.Component.MSBuild `
                            -find "MSBuild\**\Bin\MSBuild.exe" 2>$null |
                 Select-Object -First 1
        if ($found -and (Test-Path $found)) { return $found }
    }
    # 2. Known VS 2022 default paths
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Find-CMake {
    $c = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    return $null
}

# ── Check whether a rebuild is needed ───────────────────────────────────────

function Get-NeedsRebuild {
    if (-not (Test-Path $Exe)) { return $true }

    $exeTime = (Get-Item $Exe).LastWriteTimeUtc

    $watchPaths = @($SrcDir, $CmkList)
    foreach ($wp in $watchPaths) {
        if (-not (Test-Path $wp)) { continue }
        $newer = Get-ChildItem $wp -Recurse -File -ErrorAction SilentlyContinue |
                 Where-Object { $_.LastWriteTimeUtc -gt $exeTime } |
                 Select-Object -First 1
        if ($newer) { return $true }
    }
    return $false
}

# ── Configure (cmake ..) ─────────────────────────────────────────────────────

function Invoke-Configure {
    $cmake = Find-CMake
    if (-not $cmake) {
        Write-Host "cmake not found -- skipping configure step."
        return $false
    }

    Write-Host "Configuring with CMake..."
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }

    $args = @("..", "-DCMAKE_BUILD_TYPE=Release")
    & $cmake @args
    $ok = $LASTEXITCODE -eq 0
    if (-not $ok) { Write-Host "cmake configure failed." }
    return $ok
}

# ── Build ────────────────────────────────────────────────────────────────────

function Invoke-Build {
    $slnPath = Join-Path $BuildDir "imgui_browser.sln"

    # If there is no solution / cache yet, configure first.
    if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
        if (-not (Invoke-Configure)) { return $false }
    }

    $msbuild = Find-MSBuild
    if ($msbuild) {
        Write-Host "Using MSBuild: $msbuild"
        & $msbuild $slnPath `
            /p:Configuration=Release /p:Platform=x64 `
            /t:imgui_browser /m /nologo /v:minimal
        return $LASTEXITCODE -eq 0
    }

    $cmake = Find-CMake
    if ($cmake) {
        Write-Host "Using CMake: $cmake"
        & $cmake --build $BuildDir --config Release --target imgui_browser
        return $LASTEXITCODE -eq 0
    }

    Write-Host ""
    Write-Host "No build tool found. Options:"
    Write-Host "  - Install Visual Studio 2022 (any edition) with the C++ workload"
    Write-Host "  - Or open imgui-browser\build\imgui_browser.sln in Visual Studio manually"
    return $false
}

# ── Clean ────────────────────────────────────────────────────────────────────

function Invoke-Clean {
    if (Test-Path $BuildDir) {
        Write-Host "Removing $BuildDir ..."
        Remove-Item $BuildDir -Recurse -Force
        Write-Host "Build directory cleared."
    }
}

# ── Launch ───────────────────────────────────────────────────────────────────

function Start-Browser {
    if (-not (Test-Path $Exe)) {
        Write-Host "Exe not found. Cannot open."
        return
    }
    $params = @{ FilePath = $Exe; WorkingDirectory = (Split-Path $Exe) }
    if ($Url -ne "") { $params["ArgumentList"] = @("--url", $Url) }
    Start-Process @params
    Write-Host "Browser launched."
}

# ── Main ─────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "--- Crissy's Style Tool ---"

if ($Clean) {
    Invoke-Clean
    Write-Host "Rebuilding from scratch..."
    if (Invoke-Build) { Start-Browser } else { Write-Host "Build failed. Check output above." }
    exit
}

$needsBuild = Get-NeedsRebuild
$exeExists  = Test-Path $Exe

if (-not $exeExists) {
    Write-Host "No binary found. A build is required before the browser can open."
    Write-Host ""
    Write-Host "  [B] Build now"
    Write-Host "  [C] Cancel"
    Write-Host ""
    $choice = (Read-Host "Choice").Trim().ToUpper()
    if ($choice -eq "B") {
        if (Invoke-Build) { Start-Browser } else { Write-Host "Build failed. Check output above." }
    }
} elseif ($needsBuild) {
    $exeAge = (Get-Item $Exe).LastWriteTime.ToString("yyyy-MM-dd HH:mm")
    Write-Host "Source files are newer than the built binary (exe from $exeAge)."
    Write-Host ""
    Write-Host "  [B] Build and open"
    Write-Host "  [O] Open existing binary without rebuilding"
    Write-Host "  [C] Cancel"
    Write-Host ""
    $choice = (Read-Host "Choice").Trim().ToUpper()
    switch ($choice) {
        "B" { if (Invoke-Build) { Start-Browser } else { Write-Host "Build failed. Check output above." } }
        "O" { Start-Browser }
    }
} else {
    $exeAge = (Get-Item $Exe).LastWriteTime.ToString("yyyy-MM-dd HH:mm")
    Write-Host "Binary is up to date (built $exeAge)."
    Start-Browser
}
