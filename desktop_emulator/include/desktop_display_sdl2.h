#ifndef DESKTOP_DISPLAY_H
#define DESKTOP_DISPLAY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <cstdint>

// Display dimensions matching the hardware
#define EINK_WIDTH 310
#define EINK_HEIGHT 240  // Further increased to accommodate 3 rows of icons with text
#define OLED_WIDTH 256
#define OLED_HEIGHT 32

// Window scaling for better visibility
#define SCALE_FACTOR 3

class DesktopDisplay {
public:
    DesktopDisplay();
    ~DesktopDisplay();
    
    bool init();
    void cleanup();
    
    // E-Ink display methods
    void einkClear();
    void einkSetPixel(int x, int y, bool black = true);
    void einkDrawText(const std::string& text, int x, int y, int size = 12);
    void einkDrawLine(int x0, int y0, int x1, int y1, bool black = true);
    void einkDrawRect(int x, int y, int w, int h, bool filled = false, bool black = true);
    void einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black = true);
    void einkDrawCircle(int x, int y, int r, bool filled = false, bool black = true);
    void einkGetTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    void einkRefresh();
    void einkPartialRefresh();
    
    // OLED display methods
    void oledClear();
    void oledClearRect(int x, int y, int w, int h);
    void oledSetPixel(int x, int y, bool on = true);
    void oledDrawText(const std::string& text, int x, int y, int size = 8);
    void oledDrawLine(int x0, int y0, int x1, int y1, bool on = true);
    void oledDrawRect(int x, int y, int w, int h, bool filled = false, bool on = true);
    void oledUpdate();
    
    // Input handling
    bool handleEvents();
    char getLastKey();
    bool isKeyPressed(SDL_Scancode key);
    
    // Utility
    void present();
    
private:
    SDL_Window* einkWindow;
    SDL_Window* oledWindow;
    SDL_Renderer* einkRenderer;
    SDL_Renderer* oledRenderer;
    SDL_Texture* einkTexture;
    SDL_Texture* oledTexture;
    TTF_Font* font;
    TTF_Font* smallFont;
    
    // Display buffers
    std::vector<uint8_t> einkBuffer;
    std::vector<uint8_t> oledBuffer;
    
    // Input state
    char lastKey;
    bool keyPressed[SDL_NUM_SCANCODES];
    
    void updateEinkTexture();
    void updateOledTexture();
    SDL_Color getEinkColor(bool black);
    SDL_Color getOledColor(bool on);
};

// Global display instance
extern DesktopDisplay* g_display;

#endif // DESKTOP_DISPLAY_H
