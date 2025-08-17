#ifndef FREESERIFBOLD9PT7B_H
#define FREESERIFBOLD9PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeSerifBold9pt7bBitmaps[] = {0x00};
const GFXglyph FreeSerifBold9pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeSerifBold9pt7b = {
    (uint8_t*)FreeSerifBold9pt7bBitmaps,
    (GFXglyph*)FreeSerifBold9pt7bGlyphs,
    0x20, 0x7E, 18
};

#endif
