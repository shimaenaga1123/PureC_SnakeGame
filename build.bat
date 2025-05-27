@echo off
setlocal enabledelayedexpansion

echo 크로스 플랫폼 뱀 게임 빌드 스크립트
echo ========================================

:: 기본값
set BUILD_TYPE=Release
set BUILD_DIR=build
set CLEAN_BUILD=0

:: 명령줄 인수 처리
:parse_args
if "%~1"=="" goto :start_build
if /i "%~1"=="-d" set BUILD_TYPE=Debug
if /i "%~1"=="--debug" set BUILD_TYPE=Debug
if /i "%~1"=="-r" set BUILD_TYPE=Release
if /i "%~1"=="--release" set BUILD_TYPE=Release
if /i "%~1"=="-c" set CLEAN_BUILD=1
if /i "%~1"=="--clean" set CLEAN_BUILD=1
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--help" goto :show_help
shift
goto :parse_args

:show_help
echo 사용법: %0 [옵션]
echo 옵션:
echo   -d, --debug     디버그 모드로 빌드
echo   -r, --release   릴리즈 모드로 빌드 (기본값)
echo   -c, --clean     빌드 전 빌드 디렉토리 정리
echo   -h, --help      이 도움말 메시지 표시
exit /b 0

:start_build
echo [정보] 빌드 타입: %BUILD_TYPE%
echo [정보] 빌드 디렉토리: %BUILD_DIR%

:: CMake 설치 여부 확인
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [오류] CMake가 설치되어 있지 않습니다. 먼저 CMake를 설치해주세요.
    exit /b 1
)

:: 요청된 경우 빌드 디렉토리 정리
if %CLEAN_BUILD%==1 (
    echo [정보] 빌드 디렉토리 정리 중...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

:: 빌드 디렉토리 생성
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: 프로젝트 구성 및 빌드
echo [정보] 프로젝트 구성 중...
cmake -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..
if errorlevel 1 (
    echo [오류] CMake 구성 실패!
    cd ..
    exit /b 1
)

echo [정보] 프로젝트 빌드 중...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo [오류] 빌드 실패!
    cd ..
    exit /b 1
)

cd ..
echo [성공] 빌드가 성공적으로 완료되었습니다!
echo [정보] %BUILD_DIR%\%BUILD_TYPE%\snake.exe를 실행하여 플레이하세요

pause
