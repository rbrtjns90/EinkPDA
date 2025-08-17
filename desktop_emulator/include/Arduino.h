#ifndef Arduino_h
#define Arduino_h

// Desktop emulator Arduino compatibility layer
#include <stdint.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

// Arduino types
typedef std::string String;
typedef bool boolean;
typedef uint8_t byte;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define PROGMEM
#define F(x) x

// Serial mock
class SerialClass {
public:
    void begin(int baud) { std::cout << "[Serial] Begin at " << baud << " baud" << std::endl; }
    void println(const char* str) { std::cout << "[Serial] " << str << std::endl; }
    void println(const std::string& str) { std::cout << "[Serial] " << str << std::endl; }
    void print(const char* str) { std::cout << "[Serial] " << str; }
    void print(const std::string& str) { std::cout << "[Serial] " << str; }
};
extern SerialClass Serial;

// Arduino functions
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// Mock ESP32 specific includes that would normally be in Arduino.h
#define ESP32
#define CONFIG_IDF_TARGET_ESP32S3

#endif // Arduino_h
