#include "pocketmage_compat.h"
#include "desktop_display_sdl2.h"
#include "oled_service.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>

bool loadKeyboardLayout(const String& layoutName);

enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON };

extern DesktopDisplay* g_display;

void setup();
void loop();

void einkHandler_HOME();
void einkHandler_TXT();
void einkHandler_FILEWIZ();
void einkHandler_USB();
void einkHandler_TASKS();
void einkHandler_SETTINGS();
void einkHandler_CALENDAR();
void einkHandler_JOURNAL();
void einkHandler_LEXICON();

extern void TXT_INIT();
extern void FILEWIZ_INIT();
extern void TASKS_INIT();
extern void SETTINGS_INIT();
extern void CALENDAR_INIT();
extern void JOURNAL_INIT();
extern void LEXICON_INIT();

extern void applicationEinkHandler();
void emulatorSetup() {
    std::cout << "===================================" << std::endl;
    std::cout << "PocketMage Desktop Emulator v1.0" << std::endl;
    std::cout << "===================================" << std::endl;
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
    
    std::cout << "Calling PocketMage setup()..." << std::endl;
    // Call real PocketMage setup in a separate thread to avoid blocking
    std::thread setupThread([]() {
        setup();
    });
    setupThread.detach(); // Let it run independently
    
    // Give setup time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "PocketMage setup() started" << std::endl;
    
    // Initialize keyboard layout and dead keys
    std::cout << "Loading keyboard layout..." << std::endl;
    loadKeyboardLayout("us-latin");
    
    // Force initial drawing to populate the framebuffers
    std::cout << "Drawing initial HOME screen..." << std::endl;
    
    // Force PocketMage to redraw HOME screen by setting newState = true
    newState = true;
    
    std::cout << "Calling PocketMage HOME handler..." << std::endl;
    applicationEinkHandler();
    g_display->present();
    std::cout << "Initial drawing complete." << std::endl;

    // Show PocketMage on the small OLED at startup
    oled_set_lines("PocketMage", "Desktop Emulator", "Ready");
    // Will be presented by oled_present_if_dirty() in main loop
}

// Emulator-specific loop
void emulatorLoop() {
    static int frameCount = 0;
    frameCount++;
    
    // Force HOME screen redraw every 60 frames to show icons
    if (frameCount % 60 == 0) {
        extern volatile bool newState;
        newState = true;
    }
    
    // Handle SDL events first
    if (!g_display->handleEvents()) {
        return; // Quit requested
    }
    
    // Call real PocketMage loop to process input
    loop();
    
    // Call the E-Ink handler to render UI
    applicationEinkHandler();
    
    // Present only after all rendering is complete
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
        // Handle SDL events first
        if (!g_display->handleEvents()) {
            running = false;
            break;
        }
        
        // Call real PocketMage loop to process input
        loop();
        
        // Call the E-Ink handler to render UI
        applicationEinkHandler();
        
        // Present OLED updates once (main thread)
        oled_present_if_dirty();
        
        // Present only after all rendering is complete
        g_display->present();
        
        frameCount++;
        if (frameCount % 100 == 0) {
            std::cout << "[MAIN] Frame " << frameCount << std::endl;
        }
        
        // Limit frame rate
        delay(33); // ~30 FPS
    }
    
    // Cleanup
    if (g_display) {
        delete g_display;
        g_display = nullptr;
    }
    
    std::cout << "Emulator shut down." << std::endl;
    return 0;
}
