#include "globals.h"
#ifdef DESKTOP_EMULATOR
#include "U8g2lib.h"
#endif

// Vector to hold the definitions
std::vector<std::pair<String, String>> defList;
int definitionIndex = 0;

void LEXICON_INIT() {
  // OPEN SETTINGS
  currentLine = "";
  CurrentAppState = LEXICON;
  CurrentLexState = MENU;
  CurrentKBState  = NORMAL;
  newState = true;
  definitionIndex = 0;
}

void loadDefinitions(String word) {
  oledWord("Loading Definitions");
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  defList.clear();  // Clear previous results

  if (word.length() == 0 || noSD) return;

  char firstChar = tolower(word[0]);
  if (firstChar < 'a' || firstChar > 'z') return;

  String filePath = "/dict/" + String((char)toupper(firstChar)) + ".txt";

  File file = SD_MMC.open(filePath);
  if (!file) {
    oledWord("Missing Dictionary!");
    delay(2000);
    return;
  }

  word.toLowerCase();

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int defSplit = line.indexOf(')');
    if (defSplit == -1) continue;

    // Extract key and definition
    String key = line.substring(0, defSplit + 1);
    String def = line.substring(defSplit + 1);
    def.trim();

    String keyLower = key;
    keyLower.toLowerCase();

    if (keyLower.startsWith(word)) {
      defList.push_back({key, def});
    }
    else if (defList.size() > 0) {
      // No more definitions
      break;
    }
  }

  file.close();

  if (defList.empty()) {
    oledWord("No definitions found");
    delay(2000);
  }
  else {
    CurrentLexState = DEF;
    CurrentKBState  = NORMAL;
    definitionIndex = 0;
    newState = true;
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void processKB_LEXICON() {
  int currentMillis = millis();

  switch (CurrentLexState) {
    case MENU:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          loadDefinitions(currentLine);
          currentLine = "";
        }                                      
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          currentLine += " ";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // Home recieved
        else if (inchar == 12 || inchar == 27) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }
        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentLine, false);
        }
      }
      break;

    case DEF:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          loadDefinitions(currentLine);
          currentLine = "";
        }                                      
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          currentLine += " ";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // Home recieved
        else if (inchar == 12 || inchar == 27) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }

        // LEFT Recieved
        else if (inchar == 19) {
          definitionIndex--;
          if (definitionIndex < 0) definitionIndex = 0;
          newState = true;
        }
        // RIGHT Received
        else if (inchar == 21) {
          definitionIndex++;
          if (definitionIndex >= defList.size()) definitionIndex = defList.size() - 1;
          newState = true;
        }

        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentLine, false);
        }
      }
      break;
  }
}

void einkHandler_LEXICON() {
  switch (CurrentLexState) {
    case MENU:
      if (newState) {
        newState = false;

        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(0, 0, _lex0, 320, 218, GxEPD_BLACK);

        drawStatusBar("Type a Word:");

        multiPassRefesh(2);
      }
      break;
    case DEF:
      if (newState) {
        newState = false;

        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(0, 0, _lex1, 320, 218, GxEPD_BLACK);

        display.setTextColor(GxEPD_BLACK);

        // Draw Word
        display.setFont(&FreeSerif12pt7b);
        display.setCursor(12, 50);
        display.print(defList[definitionIndex].first);

        // Draw Definition
        display.setFont(&FreeSerif9pt7b);
        display.setCursor(8, 87);
        // ADD WORD WRAP
        display.print(defList[definitionIndex].second);

        drawStatusBar("Type a New Word:");

        forceSlowFullUpdate = true;
        refresh();
      }
      break;
  }
  
}


