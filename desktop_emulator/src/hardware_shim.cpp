#include "pocketmage_compat.h"
#include "desktop_display.h"
#include "SD_MMC.h"
#include "Wire.h"
#include "Preferences.h"
#include "Adafruit_TCA8418.h"
#include "GxEPD2_BW.h"
#include "U8g2lib.h"
#include "Buzzer.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

// Arduino compatibility implementations
SerialClass Serial;
SD_MMCClass SD_MMC;
RTC_DS3231 rtc;
TwoWire Wire;
Preferences preferences;
Adafruit_TCA8418 keypad;
Buzzer buzzer;

// Mock fonts
const GFXfont FreeSerif9pt7b = {};
const GFXfont FreeMonoBold9pt7b = {};

// Mock hardware objects that PocketMage expects
GxEPD2_BW display;
U8G2_SH1106_128X32_VISIONOX_F_HW_I2C u8g2;

unsigned long millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

unsigned long micros() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void delayMicroseconds(unsigned int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void randomSeed(unsigned long seed) {
    srand(seed);
}

uint32_t esp_random() {
    return rand();
}

// PocketMage compatibility functions
String removeChar(String str, char c) {
    std::string result = str.c_str();
    result.erase(std::remove(result.begin(), result.end(), c), result.end());
    return String(result);
}

void listDir(SD_MMCClass& fs, const char* dirname) {
    // Mock file listing - populate with some test files
    std::cout << "[SD] Listing directory: " << dirname << std::endl;
}

void oledWord(const String& text) {
    if (g_display) {
        g_display->oledClear();
        g_display->oledDrawText(text.c_str(), 0, 16);
        g_display->oledUpdate();
    }
}

void oledLine(const String& text, bool clear) {
    if (g_display) {
        if (clear) g_display->oledClear();
        g_display->oledDrawText(text.c_str(), 0, 16);
        g_display->oledUpdate();
    }
}

void refresh() {
    if (g_display) g_display->einkRefresh();
}

void drawStatusBar(const String& text) {
    if (g_display) {
        g_display->oledClear();
        g_display->oledDrawText(text.c_str(), 0, 16);
        g_display->oledUpdate();
    }
}

void drawThickLine(int x0, int y0, int x1, int y1, int thickness) {
    if (g_display) {
        for (int i = 0; i < thickness; i++) {
            g_display->einkDrawLine(x0 + i, y0, x1 + i, y1, true);
            g_display->einkDrawLine(x0, y0 + i, x1, y1 + i, true);
        }
    }
}

// PocketMage app initialization functions (mock implementations)
void TXT_INIT() {
    std::cout << "[TXT] App initialized" << std::endl;
}

void FILEWIZ_INIT() {
    std::cout << "[FILEWIZ] App initialized" << std::endl;
}

void USB_INIT() {
    std::cout << "[USB] App initialized" << std::endl;
}

void TASKS_INIT() {
    std::cout << "[TASKS] App initialized" << std::endl;
}

void SETTINGS_INIT() {
    std::cout << "[SETTINGS] App initialized" << std::endl;
}

void CALENDAR_INIT() {
    std::cout << "[CALENDAR] App initialized" << std::endl;
}

void JOURNAL_INIT() {
    std::cout << "[JOURNAL] App initialized" << std::endl;
}

void LEXICON_INIT() {
    std::cout << "[LEXICON] App initialized" << std::endl;
}

long random(long max) {
    return rand() % max;
}

long random(long min, long max) {
    return min + (rand() % (max - min));
}

// MockSerial implementation
void MockSerial::begin(unsigned long baud) {
    std::cout << "[Serial] Begin at " << baud << " baud" << std::endl;
}

void MockSerial::print(const std::string& str) {
    std::cout << str;
}

void MockSerial::println(const std::string& str) {
    std::cout << str << std::endl;
}

void MockSerial::flush() {
    std::cout.flush();
}

// String implementation removed - using std::string directly

// File implementation
File::File() : isOpen(false) {}

File::File(const std::string& path, const std::string& mode) : isOpen(false), filePath(path) {
    std::string fullPath = "./data/" + path;
    
    if (mode.find('w') != std::string::npos || mode.find('a') != std::string::npos) {
        outFile.open(fullPath, mode.find('a') != std::string::npos ? std::ios::app : std::ios::out);
        isOpen = outFile.is_open();
    } else {
        inFile.open(fullPath);
        isOpen = inFile.is_open();
    }
    
    if (!isOpen) {
        std::cout << "[File] Failed to open: " << fullPath << std::endl;
    }
}

File::~File() {
    close();
}

void File::close() {
    if (inFile.is_open()) inFile.close();
    if (outFile.is_open()) outFile.close();
    isOpen = false;
}

size_t File::write(const uint8_t* data, size_t len) {
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char*>(data), len);
        return len;
    }
    return 0;
}

size_t File::write(const String& str) {
    return write(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int File::read() {
    if (inFile.is_open()) {
        return inFile.get();
    }
    return -1;
}

String File::readString() {
    if (inFile.is_open()) {
        std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        return String(content);
    }
    return String();
}

String File::readStringUntil(char terminator) {
    std::string result;
    if (inFile.is_open()) {
        std::getline(inFile, result, terminator);
    }
    return String(result);
}

bool File::available() {
    return inFile.is_open() && inFile.good() && inFile.peek() != EOF;
}

void File::seek(size_t pos) {
    if (inFile.is_open()) inFile.seekg(pos);
    if (outFile.is_open()) outFile.seekp(pos);
}

size_t File::size() {
    if (!isOpen) return 0;
    auto current = file.tellg();
    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(current);
    return static_cast<size_t>(size);
}

// MockSDCard implementation
bool MockSDCard::begin(const char* mountpoint, bool mode1bit) {
    rootPath = "./data";
    std::filesystem::create_directories(rootPath);
    std::cout << "[SD] Mounted at " << rootPath << std::endl;
    return true;
}

bool MockSDCard::exists(const char* path) {
    std::string fullPath = rootPath + std::string(path);
    return std::filesystem::exists(fullPath);
}

bool MockSDCard::mkdir(const char* path) {
    std::string fullPath = rootPath + std::string(path);
    return std::filesystem::create_directories(fullPath);
}

bool MockSDCard::remove(const char* path) {
    std::string fullPath = rootPath + std::string(path);
    return std::filesystem::remove(fullPath);
}

File MockSDCard::open(const char* path, const char* mode) {
    std::string fullPath = rootPath + std::string(path);
    return File(fullPath, mode);
}

uint8_t MockSDCard::cardType() {
    return 1; // Not CARD_NONE
}

// Mock display implementations
void MockGxEPD2::init(unsigned long baud) {
    std::cout << "[E-Ink] Initialized" << std::endl;
}

void MockGxEPD2::setRotation(uint8_t rotation) {}
void MockGxEPD2::setTextColor(uint16_t color) {}
void MockGxEPD2::setFullWindow() {}

void MockGxEPD2::clearScreen() {
    if (g_display) g_display->einkClear();
}

void MockGxEPD2::display() {
    if (g_display) g_display->einkRefresh();
}

void MockGxEPD2::displayPartial() {
    if (g_display) g_display->einkPartialRefresh();
}

void MockGxEPD2::setFont(const void* font) {}

void MockGxEPD2::setCursor(int16_t x, int16_t y) {}

void MockGxEPD2::print(const String& text) {
    if (g_display) g_display->einkDrawText(text.c_str(), 0, 0);
}

void MockGxEPD2::println(const String& text) {
    if (g_display) g_display->einkDrawText(text.c_str(), 0, 0);
}

void MockGxEPD2::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (g_display) g_display->einkSetPixel(x, y, color == GxEPD_BLACK);
}

void MockGxEPD2::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (g_display) g_display->einkDrawLine(x0, y0, x1, y1, color == GxEPD_BLACK);
}

void MockGxEPD2::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (g_display) g_display->einkDrawRect(x, y, w, h, false, color == GxEPD_BLACK);
}

void MockGxEPD2::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (g_display) g_display->einkDrawRect(x, y, w, h, true, color == GxEPD_BLACK);
}

// MockU8G2 implementation
void MockU8G2::begin() {
    std::cout << "[OLED] Initialized" << std::endl;
}

void MockU8G2::setBusClock(uint32_t clock) {}
void MockU8G2::setPowerSave(uint8_t is_enable) {}

void MockU8G2::clearBuffer() {
    if (g_display) g_display->oledClear();
}

void MockU8G2::sendBuffer() {
    if (g_display) g_display->oledUpdate();
}

void MockU8G2::setFont(const uint8_t* font) {}
void MockU8G2::setCursor(int16_t x, int16_t y) {}

void MockU8G2::print(const String& text) {
    if (g_display) g_display->oledDrawText(text.c_str(), 0, 0);
}

void MockU8G2::drawStr(int16_t x, int16_t y, const char* str) {
    if (g_display) g_display->oledDrawText(str, x, y);
}

void MockU8G2::drawPixel(int16_t x, int16_t y) {
    if (g_display) g_display->oledSetPixel(x, y, true);
}

void MockU8G2::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    if (g_display) g_display->oledDrawLine(x0, y0, x1, y1, true);
}

void MockU8G2::drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (g_display) g_display->oledDrawRect(x, y, w, h, true, true);
}

// Mock hardware implementations
bool MockKeypad::begin(uint8_t addr, void* wire) { return true; }
void MockKeypad::matrix(uint8_t rows, uint8_t cols) {}
void MockKeypad::flush() {}
void MockKeypad::enableInterrupts() {}
bool MockKeypad::available() { return false; }
uint8_t MockKeypad::getKey() { return 0; }

void MockBuzzer::begin(uint8_t pin) {}
void MockBuzzer::sound(int frequency, unsigned long duration) {
    std::cout << "[Buzzer] " << frequency << "Hz for " << duration << "ms" << std::endl;
}

bool MockMPR121::begin(uint8_t addr) { return true; }
uint16_t MockMPR121::touched() { return 0; }
uint16_t MockMPR121::filteredData(uint8_t t) { return 0; }

bool MockRTC::begin() { return true; }
void MockRTC::adjust(uint32_t timestamp) {}
void MockRTC::start() {}
bool MockRTC::lostPower() { return false; }
uint32_t MockRTC::now() { return millis() / 1000; }

bool MockPreferences::begin(const char* name, bool readOnly) { return true; }
void MockPreferences::end() {}
void MockPreferences::clear() {}
bool MockPreferences::remove(const char* key) { return true; }
size_t MockPreferences::putInt(const char* key, int32_t value) { return 4; }
size_t MockPreferences::putBool(const char* key, bool value) { return 1; }
size_t MockPreferences::putString(const char* key, const String& value) { return value.length(); }
int32_t MockPreferences::getInt(const char* key, int32_t defaultValue) { return defaultValue; }
bool MockPreferences::getBool(const char* key, bool defaultValue) { return defaultValue; }
String MockPreferences::getString(const char* key, const String& defaultValue) { return defaultValue; }

// Mock GPIO and system functions
void pinMode(uint8_t pin, uint8_t mode) {}
int digitalRead(uint8_t pin) { return 0; }
void digitalWrite(uint8_t pin, uint8_t value) {}
int analogRead(uint8_t pin) { return 512; }
void attachInterrupt(uint8_t pin, void (*callback)(), int mode) {}
void xTaskCreatePinnedToCore(void (*task)(void*), const char* name, 
    uint32_t stackSize, void* params, uint8_t priority, 
    TaskHandle_t* handle, uint8_t core) {}
void vTaskDelay(uint32_t ticks) { delay(ticks); }
// yield() already defined in esp32_shims.h
void esp_sleep_enable_ext0_wakeup(uint8_t pin, int level) {}
void esp_deep_sleep_start() { exit(0); }
void setCpuFrequencyMhz(uint32_t freq) {
    std::cout << "[CPU] Set frequency to " << freq << "MHz" << std::endl;
}
