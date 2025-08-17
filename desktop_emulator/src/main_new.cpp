#include "pocketmage_compat.h"
#include "desktop_display.h"
#include "SD_MMC.h"
#include <iostream>

// Include globals to access AppState enum
enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON };

// Desktop emulator globals
DesktopDisplay* g_display = nullptr;

// Forward declarations for real PocketMage functions
void setup();
void loop();

// Forward declarations for real PocketMage display handlers
void einkHandler_HOME();
void einkHandler_TXT();
void einkHandler_FILEWIZ();
void einkHandler_USB();
void einkHandler_TASKS();
void einkHandler_SETTINGS();
void einkHandler_CALENDAR();
void einkHandler_JOURNAL();
void einkHandler_LEXICON();

// Forward declare PocketMage initialization functions
extern void TXT_INIT();
extern void FILEWIZ_INIT();
extern void TASKS_INIT();
extern void SETTINGS_INIT();
extern void CALENDAR_INIT();
extern void JOURNAL_INIT();
extern void LEXICON_INIT();

// Emulator-specific setup
void emulatorSetup() {
    std::cout << "=== PocketMage Desktop Emulator ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow keys - Navigation" << std::endl;
    std::cout << "  Enter - Select/Confirm" << std::endl;
    std::cout << "  Backspace - Delete" << std::endl;
    std::cout << "  Letters/Numbers - Text input" << std::endl;
    std::cout << "  Close window - Quit" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Initialize SDL display
    g_display = new DesktopDisplay();
    if (!g_display->init()) {
        std::cerr << "Failed to initialize display!" << std::endl;
        exit(1);
    }
    
    // Initialize PocketMage state variables first
    extern volatile bool newState;
    extern AppState CurrentAppState;
    newState = true;
    CurrentAppState = HOME;
    
    // Call real PocketMage setup
    setup();
}

// Forward declare the application handler
extern void applicationEinkHandler();

// Emulator-specific loop
void emulatorLoop() {
    if (!g_display->handleEvents()) {
        return; // Quit requested
    }
    
    // Call real PocketMage loop
    loop();
    
    // Call the E-Ink handler to render UI
    applicationEinkHandler();
    
    g_display->present();
    
    // Limit frame rate
    delay(33); // ~30 FPS
}

int main() {
    std::cout << "Starting PocketMage Desktop Emulator..." << std::endl;
    emulatorSetup();
    
    std::cout << "Entering main loop..." << std::endl;
    // Main loop
    bool running = true;
    int frameCount = 0;
    while (running) {
        emulatorLoop();
        frameCount++;
        if (frameCount % 100 == 0) {
            std::cout << "Frame " << frameCount << std::endl;
        }
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
