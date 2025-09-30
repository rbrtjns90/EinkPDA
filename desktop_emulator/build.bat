@echo off
REM PocketMage Desktop Emulator Build Script for Windows
REM This script builds the desktop emulator using Windows-specific SDL2 backend
REM Usage: build.bat [Debug|Release]
REM Default: Debug

setlocal enabledelayedexpansion

REM Parse build type argument
set BUILD_TYPE=Debug
if not "%1"=="" (
    set BUILD_TYPE=%1
)
echo Build type: %BUILD_TYPE%

REM Colors for output (using Windows console colors)
set "INFO_COLOR=[94m"
set "SUCCESS_COLOR=[92m"
set "WARNING_COLOR=[93m"
set "ERROR_COLOR=[91m"
set "NC=[0m"

REM Function to print colored output
echo %INFO_COLOR%[INFO]%NC% Building PocketMage Desktop Emulator for Windows

REM Set build directory
set BUILD_DIR=build-windows

REM Check for required tools
echo %INFO_COLOR%[INFO]%NC% Checking dependencies...

REM Check for CMake and find it in common locations
set "CMAKE_EXE=cmake"
cmake --version >nul 2>&1
if errorlevel 1 (
    echo %WARNING_COLOR%[WARNING]%NC% CMake not in PATH, searching common locations...
    
    REM Check common CMake installation paths
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Found CMake in Program Files
    ) else if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files (x86)\CMake\bin\cmake.exe"
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Found CMake in Program Files (x86)
    ) else if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Found CMake in ProgramFiles directory
    ) else (
        echo %ERROR_COLOR%[ERROR]%NC% CMake not found! Please install CMake 3.16 or later
        echo %INFO_COLOR%[INFO]%NC% Download from: https://cmake.org/download/
        echo %INFO_COLOR%[INFO]%NC% Or add CMake to your PATH
        pause
        exit /b 1
    )
) else (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Found CMake in PATH
)

REM Get CMake version
for /f "tokens=3" %%i in ('"%CMAKE_EXE%" --version ^| findstr /R "cmake version"') do set CMAKE_VERSION=%%i
echo %INFO_COLOR%[INFO]%NC% CMake version: !CMAKE_VERSION!

REM Check for Visual Studio or Build Tools
set VS_FOUND=0

REM Check for Visual Studio 2022
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1

REM Check for Visual Studio 2019
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1

REM Check for Build Tools
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set VS_FOUND=1

if !VS_FOUND! == 0 (
    echo %WARNING_COLOR%[WARNING]%NC% Visual Studio or Build Tools not detected
    echo %INFO_COLOR%[INFO]%NC% Install Visual Studio Community 2019/2022 or Build Tools for Visual Studio
    echo %INFO_COLOR%[INFO]%NC% Download from: https://visualstudio.microsoft.com/downloads/
    echo %INFO_COLOR%[INFO]%NC% Continuing anyway - CMake will try to find a suitable compiler...
) else (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Found Visual Studio or Build Tools
)

REM Check for vcpkg (recommended for SDL2 on Windows)
if exist "C:\vcpkg\vcpkg.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Found vcpkg at C:\vcpkg\
    set VCPKG_ROOT=C:\vcpkg
) else if exist "vcpkg\vcpkg.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Found vcpkg in current directory
    set VCPKG_ROOT=%CD%\vcpkg
) else if defined VCPKG_ROOT (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Found vcpkg via VCPKG_ROOT environment variable
) else (
    echo %WARNING_COLOR%[WARNING]%NC% vcpkg not found - you may need to install SDL2 manually
    echo %INFO_COLOR%[INFO]%NC% To install vcpkg and SDL2:
    echo %INFO_COLOR%[INFO]%NC%   git clone https://github.com/Microsoft/vcpkg.git
    echo %INFO_COLOR%[INFO]%NC%   cd vcpkg
    echo %INFO_COLOR%[INFO]%NC%   .\bootstrap-vcpkg.bat
    echo %INFO_COLOR%[INFO]%NC%   .\vcpkg install sdl2 sdl2-ttf
)

REM Download/fix fonts before building
echo %INFO_COLOR%[INFO]%NC% Checking and downloading fonts...
powershell -ExecutionPolicy Bypass -File "%~dp0utils\download-fonts.ps1"
if errorlevel 1 (
    echo %WARNING_COLOR%[WARNING]%NC% Font download had issues, but continuing build...
)

REM Create and enter build directory
echo %INFO_COLOR%[INFO]%NC% Creating build directory: %BUILD_DIR%
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo %INFO_COLOR%[INFO]%NC% Configuring build with CMake (Build Type: %BUILD_TYPE%)...

REM Try different configuration approaches
set CMAKE_SUCCESS=0

REM First try with vcpkg toolchain if available
if defined VCPKG_ROOT (
    echo %INFO_COLOR%[INFO]%NC% Trying configuration with vcpkg toolchain...
    "%CMAKE_EXE%" .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" >nul 2>&1
    if not errorlevel 1 (
        set CMAKE_SUCCESS=1
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Configuration successful with vcpkg
    )
)

REM If vcpkg failed or not available, try standard configuration
if !CMAKE_SUCCESS! == 0 (
    echo %INFO_COLOR%[INFO]%NC% Trying standard configuration...
    "%CMAKE_EXE%" .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    if not errorlevel 1 (
        set CMAKE_SUCCESS=1
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Configuration successful
    ) else (
        echo %ERROR_COLOR%[ERROR]%NC% CMake configuration failed!
        echo %INFO_COLOR%[INFO]%NC% Make sure SDL2 and SDL2_ttf are installed
        echo %INFO_COLOR%[INFO]%NC% Consider using vcpkg: .\vcpkg install sdl2 sdl2-ttf
        pause
        exit /b 1
    )
)

REM Build the project
echo %INFO_COLOR%[INFO]%NC% Building project (Config: %BUILD_TYPE%)...
echo.
"%CMAKE_EXE%" --build . --config %BUILD_TYPE% 2>&1
set BUILD_RESULT=%ERRORLEVEL%
echo.

REM Go back to parent directory to display results
cd ..

REM Check if build was successful
if %BUILD_RESULT% NEQ 0 (
    echo %ERROR_COLOR%[ERROR]%NC% Build failed with error code %BUILD_RESULT%
    echo %INFO_COLOR%[INFO]%NC% Check the build output above for errors
    pause
    exit /b 1
)

if exist "%BUILD_DIR%\Release\PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully!
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\%BUILD_DIR%\Release\PocketMage_Desktop_Emulator.exe
    echo.
    
    REM Check if data directory was copied
    if exist "%BUILD_DIR%\Release\data" (
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Assets copied to build directory
    ) else (
        echo %WARNING_COLOR%[WARNING]%NC% Assets directory not found - some features may not work
    )
    echo.
    
    REM Create convenience run script
    echo @echo off > run_pocketmage.bat
    echo cd /d "%%~dp0%BUILD_DIR%\Release" >> run_pocketmage.bat
    echo start PocketMage_Desktop_Emulator.exe >> run_pocketmage.bat
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Created run_pocketmage.bat in current directory
    echo.
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   .\run_pocketmage.bat
    echo %INFO_COLOR%[INFO]%NC% Or manually:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%\Release
    echo %INFO_COLOR%[INFO]%NC%   .\PocketMage_Desktop_Emulator.exe
    echo.
    goto :build_success
)

if exist "%BUILD_DIR%\Debug\PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully! (Debug build)
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\%BUILD_DIR%\Debug\PocketMage_Desktop_Emulator.exe
    echo.
    
    REM Create convenience run script
    echo @echo off > run_pocketmage.bat
    echo cd /d "%%~dp0%BUILD_DIR%\Debug" >> run_pocketmage.bat
    echo start PocketMage_Desktop_Emulator.exe >> run_pocketmage.bat
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Created run_pocketmage.bat in current directory
    echo.
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   .\run_pocketmage.bat
    echo %INFO_COLOR%[INFO]%NC% Or manually:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%\Debug
    echo %INFO_COLOR%[INFO]%NC%   .\PocketMage_Desktop_Emulator.exe
    echo.
    goto :build_success
)

if exist "%BUILD_DIR%\PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully!
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\%BUILD_DIR%\PocketMage_Desktop_Emulator.exe
    echo.
    
    REM Create convenience run script
    echo @echo off > run_pocketmage.bat
    echo cd /d "%%~dp0%BUILD_DIR%" >> run_pocketmage.bat
    echo start PocketMage_Desktop_Emulator.exe >> run_pocketmage.bat
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Created run_pocketmage.bat in current directory
    echo.
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   .\run_pocketmage.bat
    echo %INFO_COLOR%[INFO]%NC% Or manually:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%
    echo %INFO_COLOR%[INFO]%NC%   .\PocketMage_Desktop_Emulator.exe
    echo.
    goto :build_success
)

echo %ERROR_COLOR%[ERROR]%NC% Build succeeded but executable not found
echo %INFO_COLOR%[INFO]%NC% This may indicate a configuration issue
pause
exit /b 1

:build_success

echo %INFO_COLOR%[INFO]%NC% Build script completed
pause
