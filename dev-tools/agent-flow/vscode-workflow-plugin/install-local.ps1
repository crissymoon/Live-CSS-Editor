$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pkg = Join-Path $scriptDir 'package.json'
if (-not (Test-Path $pkg)) {
  throw "package.json was not found at $pkg"
}

$json = Get-Content -Raw -Path $pkg | ConvertFrom-Json
$extId = "$($json.publisher).$($json.name)-$($json.version)"
$targetRoot = Join-Path $HOME '.vscode\extensions'
$target = Join-Path $targetRoot $extId

if (-not (Test-Path $targetRoot)) {
  New-Item -ItemType Directory -Path $targetRoot | Out-Null
}

if (Test-Path $target) {
  Remove-Item -Recurse -Force $target
}

New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item -Path (Join-Path $scriptDir '*') -Destination $target -Recurse -Force

Write-Host "Installed: $extId"
Write-Host "Path: $target"
Write-Host "Restart VS Code (or run Developer: Reload Window)."
