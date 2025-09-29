#!/bin/bash

# PocketMage Desktop Emulator Build Script for Linux/macOS
# This script builds the desktop emulator using platform-specific SDL2 backends
#
# Usage: ./build.sh [--dry-run] [--clean]
#   --dry-run: Check dependencies and configuration without building
#   --clean:   Clean build directory before building

set -e  # Exit on any error

# Parse command line arguments
DRY_RUN=false
CLEAN_BUILD=false

for arg in "$@"; do
    case $arg in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help|-h)
            echo "PocketMage Desktop Emulator Build Script"
            echo ""
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --dry-run    Check dependencies without building"
            echo "  --clean      Clean build directory before building"
            echo "  --help, -h   Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

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
        print_status "Checking SDL2 installation via Homebrew..."
        if ! brew list sdl2 &> /dev/null; then
            print_status "Installing SDL2 via Homebrew..."
            brew install sdl2
        fi
        if ! brew list sdl2_ttf &> /dev/null; then
            print_status "Installing SDL2_ttf via Homebrew..."
            brew install sdl2_ttf
        fi
        print_success "SDL2 and SDL2_ttf available via Homebrew"
        
        # Set SDL2 paths for CMake on macOS
        export SDL2_DIR=$(brew --prefix sdl2)
        export SDL2_TTF_DIR=$(brew --prefix sdl2_ttf)
        print_status "SDL2 paths configured: SDL2_DIR=$SDL2_DIR"
    fi
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake not found! Please install CMake 3.16 or later"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_success "Found CMake $CMAKE_VERSION"

# Download fonts if needed
print_status "Checking and downloading fonts..."
FONTS_SCRIPT="$(dirname "$0")/fonts/download_fonts.sh"
if [[ -f "$FONTS_SCRIPT" ]]; then
    if [[ "$DRY_RUN" == true ]]; then
        print_status "Would run font downloader: $FONTS_SCRIPT"
    else
        print_status "Running font downloader..."
        if bash "$FONTS_SCRIPT"; then
            print_success "Fonts ready"
        else
            print_error "Font download failed"
            exit 1
        fi
    fi
else
    print_warning "Font downloader script not found at: $FONTS_SCRIPT"
    print_status "Fonts may need to be downloaded manually"
fi

# Handle clean build option
if [[ "$CLEAN_BUILD" == true ]]; then
    print_status "Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# Create and enter build directory
print_status "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Exit early if this is a dry run
if [[ "$DRY_RUN" == true ]]; then
    print_success "Dry run completed - all dependencies and configuration look good!"
    print_status "To build for real, run: ./build.sh"
    exit 0
fi

# Configure with CMake
print_status "Configuring build with CMake..."
if [[ "$PLATFORM" == "macOS" ]]; then
    # macOS-specific configuration
    CMAKE_ARGS=(
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
        -DCMAKE_OSX_ARCHITECTURES="$(uname -m)"
    )
    
    # Add SDL2 paths if available via Homebrew
    if command -v brew &> /dev/null; then
        CMAKE_ARGS+=(
            -DSDL2_DIR="$(brew --prefix sdl2)/lib/cmake/SDL2"
            -DSDL2_TTF_DIR="$(brew --prefix sdl2_ttf)/lib/cmake/SDL2_ttf"
        )
    fi
    
    print_status "CMake arguments: ${CMAKE_ARGS[*]}"
    cmake .. "${CMAKE_ARGS[@]}"
else
    # Linux configuration
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build the project
print_status "Building project..."
if [[ "$PLATFORM" == "macOS" ]]; then
    # Use macOS-specific CPU count detection
    NCPU=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    print_status "Building with $NCPU parallel jobs..."
    cmake --build . --config Release -j$NCPU
else
    # Linux build
    cmake --build . --config Release -j$(nproc 2>/dev/null || echo 4)
fi

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
    
    if [[ "$PLATFORM" == "macOS" ]]; then
        print_status ""
        print_status "macOS-specific notes:"
        print_status "  • If you get permission errors, run: chmod +x PocketMage_Desktop_Emulator"
        print_status "  • For e-ink simulation, set: export POCKETMAGE_EINK_SIM=1"
        print_status "  • Hotkeys: F5-F8 control e-ink simulation parameters"
        print_status "  • If SDL2 issues occur, try: brew reinstall sdl2 sdl2_ttf"
    fi
else
    print_error "Build failed - executable not found"
    if [[ "$PLATFORM" == "macOS" ]]; then
        print_error "Common macOS build issues:"
        print_error "  • Ensure Xcode Command Line Tools: xcode-select --install"
        print_error "  • Update Homebrew: brew update && brew upgrade"
        print_error "  • Check SDL2 installation: brew list sdl2 sdl2_ttf"
        print_error "  • Try clean build: rm -rf $BUILD_DIR && ./build.sh"
    fi
    exit 1
fi
