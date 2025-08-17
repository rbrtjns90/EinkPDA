#ifndef FREESANS12PT7B_H
#define FREESANS12PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeSans12pt7bBitmaps[] = {0x00};
const GFXglyph FreeSans12pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeSans12pt7b = {
    (uint8_t*)FreeSans12pt7bBitmaps,
    (GFXglyph*)FreeSans12pt7bGlyphs,
    0x20, 0x7E, 24
};

#endif
