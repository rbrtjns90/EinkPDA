# Unicode/UTF-8 Support Implementation

This document describes the Unicode and UTF-8 support implementation in the PocketMage system, enabling proper handling of international characters and accented text.

## Overview

The PocketMage system implements comprehensive UTF-8 support for text input, rendering, and manipulation. This allows users to work with international characters, accented letters, and multi-byte Unicode sequences.

## Files with Unicode/UTF-8 Support

### Core UTF-8 Implementation Files

**1. `/Code/PocketMage_V3/include/globals.h`**
- Declares UTF-8 utility functions: `utf8_length()`, `utf8_pop_back_inplace()`
- Declares UTF-8 rendering functions: `printUTF8()`, `setCursorUTF8()`, `getTextBoundsUTF8()`
- Declares `utf8SafeBackspace()` for proper character deletion
- Declares `updateKeypressUTF8()` for UTF-8 aware keyboard input
- Defines `KA_CHAR` action for UTF-8 character input

**2. `/Code/PocketMage_V3/src/sysFunc.cpp`**
- Implements `utf8_char_size()` - determines byte length of UTF-8 character
- Implements `utf8_length()` - counts Unicode codepoints in string
- Implements `utf8_pop_back_inplace()` - safely removes last Unicode character
- Implements `updateKeypressUTF8()` - UTF-8 aware keyboard event processing
- Contains emulator UTF-8 text injection via `emulatorConsumeUTF8()`

**3. `/Code/PocketMage_V3/src/einkFunc.cpp`**
- Implements UTF-8 text rendering using U8g2 library
- Functions: `printUTF8()`, `setCursorUTF8()`, `getTextBoundsUTF8()`
- Uses `u8g2Gfx` wrapper for UTF-8 compatible font rendering
- Supports `u8g2_font_unifont_t_latin` font for international characters

### Application Integration Files

**4. `/Code/PocketMage_V3/src/HOME.cpp`**
- Uses `updateKeypressUTF8()` for keyboard input processing
- Handles `KA_CHAR` action for UTF-8 character input
- Uses `utf8SafeBackspace()` for proper character deletion
- Integrates with dead key composition system

**5. `/Code/PocketMage_V3/src/TXT.cpp`**
- Uses `updateKeypressUTF8()` for text editor input
- Handles `KA_CHAR` action for UTF-8 text entry
- Uses `utf8SafeBackspace()` in editor and filename entry
- Supports UTF-8 text in documents while restricting filenames to ASCII
- Integrates with dead key composition for accented characters

### Emulator-Specific Files

**6. `/desktop_emulator/src/hardware_shim.cpp`**
- Implements `emulatorConsumeUTF8()` for host system text input
- Implements `utf8SafeBackspace()` (simplified version for emulator)
- Maps regular character keys to `KA_CHAR` action
- Handles UTF-8 text injection from host keyboard

## Key UTF-8 Functions

### Character Size Detection
```cpp
static int utf8_char_size(uint8_t lead) {
  if ((lead & 0x80) == 0x00) return 1;  // ASCII
  if ((lead & 0xE0) == 0xC0) return 2;  // 2-byte UTF-8
  if ((lead & 0xF0) == 0xE0) return 3;  // 3-byte UTF-8
  if ((lead & 0xF8) == 0xF0) return 4;  // 4-byte UTF-8
  return 1;
}
```

### String Length (Codepoint Count)
```cpp
int utf8_length(const String& s) {
  int n = 0;
  const char* p = s.c_str();
  size_t i=0, N=s.length();
  while (i<N) {
    i += utf8_char_size((uint8_t)p[i]);
    ++n;
  }
  return n;
}
```

### Safe Character Removal
```cpp
void utf8_pop_back_inplace(String& s) {
  int len = s.length();
  if (len == 0) return;
  int i = len - 1;
  // Walk backwards to find start of last UTF-8 character
  while (i > 0 && (s[i] & 0xC0) == 0x80) --i;
  s = s.substring(0, i);
}
```

### UTF-8 Aware Keyboard Input
```cpp
KeyEvent updateKeypressUTF8() {
  KeyEvent ev{false, KA_NONE, "", 0, 0};
  
#ifdef DESKTOP_EMULATOR
  // Inject host text input at higher priority
  String host;
  if (emulatorConsumeUTF8(host)) {
    KeyEvent ev{true, KA_CHAR, host, 0, 0};
    return ev;
  }
#endif
  // ... matrix scanning and key processing
}
```

## UTF-8 Rendering System

### U8g2 Integration
- Uses `U8G2_FOR_ADAFRUIT_GFX` library for UTF-8 font rendering
- Supports `u8g2_font_unifont_t_latin` font with international characters
- Provides accurate text width measurements for UTF-8 strings

### Rendering Functions
```cpp
void printUTF8(int16_t x, int16_t y, const String& text) {
  u8g2Gfx.setCursor(x, y);
  u8g2Gfx.print(text);
}

void getTextBoundsUTF8(const String& text, int16_t x, int16_t y, 
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
  *w = u8g2Gfx.getUTF8Width(text.c_str());
  *h = u8g2Gfx.getFontAscent() - u8g2Gfx.getFontDescent();
  *x1 = x;
  *y1 = y - u8g2Gfx.getFontAscent();
}
```

## Text Input Processing

### Character Input Flow
1. **Keyboard Event**: `updateKeypressUTF8()` captures input
2. **Action Classification**: Input classified as `KA_CHAR` for text
3. **Dead Key Composition**: `composeDeadIfAny()` processes accents
4. **Text Addition**: Composed UTF-8 text added to current line
5. **Display Update**: UTF-8 rendering functions display text

### Backspace Handling
1. **Backspace Event**: `KA_BACKSPACE` action detected
2. **Safe Removal**: `utf8SafeBackspace()` removes last codepoint
3. **Boundary Respect**: Function respects UTF-8 character boundaries
4. **Display Update**: Screen updated with modified text

## Emulator vs Device Differences

### Device Implementation
- Uses hardware keyboard matrix scanning
- Loads UTF-8 fonts from flash memory
- Direct UTF-8 character processing from key mappings

### Emulator Implementation
- Receives UTF-8 text from host system via SDL2
- Uses `emulatorConsumeUTF8()` to inject host keyboard input
- Simplified `utf8SafeBackspace()` implementation
- Maps ASCII keys to `KA_CHAR` actions

## Supported Character Sets

### Latin Characters
- Basic ASCII (0x00-0x7F)
- Latin-1 Supplement (0x80-0xFF)
- Latin Extended-A (0x100-0x17F)
- Latin Extended-B (0x180-0x24F)

### Accented Characters
- Acute accents: á, é, í, ó, ú
- Grave accents: à, è, ì, ò, ù
- Circumflex: â, ê, î, ô, û
- Diaeresis: ä, ë, ï, ö, ü
- Tilde: ã, ñ, õ

### Special Characters
- Currency symbols: €, £, ¥
- Mathematical symbols: ±, ×, ÷
- Punctuation: «, », ‚, „, …

## Usage Examples

### Text Entry with Accents
1. Press `'` (dead key) + `a` → `á`
2. Type normally: `Café` appears correctly
3. Backspace removes entire characters, not bytes

### International Text
1. Switch to appropriate keyboard layout
2. Enter text in target language
3. UTF-8 rendering displays correctly
4. File operations preserve encoding

## Technical Notes

### Memory Considerations
- UTF-8 strings may use 1-4 bytes per character
- String length ≠ byte count for international text
- Use `utf8_length()` for character counting

### Font Requirements
- Requires UTF-8 compatible fonts (U8g2 unifont)
- Font must contain target character glyphs
- Fallback to replacement character for missing glyphs

### File Handling
- Text files saved in UTF-8 encoding
- Filenames restricted to ASCII for compatibility
- JSON keyboard layouts support UTF-8 character definitions
