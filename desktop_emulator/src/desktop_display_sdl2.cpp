#include "desktop_display_sdl2.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>

// Global instance
DesktopDisplay* g_display = nullptr;

DesktopDisplay::DesktopDisplay() 
    : einkWindow(nullptr), oledWindow(nullptr), einkRenderer(nullptr), oledRenderer(nullptr),
      einkTexture(nullptr), oledTexture(nullptr), font(nullptr), smallFont(nullptr),
      lastKey(0) {
    memset(keyPressed, false, sizeof(keyPressed));
}

DesktopDisplay::~DesktopDisplay() {
    cleanup();
}

bool DesktopDisplay::init() {
    std::cout << "[SDL2] Initializing DesktopDisplay..." << std::endl;
    
    // Force software rendering and disable Metal completely
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "0");
    SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "0");
    SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "0");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
    SDL_SetHint("SDL_VIDEODRIVER", "cocoa");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint("SDL_METAL_PREFER_LOW_POWER_DEVICE", "0");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[SDL2] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "[SDL2] TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Create E-Ink window
    einkWindow = SDL_CreateWindow("PocketMage E-Ink Display",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  EINK_WIDTH * SCALE_FACTOR, EINK_HEIGHT * SCALE_FACTOR,
                                  SDL_WINDOW_SHOWN);
    if (!einkWindow) {
        std::cerr << "[SDL2] Failed to create E-Ink window: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    // Create renderer with explicit software flag and no acceleration
    einkRenderer = SDL_CreateRenderer(einkWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!einkRenderer) {
        std::cerr << "[SDL2] Failed to create E-Ink renderer: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    // Set viewport immediately after creating renderer
    SDL_Rect einkViewport = {0, 0, EINK_WIDTH * 3, EINK_HEIGHT * 3};
    SDL_RenderSetViewport(einkRenderer, &einkViewport);

    // Create OLED window
    oledWindow = SDL_CreateWindow("PocketMage OLED Display",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED + (EINK_HEIGHT * SCALE_FACTOR) + 50,
                                  OLED_WIDTH * SCALE_FACTOR, OLED_HEIGHT * SCALE_FACTOR,
                                  SDL_WINDOW_SHOWN);
    if (!oledWindow) {
        std::cerr << "[SDL2] Failed to create OLED window: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    // Create renderer with explicit software flag and no acceleration
    oledRenderer = SDL_CreateRenderer(oledWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!oledRenderer) {
        std::cerr << "[SDL2] Failed to create OLED renderer: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    // Set viewport immediately after creating renderer
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH * 3, OLED_HEIGHT * 3};
    SDL_RenderSetViewport(oledRenderer, &oledViewport);

    // Create textures - try ARGB8888 format instead
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
    einkBuffer.resize(EINK_WIDTH * EINK_HEIGHT, 255); // White background for E-Ink
    oledBuffer.resize(OLED_WIDTH * OLED_HEIGHT, 0);   // Black background for OLED

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

    std::cout << "[SDL2] DesktopDisplay initialized successfully" << std::endl;
    
    return true;
}

void DesktopDisplay::cleanup() {
    std::cout << "[SDL2] Cleaning up DesktopDisplay..." << std::endl;
    
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
    
    std::cout << "[SDL2] DesktopDisplay cleanup complete" << std::endl;
}

// E-Ink display methods
void DesktopDisplay::einkClear() {
    std::fill(einkBuffer.begin(), einkBuffer.end(), 255);
    std::cout << "[SDL2] E-Ink cleared to white" << std::endl;
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
    SDL_FreeSurface(textSurface);
    
    // std::cout << "[SDL2] E-Ink text drawn with SDL_ttf" << std::endl;
    
    // Auto-update display after drawing text
    updateEinkTexture();
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
    // Auto-update display after drawing bitmap
    updateEinkTexture();
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

void DesktopDisplay::einkRefresh() {
    std::cout << "[SDL2] E-Ink refresh - updating display" << std::endl;
    updateEinkTexture();
}

void DesktopDisplay::einkPartialRefresh() {
    std::cout << "[SDL2] E-Ink partial refresh - updating display" << std::endl;
    updateEinkTexture();
}

// OLED display methods
void DesktopDisplay::oledClear() {
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);
    std::cout << "[SDL2] OLED cleared to black" << std::endl;
    updateOledTexture();
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
    updateOledTexture();
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
    if (!font) {
        std::cout << "[OLED] ERROR: Font not loaded!" << std::endl;
        return;
    }
    
    std::cout << "[OLED] Font loaded, rendering text: " << text << std::endl;
    
    // std::cout << "[SDL2] Drawing OLED text: '" << text << "' at (" << x << "," << y << ") size=" << size << std::endl;
    
    // Clear the text area first (estimate text width/height)
    int estimatedWidth = text.length() * (size + 2);
    int estimatedHeight = size + 4;
    oledClearRect(x, y, estimatedWidth, estimatedHeight);
    
    if (text.empty()) return;
    
    // Use SDL_ttf to render text with UTF-8 encoding
    SDL_Color textColor = {255, 255, 255, 255}; // White text
    SDL_Color bgColor = {0, 0, 0, 255}; // Black background
    SDL_Surface* textSurface = TTF_RenderUTF8_Shaded(font, text.c_str(), textColor, bgColor);
    
    if (!textSurface) {
        std::cerr << "[SDL2] Failed to render OLED text surface: " << TTF_GetError() << std::endl;
        return;
    }
    
    std::cout << "[OLED] Text surface created: " << textSurface->w << "x" << textSurface->h << " format=" << textSurface->format->format << std::endl;
    
    // Convert surface to our framebuffer format and copy pixels
    SDL_LockSurface(textSurface);
    
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    
    // Copy pixels from text surface to our framebuffer
    for (int dy = 0; dy < textHeight && (y + dy) < OLED_HEIGHT; dy++) {
        for (int dx = 0; dx < textWidth && (x + dx) < OLED_WIDTH; dx++) {
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
                } else if (textSurface->format->BytesPerPixel == 4) {
                    // RGBA format - use red component
                    intensity = pixelPtr[0];
                }
                
                // Write to framebuffer (0 = black, 255 = white)
                int fbIndex = (y + dy) * OLED_WIDTH + (x + dx);
                if (fbIndex >= 0 && fbIndex < (int)oledBuffer.size()) {
                    // For white text on black background - only draw bright pixels
                    if (intensity > 128) {
                        oledBuffer[fbIndex] = 1; // Set pixel on
                        if (dx < 5 && dy < 5) { // Debug first few pixels
                            std::cout << "[OLED] Pixel (" << (x+dx) << "," << (y+dy) << ") intensity=" << (int)intensity << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    SDL_UnlockSurface(textSurface);
    SDL_FreeSurface(textSurface);
    
    // std::cout << "[SDL2] OLED text drawn with SDL_ttf" << std::endl;
    
    // Auto-update display after drawing text
    updateOledTexture();
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

void DesktopDisplay::oledUpdate() {
    std::cout << "[SDL2] OLED update - updating display" << std::endl;
    updateOledTexture();
}

// Input handling
bool DesktopDisplay::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        } else if (e.type == SDL_KEYDOWN) {
            // Map special keys for PocketMage compatibility
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    lastKey = 19;  // ASCII 19 - UP arrow for PocketMage
                    std::cout << "[KEYBOARD] Mapped to UP arrow" << std::endl;
                    break;
                case SDLK_DOWN:
                    lastKey = 21;  // ASCII 21 - DOWN arrow for PocketMage
                    std::cout << "[KEYBOARD] Mapped to DOWN arrow" << std::endl;
                    break;
                case SDLK_LEFT:
                    lastKey = 20;  // ASCII 20 - LEFT arrow for PocketMage
                    std::cout << "[KEYBOARD] Mapped to LEFT arrow" << std::endl;
                    break;
                case SDLK_RIGHT:
                    lastKey = 18;  // ASCII 18 - RIGHT arrow for PocketMage
                    std::cout << "[KEYBOARD] Mapped to RIGHT arrow" << std::endl;
                    break;
                case SDLK_ESCAPE:
                    lastKey = 27;  // ASCII 27 - ESC key
                    std::cout << "[KEYBOARD] Mapped to ESC" << std::endl;
                    break;
                case SDLK_HOME:
                    lastKey = 12;  // ASCII 12 - HOME key
                    std::cout << "[KEYBOARD] Mapped to HOME" << std::endl;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    lastKey = 13;  // ASCII 13 - Enter
                    std::cout << "[KEYBOARD] Mapped to ENTER" << std::endl;
                    break;
                case SDLK_BACKSPACE:
                    lastKey = 8;   // ASCII 8 - Backspace
                    std::cout << "[KEYBOARD] Mapped to BACKSPACE" << std::endl;
                    break;
                default:
                    lastKey = e.key.keysym.sym;
                    std::cout << "[SDL2] Key pressed: " << (char)lastKey << std::endl;
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

// Utility
void DesktopDisplay::present() {
    updateEinkTexture();
    // OLED updates now handled by thread-safe OLED service
}

SDL_Color DesktopDisplay::getEinkColor(bool black) {
    return black ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255};
}

SDL_Color DesktopDisplay::getOledColor(bool on) {
    return on ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
}

void DesktopDisplay::updateEinkTexture() {
    if (!einkTexture || !einkRenderer) return;
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(einkTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock E-Ink texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint32_t* dst = (uint32_t*)pixels;
    const uint8_t* src = einkBuffer.data();
    
    // Convert grayscale to RGBA (ABGR format for SDL_PIXELFORMAT_RGBA8888)
    for (int y = 0; y < EINK_HEIGHT; y++) {
        for (int x = 0; x < EINK_WIDTH; x++) {
            uint8_t gray = src[y * EINK_WIDTH + x];
            // For ARGB8888 format: Alpha, Red, Green, Blue
            uint32_t rgba = (0xFF << 24) | (gray << 16) | (gray << 8) | gray;
            dst[y * (pitch / 4) + x] = rgba;
        }
    }
    
    SDL_UnlockTexture(einkTexture);
    
    // Present to screen - set viewport BEFORE any rendering operations
    SDL_Rect einkViewport = {0, 0, EINK_WIDTH * 3, EINK_HEIGHT * 3};
    SDL_RenderSetViewport(einkRenderer, &einkViewport);
    
    SDL_SetRenderDrawColor(einkRenderer, 128, 128, 128, 255);
    SDL_RenderClear(einkRenderer);
    SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr);
    SDL_RenderPresent(einkRenderer);
}

void DesktopDisplay::updateOledTexture() {
    // DISABLED: All OLED rendering now handled by thread-safe OLED service
    // This prevents Metal command buffer conflicts
}

void DesktopDisplay::renderOledText(const std::string& line1, const std::string& line2, const std::string& line3) {
    if (!oledTexture || !oledRenderer) return;
    
    std::cout << "[OLED] Drawing text to buffer..." << std::endl;
    
    // Render text lines to buffer
    if (!line1.empty()) {
        std::cout << "[OLED] Drawing line1: " << line1 << std::endl;
        oledDrawText(line1, 0, 8);
    }
    if (!line2.empty()) {
        std::cout << "[OLED] Drawing line2: " << line2 << std::endl;
        oledDrawText(line2, 0, 16);
    }
    if (!line3.empty()) {
        std::cout << "[OLED] Drawing line3: " << line3 << std::endl;
        oledDrawText(line3, 0, 24);
    }
    
    // Update texture from buffer
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(oledTexture, nullptr, &pixels, &pitch) != 0) {
        std::cout << "[OLED] ERROR: Failed to lock texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint32_t* texturePixels = static_cast<uint32_t*>(pixels);
    int pixelCount = 0;
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int x = 0; x < OLED_WIDTH; ++x) {
            bool pixelOn = oledBuffer[y * OLED_WIDTH + x];
            texturePixels[y * OLED_WIDTH + x] = pixelOn ? 0xFFFFFFFF : 0xFF000000;
            if (pixelOn) pixelCount++;
        }
    }
    std::cout << "[OLED] Updated texture with " << pixelCount << " pixels on" << std::endl;
    
    SDL_UnlockTexture(oledTexture);
    
    // Present OLED window
    SDL_SetRenderTarget(oledRenderer, nullptr);
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH * 3, OLED_HEIGHT * 3};
    SDL_RenderSetViewport(oledRenderer, &oledViewport);
    
    SDL_SetRenderDrawColor(oledRenderer, 128, 128, 128, 255);
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}
