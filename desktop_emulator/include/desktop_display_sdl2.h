#ifndef DESKTOP_DISPLAY_SDL2_H
#define DESKTOP_DISPLAY_SDL2_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <cstdint>

// ---- Logical device sizes (scale only at the window/renderer level) ----
constexpr int EINK_WIDTH  = 320;   // Set this to your actual E-Ink width
constexpr int EINK_HEIGHT = 240;   // Set this to your actual E-Ink height
constexpr int OLED_WIDTH  = 128;   // SH1106 logical width
constexpr int OLED_HEIGHT = 32;    // SH1106 logical height

// Window-only scale (do not change logical sizes above)
constexpr int EINK_WINDOW_SCALE = 3;
constexpr int OLED_WINDOW_SCALE = 2;

class DesktopDisplay {
public:
    DesktopDisplay();
    ~DesktopDisplay();
    
    bool init();
    void cleanup();
    
    // E‑Ink API
    void einkClear();
    void einkSetPixel(int x, int y, bool black);
    void einkDrawText(const std::string& text, int x, int y, int size, bool whiteText=false);
    void einkDrawLine(int x0, int y0, int x1, int y1, bool black=true);
    void einkDrawRect(int x, int y, int w, int h, bool filled=false, bool black=true);
    void einkDrawCircle(int x, int y, int r, bool filled=false, bool black=true);
    void einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black=true);
    void einkGetTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    void einkRefresh();
    void einkPartialRefresh();
    void einkForceFullRefresh(); // Force complete screen update

    // OLED API
    void oledClear();
    void oledClearRect(int x, int y, int w, int h);
    void oledSetPixel(int x, int y, bool on);
    void oledDrawText(const std::string& text, int x, int y, int size=8);
    void oledDrawLine(int x0, int y0, int x1, int y1, bool on=true);
    void oledDrawRect(int x, int y, int w, int h, bool filled=false, bool on=true);
    void renderOledText(const std::string& l1, const std::string& l2, const std::string& l3);
    void oledUpdate();

    // Event/present
    bool handleEvents();
    char getLastKey();
    bool isKeyPressed(SDL_Scancode key);
    bool hasUTF8Input();
    std::string getUTF8Input();
    void present();
    
private:
    // Windows & renderers
    SDL_Window*   einkWindow   = nullptr;
    SDL_Renderer* einkRenderer = nullptr;
    SDL_Texture*  einkTexture  = nullptr;

    SDL_Window*   oledWindow   = nullptr;
    SDL_Renderer* oledRenderer = nullptr;
    SDL_Texture*  oledTexture  = nullptr;

    // Fonts
    TTF_Font*     font         = nullptr;
    TTF_Font*     smallFont    = nullptr;

    // Input
    char lastKey;
    bool keyPressed[SDL_NUM_SCANCODES];
    bool hasUTF8InputData;
    std::string utf8InputBuffer;

    // E‑Ink buffer (8-bit grayscale today; row‑dirty uploads)
    std::vector<uint8_t> einkBuffer;

    // OLED buffer (1 byte per pixel, 0/255)
    std::vector<uint8_t> oledBuffer;

    void updateEinkTexture();
    void updateOledTexture();

    // utils
    SDL_Color getEinkColor(bool black);
    SDL_Color getOledColor(bool on);
};

extern DesktopDisplay* g_display;

#endif // DESKTOP_DISPLAY_SDL2_H
