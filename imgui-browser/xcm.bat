@echo off
:: xcm.bat -- Windows launcher for xcm.py
:: Locates a Python 3 interpreter and delegates to xcm.py.
:: Usage: xcm.bat <subcommand> [args...]
::   xcm.bat list
::   xcm.bat run grab_bar
::   xcm.bat run explore --url https://example.com
::   xcm.bat run wasm --project C:\path\to\my-php-app
::   xcm.bat dry-run wasm
::   xcm.bat build
::   xcm.bat serve-wasm --project ..\  --port 8082
::   xcm.bat stop-wasm

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "PYFILE=%SCRIPT_DIR%xcm.py"

:: --- Locate Python 3 --------------------------------------------------
set "PY="

:: 1. py launcher (preferred on Windows)
where py >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=*" %%V in ('py -3 --version 2^>^&1') do (
        echo %%V | findstr /i "Python 3" >nul 2>&1
        if !errorlevel! == 0 (
            set "PY=py -3"
        )
    )
)

:: 2. python3 command
if not defined PY (
    where python3 >nul 2>&1
    if %errorlevel% == 0 (
        for /f "tokens=*" %%V in ('python3 --version 2^>^&1') do (
            echo %%V | findstr /i "Python 3" >nul 2>&1
            if !errorlevel! == 0 (
                set "PY=python3"
            )
        )
    )
)

:: 3. python command (may be Python 3 on some installs)
if not defined PY (
    where python >nul 2>&1
    if %errorlevel% == 0 (
        for /f "tokens=*" %%V in ('python --version 2^>^&1') do (
            echo %%V | findstr /i "Python 3" >nul 2>&1
            if !errorlevel! == 0 (
                set "PY=python"
            )
        )
    )
)

if not defined PY (
    echo [xcm] ERROR: Python 3 not found.  Install Python 3 from https://python.org
    echo [xcm] Then re-run: xcm.bat %*
    exit /b 1
)

:: --- Run xcm.py -------------------------------------------------------
%PY% "%PYFILE%" %*
exit /b %errorlevel%
