<#
.SYNOPSIS
    Builds the Code Review TUI into a distributable executable.

.DESCRIPTION
    Platform-aware build script for the Love2D Code Review TUI.

    Windows  ->  build\win\CodeReview.exe  (love.exe fused with .love archive)
                 + all Love2D runtime DLLs alongside the exe
    macOS    ->  build\mac\CodeReview.app   (LOVE.app re-bundled with .love inside)
    Linux    ->  build\linux\CodeReview.sh  (launcher shell script + .love)

    A timestamped build.log is written to the build\ folder on every run.

    Fingerprint guard: all .lua source files in review_tui\ are hashed.
    If the hash matches the last successful build for this platform, the
    compile step is skipped.  Use -Force to override.

.PARAMETER Force
    Rebuild even if the source fingerprint has not changed.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Force

.NOTES
    Requires PowerShell 5.1 or later (ships on all supported Windows/macOS/Linux).
    Love2D must be installed to produce a native executable.
    On Windows the icon is embedded when rcedit.exe is available on PATH or in
    common locations -- otherwise the .exe is still valid, just without a custom icon.
#>

#Requires -Version 5.1
[CmdletBinding()]
param(
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$SOURCE_DIR = Join-Path $SCRIPT_DIR "review_tui"
$BUILD_DIR  = Join-Path $SCRIPT_DIR "build"
$ICON_PNG   = Join-Path $SCRIPT_DIR "ava-moon.png"
$APP_NAME   = "CodeReview"

# ---------------------------------------------------------------------------
# Platform detection  (works with both Windows PowerShell 5.x and pwsh 7+)
# ---------------------------------------------------------------------------
$IS_WIN = $IsWindows -or (
    $PSVersionTable.PSVersion.Major -lt 6 -and
    $env:OS -eq "Windows_NT"
)
$IS_MAC = $IsMacOS
$IS_LIN = $IsLinux

if (-not $IS_WIN -and -not $IS_MAC -and -not $IS_LIN) {
    # Older Windows PowerShell where $IsWindows is not defined
    $IS_WIN = $true
}

$PLATFORM = if ($IS_WIN) { "win" } elseif ($IS_MAC) { "mac" } else { "linux" }

# Platform-specific output subdirectory
$OUT_DIR     = Join-Path $BUILD_DIR $PLATFORM
$LOVE_FILE   = Join-Path $BUILD_DIR "$APP_NAME.love"
$FP_FILE     = Join-Path $BUILD_DIR "build.fingerprint.$PLATFORM"
$LOG_FILE    = Join-Path $BUILD_DIR "build.log"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Log {
    param([string]$Msg, [string]$Level = "INFO")
    $line = "[{0}] [{1}] {2}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Level, $Msg
    Write-Host $line
    Add-Content -Path $LOG_FILE -Value $line -Encoding UTF8
}

function Get-SourceFingerprint {
    # Hash every .lua file and conf.lua under review_tui\ plus the icon.
    $files = Get-ChildItem -Path $SOURCE_DIR -Recurse -File |
             Where-Object { $_.Extension -in @(".lua", ".json", ".png") } |
             Sort-Object FullName
    if (Test-Path $ICON_PNG) {
        $files += Get-Item $ICON_PNG
    }
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    $all    = [System.Text.StringBuilder]::new()
    foreach ($f in $files) {
        $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
        $hash  = [BitConverter]::ToString($hasher.ComputeHash($bytes)) -replace "-"
        [void]$all.AppendLine("$hash  $($f.FullName)")
    }
    $hasher.Dispose()
    # Include platform so a cross-platform switch always triggers a rebuild
    [void]$all.AppendLine("PLATFORM=$PLATFORM")
    $final = [System.Text.Encoding]::UTF8.GetBytes($all.ToString())
    return [BitConverter]::ToString(
        [System.Security.Cryptography.SHA256]::Create().ComputeHash($final)
    ) -replace "-"
}

function Get-OSVersion {
    if ($IS_WIN) {
        $v = [System.Environment]::OSVersion.Version
        return "Windows $($v.Major).$($v.Minor) build $($v.Build)"
    } elseif ($IS_MAC) {
        $sw = & sw_vers -productVersion 2>$null
        return "macOS $sw"
    } else {
        if (Test-Path "/etc/os-release") {
            $line = Get-Content /etc/os-release |
                    Where-Object { $_ -match "^PRETTY_NAME=" } |
                    Select-Object -First 1
            return $line -replace '^PRETTY_NAME=["'']?|["'']?$'
        }
        return "Linux"
    }
}

function Find-LoveExe {
    # Windows: search PATH then common install locations
    $cmd = Get-Command love -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        "C:\Program Files\LOVE\love.exe",
        "C:\Program Files (x86)\LOVE\love.exe",
        (Join-Path $env:LOCALAPPDATA "Programs\LOVE\love.exe")
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Find-LoveApp {
    # macOS: standard install location(s)
    $candidates = @(
        "/Applications/love.app",
        "/Applications/LOVE.app",
        "$HOME/Applications/love.app",
        "$HOME/Applications/LOVE.app"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Find-Rcedit {
    # Try PATH first, then npm global bin, then typical Electron builder location
    $cmd = Get-Command rcedit -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        (Join-Path $env:APPDATA "npm\rcedit.exe"),
        (Join-Path $env:LOCALAPPDATA "npm\rcedit.exe"),
        (Join-Path $SCRIPT_DIR "tools\rcedit.exe")
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) { return $c }
    }
    return $null
}

# ---------------------------------------------------------------------------
# PNG -> ICO conversion (pure .NET, no external tools)
# Multi-size ICO: 16, 32, 48, 256  (256 stored as raw PNG per ICO spec)
# ---------------------------------------------------------------------------
function ConvertTo-Ico {
    param([string]$PngPath, [string]$OutPath)

    Add-Type -AssemblyName System.Drawing

    $sizes = @(16, 32, 48, 256)

    # Collect each resized bitmap's raw bytes
    $images = foreach ($sz in $sizes) {
        $src = [System.Drawing.Bitmap]::new($PngPath)
        $dst = [System.Drawing.Bitmap]::new($sz, $sz,
               [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g   = [System.Drawing.Graphics]::FromImage($dst)
        $g.InterpolationMode =
            [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.DrawImage($src, 0, 0, $sz, $sz)
        $g.Dispose()
        $src.Dispose()

        $ms = [System.IO.MemoryStream]::new()
        # 256x256 is stored as PNG inside ICO (modern Windows spec)
        if ($sz -eq 256) {
            $dst.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        } else {
            $dst.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        }
        $dst.Dispose()
        [PSCustomObject]@{ Size = $sz; Data = $ms.ToArray() }
        $ms.Dispose()
    }

    # Build ICO file in memory
    # ICO header: 6 bytes.  Directory: n * 16 bytes.  Image data follows.
    $stream = [System.IO.MemoryStream]::new()
    $bw     = [System.IO.BinaryWriter]::new($stream)

    # Header
    $bw.Write([uint16]0)            # reserved
    $bw.Write([uint16]1)            # type: 1 = ICO
    $bw.Write([uint16]$images.Count)

    # Directory entries placeholder; we'll fill offsets after writing image data
    $dirOffset = $stream.Position
    foreach ($img in $images) {
        $w = if ($img.Size -eq 256) { 0 } else { [byte]$img.Size }
        $h = $w
        $bw.Write([byte]$w)         # width   (0 = 256)
        $bw.Write([byte]$h)         # height  (0 = 256)
        $bw.Write([byte]0)          # colour count
        $bw.Write([byte]0)          # reserved
        $bw.Write([uint16]1)        # colour planes
        $bw.Write([uint16]32)       # bits per pixel
        $bw.Write([uint32]$img.Data.Length)
        $bw.Write([uint32]0)        # offset placeholder
    }

    # Write image data and record real offsets
    $offsets = @()
    foreach ($img in $images) {
        $offsets += [uint32]$stream.Position
        $bw.Write($img.Data)
    }

    # Patch offsets back into directory
    for ($i = 0; $i -lt $images.Count; $i++) {
        # Each dir entry is 16 bytes; offset field is at byte 12 of the entry
        $stream.Position = $dirOffset + ($i * 16) + 12
        $bw.Write($offsets[$i])
    }

    $bw.Flush()
    [System.IO.File]::WriteAllBytes($OutPath, $stream.ToArray())
    $bw.Dispose()
    $stream.Dispose()
}

# ---------------------------------------------------------------------------
# Create .love archive (zip of review_tui\ contents, not the folder itself)
# ---------------------------------------------------------------------------
function New-LoveFile {
    param([string]$Dest)

    if (Test-Path $Dest) { Remove-Item $Dest -Force }

    # PowerShell 5+ has Compress-Archive; use it if available, else use .NET
    $tmpZip = $Dest + ".zip"
    if (Test-Path $tmpZip) { Remove-Item $tmpZip -Force }

    Compress-Archive -Path (Join-Path $SOURCE_DIR "*") -DestinationPath $tmpZip -Force
    Move-Item $tmpZip $Dest
    Write-Log "Created .love archive: $Dest  ($([Math]::Round((Get-Item $Dest).Length / 1KB, 1)) KB)"
}

# ---------------------------------------------------------------------------
# Windows: fuse love.exe + .love, copy DLLs, embed icon
# ---------------------------------------------------------------------------
function Build-Windows {
    $loveExe = Find-LoveExe
    if (-not $loveExe) {
        Write-Log "Love2D not found on this machine - cannot produce .exe.  Install from https://love2d.org" "WARN"
        Write-Log "The .love file itself is usable: run with  love $LOVE_FILE" "WARN"
        return
    }

    $loveDir = Split-Path -Parent $loveExe
    Write-Log "Love2D found: $loveExe"

    New-Item -ItemType Directory -Path $OUT_DIR -Force | Out-Null

    # Fuse: copy love.exe bytes then append .love bytes
    $exeOut = Join-Path $OUT_DIR "$APP_NAME.exe"
    $loveBytes  = [System.IO.File]::ReadAllBytes($loveExe)
    $loveABytes = [System.IO.File]::ReadAllBytes($LOVE_FILE)
    $fused = New-Object byte[] ($loveBytes.Length + $loveABytes.Length)
    [Array]::Copy($loveBytes,  $fused, $loveBytes.Length)
    [Array]::Copy($loveABytes, 0, $fused, $loveBytes.Length, $loveABytes.Length)
    [System.IO.File]::WriteAllBytes($exeOut, $fused)
    Write-Log "Fused exe: $exeOut  ($([Math]::Round($fused.Length / 1MB, 2)) MB)"

    # Copy DLLs from the Love2D install directory
    $dlls = Get-ChildItem -Path $loveDir -Filter "*.dll" -File
    foreach ($dll in $dlls) {
        Copy-Item $dll.FullName (Join-Path $OUT_DIR $dll.Name) -Force
    }
    if ($dlls.Count -gt 0) {
        Write-Log "Copied $($dlls.Count) runtime DLL(s) from $loveDir"
    }

    # Icon: convert PNG -> ICO then embed via rcedit if available
    if (Test-Path $ICON_PNG) {
        $icoPath = Join-Path $BUILD_DIR "$APP_NAME.ico"
        try {
            ConvertTo-Ico -PngPath $ICON_PNG -OutPath $icoPath
            Write-Log "Icon converted: $icoPath"

            $rcedit = Find-Rcedit
            if ($rcedit) {
                & $rcedit $exeOut --set-icon $icoPath 2>&1 | ForEach-Object {
                    Write-Log "  rcedit: $_"
                }
                Write-Log "Icon embedded into exe via rcedit"
            } else {
                Write-Log "rcedit not found -- icon was not embedded into the exe." "WARN"
                Write-Log "  To embed: install rcedit (npm install -g rcedit) or place rcedit.exe in tools\" "WARN"
                Write-Log "  Then re-run:  rcedit $exeOut --set-icon $icoPath"
            }
        } catch {
            Write-Log "Icon conversion failed: $_" "WARN"
        }
    }

    Write-Log "Windows build output: $OUT_DIR"
}

# ---------------------------------------------------------------------------
# macOS: clone LOVE.app, embed .love, replace icon
# ---------------------------------------------------------------------------
function Build-Mac {
    $loveApp = Find-LoveApp
    if (-not $loveApp) {
        Write-Log "LOVE.app not found on this machine - cannot produce .app.  Install from https://love2d.org" "WARN"
        Write-Log "The .love file itself is usable: run with  love $LOVE_FILE" "WARN"
        return
    }

    Write-Log "LOVE.app found: $loveApp"
    New-Item -ItemType Directory -Path $OUT_DIR -Force | Out-Null

    $appDst = Join-Path $OUT_DIR "$APP_NAME.app"
    if (Test-Path $appDst) { Remove-Item $appDst -Recurse -Force }

    # Deep copy the LOVE.app bundle
    & cp -R $loveApp $appDst
    Write-Log "Cloned LOVE.app -> $appDst"

    # Drop the .love archive into Contents/Resources
    $resSrc = Join-Path $appDst "Contents/Resources"
    Copy-Item $LOVE_FILE (Join-Path $resSrc "$APP_NAME.love") -Force
    Write-Log "Embedded .love into app bundle"

    # Patch Info.plist: CFBundleName, CFBundleDisplayName, CFBundleIdentifier
    $plist = Join-Path $appDst "Contents/Info.plist"
    if (Test-Path $plist) {
        $content = Get-Content $plist -Raw
        $content = $content -replace "(?<=<key>CFBundleName</key>\s*<string>)[^<]*", $APP_NAME
        $content = $content -replace "(?<=<key>CFBundleDisplayName</key>\s*<string>)[^<]*", $APP_NAME
        $content = $content -replace "(?<=<key>CFBundleIdentifier</key>\s*<string>)[^<]*",
                                     "com.devtools.codereview"
        Set-Content $plist $content -Encoding UTF8
        Write-Log "Patched Info.plist"
    }

    # Icon: convert PNG -> ICNS using sips + iconutil (ships with macOS)
    if (Test-Path $ICON_PNG) {
        $icnsDir = Join-Path $BUILD_DIR "icon.iconset"
        New-Item -ItemType Directory -Path $icnsDir -Force | Out-Null

        $iconSizes = @(
            @{ Size = 16;   Name = "icon_16x16.png"      },
            @{ Size = 32;   Name = "icon_16x16@2x.png"   },
            @{ Size = 32;   Name = "icon_32x32.png"      },
            @{ Size = 64;   Name = "icon_32x32@2x.png"   },
            @{ Size = 128;  Name = "icon_128x128.png"    },
            @{ Size = 256;  Name = "icon_128x128@2x.png" },
            @{ Size = 256;  Name = "icon_256x256.png"    },
            @{ Size = 512;  Name = "icon_256x256@2x.png" },
            @{ Size = 512;  Name = "icon_512x512.png"    },
            @{ Size = 1024; Name = "icon_512x512@2x.png" }
        )

        try {
            foreach ($entry in $iconSizes) {
                $dest = Join-Path $icnsDir $entry.Name
                & sips -z $entry.Size $entry.Size $ICON_PNG --out $dest 2>/dev/null
            }
            $icnsOut = Join-Path $BUILD_DIR "$APP_NAME.icns"
            & iconutil -c icns $icnsDir --output $icnsOut 2>/dev/null
            if (Test-Path $icnsOut) {
                Copy-Item $icnsOut (Join-Path $resSrc "$APP_NAME.icns") -Force
                # Also replace the main app icon reference in plist
                if (Test-Path $plist) {
                    $content = Get-Content $plist -Raw
                    $content = $content -replace "(?<=<key>CFBundleIconFile</key>\s*<string>)[^<]*",
                                                 $APP_NAME
                    Set-Content $plist $content -Encoding UTF8
                }
                Write-Log "Icon embedded: $icnsOut"
            }
            Remove-Item $icnsDir -Recurse -Force
        } catch {
            Write-Log "Icon creation failed: $_" "WARN"
        }
    }

    Write-Log "macOS build output: $appDst"
}

# ---------------------------------------------------------------------------
# Linux: create .sh launcher alongside the .love file
# ---------------------------------------------------------------------------
function Build-Linux {
    New-Item -ItemType Directory -Path $OUT_DIR -Force | Out-Null

    $loveDst = Join-Path $OUT_DIR "$APP_NAME.love"
    Copy-Item $LOVE_FILE $loveDst -Force

    $launcher = Join-Path $OUT_DIR "$APP_NAME.sh"
    $script = @"
#!/usr/bin/env bash
# Launcher for $APP_NAME -- requires Love2D installed
DIR="`$(dirname "`$(readlink -f "`$0")")"
if command -v love &>/dev/null; then
    exec love "`$DIR/$APP_NAME.love" "`$@"
elif command -v love2d &>/dev/null; then
    exec love2d "`$DIR/$APP_NAME.love" "`$@"
else
    echo "Love2D not found. Install from https://love2d.org" >&2
    exit 1
fi
"@
    Set-Content -Path $launcher -Value $script -Encoding UTF8
    if ($IsLinux) {
        & chmod +x $launcher
    }
    Write-Log "Linux build output: $OUT_DIR"
}

# ===========================================================================
# MAIN
# ===========================================================================

# Ensure build directory exists before we try to write logs
New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null

# Rotate log (keep last 500 lines if it already exists)
if (Test-Path $LOG_FILE) {
    $existing = Get-Content $LOG_FILE -Tail 500
    Set-Content $LOG_FILE $existing -Encoding UTF8
}

Write-Log "===== Build start ======================================================"
Write-Log "Script:       $($MyInvocation.MyCommand.Path)"
Write-Log "Platform:     $PLATFORM"
Write-Log "OS:           $(Get-OSVersion)"
Write-Log "PowerShell:   $($PSVersionTable.PSVersion)"

# Log Python version
$pyCmd = $env:CODE_REVIEW_PYTHON
if (-not $pyCmd) { $pyCmd = if ($IS_WIN) { "py -3" } else { "python3" } }
try {
    $pyVer = & $pyCmd --version 2>&1 | Select-Object -First 1
    Write-Log "Python:       $pyVer"
} catch {
    Write-Log "Python:       not found (non-fatal)" "WARN"
}

# Log Love2D version
$loveExeForVer = if ($IS_WIN) { Find-LoveExe } else { $null }
if ($loveExeForVer) {
    Write-Log "Love2D exe:   $loveExeForVer"
}

# ---------------------------------------------------------------------------
# Fingerprint guard
# ---------------------------------------------------------------------------
$currentFP = Get-SourceFingerprint
Write-Log "Source fingerprint: $currentFP"

if (-not $Force -and (Test-Path $FP_FILE)) {
    $storedFP = (Get-Content $FP_FILE -Raw).Trim()
    if ($storedFP -eq $currentFP) {
        Write-Log "Fingerprint matches last build -- nothing to do."
        Write-Log "  Use  .\build.ps1 -Force  to rebuild unconditionally."
        Write-Log "===== Build skipped ===================================================="
        exit 0
    }
    Write-Log "Fingerprint changed -- proceeding with rebuild."
} elseif ($Force) {
    Write-Log "-Force flag set -- rebuilding unconditionally."
}

# ---------------------------------------------------------------------------
# Step 1: Create the platform-agnostic .love archive
# ---------------------------------------------------------------------------
New-LoveFile -Dest $LOVE_FILE

# ---------------------------------------------------------------------------
# Step 2: Platform-specific packaging
# ---------------------------------------------------------------------------
switch ($PLATFORM) {
    "win"   { Build-Windows }
    "mac"   { Build-Mac     }
    "linux" { Build-Linux   }
}

# ---------------------------------------------------------------------------
# Step 3: Save fingerprint so subsequent runs skip unchanged builds
# ---------------------------------------------------------------------------
Set-Content -Path $FP_FILE -Value $currentFP -Encoding ASCII -NoNewline

Write-Log "Fingerprint saved: $FP_FILE"
Write-Log "===== Build complete ==================================================="
