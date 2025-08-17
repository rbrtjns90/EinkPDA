# PocketMage Desktop Emulator

A desktop SDL2-based emulator for the PocketMage EinkPDA project, allowing you to develop and test the Pokédex app without constantly flashing to hardware.

## Features

- **Dual Display Windows**: Separate windows for E-Ink (310x128) and OLED (256x32) displays
- **Keyboard Input Mapping**: Desktop keyboard mapped to PocketMage controls
- **Filesystem Shim**: Loads data files from `./data/` directory
- **Real-time Testing**: See your changes immediately without hardware flashing

## Quick Start

### Prerequisites

**macOS:**
```bash
brew install sdl2 sdl2_ttf cmake
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev cmake g++
```

**Windows:**
```bash
vcpkg install sdl2 sdl2-ttf
```

### Build & Run

```bash
cd desktop_emulator
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/PocketMage_Desktop_Emulator
```

## Controls

| Desktop Key | PocketMage Function |
|-------------|-------------------|
| Arrow Keys  | Navigation (Up/Down/Left/Right) |
| Enter       | Select/Confirm |
| Backspace   | Delete/Back |
| Letters     | Text input |
| Numbers     | Numeric input |
| Space       | Space character |
| Close Window| Quit emulator |

## File Structure

```
desktop_emulator/
├── CMakeLists.txt          # Build configuration
├── include/
│   ├── desktop_display.h   # SDL2 display interface
│   └── hardware_shim.h     # Hardware abstraction layer
├── src/
│   ├── main.cpp           # Main emulator entry point
│   ├── desktop_display.cpp # SDL2 implementation
│   └── hardware_shim.cpp  # Mock hardware functions
├── data/                  # Data files directory
│   └── .gitkeep
└── README.md
```

## Display Windows

- **E-Ink Window**: 310x128 pixels, scaled 3x for visibility (930x384 on screen)
- **OLED Window**: 256x32 pixels, scaled 3x for visibility (768x96 on screen)

Both windows update in real-time as your code draws to the displays.

## Data Directory

Place your PocketMage data files in the `data/` directory:
- `sys/` - System files
- `journal/` - Journal entries  
- `dict/` - Dictionary files
- Text files, tasks, etc.

The emulator will create this structure automatically when needed.

## Development Workflow

1. **Code Changes**: Modify your PocketMage application code
2. **Build**: Run `cmake --build build` to rebuild
3. **Test**: Launch the emulator to see changes immediately
4. **Debug**: Use desktop debugging tools and console output
5. **Deploy**: Flash to hardware only for final validation

## Hardware Abstraction

The emulator provides mock implementations for:
- **Displays**: GxEPD2 (E-Ink) and U8G2 (OLED)
- **Input**: TCA8418 keyboard matrix
- **Storage**: SD card filesystem
- **Sensors**: Battery, touch slider, RTC
- **Communication**: Serial, preferences

## Extending the Emulator

To add support for more PocketMage features:

1. Add mock implementations in `hardware_shim.h/cpp`
2. Update the display interface in `desktop_display.h/cpp`  
3. Modify `main.cpp` to include your app's logic
4. Rebuild and test

## Troubleshooting

**Font Issues**: The emulator tries to load system fonts. If text doesn't appear:
- macOS: Ensure Monaco font is available
- Linux: Install DejaVu fonts (`sudo apt-get install fonts-dejavu`)
- Windows: Update font paths in `desktop_display.cpp`

**Build Errors**: Ensure SDL2 and SDL2_ttf development libraries are installed.

**Performance**: The emulator runs at ~30 FPS. Adjust the delay in `main.cpp` if needed.
