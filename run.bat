@echo off
chcp 437
setlocal

set GAME_DIR=%~dp0
set BUILD_DIR=%GAME_DIR%build
set EXECUTABLE=%BUILD_DIR%\snake.exe

echo Snake Game Runner
echo ========================================

if not exist "%EXECUTABLE%" (
    echo Game executable not found. Building the game...
    call "%GAME_DIR%build.bat"

    if errorlevel 1 (
        echo Build failed. Please check the build logs.
        pause
        exit /b 1
    )
)

if not exist "%EXECUTABLE%" (
    echo Game executable not found: %EXECUTABLE%
    echo Please make sure the game is built correctly.
    pause
    exit /b 1
)

echo Starting Snake Game...
cd /d "%GAME_DIR%"
"%EXECUTABLE%"

echo Thanks for playing!
pause