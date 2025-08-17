#include "pocketmage_compat.h"
#include "desktop_display.h"
#include "SD_MMC.h"
#include "Wire.h"
#include "Preferences.h"
#include "Adafruit_TCA8418.h"
#include "Adafruit_GFX.h"

// Access to global display instance
extern DesktopDisplay* g_display;

// Include AppState enum definition
enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON };
#include "GxEPD2_BW.h"
#include "U8g2lib.h"
#include "Buzzer.h"
#include "USB.h"

// Arduino compatibility implementations - duplicates removed
SerialClass Serial;
// rtc, u8g2, keypad are defined in real PocketMage globals.cpp
SD_MMCClass SD_MMC;
TwoWire Wire;
SPIClass SPI;
// cap and prefs are defined in real PocketMage globals.cpp

// NOTE:
// The real PocketMage globals (display, u8g2, rtc, keypad, buzzer, prefs, etc.)
// are defined in Code/PocketMage_V3/src/globals.cpp and declared in
// Code/PocketMage_V3/include/globals.h. We must NOT define them here to avoid
// duplicate symbol linker errors.

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

// PocketMage compatibility functions - removeChar is defined in real PocketMage source

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

void drawThickLine(int x0, int y0, int x1, int y1, int thickness) {
    if (g_display) {
        for (int i = 0; i < thickness; i++) {
            g_display->einkDrawLine(x0 + i, y0, x1 + i, y1, true);
        }
    }
}

// ESP32 CPU frequency control mock
void setCpuFrequencyMhz(uint32_t freq) {
    // Mock CPU frequency control
}

// ESP32 Task and FreeRTOS functions
void xTaskCreatePinnedToCore(TaskFunction_t taskFunction, const char* taskName, uint32_t stackSize, void* parameters, uint32_t priority, TaskHandle_t* taskHandle, uint32_t coreId) {
    // Mock task creation - in emulator we'll call the task function directly when needed
    std::cout << "[Task] Created task: " << taskName << std::endl;
}

void vTaskDelay(uint32_t ticks) {
    delay(ticks); // Convert to regular delay
}

void yield() {
    // Mock yield - do nothing in emulator
}

// ESP32 Sleep functions
void esp_sleep_enable_ext0_wakeup(gpio_num_t gpio_num, int level) {
    std::cout << "[Sleep] Enabled ext0 wakeup on GPIO " << gpio_num << std::endl;
}

esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() {
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

// esp_deep_sleep_start already defined in pocketmage_compat.h

// ESP32 Interrupt functions
void attachInterrupt(uint8_t pin, void (*ISR)(void), int mode) {
    std::cout << "[Interrupt] Attached interrupt to pin " << (int)pin << std::endl;
}

void detachInterrupt(uint8_t pin) {
    std::cout << "[Interrupt] Detached interrupt from pin " << (int)pin << std::endl;
}

int digitalPinToInterrupt(uint8_t pin) {
    return pin; // Simple mapping for emulator
}

// Function signature compatibility removed - using real app implementation

// Only USB_INIT is missing from real PocketMage source
void USB_INIT() {
    std::cout << "[USB] App initialized" << std::endl;
}

// PocketMage specific function implementations
void playJingle(const char* name) {
    std::cout << "[Audio] Playing jingle: " << name << std::endl;
}

void oledWord(const char* text, bool clear, bool send) {
    if (clear && g_display) g_display->oledClear();
    if (text && g_display) {
        g_display->oledDrawText(text, 0, 0, 8);
    }
    if (send && g_display) g_display->oledUpdate();
}

// Duplicate functions removed - implemented in real PocketMage source

// PocketMage configuration constants - duplicates removed, defined in real PocketMage globals.cpp
bool SET_CLOCK_ON_UPLOAD = false;
String SLEEPMODE = "NORMAL";

// useFastFullUpdate is defined in real PocketMage globals.cpp

long random(long max) {
    return rand() % max;
}

long random(long min, long max) {
    return min + (rand() % (max - min));
}

// MockSerial implementation
// SerialClass methods are implemented in the class definition

// String implementation removed - using std::string directly

// File implementation
File::File() : inFile(nullptr), outFile(nullptr), isOpen(false), isDir(false), dirIndex(0) {}

File::File(const std::string& path, const std::string& mode) : inFile(nullptr), outFile(nullptr), isOpen(false), isDir(false), filePath(path), dirIndex(0) {
    std::string fullPath = "./data/" + path;
    // Normalize directory flag
    try {
        std::filesystem::path p(fullPath);
        if (std::filesystem::exists(p) && std::filesystem::is_directory(p)) {
            // Directory handle
            isDir = true;
            isOpen = true;
            // Build directory listing (files only, to mirror Arduino FS behavior used)
            dirIndex = 0;
            dirEntries.clear();
            for (auto& entry : std::filesystem::directory_iterator(p)) {
                if (entry.is_regular_file()) {
                    dirEntries.push_back(entry.path().filename().string());
                }
            }
        } else {
            isDir = false;
            // Open file
            if (mode == "r" || mode == FILE_READ) {
                inFile = std::make_unique<std::ifstream>(fullPath);
                isOpen = inFile->is_open();
            } else if (mode == "w" || mode == FILE_WRITE) {
                outFile = std::make_unique<std::ofstream>(fullPath);
                isOpen = outFile->is_open();
            } else if (mode == "a" || mode == FILE_APPEND) {
                outFile = std::make_unique<std::ofstream>(fullPath, std::ios::app);
                isOpen = outFile->is_open();
            }
        }
    } catch (...) {
        isOpen = false;
    }
    
    if (!isOpen) {
        std::cout << "[File] Failed to open: " << fullPath << std::endl;
    }
}

File::~File() {
    if (inFile && inFile->is_open()) inFile->close();
    if (outFile && outFile->is_open()) outFile->close();
}

// Move constructor
File::File(File&& other) noexcept
    : inFile(std::move(other.inFile))
    , outFile(std::move(other.outFile))
    , isOpen(other.isOpen)
    , isDir(other.isDir)
    , filePath(std::move(other.filePath))
    , dirEntries(std::move(other.dirEntries))
    , dirIndex(other.dirIndex)
{
    other.isOpen = false;
    other.isDir = false;
    other.dirIndex = 0;
}

// Move assignment operator
File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        // Close current files
        if (inFile && inFile->is_open()) inFile->close();
        if (outFile && outFile->is_open()) outFile->close();
        
        // Move from other
        inFile = std::move(other.inFile);
        outFile = std::move(other.outFile);
        isOpen = other.isOpen;
        isDir = other.isDir;
        filePath = std::move(other.filePath);
        dirEntries = std::move(other.dirEntries);
        dirIndex = other.dirIndex;
        
        // Reset other
        other.isOpen = false;
        other.isDir = false;
        other.dirIndex = 0;
    }
    return *this;
}

void File::close() {
    if (inFile && inFile->is_open()) inFile->close();
    if (outFile && outFile->is_open()) outFile->close();
    isOpen = false;
}

size_t File::write(const uint8_t* data, size_t len) {
    if (!outFile || !outFile->is_open() || !data) return 0;
    outFile->write(reinterpret_cast<const char*>(data), len);
    return len;
}

size_t File::write(const String& str) {
    return write(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int File::read() {
    if (!inFile || !inFile->is_open()) return -1;
    return inFile->get();
}

String File::readString() {
    if (!inFile || !inFile->is_open()) return String("");
    std::string result((std::istreambuf_iterator<char>(*inFile)), std::istreambuf_iterator<char>());
    return String(result);
}

String File::readStringUntil(char terminator) {
    if (!inFile || !inFile->is_open()) return String("");
    std::string result;
    std::getline(*inFile, result, terminator);
    return String(result);
}

bool File::available() {
    if (!inFile || !inFile->is_open()) return false;
    return inFile->peek() != EOF;
}

void File::seek(size_t pos) {
    if (inFile && inFile->is_open()) inFile->seekg(pos);
    if (outFile && outFile->is_open()) outFile->seekp(pos);
}

size_t File::size() {
    if (!inFile || !inFile->is_open()) return 0;
    auto current = inFile->tellg();
    inFile->seekg(0, std::ios::end);
    auto size = inFile->tellg();
    inFile->seekg(current);
    return size;
}

bool File::isDirectory() {
    return isDir;
}

File File::openNextFile() {
    if (!isDir) return File();
    if (dirIndex >= dirEntries.size()) return File();
    std::string child = dirEntries[dirIndex++];
    // Open child as a file (read mode)
    // Convert full path back to relative path under ./data/
    std::string rel = child;
    const std::string prefix = "./data/";
    if (rel.rfind(prefix, 0) == 0) rel = rel.substr(prefix.size());
    // If the child is a directory, return a File representing that directory
    try {
        if (std::filesystem::is_directory(child)) {
            return File(rel, "r");
        }
    } catch (...) {}
    return File(rel, "r");
}

String File::name() {
    try {
        std::filesystem::path p(filePath);
        return String(p.filename().string());
    } catch (...) {
        return String("");
    }
}

bool File::print(const char* msg) {
    if (!outFile || !outFile->is_open() || msg == nullptr) return false;
    *outFile << msg;
    return outFile->good();
}

bool File::print(const String& msg) {
    return print(msg.c_str());
}

bool File::println(const char* msg) {
    if (!outFile || !outFile->is_open()) return false;
    if (msg) *outFile << msg;
    *outFile << '\n';
    return outFile->good();
}

bool File::println(const String& msg) {
    return println(msg.c_str());
}

// SD_MMC implementation methods are in SD_MMC.h

// Hardware implementations are now in their respective header files

// All mock implementations are now in their respective header files

// Mock GPIO and system functions
void pinMode(uint8_t pin, uint8_t mode) {}
int digitalRead(uint8_t pin) { return 0; }
void digitalWrite(uint8_t pin, uint8_t value) {}
int analogRead(uint8_t pin) { return 512; }

// updateKeypress is implemented in real PocketMage sysFunc.cpp
BaseType_t xTaskCreatePinnedToCore(void (*pvTaskCode)(void*), const char* const pcName,
                                  const uint32_t ulStackDepth, void* const pvParameters,
                                  UBaseType_t uxPriority, TaskHandle_t* const pvCreatedTask,
                                  const BaseType_t xCoreID) {
    std::cout << "[FreeRTOS] Created task: " << pcName << " on core " << xCoreID << std::endl;
    
    // For the einkHandler task, we need to actually call it to render the UI
    if (std::string(pcName) == "einkHandlerTask" && pvTaskCode) {
        std::cout << "[Emulator] Starting einkHandler task for UI rendering" << std::endl;
        // Call the task function once to trigger initial rendering
        pvTaskCode(pvParameters);
    }
    
    return pdPASS;
}
// yield() already defined in esp32_shims.h
void esp_sleep_enable_ext0_wakeup(uint8_t pin, int level) {}
// esp_deep_sleep_start is already defined in pocketmage_compat.h
// setCpuFrequencyMhz already defined earlier in the file

// SD_MMCClass method implementations
File SD_MMCClass::open(const String& path, const char* mode) {
    return open(path.c_str(), mode);
}

File SD_MMCClass::open(const char* path, const char* mode) {
    if (!path) return File();
    return File(std::string(path), std::string(mode ? mode : "r"));
}

bool SD_MMCClass::exists(const String& path) { return exists(path.c_str()); }

bool SD_MMCClass::exists(const char* path) {
    if (!path) return false;
    std::filesystem::path p(std::string("./data/") + path);
    return std::filesystem::exists(p);
}

bool SD_MMCClass::mkdir(const String& path) { return mkdir(path.c_str()); }

bool SD_MMCClass::mkdir(const char* path) {
    if (!path) return false;
    std::filesystem::path p(std::string("./data/") + path);
    std::error_code ec;
    return std::filesystem::create_directories(p, ec) || std::filesystem::exists(p);
}

bool SD_MMCClass::remove(const String& path) { return remove(path.c_str()); }

bool SD_MMCClass::remove(const char* path) {
    if (!path) return false;
    std::filesystem::path p(std::string("./data/") + path);
    std::error_code ec;
    return std::filesystem::remove(p, ec);
}

bool SD_MMCClass::rmdir(const String& path) { return rmdir(path.c_str()); }

bool SD_MMCClass::rmdir(const char* path) {
    if (!path) return false;
    std::filesystem::path p(std::string("./data/") + path);
    std::error_code ec;
    return std::filesystem::remove_all(p, ec) > 0;
}

bool SD_MMCClass::rename(const char* path1, const char* path2) {
    if (!path1 || !path2) return false;
    std::filesystem::path p1(std::string("./data/") + path1);
    std::filesystem::path p2(std::string("./data/") + path2);
    std::error_code ec;
    std::filesystem::create_directories(p2.parent_path(), ec);
    std::filesystem::rename(p1, p2, ec);
    return !ec;
}

bool SD_MMCClass::rename(const String& path1, const String& path2) {
    return rename(path1.c_str(), path2.c_str());
}

// Missing function stubs for linker
// removeChar function removed - implemented in real PocketMage source

// refresh() function removed - already defined earlier

void oledScroll() {
    // Mock OLED scroll function
}

void setTXTFont(const GFXfont* font) {
    // Mock font setting function
}

void vTaskDelete(void* handle) {
    // Mock FreeRTOS task deletion
}

void drawStatusBar(String status) {
    std::cout << "[StatusBar] " << status.c_str() << std::endl;
}

// Missing functions for linker
int countLines(String text, unsigned long maxWidth) {
    // Simple line counting implementation
    int lines = 1;
    for (size_t i = 0; i < text.length(); i++) {
        if (text.charAt(i) == '\n') {
            lines++;
        }
    }
    return lines;
}

// Forward declaration
void applicationEinkHandler();

void einkHandler(void* parameter) {
    // Mock FreeRTOS task - call applicationEinkHandler in loop
    while (true) {
        applicationEinkHandler();
        vTaskDelay(50);
        yield();
    }
}

void processKB_USB() {
    // Mock USB keyboard processing
    std::cout << "[USB] Processing keyboard input" << std::endl;
}

void einkHandler_USB() {
    // Mock USB handler
    std::cout << "[Display] USB handler called" << std::endl;
}

void einkTextDynamic(bool refresh, bool clear) {
    std::cout << "[EinkText] refresh=" << refresh << " clear=" << clear << std::endl;
}

void multiPassRefesh(int passes) {
    std::cout << "[MultiPass] passes=" << passes << std::endl;
}

void listDir(SD_MMCClass& fs, const char* dirname) {
    std::cout << "[ListDir] " << dirname << std::endl;
}

void oledLine(String text, bool center, String prefix) {
    if (g_display) {
        String fullText = prefix + ": " + text;
        g_display->oledDrawText(fullText.c_str(), center ? 64 : 0, 16, 8);
        g_display->oledUpdate();
    }
}

void oledWord(String text, bool center, bool newline) {
    if (g_display) {
        static int oled_y = 0;
        g_display->oledDrawText(text.c_str(), center ? 64 : 0, oled_y, 8);
        if (newline) oled_y += 8;
        if (oled_y >= 32) oled_y = 0;
        g_display->oledUpdate();
    }
}

void statusBar(String text, bool refresh) {
    if (g_display) {
        // Draw status bar at bottom of OLED display
        g_display->oledDrawText(text.c_str(), 0, 24, 8);
        if (refresh) g_display->oledUpdate();
    }
}
