//  oooooooo  ooooooooo  oooo   oooo ooooooooooo ooooooooo   ooooooooooo ooooooo  ooooo
//  888    888 888    888 888  o  888 888         888    888  888         888   888  888  
//  888oooo88  888oooo88  888 888 888 888ooooo    888    888  888ooooo     888    888888   
//  888        888    888 888888888  888         888    888  888          888   888  888  
// o888o      o888ooo888  888   o888 ooooooooooo ooooooooo  ooooooooooo  o888o o888o o888o

#include "globals.h"

// Pokemon data structures
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

// Forward declarations
void loadPokemonData();
bool loadBinaryPokemonData();
void loadSamplePokemonData();
String loadStringFromTable(const char* filename, uint16_t index);
bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize);
void drawSprite(int x, int y, const uint8_t* spriteData, int width, int height);
void rebuildSearch();
void updatePokedexOLED();
void drawPokemonList();
void drawPokemonDetail(uint16_t pokemonId);
void drawSearchScreen();
Pokemon* findPokemonById(uint16_t id);

// App state
std::vector<Pokemon> pokemonList;
std::vector<uint16_t> searchResults;
String searchQuery = "";
uint16_t currentIndex = 0;
uint16_t listTop = 0;
bool pokemonDataLoaded = false;

void POKEDEX_INIT() {
  std::cout << "[POCKETMAGE] POKEDEX_INIT() starting..." << std::endl;
  CurrentAppState = POKEDEX;
  CurrentPokedexState = POKE_LIST;
  CurrentKBState = NORMAL;
  newState = true;
  doFull = true;
  
  // Load Pokemon data if not already loaded
  if (!pokemonDataLoaded) {
    loadPokemonData();
  }
  
  // Initialize search with all Pokemon
  searchQuery = "";
  rebuildSearch();
  currentIndex = 0;
  listTop = 0;
  
  std::cout << "[POCKETMAGE] POKEDEX_INIT() complete" << std::endl;
}

void loadPokemonData() {
  std::cout << "[POKEDEX] Loading Pokemon data..." << std::endl;
  
  pokemonList.clear();
  
  // Try to load from binary files
  if (loadBinaryPokemonData()) {
    std::cout << "[POKEDEX] Loaded " << pokemonList.size() << " Pokemon from binary data" << std::endl;
    pokemonDataLoaded = true;
    return;
  }
  
  // Fallback to sample data
  std::cout << "[POKEDEX] Loading sample data..." << std::endl;
  loadSamplePokemonData();
  pokemonDataLoaded = true;
}

void loadSamplePokemonData() {
  // Sample Pokemon data for demonstration
  Pokemon bulbasaur = {1, "Bulbasaur", "Grass/Poison", "Seed Pokemon", 
    "A strange seed was planted on its back at birth. The plant sprouts and grows with this Pokemon.",
    70, 69, {45, 49, 49, 65, 65, 45}, "001_front.png"};
  pokemonList.push_back(bulbasaur);
  
  Pokemon ivysaur = {2, "Ivysaur", "Grass/Poison", "Seed Pokemon",
    "When the bulb on its back grows large, it appears to lose the ability to stand on its hind legs.",
    100, 130, {60, 62, 63, 80, 80, 60}, "002_front.png"};
  pokemonList.push_back(ivysaur);
  
  Pokemon venusaur = {3, "Venusaur", "Grass/Poison", "Seed Pokemon",
    "The flower on its back catches the sun's rays. The larger the flower, the more fragrant it becomes.",
    200, 1000, {80, 82, 83, 100, 100, 80}, "003_front.png"};
  pokemonList.push_back(venusaur);
  
  Pokemon charmander = {4, "Charmander", "Fire", "Lizard Pokemon",
    "Obviously prefers hot places. When it rains, steam is said to spout from the tip of its tail.",
    60, 85, {39, 52, 43, 60, 50, 65}, "004_front.png"};
  pokemonList.push_back(charmander);
  
  Pokemon charmeleon = {5, "Charmeleon", "Fire", "Flame Pokemon",
    "When it swings its burning tail, it elevates the temperature to unbearably hot levels.",
    110, 190, {58, 64, 58, 80, 65, 80}, "005_front.png"};
  pokemonList.push_back(charmeleon);
  
  Pokemon charizard = {6, "Charizard", "Fire/Flying", "Flame Pokemon",
    "Spits fire that is hot enough to melt boulders. Known to cause forest fires unintentionally.",
    170, 905, {78, 84, 78, 109, 85, 100}, "006_front.png"};
  pokemonList.push_back(charizard);
  
  Pokemon squirtle = {7, "Squirtle", "Water", "Tiny Turtle Pokemon",
    "After birth, its back swells and hardens into a shell. Powerfully sprays foam from its mouth.",
    50, 90, {44, 48, 65, 50, 64, 43}, "007_front.png"};
  pokemonList.push_back(squirtle);
  
  Pokemon wartortle = {8, "Wartortle", "Water", "Turtle Pokemon",
    "Often hides in water to stalk unwary prey. For swimming fast, it moves its ears to maintain balance.",
    100, 225, {59, 63, 80, 65, 80, 58}, "008_front.png"};
  pokemonList.push_back(wartortle);
  
  Pokemon blastoise = {9, "Blastoise", "Water", "Shellfish Pokemon",
    "A brutal Pokemon with pressurized water jets on its shell. They are used for high speed tackles.",
    160, 855, {79, 83, 100, 85, 105, 78}, "009_front.png"};
  pokemonList.push_back(blastoise);
  
  Pokemon pikachu = {25, "Pikachu", "Electric", "Mouse Pokemon",
    "When several of these Pokemon gather, their electricity could build and cause lightning storms.",
    40, 60, {35, 55, 40, 50, 50, 90}, "025_front.png"};
  pokemonList.push_back(pikachu);
  
  std::cout << "[POKEDEX] Loaded " << pokemonList.size() << " Pokemon" << std::endl;
}

bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize) {
  // Load sprite from pokemon_sprites.bin file
  FILE* spriteFile = fopen("./data/pokemon/pokemon_sprites.bin", "rb");
  if (!spriteFile) {
    std::cout << "[POKEDEX] Could not open pokemon_sprites.bin" << std::endl;
    return false;
  }
  
  // Read header: count (2 bytes)
  uint16_t spriteCount;
  if (fread(&spriteCount, 2, 1, spriteFile) != 1) {
    std::cout << "[POKEDEX] Error reading sprite count" << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  // Validate Pokemon ID
  if (pokemonId == 0 || pokemonId > spriteCount) {
    std::cout << "[POKEDEX] Invalid Pokemon ID for sprite: " << pokemonId << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  // Read offset for this Pokemon (4 bytes each, 0-indexed)
  uint32_t spriteOffset;
  fseek(spriteFile, 2 + (pokemonId - 1) * 4, SEEK_SET);
  if (fread(&spriteOffset, 4, 1, spriteFile) != 1) {
    std::cout << "[POKEDEX] Error reading sprite offset for Pokemon " << pokemonId << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  // Seek to sprite data location (after header)
  uint32_t dataStart = 2 + spriteCount * 4;
  fseek(spriteFile, dataStart + spriteOffset, SEEK_SET);
  
  // Read sprite size (2 bytes)
  uint16_t spriteSize;
  if (fread(&spriteSize, 2, 1, spriteFile) != 1) {
    std::cout << "[POKEDEX] Error reading sprite size for Pokemon " << pokemonId << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  // Validate buffer size
  if (spriteSize > bufferSize) {
    std::cout << "[POKEDEX] Sprite too large for buffer: " << spriteSize << " > " << bufferSize << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  // Read sprite data
  if (fread(spriteBuffer, 1, spriteSize, spriteFile) != spriteSize) {
    std::cout << "[POKEDEX] Error reading sprite data for Pokemon " << pokemonId << std::endl;
    fclose(spriteFile);
    return false;
  }
  
  fclose(spriteFile);
  std::cout << "[POKEDEX] Loaded sprite for Pokemon " << pokemonId << " (" << spriteSize << " bytes)" << std::endl;
  return true;
}

void drawSprite(int x, int y, const uint8_t* spriteData, int width, int height) {
  // Draw 1-bit bitmap sprite to E-Ink display
  // Sprite format: 1 bit per pixel, packed into bytes (8 pixels per byte)
  
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col += 8) {
      int byteIndex = (row * width + col) / 8;
      uint8_t pixelByte = spriteData[byteIndex];
      
      for (int bit = 0; bit < 8 && (col + bit) < width; bit++) {
        int pixelX = x + col + bit;
        int pixelY = y + row;
        
        // Check if pixel is within display bounds
        if (pixelX >= 0 && pixelX < display.width() && pixelY >= 0 && pixelY < display.height()) {
          // Extract bit (MSB first) - bit set means black pixel in original image
          bool isBlack = (pixelByte & (1 << (7 - bit))) != 0;
          
          // Invert the logic: bit set (1) means white background, bit clear (0) means black foreground
          if (!isBlack) {
            display.drawPixel(pixelX, pixelY, GxEPD_BLACK);
          }
        }
      }
    }
  }
}

bool loadBinaryPokemonData() {
  std::cout << "[POKEDEX] Attempting to load binary Pokemon data..." << std::endl;
  
  // Try to open the Pokemon records file
  FILE* recordFile = fopen("./data/pokemon/pokemon_data.rec", "rb");
  if (!recordFile) {
    std::cout << "[POKEDEX] Could not open pokemon_data.rec" << std::endl;
    return false;
  }
  
  // Get file size
  fseek(recordFile, 0, SEEK_END);
  size_t fileSize = ftell(recordFile);
  fseek(recordFile, 0, SEEK_SET);
  
  size_t numRecords = fileSize / 32; // 32 bytes per record
  std::cout << "[POKEDEX] Found " << numRecords << " Pokemon records" << std::endl;
  
  // Type mapping
  const char* typeNames[] = {"", "Normal", "Fire", "Water", "Electric", "Grass",
    "Ice", "Fighting", "Poison", "Ground", "Flying", "Psychic", "Bug", 
    "Rock", "Ghost", "Dragon", "Dark", "Steel", "Fairy"};
  
  for (size_t i = 0; i < numRecords; i++) {
    // Read 32-byte record
    uint8_t recordData[32];
    if (fread(recordData, 1, 32, recordFile) != 32) {
      std::cout << "[POKEDEX] Error reading record " << i << std::endl;
      break;
    }
    
    // Parse record data (little-endian)
    uint16_t id = recordData[0] | (recordData[1] << 8);
    uint16_t height = recordData[2] | (recordData[3] << 8);
    uint16_t weight = recordData[4] | (recordData[5] << 8);
    uint8_t hp = recordData[6];
    uint8_t attack = recordData[7];
    uint8_t defense = recordData[8];
    uint8_t sp_attack = recordData[9];
    uint8_t sp_defense = recordData[10];
    uint8_t speed = recordData[11];
    uint8_t type1 = recordData[12];
    uint8_t type2 = recordData[13];
    uint16_t genus_offset = recordData[14] | (recordData[15] << 8);
    uint16_t flavor_offset = recordData[16] | (recordData[17] << 8);
    
    // Create Pokemon struct
    Pokemon pokemon;
    pokemon.id = id;
    pokemon.height_cm = height;
    pokemon.weight_hg = weight;
    pokemon.stats[0] = hp;
    pokemon.stats[1] = attack;
    pokemon.stats[2] = defense;
    pokemon.stats[3] = sp_attack;
    pokemon.stats[4] = sp_defense;
    pokemon.stats[5] = speed;
    
    // Load name from string table
    pokemon.name = loadStringFromTable("pokemon_names.str", i);
    
    // Build type string
    pokemon.types = String(typeNames[type1]);
    if (type2 > 0 && type2 < 19) {
      pokemon.types += "/" + String(typeNames[type2]);
    }
    
    // Load genus and flavor text
    pokemon.genus = loadStringFromTable("pokemon_genus.str", genus_offset);
    pokemon.flavor_text = loadStringFromTable("pokemon_flavor.str", flavor_offset);
    
    // Set image filename
    pokemon.image_file = String(id) + "_front.png";
    if (id < 10) pokemon.image_file = "00" + pokemon.image_file;
    else if (id < 100) pokemon.image_file = "0" + pokemon.image_file;
    
    // Debug output for first few Pokemon
    if (i < 5) {
      std::cout << "[POKEDEX] Loaded Pokemon #" << id << ": " << pokemon.name.c_str() << std::endl;
    }
    
    pokemonList.push_back(pokemon);
  }
  
  fclose(recordFile);
  return pokemonList.size() > 0;
}

String loadStringFromTable(const char* filename, uint16_t index) {
  String filepath = String("./data/pokemon/") + filename;
  
  FILE* file = fopen(filepath.c_str(), "rb");
  if (!file) {
    std::cout << "[POKEDEX] Could not open " << filename << std::endl;
    return "Unknown";
  }
  
  // Read count (little-endian)
  uint8_t countBytes[2];
  if (fread(countBytes, 1, 2, file) != 2) {
    fclose(file);
    return "Unknown";
  }
  uint16_t count = countBytes[0] | (countBytes[1] << 8);
  
  if (index >= count) {
    fclose(file);
    return "Unknown";
  }
  
  // Read offset for this index
  fseek(file, 2 + index * 2, SEEK_SET);
  uint8_t offsetBytes[2];
  if (fread(offsetBytes, 1, 2, file) != 2) {
    fclose(file);
    return "Unknown";
  }
  uint16_t offset = offsetBytes[0] | (offsetBytes[1] << 8);
  
  // Calculate string data start position
  uint16_t stringDataStart = 2 + count * 2;
  
  // Seek to string position
  fseek(file, stringDataStart + offset, SEEK_SET);
  
  // Read null-terminated string
  String result = "";
  char c;
  while (fread(&c, 1, 1, file) == 1 && c != '\0') {
    result += c;
  }
  
  fclose(file);
  return result;
}

void rebuildSearch() {
  searchResults.clear();
  
  if (searchQuery.length() == 0) {
    // Show all Pokemon
    for (const auto& pokemon : pokemonList) {
      searchResults.push_back(pokemon.id);
    }
  } else {
    // Search by name (case insensitive)
    String lowerQuery = searchQuery;
    lowerQuery.toLowerCase();
    
    for (const auto& pokemon : pokemonList) {
      String lowerName = pokemon.name;
      lowerName.toLowerCase();
      
      if (lowerName.indexOf(lowerQuery) >= 0) {
        searchResults.push_back(pokemon.id);
      }
    }
  }
  
  currentIndex = 0;
  listTop = 0;
  newState = true;
}

Pokemon* findPokemonById(uint16_t id) {
  for (auto& pokemon : pokemonList) {
    if (pokemon.id == id) {
      return &pokemon;
    }
  }
  return nullptr;
}

void processKB_POKEDEX() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }

  disableTimeout = false;

  int currentMillis = millis();
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    char inchar = updateKeypress();
    
    switch (CurrentPokedexState) {
      case POKE_LIST:
        if (inchar == 0); // No input
        else if (inchar == 12 || inchar == 27) { // HOME or ESC - Home
          CurrentAppState = HOME;
          newState = true;
          CurrentKBState = NORMAL;
        }
        else if (inchar == 19) { // UP
          if (currentIndex > 0) {
            currentIndex--;
            if (currentIndex < listTop) {
              listTop = currentIndex;
            }
            newState = true;
          }
        }
        else if (inchar == 21) { // DOWN
          if (currentIndex + 1 < searchResults.size()) {
            currentIndex++;
            if (currentIndex >= listTop + 8) {
              listTop = currentIndex - 7;
            }
            newState = true;
          }
        }
        else if (inchar == 13) { // ENTER - View details
          if (searchResults.size() > 0) {
            CurrentPokedexState = POKE_DETAIL;
            newState = true;
            doFull = true;
          }
        }
        else if (inchar == 8) { // BACKSPACE - Search
          CurrentPokedexState = POKE_SEARCH;
          newState = true;
        }
        break;
        
      case POKE_DETAIL:
        if (inchar == 0); // No input
        else if (inchar == 8 || inchar == 12 || inchar == 27) { // BACKSPACE, HOME, or ESC - Back to list
          CurrentPokedexState = POKE_LIST;
          newState = true;
          doFull = true;
        }
        else if (inchar == 20) { // LEFT
          if (currentIndex > 0) {
            currentIndex--;
            newState = true;
            doFull = true;
          }
        }
        else if (inchar == 22) { // RIGHT
          if (currentIndex + 1 < searchResults.size()) {
            currentIndex++;
            newState = true;
            doFull = true;
          }
        }
        break;
        
      case POKE_SEARCH:
        if (inchar == 0); // No input
        else if (inchar == 13 || inchar == 27) { // ENTER or ESC - Back to list
          CurrentPokedexState = POKE_LIST;
          newState = true;
          doFull = true;
        }
        else if (inchar == 8) { // BACKSPACE
          if (searchQuery.length() > 0) {
            searchQuery.remove(searchQuery.length() - 1);
            rebuildSearch();
          } else {
            CurrentPokedexState = POKE_LIST;
            newState = true;
            doFull = true;
          }
        }
        else if (inchar >= 32 && inchar <= 126) { // Printable characters
          searchQuery += (char)inchar;
          rebuildSearch();
        }
        break;
    }
    
    // Update OLED
    currentMillis = millis();
    if (currentMillis - OLEDFPSMillis >= (1000/60)) {
      OLEDFPSMillis = currentMillis;
      updatePokedexOLED();
    }
    
    KBBounceMillis = currentMillis;
  }
}

void einkHandler_POKEDEX() {
  static bool rendering = false;
  
  if (newState && !rendering) {
    rendering = true;
    std::cout << "[POKEDEX] einkHandler_POKEDEX() called - state=" << CurrentPokedexState << std::endl;
    
    switch (CurrentPokedexState) {
      case POKE_LIST:
        drawPokemonList();
        break;
      case POKE_DETAIL:
        if (searchResults.size() > 0 && currentIndex < searchResults.size()) {
          drawPokemonDetail(searchResults[currentIndex]);
        }
        break;
      case POKE_SEARCH:
        drawSearchScreen();
        break;
    }
    
    refresh();
    newState = false;
    doFull = false;
    rendering = false;
  }
}

void drawPokemonList() {
  std::cout << "[POKEDEX] Drawing Pokemon list" << std::endl;
  
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  
  // Title
  display.setCursor(10, 20);
  display.print("Pokedex");
  
  // Draw list items
  int y = 45;
  int itemsShown = 0;
  const int maxItems = 8;
  
  for (size_t i = listTop; i < searchResults.size() && itemsShown < maxItems; i++, itemsShown++) {
    Pokemon* pokemon = findPokemonById(searchResults[i]);
    if (!pokemon) continue;
    
    // Draw Pokemon entry first to get proper positioning
    display.setCursor(10, y);
    
    // Highlight current selection - draw rectangle to cover full text height
    if (i == currentIndex) {
      display.fillRect(5, y - 16, display.width() - 10, 20, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
    } else {
      display.setTextColor(GxEPD_BLACK);
    }
    
    // Reset cursor after drawing rectangle
    display.setCursor(10, y);
    String entry = "#" + String(pokemon->id);
    if (pokemon->id < 10) entry = "#00" + String(pokemon->id);
    else if (pokemon->id < 100) entry = "#0" + String(pokemon->id);
    entry += "  " + pokemon->name;
    
    display.print(entry);
    
    y += 20;
  }
  
  // Instructions
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(10, display.height() - 10);
  display.print("↑↓ Navigate  ⏎ View  ⌫ Search");
}

void drawPokemonDetail(uint16_t pokemonId) {
  std::cout << "[POKEDEX] Drawing Pokemon detail for ID " << pokemonId << std::endl;
  std::cout << "[POKEDEX] Current index: " << currentIndex << ", searchResults size: " << searchResults.size() << std::endl;
  
  Pokemon* pokemon = findPokemonById(pokemonId);
  if (!pokemon) {
    std::cout << "[POKEDEX] ERROR: Could not find Pokemon with ID " << pokemonId << std::endl;
    std::cout << "[POKEDEX] Available Pokemon IDs: ";
    for (size_t i = 0; i < std::min((size_t)10, pokemonList.size()); i++) {
      std::cout << pokemonList[i].id << " ";
    }
    std::cout << std::endl;
    return;
  }
  
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  
  // Load and draw Pokemon sprite (64x64 pixels)
  uint8_t spriteBuffer[512]; // 64*64/8 = 512 bytes for 1-bit bitmap
  bool spriteLoaded = loadPokemonSprite(pokemon->id, spriteBuffer, sizeof(spriteBuffer));
  
  if (spriteLoaded) {
    // Draw sprite in top-right corner
    int spriteX = display.width() - 74; // 64px sprite + 10px margin
    int spriteY = 10;
    drawSprite(spriteX, spriteY, spriteBuffer, 64, 64);
    
    // Draw border around sprite
    display.drawRect(spriteX - 1, spriteY - 1, 66, 66, GxEPD_BLACK);
  }
  
  // Pokemon name and ID (adjust layout for sprite)
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 20);
  String title = "#" + String(pokemon->id) + "  " + pokemon->name;
  display.print(title);
  
  // Types and genus
  display.setFont(&FreeSans9pt7b);
  display.setCursor(10, 40);
  display.print(pokemon->types);
  
  display.setCursor(10, 58);
  display.print(pokemon->genus);
  
  // Physical stats
  display.setCursor(10, 80);
  String physicalStats = "Height: " + String((int)(pokemon->height_cm / 10.0)) + "m";
  physicalStats += "  Weight: " + String((int)(pokemon->weight_hg / 10.0)) + "kg";
  display.print(physicalStats);
  
  // Base stats
  display.setFont(&FreeSans9pt7b);
  const char* statNames[] = {"HP", "ATK", "DEF", "SpA", "SpD", "SPE"};
  int statY = 105;
  
  for (int i = 0; i < 6; i++) {
    display.setCursor(10, statY);
    display.print(statNames[i]);
    display.print(": ");
    display.print(pokemon->stats[i]);
    
    // Draw stat bar
    int barWidth = (pokemon->stats[i] * 80) / 150; // Scale to max 80px
    display.fillRect(60, statY - 8, barWidth, 8, GxEPD_BLACK);
    display.drawRect(60, statY - 8, 80, 8, GxEPD_BLACK);
    
    statY += 15;
  }
  
  // Flavor text (description)
  display.setFont(&FreeSans9pt7b);
  int textY = 210;
  String flavorText = pokemon->flavor_text;
  
  // Simple word wrapping
  int lineWidth = 0;
  int maxWidth = 280;
  String currentLine = "";
  
  for (int i = 0; i < flavorText.length(); i++) {
    char c = flavorText[i];
    currentLine += c;
    lineWidth += 8; // Approximate character width
    
    if (c == ' ' && lineWidth > maxWidth) {
      display.setCursor(10, textY);
      display.print(currentLine);
      textY += 16;
      currentLine = "";
      lineWidth = 0;
      if (textY > 250) break; // Don't overflow
    }
  }
  
  if (currentLine.length() > 0 && textY <= 250) {
    display.setCursor(10, textY);
    display.print(currentLine);
  }
  
  // Instructions
  display.setFont(&FreeSans9pt7b);
  display.setCursor(10, display.height() - 10);
  display.print("← → Navigate  ⌫ Back");
}

void drawSearchScreen() {
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  
  display.setCursor(10, 30);
  display.print("Search Pokemon");
  
  display.setFont(&FreeSans9pt7b);
  display.setCursor(10, 60);
  display.print("Query: " + searchQuery);
  
  display.setCursor(10, 90);
  display.print("Found: " + String(searchResults.size()) + " matches");
  
  display.setCursor(10, display.height() - 10);
  display.print("Type to search  ⏎ View results");
}

void updatePokedexOLED() {
  // Skip OLED updates to avoid segfault - focus on E-Ink display
  return;
  
  u8g2.clearBuffer();
  // u8g2.setFont(u8g2_font_5x7_mf); // Font not available in emulator
  
  switch (CurrentPokedexState) {
    case POKE_LIST: {
      u8g2.setCursor(0, 8);
      u8g2.print("Pokedex");
      u8g2.setCursor(0, 18);
      u8g2.print("Found: ");
      // Safe conversion to avoid potential issues with size_t
      int resultCount = static_cast<int>(searchResults.size());
      u8g2.print(resultCount);
      u8g2.setCursor(0, 28);
      if (resultCount > 0) {
        u8g2.print((currentIndex + 1));
        u8g2.print("/");
        u8g2.print(resultCount);
      }
      break;
    }
      
    case POKE_DETAIL:
      if (searchResults.size() > 0 && currentIndex < searchResults.size()) {
        Pokemon* pokemon = findPokemonById(searchResults[currentIndex]);
        if (pokemon) {
          u8g2.setCursor(0, 8);
          u8g2.print("#");
          u8g2.print(pokemon->id);
          u8g2.setCursor(0, 18);
          u8g2.print(pokemon->name.c_str());
          u8g2.setCursor(0, 28);
          u8g2.print(pokemon->types.c_str());
        }
      }
      break;
      
    case POKE_SEARCH:
      u8g2.setCursor(0, 8);
      u8g2.print("Search Mode");
      u8g2.setCursor(0, 18);
      u8g2.print(searchQuery.c_str());
      u8g2.setCursor(0, 28);
      u8g2.print("Matches: ");
      u8g2.print(searchResults.size());
      break;
  }
  
  u8g2.sendBuffer();
}