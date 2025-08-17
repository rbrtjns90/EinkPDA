#ifndef ADAFRUIT_TCA8418_H
#define ADAFRUIT_TCA8418_H

#include "pocketmage_compat.h"

class Adafruit_TCA8418 {
public:
    bool begin(uint8_t addr = 0x34) { return true; }
    bool begin(uint8_t addr, void* wire) { return true; }
    void pinMode(uint8_t pin, uint8_t mode) {}
    void digitalWrite(uint8_t pin, uint8_t value) {}
    uint8_t digitalRead(uint8_t pin) { return 0; }
    bool available() { return false; }
    uint16_t getEvent() {
        if (g_display) {
            char key = g_display->getLastKey();
            if (key != 0) {
                std::cout << "[TCA8418] getEvent() returning key: '" << key << "' (0x" << std::hex << (int)key << std::dec << ")" << std::endl;
                // Set the TCA8418_event flag so updateKeypress() knows there's input
                extern volatile bool TCA8418_event;
                TCA8418_event = true;
                
                // Convert ASCII key to TCA8418 keypad matrix position
                // PocketMage expects: 0x80 | (row*10 + col + 1)
                // For now, map common keys to reasonable positions
                int keycode = 1; // default position
                if (key >= 'a' && key <= 'z') {
                    keycode = (key - 'a') + 1; // map a-z to positions 1-26
                } else if (key >= '0' && key <= '9') {
                    keycode = (key - '0') + 27; // map 0-9 to positions 27-36
                } else if (key == ' ') {
                    keycode = 37; // space key
                } else if (key == '\n' || key == '\r') {
                    keycode = 38; // enter key
                }
                
                return 0x80 | keycode; // Key pressed format
            }
        }
        return 0;
    }
    void flush() {}
    void enableInterrupt() {}
    void disableInterrupt() {}
    void matrix(uint8_t rows, uint8_t cols) {}
    void enableInterrupts() {}
    void disableInterrupts() {}
    void writeRegister(uint8_t reg, uint8_t value) {}
    uint8_t readRegister(uint8_t reg) { return 0; }
};

#endif
