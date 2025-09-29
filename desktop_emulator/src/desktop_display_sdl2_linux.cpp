// Linux-only SDL2 display backend for PocketMage Emulator
// Builds on Linux distros with SDL2 + SDL2_ttf installed.
// Implements DesktopDisplay declared in desktop_display_sdl2.h

#if !defined(__linux__)
#error "desktop_display_sdl2_linux.cpp is Linux-only. Guard your CMake to compile this on Linux."
#endif

#include "desktop_display_sdl2.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>

#ifndef PM_LINUX_SDL2
#define PM_LINUX_SDL2 1
#endif

// ---- Global instance expected by the rest of the emulator ----
DesktopDisplay* g_display = nullptr;

// ---- Linux-specific static data ----
static uint32_t einkGrayLut[256]; // Grayscale lookup table for fast ARGB conversion

// --- Local font discovery helpers (Linux) ---
static std::string path_join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/' || a.back() == '\\') return a + "/" + b;
    return a + "/" + b;
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
        dirs.push_back(path_join(basePath, "data/fonts"));
        dirs.push_back(path_join(basePath, "fonts")); // optional
    }

    // 3) Common relative paths for local runs (cmake build trees)
    dirs.push_back("data/fonts");
    dirs.push_back("../data/fonts");
    dirs.push_back("../../data/fonts");

    // 4) Last resort: typical Linux system directories (kept as fallback only)
    dirs.push_back("/usr/share/fonts/truetype/dejavu");
    dirs.push_back("/usr/share/fonts/truetype/noto");
    dirs.push_back("/usr/share/fonts/TTF");
    dirs.push_back("/usr/share/fonts/truetype/liberation");
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
    // Try local (project) fonts first
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

    // Fallback to system fonts by absolute name (rarely needed if data/ is present)
    for (const auto& n : fontNames) {
        if (n.find('/') != std::string::npos) { // absolute-ish
            if (TTF_Font* f = TTF_OpenFont(n.c_str(), pt)) {
                std::cout << "[TTF] Loaded system font: " << n << " (" << pt << "pt)\n";
                return f;
            }
        }
    }
    return nullptr;
}

// ---- Helpers ----
static bool try_init_with_driver(const char* driver) {
    if (driver) {
        // Force a specific video driver
        setenv("SDL_VIDEODRIVER", driver, 1);
    }
    // Common hints for Linux
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");                  // robust software renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "0");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");   // avoid compositor glitches on X11
    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "1");         // better Wayland window decorations
    SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "0");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");

    if (SDL_Init(SDL_INIT_VIDEO) == 0) return true;

    std::cerr << "[SDL2] SDL_Init failed (" << (driver ? driver : "auto")
              << "): " << SDL_GetError() << std::endl;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
    return false;
}

// ---- Class implementation (DesktopDisplay) ----
DesktopDisplay::DesktopDisplay()
    : einkWindow(nullptr), oledWindow(nullptr),
      einkRenderer(nullptr), oledRenderer(nullptr),
      einkTexture(nullptr), oledTexture(nullptr),
      font(nullptr), smallFont(nullptr),
      lastKey(0) {
    std::fill(std::begin(keyPressed), std::end(keyPressed), false);
    // Initialize grayscale LUT - Direct mapping (no inversion needed now)
    for (int i = 0; i < 256; ++i) {
        uint8_t g = static_cast<uint8_t>(i); // Direct mapping: 0 = black, 255 = white
        einkGrayLut[i] = (0xFFu << 24) | (uint32_t(g) << 16) | (uint32_t(g) << 8) | uint32_t(g);
    }
}

DesktopDisplay::~DesktopDisplay() { cleanup(); }

bool DesktopDisplay::init() {
    // 1) Prefer X11 (most reliable with software renderer), then fall back to Wayland, then auto
    if (!try_init_with_driver("x11") &&
        !try_init_with_driver("wayland") &&
        !try_init_with_driver(nullptr)) {
        std::cerr << "[SDL2] Unable to initialize any Linux video driver.\n";
        return false;
    }
    std::cout << "[SDL2] Using video driver: " << (SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "unknown") << "\n";

    if (TTF_Init() != 0) {
        std::cerr << "[TTF] TTF_Init failed: " << TTF_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    // Windows
    einkWindow = SDL_CreateWindow(
        "PocketMage E‑Ink (Linux)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        EINK_WIDTH * SCALE_FACTOR, EINK_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!einkWindow) {
        std::cerr << "[SDL2] Create E‑Ink window failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    oledWindow = SDL_CreateWindow(
        "PocketMage OLED (Linux)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        OLED_WIDTH * SCALE_FACTOR, OLED_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!oledWindow) {
        std::cerr << "[SDL2] Create OLED window failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Software renderers (robust across distros/compositors)
    einkRenderer = SDL_CreateRenderer(einkWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!einkRenderer) {
        std::cerr << "[SDL2] Create E‑Ink renderer failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }
    oledRenderer = SDL_CreateRenderer(oledWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (!oledRenderer) {
        std::cerr << "[SDL2] Create OLED renderer failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Set viewport to match logical size (required by SDL2)
    SDL_Rect einkViewport = {0, 0, EINK_WIDTH, EINK_HEIGHT};
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH, OLED_HEIGHT};
    SDL_RenderSetViewport(einkRenderer, &einkViewport);
    SDL_RenderSetViewport(oledRenderer, &oledViewport);
    
    // Set logical size for proper scaling
    SDL_RenderSetLogicalSize(einkRenderer, EINK_WIDTH, EINK_HEIGHT);
    SDL_RenderSetLogicalSize(oledRenderer, OLED_WIDTH, OLED_HEIGHT);

    // Textures (ARGB8888 keeps it simple/portable)
    einkTexture = SDL_CreateTexture(einkRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, EINK_WIDTH, EINK_HEIGHT);
    if (!einkTexture) {
        std::cerr << "[SDL2] Create E‑Ink texture failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }
    oledTexture = SDL_CreateTexture(oledRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, OLED_WIDTH, OLED_HEIGHT);
    if (!oledTexture) {
        std::cerr << "[SDL2] Create OLED texture failed: " << SDL_GetError() << "\n";
        cleanup();
        return false;
    }

    // Buffers - Initialize with correct background colors to match macOS
    einkBuffer.assign(EINK_WIDTH * EINK_HEIGHT, 255); // E‑Ink white background (0 = black, 255 = white)
    oledBuffer.assign(OLED_WIDTH * OLED_HEIGHT, 0);   // OLED black

    // --- Load fonts (prefer local project files in data/fonts) ---
    {
        // Put these in data/fonts/ (copied by CMake post-build step)
        const std::vector<std::string> regularNames = {
            "DejaVuSans.ttf",          // local
            "NotoSans-Regular.ttf",    // local alt
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // system fallback
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
        };

        const std::vector<std::string> monoNames = {
            "DejaVuSansMono.ttf",      // local
            "NotoSansMono-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf"
        };

        // Choose your preference; you can use mono for OLED if you like
        font = open_font_local_first(regularNames, 12);
        if (!font) font = open_font_local_first(monoNames, 12);

        smallFont = open_font_local_first(regularNames, 8);
        if (!smallFont) smallFont = open_font_local_first(monoNames, 8);

        if (!font) {
            std::cerr << "[TTF] WARNING: No usable font found. Text rendering will be skipped.\n";
        }
    }

    // First paint
    updateEinkTexture();
    updateOledTexture();
    SDL_RaiseWindow(einkWindow);
    SDL_RaiseWindow(oledWindow);

    std::cout << "[SDL2] DesktopDisplay (Linux) initialized.\n";
    return true;
}

void DesktopDisplay::cleanup() {
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
}

// ---- E‑Ink (310×240, grayscale emulated) ----
void DesktopDisplay::einkClear() {
    std::lock_guard<std::mutex> lock(einkMutex);
    std::fill(einkBuffer.begin(), einkBuffer.end(), 255);  // Fixed: Clear to white background
    updateEinkTexture();
}

void DesktopDisplay::einkSetPixel(int x, int y, bool black) {
    std::lock_guard<std::mutex> lock(einkMutex);
    if (x < 0 || x >= EINK_WIDTH || y < 0 || y >= EINK_HEIGHT) return;
    
    int index = y * EINK_WIDTH + x;
    einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white (match macOS)
}

void DesktopDisplay::einkDrawText(const std::string& text, int x, int y, int size, bool whiteText) {
    std::lock_guard<std::mutex> lock(einkMutex);
    if (!font || text.empty()) return;
    SDL_Color c = whiteText ? SDL_Color{255,255,255,255} : SDL_Color{0,0,0,255};
    SDL_Surface* s = TTF_RenderUTF8_Solid(font, text.c_str(), c);
    if (!s) { std::cerr << "[TTF] Render text failed: " << TTF_GetError() << "\n"; return; }

    SDL_LockSurface(s);
    for (int dy = 0; dy < s->h; ++dy) {
        for (int dx = 0; dx < s->w; ++dx) {
            int fx = x + dx, fy = y + dy;
            if (fx < 0 || fy < 0 || fx >= EINK_WIDTH || fy >= EINK_HEIGHT) continue;
            const Uint8* p = (Uint8*)s->pixels + dy * s->pitch + dx * s->format->BytesPerPixel;
            Uint8 intensity = s->format->BytesPerPixel == 1 ? *p : p[0]; // sample R
            if (intensity > 0) {
                einkBuffer[fy * EINK_WIDTH + fx] = whiteText ? 255 : 0;  // Correct: 0 = black, 255 = white
            }
        }
    }
    SDL_UnlockSurface(s);
    SDL_FreeSurface(s);
    updateEinkTexture();
}

void DesktopDisplay::einkDrawLine(int x0, int y0, int x1, int y1, bool black) {
    std::lock_guard<std::mutex> lock(einkMutex);
    // Bresenham's line algorithm
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        // Direct buffer access to avoid recursive locking
        if (x0 >= 0 && x0 < EINK_WIDTH && y0 >= 0 && y0 < EINK_HEIGHT) {
            int index = y0 * EINK_WIDTH + x0;
            einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    updateEinkTexture();
}

void DesktopDisplay::einkDrawRect(int x, int y, int w, int h, bool filled, bool black) {
    std::lock_guard<std::mutex> lock(einkMutex);
    if (filled) {
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                // Direct buffer access to avoid recursive locking
                int px = x + xx, py = y + yy;
                if (px >= 0 && px < EINK_WIDTH && py >= 0 && py < EINK_HEIGHT) {
                    int index = py * EINK_WIDTH + px;
                    einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white
                }
            }
        }
    } else {
        // Draw outline using direct buffer access
        for (int xx = 0; xx < w; ++xx) {
            if (x + xx >= 0 && x + xx < EINK_WIDTH) {
                if (y >= 0 && y < EINK_HEIGHT) einkBuffer[y * EINK_WIDTH + (x + xx)] = black ? 0 : 255;  // Fixed
                if (y + h - 1 >= 0 && y + h - 1 < EINK_HEIGHT) einkBuffer[(y + h - 1) * EINK_WIDTH + (x + xx)] = black ? 0 : 255;  // Fixed
            }
        }
        for (int yy = 0; yy < h; ++yy) {
            if (y + yy >= 0 && y + yy < EINK_HEIGHT) {
                if (x >= 0 && x < EINK_WIDTH) einkBuffer[(y + yy) * EINK_WIDTH + x] = black ? 0 : 255;  // Fixed
                if (x + w - 1 >= 0 && x + w - 1 < EINK_WIDTH) einkBuffer[(y + yy) * EINK_WIDTH + (x + w - 1)] = black ? 0 : 255;  // Fixed
            }
        }
    }
    updateEinkTexture();
}

void DesktopDisplay::einkDrawCircle(int cx, int cy, int r, bool filled, bool black) {
    std::lock_guard<std::mutex> lock(einkMutex);
    if (filled) {
        for (int yy = -r; yy <= r; ++yy) {
            for (int xx = -r; xx <= r; ++xx) {
                if (xx*xx + yy*yy <= r*r) {
                    int px = cx + xx, py = cy + yy;
                    if (px >= 0 && px < EINK_WIDTH && py >= 0 && py < EINK_HEIGHT) {
                        int index = py * EINK_WIDTH + px;
                        einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white
                    }
                }
            }
        }
    } else {
        // Midpoint circle algorithm (outline) with direct buffer access
        int x = r, y = 0, err = 0;
        while (x >= y) {
            auto setCirclePixel = [&](int px, int py) {
                if (px >= 0 && px < EINK_WIDTH && py >= 0 && py < EINK_HEIGHT) {
                    int index = py * EINK_WIDTH + px;
                    einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white
                }
            };
            setCirclePixel(cx + x, cy + y); setCirclePixel(cx + y, cy + x);
            setCirclePixel(cx - y, cy + x); setCirclePixel(cx - x, cy + y);
            setCirclePixel(cx - x, cy - y); setCirclePixel(cx - y, cy - x);
            setCirclePixel(cx + y, cy - x); setCirclePixel(cx + x, cy - y);
            if (err <= 0) { y += 1; err += 2*y + 1; }
            if (err > 0)  { x -= 1; err -= 2*x + 1; }
        }
    }
    updateEinkTexture();
}

void DesktopDisplay::einkDrawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, bool black) {
    std::lock_guard<std::mutex> lock(einkMutex);
    if (!bitmap || w <= 0 || h <= 0) return;

    const int stride = (w + 7) / 8;
    const size_t bytes = size_t(stride) * size_t(h);

    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const size_t byteIndex = size_t(yy) * size_t(stride) + size_t(xx / 8);
            if (byteIndex >= bytes) {
                // Optional: log once to identify the offending asset
                static int warned = 0;
                if (warned++ < 5) std::cerr << "[BITMAP] OOB: w=" << w << " h=" << h
                                            << " stride=" << stride << " idx=" << byteIndex << "\n";
                continue;
            }
            const int bitIndex = 7 - (xx % 8);
            const bool on = (bitmap[byteIndex] >> bitIndex) & 1;
            if (on) {
                // Direct buffer access to avoid recursive locking
                int px = x + xx, py = y + yy;
                if (px >= 0 && px < EINK_WIDTH && py >= 0 && py < EINK_HEIGHT) {
                    int index = py * EINK_WIDTH + px;
                    einkBuffer[index] = black ? 0 : 255;  // Fixed: 0 = black, 255 = white
                }
            } else if (!black) {
                int px = x + xx, py = y + yy;
                if (px >= 0 && px < EINK_WIDTH && py >= 0 && py < EINK_HEIGHT) {
                    int index = py * EINK_WIDTH + px;
                    einkBuffer[index] = 0;
                }
            }
        }
    }
    updateEinkTexture();
}

void DesktopDisplay::einkGetTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    if (!text) return;
    if (x1) *x1 = x;
    if (y1) *y1 = y;
    if (w)  *w  = static_cast<uint16_t>(std::strlen(text) * 6);
    if (h)  *h  = 12;
}

void DesktopDisplay::einkRefresh() {
    std::lock_guard<std::mutex> lock(einkMutex);
    updateEinkTexture();
}

void DesktopDisplay::einkPartialRefresh() {
    std::lock_guard<std::mutex> lock(einkMutex);
    updateEinkTexture();
}

// ---- OLED (256×32, 1‑bit simulated on ARGB8888) ----
void DesktopDisplay::oledClear() {
    std::lock_guard<std::mutex> lock(oledMutex);
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);
    updateOledTexture();
}

void DesktopDisplay::oledClearRect(int x, int y, int w, int h) {
    std::lock_guard<std::mutex> lock(oledMutex);
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) {
            int px = x + xx, py = y + yy;
            if (px >= 0 && px < OLED_WIDTH && py >= 0 && py < OLED_HEIGHT) {
                int index = py * OLED_WIDTH + px;
                oledBuffer[index] = 0;
            }
        }
    updateOledTexture();
}

void DesktopDisplay::oledSetPixel(int x, int y, bool on) {
    std::lock_guard<std::mutex> lock(oledMutex);
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    
    int index = y * OLED_WIDTH + x;
    oledBuffer[index] = on ? 255 : 0;
}

void DesktopDisplay::oledDrawLine(int x0, int y0, int x1, int y1, bool on) {
    std::lock_guard<std::mutex> lock(oledMutex);
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        // Direct buffer access to avoid recursive locking
        if (x0 >= 0 && x0 < OLED_WIDTH && y0 >= 0 && y0 < OLED_HEIGHT) {
            int index = y0 * OLED_WIDTH + x0;
            oledBuffer[index] = on ? 255 : 0;
        }
        
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
    updateOledTexture();
}

void DesktopDisplay::oledDrawText(const std::string& text, int x, int y, int /*size*/) {
    std::lock_guard<std::mutex> lock(oledMutex);
    if (!font || text.empty()) return;
    SDL_Color fg{255,255,255,255}, bg{0,0,0,255};
    SDL_Surface* s = TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg);
    if (!s) { std::cerr << "[TTF] OLED text failed: " << TTF_GetError() << "\n"; return; }

    // Clear a rough area first (avoid smearing)
    int estW = static_cast<int>(text.size()) * 10; // rough
    for (int yy = 0; yy < 12 && y + yy < OLED_HEIGHT; ++yy) {
        for (int xx = 0; xx < estW && x + xx < OLED_WIDTH; ++xx) {
            if (x + xx >= 0 && y + yy >= 0) {
                int index = (y + yy) * OLED_WIDTH + (x + xx);
                oledBuffer[index] = 0;
            }
        }
    }

    SDL_LockSurface(s);
    for (int dy = 0; dy < s->h; ++dy) {
        for (int dx = 0; dx < s->w; ++dx) {
            int fx = x + dx, fy = y + dy;
            if (fx < 0 || fy < 0 || fx >= OLED_WIDTH || fy >= OLED_HEIGHT) continue;
            
            uint8_t* pixel = static_cast<uint8_t*>(s->pixels) + dy * s->pitch + dx * s->format->BytesPerPixel;
            uint8_t r, g, b;
            SDL_GetRGB(*reinterpret_cast<uint32_t*>(pixel), s->format, &r, &g, &b);
            
            // Use luminance to determine if pixel should be on
            uint8_t luminance = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
            if (luminance > 128) {
                oledBuffer[fy * OLED_WIDTH + fx] = 255;
            }
        }
    }
    SDL_UnlockSurface(s);
    SDL_FreeSurface(s);
    updateOledTexture();
}

// Internal helper that assumes oledMutex is already held
void DesktopDisplay::oledDrawTextUnlocked(const std::string& text, int x, int y, int /*size*/) {
    if (!font || text.empty()) return;
    SDL_Color fg{255,255,255,255}, bg{0,0,0,255};
    SDL_Surface* s = TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg);
    if (!s) { std::cerr << "[TTF] OLED text failed: " << TTF_GetError() << "\n"; return; }

    // Clear a rough area first (avoid smearing)
    int estW = static_cast<int>(text.size()) * 10; // rough
    for (int yy = 0; yy < 12 && y + yy < OLED_HEIGHT; ++yy) {
        for (int xx = 0; xx < estW && x + xx < OLED_WIDTH; ++xx) {
            if (x + xx >= 0 && y + yy >= 0) {
                int index = (y + yy) * OLED_WIDTH + (x + xx);
                oledBuffer[index] = 0;
            }
        }
    }

    SDL_LockSurface(s);
    for (int dy = 0; dy < s->h; ++dy) {
        for (int dx = 0; dx < s->w; ++dx) {
            int fx = x + dx, fy = y + dy;
            if (fx < 0 || fy < 0 || fx >= OLED_WIDTH || fy >= OLED_HEIGHT) continue;
            
            uint8_t* pixel = static_cast<uint8_t*>(s->pixels) + dy * s->pitch + dx * s->format->BytesPerPixel;
            uint8_t r, g, b;
            SDL_GetRGB(*reinterpret_cast<uint32_t*>(pixel), s->format, &r, &g, &b);
            
            // Use luminance to determine if pixel should be on
            uint8_t luminance = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
            if (luminance > 128) {
                oledBuffer[fy * OLED_WIDTH + fx] = 255;
            }
        }
    }
    SDL_UnlockSurface(s);
    SDL_FreeSurface(s);
    // Note: Do NOT call updateOledTexture() here - caller will do it
}

void DesktopDisplay::renderOledText(const std::string& line1, const std::string& line2, const std::string& line3) {
    std::lock_guard<std::mutex> lk(oledMutex);
    // Clear OLED first
    std::fill(oledBuffer.begin(), oledBuffer.end(), 0);
    
    // Draw all three lines atomically
    if (!line1.empty()) oledDrawTextUnlocked(line1, 0,  0, 8);
    if (!line2.empty()) oledDrawTextUnlocked(line2, 0, 11, 8);
    if (!line3.empty()) oledDrawTextUnlocked(line3, 0, 22, 8);
    
    updateOledTexture();
}

void DesktopDisplay::oledDrawRect(int x, int y, int w, int h, bool filled, bool on) {
    std::lock_guard<std::mutex> lock(oledMutex);
    if (filled) {
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                // Direct buffer access to avoid recursive locking
                int px = x + xx, py = y + yy;
                if (px >= 0 && px < OLED_WIDTH && py >= 0 && py < OLED_HEIGHT) {
                    int index = py * OLED_WIDTH + px;
                    oledBuffer[index] = on ? 255 : 0;
                }
            }
        }
    } else {
        // Draw outline using direct buffer access
        for (int xx = 0; xx < w; ++xx) {
            if (x + xx >= 0 && x + xx < OLED_WIDTH) {
                if (y >= 0 && y < OLED_HEIGHT) oledBuffer[y * OLED_WIDTH + (x + xx)] = on ? 255 : 0;
                if (y + h - 1 >= 0 && y + h - 1 < OLED_HEIGHT) oledBuffer[(y + h - 1) * OLED_WIDTH + (x + xx)] = on ? 255 : 0;
            }
        }
        for (int yy = 0; yy < h; ++yy) {
            if (y + yy >= 0 && y + yy < OLED_HEIGHT) {
                if (x >= 0 && x < OLED_WIDTH) oledBuffer[(y + yy) * OLED_WIDTH + x] = on ? 255 : 0;
                if (x + w - 1 >= 0 && x + w - 1 < OLED_WIDTH) oledBuffer[(y + yy) * OLED_WIDTH + (x + w - 1)] = on ? 255 : 0;
            }
        }
    }
    updateOledTexture();
}

void DesktopDisplay::oledUpdate() {
    std::lock_guard<std::mutex> lock(oledMutex);
    updateOledTexture();
}

// Input handling
bool DesktopDisplay::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:        lastKey = 19; break; // PM arrows mapping
                case SDLK_DOWN:      lastKey = 21; break;
                case SDLK_LEFT:      lastKey = 20; break;
                case SDLK_RIGHT:     lastKey = 18; break;
                case SDLK_ESCAPE:    lastKey = 27; break;
                case SDLK_HOME:      lastKey = 12; break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:  lastKey = 13; break;
                case SDLK_BACKSPACE: lastKey = 8;  break;
                default:             lastKey = (char)e.key.keysym.sym; break;
            }
            keyPressed[e.key.keysym.scancode] = true;
        } else if (e.type == SDL_KEYUP) {
            keyPressed[e.key.keysym.scancode] = false;
        } else if (e.type == SDL_WINDOWEVENT) {
            // Repaint on expose/resize
            if (e.window.event == SDL_WINDOWEVENT_EXPOSED ||
                e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                updateEinkTexture();
                updateOledTexture();
            }
        }
    }
    return true;
}

char DesktopDisplay::getLastKey() { char k = lastKey; lastKey = 0; return k; }
bool DesktopDisplay::isKeyPressed(SDL_Scancode sc) { return keyPressed[sc]; }

// ---- Present / Uploads ----
void DesktopDisplay::present() {
    std::lock_guard<std::mutex> lock(displayMutex);
    
    // Clear both windows
    SDL_SetRenderDrawColor(einkRenderer, 255, 255, 255, 255);
    SDL_RenderClear(einkRenderer);
    
    SDL_SetRenderDrawColor(oledRenderer, 0, 0, 0, 255);
    SDL_RenderClear(oledRenderer);
    
    // Render E-Ink display
    SDL_RenderCopy(einkRenderer, einkTexture, nullptr, nullptr);
    
    // Render OLED display
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    
    // Present both displays
    SDL_RenderPresent(einkRenderer);
    SDL_RenderPresent(oledRenderer);
}

SDL_Color DesktopDisplay::getEinkColor(bool black) { return black ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255}; }
SDL_Color DesktopDisplay::getOledColor(bool on)    { return on    ? SDL_Color{255,255,255,255} : SDL_Color{0,0,0,255}; }



void DesktopDisplay::updateEinkTexture() {
    // Note: This function is called from within eink mutex locks, so no additional locking needed
    if (!einkTexture || !einkRenderer) {
        std::cerr << "[SDL2] Invalid E-Ink texture or renderer" << std::endl;
        return;
    }
    
    // Use macOS approach: SDL_LockTexture with inline conversion (proven stable)
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
            // For ARGB8888 format: Alpha, Red, Green, Blue
            row[x] = (0xFF << 24) | (gray << 16) | (gray << 8) | gray;
        }
        row = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(row) + pitch);
    }
    SDL_UnlockTexture(einkTexture);
    
    // Only update the texture - rendering is done in present()
}

void DesktopDisplay::updateOledTexture() {
    if (!oledTexture || !oledRenderer) return;
    
    // Update texture from buffer (match macOS approach exactly)
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
    
    // Present OLED window (match macOS approach)
    SDL_SetRenderTarget(oledRenderer, nullptr);
    SDL_Rect oledViewport = {0, 0, OLED_WIDTH * 3, OLED_HEIGHT * 3};
    SDL_RenderSetViewport(oledRenderer, &oledViewport);
    
    SDL_SetRenderDrawColor(oledRenderer, 128, 128, 128, 255);
    SDL_RenderClear(oledRenderer);
    SDL_RenderCopy(oledRenderer, oledTexture, nullptr, nullptr);
    SDL_RenderPresent(oledRenderer);
}
