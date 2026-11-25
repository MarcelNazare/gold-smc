@echo off
setlocal
cd /d "%~dp0\.."

if exist .env (
    for /f "usebackq tokens=1,* delims==" %%A in (".env") do (
        echo %%A | findstr /r "^#" >nul || set "%%A=%%B"
    )
)

if "%TWELVEDATA_API_KEY%"=="" (
    echo ERROR: TWELVEDATA_API_KEY not set in .env
    exit /b 1
)

build\gold_smc.exe %1
endlocal