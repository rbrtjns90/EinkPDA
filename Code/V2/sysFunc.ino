//   .oooooo..o oooooo   oooo  .oooooo..o ooooooooooooo oooooooooooo ooo        ooooo  //
//  d8P'    `Y8  `888.   .8'  d8P'    `Y8 8'   888   `8 `888'     `8 `88.       .888'  //
//  Y88bo.        `888. .8'   Y88bo.           888       888          888b     d'888   //
//   `"Y8888o.     `888.8'     `"Y8888o.       888       888oooo8     8 Y88. .P  888   //
//       `"Y88b     `888'          `"Y88b      888       888    "     8  `888'   888   //
//  oo     .d8P      888      oo     .d8P      888       888       o  8    Y     888   //
//  8""88888P'      o888o     8""88888P'      o888o     o888ooooood8 o8o        o888o  //

void saveFile() {
  switch (TXT_APP_STYLE) {
    case 0:
      if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
      keypad.disableInterrupts();
      oledWord("Saving File");
      writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
      oledWord("Saved");
      delay(200);
      keypad.enableInterrupts();
      break;
    case 1:
      
      break;
  }
}

void checkTimeout() {
  timeoutMillis = millis();

  //Trigger timeout deep sleep
  if (!disableTimeout) {
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
          saveFile();
        }

        //Put OLED to sleep
        u8g2.setPowerSave(1);

        //Stop the einkHandler task
        if (einkHandlerTaskHandle != NULL) {
          vTaskDelete(einkHandlerTaskHandle);
          einkHandlerTaskHandle = NULL;
        }

        switch (CurrentAppState) {
          case HOME:
              display.setFullWindow();
              display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
            break;
          case TXT:
            if (SLEEPMODE == "TEXT" && editingFile != "") {
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
              statusBar(editingFile, true);
              
              display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
              display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);
            }
            else {
              //Display sleep image on E-Ink
              display.setFullWindow();
              display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
            }
            break;
        }
        
        display.nextPage();
        display.hibernate();
        
        //Sleep the device
        esp_deep_sleep_start();
      }
  }
  else {
    prevTimeMillis = millis();
  }
  
  if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
    PWR_BTN_event = false;
    
    //Save current work:
    //Only save if alltext has significant content
    if (allText.length() > 10) {
      oledWord("Saving Work");
      saveFile();
    }
    
    if (digitalRead(CHRG_SENS) == HIGH) {
      CurrentAppState = HOME;
      CurrentHOMEState = NOWLATER;
      u8g2.setPowerSave(1);
      OLEDPowerSave  = true;
      disableTimeout = true;
      newState = true;

      display.setFullWindow();
      display.fillScreen(GxEPD_WHITE);
      //display.nextPage();
      //display.hibernate();
      //display.display(true);
      //display.hibernate();
      //delay(500);
    }
    else {
      //Put OLED to sleep
      u8g2.setPowerSave(1);

      //Stop the einkHandler task
      if (einkHandlerTaskHandle != NULL) {
        vTaskDelete(einkHandlerTaskHandle);
        einkHandlerTaskHandle = NULL;
      }

      switch (CurrentAppState) {
        case HOME:
            display.setFullWindow();
            display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
          break;
        case TXT:
          if (SLEEPMODE == "TEXT" && editingFile != "") {
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
            statusBar(editingFile, true);
            
            display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);
          }
          else {
            //Display sleep image on E-Ink
            display.setFullWindow();
            display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
          }
          break;
      }
      
      display.nextPage();
      display.hibernate();
      
      //Sleep the device
      esp_deep_sleep_start();
    }
    
  }
  else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    CurrentAppState = HOME;
    CurrentHOMEState = HOME_HOME;
    PWR_BTN_event = false;
    if (OLEDPowerSave) {
      u8g2.setPowerSave(0);
      OLEDPowerSave = false;
    }
    display.fillScreen(GxEPD_WHITE);
    refresh();
    delay(200);
    newState = true;
  }
}

void TCA8418_irq() {
  TCA8418_event = true;
}

void PWR_BTN_irq() {
  PWR_BTN_event = true;
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

void printDebug() {
  DateTime now = rtc.now();
  if (now.second() != prevSec) {
    prevSec = now.second();
    Serial.print("PWR_BTN: "); Serial.print(digitalRead(PWR_BTN));
    Serial.print(", KB_INT: "); Serial.print(digitalRead(KB_IRQ));
    Serial.print(", CHRG: "); Serial.print(digitalRead(CHRG_SENS));
    Serial.print(", RTC_INT: "); Serial.print(digitalRead(RTC_INT));

    // Read and convert battery voltage
    float batteryVoltage = analogRead(BAT_SENS) * (3.3 / 4095.0) * 2;
    Serial.print(", BAT: "); Serial.print(batteryVoltage, 2); // Print with 2 decimal places

    Serial.print(", SYS_TIME: ");
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    
    Serial.println();
  }
}

//   .oooooo..o ooooooooo.   ooooo oooooooooooo oooooooooooo  .oooooo..o  //
//  d8P'    `Y8 `888   `Y88. `888' `888'     `8 `888'     `8 d8P'    `Y8  //
//  Y88bo.       888   .d88'  888   888          888         Y88bo.       //
//   `"Y8888o.   888ooo88P'   888   888oooo8     888oooo8     `"Y8888o.   //
//       `"Y88b  888          888   888    "     888    "         `"Y88b  //
//  oo     .d8P  888          888   888          888         oo     .d8P  //
//  8""88888P'  o888o        o888o o888o        o888o        8""88888P'   //

void listDir(fs::FS &fs, const char *dirname) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  // Reset fileIndex and initialize filesList with "-"
  fileIndex = 0; // Reset fileIndex
  for (int i = 0; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }

  File file = root.openNextFile();
  while (file && fileIndex < MAX_FILES) {
    if (!file.isDirectory()) {
      filesList[fileIndex++] = String(file.name()); // Store file name
    }
    file = root.openNextFile();
  }

  for (int i = 0; i < MAX_FILES; i++) {
    Serial.println(filesList[i]);
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

String readFileToString(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    oledWord("Load Failed");
    delay(500);
    return "";  // Return an empty string on failure
  }

  Serial.println("- reading from file:");
  String content = "";  // Initialize an empty String to hold the content

  while (file.available()) {
    content += (char)file.read();  // Read each character and append to the String
  }

  file.close();
  oledWord("File Loaded");
  delay(200);
  einkRefresh = FULL_REFRESH_AFTER; //Force a full refresh
  return content;  // Return the complete String
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  delay(200);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}