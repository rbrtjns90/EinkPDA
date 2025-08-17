#ifndef HARDWARE_SHIM_H
#define HARDWARE_SHIM_H

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <filesystem>

// Arduino-like types and functions
typedef uint8_t byte;
typedef bool boolean;

// Mock Arduino functions
void delay(unsigned long ms);
unsigned long millis();
void randomSeed(unsigned long seed);
long random(long max);
long random(long min, long max);

// Mock Serial class
class MockSerial {
public:
    void begin(unsigned long baud);
    void print(const std::string& str);
    void println(const std::string& str);
    void flush();
};
extern MockSerial Serial;

// Mock String class (simplified)
class String {
public:
    std::string data;
    
    String() = default;
    String(const char* str) : data(str) {}
    String(const std::string& str) : data(str) {}
    String(int num) : data(std::to_string(num)) {}
    String(float num) : data(std::to_string(num)) {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    String substring(size_t start, size_t end = std::string::npos) const;
    int indexOf(const String& str, int start = 0) const;
    String replace(const String& from, const String& to) const;
    String toLowerCase() const;
    String toUpperCase() const;
    
    String operator+(const String& other) const;
    String& operator+=(const String& other);
    bool operator==(const String& other) const;
    bool operator!=(const String& other) const;
    char operator[](size_t index) const;
};

// Mock File system
class File {
public:
    std::fstream file;
    bool isOpen;
    
    File();
    File(const std::string& path, const std::string& mode);
    ~File();
    
    operator bool() const { return isOpen; }
    void close();
    size_t write(const uint8_t* data, size_t len);
    size_t write(const std::string& str);
    int read();
    String readString();
    String readStringUntil(char terminator);
    bool available();
    void seek(size_t pos);
    size_t position();
    size_t size();
};

class MockSDCard {
public:
    std::string rootPath;
    
    bool begin(const char* mountpoint = "/sdcard", bool mode1bit = false);
    bool exists(const char* path);
    bool mkdir(const char* path);
    bool remove(const char* path);
    File open(const char* path, const char* mode = "r");
    uint8_t cardType();
};
extern MockSDCard SD_MMC;

// Constants
#define CARD_NONE 0
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

// Mock display classes
class MockGxEPD2 {
public:
    void init(unsigned long baud = 115200);
    void setRotation(uint8_t rotation);
    void setTextColor(uint16_t color);
    void setFullWindow();
    void clearScreen();
    void display();
    void displayPartial();
    void setFont(const void* font);
    void setCursor(int16_t x, int16_t y);
    void print(const String& text);
    void println(const String& text);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    
    int16_t width() { return 310; }
    int16_t height() { return 128; }
};

class MockU8G2 {
public:
    void begin();
    void setBusClock(uint32_t clock);
    void setPowerSave(uint8_t is_enable);
    void clearBuffer();
    void sendBuffer();
    void setFont(const uint8_t* font);
    void setCursor(int16_t x, int16_t y);
    void print(const String& text);
    void drawStr(int16_t x, int16_t y, const char* str);
    void drawPixel(int16_t x, int16_t y);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h);
    
    int16_t getDisplayWidth() { return 256; }
    int16_t getDisplayHeight() { return 32; }
};

// Mock hardware classes
class MockKeypad {
public:
    bool begin(uint8_t addr, void* wire);
    void matrix(uint8_t rows, uint8_t cols);
    void flush();
    void enableInterrupts();
    bool available();
    uint8_t getKey();
};

class MockBuzzer {
public:
    void begin(uint8_t pin);
    void sound(int frequency, unsigned long duration);
};

class MockMPR121 {
public:
    bool begin(uint8_t addr);
    uint16_t touched();
    uint16_t filteredData(uint8_t t);
};

class MockRTC {
public:
    bool begin();
    void adjust(uint32_t timestamp);
    void start();
    bool lostPower();
    uint32_t now();
};

class MockPreferences {
public:
    bool begin(const char* name, bool readOnly = false);
    void end();
    void clear();
    bool remove(const char* key);
    
    size_t putInt(const char* key, int32_t value);
    size_t putBool(const char* key, bool value);
    size_t putString(const char* key, const String& value);
    
    int32_t getInt(const char* key, int32_t defaultValue = 0);
    bool getBool(const char* key, bool defaultValue = false);
    String getString(const char* key, const String& defaultValue = "");
};

// Color constants
#define GxEPD_BLACK 0x0000
#define GxEPD_WHITE 0xFFFF

// Pin definitions (unused in desktop version)
#define I2C_SDA 0
#define I2C_SCL 0
#define SPI_SCK 0
#define SPI_MOSI 0
#define KB_IRQ 0
#define PWR_BTN 0
#define CHRG_SENS 0
#define BAT_SENS 0
#define RTC_INT 0
#define SD_CLK 0
#define SD_CMD 0
#define SD_D0 0
#define TCA8418_DEFAULT_ADDR 0x34
#define MPR121_ADDR 0x5A

// Mock GPIO functions
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2
void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
int analogRead(uint8_t pin);

// Mock interrupt functions
void attachInterrupt(uint8_t pin, void (*callback)(), int mode);
#define CHANGE 0
#define FALLING 1
#define RISING 2

// Mock task functions
typedef void* TaskHandle_t;
void xTaskCreatePinnedToCore(void (*task)(void*), const char* name, 
    uint32_t stackSize, void* params, uint8_t priority, 
    TaskHandle_t* handle, uint8_t core);
void vTaskDelay(uint32_t ticks);
void yield();
#define portTICK_PERIOD_MS 1

// Mock sleep functions
void esp_sleep_enable_ext0_wakeup(uint8_t pin, int level);
void esp_deep_sleep_start();

// Mock CPU functions
void setCpuFrequencyMhz(uint32_t freq);

#endif // HARDWARE_SHIM_H
