@echo off
setlocal
set SCRIPT_DIR=%~dp0
set PY_EXE=%SCRIPT_DIR%..\..\.venv\Scripts\python.exe
if exist "%PY_EXE%" (
  "%PY_EXE%" "%SCRIPT_DIR%uml_cli.py" %*
) else (
  py -3 "%SCRIPT_DIR%uml_cli.py" %*
)
endlocal
