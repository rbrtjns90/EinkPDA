#ifndef ADAFRUIT_TCA8418_H
#define ADAFRUIT_TCA8418_H

#include "pocketmage_compat.h"

class Adafruit_TCA8418 {
public:
    bool begin(uint8_t addr = 0x34) { return true; }
    void pinMode(uint8_t pin, uint8_t mode) {}
    void digitalWrite(uint8_t pin, uint8_t value) {}
    uint8_t digitalRead(uint8_t pin) { return 0; }
    void enableInterrupt() {}
    void disableInterrupt() {}
    void enableInterrupts() {}
    void disableInterrupts() {}
    uint8_t getEvent() { return 0; }
    bool available() { return false; }
    void writeRegister(uint8_t reg, uint8_t value) {}
    uint8_t readRegister(uint8_t reg) { return 0; }
    void flush() {}
};

#endif
