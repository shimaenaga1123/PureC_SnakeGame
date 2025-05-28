@echo off
chcp 437
setlocal enabledelayedexpansion

echo Cross Platform Snake Game Build Script
echo ========================================

:: Default values
set BUILD_TYPE=Release
set BUILD_DIR=build
set CLEAN_BUILD=0
set COMPILER=auto
set GENERATOR=
set PLATFORM=x64

:: Command line argument processing
:parse_args
if "%~1"=="" goto :detect_compiler
if /i "%~1"=="-d" set BUILD_TYPE=Debug
if /i "%~1"=="--debug" set BUILD_TYPE=Debug
if /i "%~1"=="-r" set BUILD_TYPE=Release
if /i "%~1"=="--release" set BUILD_TYPE=Release
if /i "%~1"=="-c" set CLEAN_BUILD=1
if /i "%~1"=="--clean" set CLEAN_BUILD=1
if /i "%~1"=="-vs" set COMPILER=vs
if /i "%~1"=="--visual-studio" set COMPILER=vs
if /i "%~1"=="-mg" set COMPILER=mingw
if /i "%~1"=="--mingw" set COMPILER=mingw
if /i "%~1"=="-x86" set PLATFORM=Win32
if /i "%~1"=="--x86" set PLATFORM=Win32
if /i "%~1"=="-x64" set PLATFORM=x64
if /i "%~1"=="--x64" set PLATFORM=x64
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--help" goto :show_help
shift
goto :parse_args

:show_help
echo Usage: %0 [options]
echo Options:
echo   -d, --debug        Build in debug mode
echo   -r, --release      Build in release mode (default)
echo   -c, --clean        Clean build directory before building
echo   -vs, --visual-studio  Use Visual Studio compiler
echo   -mg, --mingw       Use MinGW compiler
echo   -x86, --x86        Build for 32-bit (Visual Studio only)
echo   -x64, --x64        Build for 64-bit (default)
echo   -h, --help         Show this help message
echo.
echo If no compiler is specified, the script will auto-detect available compilers.
exit /b 0

:detect_compiler
if not "%COMPILER%"=="auto" goto :set_generator

echo [INFO] Auto-detecting available compilers...

:: Check for Visual Studio
set VS_FOUND=0
where cl.exe >nul 2>&1
if not errorlevel 1 set VS_FOUND=1

:: Check for Visual Studio installations
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017" set VS_FOUND=1

:: Check for MinGW
set MINGW_FOUND=0
where gcc.exe >nul 2>&1
if not errorlevel 1 set MINGW_FOUND=1
where mingw32-gcc.exe >nul 2>&1
if not errorlevel 1 set MINGW_FOUND=1

:: Decide which compiler to use
if %VS_FOUND%==1 if %MINGW_FOUND%==1 (
    echo [INFO] Both Visual Studio and MinGW detected.
    echo [INFO] Defaulting to Visual Studio. Use -mg to force MinGW.
    set COMPILER=vs
) else if %VS_FOUND%==1 (
    echo [INFO] Visual Studio detected.
    set COMPILER=vs
) else if %MINGW_FOUND%==1 (
    echo [INFO] MinGW detected.
    set COMPILER=mingw
) else (
    echo [ERROR] No supported compiler found!
    echo [ERROR] Please install Visual Studio or MinGW-w64.
    exit /b 1
)

:set_generator
if "%COMPILER%"=="vs" goto :set_vs_generator
if "%COMPILER%"=="mingw" goto :set_mingw_generator

:set_vs_generator
echo [INFO] Using Visual Studio compiler...

:: Try to detect the best Visual Studio version
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
    set GENERATOR=Visual Studio 17 2022
    echo [INFO] Found Visual Studio 2022
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" (
    set GENERATOR=Visual Studio 16 2019
    echo [INFO] Found Visual Studio 2019
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017" (
    set GENERATOR=Visual Studio 15 2017
    if "%PLATFORM%"=="x64" set GENERATOR=Visual Studio 15 2017 Win64
    echo [INFO] Found Visual Studio 2017
) else (
    :: Fallback to newest available
    set GENERATOR=Visual Studio 17 2022
    echo [INFO] Using Visual Studio 17 2022 (fallback)
)

:: Set platform for newer VS versions (2019+)
if "%GENERATOR%"=="Visual Studio 17 2022" set CMAKE_PLATFORM=-A %PLATFORM%
if "%GENERATOR%"=="Visual Studio 16 2019" set CMAKE_PLATFORM=-A %PLATFORM%

goto :start_build

:set_mingw_generator
echo [INFO] Using MinGW compiler...
set GENERATOR=MinGW Makefiles

:: Check if we can find mingw32-make
where mingw32-make.exe >nul 2>&1
if errorlevel 1 (
    where make.exe >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Neither mingw32-make nor make found in PATH!
        echo [ERROR] Please ensure MinGW-w64 is properly installed.
        exit /b 1
    )
)

goto :start_build

:start_build
echo [INFO] Build type: %BUILD_TYPE%
echo [INFO] Build directory: %BUILD_DIR%
echo [INFO] Generator: %GENERATOR%
if not "%CMAKE_PLATFORM%"=="" echo [INFO] Platform: %PLATFORM%

:: Check if CMake is installed
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is not installed. Please install CMake first.
    exit /b 1
)

:: Clean build directory if requested
if %CLEAN_BUILD%==1 (
    echo [INFO] Cleaning build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

:: Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: Configure project
echo [INFO] Configuring project...
if "%COMPILER%"=="vs" (
    cmake -G "%GENERATOR%" %CMAKE_PLATFORM% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..
) else (
    cmake -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..
)

if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)

:: Build project
echo [INFO] Building project...
if "%COMPILER%"=="vs" (
    cmake --build . --config %BUILD_TYPE% --parallel
) else (
    cmake --build . --parallel
)

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)

cd ..

:: Determine executable path
if "%COMPILER%"=="vs" (
    set EXE_PATH=%BUILD_DIR%\%BUILD_TYPE%\snake.exe
) else (
    set EXE_PATH=%BUILD_DIR%\snake.exe
)

echo [SUCCESS] Build completed successfully!
echo [INFO] Executable location: %EXE_PATH%

:: Check if executable exists and provide run instruction
if exist "%EXE_PATH%" (
    echo [INFO] Run "%EXE_PATH%" to play the game
) else (
    echo [WARNING] Executable not found at expected location.
    echo [WARNING] Please check the build output for the actual location.
)

echo.
pause