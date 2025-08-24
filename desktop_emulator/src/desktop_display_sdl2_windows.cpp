// Windows-only SDL2 display backend for PocketMage Emulator
// Builds on Windows with SDL2 + SDL2_ttf installed.
// Implements DesktopDisplay declared in desktop_display.h

#if !defined(_WIN32)
#error "desktop_display_sdl2_windows.cpp is Windows-only. Guard your CMake to compile this on Windows."
#endif

#include "desktop_display.h"
#include "pocketmage_compat.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef INPUT
#undef INPUT
#endif
#include <windows.h>
#endif

// ---- Global instance expected by the rest of the emulator ----
DesktopDisplay* g_display = nullptr;

// ---- Windows-specific static data ----
static uint32_t einkGrayLut[256]; // Grayscale lookup table for fast ARGB conversion

// --- Windows-specific font discovery helpers ---
static std::string path_join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/' || a.back() == '\\') return a + "\\" + b;
    return a + "\\" + b;
}

static std::vector<std::string> candidate_font_dirs() {
    std::vector<std::string> dirs;

    // 1) Explicit override for debugging or packaging
    if (const char* env = std::getenv("POCKETMAGE_FONT_DIR")) {
        dirs.push_back(env);
    }

    // 2) SDL base path (where the executable lives); prefer data/fonts under it
    if (char* base = SDL_GetBasePath()) {
        std::string basePath(base);
        SDL_free(base);
        dirs.push_back(path_join(basePath, "data\\fonts"));
        dirs.push_back(path_join(basePath, "fonts"));
    }

    // 3) Common relative paths for local runs
    dirs.push_back("data\\fonts");
    dirs.push_back("..\\data\\fonts");
    dirs.push_back("..\\..\\data\\fonts");

    // 4) Windows system font directories
    char windir[MAX_PATH];
    if (GetWindowsDirectoryA(windir, MAX_PATH)) {
        std::string windowsDir(windir);
        dirs.push_back(path_join(windowsDir, "Fonts"));
    }
    
    dirs.push_back("C:\\Windows\\Fonts");
    dirs.push_back("C:\\WINDOWS\\Fonts");
    
    return dirs;
}

static std::string find_font_file(const std::vector<std::string>& names) {
    for (const auto& dir : candidate_font_dirs()) {
        for (const auto& name : names) {
            std::string p = path_join(dir, name);
            FILE* f = std::fopen(p.c_str(), "rb");
            if (f) { std::fclose(f); return p; }
        }
    }
    return {};
}

static TTF_Font* open_font_local_first(const std::vector<std::string>& fontNames, int pt) {
    std::string local = find_font_file(fontNames);
    if (!local.empty()) {
        if (TTF_Font* f = TTF_OpenFont(local.c_str(), pt)) {
            std::cout << "[TTF] Loaded local font: " << local << " (" << pt << "pt)\n";
            return f;
        } else {
            std::cerr << "[TTF] Found but failed to open: " << local
                      << " :: " << TTF_GetError() << "\n";
        }
    }

    for (const auto& n : fontNames) {
        if (n.find('\\') != std::string::npos || n.find(':') != std::string::npos) {
            if (TTF_Font* f = TTF_OpenFont(n.c_str(), pt)) {
                std::cout << "[TTF] Loaded system font: " << n << " (" << pt << "pt)\n";
                return f;
            }
        }
    }
    return nullptr;
}

// ---- Class implementation (DesktopDisplay) ----
DesktopDisplay::DesktopDisplay()
    : einkWindow(nullptr), oledWindow(nullptr),
      einkRenderer(nullptr), oledRenderer(nullptr),
      einkTexture(nullptr), oledTexture(nullptr),
      font(nullptr), smallFont(nullptr),
      lastKey(0) {
    std::fill(std::begin(keyPressed), std::end(keyPressed), false);
    // Initialize grayscale LUT
    for (int i = 0; i < 256; ++i) {
        uint8_t g = static_cast<uint8_t>(255 - i);
        einkGrayLut[i] = (0xFFu << 24) | (uint32_t(g) << 16) | (uint32_t(g) << 8) | uint32_t(g);
    }
}

DesktopDisplay::~DesktopDisplay() { cleanup(); }

bool DesktopDisplay::init() {
    DEBUG_LOG("SDL2", "Initializing DesktopDisplay for Windows...");

    // Windows-specific SDL hints
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_RENDER_DIRECT3D_THREADSAFE, "1");
    SDL_SetHint(SDL_HINT_RENDER_DIRECT3D11_DEBUG, "0");
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "none");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "0");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[SDL2] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() != 0) {
        std::cerr << "[TTF] TTF_Init failed: " << TTF_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    // Create windows
    einkWindow = SDL_CreateWindow(
        "PocketMage E-Ink (Windows)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        EINK_WIDTH * SCALE_FACTOR, EINK_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!einkWindow) {
        std::cerr << "[SDL2] Create E-Ink window failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    oledWindow = SDL_CreateWindow(
        "PocketMage OLED (Windows)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        OLED_WIDTH * SCALE_FACTOR, OLED_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!oledWindow) {
        std::cerr << "[SDL2] Create OLED window failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Create renderers (prefer software for stability on Windows)
    einkRenderer = SDL_CreateRenderer(einkWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!einkRenderer) {
        std::cerr << "[SDL2] Create E-Ink renderer failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }
    
    oledRenderer = SDL_CreateRenderer(oledWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!oledRenderer) {
        std::cerr << "[SDL2] Create OLED renderer failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Set logical size for proper scaling
    SDL_RenderSetLogicalSize(einkRenderer, EINK_WIDTH, EINK_HEIGHT);
    SDL_RenderSetLogicalSize(oledRenderer, OLED_WIDTH, OLED_HEIGHT);

    // Create textures
    einkTexture = SDL_CreateTexture(einkRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, EINK_WIDTH, EINK_HEIGHT);
    if (!einkTexture) {
        std::cerr << "[SDL2] Create E-Ink texture failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }
    oledTexture = SDL_CreateTexture(oledRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, OLED_WIDTH, OLED_HEIGHT);
    if (!oledTexture) {
        std::cerr << "[SDL2] Create OLED texture failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Initialize buffers
    einkBuffer.assign(EINK_WIDTH * EINK_HEIGHT, 255);   // White background
    oledBuffer.assign(OLED_WIDTH * OLED_HEIGHT, 0);     // Black background

    // Load fonts
    const std::vector<std::string> regularNames = {
        "DejaVuSans.ttf",
        "arial.ttf",
        "calibri.ttf",
        "segoeui.ttf",
        "tahoma.ttf",
        "verdana.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf"
    };

    font = open_font_local_first(regularNames, 12);
    smallFont = open_font_local_first(regularNames, 8);

    if (!font) {
        std::cerr << "[TTF] WARNING: No usable font found. Text rendering will be skipped.\n";
    }

    // First paint
    updateEinkTexture();
    updateOledTexture();
    SDL_RaiseWindow(einkWindow);
    SDL_RaiseWindow(oledWindow);

    DEBUG_LOG("SDL2", "DesktopDisplay (Windows) initialized successfully");
    return true;
}

void DesktopDisplay::cleanup() {
    DEBUG_LOG("SDL2", "Cleaning up DesktopDisplay...");
    
    if (font) { TTF_CloseFont(font); font = nullptr; }
    if (smallFont) { TTF_CloseFont(smallFont); smallFont = nullptr; }

    if (einkTexture) { SDL_DestroyTexture(einkTexture); einkTexture = nullptr; }
    if (oledTexture) { SDL_DestroyTexture(oledTexture); oledTexture = nullptr; }

    if (einkRenderer) { SDL_DestroyRenderer(einkRenderer); einkRenderer = nullptr; }
    if (oledRenderer) { SDL_DestroyRenderer(oledRenderer); oledRenderer = nullptr; }

    if (einkWindow) { SDL_DestroyWindow(einkWindow); einkWindow = nullptr; }
    if (oledWindow) { SDL_DestroyWindow(oledWindow); oledWindow = nullptr; }

    if (TTF_WasInit()) TTF_Quit();
    SDL_Quit();
    
    DEBUG_LOG("SDL2", "DesktopDisplay cleanup complete");
}

// ---- E-Ink display methods ----
void DesktopDisplay::einkClear() {
    std::lock_guard<std::mutex> lock(einkMutex);
    std::fill(einkBuffer.begin(), einkBuffer.end(), 255); // White background
    DEBUG_LOG("SDL2", "E-Ink cleared to white");
    updateEinkTexture();
}

void DesktopDisplay::einkSetPixel(int x, int y, bool black) {
    if (x >= 0 && x < EINK_WIDTH && y >= 0 && y < EINK_HEIGHT) {
        einkBuffer[y * EINK_WIDTH + x] = black ? 0 : 255; // 0 = black, 255 = white
    }
}

void DesktopDisplay::einkDrawText(const std::string& text, int x, int y, int size, bool whiteText) {
    if (text.empty() || !font) return;
    
    SDL_Color textColor = whiteText ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Solid(font, text.c_str(), textColor);
    
    if (!textSurface) {
        std::cerr << "[SDL2] Failed to render text surface: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_LockSurface(textSurface);
    
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    
    for (int dy = 0; dy < textHeight && (y + dy) < EINK_HEIGHT; dy++) {
        for (int dx = 0; dx < textWidth && (x + dx) < EINK_WIDTH; dx++) {
            if (x + dx >= 0 && y + dy >= 0) {
                Uint8* pixelPtr = (Uint8*)textSurface->pixels + (dy * textSurface->pitch) + (dx * textSurface->format->BytesPerPixel);
                
                Uint8 intensity = 0;
                if (textSurface->format->BytesPerPixel == 1) {
                    intensity = *pixelPtr;
                } else if (textSurface->format->BytesPerPixel >= 3) {
                    intensity = pixelPtr[0];
                }
                
                int fbIndex = (y + dy) * EINK_WIDTH + (x + dx);
                if (fbIndex >= 0 && fbIndex < (int)einkBuffer.size()) {
                    if (intensity > 0) {
                        einkBuffer[fbIndex] = whiteText ? 255 : 0;
                    }
                }
            }
        }
    }
    
    SDL_UnlockSurface(textSurface);
    SDL_FreeSurface(textSurface);
    
    updateEinkTexture();
}

void DesktopDisplay::einkDrawLine(int x0, int y0, int x1, int y1, bool black) {
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
        einkDrawLine(x, y, x + w - 1, y, black);
        einkDrawLine(x, y + h - 1, x + w - 1, y + h - 1, black);
        einkDrawLine(x, y, x, y + h - 1, black);
        einkDrawLine(x + w - 1, y, x + w - 1, y + h - 1, black);
    }
}

void DesktopDisplay::einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black) {
    if (!bitmap || w <= 0 || h <= 0) return;
    
    const int stride = (w + 7) / 8;
    const size_t bytes = size_t(stride) * size_t(h);
    
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const size_t byteIndex = size_t(yy) * size_t(stride) + size_t(xx / 8);
            if (byteIndex >= bytes) continue;
            
            const int bitIndex = 7 - (xx % 8);
            const bool on = (bitmap[byteIndex] >> bitIndex) & 1;
            if (on) einkSetPixel(x + xx, y + yy, black);
            else if (!black) einkSetPixel(x + xx, y + yy, false);
        }
    }
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
    *w = len * 6;
    *h = 12;
}

void DesktopDisplay::einkRefresh() {
    DEBUG_LOG("SDL2", "E-Ink refresh - updating display");
    updateEinkTexture();
}

void DesktopDisplay::einkPartialRefresh() {
    DEBUG_LOG("SDL2", "E-Ink partial refresh - updating display");
    updateEinkTexture();
}

// ---- OLED display methods ----
void DesktopDisplay::oledClear() {
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);
    DEBUG_LOG("SDL2", "OLED cleared to black");
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
                    oledBuffer[index] = 0;
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
    if (!font || text.empty()) return;
    
    int estimatedWidth = text.length() * (size + 2);
    int estimatedHeight = size + 4;
    oledClearRect(x, y, estimatedWidth, estimatedHeight);
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color bgColor = {0, 0, 0, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Shaded(font, text.c_str(), textColor, bgColor);
    
    if (!textSurface) {
        std::cerr << "[SDL2] Failed to render OLED text surface: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_LockSurface(textSurface);
    
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    
    for (int dy = 0; dy < textHeight && (y + dy) < OLED_HEIGHT; dy++) {
        for (int dx = 0; dx < textWidth && (x + dx) < OLED_WIDTH; dx++) {
            if (x + dx >= 0 && y + dy >= 0) {
                Uint8* pixelPtr = (Uint8*)textSurface->pixels + (dy * textSurface->pitch) + (dx * textSurface->format->BytesPerPixel);
                
                Uint8 intensity = 0;
                if (textSurface->format->BytesPerPixel == 1) {
                    intensity = *pixelPtr;
                } else if (textSurface->format->BytesPerPixel >= 3) {
                    intensity = pixelPtr[0];
                }
                
                int fbIndex = (y + dy) * OLED_WIDTH + (x + dx);
                if (fbIndex >= 0 && fbIndex < (int)oledBuffer.size()) {
                    if (intensity > 128) {
                        oledBuffer[fbIndex] = 255;
                    }
                }
            }
        }
    }
    
    SDL_UnlockSurface(textSurface);
    SDL_FreeSurface(textSurface);
    
    updateOledTexture();
}

void DesktopDisplay::oledDrawLine(int x0, int y0, int x1, int y1, bool on) {
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
    DEBUG_LOG("SDL2", "OLED update - updating display");
    updateOledTexture();
}

// ---- Input handling ----
bool DesktopDisplay::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    lastKey = 19;
                    break;
                case SDLK_DOWN:
                    lastKey = 21;
                    break;
                case SDLK_LEFT:
                    lastKey = 20;
                    break;
                case SDLK_RIGHT:
                    lastKey = 18;
                    break;
                case SDLK_ESCAPE:
                    lastKey = 27;
                    break;
                case SDLK_HOME:
                    lastKey = 12;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    lastKey = 13;
                    break;
                case SDLK_BACKSPACE:
                    lastKey = 8;
                    break;
                default:
                    lastKey = e.key.keysym.sym;
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

// ---- Utility ----
void DesktopDisplay::present() {
    updateEinkTexture();
}

SDL_Color DesktopDisplay::getEinkColor(bool black) {
    return black ? SDL_Color{0, 0, 0, 255} : SDL_Color{255, 255, 255, 255};
}

SDL_Color DesktopDisplay::getOledColor(bool on) {
    return on ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255};
}

void DesktopDisplay::updateEinkTexture() {
    std::lock_guard<std::mutex> lock(einkMutex);
    
    if (!einkRenderer || !einkTexture) {
        std::cerr << "[SDL2] Invalid E-Ink renderer or texture" << std::endl;
        return;
    }
    
    // Windows: Use SDL_LockTexture with inline conversion for stability
    void* pixels;
    int pitch;
    if (SDL_LockTexture(einkTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock E-Ink texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint32_t* row = static_cast<uint32_t*>(pixels);
    for (int y = 0; y < EINK_HEIGHT; ++y) {
        for (int x = 0; x < EINK_WIDTH; ++x) {
            uint8_t gray = einkBuffer[y * EINK_WIDTH + x];
            row[x] = (0xFF << 24) | (gray << 16) | (gray << 8) | gray;
        }
        row = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(row) + pitch);
    }
    SDL_UnlockTexture(einkTexture);
    
    if (SDL_SetRenderTarget(einkRenderer, nullptr) != 0) {
        std::cerr << "[SDL2] Failed to set render target: " << SDL_GetError() << std::endl;
        return;
    }
    
    SDL_Rect einkViewport = {0, 0, EINK_WIDTH * 3, EINK_HEIGHT * 3};
    if (SDL_RenderSetViewport(einkRenderer, &einkViewport) != 0) {
        std::cerr << "[SDL2] Failed to set viewport: " << SDL_GetError() << std::endl;
        return;
    }
    
    if (SDL_SetRenderDrawColor(einkRenderer, 128, 128, 128, 255) != 0) {
        std::cerr << "[SDL2] Failed to set draw color: " << SDL_GetError() << std::endl;
        return;
    }
    
    if (SDL_RenderClear(einkRenderer) != 0) {
        std::cerr << "[SDL2] Failed to clear renderer: " << SDL_GetError() << std::endl;
        return;
    }
    
    if (SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr) != 0) {
        std::cerr << "[SDL2] Failed to copy texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    SDL_RenderPresent(einkRenderer);
}

void DesktopDisplay::updateOledTexture() {
    if (!oledTexture || !oledRenderer) return;
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(oledTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock OLED texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint32_t* texturePixels = static_cast<uint32_t*>(pixels);
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int x = 0; x < OLED_WIDTH; ++x) {
            bool pixelOn = oledBuffer[y * OLED_WIDTH + x];
            texturePixels[y * OLED_WIDTH + x] = pixelOn ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    
    SDL_UnlockTexture(oledTexture);
    
    SDL_SetRenderTarget(oledRenderer, nullptr);
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH * 3, OLED_HEIGHT * 3};
    SDL_RenderSetViewport(oledRenderer, &oledViewport);
    
    SDL_SetRenderDrawColor(oledRenderer, 128, 128, 128, 255);
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}

void DesktopDisplay::renderOledText(const std::string& line1, const std::string& line2, const std::string& line3) {
    if (!oledTexture || !oledRenderer) return;
    
    if (!line1.empty()) {
        oledDrawText(line1, 0, 8);
    }
    if (!line2.empty()) {
        oledDrawText(line2, 0, 16);
    }
    if (!line3.empty()) {
        oledDrawText(line3, 0, 24);
    }
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(oledTexture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "[SDL2] Failed to lock texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint32_t* texturePixels = static_cast<uint32_t*>(pixels);
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int x = 0; x < OLED_WIDTH; ++x) {
            bool pixelOn = oledBuffer[y * OLED_WIDTH + x];
            texturePixels[y * OLED_WIDTH + x] = pixelOn ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    
    SDL_UnlockTexture(oledTexture);
    
    SDL_SetRenderTarget(oledRenderer, nullptr);
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH * 3, OLED_HEIGHT * 3};
    SDL_RenderSetViewport(oledRenderer, &oledViewport);
    
    SDL_SetRenderDrawColor(oledRenderer, 128, 128, 128, 255);
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}
