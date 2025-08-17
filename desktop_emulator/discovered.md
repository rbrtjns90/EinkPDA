# PocketMage Desktop Emulator Build Issues - Discovery Report

## Current Status
The PocketMage desktop emulator has build and linking errors that need to be resolved to achieve successful compilation and execution.

## Key Findings

### 1. Filesystem Function Signatures Mismatch
**Issue**: The real PocketMage app uses filesystem functions with `fs::FS&` references, but the emulator lacks proper implementations.

**Real App Functions (in `sysFunc.cpp`)**:
- `void listDir(fs::FS &fs, const char *dirname)`
- `void writeFile(fs::FS &fs, const char *path, const char *message)`
- `void appendFile(fs::FS &fs, const char *path, const char *message)`
- `void renameFile(fs::FS &fs, const char *path1, const char *path2)`
- `void deleteFile(fs::FS &fs, const char *path)`
- `String readFileToString(fs::FS &fs, const char *path)`
- `void readFile(fs::FS &fs, const char *path)`

**Current Emulator Status**:
- `SD_MMCClass` does NOT inherit from `fs::FS` as required
- Missing filesystem function implementations with correct signatures
- `File` class partially implemented but missing some methods used by real app

### 2. Missing String Methods
**Issue**: The emulator's `String` class lacks methods used by the real PocketMage app.

**Missing Methods**:
- `startsWith(const String& prefix)`
- `isEmpty()`
- `toInt()`
- `toLowerCase()`
- `indexOf(char c)`
- `indexOf(const String& str)`

### 3. Missing File Mode Constants
**Issue**: Real app uses `FILE_READ`, `FILE_WRITE`, `FILE_APPEND` constants not defined in emulator.

**Used in Real App**:
- `FILE_READ` - for reading files
- `FILE_WRITE` - for writing files  
- `FILE_APPEND` - for appending to files

### 4. File Class Method Gaps
**Issue**: The emulator's `File` class is missing methods used by real app filesystem functions.

**Missing/Incomplete Methods**:
- `print(const char*)` and `print(const String&)` - return bool
- `println(const char*)` and `println(const String&)` - return bool
- Directory handling in constructor needs `isDir` member variable

**Already Implemented**:
- `isDirectory()`
- `openNextFile()`
- `name()`
- Basic file I/O methods

### 5. Build Configuration
**CMakeLists.txt Analysis**:
- Real PocketMage sources are included: `globals.cpp`, `HOME.cpp`, `TXT.cpp`, etc.
- **CRITICAL**: `sysFunc.cpp` is NOT included in build, meaning filesystem functions are missing
- Emulator must provide these functions or include `sysFunc.cpp`

### 6. Global Object Conflicts
**Status**: RESOLVED - Duplicate global definitions removed from `hardware_shim.cpp`

### 7. Inheritance Structure
**Issue**: `SD_MMCClass` must inherit from `fs::FS` for compatibility.

**Current**: `class SD_MMCClass` (standalone)
**Required**: `class SD_MMCClass : public fs::FS`

## File Locations

### Real PocketMage App
- **Source**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/src/`
- **Headers**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/Code/PocketMage_V3/include/`
- **Key Files**: `sysFunc.cpp`, `globals.h`, `globals.cpp`

### Desktop Emulator
- **Source**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/src/`
- **Headers**: `/Users/rjones/Documents/Programming files/pocketmage/EinkPDA/desktop_emulator/include/`
- **Key Files**: `hardware_shim.cpp`, `pocketmage_compat.h`, `SD_MMC.h`

## Next Steps Required

### Priority 1: Fix Filesystem Compatibility
1. **Add `sysFunc.cpp` to CMakeLists.txt** OR implement filesystem functions in emulator
2. **Make `SD_MMCClass` inherit from `fs::FS`** in `SD_MMC.h`
3. **Add missing File mode constants** (`FILE_READ`, `FILE_WRITE`, `FILE_APPEND`) to `pocketmage_compat.h`
4. **Fix File class `print`/`println` methods** to return `bool` in `hardware_shim.cpp`

### Priority 2: Complete String Class
1. **Add missing String methods** to `pocketmage_compat.h`:
   - `startsWith()`, `isEmpty()`, `toInt()`, `toLowerCase()`, `indexOf()`

### Priority 3: Test and Validate
1. **Build emulator** after fixes
2. **Test filesystem operations** with real PocketMage app code
3. **Verify runtime behavior** matches expected functionality

## Implementation Notes

### Option A: Include sysFunc.cpp in Build
- Add `${POCKETMAGE_SRC}/sysFunc.cpp` to `CMakeLists.txt`
- Ensure `SD_MMCClass` inherits from `fs::FS`
- Add missing constants and String methods

### Option B: Implement Functions in Emulator
- Create filesystem function implementations in `hardware_shim.cpp`
- Match exact signatures from `globals.h`
- Ensure compatibility with real app usage patterns

**Recommendation**: Option A (include sysFunc.cpp) for maximum compatibility and less duplication.

## Code Patterns Observed

### Real App Filesystem Usage
```cpp
// From HOME.cpp
listDir(SD_MMC, "/");

// From sysFunc.cpp  
File file = fs.open(path, FILE_WRITE);
if (file.print(message)) {
    Serial.println("- file written");
}
```

### Required Emulator Compatibility
```cpp
// SD_MMCClass must be compatible with fs::FS&
class SD_MMCClass : public fs::FS {
    // ... existing methods
};

// File constants needed
#define FILE_READ "r"
#define FILE_WRITE "w" 
#define FILE_APPEND "a"
```

## Current Build Status (Updated)

### Completed Fixes
- ✅ Added missing String methods (startsWith, isEmpty, toInt, toLowerCase, indexOf)
- ✅ Added FILE_READ, FILE_WRITE, FILE_APPEND constants
- ✅ Made SD_MMCClass inherit from fs::FS
- ✅ Fixed File class print/println methods to return bool
- ✅ Added sysFunc.cpp to CMakeLists.txt build sources
- ✅ Added missing isDir member variable to File class
- ✅ Added missing display methods (setFullWindow, fillRect, drawRect)
- ✅ Added missing keypad methods (enableInterrupts, disableInterrupts)
- ✅ Added missing Serial methods (println(), print() overloads)
- ✅ Added missing Buzzer methods (sound, end)
- ✅ Added missing String methods (charAt, trim)
- ✅ Added NOTE constants and TCA8418 register constants
- ✅ Fixed macro redefinition warnings with #ifndef guards

### Remaining Issues
- ❌ DateTime/TimeSpan operator compatibility issues
- ❌ File class copy assignment operator issues (std::ifstream/ofstream not copyable)
- ❌ Some missing Arduino utility functions

### Next Steps
1. Fix DateTime operators to work with TimeSpan properly
2. Implement File class with movable semantics or smart pointers
3. Complete remaining Arduino compatibility functions
4. Test successful build and runtime execution

This report captures the current state and provides a clear roadmap for resolving the PocketMage desktop emulator build issues.
