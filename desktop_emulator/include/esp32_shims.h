#ifndef ESP32_SHIMS_H
#define ESP32_SHIMS_H

// ESP32 library shims for desktop compilation

// GxEPD2 display library shim
#define GxEPD_WHITE 0xFF
#define GxEPD_BLACK 0x00

// U8g2 library shim  
#define U8G2_R0 0

// Wire library shim
class WireClass {
public:
    void begin() {}
    void begin(int sda, int scl) {}
};
extern WireClass Wire;

// Preferences library shim
class PreferencesClass {
public:
    bool begin(const char* name, bool readOnly = false) { return true; }
    void end() {}
    bool putString(const char* key, const char* value) { return true; }
    String getString(const char* key, const char* defaultValue = "") { return String(defaultValue); }
    bool putInt(const char* key, int value) { return true; }
    int getInt(const char* key, int defaultValue = 0) { return defaultValue; }
    bool putBool(const char* key, bool value) { return true; }
    bool getBool(const char* key, bool defaultValue = false) { return defaultValue; }
};
extern PreferencesClass preferences;

// SD_MMC library shim
class SD_MMCClass {
public:
    bool begin() { return true; }
    void end() {}
};
extern SD_MMCClass SD_MMC;

// USB library shims
class USBClass {
public:
    void begin() {}
    void end() {}
};
extern USBClass USB;

class USBMSCClass {
public:
    void begin() {}
    void end() {}
};
extern USBMSCClass MSC;

// RTC library shim
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

class RTC_DS3231 {
public:
    bool begin() { return true; }
    DateTime now() { return DateTime(); }
    void adjust(const DateTime& dt) {}
};

// Buzzer library shim
class BuzzerClass {
public:
    void begin(int pin) {}
    void tone(int frequency, int duration = 0) {}
    void noTone() {}
};

// TCA8418 keypad shim
class Adafruit_TCA8418 {
public:
    bool begin() { return true; }
    int available() { return 0; }
    int getEvent() { return 0; }
};

// MPR121 touch sensor shim
class Adafruit_MPR121 {
public:
    bool begin() { return true; }
    uint16_t touched() { return 0; }
};

// FreeRTOS shims
#define portTICK_PERIOD_MS 1
void vTaskDelay(int ticks) { delay(ticks); }
void yield() {}

// ESP CPU shims
void esp_cpu_set_cycle_count(uint32_t count) {}
uint32_t esp_cpu_get_cycle_count() { return 0; }

#endif // ESP32_SHIMS_H
