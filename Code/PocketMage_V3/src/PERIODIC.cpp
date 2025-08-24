#include "globals.h"
#include "periodic_data.h"
#include "periodic_data_pack.h"

#ifdef DESKTOP_EMULATOR
extern "C" {
  void oled_set_lines(const char* line1, const char* line2, const char* line3);
}
#endif

// Forward declarations for compatibility
void drawPERIODIC();
void einkHandler_PERIODIC() { drawPERIODIC(); }
extern char inchar;

namespace periodic {

// App state
static int sel_col = -1, sel_row = -1;  // Start with no selection
static uint8_t selZ = 0;  // No element selected initially
static bool in_detail = false;
static bool in_search = false;
static ViewMode viewMode = GRID_VIEW;

// Geometry constants for 310x240 E-ink display
static const int grid_x = 6, grid_y = 20, grid_w = 298, grid_h = 180;
static int col_w, row_h;

// Layout grid: 18 columns x 9 rows (includes f-block)
static Cell PT_LAYOUT[9][18];

// Search state
static std::vector<Filter> active_filters;
static uint64_t visible_mask[2] = {0xFFFFFFFFFFFFFFFFULL, 0x3FFFFFULL}; // bits 0-117 set

// Helper functions for data access
static const PackedElement& E(uint8_t z) {
  if (z == 0 || z > 118) {
    static const PackedElement empty = {0};
    return empty;
  }
  // PT_ELEMENTS is sized [119] with index 0 unused, elements 1-118 at indices 1-118
  return PT_ELEMENTS[z];
}

static const char* get_symbol(uint8_t z) {
  if (z == 0 || z > 118) return "";
  const PackedElement& e = E(z);
  if (e.sym_off >= PT_SYM_SIZE) return "";
  return (const char*)&PT_SYM_BYTES[e.sym_off];
}

static const char* get_name(uint8_t z) {
  if (z == 0 || z > 118) return "";
  const PackedElement& e = E(z);
  if (e.name_off >= PT_NAME_SIZE) return "";
  return (const char*)&PT_NAME_BYTES[e.name_off];
}

static const char* get_discoverer(uint8_t z) {
  if (z == 0 || z > 118) return "";
  const PackedElement& e = E(z);
  if (e.discoverer_off == 0 || e.discoverer_off >= PT_DISC_SIZE) return "";
  return (const char*)&PT_DISC_BYTES[e.discoverer_off];
}

static void build_layout() {
  // Initialize all cells as empty
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 18; c++) {
      PT_LAYOUT[r][c] = {(uint8_t)c, (uint8_t)r, 0};
    }
  }
  
  // Fill main periodic table positions
  for (int z = 1; z <= 118; z++) {
    const PackedElement& elem = E(z);
    int col, row;
    
    // Handle f-block elements (Lanthanoids and Actinoids)
    if (z >= 57 && z <= 71) {  // Lanthanoids
      col = z - 57 + 3;  // Start at column 3
      row = 7;  // Row 7 (0-indexed)
    } else if (z >= 89 && z <= 103) {  // Actinoids
      col = z - 89 + 3;  // Start at column 3
      row = 8;  // Row 8 (0-indexed)
    } else {
      // Standard periodic table positioning
      col = elem.group - 1;  // Groups 1-18 -> columns 0-17
      row = elem.period - 1; // Periods 1-7 -> rows 0-6
      
      // Adjust for f-block gap in periods 6 and 7
      if (elem.period == 6 && elem.group == 3) col = 2;  // La position
      if (elem.period == 7 && elem.group == 3) col = 2;  // Ac position
    }
    
    if (col >= 0 && col < 18 && row >= 0 && row < 9) {
      PT_LAYOUT[row][col] = {(uint8_t)col, (uint8_t)row, (uint8_t)z};
    }
  }
}

static void select_by_cell(int col, int row) {
  if (col < 0 || col >= 18 || row < 0 || row >= 9) return;
  uint8_t z = PT_LAYOUT[row][col].z;
  if (z == 0) return;
  
  sel_col = col; 
  sel_row = row; 
  selZ = z;
  newState = true;  // Trigger redraw
}

static void move_selection(int dc, int dr) {
  int c = sel_col, r = sel_row;
  
  // Try to find next valid cell in direction
  for (int step = 0; step < 18; ++step) {
    c += dc; 
    r += dr;
    if (c < 0 || c >= 18 || r < 0 || r >= 9) break;
    if (PT_LAYOUT[r][c].z) { 
      select_by_cell(c, r); 
      break; 
    }
  }
}

static void paint_cell(int x, int y, int w, int h, uint8_t z, bool selected) {
  if (z == 0 || z > 118) return;  // Empty cell or invalid
  
  const PackedElement& elem = E(z);
  
  // Background pattern based on category
  if (selected) {
    display.fillRect(x, y, w, h, GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
  } else {
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.setTextColor(GxEPD_BLACK);
  }
  
  // Atomic number (centered, larger font for better visibility)
  display.setFont(&FreeSans12pt7b);
  char num_str[4];
  snprintf(num_str, sizeof(num_str), "%d", z);
  
  int16_t x1, y1;
  uint16_t nw, nh;
  display.getTextBounds(num_str, 0, 0, &x1, &y1, &nw, &nh);
  display.setCursor(x + (w - nw) / 2, y + h / 2 + nh / 2);
  display.print(num_str);
  
  // Remove atomic mass from cells to reduce clutter
  // Mass will be shown in detail view and OLED display
}

static void paint_table() {
  display.fillScreen(GxEPD_WHITE);
  
  // Title
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.setCursor(10, 15);
  display.print("Periodic Table");
  
  // Draw grid
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 18; c++) {
      int x = grid_x + c * col_w;
      int y = grid_y + r * row_h;
      uint8_t z = PT_LAYOUT[r][c].z;
      bool selected = (c == sel_col && r == sel_row);
      paint_cell(x, y, col_w, row_h, z, selected);
    }
  }
  
  // Status line at bottom
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(10, 235);
  display.print("[Enter] Details [/] Search [Tab] View");
}

static void paint_detail() {
  display.fillScreen(GxEPD_WHITE);
  
  const PackedElement& elem = E(selZ);
  const char* symbol = get_symbol(selZ);
  const char* name = get_name(selZ);
  
  int y = 20;
  
  // Header: Symbol and Name
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.setCursor(10, y);
  display.print(symbol);
  
  display.setFont(&FreeSans12pt7b);
  display.setCursor(60, y);
  display.print(name);
  
  y += 30;
  
  // Basic properties
  display.setFont(&FreeSans9pt7b);
  
  display.setCursor(10, y);
  display.print("Atomic Number: ");
  display.print(selZ);
  y += 15;
  
  display.setCursor(10, y);
  display.print("Atomic Mass: ");
  char buf[16];
  snprintf(buf, sizeof(buf), "%.3f", elem.mass_milli / 1000.0f);
  display.print(buf);
  display.print(" u");
  y += 15;
  
  display.setCursor(10, y);
  display.print("Group: ");
  display.print(elem.group);
  display.print(", Period: ");
  display.print(elem.period);
  y += 15;
  
  display.setCursor(10, y);
  display.print("Block: ");
  display.print((char)('s' + (int)elem.block));
  display.print("-block");
  y += 15;
  
  if (elem.density_x1000 != 0) {
    display.setCursor(10, y);
    display.print("Density: ");
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", elem.density_x1000 / 1000.0f);
    display.print(buf);
    display.print(" g/cm³");
    y += 15;
  }
  
  if (elem.mp_kx100 != -1) {
    display.setCursor(10, y);
    display.print("Melting Point: ");
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", elem.mp_kx100 / 100.0f);
    display.print(buf);
    display.print(" K");
    y += 15;
  }
  
  if (elem.bp_kx100 != -1) {
    display.setCursor(10, y);
    display.print("Boiling Point: ");
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", elem.bp_kx100 / 100.0f);
    display.print(buf);
    display.print(" K");
    y += 15;
  }
  
  if (elem.en_paulingx100 != 0) {
    display.setCursor(10, y);
    display.print("Electronegativity: ");
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", elem.en_paulingx100 / 100.0f);
    display.print(buf);
    y += 15;
  }
  
  // Flags
  if (elem.flags & F_RADIOACTIVE) {
    display.setCursor(10, y);
    display.print("Radioactive");
    y += 15;
  }
  
  if (elem.flags & F_TOXIC) {
    display.setCursor(10, y);
    display.print("Toxic");
    y += 15;
  }
  
  // Back instruction
  display.setCursor(10, 230);
  display.print("[Enter] Back to table [Esc] Home");
}

static void update_oled() {
  if (selZ == 0) {
    // No element selected - show navigation help
#ifdef DESKTOP_EMULATOR
    oled_set_lines("Periodic Table", "Arrows: Navigate", "Enter: Details");
#else
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 8, "Periodic Table");
    u8g2.drawStr(0, 16, "Arrows: Navigate");
    u8g2.drawStr(0, 24, "Enter: Details");
    u8g2.sendBuffer();
#endif
    return;
  }
  
  const PackedElement& elem = E(selZ);
  const char* symbol = get_symbol(selZ);
  const char* name = get_name(selZ);
  
  // Build strings for thread-safe OLED service
  char line1[32];
  snprintf(line1, sizeof(line1), "%s %d - %s", symbol, selZ, name);
  
  char line2[32];
  snprintf(line2, sizeof(line2), "Grp %d, Per %d, %.1f u", elem.group, elem.period, elem.mass_milli / 1000.0f);
  
  char line3[32];
  if (elem.density_x1000 != 0) {
    snprintf(line3, sizeof(line3), "%c-block, %.2f g/cm³", 's' + (int)elem.block, elem.density_x1000 / 1000.0f);
  } else {
    snprintf(line3, sizeof(line3), "%c-block", 's' + (int)elem.block);
  }
  
#ifdef DESKTOP_EMULATOR
  // Use thread-safe OLED service in emulator
  oled_set_lines(line1, line2, line3);
#else
  // Direct u8g2 calls on hardware
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(0, 8, line1);
  u8g2.drawStr(0, 16, line2);
  u8g2.drawStr(0, 24, line3);
  u8g2.sendBuffer();
#endif
}

} // namespace periodic

void PERIODIC_INIT() {
  std::cout << "[POCKETMAGE] PERIODIC_INIT() starting..." << std::endl;
  CurrentAppState = PERIODIC;
  CurrentKBState = NORMAL;
  newState = true;
  doFull = true;
  
  // Compute cell dimensions
  periodic::col_w = periodic::grid_w / 18;
  periodic::row_h = periodic::grid_h / 9;
  
  // Build the layout grid
  periodic::build_layout();
  
  // Start with no element selected - show full table
  
  std::cout << "[POCKETMAGE] PERIODIC_INIT() complete" << std::endl;
}

void processKB_PERIODIC() {
  int currentMillis = millis();
  if (currentMillis - KBBounceMillis < KB_COOLDOWN) return;
  
  char inchar = updateKeypress();
  if (inchar == 0) return;  // No key pressed
  
  std::cout << "[PERIODIC] Key pressed: " << (int)inchar << std::endl;
  
  if (periodic::in_detail) {
    if (inchar == 13) {  // ENTER - back to table
      periodic::in_detail = false;
      newState = true;
      doFull = true;
      // Force immediate screen clear when returning to table view
      display.fillScreen(GxEPD_WHITE);
      refresh();
      KBBounceMillis = currentMillis;  // Set bounce time
      return;
    }
    else if (inchar == 12 || inchar == 27) {  // HOME or ESC - exit app
      CurrentAppState = HOME;
      newState = true;
      doFull = true;
      KBBounceMillis = currentMillis;
#ifdef DESKTOP_EMULATOR
      oled_set_lines("", "", "");  // Clear OLED display
#else
      u8g2.clearBuffer();
      u8g2.sendBuffer();
#endif
      // Clear the E-ink display
      display.fillScreen(GxEPD_WHITE);
      refresh();
      return;
    }
    KBBounceMillis = currentMillis;
    return;
  }
  
  // Main table navigation
  if (inchar == 20) {  // LEFT
    if (periodic::selZ == 0) {
      // First navigation - start at Hydrogen
      periodic::select_by_cell(0, 0);
    } else {
      periodic::move_selection(-1, 0);
    }
  }
  else if (inchar == 18) {  // RIGHT
    if (periodic::selZ == 0) {
      // First navigation - start at Hydrogen
      periodic::select_by_cell(0, 0);
    } else {
      periodic::move_selection(1, 0);
    }
  }
  else if (inchar == 19) {  // UP
    if (periodic::selZ == 0) {
      // First navigation - start at Hydrogen
      periodic::select_by_cell(0, 0);
    } else {
      periodic::move_selection(0, -1);
    }
  }
  else if (inchar == 21) {  // DOWN
    if (periodic::selZ == 0) {
      // First navigation - start at Hydrogen
      periodic::select_by_cell(0, 0);
    } else {
      periodic::move_selection(0, 1);
    }
  }
  else if (inchar == 13) {  // ENTER - show details
    if (periodic::selZ == 0) {
      // First navigation - start at Hydrogen
      periodic::select_by_cell(0, 0);
    } else {
      periodic::in_detail = true;
      newState = true;
      doFull = true;
      // Force immediate screen clear when entering detail view
      display.fillScreen(GxEPD_WHITE);
      refresh();
    }
  }
  else if (inchar == 27 || inchar == 12) {  // ESC or HOME - exit app
    CurrentAppState = HOME;
    newState = true;
    doFull = true;
#ifdef DESKTOP_EMULATOR
    oled_set_lines("", "", "");  // Clear OLED display
#else
    u8g2.clearBuffer();
    u8g2.sendBuffer();
#endif
    // Clear the E-ink display
    display.fillScreen(GxEPD_WHITE);
    refresh();
  }
  else if (inchar == 9) {  // TAB - cycle views (future)
    // TODO: Implement view cycling
  }
  else if (inchar == '/') {  // Search (future)
    // TODO: Implement search overlay
  }
  
  // Set bounce time for all key presses
  KBBounceMillis = currentMillis;
}

void drawPERIODIC() {
  if (newState) {
    newState = false;
    
    if (periodic::in_detail) {
      periodic::paint_detail();
    } else {
      periodic::paint_table();
    }
    
    if (doFull) {
      refresh();
      doFull = false;
    }
  }
  
  // Always update OLED
  periodic::update_oled();
}
