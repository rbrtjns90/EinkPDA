#ifndef FREEMONO12PT7B_H
#define FREEMONO12PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeMono12pt7bBitmaps[] = {0x00};
const GFXglyph FreeMono12pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeMono12pt7b = {
    (uint8_t*)FreeMono12pt7bBitmaps,
    (GFXglyph*)FreeMono12pt7bGlyphs,
    0x20, 0x7E, 24
};

#endif
