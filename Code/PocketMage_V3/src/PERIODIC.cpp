#include "globals.h"
#ifdef DESKTOP_EMULATOR
#include "U8g2lib.h"
#endif
#include "periodic_data.h"
#include "periodic_data_pack.h"

#ifdef DESKTOP_EMULATOR
extern "C" {
  void oled_set_lines(const char* line1, const char* line2, const char* line3);
}
#endif

// Shared helper for partial region updates (emulator vs hardware)
static inline void flushPartialRect(int x, int y, int w, int h) {
#ifdef DESKTOP_EMULATOR
  // Desktop: no region API; presenter already uploads only rows that changed.
  if (g_display) g_display->einkPartialRefresh();
#else
  // Real GxEPD2: push only this window; 'true' = fast (no flashing)
  // NOTE: GxEPD2 requires width/height multiples of 8 on some panels.
  int x8 = x & ~7;
  int w8 = ((x + w + 7) & ~7) - x8;
  display.updateWindow(x8, y, w8, h, true);
#endif
}

// Forward declarations for compatibility
void drawPERIODIC();
void einkHandler_PERIODIC() { drawPERIODIC(); }
extern char inchar;

namespace periodic {

// Canvas rendering state
struct Rect { int16_t x, y, w, h; };
static const int SCREEN_W = 310;
static const int SCREEN_H = 240;

// Off-screen canvas for static table (1bpp)
static uint8_t* gridCanvas = nullptr;
static uint8_t* frontCanvas = nullptr;
static bool canvasInitialized = false;
static bool didFullRefresh = false;

// App state
static int sel_col = -1, sel_row = -1;  // Start with no selection
static int prev_col = -1, prev_row = -1;  // Previous selection for partial updates
static uint8_t selZ = 0;  // No element selected initially
static bool in_detail = false;
static ViewMode viewMode = GRID_VIEW;

// Optional: black-scrub old cell before restoring it (fights ghosting)
static const bool kBlackScrub = true;
// Geometry constants for 310x240 E-ink display - maximize screen usage
static const int grid_x = 5, grid_y = 20, grid_w = 306, grid_h = 216;
static int col_w, row_h;

// Helper: draw 1px border using only fillRect (avoids buggy drawRect on emulator)
static inline void strokeRect1px(int x, int y, int w, int h, uint16_t color) {
  // Top and bottom
  display.fillRect(x,         y,         w, 1, color);
  display.fillRect(x,         y + h - 1, w, 1, color);
  // Left and right
  display.fillRect(x,         y,         1, h, color);
  display.fillRect(x + w - 1, y,         1, h, color);
}

// Layout grid: 18 columns x 9 rows (includes f-block)
static Cell PT_LAYOUT[9][18];

// Search state
static std::vector<Filter> active_filters;
static uint64_t visible_mask[2] = {0xFFFFFFFFFFFFFFFFULL, 0x3FFFFFULL}; // bits 0-117 set

// Canvas utility functions
static void initCanvases() {
  if (!canvasInitialized) {
    size_t canvasSize = (SCREEN_W * SCREEN_H + 7) / 8;  // 1bpp canvas
    gridCanvas = (uint8_t*)malloc(canvasSize);
    frontCanvas = (uint8_t*)malloc(canvasSize);
    if (gridCanvas && frontCanvas) {
      memset(gridCanvas, 0xFF, canvasSize);  // White background
      memset(frontCanvas, 0xFF, canvasSize);
      canvasInitialized = true;
      std::cout << "[PERIODIC] Canvas memory allocated successfully (" << canvasSize << " bytes each)" << std::endl;
    } else {
      std::cerr << "[PERIODIC] ERROR: Failed to allocate canvas memory!" << std::endl;
      if (gridCanvas) { free(gridCanvas); gridCanvas = nullptr; }
      if (frontCanvas) { free(frontCanvas); frontCanvas = nullptr; }
    }
  }
}

static void cleanupCanvases() {
  if (gridCanvas) { free(gridCanvas); gridCanvas = nullptr; }
  if (frontCanvas) { free(frontCanvas); frontCanvas = nullptr; }
  canvasInitialized = false;
}

static Rect alignToByte(Rect r) {
  // Align X/W to multiples of 8 pixels for 1bpp byte packing
  int x0 = r.x & ~7;
  int x1 = (r.x + r.w + 7) & ~7;
  return Rect{ int16_t(x0), r.y, int16_t(x1 - x0), r.h };
}

static Rect mergeRects(Rect a, Rect b) {
  int x0 = std::min(a.x, b.x), y0 = std::min(a.y, b.y);
  int x1 = std::max(a.x + a.w, b.x + b.w), y1 = std::max(a.y + a.h, b.y + b.h);
  return Rect{ int16_t(x0), int16_t(y0), int16_t(x1 - x0), int16_t(y1 - y0) };
}

static void blitCanvas(uint8_t* src, uint8_t* dst, Rect r) {
  // Fast copy of rectangular region between 1bpp canvases
  int bytesPerRow = (SCREEN_W + 7) / 8;
  for (int y = 0; y < r.h; y++) {
    int srcOffset = ((r.y + y) * bytesPerRow) + (r.x / 8);
    int dstOffset = ((r.y + y) * bytesPerRow) + (r.x / 8);
    int copyBytes = (r.w + 7) / 8;
    memcpy(dst + dstOffset, src + srcOffset, copyBytes);
  }
}

static Rect cellRect(int col, int row) {
  // Return rectangle for cell at given grid position
  int x = grid_x + col * col_w;
  int y = grid_y + row * row_h;
  return Rect{ int16_t(x), int16_t(y), int16_t(col_w), int16_t(row_h) };
}

// Forward declarations
static void onCursorMove(int newCol, int newRow);

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
    
    // Handle f-block elements (Lanthanoids and Actinoids) and super-heavies
    if (z >= 57 && z <= 71) {  // Lanthanoids
      col = z - 57 + 3;  // Start at column 3
      row = 6;  // Row 6 (below main table)
    } else if (z >= 89 && z <= 103) {  // Actinoids  
      col = z - 89 + 3;  // Start at column 3
      row = 7;  // Row 7 (below lanthanoids)
    } else if (z >= 104 && z <= 118) {
      // Handle super-heavy elements 104-118 in their own row below actinoids
      col = z - 104 + 3;  // Start at column 3, elements 104-118 -> columns 3-17  
      row = 8;  // Row 8 (below actinoids)
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
  
  // Use partial update system for selection
  onCursorMove(col, row);
  selZ = z;
}

static void move_selection(int dc, int dr) {
  if (sel_col == -1 || sel_row == -1) return;
  
  int new_col = sel_col + dc;
  int new_row = sel_row + dr;
  
  // Try to find next valid cell in direction
  for (int step = 0; step < 18; ++step) {
    // Bounds checking
    if (new_col < 0 || new_col >= 18 || new_row < 0 || new_row >= 9) break;
    
    // Check if target cell has an element
    if (PT_LAYOUT[new_row][new_col].z != 0) {
      // Use partial update system for cursor movement
      onCursorMove(new_col, new_row);
      selZ = PT_LAYOUT[new_row][new_col].z;
      break;
    }
    
    // Continue searching in the same direction
    new_col += dc;
    new_row += dr;
  }
}

static void drawCellNormal(int col, int row);
static void drawCellSelected(int col, int row);

static void onCursorMove(int newCol, int newRow) {
  // Always allow cursor movement - no canvas dependency

  // Remember previous
  prev_col = sel_col;
  prev_row = sel_row;
  sel_col  = newCol;
  sel_row  = newRow;

  // 1) Un-highlight previous by fully redrawing it *normal* (white interior)
  if (prev_col >= 0 && prev_row >= 0) {
    Cell& prevCell = PT_LAYOUT[prev_row][prev_col];
    if (prevCell.z > 0) drawCellNormal(prev_col, prev_row);
  }

  // 2) Draw new selection with double border (no black fill)
  Cell& curCell = PT_LAYOUT[sel_row][sel_col];
  if (curCell.z > 0) drawCellSelected(sel_col, sel_row);

  // Push the small change to the panel
  if (g_display) g_display->einkRefresh();
}

static void paint_table() {
  // Always render fresh - no canvas caching to avoid artifacts
  
  // Baseline: white screen, header
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  
  // Center the title on screen (320px wide)
  const char* title = "Periodic Table";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int centered_x = (320 - w) / 2;
  display.setCursor(centered_x, 15);
  display.print(title);

  // Draw all cells once (only selected one has double border)
  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 18; col++) {
      if (PT_LAYOUT[row][col].z == 0) continue;
      const bool isSelected = (col == sel_col && row == sel_row);
      if (isSelected) {
        drawCellSelected(col, row);
      } else {
        drawCellNormal(col, row);
      }
    }
  }
  
  // Force refresh to show the clean table
  refresh();
}


static void drawHighlight(Rect r) {
  // Invert the cell area for highlighting
  if (!frontCanvas) {
    std::cerr << "[PERIODIC] ERROR: frontCanvas is null in drawHighlight!" << std::endl;
    return;
  }
  int bytesPerRow = (SCREEN_W + 7) / 8;
  for (int y = 0; y < r.h; y++) {
    for (int x = 0; x < r.w; x++) {
      int pixelX = r.x + x;
      int pixelY = r.y + y;
      if (pixelX < SCREEN_W && pixelY < SCREEN_H) {
        int byteIndex = (pixelY * bytesPerRow) + (pixelX / 8);
        int bitIndex = 7 - (pixelX % 8);
        frontCanvas[byteIndex] ^= (1 << bitIndex);  // Invert bit
      }
    }
  }
}

static void panelPartialUpdate(Rect r) {
  // Force a proper display update by calling the display system
  // This ensures our row-dirty system detects the changes
  if (g_display) {
    g_display->einkRefresh();
  }
}

// Draw a 1px black border around a rect, without relying on drawRect on desktop.
static inline void drawBorderRect(int x, int y, int w, int h) {
#ifdef DESKTOP_EMULATOR
  // Use the emulator's low-level line primitive to guarantee an outline only.
  if (g_display) {
    g_display->einkDrawLine(x,         y,         x + w - 1, y,         true); // top
    g_display->einkDrawLine(x,         y + h - 1, x + w - 1, y + h - 1, true); // bottom
    g_display->einkDrawLine(x,         y,         x,         y + h - 1, true); // left
    g_display->einkDrawLine(x + w - 1, y,         x + w - 1, y + h - 1, true); // right
  } else {
    // Fallback, should not happen on emulator
    display.drawRect(x, y, w, h, GxEPD_BLACK);
  }
#else
  // On hardware, the GFX drawRect is reliable for a 1px outline.
  display.drawRect(x, y, w, h, GxEPD_BLACK);
#endif
}


// Helper: Draw a normal cell (white interior + single border)
static inline void drawCellNormal(int col, int row) {
  Cell& cell = PT_LAYOUT[row][col];
  if (cell.z == 0) return;
  const int x = grid_x + col * col_w;
  const int y = grid_y + row * row_h;

  // 1) Clear ENTIRE cell area to white first (critical!)
  display.fillRect(x, y, col_w, row_h, GxEPD_WHITE);

  // 2) Draw border + element symbol only (no atomic numbers)
  display.drawRect(x, y, col_w, row_h, GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&Font5x7Fixed);
  display.setCursor(x + 2, y + (row_h / 2) + 2);
  display.print(get_symbol(cell.z));
}

// Helper: Draw a selected cell (white interior + double border)
static inline void drawCellSelected(int col, int row) {
  Cell& cell = PT_LAYOUT[row][col];
  if (cell.z == 0) return;
  const int x = grid_x + col * col_w;
  const int y = grid_y + row * row_h;

  // 1) Clear ENTIRE cell area to white first (critical!)
  display.fillRect(x, y, col_w, row_h, GxEPD_WHITE);
  
  // 2) Draw double border (outer + inner)
  display.drawRect(x,   y,   col_w,   row_h,   GxEPD_BLACK);
  display.drawRect(x+1, y+1, col_w-2, row_h-2, GxEPD_BLACK);
  
  // 3) Draw element symbol only (no atomic numbers)
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&Font5x7Fixed);
  display.setCursor(x + 3, y + (row_h / 2) + 2);
  display.print(get_symbol(cell.z));
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
  
  // Initialize canvases for rendering
  periodic::initCanvases();
  
  // Clear screen to remove any artifacts from previous apps
  display.fillScreen(GxEPD_WHITE);
  refresh();
  
  CurrentAppState = PERIODIC;
  CurrentKBState = NORMAL;
  newState = true;
  doFull = true;
  
  // Reset canvas state to force fresh render
  periodic::didFullRefresh = false;
  
  // Compute cell dimensions
  periodic::col_w = periodic::grid_w / 18;
  periodic::row_h = periodic::grid_h / 9;
  
  // Build the layout grid
  periodic::build_layout();
  
  // Start with Hydrogen (H) selected at position (0,0)
  periodic::sel_col = 0;
  periodic::sel_row = 0;
  periodic::selZ = 1;  // Hydrogen
  
  std::cout << "[POCKETMAGE] PERIODIC_INIT() complete" << std::endl;
}

void processKB_PERIODIC() {
  static unsigned long lastUpdate = 0;
  int currentMillis = millis();
  if (currentMillis - KBBounceMillis < KB_COOLDOWN) return;
  
  // Prevent rapid updates that cause Metal issues
  if (currentMillis - lastUpdate < 200) return;
  lastUpdate = currentMillis;
  
  KeyEvent keyEvent = updateKeypressUTF8();
  if (keyEvent.action == KA_NONE) return;  // No key pressed
  
  char inchar = 0;
  
  // Convert KeyEvent actions to navigation keys
  switch (keyEvent.action) {
    case KA_UP:        inchar = 19; break;  // UP
    case KA_DOWN:      inchar = 21; break;  // DOWN  
    case KA_LEFT:      inchar = 20; break;  // LEFT
    case KA_RIGHT:     inchar = 18; break;  // RIGHT
    case KA_ENTER:     inchar = 13; break;  // ENTER
    case KA_ESC:       inchar = 27; break;  // ESC
    case KA_HOME:      inchar = 12; break;  // HOME
    case KA_DELETE:    inchar = 8;  break;  // DELETE
    case KA_BACKSPACE: inchar = 8;  break;  // BACKSPACE
    case KA_TAB:       inchar = 9;  break;  // TAB
    case KA_CHAR:
      if (keyEvent.text.length() == 1) {
        inchar = keyEvent.text[0];
      }
      break;
    default:
      return;  // Ignore other key events
  }
  
  if (inchar == 0) return;  // No valid key conversion
  
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
      // Clear displays before exiting
      display.fillScreen(GxEPD_WHITE);
      refresh();
      delay(10);  // Ensure Metal command buffer completion
      
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
    // Clear displays before exiting
    display.fillScreen(GxEPD_WHITE);
    refresh();
    delay(10);  // Ensure Metal command buffer completion
    
    CurrentAppState = HOME;
    newState = true;
    doFull = true;
#ifdef DESKTOP_EMULATOR
    oled_set_lines("", "", "");  // Clear OLED display
#else
    u8g2.clearBuffer();
    u8g2.sendBuffer();
#endif
    // CRITICAL: Return immediately to prevent further rendering after app exit
    return;
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
  // Skip canvas initialization - use direct rendering only
  
  if (newState) {
    newState = false;
    
    if (periodic::in_detail) {
      periodic::paint_detail();
      if (doFull) {
        refresh();
        doFull = false;
      }
    } else {
      periodic::paint_table();
      // paint_table() handles its own partial updates, no need for full refresh
    }
  }
  
  // Always update OLED
  periodic::update_oled();
}

// Cleanup function to be called when exiting the app
void cleanupPERIODIC() {
  periodic::cleanupCanvases();
}
