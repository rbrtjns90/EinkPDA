@echo off
REM PocketMage Desktop Emulator Build Script for Windows
REM This script builds the desktop emulator using Windows-specific SDL2 backend

setlocal enabledelayedexpansion

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

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo %ERROR_COLOR%[ERROR]%NC% CMake not found! Please install CMake 3.16 or later
    echo %INFO_COLOR%[INFO]%NC% Download from: https://cmake.org/download/
    pause
    exit /b 1
)

for /f "tokens=3" %%i in ('cmake --version ^| findstr /R "cmake version"') do set CMAKE_VERSION=%%i
echo %SUCCESS_COLOR%[SUCCESS]%NC% Found CMake !CMAKE_VERSION!

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

REM Create and enter build directory
echo %INFO_COLOR%[INFO]%NC% Creating build directory: %BUILD_DIR%
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo %INFO_COLOR%[INFO]%NC% Configuring build with CMake...

REM Try different configuration approaches
set CMAKE_SUCCESS=0

REM First try with vcpkg toolchain if available
if defined VCPKG_ROOT (
    echo %INFO_COLOR%[INFO]%NC% Trying configuration with vcpkg toolchain...
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" >nul 2>&1
    if not errorlevel 1 (
        set CMAKE_SUCCESS=1
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Configuration successful with vcpkg
    )
)

REM If vcpkg failed or not available, try standard configuration
if !CMAKE_SUCCESS! == 0 (
    echo %INFO_COLOR%[INFO]%NC% Trying standard configuration...
    cmake .. -DCMAKE_BUILD_TYPE=Release
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
echo %INFO_COLOR%[INFO]%NC% Building project...
cmake --build . --config Release

REM Check if build was successful
if exist "Release\PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully!
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\Release\PocketMage_Desktop_Emulator.exe
    
    REM Check if data directory was copied
    if exist "data" (
        echo %SUCCESS_COLOR%[SUCCESS]%NC% Assets copied to build directory
    ) else (
        echo %WARNING_COLOR%[WARNING]%NC% Assets directory not found - some features may not work
    )
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%
    echo %INFO_COLOR%[INFO]%NC%   Release\PocketMage_Desktop_Emulator.exe
) else if exist "Debug\PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully! (Debug build)
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\Debug\PocketMage_Desktop_Emulator.exe
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%
    echo %INFO_COLOR%[INFO]%NC%   Debug\PocketMage_Desktop_Emulator.exe
) else if exist "PocketMage_Desktop_Emulator.exe" (
    echo %SUCCESS_COLOR%[SUCCESS]%NC% Build completed successfully!
    echo %INFO_COLOR%[INFO]%NC% Executable: %CD%\PocketMage_Desktop_Emulator.exe
    
    echo %INFO_COLOR%[INFO]%NC% To run the emulator:
    echo %INFO_COLOR%[INFO]%NC%   cd %BUILD_DIR%
    echo %INFO_COLOR%[INFO]%NC%   PocketMage_Desktop_Emulator.exe
) else (
    echo %ERROR_COLOR%[ERROR]%NC% Build failed - executable not found
    echo %INFO_COLOR%[INFO]%NC% Check the build output above for errors
    pause
    exit /b 1
)

echo %INFO_COLOR%[INFO]%NC% Build script completed
pause
