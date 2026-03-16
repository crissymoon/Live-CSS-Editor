param(
    [string]$DropUrl = "http://192.168.12.41:8080/",
    [string]$Distro = ""
)

$ErrorActionPreference = "Stop"

function Get-WslPath([string]$windowsPath) {
    $normalized = $windowsPath -replace "\\", "/"
    if ($normalized -match "^([A-Za-z]):/(.*)$") {
        $drive = $matches[1].ToLower()
        $rest = $matches[2]
        return "/mnt/$drive/$rest"
    }
    throw "Could not convert Windows path to WSL path: $windowsPath"
}

function Resolve-Distro([string]$requested) {
    $raw = (& wsl.exe -l -q) 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "WSL is not ready yet. Reboot, then install a distro (example: wsl --install -d Ubuntu)."
    }

    $distros = @()
    foreach ($line in $raw) {
        $name = "$line".Trim()
        if ($name) {
            $distros += $name
        }
    }

    if ($distros.Count -eq 0) {
        throw "No WSL distro found. Install one first (example: wsl --install -d Ubuntu or wsl --install -d kali-linux)."
    }

    if ($requested) {
        if ($distros -contains $requested) {
            return $requested
        }
        throw "Requested distro '$requested' was not found. Installed: $($distros -join ', ')"
    }

    if ($distros -contains "kali-linux") {
        return "kali-linux"
    }
    if ($distros -contains "Ubuntu") {
        return "Ubuntu"
    }

    return $distros[0]
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$wslProjectDir = Get-WslPath $scriptDir
$selectedDistro = Resolve-Distro $Distro

Write-Host "[wsl-build] Using distro: $selectedDistro"
Write-Host "[wsl-build] Project path: $wslProjectDir"

$wslCommand = "cd '$wslProjectDir' && bash build-linux.sh"
& wsl.exe -d $selectedDistro -- bash -lc $wslCommand
if ($LASTEXITCODE -ne 0) {
    throw "WSL build failed. Check output above."
}

$distDir = Join-Path $scriptDir "dist"
$package = Get-ChildItem -Path $distDir -Filter "imgui-browser-linux-*.tar.gz" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

Write-Host "[wsl-build] Build complete."
if ($package) {
    Write-Host "[wsl-build] Package: $($package.FullName)"
} else {
    Write-Host "[wsl-build] Package file not found in $distDir (build succeeded but package name may differ)."
}

Write-Host "[wsl-build] Opening drop page: $DropUrl"
Start-Process $DropUrl

if ($package) {
    Start-Process explorer.exe "/select,`"$($package.FullName)`""
} else {
    Start-Process explorer.exe "$distDir"
}
