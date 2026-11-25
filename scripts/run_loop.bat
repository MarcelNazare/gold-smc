@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0\.."

:: ------------------------------------------------------------
::  Usage:
::    run_loop.bat                 every 300 s (5 min), default config
::    run_loop.bat 60              every 60 s
::    run_loop.bat 300 config\config.json
::    run_loop.bat 1               run once then exit (useful for testing)
::  Stop with Ctrl+C.
:: ------------------------------------------------------------

set "INTERVAL=%~1"
set "CONFIG=%~2"
if "%INTERVAL%"=="" set "INTERVAL=300"
if "%CONFIG%"==""   set "CONFIG=config\config.json"

:: --- sanity checks ---
if not exist ".env" (
    echo [ERROR] .env not found in %CD%
    echo         Create it with:  echo TWELVEDATA_API_KEY=your_key > .env
    exit /b 1
)
if not exist "build\gold_smc.exe" (
    echo [ERROR] build\gold_smc.exe not found
    echo         Run:  build.bat
    exit /b 1
)

:: --- load .env once, outside the loop ---
::   eol=#  skips comment lines
::   set "%%A=%%B" with quotes handles & | < > ^ in the value safely
for /f "usebackq eol=# tokens=1,* delims==" %%A in (".env") do (
    if not "%%A"=="" set "%%A=%%B"
)

if "%TWELVEDATA_API_KEY%"=="" (
    echo [ERROR] TWELVEDATA_API_KEY is empty after loading .env
    echo         File:  %CD%\.env
    echo         Check:  dir /a .env    (must be .env, not .env.txt)
    exit /b 1
)

echo ============================================================
echo  gold-smc - loop mode
echo ------------------------------------------------------------
echo  interval  : %INTERVAL% s
echo  config    : %CONFIG%
echo  env       : %CD%\.env
echo  Press Ctrl+C to stop.
echo ============================================================
echo.

set "ITER=0"
:loop
    set /a ITER+=1
    echo ------------------------------------------------------------
    echo [ITER !ITER!] %date% %time%
    echo ------------------------------------------------------------
    build\gold_smc.exe "%CONFIG%"
    set "RC=!errorlevel!"
    if !RC! neq 0 echo [WARN] gold_smc exited with code !RC!
    echo.
    timeout /t %INTERVAL% /nobreak >nul
goto loop