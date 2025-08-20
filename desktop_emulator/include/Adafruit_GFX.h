#ifndef ADAFRUIT_GFX_H
#define ADAFRUIT_GFX_H

#include "pocketmage_compat.h"

// GFX font structures
typedef struct {
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    uint8_t xAdvance;
    int8_t xOffset;
    int8_t yOffset;
} GFXglyph;

typedef struct {
    uint8_t* bitmap;
    GFXglyph* glyph;
    uint8_t first;
    uint8_t last;
    uint8_t yAdvance;
} GFXfont;

// Color constants
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0

class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual ~Adafruit_GFX() {}
    
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void startWrite() {}
    virtual void writePixel(int16_t x, int16_t y, uint16_t color) { drawPixel(x, y, color); }
    virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    virtual void endWrite() {}
    
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {}
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {}
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {}
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {}
    
    void setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }
    void setTextColor(uint16_t c) { textcolor = c; }
    void setTextSize(uint8_t s) { textsize = s; }
    void setFont(const GFXfont* f = nullptr) { gfxFont = f; }
    void print(const String& text) {}
    void println(const String& text) {}
    
    void getTextBounds(const String& str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = str.length() * 6;
        if (h) *h = 8;
    }
    
    int16_t width() const { return _width; }
    int16_t height() const { return _height; }
    
protected:
    int16_t _width, _height;
    int16_t cursor_x = 0, cursor_y = 0;
    uint16_t textcolor = WHITE;
    uint8_t textsize = 1;
    const GFXfont* gfxFont = nullptr;
};

#endif
