// Main UI Functions
#include "PokedexUI.h"
#include "PocketMageGraphics.h"
#include <string>
#include <algorithm>
#include <cmath>
#include <vector>
#include <stdint.h>

// Simplified TypeSystem for compilation
namespace TypeSystem {
  const char* getTypeName(Type type) {
    switch (type) {
      case NORMAL: return "Normal";
      case FIRE: return "Fire";
      case WATER: return "Water";
      case ELECTRIC: return "Electric";
      case GRASS: return "Grass";
      case ICE: return "Ice";
      case FIGHTING: return "Fighting";
      case POISON: return "Poison";
      case GROUND: return "Ground";
      case FLYING: return "Flying";
      case PSYCHIC: return "Psychic";
      case BUG: return "Bug";
      case ROCK: return "Rock";
      case GHOST: return "Ghost";
      case DRAGON: return "Dragon";
      case DARK: return "Dark";
      case STEEL: return "Steel";
      case FAIRY: return "Fairy";
      default: return "Unknown";
    }
  }
  
  uint16_t getTypeGray(Type type) {
    // Return different grays for different types
    switch (type) {
      case FIRE: return Gray::Dark;
      case WATER: return Gray::Medium;
      case ELECTRIC: return Gray::Light;
      case GRASS: return Gray::Medium;
      case PSYCHIC: return Gray::Light;
      case DARK: return Gray::Black;
      default: return Gray::Medium;
    }
  }
  
  uint32_t stringToTypeMask(const String& typeStr) {
    // Simple implementation - just return NORMAL for now
    return NORMAL;
  }
}

// SpriteCache implementation
SpriteCache::SpriteCache(int maxEntries) : maxEntries(maxEntries), accessCounter(0) {
  cache.resize(maxEntries);
  for (auto& entry : cache) {
    entry.valid = false;
    entry.data64 = nullptr;
    entry.data32 = nullptr;
    entry.lastUsed = 0;
  }
}

SpriteCache::~SpriteCache() {}

void SpriteCache::setLoader(bool (*loaderFunc)(uint16_t, uint8_t*, int, int, int)) {
  spriteLoader = loaderFunc;
}

void SpriteCache::evictLRU() {
  // Simple stub - do nothing for now
}

void SpriteCache::downscale64to32(const uint8_t* src, uint8_t* dst) {
  // Simple stub - do nothing for now
}

const uint8_t* SpriteCache::get32(uint16_t id) {
  // Validate ID range
  if (id == 0 || id > 151) {  // Gen 1 Pokemon only
    std::cout << "[CACHE] Invalid Pokemon ID: " << id << std::endl;
    return nullptr;
  }
  
  // Check if already cached
  for (auto& entry : cache) {
    if (entry.valid && entry.id == id && entry.data32) {
      entry.lastUsed = ++accessCounter;
      return entry.data32;
    }
  }
  
  // Load actual sprite from pokemon_sprites.bin file (sprites are 64x64)
  static uint8_t spriteBuffer1bpp[64 * 64 / 8]; // 64x64 1-bit bitmap from file = 512 bytes
  static uint8_t spriteBuffer4bpp[32 * 32 / 2]; // 32x32 4bpp bitmap for display
  
  // Load 64x64 1-bit sprite from binary file
  extern bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize);
  if (!loadPokemonSprite(id, spriteBuffer1bpp, sizeof(spriteBuffer1bpp))) {
    std::cout << "[CACHE] Failed to load sprite for Pokemon " << id << std::endl;
    return nullptr;
  }
  
  // Downscale from 64x64 to 32x32 and convert 1bpp to 4bpp format
  memset(spriteBuffer4bpp, 0, sizeof(spriteBuffer4bpp));
  int stride = (32 + 1) / 2; // Bytes per row in 4bpp format
  
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
      // Sample from 64x64 source (2x2 downscaling)
      int srcY = y * 2;
      int srcX = x * 2;
      int srcByteIndex = (srcY * 64 + srcX) / 8;
      int srcBitIndex = (srcY * 64 + srcX) % 8;
      bool pixel = (spriteBuffer1bpp[srcByteIndex] >> (7 - srcBitIndex)) & 1;
      
      // Invert the pixel logic - 0 bit = black pixel, 1 bit = white/transparent
      if (!pixel) {
        int dstByteIndex = y * stride + x / 2;
        if (x % 2 == 0) {
          spriteBuffer4bpp[dstByteIndex] |= 0x0F; // Low nibble - black pixel
        } else {
          spriteBuffer4bpp[dstByteIndex] |= 0xF0; // High nibble - black pixel
        }
      }
    }
  }
  
  return spriteBuffer4bpp;
}

const uint8_t* SpriteCache::get64(uint16_t id) {
  // Validate ID range
  if (id == 0 || id > 151) {  // Gen 1 Pokemon only
    std::cout << "[CACHE] Invalid Pokemon ID: " << id << std::endl;
    return nullptr;
  }
  
  // Check if already cached
  for (auto& entry : cache) {
    if (entry.valid && entry.id == id && entry.data64) {
      entry.lastUsed = ++accessCounter;
      return entry.data64;
    }
  }
  
  // Load actual sprite from pokemon_sprites.bin file
  static uint8_t spriteBuffer1bpp[64 * 64 / 8]; // 64x64 1-bit bitmap from file
  static uint8_t spriteBuffer4bpp[64 * 64 / 2]; // 64x64 4bpp bitmap for display
  
  // Load 1-bit sprite from binary file
  extern bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize);
  if (!loadPokemonSprite(id, spriteBuffer1bpp, sizeof(spriteBuffer1bpp))) {
    std::cout << "[CACHE] Failed to load 64x64 sprite for Pokemon " << id << std::endl;
    return nullptr;
  }
  
  // Convert 1bpp to 4bpp format
  memset(spriteBuffer4bpp, 0, sizeof(spriteBuffer4bpp));
  int stride = (64 + 1) / 2; // Bytes per row in 4bpp format
  
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      // Extract 1-bit pixel from source
      int srcByteIndex = (y * 64 + x) / 8;
      int srcBitIndex = (y * 64 + x) % 8;
      bool pixel = (spriteBuffer1bpp[srcByteIndex] >> (7 - srcBitIndex)) & 1;
      
      // Convert to 4bpp format - invert pixel logic
      if (!pixel) {
        int dstByteIndex = y * stride + x / 2;
        if (x % 2 == 0) {
          spriteBuffer4bpp[dstByteIndex] |= 0x0F; // Low nibble - black pixel
        } else {
          spriteBuffer4bpp[dstByteIndex] |= 0xF0; // High nibble - black pixel
        }
      }
    }
  }
  
  return spriteBuffer4bpp;
}

void SpriteCache::preload(uint16_t id) {
  // Simple stub - do nothing for now
}

// SearchModel implementation
void SearchModel::applyFilters(const std::vector<DexMon>& mons, const DexFilters& filters, std::vector<int>& indices) {
  indices.clear();
  for (int i = 0; i < (int)mons.size(); i++) {
    bool matches = true;
    
    // Text filter
    if (!filters.query.empty()) {
      if (mons[i].nameLower.find(filters.query) == std::string::npos) {
        matches = false;
      }
    }
    
    // Type filter
    if (filters.typeMask != 0 && (mons[i].typeMask & filters.typeMask) == 0) {
      matches = false;
    }
    
    // Generation filter
    if (mons[i].gen < filters.genMin || mons[i].gen > filters.genMax) {
      matches = false;
    }
    
    if (matches) {
      indices.push_back(i);
    }
  }
}

void SearchModel::sortIndices(std::vector<int>& indices, const std::vector<DexMon>& mons, int sortType) {
  switch (sortType) {
    case 0: // Sort by ID
      std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return mons[a].id < mons[b].id;
      });
      break;
    case 1: // Sort by name
      std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return mons[a].nameLower < mons[b].nameLower;
      });
      break;
    default:
      // Default to ID sort
      std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return mons[a].id < mons[b].id;
      });
      break;
  }
}

bool SearchModel::matchesQuery(const DexMon& mon, const std::string& query) {
  if (query.empty()) return true;
  return mon.nameLower.find(query) != std::string::npos;
}

bool SearchModel::matchesFilters(const DexMon& mon, const DexFilters& filters) {
  // Text filter
  if (!filters.query.empty() && !matchesQuery(mon, filters.query)) {
    return false;
  }
  
  // Type filter
  if (filters.typeMask != 0 && (mon.typeMask & filters.typeMask) == 0) {
    return false;
  }
  
  // Generation filter
  if (mon.gen < filters.genMin || mon.gen > filters.genMax) {
    return false;
  }
  
  // Favorites filter
  if (filters.favoritesOnly && !mon.favorite) {
    return false;
  }
  
  // Stat filters
  for (int i = 0; i < 6; i++) {
    if (mon.stats[i] < filters.statMin[i] || mon.stats[i] > filters.statMax[i]) {
      return false;
    }
  }
  
  return true;
}

// StatChart implementation
void StatChart::drawRadar(IGraphics& gfx, int x, int y, int size, const uint16_t* stats) {
  // Simple radar chart implementation
  int centerX = x + size / 2;
  int centerY = y + size / 2;
  int radius = size / 2 - 10;
  
  // Draw hexagon outline
  for (int i = 0; i < 6; i++) {
    float angle1 = (i * 60.0f - 90.0f) * 3.14159f / 180.0f;
    float angle2 = ((i + 1) * 60.0f - 90.0f) * 3.14159f / 180.0f;
    
    int x1 = centerX + (int)(cos(angle1) * radius);
    int y1 = centerY + (int)(sin(angle1) * radius);
    int x2 = centerX + (int)(cos(angle2) * radius);
    int y2 = centerY + (int)(sin(angle2) * radius);
    
    // Draw line (simplified - just draw a rectangle)
    gfx.fillRect(x1, y1, 2, 2, Gray::Black);
    gfx.fillRect(x2, y2, 2, 2, Gray::Black);
  }
  
  // Draw stat values
  for (int i = 0; i < 6; i++) {
    float angle = (i * 60.0f - 90.0f) * 3.14159f / 180.0f;
    int statRadius = (stats[i] * radius) / 255;
    
    int statX = centerX + (int)(cos(angle) * statRadius);
    int statY = centerY + (int)(sin(angle) * statRadius);
    
    gfx.fillRect(statX - 1, statY - 1, 3, 3, Gray::Dark);
  }
}

void StatChart::drawMiniBar(IGraphics& gfx, int x, int y, int w, int value, int maxValue, const char* label) {
  // Draw label
  gfx.setFont(0);
  gfx.drawText(x, y + 8, String(label), Gray::Black);
  
  // Draw bar background
  gfx.fillRect(x + 30, y, w - 30, 8, Gray::Light);
  
  // Draw bar fill
  int fillWidth = ((w - 30) * value) / maxValue;
  gfx.fillRect(x + 30, y, fillWidth, 8, Gray::Dark);
  
  // Draw value text
  gfx.drawText(x + w - 20, y + 8, String(value), Gray::Black);
}

namespace PokedexUI {
  void drawBreadcrumb(IGraphics& gfx, const DexState& state) {
    gfx.setFont(0); // Small font
    String breadcrumb = "Pokedex";
    
    switch (state.view) {
      case DexView::List:
        breadcrumb += " > List";
        break;
      case DexView::Detail:
        breadcrumb += " > Detail";
        break;
      case DexView::Search:
        breadcrumb += " > Search";
        break;
      case DexView::Compare:
        breadcrumb += " > Compare";
        break;
    }
    
    // Don't fill background - just draw text directly
    gfx.drawText(Layout::padding, 12, breadcrumb, Gray::Black);
  }
  
  void drawTypeChip(IGraphics& gfx, int x, int y, TypeSystem::Type type) {
    const char* name = TypeSystem::getTypeName(type);
    uint16_t color = TypeSystem::getTypeGray(type);
    
    int chipW = strlen(name) * 6 + 8; // Approximate width
    int chipH = 16;
    
    // Draw rounded rectangle (approximate with regular rect)
    gfx.fillRect(x, y, chipW, chipH, color);
    gfx.drawRect(x, y, chipW, chipH, Gray::Black);
    
    // Draw text
    gfx.setFont(0); // Small font
    uint16_t textColor = (color == Gray::Black || color == Gray::Dark) ? Gray::White : Gray::Black;
    gfx.drawText(x + 4, y + 12, String(name), textColor);
  }
  
  void drawPokemonGrid(IGraphics& gfx, const DexState& state, const std::vector<DexMon>& mons, SpriteCache& cache) {
    drawBreadcrumb(gfx, state);
    
    int cellW = gfx.screenW() / 2;
    int startY = Layout::topY + 10;
    int itemsPerRow = 2;
    int visibleRows = (gfx.screenH() - startY - 30) / Layout::cellH;
    int maxVisibleItems = visibleRows * itemsPerRow;
    
    // Calculate which items to show based on selection
    int selectedRow = state.selected / itemsPerRow;
    int startRow = std::max(0, selectedRow - visibleRows / 2);
    int startIndex = startRow * itemsPerRow;
    int endIndex = std::min((int)state.filteredIndex.size(), startIndex + maxVisibleItems);
    
    for (int i = startIndex; i < endIndex; i++) {
      if (i >= state.filteredIndex.size()) break;
      
      const DexMon& mon = mons[state.filteredIndex[i]];
      
      int row = (i - startIndex) / itemsPerRow;
      int col = (i - startIndex) % itemsPerRow;
      
      int cellX = col * cellW;
      int cellY = startY + row * Layout::cellH;
      
      // Selection highlight - use thick border instead of fill
      if (i == state.selected) {
        gfx.drawRect(cellX + 1, cellY + 1, cellW - 2, Layout::cellH - 2, Gray::Black);
        gfx.drawRect(cellX + 2, cellY + 2, cellW - 4, Layout::cellH - 4, Gray::Black);
      }
      
      // Mini sprite
      const uint8_t* sprite = cache.get32(mon.id);
      if (sprite) {
        gfx.drawSprite(cellX + Layout::padding, cellY + Layout::padding, sprite, 32, 32);
      }
      
      // Pokemon info
      int textX = cellX + Layout::padding + 36;
      int textY = cellY + Layout::padding + 12;
      
      gfx.setFont(1); // Regular font
      String idStr = "#" + String(mon.id);
      if (mon.id < 10) idStr = "#00" + String(mon.id);
      else if (mon.id < 100) idStr = "#0" + String(mon.id);
      
      gfx.drawText(textX, textY, idStr, Gray::Black);
      gfx.drawText(textX, textY + 14, String(mon.nameLower.c_str()), Gray::Black);
      
      // HP bar (mini)
      int hpBarW = 40;
      int hpBarX = cellX + cellW - hpBarW - Layout::padding;
      int hpBarY = cellY + Layout::cellH - 12;
      
      int hpWidth = (hpBarW * mon.stats[0]) / 255;
      gfx.fillRect(hpBarX, hpBarY, hpBarW, 4, Gray::Light);
      gfx.fillRect(hpBarX, hpBarY, hpWidth, 4, Gray::Dark);
    }
    
    // Footer with count
    int footerY = gfx.screenH() - 20;
    gfx.fillRect(0, footerY, gfx.screenW(), 20, Gray::Light);
    gfx.setFont(0);
    String footer = String(state.selected + 1) + " / " + String((int)state.filteredIndex.size());
    gfx.drawText(Layout::padding, gfx.screenH() - 16, footer, Gray::Black);
    
    // Instructions
    String instructions = "↑↓←→ Navigate  ⏎ View  ⌫ Search";
    gfx.drawText(gfx.screenW() - 200, footerY + 14, instructions, Gray::Black);
  }
  
  void drawPokemonDetail(IGraphics& gfx, const DexState& state, const std::vector<DexMon>& mons, SpriteCache& cache) {
    if (state.filteredIndex.empty() || state.selected >= (int)state.filteredIndex.size()) return;
    
    const DexMon& mon = mons[state.filteredIndex[state.selected]];
    
    drawBreadcrumb(gfx, state);
    
    int contentY = Layout::topY + 10;
    
    // Header section with type-based background
    TypeSystem::Type primaryType = (TypeSystem::Type)(mon.typeMask & 0x3FFFF); // Get first type
    uint16_t headerColor = TypeSystem::getTypeGray(primaryType);
    
    // Don't fill header with solid color - just draw border
    gfx.drawRect(0, contentY, gfx.screenW(), Layout::tabHeight, Gray::Black);
    gfx.setFont(2); // Bold font
    
    // Pokemon name and ID
    String title = "#" + String(mon.id) + "  " + String(mon.nameLower.c_str());
    uint16_t titleColor = (headerColor == Gray::Black || headerColor == Gray::Dark) ? Gray::White : Gray::Black;
    gfx.drawText(Layout::padding, contentY + 18, title, titleColor);
    
    contentY += Layout::tabHeight + 10;
    
    // Main content area
    int leftCol = Layout::padding;
    int rightCol = gfx.screenW() / 2 + 10;
    
    // Large sprite on left
    const uint8_t* sprite = cache.get64(mon.id);
    if (sprite) {
      gfx.drawSprite(leftCol, contentY, sprite, 64, 64);
      gfx.drawRect(leftCol - 1, contentY - 1, 66, 66, Gray::Black);
    }
    
    // Type chips next to sprite
    int chipX = leftCol + 70;
    int chipY = contentY + 10;
    
    // Draw type chips for all types in typeMask
    int currentChipY = chipY;
    for (int i = 1; i <= 18; i++) {
      TypeSystem::Type type = (TypeSystem::Type)(1 << (i - 1));
      if (mon.typeMask & type) {
        drawTypeChip(gfx, chipX, currentChipY, type);
        currentChipY += 20;
      }
    }
    
    // Radar chart on right side
    int radarCenterX = rightCol + 60;
    int radarCenterY = contentY + 40;
    StatChart::drawRadar(gfx, radarCenterX, radarCenterY, 50, mon.stats);
    
    // Tab bar
    int tabY = contentY + 90;
    const char* tabNames[] = {"Info", "Stats", "Moves", "Evo", "Loc"};
    int tabWidth = gfx.screenW() / 5;
    
    for (int i = 0; i < 5; i++) {
      int tabX = i * tabWidth;
      bool isSelected = (i == (int)state.tab);
      uint16_t textColor = Gray::Black;
      
      // Draw tab border, fill selected tab
      if (isSelected) {
        gfx.fillRect(tabX, tabY, tabWidth, Layout::tabHeight, Gray::White);
        gfx.drawRect(tabX, tabY, tabWidth, Layout::tabHeight, Gray::Black);
        gfx.drawRect(tabX + 1, tabY + 1, tabWidth - 2, Layout::tabHeight - 2, Gray::Black);
      } else {
        gfx.drawRect(tabX, tabY, tabWidth, Layout::tabHeight, Gray::Black);
      }
      gfx.setFont(0);
      gfx.drawText(tabX + 8, tabY + 16, String(tabNames[i]), textColor);
    }
    
    // Tab content
    int contentAreaY = tabY + Layout::tabHeight + 10;
    int contentAreaH = gfx.screenH() - contentAreaY - 30;
    
    switch (state.tab) {
      case DetailTab::Info: {
        // Info tab - show basic information
        gfx.setFont(1);
        gfx.drawText(leftCol, contentAreaY + 20, "Basic Information", Gray::Black);
        gfx.drawText(leftCol, contentAreaY + 40, "Height: 1.0m", Gray::Black);
        gfx.drawText(leftCol, contentAreaY + 60, "Weight: 10.0kg", Gray::Black);
        break;
      }
      case DetailTab::Stats: {
        // Stats tab - show stat values
        gfx.setFont(1);
        gfx.drawText(leftCol, contentAreaY + 20, "Base Stats", Gray::Black);
        const char* statNames[] = {"HP", "ATK", "DEF", "SpA", "SpD", "SPE"};
        for (int i = 0; i < 6; i++) {
          int y = contentAreaY + 40 + i * 20;
          gfx.drawText(leftCol, y, String(statNames[i]) + ": " + String(mon.stats[i]), Gray::Black);
        }
        break;
      }
      case DetailTab::Moves: {
        // Moves tab - placeholder
        gfx.setFont(1);
        gfx.drawText(leftCol, contentAreaY + 20, "Moves (Coming Soon)", Gray::Black);
        break;
      }
      case DetailTab::Evolution: {
        // Evolution tab - placeholder
        gfx.setFont(1);
        gfx.drawText(leftCol, contentAreaY + 20, "Evolution (Coming Soon)", Gray::Black);
        break;
      }
      case DetailTab::Location: {
        // Location tab - placeholder
        gfx.setFont(1);
        gfx.drawText(leftCol, contentAreaY + 20, "Locations (Coming Soon)", Gray::Black);
        break;
      }
    }
    
    // Footer with navigation
    int footerY = gfx.screenH() - 20;
    gfx.fillRect(0, footerY, gfx.screenW(), 20, Gray::Light);
    gfx.setFont(0);
    String footer = String(state.selected + 1) + " / " + String((int)state.filteredIndex.size());
    gfx.drawText(Layout::padding, gfx.screenH() - 16, footer, Gray::Black);
    
    String instructions = "← → Navigate  ↑↓ Tabs  ⌫ Back";
    gfx.drawText(gfx.screenW() - 180, footerY + 14, instructions, Gray::Black);
  }
  
  void drawSearchScreen(IGraphics& gfx, const DexState& state) {
    drawBreadcrumb(gfx, state);
    
    int contentY = Layout::topY + 10;
    
    gfx.setFont(2); // Bold font
    gfx.drawText(Layout::padding, contentY + 20, "Advanced Search", Gray::Black);
    
    contentY += 40;
    
    // Quick filters row
    gfx.setFont(1);
    gfx.drawText(Layout::padding, contentY, "Quick Filters:", Gray::Black);
    contentY += 20;
    
    const char* quickFilters[] = {"All", "Starters", "Legendaries", "Favorites"};
    int filterWidth = 80;
    
    for (int i = 0; i < 4; i++) {
      int filterX = Layout::padding + i * (filterWidth + 10);
      bool selected = false; // TODO: track selected quick filter
      
      uint16_t bgColor = selected ? Gray::Dark : Gray::Light;
      uint16_t textColor = selected ? Gray::White : Gray::Black;
      
      gfx.fillRect(filterX, contentY, filterWidth, 24, bgColor);
      gfx.drawRect(filterX, contentY, filterWidth, 24, Gray::Black);
      gfx.setFont(0);
      gfx.drawText(filterX + 8, contentY + 16, String(quickFilters[i]), textColor);
    }
    
    contentY += 40;
    
    // Type filters
    gfx.setFont(1);
    gfx.drawText(Layout::padding, contentY, "Types:", Gray::Black);
    contentY += 20;
    
    // Draw type chips in rows
    TypeSystem::Type types[] = {
      TypeSystem::NORMAL, TypeSystem::FIRE, TypeSystem::WATER, TypeSystem::ELECTRIC,
      TypeSystem::GRASS, TypeSystem::ICE, TypeSystem::FIGHTING, TypeSystem::POISON,
      TypeSystem::GROUND, TypeSystem::FLYING, TypeSystem::PSYCHIC, TypeSystem::BUG,
      TypeSystem::ROCK, TypeSystem::GHOST, TypeSystem::DRAGON, TypeSystem::DARK,
      TypeSystem::STEEL, TypeSystem::FAIRY
    };
    
    int chipX = Layout::padding;
    int chipY = contentY;
    int chipsPerRow = 6;
    
    for (int i = 0; i < 18; i++) {
      if (i > 0 && i % chipsPerRow == 0) {
        chipY += 24;
        chipX = Layout::padding;
      }
      
      bool selected = (state.filters.typeMask & types[i]) != 0;
      
      // Modify chip appearance for selection
      if (selected) {
        gfx.fillRect(chipX - 2, chipY - 2, 64, 20, Gray::Dark);
      }
      
      drawTypeChip(gfx, chipX, chipY, types[i]);
      chipX += 70;
    }
    
    contentY = chipY + 40;
    
    // Generation filter
    gfx.setFont(1);
    gfx.drawText(Layout::padding, contentY, "Generation:", Gray::Black);
    String genRange = String(state.filters.genMin) + " - " + String(state.filters.genMax);
    gfx.drawText(Layout::padding + 100, contentY, genRange, Gray::Black);
    
    contentY += 30;
    
    // Search query
    gfx.drawText(Layout::padding, contentY, "Name Search:", Gray::Black);
    contentY += 20;
    
    // Query input box
    gfx.fillRect(Layout::padding, contentY, gfx.screenW() - Layout::padding * 2, 24, Gray::White);
    gfx.drawRect(Layout::padding, contentY, gfx.screenW() - Layout::padding * 2, 24, Gray::Black);
    gfx.setFont(1);
    gfx.drawText(Layout::padding + 5, contentY + 16, String(state.filters.query.c_str()), Gray::Black);
    
    contentY += 40;
    
    // Results count
    gfx.setFont(1);
    String results = "Found: " + String((int)state.filteredIndex.size()) + " Pokemon";
    gfx.drawText(Layout::padding, contentY, results, Gray::Black);
    
    // Footer
    int footerY = gfx.screenH() - 20;
    gfx.fillRect(0, footerY, gfx.screenW(), 20, Gray::Light);
    gfx.setFont(0);
    String instructions = "Type to search  ⏎ View results  ⌫ Clear";
    gfx.drawText(Layout::padding, footerY + 14, instructions, Gray::Black);
  }
  
  // Tab content drawing functions
  void drawInfoTab(IGraphics& gfx, int x, int y, int w, int h, const DexMon& mon) {
    gfx.setFont(1);
    gfx.drawText(x, y + 20, "Genus: Seed Pokemon", Gray::Black); // TODO: get from data
    gfx.drawText(x, y + 40, "Height: 0.7m", Gray::Black); // TODO: get from data
    gfx.drawText(x, y + 60, "Weight: 6.9kg", Gray::Black); // TODO: get from data
    
    // Flavor text (wrapped)
    gfx.setFont(0);
    String flavorText = "A strange seed was planted on its back at birth. The plant sprouts and grows with this Pokemon."; // TODO: get from data
    
    int textY = y + 90;
    int lineHeight = 14;
    int maxWidth = w - 20;
    
    // Simple word wrapping
    String currentLine = "";
    int lineWidth = 0;
    
    for (int i = 0; i < flavorText.length(); i++) {
      char c = flavorText[i];
      currentLine += c;
      lineWidth += 6; // Approximate character width
      
      if (c == ' ' && lineWidth > maxWidth) {
        gfx.drawText(x, textY, currentLine, Gray::Black);
        textY += lineHeight;
        currentLine = "";
        lineWidth = 0;
        if (textY > y + h - lineHeight) break;
      }
    }
    
    if (currentLine.length() > 0 && textY <= y + h - lineHeight) {
      gfx.drawText(x, textY, currentLine, Gray::Black);
    }
  }
  
  void drawStatsTab(IGraphics& gfx, int x, int y, int w, int h, const DexMon& mon) {
    const char* statNames[] = {"HP", "Attack", "Defense", "Sp. Attack", "Sp. Defense", "Speed"};
    
    int statY = y + 20;
    for (int i = 0; i < 6; i++) {
      StatChart::drawMiniBar(gfx, x, statY, 120, mon.stats[i], 255, statNames[i]);
      statY += 25;
    }
    
    // Total stats
    int total = 0;
    for (int i = 0; i < 6; i++) {
      total += mon.stats[i];
    }
    
    gfx.setFont(1);
    gfx.drawText(x, statY + 20, "Total: " + String(total), Gray::Black);
  }
  
  void drawMovesTab(IGraphics& gfx, int x, int y, int w, int h, const DexMon& mon) {
    gfx.setFont(1);
    gfx.drawText(x, y + 20, "Moves (Coming Soon)", Gray::Medium);
    
    // Placeholder move list
    const char* moves[] = {"Tackle", "Growl", "Vine Whip", "Poison Powder"};
    gfx.setFont(0);
    
    for (int i = 0; i < 4; i++) {
      gfx.drawText(x + 20, y + 50 + i * 16, String(moves[i]), Gray::Black);
    }
  }
  
  void drawEvolutionTab(IGraphics& gfx, int x, int y, int w, int h, const DexMon& mon) {
    gfx.setFont(1);
    gfx.drawText(x, y + 20, "Evolution (Coming Soon)", Gray::Medium);
    
    // Placeholder evolution chain
    gfx.setFont(0);
    gfx.drawText(x + 20, y + 50, "Bulbasaur → Ivysaur → Venusaur", Gray::Black);
  }
  
  void drawLocationTab(IGraphics& gfx, int x, int y, int w, int h, const DexMon& mon) {
    gfx.setFont(1);
    gfx.drawText(x, y + 20, "Locations (Coming Soon)", Gray::Medium);
    
    // Placeholder location info
    gfx.setFont(0);
    gfx.drawText(x + 20, y + 50, "Route 1, Pallet Town", Gray::Black);
  }
  
  void refreshFilterAndSort(DexState& state, const std::vector<DexMon>& mons) {
    SearchModel::applyFilters(mons, state.filters, state.filteredIndex);
    SearchModel::sortIndices(state.filteredIndex, mons, state.sort);
    clampSelection(state);
  }
  
  void clampSelection(DexState& state) {
    if (state.filteredIndex.empty()) {
      state.selected = 0;
      return;
    }
    
    if (state.selected >= (int)state.filteredIndex.size()) {
      state.selected = (int)state.filteredIndex.size() - 1;
    }
    
    if (state.selected < 0) {
      state.selected = 0;
    }
  }
  
  void handleNavigation(DexState& state, char key, const std::vector<DexMon>& mons) {
    std::cout << "[POKEDEX_NAV] Handling key: " << (int)key << " in view: " << (int)state.view << std::endl;
    switch (state.view) {
      case DexView::List:
        switch (key) {
          case 19: // UP
            std::cout << "[POKEDEX_NAV] UP arrow pressed" << std::endl;
            if (state.selected >= 2) {
              state.selected -= 2; // Move up one row (2 columns)
            }
            break;
          case 21: // DOWN
            std::cout << "[POKEDEX_NAV] DOWN arrow pressed" << std::endl;
            if (state.selected + 2 < (int)state.filteredIndex.size()) {
              state.selected += 2;
            }
            break;
          case 20: // LEFT
            std::cout << "[POKEDEX_NAV] LEFT arrow pressed" << std::endl;
            if (state.selected > 0) {
              state.selected--;
            }
            break;
          case 18: // RIGHT
            std::cout << "[POKEDEX_NAV] RIGHT arrow pressed" << std::endl;
            if (state.selected + 1 < (int)state.filteredIndex.size()) {
              state.selected++;
            }
            break;
          case 13: // ENTER
            state.view = DexView::Detail;
            break;
          case 8: // BACKSPACE
            state.view = DexView::Search;
            break;
        }
        break;
        
      case DexView::Detail:
        switch (key) {
          case 19: // UP - Previous tab
            state.tab = (DetailTab)((int)state.tab - 1);
            if ((int)state.tab < 0) state.tab = DetailTab::Location;
            break;
          case 21: // DOWN - Next tab
            state.tab = (DetailTab)(((int)state.tab + 1) % 5);
            break;
          case 20: // LEFT - Previous Pokemon
            if (state.selected > 0) state.selected--;
            break;
          case 18: // RIGHT - Next Pokemon
            if (state.selected + 1 < (int)state.filteredIndex.size()) state.selected++;
            break;
          case 8: case 27: // BACKSPACE or ESC
            state.view = DexView::List;
            break;
        }
        break;
        
      case DexView::Search:
        switch (key) {
          case 13: // ENTER - View results
            state.view = DexView::List;
            break;
          case 8: // BACKSPACE - Clear query
            if (!state.filters.query.empty()) {
              state.filters.query.pop_back();
              refreshFilterAndSort(state, mons);
            } else {
              state.view = DexView::List;
            }
            break;
          default:
            if (key >= 32 && key <= 126) { // Printable characters
              state.filters.query += (char)tolower(key);
              refreshFilterAndSort(state, mons);
            }
            break;
        }
        break;
        
      case DexView::Compare:
        // Compare view navigation - simple stub for now
        switch (key) {
          case 8: case 27: // BACKSPACE or ESC
            state.view = DexView::List;
            break;
        }
        break;
    }
    
    clampSelection(state);
  }

// Draw one grid cell (shared by full & partial paths)
static void drawOneCell(IGraphics& gfx,
                        const DexState& state,
                        const DexMon& mon,
                        SpriteCache& cache,
                        int screenX, int screenY, int cellW, bool selected)
{
  // Cell frame
  if (selected) {
    gfx.drawRect(screenX + 1, screenY + 1, cellW - 2, Layout::cellH - 2, Gray::Black);
    gfx.drawRect(screenX + 2, screenY + 2, cellW - 4, Layout::cellH - 4, Gray::Black);
  } else {
    gfx.drawRect(screenX, screenY, cellW, Layout::cellH, Gray::Black);
  }

  // Mini sprite
  const uint8_t* sprite = cache.get32(mon.id);
  if (sprite) {
    gfx.drawSprite(screenX + Layout::padding, screenY + Layout::padding, sprite, 32, 32);
  }

  // Text
  int textX = screenX + Layout::padding + 36;
  int textY = screenY + Layout::padding + 12;
  gfx.setFont(1);
  String idStr = "#" + String(mon.id);
  if (mon.id < 10) idStr = "#00" + String(mon.id);
  else if (mon.id < 100) idStr = "#0" + String(mon.id);
  gfx.drawText(textX, textY, idStr, Gray::Black);
  gfx.drawText(textX, textY + 14, String(mon.nameLower.c_str()), Gray::Black);

  // Small HP bar
  int hpBarW = 40;
  int hpBarX = screenX + cellW - hpBarW - Layout::padding;
  int hpBarY = screenY + Layout::cellH - 12;
  int hpWidth = (hpBarW * mon.stats[0]) / 255;
  gfx.fillRect(hpBarX, hpBarY, hpBarW, 4, Gray::Light);
  gfx.fillRect(hpBarX, hpBarY, hpWidth, 4, Gray::Dark);
}

// Compute rect for an absolute list index i given the current viewport logic in drawPokemonGrid()
bool gridCellRectForIndex(IGraphics& gfx, const DexState& state, int i, Rect& out) {
  if (i < 0 || i >= (int)state.filteredIndex.size()) return false;

  const int cellW  = gfx.screenW() / 2;
  const int startY = Layout::topY + 10;
  const int itemsPerRow = 2;
  const int visibleRows = (gfx.screenH() - startY - 30) / Layout::cellH;
  const int maxVisibleItems = visibleRows * itemsPerRow;

  // IMPORTANT: drawPokemonGrid() centers the viewport around selection, not a persistent scroll.
  // We must mirror that exactly; otherwise incremental rects would drift.
  const int selectedRow = state.selected / itemsPerRow;
  const int startRow = std::max(0, selectedRow - visibleRows / 2);
  const int startIndex = startRow * itemsPerRow;
  const int endIndex = std::min((int)state.filteredIndex.size(), startIndex + maxVisibleItems);

  if (i < startIndex || i >= endIndex) return false;

  const int row = (i - startIndex) / itemsPerRow;
  const int col = (i - startIndex) % itemsPerRow;
  out.x = col * cellW;
  out.y = startY + row * Layout::cellH;
  out.w = cellW;
  out.h = Layout::cellH;
  return true;
}

// Repaint only prev/current cells; fall back to full redraw when viewport moves
void updateListSelection(IGraphics& gfx,
                         DexState& state,
                         const std::vector<DexMon>& mons,
                         SpriteCache& cache,
                         int prevSelected)
{
  // If list is empty, nothing to do
  if (state.filteredIndex.empty() || state.selected < 0 || state.selected >= (int)state.filteredIndex.size())
    return;

  Rect curR;
  const bool curVis = gridCellRectForIndex(gfx, state, state.selected, curR);

  Rect prevR;
  const bool prevVis = (prevSelected >= 0) && gridCellRectForIndex(gfx, state, prevSelected, prevR);

  // If the viewport changed (e.g., selection moved enough to recenter the grid),
  // our two-rect update isn't sufficient to keep neighboring cells correct.
  // In that case: do a full redraw (exactly what drawPokemonGrid does).
  const bool viewportChanged = (!prevVis && prevSelected >= 0);

  if (viewportChanged) {
    // Full repaint path (safe and consistent)
    drawPokemonGrid(gfx, state, mons, cache);
    // One call is enough; your handler will call refresh() afterwards on hardware,
    // or the emulator will push rows that changed.
    return;
  }

  // Fast path: only prev + current cells

  if (prevVis) {
    // Optional black-scrub on the outgoing cell to reduce ghosting
#ifdef DESKTOP_EMULATOR
    const bool blackScrub = true;
#else
    const bool blackScrub = true;
#endif
    if (blackScrub) {
      gfx.fillRect(prevR.x, prevR.y, prevR.w, prevR.h, Gray::Black);
      gfx.flushPartial(prevR.x, prevR.y, prevR.w, prevR.h);
    }

    // Redraw previous cell as "normal"
    const DexMon& prevMon = mons[state.filteredIndex[prevSelected]];
    gfx.fillRect(prevR.x, prevR.y, prevR.w, prevR.h, Gray::White);
    drawOneCell(gfx, state, prevMon, cache, prevR.x, prevR.y, prevR.w, /*selected=*/false);
    gfx.flushPartial(prevR.x, prevR.y, prevR.w, prevR.h);
  }

  if (curVis) {
    const DexMon& curMon = mons[state.filteredIndex[state.selected]];
    gfx.fillRect(curR.x, curR.y, curR.w, curR.h, Gray::White); // ensure clean background
    drawOneCell(gfx, state, curMon, cache, curR.x, curR.y, curR.w, /*selected=*/true);
    gfx.flushPartial(curR.x, curR.y, curR.w, curR.h);
  }
}

}
