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

static void renderStaticTableOnce() {
  if (!canvasInitialized) {
    initCanvases();
  }
  
  std::cout << "[PERIODIC] Rendering static table to canvas..." << std::endl;
  
  // Clear grid canvas to white
  size_t canvasSize = (SCREEN_W * SCREEN_H + 7) / 8;
  memset(gridCanvas, 0xFF, canvasSize);
  
  // Calculate cell dimensions
  col_w = grid_w / 18;  // 18 columns
  row_h = grid_h / 9;   // 9 rows
  
  // Render to off-screen canvas using display as temporary target
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  
  // Draw title
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 15);
  display.print("Periodic Table");
  
  // Draw all element cells (without selection highlighting)
  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 18; col++) {
      Cell& cell = PT_LAYOUT[row][col];
      if (cell.z == 0) continue;  // Skip empty cells
      
      int x = grid_x + col * col_w;
      int y = grid_y + row * row_h;
      
      // Draw cell border (no fill)
      display.drawRect(x, y, col_w, row_h, GxEPD_BLACK);
      display.setTextColor(GxEPD_BLACK);
      
      // Draw element symbol
      display.setFont(&FreeMono12pt7b);
      display.setCursor(x + 2, y + 12);
      display.print(get_symbol(cell.z));
      
      // Draw atomic number
      display.setFont(&FreeMonoBold9pt7b);
      display.setCursor(x + 1, y + row_h - 2);
      display.print(cell.z);
    }
  }
  
  // Copy display buffer to grid canvas (this is a simplified approach)
  // In a real implementation, you'd render directly to the canvas
  memcpy(frontCanvas, gridCanvas, canvasSize);
  
  // Push full-screen baseline to panel
  refresh();
  didFullRefresh = true;
}

static void drawHighlight(Rect r) {
  // Invert the cell area for highlighting
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
  // Simplified partial update - just do a full refresh for now
  // This eliminates the Metal GPU crashes while maintaining functionality
  refresh();
}

static void onCursorMove(int newCol, int newRow) {
  if (!didFullRefresh) {
    renderStaticTableOnce();
    return;
  }
  
  prev_col = sel_col;
  prev_row = sel_row;
  sel_col = newCol;
  sel_row = newRow;
  
  if (prev_col >= 0 && prev_row >= 0) {
    Rect rcPrev = alignToByte(cellRect(prev_col, prev_row));
    Rect rcCur = alignToByte(cellRect(sel_col, sel_row));
    
    // Restore old cell from static background
    blitCanvas(gridCanvas, frontCanvas, rcPrev);
    
    // Ensure new cell starts from base
    blitCanvas(gridCanvas, frontCanvas, rcCur);
    
    // Draw highlight on new cell
    drawHighlight(rcCur);
    
    // Update only the changed regions
    panelPartialUpdate(rcPrev);  // Clear old highlight
    panelPartialUpdate(rcCur);   // Draw new highlight
  } else {
    // First selection - just highlight the new cell
    Rect rcCur = alignToByte(cellRect(sel_col, sel_row));
    blitCanvas(gridCanvas, frontCanvas, rcCur);
    drawHighlight(rcCur);
    panelPartialUpdate(rcCur);
  }
}

static void paint_table() {
  if (!didFullRefresh) {
    renderStaticTableOnce();
  }
  
  // Draw the table with current selection highlighted
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  
  // Draw title
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 15);
  display.print("Periodic Table");
  
  // Draw all element cells
  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 18; col++) {
      Cell& cell = PT_LAYOUT[row][col];
      if (cell.z == 0) continue;  // Skip empty cells
      
      int x = grid_x + col * col_w;
      int y = grid_y + row * row_h;
      
      bool isSelected = (col == sel_col && row == sel_row);
      
      // Draw cell background - inverted if selected
      if (isSelected) {
        display.fillRect(x, y, col_w, row_h, GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
      } else {
        display.drawRect(x, y, col_w, row_h, GxEPD_BLACK);
        display.setTextColor(GxEPD_BLACK);
      }
      
      // Draw element symbol
      display.setFont(&FreeMono12pt7b);
      display.setCursor(x + 2, y + 12);
      display.print(get_symbol(cell.z));
      
      // Draw atomic number
      display.setFont(&FreeMonoBold9pt7b);
      display.setCursor(x + 1, y + row_h - 2);
      display.print(cell.z);
    }
  }
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
  // Initialize canvases on first run
  if (!periodic::canvasInitialized) {
    periodic::initCanvases();
  }
  
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
