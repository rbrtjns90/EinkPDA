#include "desktop_display.h"
#include "hardware_shim.h"
#include <iostream>
#include <thread>

// Include the original PocketMage headers with our shims
#define Arduino_h // Prevent Arduino.h from being included

// Mock config values since we can't include the original config.h
#define TXT_APP_STYLE 0
#define MAX_FILES 100
#define SET_CLOCK_ON_UPLOAD false

// Mock the hardware objects that the original code expects
MockGxEPD2 display;
MockU8G2 u8g2;
MockKeypad keypad;
MockBuzzer buzzer;
MockMPR121 cap;
MockRTC rtc;
MockPreferences prefs;

// Mock the global variables from globals.h
volatile int einkRefresh = 0;
int OLEDFPSMillis = 0;
int KBBounceMillis = 0;
volatile int timeoutMillis = 0;
volatile int prevTimeMillis = 0;
volatile bool TCA8418_event = false;
volatile bool PWR_BTN_event = false;
volatile bool SHFT = false;
volatile bool FN = false;
volatile bool newState = false;
bool noTimeout = false;
volatile bool OLEDPowerSave = false;
volatile bool disableTimeout = false;
volatile int battState = 100;
volatile int prevBattState = 100;
unsigned int flashMillis = 0;
int prevTime = 0;
uint8_t prevSec = 0;
TaskHandle_t einkHandlerTaskHandle = nullptr;
char currentKB[4][10];
volatile bool SDCARD_INSERT = true;
bool noSD = false;
volatile bool SDActive = true;

// Keyboard arrays
char keysArray[4][10] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';'},
    {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'}
};

char keysArraySHFT[4][10] = {
    {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'},
    {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'}
};

char keysArrayFN[4][10] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';'},
    {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'}
};

// Enum states
enum KBState { NORMAL, SHIFT, FUNC };
KBState CurrentKBState = NORMAL;

enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON };
AppState CurrentAppState = HOME;

const String appStateNames[] = {"HOME", "TXT", "FILEWIZ", "USB_APP", "BT", "SETTINGS", "TASKS", "CALENDAR", "JOURNAL", "LEXICON"};

uint8_t partialCounter = 0;
volatile bool forceSlowFullUpdate = false;

// Settings variables
int TIMEOUT = 30;
bool DEBUG_VERBOSE = false;
bool SYSTEM_CLOCK = true;
bool SHOW_YEAR = false;
bool SAVE_POWER = false;
bool ALLOW_NO_MICROSD = true;
bool HOME_ON_BOOT = true;
int OLED_BRIGHTNESS = 255;
int OLED_MAX_FPS = 30;

// TXT app variables
String currentWord = "";
String allText = "";
String prevAllText = "";
String prevLastLine = "";
bool prevBKSP = false;
int scroll = 0;
int lines = 0;
String outLines[13];
String lines_prev[13];
String filesList[100]; // MAX_FILES not defined, using 100
uint8_t fileIndex = 0;
String editingFile = "";
String prevEditingFile = "";
String excludedFiles[3] = {"", "", ""};

enum TXTState { TXT_, WIZ0, WIZ1, WIZ2, WIZ3, FONT };
TXTState CurrentTXTState = TXT_;

String currentLine = "";
const void* currentFont = nullptr;
uint8_t maxCharsPerLine = 29;
uint8_t maxLines = 13;
uint8_t fontHeight = 10;
uint8_t lineSpacing = 2;
volatile bool newLineAdded = false;
volatile bool doFull = false;
std::vector<String> allLines;
volatile long int dynamicScroll = 0;
volatile long int prev_dynamicScroll = 0;
int lastTouch = 0;
unsigned long lastTouchTime = 0;

// Other app state variables
std::vector<std::vector<String>> tasks;
uint8_t selectedTask = 0;

enum TasksState { TASKS0, TASKS0_NEWTASK, TASKS1, TASKS1_EDITTASK };
TasksState CurrentTasksState = TASKS0;

uint8_t newTaskState = 0;
uint8_t editTaskState = 0;
String newTaskName = "";
String newTaskDueDate = "";

enum HOMEState { HOME_HOME, NOWLATER };
HOMEState CurrentHOMEState = HOME_HOME;

enum FileWizState { WIZ0_, WIZ1_, WIZ1_YN, WIZ2_R, WIZ2_C, WIZ3_ };
FileWizState CurrentFileWizState = WIZ0_;
String workingFile = "";

enum SettingsState { settings0, settings1 };
SettingsState CurrentSettingsState = settings0;

enum CalendarState { WEEK, MONTH, NEW_EVENT, VIEW_EVENT, SUN, MON, TUE, WED, THU, FRI, SAT };
CalendarState CurrentCalendarState = WEEK;

enum LexState {MENU, DEF};
LexState CurrentLexState = MENU;

enum JournalState {J_MENU, J_TXT};
JournalState CurrentJournalState = J_MENU;

// RTC constants
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Simple keyboard input handler for desktop
char lastInputChar = 0;
bool keyboardInputAvailable = false;

// Forward declarations
void applicationEinkHandler();
void processKB();

char updateKeypress() {
    if (!g_display) return 0;
    
    char key = g_display->getLastKey();
    if (key != 0) {
        keyboardInputAvailable = true;
        lastInputChar = key;
        
        // Handle special keys
        switch (key) {
            case 'U': return 'U'; // Up arrow
            case 'D': return 'D'; // Down arrow  
            case 'L': return 'L'; // Left arrow
            case 'R': return 'R'; // Right arrow
            case '\n': return '\n'; // Enter
            case '\b': return '\b'; // Backspace
            default:
                if (key >= 'a' && key <= 'z') {
                    return SHFT ? (key - 'a' + 'A') : key;
                }
                return key;
        }
    }
    
    keyboardInputAvailable = false;
    return 0;
}

// Simplified versions of some system functions
void checkTimeout() {
    // Desktop version doesn't need timeout
}

void PWR_BTN_irq() {
    PWR_BTN_event = true;
}

void TCA8418_irq() {
    TCA8418_event = true;
}

void updateBattState() {
    // Mock battery at 100%
    battState = 100;
}

void printDebug() {
    if (DEBUG_VERBOSE) {
        std::cout << "[Debug] App: " << appStateNames[CurrentAppState].data 
                  << " Battery: " << battState << "%" << std::endl;
    }
}

// Enhanced app functions showing PocketMage-style interface
void einkHandler_HOME() {
    if (!g_display) return;
    
    g_display->einkClear();
    
    // Draw title bar
    g_display->einkDrawRect(0, 0, 310, 20, true, true);
    g_display->einkDrawText("PocketMage V3", 10, 5);
    
    // Draw app icons/menu
    g_display->einkDrawText("Applications:", 10, 30);
    g_display->einkDrawText("> Text Editor", 20, 50);
    g_display->einkDrawText("  File Manager", 20, 65);
    g_display->einkDrawText("  Tasks", 20, 80);
    g_display->einkDrawText("  Calendar", 20, 95);
    g_display->einkDrawText("  Settings", 20, 110);
    
    // Draw selection indicator
    g_display->einkDrawText("*", 10, 50);
    
    g_display->einkRefresh();
}

void processKB_HOME() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[Input] Key pressed: '" << key << "'" << std::endl;
        
        // Update OLED with system info
        if (g_display) {
            g_display->oledClear();
            g_display->oledDrawText("PocketMage V3 - Battery: 100%", 5, 5);
            g_display->oledDrawText("Key: " + std::string(1, key) + " | Apps: 5", 5, 20);
            g_display->oledUpdate();
        }
        
        // Handle navigation
        switch (key) {
            case 'U': // Up arrow
                std::cout << "[Navigation] Up - Previous app" << std::endl;
                break;
            case 'D': // Down arrow
                std::cout << "[Navigation] Down - Next app" << std::endl;
                break;
            case '\n': // Enter
                std::cout << "[Navigation] Enter - Launch Text Editor" << std::endl;
                break;
            case 't':
                std::cout << "[Demo] Switching to text editor mode" << std::endl;
                CurrentAppState = TXT;
                applicationEinkHandler();
                break;
        }
    }
}

// Text editor demo
void einkHandler_TXT() {
    if (!g_display) return;
    
    g_display->einkClear();
    
    // Draw title bar
    g_display->einkDrawRect(0, 0, 310, 20, true, true);
    g_display->einkDrawText("Text Editor", 10, 5);
    
    // Draw text content
    g_display->einkDrawText("Sample text file:", 10, 30);
    g_display->einkDrawText("Hello from PocketMage!", 10, 50);
    g_display->einkDrawText("This is a demo of the", 10, 65);
    g_display->einkDrawText("desktop emulator running", 10, 80);
    g_display->einkDrawText("actual PocketMage code.", 10, 95);
    g_display->einkDrawText("Press 'h' to go home.", 10, 110);
    
    // Draw cursor
    g_display->einkDrawText("_", 200, 95);
    
    g_display->einkRefresh();
}

void processKB_TXT() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[TXT] Key pressed: '" << key << "'" << std::endl;
        
        if (g_display) {
            g_display->oledClear();
            g_display->oledDrawText("Text Editor - Editing file", 5, 5);
            g_display->oledDrawText("Char: " + std::string(1, key) + " | Line: 5", 5, 20);
            g_display->oledUpdate();
        }
        
        if (key == 'h') {
            std::cout << "[TXT] Returning to home" << std::endl;
            CurrentAppState = HOME;
            applicationEinkHandler();
        }
    }
}

void applicationEinkHandler() {
    switch (CurrentAppState) {
        case HOME:
            einkHandler_HOME();
            break;
        case TXT:
            einkHandler_TXT();
            break;
        default:
            einkHandler_HOME();
            break;
    }
}

void processKB() {
    switch (CurrentAppState) {
        case HOME:
            processKB_HOME();
            break;
        case TXT:
            processKB_TXT();
            break;
        default:
            processKB_HOME();
            break;
    }
}

// Mock setup function
void setup() {
    Serial.begin(115200);
    Serial.println("PocketMage Desktop Emulator Starting...");
    
    // Initialize SDL display
    g_display = new DesktopDisplay();
    if (!g_display->init()) {
        std::cerr << "Failed to initialize display!" << std::endl;
        exit(1);
    }
    
    // Initialize mock SD card
    SD_MMC.begin();
    
    Serial.println("Setup complete!");
    
    // Show initial screen
    applicationEinkHandler();
}

// Mock loop function  
void loop() {
    if (!g_display->handleEvents()) {
        return; // Quit requested
    }
    
    processKB();
    g_display->present();
    
    // Limit frame rate
    delay(33); // ~30 FPS
}

int main() {
    std::cout << "=== PocketMage Desktop Emulator ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow keys - Navigation" << std::endl;
    std::cout << "  Enter - Select/Confirm" << std::endl;
    std::cout << "  Backspace - Delete" << std::endl;
    std::cout << "  Letters/Numbers - Text input" << std::endl;
    std::cout << "  Close window - Quit" << std::endl;
    std::cout << "===================================" << std::endl;
    
    setup();
    
    // Main loop
    bool running = true;
    while (running) {
        loop();
        if (!g_display || !g_display->handleEvents()) {
            running = false;
        }
    }
    
    // Cleanup
    if (g_display) {
        delete g_display;
        g_display = nullptr;
    }
    
    std::cout << "Emulator shut down." << std::endl;
    return 0;
}
