#include  <Arduino.h>
#include  <GxEPD2_BW.h>
#include  <U8g2lib.h>
#include  <Wire.h>
#include  <Adafruit_TCA8418.h>
#include  "Adafruit_MPR121.h"
#include  "freertos/FreeRTOS.h"
#include  "mpr121.h"
#include  "freertos/task.h"
#include  "assets.h"
#include  "FS.h"
#include  "SPIFFS.h"

#include  <Fonts/FreeMonoBold9pt7b.h>

//PINS
#define   I2C_SCL               35
#define   I2C_SDA               36
#define   KB_IRQ                 8
#define   PWR_BTN               38
#define   MAG_SENS               3
#define   BAT_SENS               6
#define   MPR121_ADDR         0x5A

//CONFIG
#define   KB_COOLDOWN                  50  //Keypress cooldown
#define   FULL_REFRESH_AFTER           20  //Perform a full refresh after __ chars
#define   MAX_FILES                    10  //Number of files to store
#define   TIMEOUT                      15  //Time until automatic sleep (Seconds)
#define   FORMAT_SPIFFS_IF_FAILED   false

//DISPLAY AND KEYBOARD SETUP
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
  {'0', '1', '2', '3', '4', '5', '6', '7' , '8' , '9'},
  {'#', '!', '$', ':', ';', '(', ')', '\'', '\"',  8 },     //8:BKSP
  { 9 , '%', '-', '&', '+', '-', '/', '?' , ',' , 13 },     //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ',  5 ,  7  ,  6  ,  0 }      //17:SHFT, 18:FN, 5:LOAD, 20:SEL, 6:SAVE, 7:FILE
};

//VARIABLES
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
int             scroll          = 0;
int             lines           = 0;
String          outLines[13];
String          lines_prev[13];
String          filesList[MAX_FILES];
uint8_t         fileIndex       = 0;
String          editingFile     = "";
String          prevEditingFile = "";

boolean         touchStates[12];

TaskHandle_t einkHandlerTaskHandle = NULL;

char currentKB[4][10];

enum KBState {NORMAL, SHIFT, FUNC};
KBState CurrentKBState = NORMAL;

enum AppState {HOME, TXT, FILEWIZ, USB, BT, SETTINGS, DEBUG};
AppState CurrentAppState = HOME;

//FUNCTIONS
void TCA8418_irq() {
  TCA8418_event = true;
}

void checkTimeout() {
  timeoutMillis = millis();

  //Trigger timeout deep sleep
  if (timeoutMillis - prevTimeMillis >= TIMEOUT*1000) {
    Serial.println("Device Idle... Deep Sleeping");
    //Give a chance to keep device awake
    oledWord("Going to sleep!");
    int i = millis();
    int j = millis();
    while ((j - i) <= 4000) {  //10 sec
      j = millis();
      if (digitalRead(KB_IRQ) == 0) {
        oledWord("My Hero!");
        delay(500);
        prevTimeMillis = millis();
        TCA8418_event = false;
        return;
      }
    }


    //Save current work:
    //Only save if alltext has significant content
    if (allText.length() > 10) {
      //No current file, save in temp.txt
      if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
      //Save to SPIFFS
      keypad.disableInterrupts();
      oledWord("Saving File");
      delay(200);
      writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
      oledWord("Saved");
      delay(200);
      keypad.enableInterrupts();
    }

    //Put OLED to sleep
    u8g2.setPowerSave(1);

    //Stop the einkHandler task
    if (einkHandlerTaskHandle != NULL) {
      vTaskDelete(einkHandlerTaskHandle);
      einkHandlerTaskHandle = NULL;
    }

    //Display sleep image on E-Ink
    display.setFullWindow();
    display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
    display.nextPage();
    display.hibernate();
    
    //Sleep the device
    esp_deep_sleep_start();
  }
}

char updateKeypress() {
  if (TCA8418_event == true) {
    int k = keypad.getEvent();
    
    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0) TCA8418_event = false;

    if (k & 0x80) {   //Key pressed, not released
      k &= 0x7F;
      k--;
      //return currentKB[k/10][k%10];
      if ((k/10) < 4) {
        //Key was pressed, reset timeout counter
        prevTimeMillis = millis();

        //Return Key
        switch (CurrentKBState) {
          case NORMAL:
            return keysArray[k/10][k%10];
            break;
          case SHIFT:
            return keysArraySHFT[k/10][k%10];
            break;
          case FUNC:
            return keysArrayFN[k/10][k%10];
            break;
        }
      }
      else return 0;
    }
    else return 0;
  }
  else return 0;
}

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
  }  
}

void oledWord(String word) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB24_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+12,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB18_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+9,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB14_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+7,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB12_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+6,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB10_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+5,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  else {
    u8g2.drawStr(128 - u8g2.getStrWidth(word.c_str()),16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  
}

void bottomText(String input) {
  display.setFont(&FreeMonoBold9pt7b);
  display.setPartialWindow(0,display.height()-20,display.width(),20);
  display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
  display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
  display.setCursor(4, display.height()-6);
  display.print(input);

  switch (CurrentKBState) {
    case NORMAL:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
      break;
    case SHIFT:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[0], 30, 20, GxEPD_BLACK);
      break;
    case FUNC:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[1], 30, 20, GxEPD_BLACK);
      break;
  }
  display.drawRect(display.width()-30,display.height()-20,30,20,GxEPD_BLACK);
}

void refresh() {
  display.nextPage();
  display.hibernate();
  display.fillScreen(GxEPD_WHITE);
}

void einkHandler(void *parameter) {
  delay(1000);
  while (true) {
    switch (CurrentAppState) {
      case HOME:
        einkHandler_HOME();
      case TXT:
        einkHandler_TXT();
        break;
      case FILEWIZ:
        einkHandler_FILEWIZ();
        break;
      case DEBUG:
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(0, 10);
        
        float vBat = 2.0*(float)analogReadMilliVolts(BAT_SENS)/1000.0 + 0.2;
        display.print("Battery Voltage   : " + String(vBat) + "V\n");
        
        display.print("Power Button State: " + String(digitalRead(PWR_BTN)) + "\n");
        
        display.print("Hall Effect State : " + String(digitalRead(MAG_SENS)) + "\n");
        
        display.print("ESP32 Chip Temp   : " + String(temperatureRead()) + "C\n");
        
        /*size_t totalBytes = SPIFFS.totalBytes();
        size_t usedBytes = SPIFFS.usedBytes();
        size_t freeBytes = totalBytes - usedBytes;
        int freePercent = (freeBytes/totalBytes)*100;
        display.print("SPIFFS Usage: \n");
        display.print(String(usedBytes) + "/" + String(totalBytes) + "Bytes Used\n(" + String(freePercent) + "% free)\n");*/

        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.drawRect(display.width()-30,display.height()-20,30,20,GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(4, display.height()-6);
        display.print("***DEBUG MODE***");
        refresh();
        delay(5000);
        break;
    }
  }
}

//SETUP AND LOOP
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  //EINK SETUP
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

  //MPR121 SETUP

  //POWER SETUP
  pinMode(PWR_BTN, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);

  //OLED SETUP
  u8g2.begin();
  u8g2.clearBuffer();	
  u8g2.sendBuffer();
  u8g2.setPowerSave(0);

  //KEYBOARD SETUP
  if (! keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("Error Initializing the Keyboard");
    while (1);
  }
  keypad.matrix(4, 10);
  pinMode(KB_IRQ, INPUT);
  attachInterrupt(digitalPinToInterrupt(KB_IRQ), TCA8418_irq, CHANGE);
  keypad.flush();
  keypad.enableInterrupts();

  //SPIFFS SETUP
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    oledWord("SPIFFS Failed");
    delay(1000);
  }
}

void loop() {
  //Tasks
  checkTimeout();
  processKB();

  //Debug
  //Serial.println(("PWR: "    + String(digitalRead(PWR_BTN))).c_str());
  //Serial.println(("KB IRQ: " + String(digitalRead(KB_IRQ))).c_str());
}
