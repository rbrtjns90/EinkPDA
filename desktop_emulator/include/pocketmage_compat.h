#ifndef POCKETMAGE_COMPAT_H
#define POCKETMAGE_COMPAT_H

// Comprehensive PocketMage compatibility header for desktop compilation
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include "desktop_display.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <thread>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

// Forward declarations
class String;
class DateTime;
class TimeSpan;
class File;
class TwoWire;
class Preferences;
class Adafruit_TCA8418;
template<typename T, int H> class GxEPD2_BW;
class U8G2_SH1106_128X32_VISIONOX_F_HW_I2C;
class SD_MMCClass;
class DesktopDisplay;

// Global display instance
extern DesktopDisplay* g_display;

void listDir(SD_MMCClass& fs, const char* dirname);

// Arduino basic types
typedef uint8_t byte;
typedef bool boolean;

// Arduino String class with additional methods
class String {
private:
    std::string data;
public:
    String() = default;
    String(const char* str) : data(str ? str : "") {}
    String(const std::string& str) : data(str) {}
    String(int value) : data(std::to_string(value)) {}
    String(unsigned int value) : data(std::to_string(value)) {}
    String(long value) : data(std::to_string(value)) {}
    String(unsigned long value) : data(std::to_string(value)) {}
    String(float value) : data(std::to_string(value)) {}
    String(double value) : data(std::to_string(value)) {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const String& other) const { return data != other.data; }
    String operator+(const String& other) const { return String(data + other.data); }
    String& operator+=(const String& other) { data += other.data; return *this; }
    String& operator+=(const char* str) { if (str) data += str; return *this; }
    String& operator+=(char c) { data += c; return *this; }
    char operator[](size_t index) const { return index < data.size() ? data[index] : '\0'; }
    void remove(size_t index) { if (index < data.size()) data.erase(index, 1); }
    String substring(size_t start, size_t end = std::string::npos) const {
        if (end == std::string::npos) return String(data.substr(start));
        return String(data.substr(start, end - start));
    }
    
    // Additional methods needed by PocketMage
    bool endsWith(const String& suffix) const {
        if (suffix.length() > data.length()) return false;
        return data.compare(data.length() - suffix.length(), suffix.length(), suffix.data) == 0;
    }
    
    int lastIndexOf(char c) const {
        size_t pos = data.find_last_of(c);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    
    int lastIndexOf(const String& str) const {
        size_t pos = data.find_last_of(str.data);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    
    bool startsWith(const String& prefix) const {
        if (prefix.length() > data.length()) return false;
        return data.compare(0, prefix.length(), prefix.data) == 0;
    }
    
    bool isEmpty() const {
        return data.empty();
    }
    
    int toInt() const {
        try {
            return std::stoi(data);
        } catch (...) {
            return 0;
        }
    }
    
    void toLowerCase() {
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    }
    
    void toUpperCase() {
        std::transform(data.begin(), data.end(), data.begin(), ::toupper);
    }
    
    int indexOf(char c) const {
        size_t pos = data.find(c);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }
    
    int indexOf(char c, size_t start) const {
        size_t pos = data.find(c, start);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }
    
    int indexOf(const String& str) const {
        size_t pos = data.find(str.data);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }
    
    bool operator<(const String& other) const {
        return data < other.data;
    }
    
    bool operator>(const String& other) const {
        return data > other.data;
    }
    
    bool operator<=(const String& other) const {
        return data <= other.data;
    }
    
    bool operator>=(const String& other) const {
        return data >= other.data;
    }
    
    char charAt(size_t index) const {
        return index < data.size() ? data[index] : '\0';
    }
    
    bool equals(const String& other) const {
        return data == other.data;
    }
    
    void trim() {
        // Remove leading whitespace
        data.erase(data.begin(), std::find_if(data.begin(), data.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // Remove trailing whitespace
        data.erase(std::find_if(data.rbegin(), data.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), data.end());
    }
};

// Allow streaming our Arduino-like String to iostreams
inline std::ostream& operator<<(std::ostream& os, const String& s) {
    os << s.c_str();
    return os;
}

// (boolean/byte already defined above)

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define PROGMEM
#define F(x) x

// File mode constants
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

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
    void println() { std::cout << "[Serial] " << std::endl; }
    void print(const String& str) { std::cout << "[Serial] " << str.c_str(); }
    void print(int val) { std::cout << "[Serial] " << val; }
    void print(float val, int precision = 2) { std::cout << "[Serial] " << std::fixed << std::setprecision(precision) << val; }
    void printf(const char* format, ...) { std::cout << "[Serial] printf" << std::endl; }
    void write(int val) { std::cout << "[Serial] " << (char)val; }
};
extern SerialClass Serial;

// Forward declare TimeSpan
class TimeSpan;

// DateTime mock for RTC
class DateTime {
public:
    DateTime() {}
    DateTime(int year, int month, int day) {}
    DateTime(int year, int month, int day, int hour, int minute, int second) {}
    int year() const { return 2025; }
    int month() const { return 8; }
    int day() const { return 17; }
    int hour() const { return 8; }
    int minute() const { return 14; }
    int second() const { return 0; }
    int dayOfTheWeek() const { return 0; } // 0 = Sunday
    
    DateTime operator-(const TimeSpan& ts) const;
    DateTime operator+(const TimeSpan& ts) const;
    TimeSpan operator-(const DateTime& other) const;
    String toString() const { return String("2025-08-17 08:14:00"); }
};

// RTC mock
class RTC_DS3231 {
public:
    bool begin() { return true; }
    DateTime now() { return DateTime(); }
    void adjust(const DateTime& dt) {}
};
extern RTC_DS3231 rtc;

// Hardware objects will be provided by the actual mock headers
extern U8G2_SH1106_128X32_VISIONOX_F_HW_I2C u8g2;
extern Adafruit_TCA8418 keypad;

// Mock hardware functions
void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
int analogRead(uint8_t pin);

// PocketMage specific functions that need to be mocked
void listDir(SD_MMCClass& fs, const char* dirname);
void refresh();
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

// ESP32 CPU frequency control
void setCpuFrequencyMhz(uint32_t freq);

// std::min and std::max compatibility
template<typename T>
const T& min(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template<typename T>
const T& max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

// Forward declare File class
class File;

// Filesystem namespace
namespace fs {
    class FS {
    public:
        virtual File open(const char* path, const char* mode = "r") = 0;
        virtual File open(const String& path, const char* mode = "r") = 0;
        virtual bool rename(const char* path1, const char* path2) = 0;
        virtual bool remove(const char* path) = 0;
        virtual ~FS() = default;
    };
}

// ESP32 event types
typedef void* esp_event_base_t;
typedef struct {} sdmmc_card_t;

// Additional RTC types
typedef RTC_DS3231 RTC_PCF8563;

// Display types for globals.h compatibility
class GxEPD2_310_GDEQ031T10 {
public:
    static const int HEIGHT = 128;
    static volatile bool useFastFullUpdate;
    
    GxEPD2_310_GDEQ031T10(int cs, int dc, int rst, int busy) {}
    
    void init(uint32_t serial_diag_bitrate = 0) {
        std::cout << "[DISPLAY] E-Ink display initialized" << std::endl;
        if (g_display) {
            g_display->einkClear();
            // Draw a test pattern to verify display is working
            g_display->einkDrawText("PocketMage Emulator", 10, 10);
            g_display->einkDrawRect(5, 5, 300, 118, false);
        }
    }
    void setRotation(uint8_t r) {}
    void setTextColor(uint16_t color) {}
    void setFullWindow() {}
    void fillScreen(uint16_t color) {
        if (g_display) g_display->einkClear();
    }
    void display(bool partial = false) {
        if (g_display) g_display->einkRefresh();
    }
    void hibernate() {}
    void nextPage() {}
    
    int width() const { return 310; }
    int height() const { return 128; }
    
    void setCursor(int x, int y) { cursor_x = x; cursor_y = y; }
    void setFont(const void* font) {}
    void print(const char* text) {
        if (g_display && text) {
            std::cout << "[DISPLAY] Drawing text: '" << text << "' at (" << cursor_x << "," << cursor_y << ")" << std::endl;
            g_display->einkDrawText(text, cursor_x, cursor_y);
            cursor_x += strlen(text) * 8;
        }
    }
    void print(const String& text) { print(text.c_str()); }
    void drawRect(int x, int y, int w, int h, uint16_t color) {
        if (g_display) g_display->einkDrawRect(x, y, w, h);
    }
    void fillRect(int x, int y, int w, int h, uint16_t color) {
        if (g_display) g_display->einkDrawRect(x, y, w, h, true);
    }
    void drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, uint16_t color) {
        // Mock bitmap as rectangle for now
        if (g_display) g_display->einkDrawRect(x, y, w, h);
    }
    void drawPixel(int x, int y, uint16_t color) {
        // Mock pixel drawing
    }
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
        if (g_display) g_display->einkDrawLine(x0, y0, x1, y1);
    }
    void drawCircle(int x, int y, int r, uint16_t color) {
        // Mock circle drawing
    }
    void fillCircle(int x, int y, int r, uint16_t color) {
        // Mock filled circle
    }
    void getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        // Mock text bounds calculation
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = text ? strlen(text) * 8 : 0;
        if (h) *h = 16;
    }
    void getTextBounds(const String& text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        getTextBounds(text.c_str(), x, y, x1, y1, w, h);
    }
    void setPartialWindow(int x, int y, int w, int h) {}
    
private:
    int cursor_x = 0, cursor_y = 0;
};

// Template compatibility for GxEPD2_BW
template<typename T, int H>
class GxEPD2_BW : public GxEPD2_310_GDEQ031T10 {
public:
    GxEPD2_BW(const T& driver) : GxEPD2_310_GDEQ031T10(0, 0, 0, 0) {}
};

// Display instance expected by globals.h
extern GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display;

typedef U8G2_SH1106_128X32_VISIONOX_F_HW_I2C U8G2_SSD1326_ER_256X32_F_4W_HW_SPI;

// ESP32 pin constants (will be overridden by config.h if present)
#ifndef EPD_CS
#define EPD_CS 5
#endif
#ifndef EPD_DC
#define EPD_DC 17
#endif
#ifndef EPD_RST
#define EPD_RST 16
#endif
#ifndef EPD_BUSY
#define EPD_BUSY 4
#endif
#ifndef OLED_CS
#define OLED_CS 15
#endif
#ifndef OLED_DC
#define OLED_DC 2
#endif
#ifndef OLED_RST
#define OLED_RST 0
#endif
#define U8G2_R2 2

// Musical note constants
#define NOTE_A8 7040
#define NOTE_B8 7902
#define NOTE_C8 8372
#define NOTE_D8 9397

// Arduino constants
#define DEC 10

// Arduino utility functions (inline to avoid multiple definitions)
inline bool isDigit(char c) { return std::isdigit(c); }
inline uint32_t getCpuFrequencyMhz() { return 240; }
inline void esp_deep_sleep_start() { /* Mock deep sleep */ }

// GFX font type is defined in Adafruit_GFX.h

// TCA8418 register constants
#define TCA8418_REG_INT_STAT 0x02

// TimeSpan class for DateTime calculations
class TimeSpan {
public:
    TimeSpan(int days, int hours, int minutes, int seconds) {}
    int days() const { return 0; }
    int hours() const { return 0; }
    int minutes() const { return 0; }
    int seconds() const { return 0; }
};

// DateTime operator implementations
inline DateTime DateTime::operator-(const TimeSpan& ts) const {
    return DateTime();
}

inline DateTime DateTime::operator+(const TimeSpan& ts) const {
    return DateTime();
}

inline TimeSpan DateTime::operator-(const DateTime& other) const {
    return TimeSpan(0, 0, 0, 0);
}

// String concatenation operators for const char* + String
inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

// Function signature compatibility

#endif // POCKETMAGE_COMPAT_H
