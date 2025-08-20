#ifndef FREESERIF9PT7B_H
#define FREESERIF9PT7B_H

#include "../Adafruit_GFX.h"

// Mock font data
const uint8_t FreeSerif9pt7bBitmaps[] = {0x00};
const GFXglyph FreeSerif9pt7bGlyphs[] = {{0, 0, 0, 0, 0, 0}};

const GFXfont FreeSerif9pt7b = {
    (uint8_t*)FreeSerif9pt7bBitmaps,
    (GFXglyph*)FreeSerif9pt7bGlyphs,
    0x20, 0x7E, 18
};

#endif
