@echo off
setlocal EnableDelayedExpansion EnableExtensions
chcp 65001 >nul

REM ============================================================
REM  build.bat  -  Build script for gold-smc (MinGW + vcpkg)
REM
REM  Usage:
REM    build.bat                    Configure (if needed) + build + test
REM    build.bat clean              Wipe build/ and configure fresh, then build + test
REM    build.bat build              Configure (if needed) + build only (skip tests)
REM    build.bat test               Run tests only (build must exist)
REM    build.bat run                Build + run the strategy once
REM    build.bat release            Same as 'build.bat' but explicitly Release
REM    build.bat debug              Debug build (with symbols, no optimizations)
REM
REM  Environment overrides (set before invoking):
REM    MINGW_BIN    Path to MinGW g++ (default: C:\mingw64\bin)
REM    VCPKG_ROOT   Path to vcpkg install (default: C:\dev\vcpkg)
REM    BUILD_DIR    Build output folder    (default: build)
REM    GENERATOR    CMake generator       (default: Ninja)
REM    JOBS         Parallel build jobs   (default: number of CPUs)
REM ============================================================

REM ---------- path resolution with defaults ----------
if not defined MINGW_BIN  set "MINGW_BIN=C:\mingw64\bin"
if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\dev\vcpkg"
if not defined BUILD_DIR  set "BUILD_DIR=build"
if not defined GENERATOR  set "GENERATOR=Ninja"

set "GXX=%MINGW_BIN%\g++.exe"
set "TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

REM ---------- parse args ----------
set "ACTION=all"
set "BUILD_TYPE=Release"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="clean"     (set "ACTION=clean"      & shift & goto parse_args)
if /i "%~1"=="build"     (set "ACTION=build"      & shift & goto parse_args)
if /i "%~1"=="test"      (set "ACTION=test"       & shift & goto parse_args)
if /i "%~1"=="run"       (set "ACTION=run"        & shift & goto parse_args)
if /i "%~1"=="release"   (set "BUILD_TYPE=Release" & shift & goto parse_args)
if /i "%~1"=="debug"     (set "BUILD_TYPE=Debug"   & shift & goto parse_args)
if /i "%~1"=="-h"        goto usage
if /i "%~1"=="--help"    goto usage
if /i "%~1"=="/?"        goto usage
echo [ERROR] Unknown argument: %~1
goto usage
:args_done

REM ---------- locate project root (this script's directory) ----------
pushd "%~dp0"

REM ---------- default JOBS ----------
if not defined JOBS (
    for /f "tokens=2 delims==" %%A in ('wmic cpu get NumberOfLogicalProcessors /value 2^>nul ^| find "="') do set "JOBS=%%A"
    if not defined JOBS set "JOBS=4"
)

echo.
echo ============================================================
echo   gold-smc build
echo ------------------------------------------------------------
echo   Action      : %ACTION%
echo   Build type  : %BUILD_TYPE%
echo   Project     : %CD%
echo   Build dir   : %BUILD_DIR%
echo   Generator   : %GENERATOR%
echo   Compiler    : %GXX%
echo   vcpkg root  : %VCPKG_ROOT%
echo   Jobs        : %JOBS%
echo ============================================================
echo.

REM ---------- prerequisite checks ----------
if not exist "%GXX%" (
    echo [ERROR] g++ not found at "%GXX%"
    echo         Set MINGW_BIN to your MinGW bin folder, e.g.:
    echo           set MINGW_BIN=C:\msys64\ucrt64\bin
    goto :fail
)
if not exist "%TOOLCHAIN%" (
    echo [ERROR] vcpkg toolchain not found at "%TOOLCHAIN%"
    echo         Set VCPKG_ROOT to your vcpkg install, e.g.:
    echo           set VCPKG_ROOT=C:\dev\vcpkg
    goto :fail
)
where cmake >nul 2>nul || (
    echo [ERROR] cmake not found on PATH
    goto :fail
)

REM ---------- 'clean' : wipe build dir first ----------
if /i "%ACTION%"=="clean" (
    echo [1/3] Cleaning "%BUILD_DIR%" ...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    set "ACTION=all"
)

REM ---------- 'test' : no build, just run ctest ----------
if /i "%ACTION%"=="test" (
    if not exist "%BUILD_DIR%" (
        echo [ERROR] "%BUILD_DIR%" does not exist. Run 'build.bat build' first.
        goto :fail
    )
    echo [1/1] Running tests ...
    ctest --test-dir "%BUILD_DIR%" -C %BUILD_TYPE% --output-on-failure
    if errorlevel 1 goto :fail
    goto :success
)

REM ---------- configure (only if no CMakeCache) ----------
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [1/3] Configuring ^(fresh^) ...
    cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR%" ^
          -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DCMAKE_CXX_COMPILER="%GXX%" ^
          -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
          -DVCPKG_TARGET_TRIPLET=x64-mingw-static ^
          -DVCPKG_HOST_TRIPLET=x64-mingw-static
    if errorlevel 1 goto :fail
) else (
    echo [1/3] Build dir already configured ^(reusing^).
    echo       To force a fresh configure, run: build.bat clean
)

REM ---------- build ----------
echo [2/3] Building ^(%BUILD_TYPE%^) ...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %JOBS%
if errorlevel 1 goto :fail

REM ---------- tests (for 'all' / 'clean' / 'release' / 'debug') ----------
if /i not "%ACTION%"=="build" if /i not "%ACTION%"=="run" (
    echo [3/3] Running tests ...
    ctest --test-dir "%BUILD_DIR%" -C %BUILD_TYPE% --output-on-failure
    if errorlevel 1 goto :fail
) else (
    echo [3/3] Skipping tests ^(action=%ACTION%^).
)

REM ---------- run (for 'run') ----------
if /i "%ACTION%"=="run" (
    echo.
    echo === Running gold_smc ===
    "%BUILD_DIR%\gold_smc.exe"
    if errorlevel 1 goto :fail
)

goto :success

REM ============================================================
:usage
echo.
echo Usage: build.bat [action] [build-type]
echo.
echo Actions:
echo   (none)     Configure + build + test          [default]
echo   clean      Wipe build dir, then configure + build + test
echo   build      Configure + build only (no tests)
echo   test       Run tests only (build dir must exist)
echo   run        Build + run the strategy once
echo.
echo Build type:
echo   release    Release build with optimizations [default]
echo   debug      Debug build with symbols
echo.
echo Examples:
echo   build.bat
echo   build.bat clean
echo   build.bat debug
echo   build.bat run
echo   build.bat clean release
echo.
echo Environment overrides:
echo   MINGW_BIN   ^(default C:\mingw64\bin^)
echo   VCPKG_ROOT  ^(default C:\dev\vcpkg^)
echo   BUILD_DIR   ^(default build^)
echo   GENERATOR   ^(default Ninja^)
echo   JOBS        ^(default: number of logical CPUs^)
echo.
echo Example with overrides:
echo   set MINGW_BIN=C:\msys64\ucrt64\bin
echo   set VCPKG_ROOT=C:\tools\vcpkg
echo   build.bat clean debug
echo.
popd
endlocal
exit /b 1

:fail
echo.
echo [FAILED] Build aborted.
popd
endlocal
exit /b 1

:success
echo.
echo [DONE] Build succeeded.
echo        Binary: "%CD%\%BUILD_DIR%\gold_smc.exe"
popd
endlocal
exit /b 0