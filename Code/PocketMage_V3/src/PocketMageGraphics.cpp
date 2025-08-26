#include "PocketMageGraphics.h"
#include "globals.h"
#include <algorithm>
#include <iostream>

// Forward declarations
struct Pokemon {
  uint16_t id;
  String name;
  String types;
  String genus;
  String flavor_text;
  uint16_t height_cm;
  uint16_t weight_hg;
  uint16_t stats[6]; // HP, ATK, DEF, SpA, SpD, SPE
  String image_file;
};

extern std::vector<Pokemon> pokemonList;
bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize);

// Graphics adapter implementation for PocketMage
int PocketMageGraphics::screenW() const {
  return display.width();
}

int PocketMageGraphics::screenH() const {
  return display.height();
}

void PocketMageGraphics::fillRect(int x, int y, int w, int h, uint16_t color) {
  display.fillRect(x, y, w, h, color);
}

void PocketMageGraphics::drawRect(int x, int y, int w, int h, uint16_t color) {
  display.drawRect(x, y, w, h, color);
}

void PocketMageGraphics::drawText(int x, int y, const String& text, uint16_t color) {
  display.setCursor(x, y);
  display.setTextColor(color);
  display.print(text);
}

void PocketMageGraphics::drawSprite(int x, int y, const uint8_t* data, int w, int h) {
  // Validate input parameters
  if (!data || w <= 0 || h <= 0) {
    std::cout << "[GRAPHICS] Invalid sprite data or dimensions" << std::endl;
    return;
  }
  
  // Bounds check for screen coordinates
  if (x >= display.width() || y >= display.height() || x + w < 0 || y + h < 0) {
    std::cout << "[GRAPHICS] Sprite outside screen bounds" << std::endl;
    return;
  }
  
  // Convert 4bpp sprite data to 1bpp for E-Ink display
  int stride = (w + 1) / 2; // Bytes per row in 4bpp format
  int maxDataSize = stride * h;
  
  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col += 2) {
      int srcIndex = row * stride + col / 2;
      
      // Bounds check for data access
      if (srcIndex >= maxDataSize) {
        std::cout << "[GRAPHICS] Data access out of bounds at index " << srcIndex << std::endl;
        continue;
      }
      
      uint8_t srcByte = data[srcIndex];
      
      // Extract two 4bpp pixels (low nibble first, then high nibble)
      uint8_t pixel1 = srcByte & 0xF;
      uint8_t pixel2 = (srcByte >> 4) & 0xF;
      
      // Convert to 1bpp (threshold at 8)
      bool bit1 = pixel1 > 8;
      bool bit2 = (col + 1 < w) ? (pixel2 > 8) : false;
      
      // Draw pixels with bounds checking
      int pixelX1 = x + col;
      int pixelY1 = y + row;
      int pixelX2 = x + col + 1;
      
      if (pixelX1 >= 0 && pixelX1 < display.width() && pixelY1 >= 0 && pixelY1 < display.height()) {
        display.drawPixel(pixelX1, pixelY1, bit1 ? GxEPD_BLACK : GxEPD_WHITE);
      }
      
      if (col + 1 < w && pixelX2 >= 0 && pixelX2 < display.width() && pixelY1 >= 0 && pixelY1 < display.height()) {
        display.drawPixel(pixelX2, pixelY1, bit2 ? GxEPD_BLACK : GxEPD_WHITE);
      }
    }
  }
}

void PocketMageGraphics::flushPartial(int x, int y, int w, int h) {
  // Prevent Metal encoder issues by severely limiting display updates
  static unsigned long lastFlush = 0;
  static bool flushing = false;
  unsigned long now = millis();
  
  // Only allow updates every 1 second to prevent Metal encoder issues
  if (flushing || now - lastFlush < 1000) {
    return;
  }
  
  flushing = true;
  lastFlush = now;
  
  // Skip display updates entirely to prevent Metal encoder errors
  // The display will be updated through other mechanisms
  
  flushing = false;
}

void PocketMageGraphics::setFont(int size) {
  switch (size) {
    case 0: // Small
      display.setFont(&FreeSans9pt7b);
      break;
    case 1: // Regular
      display.setFont(&FreeMonoBold9pt7b);
      break;
    case 2: // Bold
      display.setFont(&FreeMonoBold9pt7b);
      break;
    default:
      display.setFont(&FreeSans9pt7b);
      break;
  }
}

// Global instances for the new UI system
static PocketMageGraphics gfx;
static SpriteCache spriteCache(24);
static std::vector<DexMon> pokemonData;
static DexState dexState;

// Sprite loader function for the cache
bool loadPokemonSprite4bpp(uint16_t id, uint8_t* out, int stride, int w, int h) {
  // Validate input parameters
  if (!out || stride <= 0 || w <= 0 || h <= 0) {
    std::cout << "[SPRITE] Invalid parameters for sprite loading" << std::endl;
    return false;
  }
  
  // Load the existing 1bpp sprite and convert to 4bpp
  uint8_t spriteBuffer[512]; // 64*64/8 = 512 bytes for 1-bit bitmap
  
  if (!loadPokemonSprite(id, spriteBuffer, sizeof(spriteBuffer))) {
    std::cout << "[SPRITE] Failed to load sprite for Pokemon ID " << id << std::endl;
    return false;
  }
  
  int maxSrcBytes = sizeof(spriteBuffer);
  int maxDstBytes = stride * h;
  
  // Convert 1bpp to 4bpp
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x += 2) {
      // Calculate source byte and bit positions correctly
      int pixel1Pos = y * w + x;
      int pixel2Pos = y * w + x + 1;
      
      int srcByteIndex1 = pixel1Pos / 8;
      int srcBitIndex1 = pixel1Pos % 8;
      
      // Bounds check for source buffer
      if (srcByteIndex1 >= maxSrcBytes) {
        std::cout << "[SPRITE] Source buffer overflow at index " << srcByteIndex1 << std::endl;
        continue;
      }
      
      uint8_t srcByte1 = spriteBuffer[srcByteIndex1];
      bool pixel1 = (srcByte1 & (1 << (7 - srcBitIndex1))) != 0;
      
      bool pixel2 = false;
      if (x + 1 < w) {
        int srcByteIndex2 = pixel2Pos / 8;
        int srcBitIndex2 = pixel2Pos % 8;
        
        if (srcByteIndex2 < maxSrcBytes) {
          uint8_t srcByte2 = spriteBuffer[srcByteIndex2];
          pixel2 = (srcByte2 & (1 << (7 - srcBitIndex2))) != 0;
        }
      }
      
      uint8_t gray1 = pixel1 ? 15 : 0; // White or black in 4bpp
      uint8_t gray2 = pixel2 ? 15 : 0;
      
      int dstIndex = (y * stride + x) / 2;
      
      // Bounds check for destination buffer
      if (dstIndex >= maxDstBytes) {
        std::cout << "[SPRITE] Destination buffer overflow at index " << dstIndex << std::endl;
        continue;
      }
      
      out[dstIndex] = (gray1 & 0xF) | ((gray2 & 0xF) << 4);
    }
  }
  
  return true;
}

// Convert existing Pokemon data to new format
void buildDexMonData() {
  pokemonData.clear();
  
  for (const Pokemon& pokemon : pokemonList) {
    DexMon mon;
    mon.id = pokemon.id;
    mon.nameLower = std::string(pokemon.name.c_str());
    std::transform(mon.nameLower.begin(), mon.nameLower.end(), mon.nameLower.begin(), ::tolower);
    
    // Convert type string to type mask
    mon.typeMask = TypeSystem::stringToTypeMask(pokemon.types);
    if (mon.typeMask == 0) mon.typeMask = TypeSystem::NORMAL; // Default fallback
    
    // Estimate generation from ID (rough approximation)
    if (mon.id <= 151) mon.gen = 1;
    else if (mon.id <= 251) mon.gen = 2;
    else if (mon.id <= 386) mon.gen = 3;
    else if (mon.id <= 493) mon.gen = 4;
    else if (mon.id <= 649) mon.gen = 5;
    else if (mon.id <= 721) mon.gen = 6;
    else if (mon.id <= 809) mon.gen = 7;
    else if (mon.id <= 905) mon.gen = 8;
    else mon.gen = 9;
    
    // Copy stats
    for (int i = 0; i < 6; i++) {
      mon.stats[i] = pokemon.stats[i];
    }
    
    mon.favorite = false; // TODO: implement favorites system
    
    pokemonData.push_back(mon);
  }
  
  std::cout << "[POKEDEX] Built DexMon data for " << pokemonData.size() << " Pokemon" << std::endl;
}

// Initialize the new UI system
void initializeNewPokedexUI() {
  // Set up sprite cache
  spriteCache.setLoader(loadPokemonSprite4bpp);
  
  // Build Pokemon data from existing system
  buildDexMonData();
  
  // Initialize state
  dexState.view = DexView::List;
  dexState.selected = 0;
  dexState.scroll = 0;
  dexState.tab = DetailTab::Info;
  
  // Ensure we have Pokemon data
  if (pokemonData.empty()) {
    std::cout << "[POKEDEX] Warning: No Pokemon data loaded" << std::endl;
  } else {
    std::cout << "[POKEDEX] Loaded " << pokemonData.size() << " Pokemon" << std::endl;
  }
  
  // Apply initial filters to populate filteredIndex
  PokedexUI::refreshFilterAndSort(dexState, pokemonData);
  
  std::cout << "[POKEDEX] Filtered index size: " << dexState.filteredIndex.size() << std::endl;
  std::cout << "[POKEDEX] New UI system initialized" << std::endl;
}

// Access functions for external use
PocketMageGraphics& getGraphicsAdapter() {
  return gfx;
}

SpriteCache& getSpriteCache() {
  return spriteCache;
}

std::vector<DexMon>& getPokemonData() {
  return pokemonData;
}

DexState& getDexStateRef() {
  return dexState;
}
