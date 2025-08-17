# Duplicate Symbol Analysis for PocketMage Desktop Emulator

This document details all duplicate symbol definitions causing linker errors and their locations.

## Duplicate Symbols Found

### 1. Functions

#### `TCA8418_irq()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `processKB()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/PocketMageV3.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `updateBattState()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `removeChar(String, char)`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `setup()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/PocketMageV3.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `loop()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/PocketMageV3.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `PWR_BTN_irq()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `printDebug()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

#### `checkTimeout()`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/sysFunc.cpp`
- **Action**: Remove from emulator (real implementation needed)

### 2. Global Variables

#### `_noTimeout`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_editingFile`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_SAVE_POWER`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_keypad`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_DEBUG_VERBOSE`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_rtc`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

#### `_u8g2`
- **Emulator**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
- **Real Code**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/globals.cpp`
- **Action**: Remove from emulator (real variable needed)

## Resolution Strategy

1. **Remove all duplicate function implementations** from `hardware_shim.cpp` - the real PocketMage implementations should be used
2. **Remove all duplicate global variable definitions** from `hardware_shim.cpp` - the real PocketMage globals should be used
3. **Keep only hardware abstraction layer functions** in `hardware_shim.cpp` that provide SDL2 integration
4. **Use extern declarations** for functions and variables that need to be referenced but are defined in real PocketMage code

## Files to Modify

### Primary File
- `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/hardware_shim.cpp`
  - Remove duplicate function implementations
  - Remove duplicate global variable definitions
  - Add extern declarations where needed

### Secondary Files (if needed)
- `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/include/pocketmage_compat.h`
  - Ensure no conflicting declarations

## Current Status
- Compilation successful with warnings
- Linker failing due to 16 duplicate symbols
- Need to clean up hardware_shim.cpp to remove duplicates
