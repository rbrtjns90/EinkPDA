#ifndef U8G2LIB_H
#define U8G2LIB_H

// Mock U8g2 library for desktop compilation
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "desktop_display.h"
#include "oled_service.h"
#include "pocketmage_compat.h"

extern DesktopDisplay* g_display;

class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C {
private:
    int16_t cursor_x = 0, cursor_y = 0;
    static std::string textLines[3]; // Static text accumulation buffer
    
public:
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C() {}
    U8G2_SH1106_128X32_VISIONOX_F_HW_I2C(uint8_t rotation, int cs, int dc, int rst) {
        (void)rotation; (void)cs; (void)dc; (void)rst;
    }
    void begin() {}
    void setBusClock(uint32_t clock) {}
    void setPowerSave(uint8_t is_enable) {}
    void clearBuffer() {
        textLines[0].clear();
        textLines[1].clear();
        textLines[2].clear();
    }
    void sendBuffer() {
        // Send accumulated text to OLED service
        DEBUG_LOG("OLED", "Rendering: '" + textLines[0] + "' / '" + textLines[1] + "' / '" + textLines[2] + "'");
        oled_set_lines(textLines[0].c_str(), textLines[1].c_str(), textLines[2].c_str());
    }
    void setFont(const uint8_t* font) {}
    void setCursor(int16_t x, int16_t y) {
        cursor_x = x;
        cursor_y = y;
    }
    void print(const std::string& text) {
        int line = cursor_y / 11; // Approximate line height
        if (line >= 0 && line < 3) {
            textLines[line] += text;
        }
        cursor_x += text.length() * 6; // Approximate character width
    }
    void print(const char* text) {
        print(std::string(text));
    }
    void print(int value) {
        print(std::to_string(value));
    }
    void print(unsigned int value) {
        print(std::to_string(value));
    }
    void print(long value) {
        print(std::to_string(value));
    }
    void print(unsigned long value) {
        print(std::to_string(value));
    }
    void drawStr(int16_t x, int16_t y, const char* text) {
        int line = y / 11; // Approximate line height
        if (line >= 0 && line < 3) {
            textLines[line] = text; // Replace line content
        }
    }
    void drawPixel(int16_t x, int16_t y) {
        // No-op in emulator - handled by OLED service
    }
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        // No-op in emulator - handled by OLED service
    }
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
        // No-op in emulator - handled by OLED service
    }
    void drawVLine(int16_t x, int16_t y, int16_t h) {
        // No-op in emulator - handled by OLED service
    }
    void drawHLine(int16_t x, int16_t y, int16_t w) {
        // No-op in emulator - handled by OLED service
    }
    void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const unsigned char* bitmap) {
        // No-op in emulator - handled by OLED service
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
