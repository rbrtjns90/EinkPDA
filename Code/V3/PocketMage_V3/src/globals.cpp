#include "globals.h"

//   .oooooo..o oooooooooooo ooooooooooooo ooooo     ooo ooooooooo.    //
//  d8P'    `Y8 `888'     `8 8'   888   `8 `888'     `8' `888   `Y88.  //
//  Y88bo.       888              888       888       8   888   .d88'  //
//   `"Y8888o.   888oooo8         888       888       8   888ooo88P'   //
//       `"Y88b  888    "         888       888       8   888          //
//  oo     .d8P  888       o      888       `88.    .8'   888          //
//  8""88888P'  o888ooooood8     o888o        `YbodP'    o888o         //

// Display setup
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
//volatile bool useFastFullUpdate = true;
volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, OLED_CS, OLED_DC, OLED_RST); //256x32
//U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); //128x32

// Keypad setup
Adafruit_TCA8418 keypad;

// Buzzer
Buzzer buzzer(17);

char keysArray[4][10] = {
  { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' },
  { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 8 },  //8:BKSP
  { 9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', 13 },   //9:TAB, 13:CR
  { 0, 17, 18, ' ', ' ', ' ', 19, 20, 21, 0 }          //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:-> 
};

char keysArraySHFT[4][10] = {
  { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
  { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 8 },
  { 9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', 13 },
  { 0, 17, 18, ' ', ' ', ' ', 19, 20, 21, 0 }
};

char keysArrayFN[4][10] = {
  { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' },
  { '#', '!', '$', ':', ';', '(', ')', '\'', '\"', 12 },
  { 14, '%', '_', '&', '+', '-', '/', '?', ',', 13 },
  { 0, 17, 18, ' ', ' ', ' ', 5, 7, 6, 0 }
};

// Touch slider setup
Adafruit_MPR121 cap = Adafruit_MPR121();

// RTC setup
RTC_PCF8563 rtc;
const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// USB
USBMSC msc;

// VARIABLES
  // GENERAL
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
  volatile bool forceSlowFullUpdate = true;
  volatile bool SDCARD_INSERT = false;
  bool noSD = false;

  const String appStateNames[] = { "txt", "filewiz", "usb", "bt", "settings", "tasks" };
  const unsigned char *appIcons[6] = { _homeIcons2, _homeIcons3, _homeIcons4, _homeIcons5, _homeIcons6, taskIconTasks0 };
  AppState CurrentAppState = HOME;

  // <TXT.ino>
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
  String editingFile = "";
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

  // <TASKS.ino>
  std::vector<std::vector<String>> tasks;
  uint8_t selectedTask = 0;
  TasksState CurrentTasksState = TASKS0;
  uint8_t newTaskState = 0;
  uint8_t editTaskState = 0;
  String newTaskName = "";
  String newTaskDueDate = "";

  // <HOME.ino>
  HOMEState CurrentHOMEState = HOME_HOME;

  // <FILEWIZ.ino> 
  FileWizState CurrentFileWizState = WIZ0_;
  String workingFile = "";