#!/bin/bash

# PocketMage Desktop Emulator Build Script for Linux/macOS
# This script builds the desktop emulator using platform-specific SDL2 backends

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    BUILD_DIR="build-linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    BUILD_DIR="build-macos"
else
    print_error "Unsupported platform: $OSTYPE"
    exit 1
fi

print_status "Building PocketMage Desktop Emulator for $PLATFORM"

# Check for required dependencies
print_status "Checking dependencies..."

if [[ "$PLATFORM" == "Linux" ]]; then
    # Check for SDL2 development packages on Linux
    if ! pkg-config --exists sdl2; then
        print_error "SDL2 development package not found!"
        print_status "On Ubuntu/Debian, install with: sudo apt-get install libsdl2-dev libsdl2-ttf-dev"
        print_status "On Fedora/RHEL, install with: sudo dnf install SDL2-devel SDL2_ttf-devel"
        print_status "On Arch Linux, install with: sudo pacman -S sdl2 sdl2_ttf"
        exit 1
    fi
    
    if ! pkg-config --exists SDL2_ttf; then
        print_error "SDL2_ttf development package not found!"
        print_status "Install SDL2_ttf development package for your distribution"
        exit 1
    fi
    
    print_success "SDL2 and SDL2_ttf found via pkg-config"
elif [[ "$PLATFORM" == "macOS" ]]; then
    # Check for Homebrew and SDL2 on macOS
    if ! command -v brew &> /dev/null; then
        print_warning "Homebrew not found. Install from https://brew.sh/"
        print_status "Attempting to use system SDL2..."
    else
        if ! brew list sdl2 &> /dev/null || ! brew list sdl2_ttf &> /dev/null; then
            print_status "Installing SDL2 via Homebrew..."
            brew install sdl2 sdl2_ttf
        fi
        print_success "SDL2 and SDL2_ttf found via Homebrew"
    fi
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake not found! Please install CMake 3.16 or later"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_success "Found CMake $CMAKE_VERSION"

# Create and enter build directory
print_status "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring build with CMake..."
if [[ "$PLATFORM" == "macOS" ]]; then
    # macOS-specific configuration
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
else
    # Linux configuration
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build the project
print_status "Building project..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check if build was successful
if [[ -f "PocketMage_Desktop_Emulator" ]]; then
    print_success "Build completed successfully!"
    print_status "Executable: $PWD/PocketMage_Desktop_Emulator"
    
    # Check if data directory was copied
    if [[ -d "data" ]]; then
        print_success "Assets copied to build directory"
    else
        print_warning "Assets directory not found - some features may not work"
    fi
    
    print_status "To run the emulator:"
    print_status "  cd $BUILD_DIR"
    print_status "  ./PocketMage_Desktop_Emulator"
else
    print_error "Build failed - executable not found"
    exit 1
fi
