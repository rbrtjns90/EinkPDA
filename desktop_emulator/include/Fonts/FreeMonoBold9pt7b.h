#ifndef FREEMONOBOLD9PT7B_H
#define FREEMONOBOLD9PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeMonoBold9pt7bBitmaps[] = {0x00};
const GFXglyph FreeMonoBold9pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeMonoBold9pt7b = {
    (uint8_t*)FreeMonoBold9pt7bBitmaps,
    (GFXglyph*)FreeMonoBold9pt7bGlyphs,
    0x20, 0x7E, 18
};

#endif
