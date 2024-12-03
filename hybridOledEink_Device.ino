#include  <Arduino.h>
#include  <GxEPD2_BW.h>
#include  <U8g2lib.h>
#include  <Wire.h>
#include  <Adafruit_TCA8418.h>
#include  "freertos/FreeRTOS.h"
#include  "freertos/task.h"

#include  <Fonts/FreeMonoBold9pt7b.h>

//PINS
#define   I2C_SCL               35
#define   I2C_SDA               36
#define   KB_IRQ                 8

//CONFIG
#define   KB_COOLDOWN          100  //Keypress cooldown
#define   FULL_REFRESH_AFTER    20  //Perform a full refresh after __ chars

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
    { 9 , '%', '-', '&', '+', '_', '/', '?' , ',' , 13 },     //9:TAB, 13:CR
    { 0 , 17 , 18 , ' ', ' ', ' ', 19 , 20  , 21  ,  0 }      //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};


//VARIABLES
String currentWord            = "";
String allText                = "";
String prevAllText            = "";
volatile int einkRefresh      = 20;
int OLEDFPSMillis             = 0;
int KBBounceMillis            = 0;
volatile bool TCA8418_event   = false;
volatile bool SHFT            = false;
volatile bool FN              = false;

//FUNCTIONS
void TCA8418_irq() {
  TCA8418_event = true;
}

char updateKeypress() {
  if (TCA8418_event == true) {
    int k = keypad.getEvent();
    
    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0) TCA8418_event = false;

    /*if (k & 0x80) Serial.print("PRESS\tR: ");
    else Serial.print("RELEASE\tR: ");
    k &= 0x7F;
    k--;
    Serial.print(k / 10);
    Serial.print("\tC: ");
    Serial.print(k % 10);
    Serial.println();*/
    if (k & 0x80) {   //Key pressed, not released
      k &= 0x7F;
      k--;
      return keysArray[k/10][k%10];
    }
    else return 0;

  }
  else return 0;
}

void processKB() {
  int currentMillis = millis();
  //Make sure oled only updates at 60fps
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = updateKeypress();
    
    if (inchar == 0);                             //No char recieved
    else if (inchar == 32) {                      //Space Recieved
      allText += (currentWord + " ");
      currentWord = "";
    }
    else if (inchar == 13) {                      //CR Recieved
      allText += (currentWord + "\n");
      currentWord = "";
    }
    else if (inchar == 20) {                      //ESC Recieved
      allText ="";
      currentWord = "";
      oledWord("Clearing...");
      delay(500);
    }
    else if (inchar == 127 || inchar == 8) {      //BKSP recieved
      if (currentWord.length() > 0) {
        currentWord = currentWord.substring(0, currentWord.length() - 1);
      }
    }
    else currentWord += inchar;
    
    currentMillis = millis();
    //Make sure oled only updates at 60fps
    if (currentMillis - OLEDFPSMillis >= 16) {
      OLEDFPSMillis = currentMillis;
      oledWord(currentWord);
    }

    KBBounceMillis = currentMillis;
  }

  
}

char scanSerial() {
  char incomingChar;

  if (Serial.available() > 0) {
    // Read the incoming character
    incomingChar = Serial.read();
    
    // Echo the character back to the terminal
    Serial.print("You typed: ");
    Serial.println(incomingChar);
    return incomingChar;
  }
  return 255;
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

void einkText(String text) {
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 10);
  display.print(text);
  display.nextPage();
  display.hibernate();
}

void einkTextPartial(String text) {
  display.setFont(&FreeMonoBold9pt7b);

  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  int bottomY = 10+tbh;

  display.setPartialWindow(0,0,display.width(),display.height());
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(0, 10);
  display.print(text);
  display.nextPage();
  display.hibernate();
  display.setFullWindow();
}

void einkHandler(void *parameter) {
  while (true) {
    if (prevAllText != allText) {
      if (allText ==  "") {
        einkText(allText);
      }

      prevAllText = allText;
      einkRefresh++;

      if (einkRefresh > FULL_REFRESH_AFTER) {
        einkText(allText);
        einkRefresh = 0;
      }
      else einkTextPartial(allText);
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
    NULL,                   // Task handle (optional)
    1                       // Core ID (0 for core 0, 1 for core 1)
  );
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  //OLED SETUP
  u8g2.begin();
  u8g2.clearBuffer();	
  u8g2.sendBuffer();

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

  //STARTING LOOP
}

void loop() {
  processKB();
}
