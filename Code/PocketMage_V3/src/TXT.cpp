//  ooooooooooooo ooooooo  ooooo ooooooooooooo  //
//  8'   888   `8  `8888    d8'  8'   888   `8  //
//       888         Y888..8P         888       //
//       888          `8888'          888       //
//       888         .8PY888.         888       //
//       888        d8'  `888b        888       //
//      o888o     o888o  o88888o     o888o      //
#include "globals.h"

void TXT_INIT() {
  if (editingFile != "") loadFile();
  CurrentAppState = TXT;
  CurrentKBState  = NORMAL;
  dynamicScroll = 0;
  newLineAdded = true;
}

// OLD MAINS
void processKB_TXT() {
  /*if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }

  disableTimeout = false;

  int currentMillis = millis();
  //Make sure oled only updates at 60fps
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = updateKeypress();
    switch (CurrentTXTState) {
      case TXT_:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          allText += (currentWord + " ");
          currentWord = "";
        }
        //CR Recieved
        else if (inchar == 13) {                          
          allText += (currentWord + "\n");
          currentWord = "";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          allText ="";
          currentWord = "";
          oledWord("Clearing...");
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          delay(300);
        }
        else if (inchar == 19) {                                  
          scroll--;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
        }
        else if (inchar == 21) {                                  
          scroll++;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //SAVE Recieved
        else if (inchar == 6) {
          //File exists, save normally
          if (editingFile != "" && editingFile != "-") {
            keypad.disableInterrupts();
            oledWord("Saving File");
            writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
            oledWord("Saved");
            delay(200);
            keypad.enableInterrupts();
            CurrentKBState = NORMAL;
            newState = true;
          }
          //File does not exist, make a new one
          else {
            CurrentTXTState = WIZ3;
            currentWord = "";
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
          }
        }
        //LOAD Recieved
        else if (inchar == 5) {
          keypad.disableInterrupts();
          oledWord("Loading File");
          allText = readFileToString(SPIFFS, ("/" + editingFile).c_str());
          keypad.enableInterrupts();
          CurrentKBState = NORMAL;
          newState = true;
        }
        //FILE Recieved
        else if (inchar == 7) {
          CurrentTXTState = WIZ0;
          CurrentKBState = NORMAL;
          newState = true;
        }

        else {
          currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
            newState = true;
          }
        }
        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ0:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          display.fillScreen(GxEPD_WHITE);
        }
        else if (inchar >= '0' && inchar <= '9') {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          //Edit a new file
          if (filesList[fileIndex - 1] != editingFile) {
            //Selected file does not exist, create a new one
            if (filesList[fileIndex - 1] == "-") {
              CurrentTXTState = WIZ3;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //Selected file exists, prompt to save current file
            else {      
              prevEditingFile = editingFile;
              editingFile = filesList[fileIndex - 1];      
              CurrentTXTState = WIZ1;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //Selected file is current file, return to editor
          else {
            CurrentTXTState = TXT_;
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
            display.fillScreen(GxEPD_WHITE);
          }

        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ1:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = WIZ0;
          CurrentKBState = FUNC;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          display.fillScreen(GxEPD_WHITE);
        }
        else {
          int numSelect = (inchar == '0') ? 10 : (inchar - '0');
          //YES (save current file)
          if (numSelect == 1) {
            Serial.println("YES (save current file)");
            //File to be saved does not exist
            if (prevEditingFile == "" || prevEditingFile == "-") {
              CurrentTXTState = WIZ2;
              currentWord = "";
              CurrentKBState = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //File to be saved exists
            else {
              //Save current file
              keypad.disableInterrupts();
              oledWord("Saving File");
              writeFile(SPIFFS, prevEditingFile.c_str(), allText.c_str());
              oledWord("Saved");
              delay(200);
              //Load new file
              oledWord("Loading File");
              allText = readFileToString(SPIFFS, editingFile.c_str());
              keypad.enableInterrupts();
              //Return to TXT
              CurrentTXTState = TXT_;
              CurrentKBState = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //NO  (don't save current file)
          else if (numSelect == 2) {
            Serial.println("NO  (don't save current file)");
            //Just load new file
            keypad.disableInterrupts();
            oledWord("Loading File");
            allText = readFileToString(SPIFFS, ("/" + editingFile).c_str());
            keypad.enableInterrupts();
            //Return to TXT
            CurrentTXTState = TXT_;
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
            display.fillScreen(GxEPD_WHITE);
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;

      case WIZ2:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          prevEditingFile = "/" + currentWord + ".txt";

          //Save the file
          keypad.disableInterrupts();
          oledWord("Saving File");
          writeFile(SPIFFS, prevEditingFile.c_str(), allText.c_str());
          oledWord("Saved");
          delay(200);
          //Load new file

          keypad.enableInterrupts();

          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ3:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          editingFile = "/" + currentWord + ".txt";

          //Save the file
          keypad.disableInterrupts();
          oledWord("Saving " + editingFile);
          writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
          oledWord("Saved " + editingFile);
          delay(200);
          keypad.enableInterrupts();
          //Ask to save prev file
          
          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;

    }
    KBBounceMillis = currentMillis;
  }*/
}

void einkHandler_TXT() {
  /*if ((prevAllText != allText) || newState) {
    newState = false;
    switch (CurrentTXTState) {
      case TXT_:
        prevAllText = allText;
        einkTextPartial(allText);

        statusBar("C:" + String(countVisibleChars(allText)) + ",L:" + String(countLines(allText)) + "," + editingFile);
        
        refresh();
        break;
      case WIZ0:
        prevAllText = allText;
        einkRefresh = FULL_REFRESH_AFTER + 1;
        display.setFullWindow();
        einkTextPartial(allText, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[0],200,218, GxEPD_BLACK);

        keypad.disableInterrupts();
        listDir(SD_MMC, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < MAX_FILES; i++) {
          display.setCursor(88, 54+(17*i));
          display.print(filesList[i]);
        }

        display.nextPage();
        display.hibernate();
        CurrentKBState = FUNC;
        break;
      case WIZ1:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[1],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = FUNC;
        break;
      case WIZ2:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[2],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = NORMAL;
        break;
      case WIZ3:
        prevAllText = allText;
        einkRefresh = FULL_REFRESH_AFTER + 1;
        display.setFullWindow();
        einkTextPartial(allText, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[3],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = NORMAL;
        break;
    }
  }*/
}

// NEW MAINS
void processKB_TXT_NEW() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }

  disableTimeout = false;

  int currentMillis = millis();
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = updateKeypress();
    switch (CurrentTXTState) {
      case TXT_:
        // SET MAXIMUMS AND FONT
        setTXTFont(currentFont);

        // UPDATE SCROLLBAR
        updateScrollFromTouch();

        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        else if (inchar == 12) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }
        //TAB Recieved
        else if (inchar == 9) {                                  
          currentLine += "    ";
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
        //CR Recieved
        else if (inchar == 13) {                          
          allLines.push_back(currentLine);
          currentLine = "";
          newLineAdded = true;
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          allLines.clear();
          currentLine = "";
          oledWord("Clearing...");
          doFull = true;
          newLineAdded = true;
          delay(300);
        }
        // LEFT
        else if (inchar == 19) {                                  
          
        }
        // RIGHT
        else if (inchar == 21) {                                  
          
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        //SAVE Recieved
        else if (inchar == 6) {
          //File exists, save normally
          if (editingFile != "" && editingFile != "-") {
            saveFile();
            CurrentKBState = NORMAL;
            newLineAdded = true;
          }
          //File does not exist, make a new one
          else {
            CurrentTXTState = WIZ3;
            currentLine = "";
            CurrentKBState = NORMAL;
            doFull = true;
            newState = true;
          }
        }
        //LOAD Recieved
        else if (inchar == 5) {
          loadFile();
          CurrentKBState = NORMAL;
          newLineAdded = true;
        }
        //FILE Recieved
        else if (inchar == 7) {
          CurrentTXTState = WIZ0;
          CurrentKBState = NORMAL;
          newState = true;
        }
        // Font Switcher 
        else if (inchar == 14) {                                  
          CurrentTXTState = FONT;
          CurrentKBState = FUNC;
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
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/60)) {
          OLEDFPSMillis = currentMillis;
          // ONLY SHOW OLEDLINE WHEN NOT IN SCROLL MODE
          if (lastTouch == -1) {
            oledLine(currentLine);
            if (prev_dynamicScroll != dynamicScroll) prev_dynamicScroll = dynamicScroll;
          }
          else oledScroll();
        }

        if (currentLine.length() > 0) {
          int16_t x1, y1;
          uint16_t charWidth, charHeight;
          display.getTextBounds(currentLine, 0, 0, &x1, &y1, &charWidth, &charHeight);

          if (charWidth >= display.width()-5) {
            // If currentLine ends with a space, just start a new line
            if (currentLine.endsWith(" ")) {
              allLines.push_back(currentLine);
              currentLine = "";
            }
            // If currentLine ends with a letter, we are in the middle of a word
            else {
              int lastSpace = currentLine.lastIndexOf(' ');
              String partialWord;

              if (lastSpace != -1) {
                partialWord = currentLine.substring(lastSpace + 1);
                currentLine = currentLine.substring(0, lastSpace);  // Strip partial word
                allLines.push_back(currentLine);
                currentLine = partialWord;  // Start new line with the partial word
              } 
              // No spaces found, whole line is a single word
              else {
                allLines.push_back(currentLine);
                currentLine = "";
              }
            }
            newLineAdded = true;
          }
        }

        break;
      case WIZ0:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newLineAdded = true;
          currentWord = "";
          currentLine = "";
          display.fillScreen(GxEPD_WHITE);
        }
        else if (inchar >= '0' && inchar <= '9'){
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          //Edit a new file
          if (filesList[fileIndex - 1] != editingFile) {
            //Selected file does not exist, create a new one
            if (filesList[fileIndex - 1] == "-") {
              CurrentTXTState = WIZ3;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //Selected file exists, prompt to save current file
            else {      
              prevEditingFile = editingFile;
              editingFile = filesList[fileIndex - 1];      
              CurrentTXTState = WIZ1;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //Selected file is current file, return to editor
          else {
            CurrentKBState = NORMAL;
            CurrentTXTState = TXT_;
            newLineAdded = true;
            currentWord = "";
            currentLine = "";
            display.fillScreen(GxEPD_WHITE);
          }

        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        break;
      case WIZ1:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = WIZ0;
          CurrentKBState = FUNC;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          display.fillScreen(GxEPD_WHITE);
        }
        else if (inchar >= '0' && inchar <= '9'){
          int numSelect = (inchar == '0') ? 10 : (inchar - '0');
          //YES (save current file)
          if (numSelect == 1) {
            Serial.println("YES (save current file)");
            //File to be saved does not exist
            if (prevEditingFile == "" || prevEditingFile == "-") {
              CurrentTXTState = WIZ2;
              currentWord = "";
              CurrentKBState = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //File to be saved exists
            else {
              //Save current file
              saveFile();

              delay(200);
              //Load new file
              loadFile();
              //Return to TXT
              CurrentTXTState = TXT_;
              CurrentKBState = NORMAL;
              newLineAdded = true;
              currentWord = "";
              currentLine = "";
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //NO  (don't save current file)
          else if (numSelect == 2) {
            Serial.println("NO  (don't save current file)");
            //Just load new file
            loadFile();
            //Return to TXT
            CurrentTXTState = TXT_;
            CurrentKBState = NORMAL;
            newLineAdded = true;
            currentWord = "";
            currentLine = "";
            display.fillScreen(GxEPD_WHITE);
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        break;

      case WIZ2:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          prevEditingFile = "/" + currentWord + ".txt";

          //Save the file
          saveFile();

          delay(200);
          //Load new file
          loadFile();

          keypad.enableInterrupts();

          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newLineAdded = true;
          currentWord = "";
          currentLine = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        break;
      case WIZ3:
        //No char recieved
        if (inchar == 0);                                         
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
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          editingFile = "/" + currentWord + ".txt";

          //Save the file
          saveFile();
          //Ask to save prev file
          
          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newLineAdded = true;
          currentWord = "";
          currentLine = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        break;
      case FONT:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newLineAdded = true;
          currentWord = "";
          currentLine = "";
          display.fillScreen(GxEPD_WHITE);
        }
        else if (inchar >= '0' && inchar <= '9') {
          int fontIndex = (inchar == '0') ? 10 : (inchar - '0');
          switch (fontIndex) {
            case 1:
              currentFont = &FreeMonoBold9pt7b;
              break;
            case 2:
              currentFont = &FreeSans9pt7b;
              break;
            case 3:
              currentFont = &FreeSerif9pt7b;
              break;
            case 4:
              currentFont = &FreeSerifBold9pt7b;
              break;
            case 5:
              currentFont = &FreeMono12pt7b;
              break;
            case 6:
              currentFont = &FreeSans12pt7b;
              break;
            case 7:
              currentFont = &FreeSerif12pt7b;
              break;
            default:
              currentFont = &FreeMonoBold9pt7b;
              break;
          }
          // SET THE FONT
          setTXTFont(currentFont);

          // UPDATE THE ARRAY TO MATCH NEW FONT SIZE
          String fullTextStr = vectorToString();
          stringToVector(fullTextStr);

          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newLineAdded = true;
          currentWord = "";
          currentLine = "";
          display.fillScreen(GxEPD_WHITE);
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        break;

    }
    KBBounceMillis = currentMillis;
  }
}

void einkHandler_TXT_NEW() {
  if (newLineAdded || newState) {
    switch (CurrentTXTState) {
      case TXT_:
        if (newState && doFull) {
          display.fillScreen(GxEPD_WHITE);
          refresh();
        }
        if (newLineAdded && !newState) {
          einkTextDynamic(true);
          refresh();
        }
        break;
      case WIZ0:
        display.setFullWindow();
        einkTextDynamic(true, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[0],200,218, GxEPD_BLACK);

        keypad.disableInterrupts();
        listDir(SD_MMC, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < MAX_FILES; i++) {
          display.setCursor(88, 54+(17*i));
          display.print(filesList[i]);
        }

        refresh();
        CurrentKBState = FUNC;
        break;
      case WIZ1:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[1],200,218, GxEPD_BLACK);

        refresh();
        CurrentKBState = FUNC;
        break;
      case WIZ2:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[2],200,218, GxEPD_BLACK);

        refresh();
        CurrentKBState = NORMAL;
        break;
      case WIZ3:
        display.setFullWindow();
        einkTextDynamic(true, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[3],200,218, GxEPD_BLACK);

        refresh();
        CurrentKBState = NORMAL;
        break;
      case FONT:
        display.setFullWindow();
        einkTextDynamic(true, true);      
        
        drawStatusBar("Select a Font (0-9)");

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fontfont0,200,218, GxEPD_BLACK);

        keypad.disableInterrupts();
        listDir(SD_MMC, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < 7; i++) {
          display.setCursor(88, 54+(17*i));
          switch (i) {
            case 0:
              display.setFont(&FreeMonoBold9pt7b);
              break;
            case 1:
              display.setFont(&FreeSans9pt7b);
              break;
            case 2:
              display.setFont(&FreeSerif9pt7b);
              break;
            case 3:
              display.setFont(&FreeSerifBold9pt7b);
              break;
            case 4:
              display.setFont(&FreeMono12pt7b);
              break;
            case 5:
              display.setFont(&FreeSans12pt7b);
              break;
            case 6:
              display.setFont(&FreeSerif12pt7b);
              break;
          }
          display.print("Font Number " + String(i+1));
        }

        refresh();
        CurrentKBState = FUNC;
        break;
    
    }
    newState = false;
    newLineAdded = false;
  }
}

bool splitIntoLines(const char* input, int scroll_) {
  size_t    maxLineLength = 29;                 //Chars per line
  size_t    maxLines      = 100;               //Total lines
  size_t    inputLength   = strlen(input);
  uint8_t   charCounter   = 0;
  uint16_t  lineCounter   = 0;

  //Declare a large local array
  String fullLines[maxLines];

  //Clear fullLines
  for (int i = 0; i < maxLines; i++) {
    fullLines[i] = "";
  }

  //Increment through input one char at a time
  for (size_t c = 0; c < inputLength && lineCounter < maxLines; c++) { 
    if (input[c] == '\n') {
      charCounter = 0;
      lineCounter++;
      continue;
    }
    else if (charCounter > (maxLineLength-1)) {
      charCounter = 0;
      lineCounter++;
    }
    fullLines[lineCounter] += input[c];
    charCounter++;
  }

  for (int i = 0; i < 13; i++) {
    outLines[i] = "";
  }

  /*for (int i = 0; i < maxLines; i++) {
    printf(("fullLines " + String(i) + ": " + fullLines[i] + "\n").c_str());
  }*/

  int lineCounter_ = 0;
  if (lineCounter < 13) {
    scroll_ = 0;
    lineCounter_ = 13;
  }
  else lineCounter_ = lineCounter;
  
  uint8_t j = 0;
  for (uint16_t i = (lineCounter_-13-scroll_); j < 13; i++, j++) { //i: fullLines index, j:outLines index
    if (i >= 0 && i < maxLines) {
      outLines[j] = fullLines[i];  // Copy lines from fullLines to outLines
      //printf(("Line " + String(j) + ": " + outLines[j] + "\n").c_str());
    }
  }

  static String prevTopLine = "";  // Static to keep track of the previous top line
  String topLine = outLines[0];

  if (topLine != prevTopLine && lineCounter > 12) {
    // If the top line has changed, it's a new line
    einkRefresh = FULL_REFRESH_AFTER + 1;
    newState = true;
    prevTopLine = topLine;  // Update the previous top line
    return true;
  }
  else return false;
}

int countWords(String str) {
  int count = 0;
  bool inWord = false;

  // Loop through each character in the String
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);

    // If the character is a space, mark the end of a word
    if (c == ' ') {
      if (inWord) {
        count++;  // We've encountered the end of a word
        inWord = false;
      }
    } 
    else {
      // If the character is not a space, we're in a word
      inWord = true;
    }
  }

  // If the last character is not a space, we have one more word
  if (inWord) {
      count++;
  }

  return count;
}

int countVisibleChars(String input) {
  int count = 0;

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    // Check if the character is a visible character or space
    if (c >= 32 && c <= 126) { // ASCII range for printable characters and space
      count++;
    }
  }

  return count;
}

void updateScrollFromTouch() {
  uint16_t touched = cap.touched();  // Read touch state
  int newTouch = -1;

  // Find the first active touch point (lowest index first)
  for (int i = 0; i < 9; i++) {
    if (touched & (1 << i)) {
      newTouch = i;
      Serial.print("Prev pad: ");
      Serial.print(lastTouch);
      Serial.print("   Touched pad: ");
      Serial.println(newTouch);
      break;
    }
  }

  unsigned long currentTime = millis();

  if (newTouch != -1) {  // If a touch is detected
    Serial.println("Touch Detected");
    if (lastTouch != -1) {  // Compare with previous touch
      int touchDelta = abs(newTouch - lastTouch);
      if (touchDelta <= 2) {  // Ignore large jumps (adjust threshold if needed)
        int maxScroll = max(0, (int)allLines.size() - maxLines);  // Ensure a valid scroll range
        if (newTouch > lastTouch) {
          dynamicScroll = min((int)(dynamicScroll + 1), maxScroll);
        } else if (newTouch < lastTouch) {
          dynamicScroll = max((int)(dynamicScroll - 1), 0);
        }
      }
    }
    lastTouch = newTouch;  // Always update lastTouch
    lastTouchTime = currentTime;  // Reset timeout timer
  } 
  else if (lastTouch != -1 && (currentTime - lastTouchTime > TOUCH_TIMEOUT_MS)) {
    // RESET LASTTOUCH AFTER TIMEOUT
    lastTouch = -1;
    // ONLY UPDATE IF SCROLL HAS CHANGED
    if (prev_dynamicScroll != dynamicScroll) newLineAdded = true;
  }
}