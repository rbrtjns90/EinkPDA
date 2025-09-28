#pragma once

#include "globals.h"
#include <vector>
#include <string>
#include <cstdint>

// UI State & Data Model
enum class DexView { List, Detail, Search, Compare };
enum class DetailTab { Info, Stats, Moves, Evolution, Location };

struct DexFilters {
  uint32_t typeMask = 0;      // OR of allowed types; 0 = all
  int genMin = 1, genMax = 9; // generation range
  bool favoritesOnly = false;
  int statMin[6] = {0};
  int statMax[6] = {255,255,255,255,255,255};
  std::string query;          // lowercase query
};

struct DexState {
  DexView view = DexView::List;
  int selected = 0;           // index in filtered list
  int scroll   = 0;           // pixel scroll for list; content offset for detail
  DetailTab tab = DetailTab::Info;
  int sort = 0;               // 0=ID, 1=Name, 2=Type, 3=Stats
  DexFilters filters;
  std::vector<int> filteredIndex; // visible indices -> mons[]
};

// Lightweight Pokemon data for UI
struct DexMon {
  uint16_t id;
  std::string nameLower;
  uint32_t typeMask;
  int gen;
  uint16_t stats[6]; // HP, ATK, DEF, SpA, SpD, SPE
  bool favorite = false;
};

// Visual System Constants
namespace Gray {
  const uint16_t White = GxEPD_WHITE;
  const uint16_t Light = GxEPD_WHITE; // Light backgrounds
  const uint16_t Medium = GxEPD_WHITE; // Medium backgrounds  
  const uint16_t Dark  = GxEPD_BLACK; // Dark selections/highlights
  const uint16_t Black = GxEPD_BLACK;
}

namespace Layout {
  const int cellH = 44;
  const int topY = 20;
  const int spriteSize = 32;
  const int padding = 6;
  const int tabHeight = 24;
  const int maxItemsPerPage = 8;
}

// Type system
namespace TypeSystem {
  enum Type {
    NONE = 0,
    NORMAL = 1, FIRE = 2, WATER = 4, ELECTRIC = 8, GRASS = 16,
    ICE = 32, FIGHTING = 64, POISON = 128, GROUND = 256, FLYING = 512,
    PSYCHIC = 1024, BUG = 2048, ROCK = 4096, GHOST = 8192, DRAGON = 16384,
    DARK = 32768, STEEL = 65536, FAIRY = 131072
  };
  
  const char* getTypeName(Type type);
  uint16_t getTypeGray(Type type);
  uint32_t stringToTypeMask(const String& typeStr);
}

// Graphics Interface
class IGraphics {
public:
  virtual ~IGraphics() = default;
  virtual int screenW() const = 0;
  virtual int screenH() const = 0;
  virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;
  virtual void drawRect(int x, int y, int w, int h, uint16_t color) = 0;
  virtual void drawText(int x, int y, const String& text, uint16_t color) = 0;
  virtual void drawSprite(int x, int y, const uint8_t* data, int w, int h) = 0;
  virtual void flushPartial(int x, int y, int w, int h) = 0;
  virtual void setFont(int size) = 0; // 0=small, 1=regular, 2=bold
};

// Sprite Cache
class SpriteCache {
private:
  struct CacheEntry {
    uint16_t id;
    uint8_t* data64;  // 64x64 sprite
    uint8_t* data32;  // 32x32 downscaled
    bool valid;
    int lastUsed;
  };
  
  std::vector<CacheEntry> cache;
  int maxEntries;
  int accessCounter;
  
public:
  SpriteCache(int maxEntries = 24);
  ~SpriteCache();
  
  const uint8_t* get64(uint16_t id);
  const uint8_t* get32(uint16_t id);
  void preload(uint16_t id);
  void setLoader(bool (*loader)(uint16_t id, uint8_t* out, int stride, int w, int h));
  
private:
  bool (*spriteLoader)(uint16_t id, uint8_t* out, int stride, int w, int h) = nullptr;
  void evictLRU();
  void downscale64to32(const uint8_t* src, uint8_t* dst);
};

// Search and Filter Model
namespace SearchModel {
  void applyFilters(const std::vector<DexMon>& mons, const DexFilters& filters, std::vector<int>& result);
  void sortIndices(std::vector<int>& indices, const std::vector<DexMon>& mons, int sortType);
  bool matchesQuery(const DexMon& mon, const std::string& query);
  bool matchesFilters(const DexMon& mon, const DexFilters& filters);
}

// Stat Chart Rendering
namespace StatChart {
  void drawRadar(IGraphics& gfx, int centerX, int centerY, int radius, const uint16_t stats[6]);
  void drawMiniBar(IGraphics& gfx, int x, int y, int width, int value, int maxValue, const char* label);
}

// Main UI Functions
namespace PokedexUI {
  struct Rect { int x, y, w, h; };

  void drawBreadcrumb(IGraphics& gfx, const DexState& state);
  void drawPokemonGrid(IGraphics& gfx, const DexState& state, const std::vector<DexMon>& mons, SpriteCache& cache);
  void drawPokemonDetail(IGraphics& gfx, const DexState& state, const std::vector<DexMon>& mons, SpriteCache& cache);
  void drawSearchScreen(IGraphics& gfx, const DexState& state);
  void drawTypeChip(IGraphics& gfx, int x, int y, TypeSystem::Type type);
  
  void refreshFilterAndSort(DexState& state, const std::vector<DexMon>& mons);
  void handleNavigation(DexState& state, char key, const std::vector<DexMon>& mons);
  void clampSelection(DexState& state);

  // Returns on-screen cell rectangle for list index i; returns false if not visible
  bool gridCellRectForIndex(IGraphics& gfx, const DexState& state, int i, Rect& out);

  // Repaint only the cell you left and the cell you entered.
  // prevSelected = -1 means "no previous" (will paint only current).
  void updateListSelection(IGraphics& gfx,
                           DexState& state,
                           const std::vector<DexMon>& mons,
                           SpriteCache& cache,
                           int prevSelected);
}
