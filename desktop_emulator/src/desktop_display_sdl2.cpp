#include "desktop_display_sdl2.h"
#include "pocketmage_compat.h"
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <chrono>

namespace {
    struct EInkSimConfig {
        bool  enabled          = false;
        bool  flash_full       = true;
        bool  flash_partial    = false;
        int   full_ms          = 0;
        int   partial_ms       = 0;
        float ghosting_full    = 0.02f;
        float ghosting_partial = 0.08f;
        float full_threshold   = 0.22f;
        int   wipe_step_px     = 18;
        int   diff_threshold   = 24;
    };
    static EInkSimConfig g_einkSimCfg;
    static bool g_einkSimEnabled = false;
    static bool g_einkSimForceFull = false;
}

// Global instance
DesktopDisplay* g_display = nullptr;

DesktopDisplay::DesktopDisplay() 
    : einkWindow(nullptr), oledWindow(nullptr), einkRenderer(nullptr), oledRenderer(nullptr),
      einkTexture(nullptr), oledTexture(nullptr), font(nullptr), smallFont(nullptr),
      lastKey(0), hasUTF8InputData(false) {
    memset(keyPressed, false, sizeof(keyPressed));
    
    const char* _einkSim = std::getenv("POCKETMAGE_EINK_SIM");
    if (_einkSim && _einkSim[0] && _einkSim[0] != '0') {
        g_einkSimEnabled = true;
        std::cout << "[EINK SIM] Enabled via POCKETMAGE_EINK_SIM" << std::endl;
    }
}

DesktopDisplay::~DesktopDisplay() {
    cleanup();
}

bool DesktopDisplay::init() {
    DEBUG_LOG("SDL2", "Initializing DesktopDisplay...");
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "0");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[SDL2] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_StartTextInput();

    if (TTF_Init() == -1) {
        std::cerr << "[SDL2] TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    einkWindow = SDL_CreateWindow("PocketMage E-Ink Display",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  EINK_WIDTH * EINK_WINDOW_SCALE, EINK_HEIGHT * EINK_WINDOW_SCALE,
                                  SDL_WINDOW_SHOWN);
    if (!einkWindow) {
        std::cerr << "[SDL2] Failed to create E-Ink window: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    einkRenderer = SDL_CreateRenderer(einkWindow, -1, SDL_RENDERER_SOFTWARE);
    if (!einkRenderer) {
        std::cerr << "[SDL2] Failed to create E-Ink software renderer: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(einkRenderer, &info) == 0) {
        std::cout << "[SDL2] E-Ink renderer: " << info.name << " (flags: " << info.flags << ")" << std::endl;
    }
    
    SDL_RenderSetLogicalSize(einkRenderer, EINK_WIDTH, EINK_HEIGHT);
    SDL_RenderSetIntegerScale(einkRenderer, SDL_TRUE);
    oledWindow = SDL_CreateWindow("PocketMage OLED Display",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED + (EINK_HEIGHT * EINK_WINDOW_SCALE) + 50,
                                  OLED_WIDTH * OLED_WINDOW_SCALE, OLED_HEIGHT * OLED_WINDOW_SCALE,
                                  SDL_WINDOW_SHOWN);
    if (!oledWindow) {
        std::cerr << "[SDL2] Failed to create OLED window: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    oledRenderer = SDL_CreateRenderer(oledWindow, -1, SDL_RENDERER_SOFTWARE);
    if (!oledRenderer) {
        std::cerr << "[SDL2] Failed to create OLED software renderer: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    SDL_RendererInfo oledInfo;
    if (SDL_GetRendererInfo(oledRenderer, &oledInfo) == 0) {
        std::cout << "[SDL2] OLED renderer: " << oledInfo.name << " (flags: " << oledInfo.flags << ")" << std::endl;
    }
    
    SDL_RenderSetLogicalSize(oledRenderer, OLED_WIDTH, OLED_HEIGHT);
    SDL_RenderSetIntegerScale(oledRenderer, SDL_TRUE);
    einkTexture = SDL_CreateTexture(einkRenderer, SDL_PIXELFORMAT_ARGB8888, 
                                    SDL_TEXTUREACCESS_STREAMING, EINK_WIDTH, EINK_HEIGHT);
    if (!einkTexture) {
        std::cerr << "[SDL2] Failed to create E-Ink texture: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    oledTexture = SDL_CreateTexture(oledRenderer, SDL_PIXELFORMAT_ARGB8888, 
                                    SDL_TEXTUREACCESS_STREAMING, OLED_WIDTH, OLED_HEIGHT);
    if (!oledTexture) {
        std::cerr << "[SDL2] Failed to create OLED texture: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    // Initialize framebuffers
    einkBuffer.assign(EINK_WIDTH * EINK_HEIGHT, 255); // White background for E‑Ink
    oledBuffer.assign(OLED_WIDTH * OLED_HEIGHT, 0);   // Black background for OLED

    // Load fonts
    const char* fontPaths[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/arial.ttf"
    };
    
    for (const char* path : fontPaths) {
        font = TTF_OpenFont(path, 12);
        if (font) {
            std::cout << "[SDL2] Loaded font: " << path << std::endl;
            break;
        }
    }
    
    for (const char* path : fontPaths) {
        smallFont = TTF_OpenFont(path, 8);
        if (smallFont) break;
    }
    
    if (!font) {
        std::cerr << "[SDL2] Warning: Could not load any font" << std::endl;
    }

    DEBUG_LOG("SDL2", "DesktopDisplay initialized successfully");
    
    return true;
}

void DesktopDisplay::cleanup() {
    DEBUG_LOG("SDL2", "Cleaning up DesktopDisplay...");
    
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    if (smallFont) {
        TTF_CloseFont(smallFont);
        smallFont = nullptr;
    }
    
    if (einkTexture) {
        SDL_DestroyTexture(einkTexture);
        einkTexture = nullptr;
    }
    if (oledTexture) {
        SDL_DestroyTexture(oledTexture);
        oledTexture = nullptr;
    }
    
    if (einkRenderer) {
        SDL_DestroyRenderer(einkRenderer);
        einkRenderer = nullptr;
    }
    if (oledRenderer) {
        SDL_DestroyRenderer(oledRenderer);
        oledRenderer = nullptr;
    }
    
    if (einkWindow) {
        SDL_DestroyWindow(einkWindow);
        einkWindow = nullptr;
    }
    if (oledWindow) {
        SDL_DestroyWindow(oledWindow);
        oledWindow = nullptr;
    }
    
    TTF_Quit();
    SDL_Quit();
    
    DEBUG_LOG("SDL2", "DesktopDisplay cleanup complete");
}

// E-Ink display methods
void DesktopDisplay::einkClear() {
    std::fill(einkBuffer.begin(), einkBuffer.end(), 255);
    // Force immediate refresh to clear artifacts when transitioning between apps
    updateEinkTexture();
}

void DesktopDisplay::einkSetPixel(int x, int y, bool black) {
    if (x >= 0 && x < EINK_WIDTH && y >= 0 && y < EINK_HEIGHT) {
        // For E-Ink: 0 = black, 255 = white
        einkBuffer[y * EINK_WIDTH + x] = black ? 0 : 255;
    }
}

void DesktopDisplay::einkDrawText(const std::string& text, int x, int y, int size, bool whiteText) {
    if (text.empty() || !font) return;
    
    // std::cout << "[SDL2] Drawing E-Ink text: '" << text << "' at (" << x << "," << y << ") size=" << size << " white=" << whiteText << std::endl;
    
    // Use SDL_ttf to render text with UTF-8 encoding
    SDL_Color textColor = whiteText ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Solid(font, text.c_str(), textColor);
    
    if (!textSurface) {
        std::cerr << "[SDL2] Failed to render text surface: " << TTF_GetError() << std::endl;
        return;
    }
    
    // Convert surface to our framebuffer format and copy pixels
    SDL_LockSurface(textSurface);
    
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    
    // Copy pixels from text surface to our framebuffer
    for (int dy = 0; dy < textHeight && (y + dy) < EINK_HEIGHT; dy++) {
        for (int dx = 0; dx < textWidth && (x + dx) < EINK_WIDTH; dx++) {
            if (x + dx >= 0 && y + dy >= 0) {
                // Get pixel from text surface
                Uint8* pixelPtr = (Uint8*)textSurface->pixels + (dy * textSurface->pitch) + (dx * textSurface->format->BytesPerPixel);
                
                Uint8 intensity = 0;
                if (textSurface->format->BytesPerPixel == 1) {
                    // 8-bit grayscale
                    intensity = *pixelPtr;
                } else if (textSurface->format->BytesPerPixel >= 3) {
                    // RGB format - use red component
                    intensity = pixelPtr[0];
                }
                
                // Write to framebuffer (0 = black, 255 = white)
                int fbIndex = (y + dy) * EINK_WIDTH + (x + dx);
                if (fbIndex >= 0 && fbIndex < (int)einkBuffer.size()) {
                    if (whiteText) {
                        // For white text, write white pixels where text exists
                        if (intensity > 0) {
                            einkBuffer[fbIndex] = 255; // White text
                        }
                    } else {
                        // For black text, write black pixels where text exists
                        if (intensity > 0) {
                            einkBuffer[fbIndex] = 0; // Black text
                        }
                    }
                }
            }
        }
    }
    
    SDL_UnlockSurface(textSurface);
    if (textSurface) {
        SDL_FreeSurface(textSurface);
        textSurface = nullptr;
    }
    
    // std::cout << "[SDL2] E-Ink text drawn with SDL_ttf" << std::endl;
    
    // No auto-present; caller will present once per frame
}

void DesktopDisplay::einkDrawLine(int x0, int y0, int x1, int y1, bool black) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        einkSetPixel(x0, y0, black);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void DesktopDisplay::einkDrawRect(int x, int y, int w, int h, bool filled, bool black) {
    if (filled) {
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                einkSetPixel(x + dx, y + dy, black);
            }
        }
    } else {
        // Draw outline
        einkDrawLine(x, y, x + w - 1, y, black);
        einkDrawLine(x, y + h - 1, x + w - 1, y + h - 1, black);
        einkDrawLine(x, y, x, y + h - 1, black);
        einkDrawLine(x + w - 1, y, x + w - 1, y + h - 1, black);
    }
}

void DesktopDisplay::einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black) {
    // Simple bitmap drawing - assume 1 bit per pixel
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int byteIndex = (dy * ((w + 7) / 8)) + (dx / 8);
            int bitIndex = 7 - (dx % 8);
            bool pixelSet = (bitmap[byteIndex] >> bitIndex) & 1;
            
            if (pixelSet) {
                // Draw black pixels when bitmap bit is set and black=true
                einkSetPixel(x + dx, y + dy, black);
            } else if (!black) {
                // Draw white pixels when bitmap bit is not set and black=false
                einkSetPixel(x + dx, y + dy, false);
            }
        }
    }
    // No auto-present; caller will present once per frame
}

void DesktopDisplay::einkDrawCircle(int x, int y, int r, bool filled, bool black) {
    if (filled) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r) {
                    einkSetPixel(x + dx, y + dy, black);
                }
            }
        }
    } else {
        // Bresenham's circle algorithm
        int dx = 0;
        int dy = r;
        int d = 3 - 2 * r;
        
        while (dy >= dx) {
            einkSetPixel(x + dx, y + dy, black);
            einkSetPixel(x - dx, y + dy, black);
            einkSetPixel(x + dx, y - dy, black);
            einkSetPixel(x - dx, y - dy, black);
            einkSetPixel(x + dy, y + dx, black);
            einkSetPixel(x - dy, y + dx, black);
            einkSetPixel(x + dy, y - dx, black);
            einkSetPixel(x - dy, y - dx, black);
            
            dx++;
            if (d > 0) {
                dy--;
                d = d + 4 * (dx - dy) + 10;
            } else {
                d = d + 4 * dx + 6;
            }
        }
    }
}

void DesktopDisplay::einkGetTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    if (!text) return;
    
    int len = strlen(text);
    *x1 = x;
    *y1 = y;
    *w = len * 6; // Approximate character width
    *h = 12; // Approximate character height
}

void DesktopDisplay::einkRefresh() { updateEinkTexture(); }

void DesktopDisplay::einkPartialRefresh() { updateEinkTexture(); }

void DesktopDisplay::einkForceFullRefresh() {
    if (!einkTexture || !einkRenderer) return;
    
    void* pixels; int pitch;
    if (SDL_LockTexture(einkTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock E-Ink texture: " << SDL_GetError() << std::endl;
        return;
    }
    auto* dst = static_cast<uint32_t*>(pixels);
    const int stride = pitch / 4;
    for (int y = 0; y < EINK_HEIGHT; ++y) {
        const uint8_t* src = &einkBuffer[y * EINK_WIDTH];
        uint32_t* out = &dst[y * stride];
        for (int x = 0; x < EINK_WIDTH; ++x) {
            uint8_t g = src[x];
            out[x] = 0xFF000000u | (g << 16) | (g << 8) | g;
        }
    }
    SDL_UnlockTexture(einkTexture);
    
    // Ensure viewport is properly set for software renderer
    SDL_Rect viewport = {0, 0, EINK_WIDTH, EINK_HEIGHT};
    SDL_RenderSetViewport(einkRenderer, &viewport);
    
    // Clear renderer first to prevent artifacts
    SDL_SetRenderDrawColor(einkRenderer, 255, 255, 255, 255);
    SDL_RenderClear(einkRenderer);
    SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr);
    SDL_RenderPresent(einkRenderer);
}

// OLED display methods
void DesktopDisplay::oledClear() {
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);
    // buffer-only; caller will present once
}

void DesktopDisplay::oledClearRect(int x, int y, int w, int h) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < OLED_WIDTH && py >= 0 && py < OLED_HEIGHT) {
                int index = py * OLED_WIDTH + px;
                if (index >= 0 && index < oledBuffer.size()) {
                    oledBuffer[index] = 0; // Black
                }
            }
        }
    }
    // buffer-only
}

void DesktopDisplay::oledSetPixel(int x, int y, bool on) {
    if (x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT) {
        int index = y * OLED_WIDTH + x;
        if (index >= 0 && index < oledBuffer.size()) {
            oledBuffer[index] = on ? 255 : 0;
        }
    }
}

void DesktopDisplay::oledDrawText(const std::string& text, int x, int y, int size) {
    if (!font || text.empty()) return;

    // No need to clear here - renderOledText() already clears the whole buffer

    SDL_Color fg = {255,255,255,255};
    SDL_Color bg = {0,0,0,255};
    SDL_Surface* s = TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg);
    if (!s) { std::cerr << "[OLED] TTF_RenderUTF8_Shaded failed: " << TTF_GetError() << "\n"; return; }

    SDL_LockSurface(s);
    const int bpp = s->format->BytesPerPixel;
    for (int dy = 0; dy < s->h; ++dy) {
        int py = y + dy - (s->h - 1);
        if (py < 0 || py >= OLED_HEIGHT) continue;
        for (int dx = 0; dx < s->w; ++dx) {
            int px = x + dx;
            if (px < 0 || px >= OLED_WIDTH) continue;

            const Uint8* p = (Uint8*)s->pixels + dy * s->pitch + dx * bpp;
            Uint8 intensity = (bpp >= 3) ? p[0] : *p;   // use R channel
            if (intensity > 128) {
                oledBuffer[py * OLED_WIDTH + px] = 255; // ON
            }
        }
    }
    SDL_UnlockSurface(s);
    if (s) {
        SDL_FreeSurface(s);
        s = nullptr;
    }

    // REMOVE: updateOledTexture();
}

void DesktopDisplay::oledDrawLine(int x0, int y0, int x1, int y1, bool on) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        oledSetPixel(x0, y0, on);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void DesktopDisplay::oledDrawRect(int x, int y, int w, int h, bool filled, bool on) {
    if (filled) {
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                oledSetPixel(x + dx, y + dy, on);
            }
        }
    } else {
        // Draw outline
        oledDrawLine(x, y, x + w - 1, y, on);
        oledDrawLine(x, y + h - 1, x + w - 1, y + h - 1, on);
        oledDrawLine(x, y, x, y + h - 1, on);
        oledDrawLine(x + w - 1, y, x + w - 1, y + h - 1, on);
    }
}

void DesktopDisplay::oledUpdate() { updateOledTexture(); }

// UTF-8 text input handling
bool DesktopDisplay::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        } else if (e.type == SDL_TEXTINPUT) {
            // Skip SDL text input - use keyboard layout system instead
            std::cout << "[UTF8] Skipping SDL text input: '" << e.text.text << "' - using layout system" << std::endl;
        } else if (e.type == SDL_KEYDOWN) {
            // Check for modifier keys and special combinations
            SDL_Keymod modifiers = SDL_GetModState();
            bool ctrlPressed = (modifiers & KMOD_CTRL); // Use Ctrl as Fn key on desktop
            
            // Handle Ctrl+K combination for keyboard layout switching
            if (ctrlPressed && e.key.keysym.sym == SDLK_k) {
                lastKey = -1; // Special code for Ctrl+K (Fn+K equivalent)
                std::cout << "[KEYBOARD] Ctrl+K detected for layout switching" << std::endl;
                return true;
            }
            
            // Map all keys through PocketMage keyboard layout system
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    lastKey = 19;  // ASCII 19 - UP arrow for PocketMage
                    // std::cout << "[KEYBOARD] Mapped to UP arrow" << std::endl;
                    break;
                case SDLK_DOWN:
                    lastKey = 21;  // ASCII 21 - DOWN arrow for PocketMage
                    // std::cout << "[KEYBOARD] Mapped to DOWN arrow" << std::endl;
                    break;
                case SDLK_LEFT:
                    lastKey = 20;  // ASCII 20 - LEFT arrow for PocketMage
                    // std::cout << "[KEYBOARD] Mapped to LEFT arrow" << std::endl;
                    break;
                case SDLK_RIGHT:
                    lastKey = 18;  // ASCII 18 - RIGHT arrow for PocketMage (matches UTF8 handler)
                    // std::cout << "[KEYBOARD] Mapped to RIGHT arrow" << std::endl;
                    break;
                case SDLK_ESCAPE:
                    lastKey = 27;  // ASCII 27 - ESC key
                    // std::cout << "[KEYBOARD] Mapped to ESC" << std::endl;
                    break;
                case SDLK_HOME:
                    lastKey = 12;  // ASCII 12 - HOME key
                    // std::cout << "[KEYBOARD] Mapped to HOME" << std::endl;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    lastKey = 13;  // ASCII 13 - Enter
                    // std::cout << "[KEYBOARD] Mapped to ENTER" << std::endl;
                    break;
                case SDLK_BACKSPACE:
                    lastKey = 8;   // ASCII 8 - Backspace
                    // std::cout << "[KEYBOARD] Mapped to BACKSPACE" << std::endl;
                    break;
                case SDLK_TAB:
                    lastKey = 9;   // ASCII 9 - Tab
                    // std::cout << "[KEYBOARD] Mapped to TAB" << std::endl;
                    break;
                case SDLK_SPACE:
                    lastKey = 32;  // ASCII 32 - Space
                    // std::cout << "[KEYBOARD] Mapped to SPACE" << std::endl;
                    break;
                case SDLK_F5:
                    g_einkSimEnabled = !g_einkSimEnabled;
                    std::cout << "[EINK SIM] " << (g_einkSimEnabled ? "ON" : "OFF") << std::endl;
                    break;

                case SDLK_F6:
                    g_einkSimForceFull = true;
                    std::cout << "[EINK SIM] Force full refresh on next frame" << std::endl;
                    break;

                case SDLK_F7: {
                    float gp = g_einkSimCfg.ghosting_partial + 0.04f;
                    if (gp > 0.12f) gp = 0.0f;
                    g_einkSimCfg.ghosting_partial = gp;
                    std::cout << "[EINK SIM] Partial ghosting = " << gp << std::endl;
                    break;
                }

                case SDLK_F8:
                    g_einkSimCfg.wipe_step_px = (g_einkSimCfg.wipe_step_px == 18 ? 10 : 18);
                    std::cout << "[EINK SIM] wipe_step_px = " << g_einkSimCfg.wipe_step_px << std::endl;
                    break;

                // Map apostrophe key to trigger dead key processing
                case SDLK_QUOTE:
                    lastKey = 200; // Special code for apostrophe dead key (avoid conflict with 'd'=100)
                    std::cout << "[KEYBOARD] Apostrophe key detected for dead key processing" << std::endl;
                    break;
                default:
                    // Map regular character keys through layout system
                    if (e.key.keysym.sym >= 32 && e.key.keysym.sym <= 126) {
                        lastKey = e.key.keysym.sym;
                        // std::cout << "[KEYBOARD] Character key: " << (char)lastKey << " (" << lastKey << ")" << std::endl;
                    } else {
                        lastKey = e.key.keysym.sym;
                        std::cout << "[SDL2] Special key pressed: " << lastKey << std::endl;
                    }
                    break;
            }
            keyPressed[e.key.keysym.scancode] = true;
        } else if (e.type == SDL_KEYUP) {
            keyPressed[e.key.keysym.scancode] = false;
        }
    }
    return true;
}

char DesktopDisplay::getLastKey() {
    char key = lastKey;
    lastKey = 0;
    return key;
}

bool DesktopDisplay::isKeyPressed(SDL_Scancode key) {
    return keyPressed[key];
}

// UTF-8 text input methods
bool DesktopDisplay::hasUTF8Input() {
    return hasUTF8InputData;
}

std::string DesktopDisplay::getUTF8Input() {
    if (hasUTF8InputData) {
        std::string result = utf8InputBuffer;
        utf8InputBuffer.clear();
        hasUTF8InputData = false;
        return result;
    }
    return "";
}

// Utility
void DesktopDisplay::present() {
    updateEinkTexture();
    // OLED is presented by OledService on main thread, not here
}

SDL_Color DesktopDisplay::getEinkColor(bool black) {
    return black ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255};
}

SDL_Color DesktopDisplay::getOledColor(bool on) {
    return on ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
}

void DesktopDisplay::updateEinkTexture() {
    if (!einkTexture || !einkRenderer) return;

    // Row‑dirty upload (fast path, works for both sim on/off)
    static std::vector<uint8_t> prevBuf; 
    const size_t N = size_t(EINK_WIDTH) * size_t(EINK_HEIGHT);
    if (prevBuf.size() != N) prevBuf.assign(N, 0); // Initialize to black so first clear is detected

    // Find changed rows
    std::vector<int> dirtyRows;
    dirtyRows.reserve(EINK_HEIGHT);
    for (int y = 0; y < EINK_HEIGHT; ++y) {
        const uint8_t* r  = &einkBuffer[y * EINK_WIDTH];
        const uint8_t* pr = &prevBuf[y * EINK_WIDTH];
        if (std::memcmp(r, pr, EINK_WIDTH) != 0) dirtyRows.push_back(y);
    }
    if (dirtyRows.empty()) return;

    // Upload only dirty rows (respect pitch)
    void* pixels; int pitch;
    if (SDL_LockTexture(einkTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock E-Ink texture: " << SDL_GetError() << std::endl;
        return;
    }
    auto* dst = static_cast<uint32_t*>(pixels);
    const int stride = pitch / 4;
    for (int y : dirtyRows) {
        const uint8_t* src = &einkBuffer[y * EINK_WIDTH];
        uint32_t* out = &dst[y * stride];
        for (int x = 0; x < EINK_WIDTH; ++x) {
            uint8_t g = src[x];
            out[x] = 0xFF000000u | (g << 16) | (g << 8) | g;
        }
        std::memcpy(&prevBuf[y * EINK_WIDTH], src, EINK_WIDTH);
    }
    SDL_UnlockTexture(einkTexture);
    
    // Ensure viewport is properly set for software renderer
    SDL_Rect viewport = {0, 0, EINK_WIDTH, EINK_HEIGHT};
    SDL_RenderSetViewport(einkRenderer, &viewport);
    
    // Clear renderer to prevent artifacts
    SDL_SetRenderDrawColor(einkRenderer, 255, 255, 255, 255);
    SDL_RenderClear(einkRenderer);
    SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr);
    SDL_RenderPresent(einkRenderer);
}

void DesktopDisplay::updateOledTexture() {
    // Single, pitch-safe upload + present
    if (!oledTexture || !oledRenderer) return;
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(oledTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[OLED] Failed to lock texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    auto* texturePixels = static_cast<uint32_t*>(pixels);
    const int stride = pitch / 4;
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        const uint8_t* row = &oledBuffer[y * OLED_WIDTH];
        uint32_t* dst = &texturePixels[y * stride];
        for (int x = 0; x < OLED_WIDTH; ++x) {
            dst[x] = row[x] ? 0xFFFFFFFFu : 0xFF000000u;
        }
    }
    
    SDL_UnlockTexture(oledTexture);
    
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}

void DesktopDisplay::renderOledText(const std::string& l1,
                                    const std::string& l2,
                                    const std::string& l3) {
    if (!oledTexture || !oledRenderer) return;

    // Clear whole OLED buffer once
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);

    // Draw text with better positioning to avoid top cutoff (8px font)
    if (!l1.empty()) oledDrawText(l1, 0, 12, 8);
    if (!l2.empty()) oledDrawText(l2, 0, 20, 8);
    if (!l3.empty()) oledDrawText(l3, 0, 28, 8);

    // Present once
    updateOledTexture();
}

