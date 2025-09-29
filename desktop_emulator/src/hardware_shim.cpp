#include "pocketmage_compat.h"
#include "desktop_display_sdl2.h"
#include "oled_service.h"
#include "globals.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include "SD_MMC.h"
#include "Wire.h"
#include "Preferences.h"
#include "Adafruit_TCA8418.h"
#include "Adafruit_GFX.h"

extern DesktopDisplay* g_display;
#include "GxEPD2_BW.h"
#include "U8g2lib.h"
#include "Buzzer.h"
#include "USB.h"

SerialClass Serial;
SD_MMCClass SD_MMC;
TwoWire Wire;
SPIClass SPI;

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
    oled_set_lines(text.c_str(), "", "");
}

void oledLine(const String& text, bool /*clear*/) {
    oled_set_lines(text.c_str(), "", "");
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


// Only USB_INIT is missing from real PocketMage source
void USB_INIT() {
    std::cout << "[USB] App initialized" << std::endl;
}

// PocketMage specific function implementations
void playJingle(const char* name) {
    std::cout << "[Audio] Playing jingle: " << name << std::endl;
}

void oledWord(const char* text, bool /*clear*/, bool /*send*/) {
    oled_set_lines(text ? text : "", "", "");
}


long random(long max) {
    return rand() % max;
}

long random(long min, long max) {
    return min + (rand() % (max - min));
}


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


// Mock GPIO and system functions
void pinMode(uint8_t pin, uint8_t mode) {}
int digitalRead(uint8_t pin) { return 0; }
void digitalWrite(uint8_t pin, uint8_t value) {}
int analogRead(uint8_t pin) { return 512; }

bool emulatorConsumeUTF8(String& output) {
    if (g_display && g_display->hasUTF8Input()) {
        std::string utf8Text = g_display->getUTF8Input();
        output = String(utf8Text.c_str());
        std::cout << "[UTF8] emulatorConsumeUTF8() returning: '" << output.c_str() << "'" << std::endl;
        return true;
    }
    return false;
}

// Missing function implementations for linker
char updateKeypress() {
    // Legacy keyboard function - return no key pressed
    return 0;
}

String utf8SafeBackspace(const String& text) {
    if (text.length() == 0) return text;
    
    // Simple implementation - remove last character
    // In a full implementation, this would handle UTF-8 multi-byte characters properly
    String result = text;
    result.remove(result.length() - 1);
    return result;
}

bool loadKeyboardLayout(const String& layoutName) {
    std::cout << "[UTF8] loadKeyboardLayout() called with: " << layoutName.c_str() << std::endl;
    
    // Manually populate DeadTable for emulator since JSON loading isn't working
    DeadTable.clear();
    
    // Apostrophe (') dead key combinations
    DeadTable.push_back({"'", "a", "á"});
    DeadTable.push_back({"'", "e", "é"});
    DeadTable.push_back({"'", "i", "í"});
    DeadTable.push_back({"'", "o", "ó"});
    DeadTable.push_back({"'", "u", "ú"});
    DeadTable.push_back({"'", "y", "ý"});
    DeadTable.push_back({"'", "A", "Á"});
    DeadTable.push_back({"'", "E", "É"});
    DeadTable.push_back({"'", "I", "Í"});
    DeadTable.push_back({"'", "O", "Ó"});
    DeadTable.push_back({"'", "U", "Ú"});
    DeadTable.push_back({"'", "Y", "Ý"});
    DeadTable.push_back({"'", "c", "ć"});
    DeadTable.push_back({"'", "C", "Ć"});
    DeadTable.push_back({"'", "n", "ń"});
    DeadTable.push_back({"'", "N", "Ń"});
    DeadTable.push_back({"'", "s", "ś"});
    DeadTable.push_back({"'", "S", "Ś"});
    DeadTable.push_back({"'", "z", "ź"});
    DeadTable.push_back({"'", "Z", "Ź"});
    
    std::cout << "[UTF8] Populated DeadTable with " << DeadTable.size() << " rules" << std::endl;
    
    return true;
}

KeyEvent updateKeypressUTF8() {
    KeyEvent ev{false, KA_NONE, "", 0, 0};

#ifdef DESKTOP_EMULATOR
    // Check for special keys first
    if (g_display) {
        char lastKey = g_display->getLastKey();
        if (lastKey != 0) {
            ev.hasEvent = true;
            std::cout << "[UTF8] Special key detected: " << (int)lastKey << std::endl;
            
            switch (lastKey) {
                case 8: ev.action = KA_BACKSPACE; return ev;
                case 13: ev.action = KA_ENTER; return ev;
                case 27: ev.action = KA_ESC; return ev;
                case 9: ev.action = KA_TAB; return ev;
                case 32: ev.action = KA_SPACE; return ev;
                case 18: ev.action = KA_RIGHT; return ev;
                case 19: ev.action = KA_UP; return ev;
                case 20: ev.action = KA_LEFT; return ev;
                case 21: ev.action = KA_DOWN; return ev;
                case 12: ev.action = KA_HOME; return ev;
                case -1: ev.action = KA_CYCLE_LAYOUT; std::cout << "[KEYBOARD] Fn+K mapped to KA_CYCLE_LAYOUT" << std::endl; return ev;
                case 200: ev.action = KA_DEAD; ev.text = "'"; std::cout << "[KEYBOARD] Apostrophe mapped to KA_DEAD" << std::endl; return ev;
                default:
                    // Handle regular character keys (a-z, A-Z, 0-9, etc.)
                    if (lastKey >= 32 && lastKey <= 126) {
                        ev.action = KA_CHAR;
                        ev.text = String((char)lastKey);
                        std::cout << "[KEYBOARD] Character key mapped: '" << (char)lastKey << "'" << std::endl;
                        return ev;
                    } else {
                        std::cout << "[UTF8] Unmapped special key: " << (int)lastKey << std::endl;
                        ev.hasEvent = false;
                        break;
                    }
            }
        }
    }
    
    // Check for UTF-8 text input
    String host;
    if (emulatorConsumeUTF8(host)) {
        KeyEvent ev{true, KA_CHAR, host, 0, 0};
        std::cout << "[UTF8] Text input: '" << host.c_str() << "'" << std::endl;
        return ev;
    }
#endif

    // For emulator, return no event
    return ev;
}

void mirrorLayoutToLegacy() {
    // Mirror current layout to legacy arrays for compatibility
    // Implementation would copy CurrentLayout to legacy key arrays
    std::cout << "[UTF8] mirrorLayoutToLegacy() called" << std::endl;
}

void stringToVector(String inputText) {
    // Implementation for the void version declared in globals.h
    std::cout << "[UTF8] stringToVector() called with: " << inputText.c_str() << std::endl;
}

void cycleKeyboardLayout() {
    // Available keyboard layouts in order
    static const String layouts[] = {"us-basic", "us-latin", "fr-azerty", "de-qwertz"};
    static const int layoutCount = 4;
    
    // Find current layout index
    int currentIndex = 0;
    for (int i = 0; i < layoutCount; i++) {
        if (CurrentLayoutName == layouts[i]) {
            currentIndex = i;
            break;
        }
    }
    
    // Cycle to next layout
    int nextIndex = (currentIndex + 1) % layoutCount;
    String nextLayout = layouts[nextIndex];
    
    std::cout << "[KEYBOARD] Cycling from " << CurrentLayoutName.c_str() << " to " << nextLayout.c_str() << std::endl;
    
    // For emulator, just update the current layout name and show message
    CurrentLayoutName = nextLayout;
    oled_set_lines(("Keyboard: " + nextLayout).c_str(), "", "");
    std::cout << "[KEYBOARD] Switched to layout: " << nextLayout.c_str() << std::endl;
}

// Full implementations for missing sysFunc.cpp functions
void printDebug() {
    std::cout << "[DEBUG] Debug print called" << std::endl;
}

String removeChar(String str, char c) {
    String result = "";
    for (int i = 0; i < str.length(); i++) {
        if (str[i] != c) result += str[i];
    }
    return result;
}

void PWR_BTN_irq() {
    std::cout << "[IRQ] Power button interrupt" << std::endl;
}

void TCA8418_irq() {
    std::cout << "[IRQ] TCA8418 keyboard interrupt" << std::endl;
    extern volatile bool TCA8418_event;
    TCA8418_event = true;
}

int stringToInt(String str) {
    return str.toInt();
}

void appendToFile(String filename, String content) {
    std::cout << "[FILE] Appending to " << filename.c_str() << ": " << content.c_str() << std::endl;
    // Mock file operation - could implement actual file I/O if needed
}

void checkTimeout() {
    // Mock timeout check - PocketMage uses this for power management
}

std::vector<String> stringToVectorImpl(String str) {
    std::vector<String> result;
    String current = "";
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            if (current.length() > 0) {
                result.push_back(current);
                current = "";
            }
        } else {
            current += str[i];
        }
    }
    if (current.length() > 0) {
        result.push_back(current);
    }
    return result;
}

String vectorToString() {
    // Mock implementation - would normally convert vector back to string
    return "";
}

void updateBattState() {
    std::cout << "[BATTERY] Battery state updated" << std::endl;
    // Mock battery state update
}

void setTimeFromString(String timeStr) {
    std::cout << "[TIME] Setting time from: " << timeStr.c_str() << std::endl;
    // Mock time setting
}

void delFile(String filename) {
    std::cout << "[FILE] Deleting file: " << filename.c_str() << std::endl;
    // Mock file deletion
}

void renFile(String oldName, String newName) {
    std::cout << "[FILE] Renaming " << oldName.c_str() << " to " << newName.c_str() << std::endl;
    // Mock file rename
}

void copyFile(String src, String dest) {
    std::cout << "[FILE] Copying " << src.c_str() << " to " << dest.c_str() << std::endl;
    // Mock file copy
}

void loadFile(bool param) {
    std::cout << "[FILE] Loading file with param: " << param << std::endl;
    // Mock file loading
}

void saveFile() {
    std::cout << "[FILE] Saving file" << std::endl;
    // Mock file saving
}

void loadState(bool param) {
    std::cout << "[STATE] Loading state with param: " << param << std::endl;
    // Mock state loading - PocketMage uses this to restore app state
}
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
    std::cout << "[EinkTextDynamic] refresh=" << refresh << " clear=" << clear << std::endl;
    
    if (g_display) {
        if (clear) {
            g_display->einkClear();
        }
        
        // Render text from allLines vector and currentLine
        int y = 20; // Start position
        int lineHeight = 16;
        int maxLines = (128 - 40) / lineHeight; // Leave space for status bar
        
        // Calculate starting line based on scroll
        int startLine = std::max(0, (int)allLines.size() - maxLines + (int)dynamicScroll);
        
        // Debug: Show what we're trying to render
        std::cout << "[EinkTextDynamic] allLines.size()=" << allLines.size() << " currentLine='" << currentLine << "'" << std::endl;
        
        // Render lines from allLines vector
        for (int i = startLine; i < allLines.size() && y < 100; i++) {
            if (i >= 0 && allLines[i].length() > 0) {
                std::cout << "[EinkTextDynamic] Drawing line " << i << ": '" << allLines[i] << "'" << std::endl;
                g_display->einkDrawText(allLines[i].c_str(), 5, y, 12);
            }
            y += lineHeight;
        }
        
        // Render current line being typed
        if (currentLine.length() > 0 && y < 100) {
            std::cout << "[EinkTextDynamic] Drawing current line: '" << currentLine << "'" << std::endl;
            g_display->einkDrawText(currentLine.c_str(), 5, y, 12);
        }
    }
}

void einkTextPartial(String text, bool clear) {
    std::cout << "[EinkTextPartial] text='" << text.c_str() << "' clear=" << clear << std::endl;
    
    if (g_display) {
        if (clear) {
            g_display->einkClear();
        }
        
        // Simple text rendering - split by lines and render each line
        String currentText = text;
        int y = 20; // Start position
        int lineHeight = 16;
        
        while (currentText.length() > 0 && y < 128) {
            int newlinePos = currentText.indexOf('\n');
            String line;
            
            if (newlinePos != -1) {
                line = currentText.substring(0, newlinePos);
                currentText = currentText.substring(newlinePos + 1);
            } else {
                line = currentText;
                currentText = "";
            }
            
            if (line.length() > 0) {
                g_display->einkDrawText(line.c_str(), 5, y, 12);
            }
            y += lineHeight;
        }
    }
}

void multiPassRefesh(int passes) {
    std::cout << "[MultiPass] passes=" << passes << std::endl;
}

void listDir(SD_MMCClass& fs, const char* dirname) {
    std::cout << "[ListDir] " << dirname << std::endl;
}

void oledLine(String text, bool center, String prefix) {
    String fullText = prefix + ": " + text;
    oled_set_lines(fullText.c_str(), "", "");
}

void oledWord(String text, bool center, bool newline) {
    oled_set_lines(text.c_str(), "", "");
}

void statusBar(String text, bool refresh) {
    oled_set_lines("", "", text.c_str());
}
