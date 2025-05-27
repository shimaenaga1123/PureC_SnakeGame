@echo off
setlocal

set GAME_DIR=%~dp0
set BUILD_DIR=%GAME_DIR%build
set EXECUTABLE=%BUILD_DIR%\Release\snake.exe

echo 🐍 크로스 플랫폼 뱀 게임 실행기 🐍
echo ========================================

:: 게임이 빌드되어 있는지 확인
if not exist "%EXECUTABLE%" (
    echo 게임을 찾을 수 없습니다. 게임 빌드 중...
    call "%GAME_DIR%build.bat"

    if errorlevel 1 (
        echo 빌드 실패. 빌드 로그를 확인해주세요.
        pause
        exit /b 1
    )
)

:: 빌드 후 실행 파일 존재 여부 확인
if not exist "%EXECUTABLE%" (
    echo 게임 실행 파일을 찾을 수 없습니다: %EXECUTABLE%
    echo 게임이 올바르게 빌드되었는지 확인해주세요.
    pause
    exit /b 1
)

:: 게임 실행
echo 뱀 게임 시작 중...
cd /d "%GAME_DIR%"
"%EXECUTABLE%"

echo 플레이해주셔서 감사합니다!
pause
