#ifndef U8G2LIB_H
#define U8G2LIB_H

// Mock U8g2 library for desktop compilation
#include "pocketmage_compat.h"
#include "desktop_display.h"

class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C {
private:
    int16_t cursor_x = 0, cursor_y = 0;
    
public:
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C() {}
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C(uint8_t rotation, int cs, int dc, int rst) {
        (void)rotation; (void)cs; (void)dc; (void)rst;
    }
    void begin() {}
    void setBusClock(uint32_t clock) {}
    void setPowerSave(uint8_t is_enable) {}
    void clearBuffer() {
        if (g_display) g_display->oledClear();
    }
    void sendBuffer() {
        if (g_display) g_display->oledUpdate();
    }
    void setFont(const uint8_t* font) {}
    void setCursor(int16_t x, int16_t y) {
        cursor_x = x;
        cursor_y = y;
    }
    void print(const String& text) {
        if (g_display && !text.isEmpty()) {
            g_display->oledDrawText(text.c_str(), cursor_x, cursor_y, 8);
            cursor_x += text.length() * 6; // Approximate character width
        }
    }
    void drawStr(int16_t x, int16_t y, const char* str) {
        if (g_display && str) {
            g_display->oledDrawText(str, x, y, 8);
        }
    }
    void drawPixel(int16_t x, int16_t y) {
        if (g_display) g_display->oledSetPixel(x, y, true);
    }
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        if (g_display) g_display->oledDrawLine(x0, y0, x1, y1, true);
    }
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
        if (g_display) g_display->oledDrawRect(x, y, w, h, true, true);
    }
    void drawVLine(int16_t x, int16_t y, int16_t h) {
        if (g_display) g_display->oledDrawLine(x, y, x, y + h - 1, true);
    }
    void drawHLine(int16_t x, int16_t y, int16_t w) {
        if (g_display) g_display->oledDrawLine(x, y, x + w - 1, y, true);
    }
    void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const unsigned char* bitmap) {
        if (g_display && bitmap) {
            // Draw bitmap pixel by pixel
            int byteWidth = (w + 7) / 8;
            for (int dy = 0; dy < h; dy++) {
                for (int dx = 0; dx < w; dx++) {
                    int byteIndex = dy * byteWidth + dx / 8;
                    int bitIndex = dx % 8;
                    bool pixelOn = (bitmap[byteIndex] >> bitIndex) & 1;
                    if (pixelOn) {
                        g_display->oledSetPixel(x + dx, y + dy, true);
                    }
                }
            }
        }
    }
    uint16_t getStrWidth(const char* str) { return str ? static_cast<uint16_t>(strlen(str) * 6) : 0; }
    uint16_t getDisplayWidth() const { return 128; }
    uint16_t getDisplayHeight() const { return 32; }
    void setContrast(uint8_t value) {
        // OLED contrast control - could be implemented if needed
    }
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
