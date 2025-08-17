#include "pocketmage_compat.h"
#include "desktop_display.h"
#include "SD_MMC.h"
#include "freertos/task.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

// Include real PocketMage globals
#include "../Code/PocketMage_V3/include/globals.h"

// Desktop emulator globals
DesktopDisplay* g_display = nullptr;

// Simple keyboard input handler for desktop
char lastInputChar = 0;
bool keyboardInputAvailable = false;

// Forward declarations for emulator-specific functions
void emulatorSetup();
void emulatorLoop();

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
        std::cout << "[Debug] App: " << appStateNames[CurrentAppState] 
                  << " Battery: " << battState << "%" << std::endl;
    }
}

// Use real PocketMage home screen implementation
int selectedApp = 0;
const int totalApps = 9;

// Forward declare real PocketMage functions
extern void drawHome();
extern void einkHandler_HOME();
extern void processKB_HOME();

// The real drawHome() function will be provided by PocketMage HOME.cpp
// Remove the mock implementation to use the actual PocketMage code

void einkHandler_HOME() {
    if (newState) {
        newState = false;
        drawHome();
    }
}

void processKB_HOME() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[Input] Key pressed: '" << key << "'" << std::endl;
        
        // Update OLED with system info
        if (g_display) {
            g_display->oledClear();
            g_display->oledDrawText("PocketMage V3 - Battery: 100%", 5, 5);
            g_display->oledDrawText("App: " + std::to_string(selectedApp + 1) + "/5", 5, 20);
            g_display->oledUpdate();
        }
        
        // Handle navigation with actual app selection
        switch (key) {
            case 'L': // Left arrow
                selectedApp = (selectedApp - 1 + totalApps) % totalApps;
                newState = true;
                std::cout << "[Navigation] Left - Selected app: " << selectedApp << std::endl;
                break;
            case 'R': // Right arrow
                selectedApp = (selectedApp + 1) % totalApps;
                newState = true;
                std::cout << "[Navigation] Right - Selected app: " << selectedApp << std::endl;
                break;
            case '\n': // Enter
                {
                    std::cout << "[Navigation] Enter - Launching app: " << selectedApp << std::endl;
                    // Map selectedApp index to actual AppState enum values
                    AppState appStates[] = {TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON};
                    if (selectedApp < sizeof(appStates)/sizeof(appStates[0])) {
                        CurrentAppState = appStates[selectedApp];
                        newState = true;
                        std::cout << "[Navigation] Launching: " << appStateNames[CurrentAppState].c_str() << std::endl;
                    }
                }
                break;
            case 't':
                selectedApp = 0; // Select TXT app
                CurrentAppState = TXT;
                newState = true;
                std::cout << "[Demo] Direct switch to text editor" << std::endl;
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

// File manager demo
void einkHandler_FILEWIZ() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("FILE MANAGER", 10, 20);
    g_display->einkDrawText("Files:", 10, 40);
    g_display->einkDrawText("- document.txt", 20, 60);
    g_display->einkDrawText("- notes.txt", 20, 80);
    g_display->einkDrawText("- config.cfg", 20, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

// Tasks demo
void einkHandler_TASKS() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("TASKS", 10, 20);
    g_display->einkDrawText("Today:", 10, 40);
    g_display->einkDrawText("[ ] Review code", 20, 60);
    g_display->einkDrawText("[x] Test emulator", 20, 80);
    g_display->einkDrawText("[ ] Write docs", 20, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

// Calendar demo
void einkHandler_CALENDAR() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("CALENDAR", 10, 20);
    g_display->einkDrawText("August 2025", 10, 40);
    g_display->einkDrawText("S  M  T  W  T  F  S", 10, 60);
    g_display->einkDrawText("               1  2", 10, 80);
    g_display->einkDrawText("3  4  5  6  7  8  9", 10, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

// Settings demo
void einkHandler_SETTINGS() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("SETTINGS", 10, 20);
    g_display->einkDrawText("Display:", 10, 40);
    g_display->einkDrawText("  Brightness: 100%", 20, 60);
    g_display->einkDrawText("  Timeout: 30s", 20, 80);
    g_display->einkDrawText("System:", 10, 100);
    g_display->einkDrawText("  Debug: OFF", 20, 120);
    g_display->einkDrawText("Press 'h' for Home", 10, 140);
    g_display->einkRefresh();
}

// USB demo
void einkHandler_USB() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("USB STORAGE", 10, 20);
    g_display->einkDrawText("Status: Connected", 10, 40);
    g_display->einkDrawText("Mode: Mass Storage", 10, 60);
    g_display->einkDrawText("Files accessible", 10, 80);
    g_display->einkDrawText("from computer", 10, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

// Journal demo
void einkHandler_JOURNAL() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("JOURNAL", 10, 20);
    g_display->einkDrawText("Today's Entry:", 10, 40);
    g_display->einkDrawText("Working on PocketMage", 10, 60);
    g_display->einkDrawText("emulator development.", 10, 80);
    g_display->einkDrawText("Great progress!", 10, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

// Lexicon demo
void einkHandler_LEXICON() {
    if (!g_display) return;
    
    g_display->einkClear();
    g_display->einkDrawText("LEXICON", 10, 20);
    g_display->einkDrawText("Dictionary & Thesaurus", 10, 40);
    g_display->einkDrawText("Search: emulator", 10, 60);
    g_display->einkDrawText("Definition: A system", 10, 80);
    g_display->einkDrawText("that mimics another", 10, 100);
    g_display->einkDrawText("Press 'h' for Home", 10, 120);
    g_display->einkRefresh();
}

void applicationEinkHandler() {
    switch (CurrentAppState) {
        case HOME:
            einkHandler_HOME();
            break;
        case TXT:
            einkHandler_TXT();
            break;
        case FILEWIZ:
            einkHandler_FILEWIZ();
            break;
        case TASKS:
            einkHandler_TASKS();
            break;
        case SETTINGS:
            einkHandler_SETTINGS();
            break;
        case USB_APP:
            einkHandler_USB();
            break;
        case CALENDAR:
            einkHandler_CALENDAR();
            break;
        case LEXICON:
            einkHandler_LEXICON();
            break;
        case JOURNAL:
            einkHandler_JOURNAL();
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
        case FILEWIZ:
            processKB_FILEWIZ();
            break;
        case TASKS:
            processKB_TASKS();
            break;
        case SETTINGS:
            processKB_SETTINGS();
            break;
        case USB_APP:
            processKB_USB();
            break;
        case CALENDAR:
            processKB_CALENDAR();
            break;
        case LEXICON:
            processKB_LEXICON();
            break;
        case JOURNAL:
            processKB_JOURNAL();
            break;
        default:
            processKB_HOME();
            break;
    }
}

// Keyboard handlers for other apps
void processKB_FILEWIZ() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[FILES] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[FILES] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_TASKS() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[TASKS] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[TASKS] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_CALENDAR() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[CALENDAR] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[CALENDAR] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_SETTINGS() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[SETTINGS] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[SETTINGS] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_USB() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[USB] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[USB] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_JOURNAL() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[JOURNAL] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[JOURNAL] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
    }
}

void processKB_LEXICON() {
    char key = updateKeypress();
    if (key != 0) {
        std::cout << "[LEXICON] Key pressed: '" << key << "'" << std::endl;
        if (key == 'h') {
            std::cout << "[LEXICON] Returning to home" << std::endl;
            CurrentAppState = HOME;
            newState = true;
        }
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
    
    // Initialize state
    newState = true;
    CurrentAppState = HOME;
    
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
    applicationEinkHandler(); // Update display after processing input
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
