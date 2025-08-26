#include "globals.h"
#include "sdmmc_cmd.h"
#ifdef DESKTOP_EMULATOR
#include "U8g2lib.h"
#endif

//   .oooooo..o oooooooooooo ooooooooooooo ooooo     ooo ooooooooo.    //
//  d8P'    `Y8 `888'     `8 8'   888   `8 `888'     `8' `888   `Y88.  //
//  Y88bo.       888              888       888       8   888   .d88'  //
//   `"Y8888o.   888oooo8         888       888       8   888ooo88P'   //
//       `"Y88b  888    "         888       888       8   888          //
//  oo     .d8P  888       o      888       `88.    .8'   888          //
//  8""88888P'  o888ooooood8     o888o        `YbodP'    o888o         //

// Display setup
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, OLED_CS, OLED_DC, OLED_RST); //256x32

// Keypad setup
Adafruit_TCA8418 keypad;

// Buzzer
Buzzer buzzer(17);

char keysArray[4][10] = {
  { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' },
  { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   8 },  //8:BKSP
  {   9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.',  13 },  //9:TAB, 13:CR
  {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }   //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:-> 
};

char keysArraySHFT[4][10] = {
  { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
  { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   8 },
  {   9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',',  13 },
  {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }
};

char keysArrayFN[4][10] = {
  { '1', '2', '3', '4', '5', '6', '7',  '8',  '9', '0' },
  { '#', '!', '$', ':', ';', '(', ')', '\'', '\"',   8 },
  {  14, '%', '_', '&', '+', '-', '/',  '?',  ',',  13 },
  {   0,  17,  18, ' ', ' ', ' ',  12,    7,    6,   0 }
};

// Touch slider setup
Adafruit_MPR121 cap = Adafruit_MPR121();

// RTC setup
RTC_PCF8563 rtc;
const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// USB
USBMSC msc;
bool mscEnabled = false;
sdmmc_card_t* card = nullptr;

// VARIABLES
// GENERAL
Preferences prefs;
int TIMEOUT;              // Time until automatic sleep (Seconds)
bool DEBUG_VERBOSE;       // Spit out some extra information
bool SYSTEM_CLOCK;        // Enable a small clock on the bottom of the screen.
bool SHOW_YEAR;           // Show the year on the clock
bool SAVE_POWER;          // Enable a slower CPU clock speed to save battery with little cost to performance
bool ALLOW_NO_MICROSD;    // Allow the device to operate with no SD card
bool HOME_ON_BOOT;        // Always start the home app on boot
int OLED_BRIGHTNESS;      // Brightness of the OLED (0-255)
int OLED_MAX_FPS;         // Define the max oled FPS

volatile int einkRefresh = FULL_REFRESH_AFTER;
int OLEDFPSMillis = 0;
int KBBounceMillis = 0;
volatile int timeoutMillis = 0;
volatile int prevTimeMillis = 0;
volatile bool TCA8418_event = false;
volatile bool PWR_BTN_event = false;
volatile bool SHFT = false;
volatile bool FN = false;
volatile bool newState = false;
bool noTimeout = false;
volatile bool OLEDPowerSave = false;
volatile bool disableTimeout = false;
volatile int battState = 0;
volatile int prevBattState = 0;
unsigned int flashMillis = 0;
int prevTime = 0;
uint8_t prevSec = 0;
TaskHandle_t einkHandlerTaskHandle = NULL;
char currentKB[4][10];
KBState CurrentKBState = NORMAL;
uint8_t partialCounter = 0;
volatile bool forceSlowFullUpdate = false;
volatile bool SDCARD_INSERT = false;
bool noSD = false;
volatile bool SDActive = false;

const String appStateNames[] = { "txt", "filewiz", "usb", "bt", "settings", "tasks", "calendar", "journal", "lexicon", "pokedex", "periodic" };
const unsigned char *appIcons[11] = { _homeIcons2, _homeIcons3, _homeIcons4, _homeIcons5, _homeIcons6, taskIconTasks0, _homeIcons7, _homeIcons8, _homeIcons9, pokedexIcon, atomIcon};

AppState CurrentAppState;

// <TXT.cpp>
String currentWord = "";
String allText = "";
String prevAllText = "";
String prevLastLine = "";
bool prevBKSP = false;
int scroll = 0;
int lines = 0;
String outLines[13];
String lines_prev[13];
String filesList[MAX_FILES];
uint8_t fileIndex = 0;
String editingFile;
String prevEditingFile = "";
String excludedFiles[3] = { "/temp.txt", "/settings.txt", "/tasks.txt" };
TXTState CurrentTXTState = TXT_;

String currentLine = "";
const GFXfont *currentFont = (GFXfont *)&FreeSerif9pt7b;
uint8_t maxCharsPerLine = 0;
uint8_t maxLines = 0;
uint8_t fontHeight = 0;
uint8_t lineSpacing = 6;  // LINE SPACING IN PIXELS
volatile bool newLineAdded = true;
volatile bool doFull = false;
std::vector<String> allLines;
volatile long int dynamicScroll = 0;
volatile long int prev_dynamicScroll = 0;
int lastTouch = -1;
unsigned long lastTouchTime = 0;

// <TASKS.cpp>
std::vector<std::vector<String>> tasks;
uint8_t selectedTask = 0;
TasksState CurrentTasksState = TASKS0;
uint8_t newTaskState = 0;
uint8_t editTaskState = 0;
String newTaskName = "";
String newTaskDueDate = "";

// <HOME.cpp>
HOMEState CurrentHOMEState = HOME_HOME;

// <FILEWIZ.cpp> 
FileWizState CurrentFileWizState = WIZ0_;
String workingFile = "";

// <SETTINGS.cpp>
SettingsState CurrentSettingsState = settings0;

// <CALENDAR.cpp>
CalendarState CurrentCalendarState = MONTH;

// <LEXICON.cpp>
LexState CurrentLexState = MENU;

// <JOURNAL.cpp>
JournalState CurrentJournalState;

// <POKEDEX.cpp>
PokedexState CurrentPokedexState = POKE_LIST;

// UTF-8 Keyboard Layout System
KeyboardLayout CurrentLayout;
std::vector<DeadRule> DeadTable;
String CurrentDead = "";
String CurrentLayoutName = "us-latin"; // default; persisted in Preferences

// Initialize a basic fallback keyboard layout when JSON loading fails
void initializeFallbackLayout() {
  CurrentLayout.name = "Fallback US";
  CurrentLayout.description = "Basic fallback layout";
  
  // Initialize normal layer - basic QWERTY
  CurrentLayout.normal[0][0] = {KA_CHAR, "1"}; CurrentLayout.normal[0][1] = {KA_CHAR, "2"}; CurrentLayout.normal[0][2] = {KA_CHAR, "3"}; CurrentLayout.normal[0][3] = {KA_CHAR, "4"}; CurrentLayout.normal[0][4] = {KA_CHAR, "5"};
  CurrentLayout.normal[0][5] = {KA_CHAR, "6"}; CurrentLayout.normal[0][6] = {KA_CHAR, "7"}; CurrentLayout.normal[0][7] = {KA_CHAR, "8"}; CurrentLayout.normal[0][8] = {KA_CHAR, "9"}; CurrentLayout.normal[0][9] = {KA_CHAR, "0"};
  
  CurrentLayout.normal[1][0] = {KA_CHAR, "q"}; CurrentLayout.normal[1][1] = {KA_CHAR, "w"}; CurrentLayout.normal[1][2] = {KA_CHAR, "e"}; CurrentLayout.normal[1][3] = {KA_CHAR, "r"}; CurrentLayout.normal[1][4] = {KA_CHAR, "t"};
  CurrentLayout.normal[1][5] = {KA_CHAR, "y"}; CurrentLayout.normal[1][6] = {KA_CHAR, "u"}; CurrentLayout.normal[1][7] = {KA_CHAR, "i"}; CurrentLayout.normal[1][8] = {KA_CHAR, "o"}; CurrentLayout.normal[1][9] = {KA_CHAR, "p"};
  
  CurrentLayout.normal[2][0] = {KA_CHAR, "a"}; CurrentLayout.normal[2][1] = {KA_CHAR, "s"}; CurrentLayout.normal[2][2] = {KA_CHAR, "d"}; CurrentLayout.normal[2][3] = {KA_CHAR, "f"}; CurrentLayout.normal[2][4] = {KA_CHAR, "g"};
  CurrentLayout.normal[2][5] = {KA_CHAR, "h"}; CurrentLayout.normal[2][6] = {KA_CHAR, "j"}; CurrentLayout.normal[2][7] = {KA_CHAR, "k"}; CurrentLayout.normal[2][8] = {KA_CHAR, "l"}; CurrentLayout.normal[2][9] = {KA_ENTER, ""};
  
  CurrentLayout.normal[3][0] = {KA_SHIFT, ""}; CurrentLayout.normal[3][1] = {KA_CHAR, "z"}; CurrentLayout.normal[3][2] = {KA_CHAR, "x"}; CurrentLayout.normal[3][3] = {KA_CHAR, "c"}; CurrentLayout.normal[3][4] = {KA_CHAR, "v"};
  CurrentLayout.normal[3][5] = {KA_CHAR, "b"}; CurrentLayout.normal[3][6] = {KA_CHAR, "n"}; CurrentLayout.normal[3][7] = {KA_CHAR, "m"}; CurrentLayout.normal[3][8] = {KA_BACKSPACE, ""}; CurrentLayout.normal[3][9] = {KA_FN, ""};
  
  // Initialize shift layer
  CurrentLayout.shift_[0][0] = {KA_CHAR, "!"}; CurrentLayout.shift_[0][1] = {KA_CHAR, "@"}; CurrentLayout.shift_[0][2] = {KA_CHAR, "#"}; CurrentLayout.shift_[0][3] = {KA_CHAR, "$"}; CurrentLayout.shift_[0][4] = {KA_CHAR, "%"};
  CurrentLayout.shift_[0][5] = {KA_CHAR, "^"}; CurrentLayout.shift_[0][6] = {KA_CHAR, "&"}; CurrentLayout.shift_[0][7] = {KA_CHAR, "*"}; CurrentLayout.shift_[0][8] = {KA_CHAR, "("}; CurrentLayout.shift_[0][9] = {KA_CHAR, ")"};
  
  CurrentLayout.shift_[1][0] = {KA_CHAR, "Q"}; CurrentLayout.shift_[1][1] = {KA_CHAR, "W"}; CurrentLayout.shift_[1][2] = {KA_CHAR, "E"}; CurrentLayout.shift_[1][3] = {KA_CHAR, "R"}; CurrentLayout.shift_[1][4] = {KA_CHAR, "T"};
  CurrentLayout.shift_[1][5] = {KA_CHAR, "Y"}; CurrentLayout.shift_[1][6] = {KA_CHAR, "U"}; CurrentLayout.shift_[1][7] = {KA_CHAR, "I"}; CurrentLayout.shift_[1][8] = {KA_CHAR, "O"}; CurrentLayout.shift_[1][9] = {KA_CHAR, "P"};
  
  CurrentLayout.shift_[2][0] = {KA_CHAR, "A"}; CurrentLayout.shift_[2][1] = {KA_CHAR, "S"}; CurrentLayout.shift_[2][2] = {KA_CHAR, "D"}; CurrentLayout.shift_[2][3] = {KA_CHAR, "F"}; CurrentLayout.shift_[2][4] = {KA_CHAR, "G"};
  CurrentLayout.shift_[2][5] = {KA_CHAR, "H"}; CurrentLayout.shift_[2][6] = {KA_CHAR, "J"}; CurrentLayout.shift_[2][7] = {KA_CHAR, "K"}; CurrentLayout.shift_[2][8] = {KA_CHAR, "L"}; CurrentLayout.shift_[2][9] = {KA_ENTER, ""};
  
  CurrentLayout.shift_[3][0] = {KA_SHIFT, ""}; CurrentLayout.shift_[3][1] = {KA_CHAR, "Z"}; CurrentLayout.shift_[3][2] = {KA_CHAR, "X"}; CurrentLayout.shift_[3][3] = {KA_CHAR, "C"}; CurrentLayout.shift_[3][4] = {KA_CHAR, "V"};
  CurrentLayout.shift_[3][5] = {KA_CHAR, "B"}; CurrentLayout.shift_[3][6] = {KA_CHAR, "N"}; CurrentLayout.shift_[3][7] = {KA_CHAR, "M"}; CurrentLayout.shift_[3][8] = {KA_BACKSPACE, ""}; CurrentLayout.shift_[3][9] = {KA_FN, ""};
  
  // Initialize fn layer with symbols and navigation
  CurrentLayout.fn[0][0] = {KA_CHAR, "1"}; CurrentLayout.fn[0][1] = {KA_CHAR, "2"}; CurrentLayout.fn[0][2] = {KA_CHAR, "3"}; CurrentLayout.fn[0][3] = {KA_CHAR, "4"}; CurrentLayout.fn[0][4] = {KA_CHAR, "5"};
  CurrentLayout.fn[0][5] = {KA_CHAR, "6"}; CurrentLayout.fn[0][6] = {KA_CHAR, "7"}; CurrentLayout.fn[0][7] = {KA_CHAR, "8"}; CurrentLayout.fn[0][8] = {KA_CHAR, "9"}; CurrentLayout.fn[0][9] = {KA_CHAR, "0"};
  
  CurrentLayout.fn[1][0] = {KA_CHAR, "-"}; CurrentLayout.fn[1][1] = {KA_CHAR, "="}; CurrentLayout.fn[1][2] = {KA_CHAR, "["}; CurrentLayout.fn[1][3] = {KA_CHAR, "]"}; CurrentLayout.fn[1][4] = {KA_CHAR, "\\"};
  CurrentLayout.fn[1][5] = {KA_LEFT, ""}; CurrentLayout.fn[1][6] = {KA_RIGHT, ""}; CurrentLayout.fn[1][7] = {KA_UP, ""}; CurrentLayout.fn[1][8] = {KA_DOWN, ""}; CurrentLayout.fn[1][9] = {KA_CHAR, "`"};
  
  CurrentLayout.fn[2][0] = {KA_TAB, ""}; CurrentLayout.fn[2][1] = {KA_SAVE, ""}; CurrentLayout.fn[2][2] = {KA_ESC, ""}; CurrentLayout.fn[2][3] = {KA_FILE, ""}; CurrentLayout.fn[2][4] = {KA_FONT, ""};
  CurrentLayout.fn[2][5] = {KA_HOME, ""}; CurrentLayout.fn[2][6] = {KA_CHAR, ";"}; CurrentLayout.fn[2][7] = {KA_CHAR, "'"}; CurrentLayout.fn[2][8] = {KA_LOAD, ""}; CurrentLayout.fn[2][9] = {KA_ENTER, ""};
  
  CurrentLayout.fn[3][0] = {KA_SHIFT, ""}; CurrentLayout.fn[3][1] = {KA_CHAR, ","}; CurrentLayout.fn[3][2] = {KA_CHAR, "."}; CurrentLayout.fn[3][3] = {KA_CLEAR, ""}; CurrentLayout.fn[3][4] = {KA_CHAR, "/"};
  CurrentLayout.fn[3][5] = {KA_SPACE, " "}; CurrentLayout.fn[3][6] = {KA_CHAR, "?"}; CurrentLayout.fn[3][7] = {KA_CHAR, ":"}; CurrentLayout.fn[3][8] = {KA_BACKSPACE, ""}; CurrentLayout.fn[3][9] = {KA_FN, ""};
  
  // No dead keys in fallback layout
  CurrentLayout.deadKeys.clear();
  
  // Mirror to legacy arrays
  mirrorLayoutToLegacy();
}

// Dead-key composition (data-driven)
String composeDeadIfAny(const String& base) {
  if (CurrentDead.length() == 0) return base;
  
  std::cout << "[DEAD] Looking for accent: '" << CurrentDead.c_str() << "' + base: '" << base.c_str() << "'" << std::endl;
  std::cout << "[DEAD] DeadTable size: " << DeadTable.size() << std::endl;
  
  for (const auto& r : DeadTable) {
    std::cout << "[DEAD] Checking rule: '" << r.accent.c_str() << "' + '" << r.base.c_str() << "' -> '" << r.out.c_str() << "'" << std::endl;
    if (r.accent == CurrentDead && r.base == base) {
      CurrentDead = "";
      std::cout << "[DEAD] Match found! Returning: '" << r.out.c_str() << "'" << std::endl;
      return r.out;
    }
  }
  // fallback: just emit the base character (no accent found)
  std::cout << "[DEAD] No match found, returning base character: '" << base.c_str() << "'" << std::endl;
  CurrentDead = "";
  return base;
}