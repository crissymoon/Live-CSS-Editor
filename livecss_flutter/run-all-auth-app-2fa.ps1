param(
    [string]$AuthRoot = (Join-Path $PSScriptRoot "..\page-builder\xcm_auth"),
    [int]$AuthPort = 9100,
    [int]$TestPagePort = 9400,
    [string]$TotpSecret = "JBSWY3DPEHPK3PXP",
    [switch]$RestartAuth
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

function Stop-PortListeners {
    param([int]$Port)

    $listeners = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue
    foreach ($listener in $listeners) {
        $pidValue = $listener.OwningProcess
        $proc = Get-Process -Id $pidValue -ErrorAction SilentlyContinue
        if ($proc) {
            Write-Host "Stopping process on port ${Port}: $($proc.ProcessName) (PID $($proc.Id))"
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

$authUrl = "http://127.0.0.1:$AuthPort"
$testPage = "http://127.0.0.1:$TestPagePort"

Write-Host "Preparing xcm_auth on $authUrl ..."
$shouldStartAuth = $RestartAuth.IsPresent -or -not (Test-AuthHealth -BaseUrl $authUrl)
if ($shouldStartAuth) {
    Stop-PortListeners -Port $AuthPort

    $authCommand = @(
        "`$env:SERVER_ADDR=':$AuthPort';"
        "`$env:DB_DRIVER='sqlite';"
        "`$env:DB_DSN='./xcm_auth_dev.db';"
        "`$env:TWOFA_ENABLED='false';"
        "`$env:APP_URL='$testPage';"
        "go run ./cmd"
    ) -join ' '

    $authProc = Start-Process -FilePath 'pwsh' -WorkingDirectory $AuthRoot -ArgumentList @(
        '-NoExit',
        '-Command',
        $authCommand
    ) -PassThru

    Write-Host "Started xcm_auth terminal PID $($authProc.Id). Waiting for health..."
    $healthy = $false
    for ($i = 0; $i -lt 60; $i++) {
        Start-Sleep -Milliseconds 500
        if (Test-AuthHealth -BaseUrl $authUrl) {
            $healthy = $true
            break
        }
    }

    if (-not $healthy) {
        throw "xcm_auth failed to become healthy at $authUrl/health"
    }
}

Write-Host "xcm_auth is healthy."

Write-Host "Starting bridge test server on port $TestPagePort ..."
Stop-PortListeners -Port $TestPagePort
$bridgeScript = Join-Path $AuthRoot 'smoke\totp_bridge_server.py'
$credentialsPath = Join-Path $AuthRoot 'dev-credentials.json'
$testServer = $null
if (Get-Command python -ErrorAction SilentlyContinue) {
    $testServerCmd = "python `"$bridgeScript`" --host 127.0.0.1 --port $TestPagePort --auth-url $authUrl --credentials `"$credentialsPath`" --secret $TotpSecret"
    $testServer = Start-Process -FilePath 'pwsh' -WorkingDirectory $AuthRoot -ArgumentList @(
        '-NoExit',
        '-Command',
        $testServerCmd
    ) -PassThru
    Write-Host "Started bridge test server terminal PID $($testServer.Id)."
} else {
    throw "Python is required for the bridge test server. Please install Python or run the bridge manually."
}

Write-Host "Opening 2FA login test page..."
Start-Process $testPage

Write-Host "Launching Flutter app in Chrome..."
Start-Process -FilePath 'flutter' -WorkingDirectory $PSScriptRoot -ArgumentList @(
    'run',
    '-d',
    'chrome',
    '--dart-define',
    "XCM_AUTH_URL=$authUrl"
)

Write-Host "Ready."
Write-Host "1) Login to the app (xcm_auth is running with TWOFA_ENABLED=false)."
Write-Host "2) In the test page, run website login (step 1) then enter TOTP from app (step 2)."
Write-Host "3) Add this seed into the app for test-site codes: $TotpSecret"
Write-Host "4) Flutter app is running in Chrome against $authUrl"
