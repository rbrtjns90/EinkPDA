// @Ashtf 2025
// https://patorjk.com/software/taag/#p=display&v=0&f=Roman&t=Type%20Something%20

//   .oooooo..o oooooooooooo ooooooooooooo ooooo     ooo ooooooooo.    //
//  d8P'    `Y8 `888'     `8 8'   888   `8 `888'     `8' `888   `Y88.  //
//  Y88bo.       888              888       888       8   888   .d88'  //
//   `"Y8888o.   888oooo8         888       888       8   888ooo88P'   //
//       `"Y88b  888    "         888       888       8   888          //
//  oo     .d8P  888       o      888       `88.    .8'   888          //
//  8""88888P'  o888ooooood8     o888o        `YbodP'    o888o         //  

// LIBRARIES
#include  <Arduino.h>
#include  <GxEPD2_BW.h>
#include  <U8g2lib.h>
#include  <Wire.h>
#include  <Adafruit_TCA8418.h>
#include  "freertos/FreeRTOS.h"
#include  "freertos/task.h"
#include  "mpr121.h"
#include  "assets.h"
#include  "FS.h"
#include  "SPIFFS.h"

// FONTS
#include  <Fonts/FreeMonoBold9pt7b.h>

// PIN DEFINITION
#define   I2C_SCL         35
#define   I2C_SDA         36
#define   KB_IRQ           8
#define   PWR_BTN         38
#define   MAG_SENS         3
#define   BAT_SENS         6
#define   MPR121_ADDR   0x5A

// CONFIGURATION & SETTINGS
#define   KB_COOLDOWN                  50  // Keypress cooldown
#define   FULL_REFRESH_AFTER           20  // Perform a full refresh after __ chars
#define   MAX_FILES                    10  // Number of files to store
#define   TIMEOUT                      15  // Time until automatic sleep (Seconds)
#define   FORMAT_SPIFFS_IF_FAILED    true  // Format the SPIFFS filesystem if mount fails
const     String SLEEPMODE =       "TEXT"; // TEXT, SPLASH, CALENDAR

// DISPLAY AND KEYBOARD SETUP
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(/*CS=*/ 2, /*DC=*/ 21, /*RST=*/ 16, /*BUSY=*/ 37)); 
U8G2_SH1106_128X32_VISIONOX_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 
Adafruit_TCA8418 keypad;

char keysArray[4][10] = {
  {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
  {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',  8 },       //8:BKSP
  { 9 , 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', 13 },       //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ', 19 , 20 , 21 ,  0 }        //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};

char keysArraySHFT[4][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',  8 },       //8:BKSP
  { 9 , 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', 13 },       //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ', 19 , 20 , 21 ,  0 }        //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};

char keysArrayFN[4][10] = {
  {'1', '2', '3', '4', '5', '6', '7' , '8' , '9', '0'},
  {'#', '!', '$', ':', ';', '(', ')', '\'', '\"',  8 },     //8:BKSP
  { 9 , '%', '-', '&', '+', '-', '/', '?' , ',' , 13 },     //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ',  5 ,  7  ,  6  ,  0 }      //17:SHFT, 18:FN, 5:LOAD, 20:SEL, 6:SAVE, 7:FILE
};

// TOUCH SLIDER SETUP
bool touchStates[12];

// VARIABLES
String          currentWord     = "";
String          allText         = "";
String          prevAllText     = "";
String          prevLastLine    = "";

volatile int    einkRefresh     = FULL_REFRESH_AFTER;
int             OLEDFPSMillis   = 0;
int             KBBounceMillis  = 0;
volatile int    timeoutMillis   = 0;
volatile int    prevTimeMillis  = 0;
volatile bool   TCA8418_event   = false;
volatile bool   SHFT            = false;
volatile bool   FN              = false;
volatile bool   newState        = true;
bool            prevBKSP        = false;
bool            noTimeout       = false;

int             scroll          = 0;
int             lines           = 0;
String          outLines[13];
String          lines_prev[13];
String          filesList[MAX_FILES];
uint8_t         fileIndex       = 0;
String          editingFile     = "";
String          prevEditingFile = "";
String          excludedFiles[] = {"/temp.txt", "/settings.txt"};

TaskHandle_t einkHandlerTaskHandle = NULL;

char currentKB[4][10];

enum KBState {NORMAL, SHIFT, FUNC};
KBState CurrentKBState = NORMAL;

//        .o.       ooooooooo.   ooooooooo.    .oooooo..o  //
//       .888.      `888   `Y88. `888   `Y88. d8P'    `Y8  //  
//      .8"888.      888   .d88'  888   .d88' Y88bo.       //
//     .8' `888.     888ooo88P'   888ooo88P'   `"Y8888o.   //
//    .88ooo8888.    888          888              `"Y88b  //
//   .8'     `888.   888          888         oo     .d8P  //
//  o88o     o8888o o888o        o888o        8""88888P'   //

// ADD APPLICATION, NAME, AND ICON TO LISTS
enum AppState                     {HOME   , TXT   , FILEWIZ   , USB   , BT  , SETTINGS  , DEBUG   };
const String appStateNames[] =    {"HOME" , "TXT" , "FILEWIZ" , "USB" , "BT", "SETTINGS", "DEBUG" };
const unsigned char* appIcons[] = {_homeIcons2,_homeIcons3,_homeIcons4,_homeIcons5,_homeIcons6};

// ADD E-INK HANDLER APP SCRIPTS HERE
void applicationEinkHandler() {
  switch (CurrentAppState) {
    case HOME:
      einkHandler_HOME();
      break;
    case TXT:
      einkHandler_TXT();
      break;
    case FILEWIZ:
      einkHandler_FILEWIZ();
      break;
    case DEBUG:
      break;
    // ADD APP CASES HERE
    default:
      einkHandler_HOME();
      break;

  }
}

// ADD PROCESS/KEYBOARD APP SCRIPTS HERE
void processKB() {
  switch (CurrentAppState) {
    case HOME:
      processKB_HOME();
      break;
    case TXT:
      processKB_TXT();
      break;
    case FILEWIZ:
      processKB_FILEWIZ();
      break;
    case DEBUG:
      break;
    // ADD APP CASES HERE
    default:
      einkHandler_HOME();
      break;
  }  
}

//  ooo        ooooo       .o.       ooooo ooooo      ooo  //
//  `88.       .888'      .888.      `888' `888b.     `8'  //
//   888b     d'888      .8"888.      888   8 `88b.    8   //
//   8 Y88. .P  888     .8' `888.     888   8   `88b.  8   //
//   8  `888'   888    .88ooo8888.    888   8     `88b.8   //
//   8    Y     888   .8'     `888.   888   8       `888   //
//  o8o        o888o o88o     o8888o o888o o8o        `8   //

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // EINK HANDLER SETUP
  xTaskCreatePinnedToCore(
    einkHandler,            // Function name (your user-defined function)
    "einkHandlerTask",      // Task name
    10000,                  // Stack size (in bytes)
    NULL,                   // Parameters (none in this case)
    1,                      // Priority (1 is low priority)
    &einkHandlerTaskHandle, // Task handle
    1                       // Core ID (0 for core 0, 1 for core 1)
  );
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  // MPR121 / SLIDER
  // mpr121_setup();

  // POWER SETUP
  pinMode(PWR_BTN, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);

  // OLED SETUP
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.clearBuffer();	
  u8g2.sendBuffer();

  // KEYBOARD SETUP
  if (! keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("Error Initializing the Keyboard");
    while (1);
  }
  keypad.matrix(4, 10);
  pinMode(KB_IRQ, INPUT);
  attachInterrupt(digitalPinToInterrupt(KB_IRQ), TCA8418_irq, CHANGE);
  keypad.flush();
  keypad.enableInterrupts();

  // SPIFFS SETUP
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    oledWord("SPIFFS Failed");
    delay(1000);
  }

  AppState CurrentAppState = HOME;
}

void loop() {
  if (!noTimeout) checkTimeout();
  processKB();
}