#ifndef U8G2LIB_H
#define U8G2LIB_H

// Mock U8g2 library for desktop compilation
#include "pocketmage_compat.h"

class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C {
public:
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C() {}
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C(uint8_t rotation, int cs, int dc, int rst) {
        (void)rotation; (void)cs; (void)dc; (void)rst;
    }
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
    void drawVLine(int16_t x, int16_t y, int16_t h) {}
    void drawHLine(int16_t x, int16_t y, int16_t w) {}
    void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const unsigned char* bitmap) {}
    uint16_t getStrWidth(const char* str) { return str ? static_cast<uint16_t>(strlen(str) * 6) : 0; }
    uint16_t getDisplayWidth() const { return 128; }
    uint16_t getDisplayHeight() const { return 32; }
    void setContrast(uint8_t value) {}
};

// Provide minimal font symbol stubs via macros (pointers are unused in mocks)
#ifndef u8g2_font_ncenB24_tr
#define u8g2_font_ncenB24_tr (nullptr)
#endif
#ifndef u8g2_font_ncenB18_tr
#define u8g2_font_ncenB18_tr (nullptr)
#endif
#ifndef u8g2_font_ncenB14_tr
#define u8g2_font_ncenB14_tr (nullptr)
#endif
#ifndef u8g2_font_ncenB12_tr
#define u8g2_font_ncenB12_tr (nullptr)
#endif
#ifndef u8g2_font_ncenB10_tr
#define u8g2_font_ncenB10_tr (nullptr)
#endif
#ifndef u8g2_font_ncenB08_tr
#define u8g2_font_ncenB08_tr (nullptr)
#endif
#ifndef u8g2_font_5x7_tf
#define u8g2_font_5x7_tf (nullptr)
#endif

#endif // U8G2LIB_H
