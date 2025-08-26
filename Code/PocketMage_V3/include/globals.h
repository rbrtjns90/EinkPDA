#ifndef GLOBALS_H
#define GLOBALS_H

// LIBRARIES
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#include <map>
#include <Adafruit_TCA8418.h>
#include <vector>
#include <algorithm>
#include <Buzzer.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include <Preferences.h>
#include <stdint.h>
#include "Adafruit_MPR121.h"
#include "esp_cpu.h"
#include "RTClib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "assets.h"
#include "config.h"

// FONTS
// 3x7
#include "Fonts/Font3x7FixedNum.h"
#include "Fonts/Font4x5Fixed.h"
#include "Fonts/Font5x7Fixed.h"

// 9x7
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
// 12x7
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
// U8G2 FONTS
//U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
//u8g2_font_4x6_mf
//u8g2_font_5x7_mf
//u8g2_font_spleen5x8_mf
//u8g2_font_boutique_bitmap_9x9_tf
//u8g2_font_courR08_tf.h

// Display
extern GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display;
extern U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2;           // 256x32 SPI OLED

// Keypad
extern Adafruit_TCA8418 keypad;
extern char keysArray[4][10];
extern char keysArraySHFT[4][10];
extern char keysArrayFN[4][10];

// Buzzer
extern Buzzer buzzer;

// Touch slider
extern Adafruit_MPR121 cap;

// RTC
extern RTC_PCF8563 rtc;
extern const char daysOfTheWeek[7][12];

// USB
#define ARDUINO_USB_MODE 1
extern USBMSC msc;
extern bool mscEnabled;
extern sdmmc_card_t* card;

// GENERAL

// Settings editable on-device
extern Preferences prefs;
extern int TIMEOUT;            // Time until automatic sleep (Seconds)
extern bool DEBUG_VERBOSE;     // Spit out some extra information
extern bool SYSTEM_CLOCK;      // Enable a small clock on the bottom of the screen.
extern bool SHOW_YEAR;         // Show the year on the clock
extern bool SAVE_POWER;        // Enable a slower CPU clock speed to save battery with little cost to performance
extern bool ALLOW_NO_MICROSD;  // Allow the device to operate with no SD card
extern bool HOME_ON_BOOT;      // Always start the home app on boot
extern int OLED_BRIGHTNESS;    // Brightness of the OLED (0-255)
extern int OLED_MAX_FPS;       // Define the max oled FPS

extern volatile int einkRefresh;
extern int OLEDFPSMillis;
extern int KBBounceMillis;
extern volatile int timeoutMillis;
extern volatile int prevTimeMillis;
extern volatile bool TCA8418_event;
extern volatile bool PWR_BTN_event;
extern volatile bool SHFT;
extern volatile bool FN;
extern volatile bool newState;
extern bool noTimeout;
extern volatile bool OLEDPowerSave;
extern volatile bool disableTimeout;
extern volatile int battState;
extern volatile int prevBattState;
extern unsigned int flashMillis;
extern int prevTime;
extern uint8_t prevSec;
extern TaskHandle_t einkHandlerTaskHandle;
extern char currentKB[4][10];
extern volatile bool SDCARD_INSERT;
extern bool noSD;
extern volatile bool SDActive;

enum KBState { NORMAL, SHIFT, FUNC };
extern KBState CurrentKBState;

// ---- Keyboard (UTF-8 aware) ----
enum KeyAction : uint8_t {
  KA_NONE = 0,
  KA_CHAR,       // text contains UTF-8 character(s)
  KA_DEAD,       // text contains the accent/dead key symbol (e.g., "´")
  KA_BACKSPACE,  // delete previous codepoint
  KA_TAB,
  KA_ENTER,
  KA_SHIFT,      // toggle shift layer
  KA_FN,         // toggle fn layer
  KA_LEFT,
  KA_RIGHT,
  KA_SELECT,
  KA_HOME,
  KA_DELETE,
  KA_SPACE,
  KA_CLEAR,
  KA_FONT,
  // Dead key actions
  KA_DEAD_ACUTE,
  KA_DEAD_GRAVE,
  KA_DEAD_CIRCUMFLEX,
  KA_DEAD_TILDE,
  KA_DEAD_DIAERESIS,
  // App actions
  KA_SAVE,
  KA_LOAD,
  KA_FILE,
  KA_ESC,
  KA_UP,
  KA_DOWN,
  KA_CYCLE_LAYOUT
};

struct KeyMapping {
  KeyAction action;
  String    text;     // UTF-8; used if action is KA_CHAR or KA_DEAD
};

struct KeyEvent {
  bool      hasEvent; // true if an input was produced
  KeyAction action;
  String    text;     // UTF-8 payload
  uint8_t   row;      // 0..3
  uint8_t   col;      // 0..9
};

// 3 layers: normal/shift/fn, 4x10 matrix each
struct KeyboardLayout {
  String name;
  String description;
  KeyMapping normal[4][10];
  KeyMapping shift_[4][10];
  KeyMapping fn[4][10];
  std::map<String, std::map<String, String>> deadKeys;
};

// Dead-key composition rule
struct DeadRule {
  String accent;  // e.g., "´"
  String base;    // e.g., "a"
  String out;     // e.g., "á"
};

// Active keyboard layout and composition table
extern KeyboardLayout CurrentLayout;
extern std::vector<DeadRule> DeadTable;
extern String CurrentDead;         // empty if none
extern String CurrentLayoutName;   // persisted in Preferences

extern uint8_t partialCounter;
extern volatile bool forceSlowFullUpdate;

enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON, POKEDEX, PERIODIC };
extern const String appStateNames[];
extern const unsigned char *appIcons[11];
extern AppState CurrentAppState;

// <TXT.cpp>
extern String currentWord;
extern String allText;
extern String prevAllText;
extern String prevLastLine;
extern bool prevBKSP;
extern int scroll;
extern int lines;
extern String outLines[13];
extern String lines_prev[13];
extern String filesList[MAX_FILES];
extern uint8_t fileIndex;
extern String editingFile;
extern String prevEditingFile;
extern String excludedFiles[3];

enum TXTState { TXT_, WIZ0, WIZ1, WIZ2, WIZ3, FONT };
extern TXTState CurrentTXTState;

extern String currentLine;
extern const GFXfont *currentFont;
extern uint8_t maxCharsPerLine;
extern uint8_t maxLines;
extern uint8_t fontHeight;
extern uint8_t lineSpacing;
extern volatile bool newLineAdded;
extern volatile bool doFull;
extern std::vector<String> allLines;
extern volatile long int dynamicScroll;
extern volatile long int prev_dynamicScroll;
extern int lastTouch;
extern unsigned long lastTouchTime;

// <TASKS.cpp>
extern std::vector<std::vector<String>> tasks;
extern uint8_t selectedTask;
enum TasksState { TASKS0, TASKS0_NEWTASK, TASKS1, TASKS1_EDITTASK };
extern TasksState CurrentTasksState;
extern uint8_t newTaskState;
extern uint8_t editTaskState;
extern String newTaskName;
extern String newTaskDueDate;

// <HOME.cpp>
enum HOMEState { HOME_HOME, NOWLATER };
extern HOMEState CurrentHOMEState;

// <FILEWIZ.cpp>
enum FileWizState { WIZ0_, WIZ1_, WIZ1_YN, WIZ2_R, WIZ2_C, WIZ3_ };
extern FileWizState CurrentFileWizState;
extern String workingFile;

// <settings.cpp>
enum SettingsState { settings0, settings1 };
extern SettingsState CurrentSettingsState;

// <CALENDAR.cpp>
enum CalendarState { WEEK, MONTH, NEW_EVENT, VIEW_EVENT, SUN, MON, TUE, WED, THU, FRI, SAT };
extern CalendarState CurrentCalendarState;

// <LEXICON.cpp>
enum LexState {MENU, DEF};
extern LexState CurrentLexState;

// <JOURNAL.cpp>
enum JournalState {J_MENU, J_TXT};
extern JournalState CurrentJournalState;

// <POKEDEX.cpp>
enum PokedexState {POKE_LIST, POKE_DETAIL, POKE_SEARCH};
extern PokedexState CurrentPokedexState;


// FUNCTION PROTOTYPES
// <sysFunc.cpp>
// SYSTEM
void checkTimeout();
void PWR_BTN_irq();
void TCA8418_irq();
char updateKeypress();
void printDebug();
String vectorToString();
void stringToVector(String inputText);
void saveFile();
void writeMetadata(const String& path);
void loadFile(bool showOLED = true);
void delFile(String fileName);
void deleteMetadata(String path);
void renFile(String oldFile, String newFile);
void renMetadata(String oldPath, String newPath);
void copyFile(String oldFile, String newFile);
void updateBattState();
String removeChar(String str, char character);
void appendToFile(String path, String inText);
void setCpuSpeed(int newFreq);
void playJingle(String jingle);
void deepSleep(bool alternateScreenSaver = false);
void loadState(bool changeState = true);
int stringToInt(String str);

// UTF-8 keyboard layout functions
bool loadKeyboardLayout(const String& filename);
void setKeyboardLayout(const KeyboardLayout& layout);
void initializeFallbackLayout();
String getCurrentLayoutName();
void mirrorLayoutToLegacy();
String composeDeadKey(const String& deadKey, const String& baseChar);
String utf8SafeBackspace(const String& text);
int utf8CharLength(char firstByte);
KeyEvent updateKeypressUTF8();
void cycleKeyboardLayout();

// UTF-8 rendering functions
void printUTF8(int16_t x, int16_t y, const String& text);
void printUTF8(const String& text);
void setCursorUTF8(int16_t x, int16_t y);
void getTextBoundsUTF8(const String& text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

// Layout I/O
bool loadKeyboardLayoutFromFile(const char* path);
bool selectKeyboardLayout(const String& name); // loads /sys/kbd/<name>.json
void applyLayoutToLegacyArrays();              // keeps old ASCII code working

// UTF-8 helpers
int  utf8_length(const String& s);             // number of codepoints
void utf8_pop_back_inplace(String& s);         // erase last codepoint safely
String composeDeadIfAny(const String& base);   // returns composed char(s)
bool loadKeyboardLayout(const String& layoutName); // load keyboard layout

// UTF-8 aware key reader (new); legacy updateKeypress() remains for non-TXT apps
KeyEvent updateKeypressUTF8();

// microSD
void listDir(fs::FS &fs, const char *dirname);
void readFile(fs::FS &fs, const char *path);
String readFileToString(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void setTimeFromString(String timeStr);

// <OLEDFunc.cpp>
void oledWord(String word, bool allowLarge = false, bool showInfo = true);
void oledLine(String line, bool doProgressBar = true, String bottomMsg = "");
void oledScroll();
void infoBar();

// <einkFunc.cpp>
void refresh();
void einkHandler(void *parameter);
void statusBar(String input, bool fullWindow = false);
void einkTextPartial(String text, bool noRefresh = false);
void drawThickLine(int x0, int y0, int x1, int y1, int thickness);
int  countLines(String input, size_t maxLineLength = 29);
void einkTextDynamic(bool doFull_, bool noRefresh = false);
void setTXTFont(const GFXfont *font);
void setFastFullRefresh(bool setting);
void drawStatusBar(String input);
void multiPassRefesh(int passes);

// <FILEWIZ.cpp>
void FILEWIZ_INIT();
void processKB_FILEWIZ();
void einkHandler_FILEWIZ();

// <TXT.cpp>
void TXT_INIT();
void processKB_TXT();
void einkHandler_TXT();
void processKB_TXT_NEW();
void einkHandler_TXT_NEW();
bool splitIntoLines(const char* input, int scroll_);
int countWords(String str);
int countVisibleChars(String input);
void updateScrollFromTouch();

// <HOME.cpp>
void einkHandler_HOME();
void processKB_HOME();
void commandSelect(String command);
void drawHome();

// <TASKS.cpp>
void TASKS_INIT();
void sortTasksByDueDate(std::vector<std::vector<String>> &tasks);
void addTask(String taskName, String dueDate, String priority, String completed);
void updateTaskArray();
void updateTasksFile();
void deleteTask(int index);
String convertDateFormat(String yyyymmdd);
void einkHandler_TASKS();
void processKB_TASKS();

// <settings.cpp>
void SETTINGS_INIT();
void processKB_settings();
void einkHandler_settings();
void settingCommandSelect(String command);

// <USB.cpp>
void USB_INIT();
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize);
static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize);
static bool onStartStop(uint8_t, bool start, bool eject);
static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void processKB_USB();
void einkHandler_USB();
void USBAppSetup();

// <CALENDAR.cpp>
void CALENDAR_INIT();
void processKB_CALENDAR();
void einkHandler_CALENDAR();

// <LEXICON.cpp>
void LEXICON_INIT();
void processKB_LEXICON();
void einkHandler_LEXICON();

// <JOURNAL.cpp>
void JOURNAL_INIT();
void processKB_JOURNAL();
void einkHandler_JOURNAL();

// <POKEDEX.cpp>
void POKEDEX_INIT();
void processKB_POKEDEX();
void einkHandler_POKEDEX();

// <PocketMage>
void applicationEinkHandler();
void processKB();

#endif // GLOBALS_H