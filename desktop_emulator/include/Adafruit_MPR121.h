#ifndef ADAFRUIT_MPR121_H
#define ADAFRUIT_MPR121_H

#include "pocketmage_compat.h"

class Adafruit_MPR121 {
public:
    bool begin(uint8_t i2caddr = 0x5A) { return true; }
    uint16_t touched() { return 0; }
    uint16_t filteredData(uint8_t t) { return 512; }
    uint16_t baselineData(uint8_t t) { return 512; }
    void setThresholds(uint8_t touch, uint8_t release) {}
    void writeRegister(uint8_t reg, uint8_t value) {}
    uint8_t readRegister8(uint8_t reg) { return 0; }
    uint16_t readRegister16(uint8_t reg) { return 0; }
};

#endif
