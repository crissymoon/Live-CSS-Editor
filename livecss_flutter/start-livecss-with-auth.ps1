param(
    [string]$AuthRoot = (Join-Path $PSScriptRoot "..\page-builder\xcm_auth"),
    [string]$AuthUrl = "http://127.0.0.1:9100",
    [switch]$RunSmokeTest
)

$ErrorActionPreference = 'Stop'

function Require-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found in PATH."
    }
}

function Test-AuthHealth {
    param([string]$BaseUrl)

    try {
        $response = Invoke-RestMethod -Uri "$BaseUrl/health" -Method Get -TimeoutSec 2
        return ($response.ok -eq $true)
    }
    catch {
        return $false
    }
}

function Stop-ConflictingPort9100IfNeeded {
    $listeners = Get-NetTCPConnection -State Listen -LocalPort 9100 -ErrorAction SilentlyContinue
    foreach ($listener in $listeners) {
        $proc = Get-Process -Id $listener.OwningProcess -ErrorAction SilentlyContinue
        if ($null -ne $proc -and ($proc.ProcessName -eq 'dart' -or $proc.ProcessName -eq 'dartvm')) {
            Write-Host "Stopping conflicting process on port 9100: $($proc.ProcessName) (PID $($proc.Id))"
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "Checking prerequisites..."
Require-Command -Name 'flutter'
Require-Command -Name 'go'
Require-Command -Name 'pwsh'

if (-not (Test-Path (Join-Path $PSScriptRoot 'pubspec.yaml'))) {
    throw "This script must be inside the Flutter app root. Could not find pubspec.yaml in $PSScriptRoot"
}

if (-not (Test-Path $AuthRoot)) {
    throw "xcm_auth root was not found at: $AuthRoot"
}

Write-Host "Ensuring xcm_auth is running at $AuthUrl ..."
if (-not (Test-AuthHealth -BaseUrl $AuthUrl)) {
    Stop-ConflictingPort9100IfNeeded

    $authCommand = @(
        '$env:SERVER_ADDR=":9100";'
        '$env:DB_DRIVER="sqlite";'
        '$env:DB_DSN="./xcm_auth_dev.db";'
        '$env:TWOFA_ENABLED="false";'
        'go run ./cmd'
    ) -join ' '

    $authProc = Start-Process -FilePath 'pwsh' -WorkingDirectory $AuthRoot -ArgumentList @(
        '-NoExit',
        '-Command',
        $authCommand
    ) -PassThru

    Write-Host "Started xcm_auth in new terminal (PID $($authProc.Id)). Waiting for health..."

    $healthy = $false
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 500
        if (Test-AuthHealth -BaseUrl $AuthUrl) {
            $healthy = $true
            break
        }
    }

    if (-not $healthy) {
        throw "xcm_auth failed to become healthy at $AuthUrl/health"
    }
}

Write-Host "xcm_auth is healthy."

if ($RunSmokeTest) {
    $smokeScript = Join-Path $AuthRoot "smoke\smoke_login.ps1"
    $credentials = Join-Path $AuthRoot "dev-credentials.json"

    if (-not (Test-Path $smokeScript)) {
        throw "Smoke script not found: $smokeScript"
    }

    Write-Host "Running xcm_auth smoke login test..."
    & $smokeScript -BaseUrl $AuthUrl -CredentialsPath $credentials
    if ($LASTEXITCODE -ne 0) {
        throw "Smoke login test failed with exit code $LASTEXITCODE"
    }
    Write-Host "Smoke login test passed."
}

Write-Host "Launching Flutter app in Chrome..."

Start-Process -FilePath 'flutter' -WorkingDirectory $PSScriptRoot -ArgumentList @(
    'run',
    '-d',
    'chrome',
    '--dart-define',
    "XCM_AUTH_URL=$AuthUrl"
)

Write-Host "Done. Flutter launch command started in a new process."
Write-Host "If Chrome does not auto-open, run: flutter run -d chrome --dart-define XCM_AUTH_URL=$AuthUrl"
