#ifndef U8G2LIB_H
#define U8G2LIB_H

// Mock U8g2 library for desktop compilation
#include "pocketmage_compat.h"

class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C {
public:
    void begin() {}
    void setBusClock(uint32_t clock) {}
    void setPowerSave(uint8_t is_enable) {}
    void clearBuffer() {}
    void sendBuffer() {}
    void setFont(const uint8_t* font) {}
    void setCursor(int16_t x, int16_t y) {}
    void print(const String& text) {}
    void drawStr(int16_t x, int16_t y, const char* str) {}
    void drawPixel(int16_t x, int16_t y) {}
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {}
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {}
};

#endif // U8G2LIB_H
