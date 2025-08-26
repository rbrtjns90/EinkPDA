# Dead Key Keyboard Implementation

This document describes the dead key functionality implementation for the PocketMage keyboard system, specifically for the US Latin keyboard layout with apostrophe (') dead key support.

## Overview

Dead keys allow users to create accented characters by pressing an accent key followed by a base character. For example, pressing `'` then `a` produces `á`.

## Files with Dead Key Behavior

### Core Implementation Files

**1. `/Code/PocketMage_V3/include/globals.h`**
- Defines `KA_DEAD` enum value and other dead key actions
- Declares `DeadRule` struct and `DeadTable` vector
- Declares `CurrentDead` string variable
- Function declarations for `composeDeadIfAny()` and `loadKeyboardLayout()`

**2. `/Code/PocketMage_V3/src/globals.cpp`**
- Implements `composeDeadIfAny()` function with debug output
- Defines global variables: `DeadTable`, `CurrentDead`
- Contains dead key composition logic

**3. `/Code/PocketMage_V3/src/sysFunc.cpp`**
- Contains JSON parsing for dead key mappings from keyboard layout files
- Implements `loadKeyboardLayoutFromFile()` that populates `DeadTable` from JSON
- Has duplicate `composeDeadIfAny()` function (fallback version)
- Parses `"dead:"` prefix in keyboard mapping strings

### Application Integration Files

**4. `/Code/PocketMage_V3/src/HOME.cpp`**
- Handles `KA_DEAD` action in command line interface
- Sets `CurrentDead` when dead key is pressed
- Uses `composeDeadIfAny()` for character composition

**5. `/Code/PocketMage_V3/src/TXT.cpp`**
- Handles `KA_DEAD` action in text editor
- Sets `CurrentDead` when dead key is pressed  
- Uses `composeDeadIfAny()` for character composition

### Emulator-Specific Files

**6. `/desktop_emulator/src/hardware_shim.cpp`**
- Maps apostrophe key (keycode 100) to `KA_DEAD` action
- Manually populates `DeadTable` with apostrophe combinations
- Implements emulator version of `loadKeyboardLayout()`

**7. `/desktop_emulator/src/main_new.cpp`**
- Calls `loadKeyboardLayout("us-latin")` during emulator startup
- Contains forward declaration for `loadKeyboardLayout()`

### Configuration Files

**8. `/desktop_emulator/data/sys/kbd/us-latin.json`**
- Contains dead key mappings in JSON format under `"layers"."dead"`
- Defines apostrophe dead key combinations for accented characters

**9. `/Code/PocketMage_V3/data/sys/kbd/us-latin.json`**
- Device version of the keyboard layout with dead key mappings

## How It Works

### Dead Key Activation
1. User presses apostrophe (') key
2. Hardware shim maps keycode 100 to `KA_DEAD` action
3. Application (HOME or TXT) sets `CurrentDead = "'"`

### Character Composition
1. User presses base character (e.g., 'a')
2. Application calls `composeDeadIfAny("a")`
3. Function searches `DeadTable` for accent "'" + base "a"
4. Returns composed character "á" if found, otherwise returns base character
5. `CurrentDead` is cleared after composition

### Supported Combinations

The apostrophe (') dead key supports these combinations:

| Base | Result | Base | Result |
|------|--------|------|--------|
| a    | á      | A    | Á      |
| e    | é      | E    | É      |
| i    | í      | I    | Í      |
| o    | ó      | O    | Ó      |
| u    | ú      | U    | Ú      |
| y    | ý      | Y    | Ý      |
| c    | ć      | C    | Ć      |
| n    | ń      | N    | Ń      |
| s    | ś      | S    | Ś      |
| z    | ź      | Z    | Ź      |

## Key Data Structures

### DeadRule
```cpp
struct DeadRule {
  String accent;  // The dead key character (e.g., "'")
  String base;    // The base character (e.g., "a")
  String out;     // The composed result (e.g., "á")
};
```

### Global Variables
- `std::vector<DeadRule> DeadTable` - Contains all dead key rules
- `String CurrentDead` - Currently active dead key (empty if none)
- `String CurrentLayoutName` - Name of current keyboard layout

## Implementation Notes

### Emulator vs Device
- **Device**: Loads dead key mappings from JSON files via `loadKeyboardLayoutFromFile()`
- **Emulator**: Manually populates `DeadTable` in `hardware_shim.cpp` since JSON loading isn't working

### Fallback Behavior
If no matching dead key combination is found, the function returns just the base character (not accent + base) to avoid unwanted character output.

### Debug Output
The implementation includes extensive debug logging to track:
- Dead key activation
- Composition attempts
- DeadTable population
- Match results

## Usage

### In HOME Command Line
1. Press `'` (apostrophe) - activates dead key
2. Press `a` - outputs `á`
3. Character appears in command line

### In TXT Editor
1. Press `'` (apostrophe) - activates dead key  
2. Press `a` - outputs `á`
3. Character appears in text document

Both applications use the same underlying composition logic through `composeDeadIfAny()`.
