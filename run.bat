@echo off
setlocal

echo ========================================================
echo   Starting MultiCoreX Backend Server
echo ========================================================

if not exist build\multicorex_server.exe (
    echo Executable not found. Running build.bat first...
    call build.bat
)

echo.
echo Server starting at http://127.0.0.1:8080
echo Opening web browser...
start http://127.0.0.1:8080
echo.

build\multicorex_server.exe --host 127.0.0.1 --port 8080 --web frontend
pause
