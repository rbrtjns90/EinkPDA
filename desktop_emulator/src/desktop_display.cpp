#include "desktop_display.h"
#include <iostream>
#include <cstring>

// g_display is defined in main_new.cpp

DesktopDisplay::DesktopDisplay() 
    : einkWindow(nullptr), oledWindow(nullptr), 
      einkRenderer(nullptr), oledRenderer(nullptr),
      einkTexture(nullptr), oledTexture(nullptr),
      font(nullptr), smallFont(nullptr), lastKey(0) {
    
    einkBuffer.resize(EINK_WIDTH * EINK_HEIGHT, 255); // White background
    oledBuffer.resize(OLED_WIDTH * OLED_HEIGHT, 0);   // Black background
    memset(keyPressed, 0, sizeof(keyPressed));
}

DesktopDisplay::~DesktopDisplay() {
    cleanup();
}

bool DesktopDisplay::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    // Create E-Ink window with explicit positioning
    einkWindow = SDL_CreateWindow("PocketMage E-Ink Display (310x128)",
        100, 100,  // Explicit position instead of SDL_WINDOWPOS_UNDEFINED
        EINK_WIDTH * SCALE_FACTOR, EINK_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    
    if (!einkWindow) {
        std::cerr << "E-Ink window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create OLED window with explicit positioning
    oledWindow = SDL_CreateWindow("PocketMage OLED Display (256x32)",
        100 + EINK_WIDTH * SCALE_FACTOR + 50, 100,  // Position next to E-Ink window
        OLED_WIDTH * SCALE_FACTOR, OLED_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    
    if (!oledWindow) {
        std::cerr << "OLED window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create renderers
    einkRenderer = SDL_CreateRenderer(einkWindow, -1, SDL_RENDERER_ACCELERATED);
    oledRenderer = SDL_CreateRenderer(oledWindow, -1, SDL_RENDERER_ACCELERATED);
    
    if (!einkRenderer || !oledRenderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create textures
    einkTexture = SDL_CreateTexture(einkRenderer, SDL_PIXELFORMAT_RGB24, 
        SDL_TEXTUREACCESS_STREAMING, EINK_WIDTH, EINK_HEIGHT);
    oledTexture = SDL_CreateTexture(oledRenderer, SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING, OLED_WIDTH, OLED_HEIGHT);
    
    if (!einkTexture || !oledTexture) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load fonts - try multiple paths
    const char* fontPaths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier New.ttf",
        "/System/Library/Fonts/Menlo.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        nullptr
    };
    
    for (int i = 0; fontPaths[i] && !font; i++) {
        font = TTF_OpenFont(fontPaths[i], 12);
        if (font) {
            std::cout << "[Font] Loaded: " << fontPaths[i] << std::endl;
        }
    }
    
    for (int i = 0; fontPaths[i] && !smallFont; i++) {
        smallFont = TTF_OpenFont(fontPaths[i], 8);
        if (smallFont) {
            std::cout << "[Font] Small loaded: " << fontPaths[i] << std::endl;
        }
    }
    
    if (!font || !smallFont) {
        std::cerr << "Warning: Could not load fonts. Text rendering may not work." << std::endl;
    }
    
    // Force windows to be visible and raised on macOS
    SDL_ShowWindow(einkWindow);
    SDL_ShowWindow(oledWindow);
    SDL_RaiseWindow(einkWindow);
    SDL_RaiseWindow(oledWindow);
    
    // Additional macOS-specific window management
    SDL_SetWindowAlwaysOnTop(einkWindow, SDL_TRUE);
    SDL_SetWindowAlwaysOnTop(oledWindow, SDL_TRUE);
    
    // Force focus to the windows
    SDL_SetWindowInputFocus(einkWindow);
    
    // Clear both displays initially
    einkClear();
    oledClear();
    present();
    
    // Give SDL time to process window events
    SDL_PumpEvents();
    
    std::cout << "[Display] SDL2 windows initialized and visible" << std::endl;
    
    return true;
}

void DesktopDisplay::cleanup() {
    if (font) TTF_CloseFont(font);
    if (smallFont) TTF_CloseFont(smallFont);
    if (einkTexture) SDL_DestroyTexture(einkTexture);
    if (oledTexture) SDL_DestroyTexture(oledTexture);
    if (einkRenderer) SDL_DestroyRenderer(einkRenderer);
    if (oledRenderer) SDL_DestroyRenderer(oledRenderer);
    if (einkWindow) SDL_DestroyWindow(einkWindow);
    if (oledWindow) SDL_DestroyWindow(oledWindow);
    
    TTF_Quit();
    SDL_Quit();
}

void DesktopDisplay::einkClear() {
    std::fill(einkBuffer.begin(), einkBuffer.end(), 255); // White
}

void DesktopDisplay::einkSetPixel(int x, int y, bool black) {
    if (x >= 0 && x < EINK_WIDTH && y >= 0 && y < EINK_HEIGHT) {
        einkBuffer[y * EINK_WIDTH + x] = black ? 0 : 255;
    }
}

void DesktopDisplay::einkDrawText(const std::string& text, int x, int y, int size) {
    if (!font) return;
    
    SDL_Color color = {0, 0, 0, 255}; // Black text
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!textSurface) return;
    
    // Clear area first
    for (int dy = 0; dy < textSurface->h && y + dy < EINK_HEIGHT; dy++) {
        for (int dx = 0; dx < textSurface->w && x + dx < EINK_WIDTH; dx++) {
            einkSetPixel(x + dx, y + dy, false); // Clear to white
        }
    }
    
    // Copy text to buffer with proper pixel format handling
    SDL_LockSurface(textSurface);
    for (int dy = 0; dy < textSurface->h && y + dy < EINK_HEIGHT; dy++) {
        for (int dx = 0; dx < textSurface->w && x + dx < EINK_WIDTH; dx++) {
            Uint8* pixels = (Uint8*)textSurface->pixels;
            int pixelIndex = dy * textSurface->pitch + dx * textSurface->format->BytesPerPixel;
            
            Uint32 pixel;
            if (textSurface->format->BytesPerPixel == 4) {
                pixel = *(Uint32*)(pixels + pixelIndex);
            } else if (textSurface->format->BytesPerPixel == 3) {
                pixel = pixels[pixelIndex] | (pixels[pixelIndex + 1] << 8) | (pixels[pixelIndex + 2] << 16);
            } else {
                continue;
            }
            
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, textSurface->format, &r, &g, &b, &a);
            
            // If pixel is dark enough and not transparent
            if (a > 128 && (r + g + b) / 3 < 128) {
                einkSetPixel(x + dx, y + dy, true);
            }
        }
    }
    SDL_UnlockSurface(textSurface);
    SDL_FreeSurface(textSurface);
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
        for (int dx = 0; dx < w; dx++) {
            einkSetPixel(x + dx, y, black);
            einkSetPixel(x + dx, y + h - 1, black);
        }
        for (int dy = 0; dy < h; dy++) {
            einkSetPixel(x, y + dy, black);
            einkSetPixel(x + w - 1, y + dy, black);
        }
    }
}

void DesktopDisplay::einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black) {
    if (!bitmap) {
        std::cout << "[BITMAP] Error: bitmap is null" << std::endl;
        return;
    }
    
    std::cout << "[BITMAP] Drawing " << w << "x" << h << " bitmap at (" << x << "," << y << ")" << std::endl;
    
    int byteWidth = (w + 7) / 8; // Bitmap width in bytes
    int pixelsDrawn = 0;
    
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int byteIndex = dy * byteWidth + dx / 8;
            int bitIndex = 7 - (dx % 8); // MSB first bit order
            bool pixelOn = (bitmap[byteIndex] >> bitIndex) & 1;
            if (pixelOn) {
                einkSetPixel(x + dx, y + dy, black);
                pixelsDrawn++;
            }
        }
    }
    
    std::cout << "[BITMAP] Drew " << pixelsDrawn << " pixels" << std::endl;
}

void DesktopDisplay::einkDrawCircle(int x, int y, int r, bool filled, bool black) {
    if (filled) {
        // Draw filled circle
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r) {
                    einkSetPixel(x + dx, y + dy, black);
                }
            }
        }
    } else {
        // Draw circle outline using Bresenham's circle algorithm
        int dx = 0;
        int dy = r;
        int d = 3 - 2 * r;
        
        while (dy >= dx) {
            // Draw 8 symmetric points
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
    if (!text || !font) {
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = text ? strlen(text) * 8 : 0;
        if (h) *h = 16;
        return;
    }
    
    int textW, textH;
    if (TTF_SizeText(font, text, &textW, &textH) == 0) {
        if (x1) *x1 = x;
        if (y1) *y1 = y - textH; // TTF coordinates are different
        if (w) *w = textW;
        if (h) *h = textH;
    } else {
        // Fallback calculation
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = strlen(text) * 8;
        if (h) *h = 16;
    }
}

void DesktopDisplay::einkRefresh() {
    updateEinkTexture();
}

void DesktopDisplay::einkPartialRefresh() {
    updateEinkTexture();
}

void DesktopDisplay::oledClear() {
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0); // Black
}

void DesktopDisplay::oledSetPixel(int x, int y, bool on) {
    if (x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT) {
        oledBuffer[y * OLED_WIDTH + x] = on ? 255 : 0;
    }
}

void DesktopDisplay::oledDrawText(const std::string& text, int x, int y, int size) {
    if (!smallFont) return;
    
    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface* textSurface = TTF_RenderText_Blended(smallFont, text.c_str(), color);
    if (!textSurface) return;
    
    // Clear area first
    for (int dy = 0; dy < textSurface->h && y + dy < OLED_HEIGHT; dy++) {
        for (int dx = 0; dx < textSurface->w && x + dx < OLED_WIDTH; dx++) {
            oledSetPixel(x + dx, y + dy, false); // Clear to black
        }
    }
    
    // Copy text to buffer with proper pixel format handling
    SDL_LockSurface(textSurface);
    for (int dy = 0; dy < textSurface->h && y + dy < OLED_HEIGHT; dy++) {
        for (int dx = 0; dx < textSurface->w && x + dx < OLED_WIDTH; dx++) {
            Uint8* pixels = (Uint8*)textSurface->pixels;
            int pixelIndex = dy * textSurface->pitch + dx * textSurface->format->BytesPerPixel;
            
            Uint32 pixel;
            if (textSurface->format->BytesPerPixel == 4) {
                pixel = *(Uint32*)(pixels + pixelIndex);
            } else if (textSurface->format->BytesPerPixel == 3) {
                pixel = pixels[pixelIndex] | (pixels[pixelIndex + 1] << 8) | (pixels[pixelIndex + 2] << 16);
            } else {
                continue;
            }
            
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, textSurface->format, &r, &g, &b, &a);
            
            // If pixel is bright enough and not transparent
            if (a > 128 && (r + g + b) / 3 > 128) {
                oledSetPixel(x + dx, y + dy, true);
            }
        }
    }
    SDL_UnlockSurface(textSurface);
    SDL_FreeSurface(textSurface);
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
        oledDrawLine(x, y, x + w - 1, y, on);
        oledDrawLine(x, y + h - 1, x + w - 1, y + h - 1, on);
        oledDrawLine(x, y, x, y + h - 1, on);
        oledDrawLine(x + w - 1, y, x + w - 1, y + h - 1, on);
    }
}

void DesktopDisplay::oledUpdate() {
    updateOledTexture();
}

bool DesktopDisplay::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        } else if (e.type == SDL_KEYDOWN) {
            keyPressed[e.key.keysym.scancode] = true;
            
            // Map common keys to characters
            switch (e.key.keysym.sym) {
                case SDLK_a: case SDLK_b: case SDLK_c: case SDLK_d: case SDLK_e:
                case SDLK_f: case SDLK_g: case SDLK_h: case SDLK_i: case SDLK_j:
                case SDLK_k: case SDLK_l: case SDLK_m: case SDLK_n: case SDLK_o:
                case SDLK_p: case SDLK_q: case SDLK_r: case SDLK_s: case SDLK_t:
                case SDLK_u: case SDLK_v: case SDLK_w: case SDLK_x: case SDLK_y:
                case SDLK_z:
                    lastKey = e.key.keysym.sym;
                    break;
                case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
                case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
                    lastKey = e.key.keysym.sym;
                    break;
                case SDLK_SPACE:
                    lastKey = ' ';
                    break;
                case SDLK_RETURN:
                    lastKey = '\n';
                    break;
                case SDLK_BACKSPACE:
                    lastKey = '\b';
                    break;
                case SDLK_UP:
                    lastKey = 'U';
                    break;
                case SDLK_DOWN:
                    lastKey = 'D';
                    break;
                case SDLK_LEFT:
                    lastKey = 'L';
                    break;
                case SDLK_RIGHT:
                    lastKey = 'R';
                    break;
                default:
                    lastKey = 0;
                    break;
            }
        } else if (e.type == SDL_KEYUP) {
            keyPressed[e.key.keysym.scancode] = false;
        }
    }
    return true;
}

char DesktopDisplay::getLastKey() {
    char key = lastKey;
    lastKey = 0; // Clear after reading
    return key;
}

bool DesktopDisplay::isKeyPressed(SDL_Scancode key) {
    return keyPressed[key];
}

void DesktopDisplay::present() {
    updateEinkTexture();
    updateOledTexture();
    
    // Present E-Ink
    SDL_SetRenderDrawColor(einkRenderer, 255, 255, 255, 255);
    SDL_RenderClear(einkRenderer);
    SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr);
    SDL_RenderPresent(einkRenderer);
    
    // Present OLED
    SDL_SetRenderDrawColor(oledRenderer, 0, 0, 0, 255);
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}

void DesktopDisplay::updateEinkTexture() {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(einkTexture, nullptr, &pixels, &pitch) == 0) {
        Uint8* pixelBytes = (Uint8*)pixels;
        
        for (int y = 0; y < EINK_HEIGHT; y++) {
            for (int x = 0; x < EINK_WIDTH; x++) {
                Uint8 value = einkBuffer[y * EINK_WIDTH + x];
                int index = y * pitch + x * 3;
                if (index + 2 < pitch * EINK_HEIGHT) {
                    pixelBytes[index] = value;     // R
                    pixelBytes[index + 1] = value; // G
                    pixelBytes[index + 2] = value; // B
                }
            }
        }
        
        SDL_UnlockTexture(einkTexture);
    }
}

void DesktopDisplay::updateOledTexture() {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(oledTexture, nullptr, &pixels, &pitch) == 0) {
        Uint8* pixelBytes = (Uint8*)pixels;
        
        for (int y = 0; y < OLED_HEIGHT; y++) {
            for (int x = 0; x < OLED_WIDTH; x++) {
                Uint8 value = oledBuffer[y * OLED_WIDTH + x];
                int index = y * pitch + x * 3;
                if (index + 2 < pitch * OLED_HEIGHT) {
                    pixelBytes[index] = value;     // R
                    pixelBytes[index + 1] = value; // G
                    pixelBytes[index + 2] = value; // B
                }
            }
        }
        
        SDL_UnlockTexture(oledTexture);
    }
}

SDL_Color DesktopDisplay::getEinkColor(bool black) {
    return black ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255};
}

SDL_Color DesktopDisplay::getOledColor(bool on) {
    return on ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
}
