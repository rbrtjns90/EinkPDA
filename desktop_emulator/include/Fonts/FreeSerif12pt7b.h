#ifndef FREESERIF12PT7B_H
#define FREESERIF12PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeSerif12pt7bBitmaps[] = {0x00};
const GFXglyph FreeSerif12pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeSerif12pt7b = {
    (uint8_t*)FreeSerif12pt7bBitmaps,
    (GFXglyph*)FreeSerif12pt7bGlyphs,
    0x20, 0x7E, 24
};

#endif
