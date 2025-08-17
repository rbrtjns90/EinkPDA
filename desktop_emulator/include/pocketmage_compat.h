#ifndef POCKETMAGE_COMPAT_H
#define POCKETMAGE_COMPAT_H

// Comprehensive PocketMage compatibility header for desktop compilation
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>

// Forward declarations for compatibility
class SD_MMCClass;
class TwoWire;
class Preferences;
class Adafruit_TCA8418;
class GxEPD2_BW;
class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C;

void listDir(SD_MMCClass& fs, const char* dirname);

// Arduino basic types
typedef uint8_t byte;
typedef bool boolean;

// Arduino String class compatibility
class String {
private:
    std::string data;
public:
    String() = default;
    String(const char* str) : data(str ? str : "") {}
    String(const std::string& str) : data(str) {}
    String(int value) : data(std::to_string(value)) {}
    String(float value) : data(std::to_string(value)) {}
    String(double value) : data(std::to_string(value)) {}
    
    size_t length() const { return data.length(); }
    const char* c_str() const { return data.c_str(); }
    
    String substring(size_t start, size_t end = std::string::npos) const {
        if (end == std::string::npos) return String(data.substr(start));
        return String(data.substr(start, end - start));
    }
    
    int indexOf(const String& str, int start = 0) const {
        size_t pos = data.find(str.data, start);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    
    bool startsWith(const String& str) const {
        return data.substr(0, str.length()) == str.data;
    }
    
    String toLowerCase() const {
        std::string result = data;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return String(result);
    }
    
    void remove(size_t index) { 
        if (index < data.size()) data.erase(index, 1); 
    }
    
    int toInt() const { return std::stoi(data); }
    
    String operator+(const String& other) const { return String(data + other.data); }
    String& operator+=(const String& other) { data += other.data; return *this; }
    String& operator+=(const char* str) { data += str; return *this; }
    String& operator+=(char c) { data += c; return *this; }
    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const String& other) const { return data != other.data; }
    char operator[](size_t index) const { return index < data.size() ? data[index] : '\0'; }
};

// Arduino types
typedef bool boolean;
typedef uint8_t byte;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define PROGMEM
#define F(x) x

// Display constants
#define GxEPD_WHITE 0xFF
#define GxEPD_BLACK 0x00
#define U8G2_R0 0

// Arduino functions
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
void randomSeed(unsigned long seed);
long random(long max);
long random(long min, long max);
uint32_t esp_random();

// Serial mock
class SerialClass {
public:
    void begin(int baud) {}
    void println(const String& str) { std::cout << "[Serial] " << str.c_str() << std::endl; }
    void print(const String& str) { std::cout << "[Serial] " << str.c_str(); }
};
extern SerialClass Serial;

// DateTime mock for RTC
class DateTime {
public:
    DateTime() {}
    DateTime(int year, int month, int day, int hour, int minute, int second) {}
    int year() const { return 2025; }
    int month() const { return 8; }
    int day() const { return 17; }
    int hour() const { return 8; }
    int minute() const { return 14; }
    int second() const { return 0; }
};

// RTC mock
class RTC_DS3231 {
public:
    bool begin() { return true; }
    DateTime now() { return DateTime(); }
    void adjust(const DateTime& dt) {}
};
extern RTC_DS3231 rtc;

// Forward declare display classes
class MockGxEPD2;
class MockU8G2;
class MockKeypad;

extern MockGxEPD2 display;
extern MockU8G2 u8g2;
extern MockKeypad keypad;

// Mock hardware functions
void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
int analogRead(uint8_t pin);

// PocketMage specific functions that need to be mocked
void listDir(SD_MMCClass& fs, const char* dirname);
void oledWord(const String& text);
void oledLine(const String& text, bool clear = true);
void refresh();
void drawStatusBar(const String& text);
void drawThickLine(int x0, int y0, int x1, int y1, int thickness);
String removeChar(String str, char c);

// PocketMage app initialization functions
void TXT_INIT();
void FILEWIZ_INIT();
void USB_INIT();
void TASKS_INIT();
void SETTINGS_INIT();
void CALENDAR_INIT();
void JOURNAL_INIT();
void LEXICON_INIT();

// Font declarations (mock)
struct GFXfont {};
extern const GFXfont FreeSerif9pt7b;
extern const GFXfont FreeMonoBold9pt7b;

#endif // POCKETMAGE_COMPAT_H
