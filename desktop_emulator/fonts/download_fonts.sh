#!/bin/bash

# Font Downloader Script for PocketMage Desktop Emulator
# Downloads DejaVu fonts if they don't exist locally
# Usage: ./download_fonts.sh

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONTS_DIR="$SCRIPT_DIR"

# Font URLs and filenames
DEJAVU_SANS_URL="https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSans.ttf"
DEJAVU_SANS_MONO_URL="https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSansMono.ttf"

DEJAVU_SANS_FILE="$FONTS_DIR/DejaVuSans.ttf"
DEJAVU_SANS_MONO_FILE="$FONTS_DIR/DejaVuSansMono.ttf"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}PocketMage Font Downloader${NC}"
echo "=================================="

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to download a file with progress
download_file() {
    local url="$1"
    local output="$2"
    local filename="$(basename "$output")"
    
    echo -e "${YELLOW}Downloading $filename...${NC}"
    
    if command_exists curl; then
        curl -L --progress-bar -o "$output" "$url"
    elif command_exists wget; then
        wget --progress=bar:force -O "$output" "$url"
    else
        echo -e "${RED}Error: Neither curl nor wget found. Please install one of them.${NC}"
        exit 1
    fi
    
    if [ -f "$output" ] && [ -s "$output" ]; then
        local size=$(du -h "$output" | cut -f1)
        echo -e "${GREEN}✓ Downloaded $filename ($size)${NC}"
    else
        echo -e "${RED}✗ Failed to download $filename${NC}"
        exit 1
    fi
}

# Function to verify font file
verify_font() {
    local font_file="$1"
    local filename="$(basename "$font_file")"
    
    # Check if file exists and has reasonable size (> 100KB)
    if [ -f "$font_file" ] && [ $(stat -f%z "$font_file" 2>/dev/null || stat -c%s "$font_file" 2>/dev/null || echo 0) -gt 100000 ]; then
        local size=$(du -h "$font_file" | cut -f1)
        echo -e "${GREEN}✓ $filename exists ($size)${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠ $filename missing or invalid${NC}"
        return 1
    fi
}

# Check existing fonts
echo -e "\n${BLUE}Checking existing fonts...${NC}"

need_dejavu_sans=false
need_dejavu_sans_mono=false

if ! verify_font "$DEJAVU_SANS_FILE"; then
    need_dejavu_sans=true
fi

if ! verify_font "$DEJAVU_SANS_MONO_FILE"; then
    need_dejavu_sans_mono=true
fi

# Download missing fonts
if [ "$need_dejavu_sans" = true ] || [ "$need_dejavu_sans_mono" = true ]; then
    echo -e "\n${BLUE}Downloading missing fonts...${NC}"
    
    if [ "$need_dejavu_sans" = true ]; then
        download_file "$DEJAVU_SANS_URL" "$DEJAVU_SANS_FILE"
    fi
    
    if [ "$need_dejavu_sans_mono" = true ]; then
        download_file "$DEJAVU_SANS_MONO_URL" "$DEJAVU_SANS_MONO_FILE"
    fi
    
    echo -e "\n${GREEN}Font download complete!${NC}"
else
    echo -e "\n${GREEN}All fonts already present!${NC}"
fi

# Final verification
echo -e "\n${BLUE}Final verification...${NC}"
fonts_ok=true

if ! verify_font "$DEJAVU_SANS_FILE"; then
    fonts_ok=false
fi

if ! verify_font "$DEJAVU_SANS_MONO_FILE"; then
    fonts_ok=false
fi

if [ "$fonts_ok" = true ]; then
    echo -e "\n${GREEN}✓ All fonts ready for PocketMage emulator!${NC}"
    echo -e "${BLUE}Fonts location: $FONTS_DIR${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Font setup incomplete. Please check the errors above.${NC}"
    exit 1
fi
