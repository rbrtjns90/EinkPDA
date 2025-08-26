#!/bin/bash

# Build script for PocketMage Desktop Emulator
set -e

echo "[INFO] Building PocketMage Desktop Emulator (Test Version) for macOS"

# Check dependencies
echo "[INFO] Checking dependencies..."

# Check for SDL2 via Homebrew
if brew list sdl2 &>/dev/null && brew list sdl2_ttf &>/dev/null; then
    echo "[SUCCESS] SDL2 and SDL2_ttf found via Homebrew"
else
    echo "[ERROR] SDL2 or SDL2_ttf not found. Please install with:"
    echo "  brew install sdl2 sdl2_ttf"
    exit 1
fi

# Check for CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo "[SUCCESS] Found CMake $CMAKE_VERSION"
else
    echo "[ERROR] CMake not found. Please install with:"
    echo "  brew install cmake"
    exit 1
fi

# Create build directory
BUILD_DIR="build-test"
echo "[INFO] Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure build with CMake
echo "[INFO] Configuring build with CMake..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build project
echo "[INFO] Building project..."
make -j$(sysctl -n hw.ncpu)

echo "[SUCCESS] Build completed successfully!"
echo "[INFO] Executable: $(pwd)/PocketMage_Desktop_Emulator"
echo "[SUCCESS] Assets copied to build directory"

echo "[INFO] To run the emulator:"
echo "[INFO]   cd $BUILD_DIR"
echo "[INFO]   ./PocketMage_Desktop_Emulator"