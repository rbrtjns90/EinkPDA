#pragma once

#include "globals.h"
#include "PokedexUI.h"

// Graphics adapter implementation for PocketMage
class PocketMageGraphics : public IGraphics {
public:
  int screenW() const override;
  int screenH() const override;
  void fillRect(int x, int y, int w, int h, uint16_t color) override;
  void drawRect(int x, int y, int w, int h, uint16_t color) override;
  void drawText(int x, int y, const String& text, uint16_t color) override;
  void drawSprite(int x, int y, const uint8_t* data, int w, int h) override;
  void flushPartial(int x, int y, int w, int h) override;
  void setFont(int size) override;
};

// Initialize the new Pokedex UI system
void initializeNewPokedexUI();

// Access functions for external use
PocketMageGraphics& getGraphicsAdapter();
SpriteCache& getSpriteCache();
std::vector<DexMon>& getPokemonData();
DexState& getDexStateRef();

// Sprite loader for the cache system
bool loadPokemonSprite4bpp(uint16_t id, uint8_t* out, int stride, int w, int h);

// Data conversion utilities
void buildDexMonData();
