//  ooooo   ooooo   .oooooo.   ooo        ooooo oooooooooooo  //
//  `888'   `888'  d8P'  `Y8b  `88.       .888' `888'     `8  //
//   888     888  888      888  888b     d'888   888          //
//   888ooooo888  888      888  8 Y88. .P  888   888oooo8     //
//   888     888  888      888  8  `888'   888   888    "     //
//   888     888  `88b    d88'  8    Y     888   888       o  //
//  o888o   o888o  `Y8bood8P'  o8o        o888o o888ooooood8  //
#include "globals.h"
#ifdef DESKTOP_EMULATOR
#include "U8g2lib.h"
#endif

// Forward declarations
void PERIODIC_INIT();

void commandSelect(String command) {
  std::cout << "[POCKETMAGE] commandSelect() called with: '" << command.c_str() << "'" << std::endl;
  command.toLowerCase();
  std::cout << "[POCKETMAGE] After toLowerCase(): '" << command.c_str() << "'" << std::endl;

  // OPEN IN FILE WIZARD
  if (command.startsWith("-")) {
    command = removeChar(command, ' ');
    command = removeChar(command, '-');
    keypad.disableInterrupts();
    listDir(SD_MMC, "/");
    keypad.enableInterrupts();

    for (uint8_t i = 0; i < (sizeof(filesList) / sizeof(filesList[0])); i++) {
      String lowerFileName = filesList[i]; 
      lowerFileName.toLowerCase();
      if (command == lowerFileName || (command+".txt") == lowerFileName || ("/"+command+".txt") == lowerFileName) {
        workingFile = filesList[i];
        CurrentAppState = FILEWIZ;
        CurrentFileWizState = WIZ1_;
        CurrentKBState  = FUNC;
        newState = true;
        return;
      }
    }
  }

  // OPEN IN TXT EDITOR
  if (command.startsWith("/")) {
    command = removeChar(command, ' ');
    command = removeChar(command, '/');
    keypad.disableInterrupts();
    listDir(SD_MMC, "/");
    keypad.enableInterrupts();

    for (uint8_t i = 0; i < (sizeof(filesList) / sizeof(filesList[0])); i++) {
      String lowerFileName = filesList[i]; 
      lowerFileName.toLowerCase();
      if (command == lowerFileName || (command+".txt") == lowerFileName || ("/"+command+".txt") == lowerFileName) {
        editingFile = filesList[i];
        loadFile();
        CurrentAppState = TXT;
        CurrentTXTState = TXT_;
        CurrentKBState  = NORMAL;
        newLineAdded = true;
        return;
      }
    }
  }

  // Dice Roll
  if (command.startsWith("roll d")) {
    String numStr = command.substring(6);
    int sides = numStr.toInt();
    if (sides < 1) {
      oledWord("Please enter a valid number");
      delay(2000);
    } 
    else if (sides == 1) {
      oledWord("D1: you rolled a 1, duh!");
      delay(2000);
    }
    else {
      int roll = (esp_random() % sides) + 1;
      if (roll == sides)  oledWord("D" + String(sides) + ": " + String(roll) + "!!!");
      else if (roll == 1) oledWord("D" + String(sides) + ": " + String(roll) + " :(");
      else                oledWord("D" + String(sides) + ": " + String(roll));
      delay(3000);
      CurrentKBState = NORMAL;
    }
  }

  else if (command == "home") {
    oledWord("You're home, silly!");
    delay(1000);
  } 
  /////////////////////////////
  else if (command == "note" || command == "text" || command == "write" || command == "notebook" || command == "notepad" || command == "txt" || command == "1") {
    std::cout << "[POCKETMAGE] Matched txt command! Calling TXT_INIT()" << std::endl;
    TXT_INIT();
  }
  /////////////////////////////
  else if (command == "file wizard" || command == "wiz" || command == "file wiz" || command == "file" || command == "filewiz" || command == "2") {
    std::cout << "[POCKETMAGE] Matched filewiz command! Calling FILEWIZ_INIT()" << std::endl;
    FILEWIZ_INIT();
  }
  /////////////////////////////
  else if (command == "back up" || command == "export" || command == "transfer" || command == "usb transfer" || command == "usb" || command == "3") {
    USB_INIT();
  }
  /////////////////////////////
  else if (command == "tasks" || command == "task" || command == "6") {
    std::cout << "[POCKETMAGE] Matched tasks command! Calling TASKS_INIT()" << std::endl;
    TASKS_INIT();
  }
  /////////////////////////////
  else if (command == "bluetooth" || command == "bt" || command == "4") {
    // OPEN BLUETOOTH
  }
  /////////////////////////////
  else if (command == "preferences" || command == "setting" || command == "settings" || command == "5") {
    std::cout << "[POCKETMAGE] Matched settings command! Calling SETTINGS_INIT()" << std::endl;
    SETTINGS_INIT();
  }
  else if (command == "cal" || command == "calendar" || command == "7") {
    CALENDAR_INIT();
  }
  else if (command == "lex" || command == "lexicon" || command == "dict" || command == "dictionary" || command == "9") {
    LEXICON_INIT();
  }
  else if (command == "journ" || command == "journal" || command == "daily" || command == "8") {
    JOURNAL_INIT();
  }
  /////////////////////////////
  else if (command == "pokedex" || command == "pokemon" || command == "poke" || command == "10") {
    std::cout << "[POCKETMAGE] Matched pokedex command! Calling POKEDEX_INIT()" << std::endl;
    POKEDEX_INIT();
  }
  /////////////////////////////
  else if (command == "periodic" || command == "elements" || command == "table" || command == "11") {
    std::cout << "[POCKETMAGE] Matched periodic command! Calling PERIODIC_INIT()" << std::endl;
    PERIODIC_INIT();
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
    settingCommandSelect(command);
  }
}

void processKB_HOME() {
  int currentMillis = millis();

  switch (CurrentHOMEState) {
    case HOME_HOME:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        KeyEvent keyEvent = updateKeypressUTF8();
        bool hasInput = keyEvent.hasEvent;
        
        if (hasInput) {
          std::cout << "[POCKETMAGE] processKB_HOME received UTF-8 input: action=" << keyEvent.action << " text='" << keyEvent.text.c_str() << "'" << std::endl;
        }
        
        // HANDLE INPUTS
        //No input received
        if (!hasInput);   
        //CR Received
        else if (keyEvent.action == KA_ENTER) {                          
          std::cout << "[POCKETMAGE] Enter pressed! Executing command: '" << currentLine.c_str() << "'" << std::endl;
          commandSelect(currentLine);
          currentLine = "";
        }                                      
        //SHIFT and FN are handled automatically by updateKeypressUTF8()
        //Space Received
        else if (keyEvent.action == KA_SPACE) {                                  
          currentLine += " ";
        }
        // Home/ESC received
        else if (keyEvent.action == KA_HOME || keyEvent.action == KA_ESC) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }
        //CLEAR Received
        else if (keyEvent.action == KA_CLEAR) {                                  
          currentLine = "";
        }
        //BKSP Received
        else if (keyEvent.action == KA_BACKSPACE) {                  
          if (currentLine.length() > 0) {
            currentLine = utf8SafeBackspace(currentLine);
          }
        }
        // Cycle keyboard layout (Fn+K)
        else if (keyEvent.action == KA_CYCLE_LAYOUT) {
          cycleKeyboardLayout();
        }
        // Handle dead key input
        else if (keyEvent.action == KA_DEAD && keyEvent.text.length() > 0) {
          CurrentDead = keyEvent.text;
          std::cout << "[POCKETMAGE] Dead key activated: '" << keyEvent.text.c_str() << "'" << std::endl;
        }
        // Handle UTF-8 character input
        else if (keyEvent.action == KA_CHAR && keyEvent.text.length() > 0) {
          String composedText = composeDeadIfAny(keyEvent.text);
          currentLine += composedText;
          std::cout << "[POCKETMAGE] Added UTF-8 text to command: '" << composedText.c_str() << "', currentLine now: '" << currentLine.c_str() << "'" << std::endl;
          // Reset modifier states after character input (except for numbers in FN mode)
          if (CurrentKBState == FUNC) {
            // Check if the input contains digits to keep FN mode active
            bool hasDigit = false;
            for (int i = 0; i < keyEvent.text.length(); i++) {
              if (keyEvent.text[i] >= '0' && keyEvent.text[i] <= '9') {
                hasDigit = true;
                break;
              }
            }
            if (!hasDigit && CurrentKBState != NORMAL) {
              CurrentKBState = NORMAL;
            }
          } else if (CurrentKBState != NORMAL) {
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
        newState = false;
        drawHome();
        refresh();
        //multiPassRefesh(2);
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
        //151,68
        if (!tasks.empty()) {
          if (DEBUG_VERBOSE) Serial.println("Printing Tasks");

          int loopCount = std::min((int)tasks.size(), 7);
          for (int i = 0; i < loopCount; i++) {
            display.setFont(&FreeSerif9pt7b);
            // PRINT TASK NAME
            display.setCursor(151, 68 + (25 * i));
            display.print(tasks[i][0].c_str());
          }
        }

        forceSlowFullUpdate = true;
        refresh();
      }
      break;
  }
}

void drawHome() {
  display.setRotation(3);
  // Clear screen using the same pattern as PERIODIC app
  display.fillScreen(GxEPD_WHITE);
  refresh();
  delay(10);  // Ensure Metal command buffer completion
  
  int16_t x1, y1;
  uint16_t charWidth, charHeight;
  uint8_t appsPerRow = 5; // Number of apps per row
  uint8_t spacingX = 60;  // Horizontal spacing
  uint8_t spacingY = 75;  // Vertical spacing (increased for better layout)
  uint8_t iconSize = 40;  // Icon width and height
  uint8_t startX = 20;    // Initial X position
  uint8_t startY = 20;    // Initial Y position

  display.setFont(&FreeSerif9pt7b);
  for (int i = 0; i < sizeof(appIcons) / sizeof(appIcons[0]); i++) {
    int row = i / appsPerRow;
    int col = i % appsPerRow;
    
    int xPos = startX + (spacingX * col);
    int yPos = startY + (spacingY * row);

    display.drawBitmap(xPos, yPos, appIcons[i], iconSize, iconSize, GxEPD_BLACK);
    display.getTextBounds(appStateNames[i], 0, 0, &x1, &y1, &charWidth, &charHeight);
    display.setCursor(xPos + (iconSize / 2) - (charWidth / 2), yPos + iconSize + 13);
    display.print(appStateNames[i]);
  }
  display.setFont(&FreeMonoBold9pt7b);

  drawStatusBar("Type a Command:");
}