//  ooooo   ooooo   .oooooo.   ooo        ooooo oooooooooooo  //
//  `888'   `888'  d8P'  `Y8b  `88.       .888' `888'     `8  //
//   888     888  888      888  888b     d'888   888          //
//   888ooooo888  888      888  8 Y88. .P  888   888oooo8     //
//   888     888  888      888  8  `888'   888   888    "     //
//   888     888  `88b    d88'  8    Y     888   888       o  //
//  o888o   o888o  `Y8bood8P'  o8o        o888o o888ooooood8  //

void commandSelect(String command) {
  command.toLowerCase();

  if (command == "home") {
    oledWord("You're home, silly!");
    delay(1000);
  } 
  /////////////////////////////
  else if (command == "note" || command == "text" || command == "write" || command == "notebook" || command == "notepad" || command == "txt") {
    CurrentAppState = TXT;
    CurrentKBState  = NORMAL;
    newLineAdded = true;
  }
  /////////////////////////////
  else if (command == "file wizard" || command == "wiz" || command == "file wiz" || command == "file") {
    // OPEN FILE WIZ
  }
  /////////////////////////////
  else if (command == "back up" || command == "export" || command == "transfer" || command == "usb transfer" || command == "usb") {
    // OPEN USB FILE TRANSFER
  }
  /////////////////////////////
  else if (command == "bluetooth" || command == "bt") {
    // OPEN BLUETOOTH
  }
  /////////////////////////////
  else if (command == "preferences" || command == "setting" || command == "settings") {
    // OPEN SETTINGS
  }
  /////////////////////////////
  else if (command == "i farted") {
    oledWord("That smells");
    delay(1000);
  } 
  else if (command == "poop") {
    oledWord("Yuck");
    delay(1000);
  } 
  else if (command == "hello") {
    oledWord("Hey, you!");
    delay(1000);
  } 
  else if (command == "hi") {
    oledWord("What's up?");
    delay(1000);
  } 
  else if (command == "i love you") {
    oledWord("luv u 2 <3");
    delay(1000);
  } 
  else if (command == "what can you do") {
    oledWord("idk man");
    delay(1000);
  } 
  else if (command == "alexa") {
    oledWord("...");
    delay(1000);
  } 
  else {
    oledWord("Huh?");
    delay(1000);
  }
}

void processKB_HOME() {
  int currentMillis = millis();

  switch (CurrentHOMEState) {
    case HOME_HOME:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          commandSelect(currentLine);
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
        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentLine, false);
        }
      }
      /*disableTimeout = false;
      if (OLEDPowerSave) {
        u8g2.setPowerSave(0);
        OLEDPowerSave = false;
      }
      CurrentKBState = FUNC;
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {}
        else {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          //Edit a new file
          switch (fileIndex) {
            case 1: //TXT
              CurrentAppState = TXT;
              CurrentKBState  = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              //newState = true;
              newLineAdded = true;
              break;
            case 2: //FILE WIZARD
              CurrentAppState = FILEWIZ;
              CurrentKBState  = FUNC;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              break;
            case 3: //USB TRANSFER
              CurrentAppState = USB;
              break;
            case 4: //BLUETOOTH TRANSFER
              CurrentAppState = BT;
              break;
            case 5: //SETTINGS
              CurrentAppState = SETTINGS;
              break;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        KBBounceMillis = currentMillis;
      }*/
      break;

    case NOWLATER:
      DateTime now = rtc.now();
      if (prevTime != now.minute()) {
        prevTime = now.minute();
        newState = true;
      }
      else newState = false;
      break;
  }
}

void einkHandler_HOME() {
  switch (CurrentHOMEState) {
    case HOME_HOME:
      if (newState) {
        display.setRotation(3);
        display.fillScreen(GxEPD_WHITE);

        for (int i = 0; i < 5; i++) {
          display.drawBitmap(20 + (60*i), 20, homeIconsAllArray[i], 40, 40, GxEPD_BLACK);
        }

        drawStatusBar(" Type What You Want To Do");

        refresh();
        newState = false;
      }
      break;

    case NOWLATER:
      if (newState) {
        newState = false;

        // BACKGROUND
        display.drawBitmap(0, 0, nowLaterallArray[0], 320, 240, GxEPD_BLACK);

        // CLOCK HANDS
        float pi = 3.14159;

        float hourLength    = 25;
        float minuteLength  = 40;
        uint8_t hourWidth   = 5;
        uint8_t minuteWidth = 2;

        uint8_t centerX     = 76;
        uint8_t centerY     = 94;

        DateTime now = rtc.now();

        // Convert time to proper angles in radians
        float minuteAngle = (now.minute() / 60.0) * 2 * pi;  
        float hourAngle   = ((now.hour() % 12) / 12.0 + (now.minute() / 60.0) / 12.0) * 2 * pi;

        // Convert angles to coordinates
        uint8_t minuteX = (minuteLength * cos(minuteAngle - pi/2)) + centerX;
        uint8_t minuteY = (minuteLength * sin(minuteAngle - pi/2)) + centerY;
        uint8_t hourX   = (hourLength   * cos(hourAngle   - pi/2)) + centerX;
        uint8_t hourY   = (hourLength   * sin(hourAngle   - pi/2)) + centerY;

        drawThickLine(centerX, centerY, minuteX, minuteY, minuteWidth);
        drawThickLine(centerX, centerY, hourX  , hourY  , hourWidth);

        // WEATHER

        // TASKS/CALENDAR

        refresh();
      }
      break;
  }
  
}