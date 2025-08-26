//  oooooooooooo ooooo ooooo        oooooooooooo oooooo   oooooo     oooo ooooo  oooooooooooo  //
//  `888'     `8 `888' `888'        `888'     `8  `888.    `888.     .8'  `888' d'""""""d888'  //
//   888          888   888          888           `888.   .8888.   .8'    888        .888P    //
//   888oooo8     888   888          888oooo8       `888  .8'`888. .8'     888       d888'     //
//   888    "     888   888          888    "        `888.8'  `888.8'      888     .888P       //
//   888          888   888       o  888       o      `888'    `888'       888    d888'    .P  //
//  o888o        o888o o888ooooood8 o888ooooood8       `8'      `8'       o888o .8888888888P   //
#include "globals.h"
#ifdef DESKTOP_EMULATOR
#include "U8g2lib.h"
#endif

void FILEWIZ_INIT() {
  CurrentAppState = FILEWIZ;
  CurrentKBState  = NORMAL;
  forceSlowFullUpdate = true;
  newState = true;
}

void processKB_FILEWIZ() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }
  int currentMillis = millis();

  switch (CurrentFileWizState) {
    case WIZ0_:
      disableTimeout = false;

      CurrentKBState = FUNC;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12 || inchar == 27) {
          CurrentAppState = HOME;
          currentLine     = "";
          CurrentKBState  = NORMAL;
          CurrentHOMEState = HOME_HOME;
          newState = true;
          break;
        }
        else if (inchar >= '0' && inchar <= '9') {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          // SET WORKING FILE
          String selectedFile = filesList[fileIndex - 1];
          if (selectedFile != "-" && selectedFile != "") {
            workingFile = selectedFile;
            // GO TO WIZ1_
            CurrentFileWizState = WIZ1_;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        KBBounceMillis = currentMillis;
      }
      break;

    case WIZ1_:
      disableTimeout = false;

      CurrentKBState = FUNC;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12 || inchar == 27) {
          CurrentFileWizState = WIZ0_;
          newState = true;
          break;
        }
        else if (inchar >= '1' && inchar <= '4') {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          // SELECT OPTION
          switch (fileIndex) {
            case 1: // RENAME
              CurrentFileWizState = WIZ2_R;
              newState = true;
              break;
            case 2: //DELETE
              CurrentFileWizState = WIZ1_YN;
              newState = true;
              break;
            case 3: // COPY
              CurrentFileWizState = WIZ2_C;
              newState = true;
              break;
            case 4: // ELABORATE
              break;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case WIZ1_YN:
      disableTimeout = false;

      CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12 || inchar == 27) {
          CurrentFileWizState = WIZ1_;
          newState = true;
          break;
        }
        // Y RECIEVED
        else if (inchar == 'y' || inchar == 'Y') {
          // DELETE FILE
          delFile(workingFile);
          
          // RETURN TO FILE WIZ HOME
          CurrentFileWizState = WIZ0_;
          newState = true;
          break;
        }
        // N RECIEVED
        else if (inchar == 'n' || inchar == 'N') {
          // GO BACK
          CurrentFileWizState = WIZ1_;
          newState = true;
          break;
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentWord, false);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case WIZ2_R:
      disableTimeout = false;

      //CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
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
        else if (inchar == 12) {
          CurrentFileWizState = WIZ1_;
          CurrentKBState = NORMAL;
          currentWord = "";
          currentLine = "";
          newState = true;
          break;
        }
        //ENTER Recieved
        else if (inchar == 13) {      
          // RENAME FILE                    
          String newName = "/" + currentWord + ".txt";
          renFile(workingFile, newName);

          // RETURN TO WIZ0
          CurrentFileWizState = WIZ0_;
          CurrentKBState = NORMAL;
          newState = true;
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
      }
      break;
    case WIZ2_C:
      disableTimeout = false;

      //CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
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
        else if (inchar == 12) {
          CurrentFileWizState = WIZ1_;
          CurrentKBState = NORMAL;
          currentWord = "";
          currentLine = "";
          newState = true;
          break;
        }
        //ENTER Recieved
        else if (inchar == 13) {      
          // RENAME FILE                    
          String newName = "/" + currentWord + ".txt";
          copyFile(workingFile, newName);

          // RETURN TO WIZ0
          CurrentFileWizState = WIZ0_;
          CurrentKBState = NORMAL;
          newState = true;
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
      }
      break;
  
  }
}

void einkHandler_FILEWIZ() {
  switch (CurrentFileWizState) {
    case WIZ0_:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("Select a File (0-9)");
        display.drawBitmap(0, 0, fileWizardallArray[0], 320, 218, GxEPD_BLACK);

        // DRAW FILE LIST
        keypad.disableInterrupts();
        listDir(SD_MMC, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < MAX_FILES; i++) {
          display.setCursor(30, 54+(17*i));
          display.print(filesList[i]);
        }

        refresh();
      }
      break;
    case WIZ1_:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("- " + workingFile);
        display.drawBitmap(0, 0, fileWizardallArray[1], 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
    case WIZ1_YN:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("DEL:" + workingFile + "?(Y/N)");
        display.drawBitmap(0, 0, fileWizardallArray[1], 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
    case WIZ2_R:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("Enter New Filename:");
        display.drawBitmap(0, 0, fileWizardallArray[2], 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
    case WIZ2_C:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("Enter Name For Copy:");
        display.drawBitmap(0, 0, fileWizardallArray[2], 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
  }
}