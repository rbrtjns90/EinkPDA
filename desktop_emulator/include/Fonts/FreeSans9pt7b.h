#ifndef FREESANS9PT7B_H
#define FREESANS9PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeSans9pt7bBitmaps[] = {0x00};
const GFXglyph FreeSans9pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeSans9pt7b = {
    (uint8_t*)FreeSans9pt7bBitmaps,
    (GFXglyph*)FreeSans9pt7bGlyphs,
    0x20, 0x7E, 18
};

#endif
