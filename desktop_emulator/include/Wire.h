#ifndef WIRE_H
#define WIRE_H

// Mock Wire library for desktop compilation
#include "pocketmage_compat.h"

class TwoWire {
public:
    void begin() {}
    void begin(int sda, int scl) {}
    void setClock(uint32_t clock) {}
    void beginTransmission(uint8_t address) {}
    uint8_t endTransmission() { return 0; }
    uint8_t requestFrom(uint8_t address, uint8_t quantity) { return 0; }
    int available() { return 0; }
    int read() { return 0; }
    size_t write(uint8_t data) { return 1; }
};

extern TwoWire Wire;

#endif // WIRE_H
