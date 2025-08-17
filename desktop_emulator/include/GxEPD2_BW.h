#ifndef GXEPD2_BW_H
#define GXEPD2_BW_H

// Mock GxEPD2 library for desktop compilation
#include "pocketmage_compat.h"

// Mock GxEPD2 display class
class GxEPD2_BW {
public:
    void init(unsigned long baud = 0) {}
    void setRotation(uint8_t rotation) {}
    void setTextColor(uint16_t color) {}
    void setFullWindow() {}
    void clearScreen() {}
    void display() {}
    void displayPartial() {}
    void setFont(const void* font) {}
    void setCursor(int16_t x, int16_t y) {}
    void print(const String& text) {}
    void println(const String& text) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) {}
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {}
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    void fillScreen(uint16_t color) {}
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {}
    void getTextBounds(const String& text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = text.length() * 6;
        if (h) *h = 8;
    }
};

#endif // GXEPD2_BW_H
