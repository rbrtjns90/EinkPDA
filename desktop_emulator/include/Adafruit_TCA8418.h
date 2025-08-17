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
    uint16_t getEvent() { return 0; }
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
