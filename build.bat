@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   MultiCoreX - Build Script (MSVC + CMake)
echo ========================================================

set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS% (
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist %VCVARS% (
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist %VCVARS% (
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

echo Initializing Visual Studio build environment...
call %VCVARS%
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to initialize Visual Studio C++ environment.
    pause
    exit /b %ERRORLEVEL%
)

if not exist build (
    mkdir build
)

echo Configuring CMake build...
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b %ERRORLEVEL%
)

echo Building MultiCoreX executables...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo   BUILD SUCCEEDED!
echo   Server executable: build\multicorex_server.exe
echo   Tests executable:  build\tests\multicorex_tests.exe
echo ========================================================
echo.
pause
