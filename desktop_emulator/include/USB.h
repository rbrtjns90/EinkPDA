#ifndef USB_H
#define USB_H

#include "pocketmage_compat.h"

class USBSerial {
public:
    void begin(unsigned long baud = 115200) {}
    void end() {}
    void print(const String& text) {
        std::cout << text.c_str();
    }
    void println(const String& text) {
        std::cout << text.c_str() << std::endl;
    }
    void flush() {}
    bool available() { return false; }
    int read() { return -1; }
    String readString() { return String(); }
};

class USBClass {
public:
    void begin() {}
    void end() {}
    bool connected() { return false; }
    void setManufacturer(const String& name) {}
    void setProduct(const String& name) {}
    void setSerial(const String& serial) {}
};

extern USBClass USB;
extern USBSerial Serial1;

#endif
