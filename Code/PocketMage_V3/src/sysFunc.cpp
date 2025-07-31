//   .oooooo..o oooooo   oooo  .oooooo..o ooooooooooooo oooooooooooo ooo        ooooo  //
//  d8P'    `Y8  `888.   .8'  d8P'    `Y8 8'   888   `8 `888'     `8 `88.       .888'  //
//  Y88bo.        `888. .8'   Y88bo.           888       888          888b     d'888   //
//   `"Y8888o.     `888.8'     `"Y8888o.       888       888oooo8     8 Y88. .P  888   //
//       `"Y88b     `888'          `"Y88b      888       888    "     8  `888'   888   //
//  oo     .d8P      888      oo     .d8P      888       888       o  8    Y     888   //
//  8""88888P'      o888o     8""88888P'      o888o     o888ooooood8 o8o        o888o  //
#include "globals.h"

// High-Level File Operations
void saveFile() {
  if (noSD) {
    oledWord("SAVE FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    String textToSave = vectorToString();
    if (DEBUG_VERBOSE) {
      Serial.println("Text to save:");
      Serial.println(textToSave);
    }
    if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
    keypad.disableInterrupts();
    if (!editingFile.startsWith("/")) editingFile = "/" + editingFile;
    oledWord("Saving File: "+ editingFile);
    writeFile(SD_MMC, (editingFile).c_str(), textToSave.c_str());
    oledWord("Saved: "+ editingFile);

    // Write MetaData
    writeMetadata(editingFile);
    
    delay(1000);
    keypad.enableInterrupts();
    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

void writeMetadata(const String& path) {
  File file = SD_MMC.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("Invalid file for metadata.");
    return;
  }

  // Get file size
  size_t fileSizeBytes = file.size();
  file.close();

  // Format size string
  String fileSizeStr = String(fileSizeBytes) + " Bytes";

  // Get line and char counts
  int charCount = countVisibleChars(readFileToString(SD_MMC, path.c_str()));

  String charStr  = String(charCount) + " Char";

  // Get current time from RTC
  DateTime now = rtc.now();
  char timestamp[20];
  sprintf(timestamp, "%04d%02d%02d-%02d%02d",
          now.year(), now.month(), now.day(), now.hour(), now.minute());

  // Compose new metadata line
  String newEntry = path + "|" + timestamp + "|" + fileSizeStr + "|" + charStr;

  const char* metaPath = SYS_METADATA_FILE;

  // Read existing entries and rebuild the file without duplicates
  File metaFile = SD_MMC.open(metaPath, FILE_READ);
  String updatedMeta = "";
  bool replaced = false;

  if (metaFile) {
    while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (line.startsWith(path + "|")) {
        updatedMeta += newEntry + "\n";
        replaced = true;
      } else if (line.length() > 1) {
        updatedMeta += line + "\n";
      }
    }
    metaFile.close();
  }

  if (!replaced) {
    updatedMeta += newEntry + "\n";
  }

  // Write back the updated metadata
  metaFile = SD_MMC.open(metaPath, FILE_WRITE);
  if (!metaFile) {
    Serial.println("Failed to open metadata file for writing.");
    return;
  }
  metaFile.print(updatedMeta);
  metaFile.close();

  Serial.println("Metadata updated.");
}

void loadFile(bool showOLED) {
  if (noSD) {
    oledWord("LOAD FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    keypad.disableInterrupts();
    if (showOLED) oledWord("Loading File");
    if (!editingFile.startsWith("/")) editingFile = "/" + editingFile;
    String textToLoad = readFileToString(SD_MMC, (editingFile).c_str());
    if (DEBUG_VERBOSE) {
      Serial.println("Text to load:");
      Serial.println(textToLoad);
    }
    stringToVector(textToLoad);
    keypad.enableInterrupts();
    if (showOLED) oledWord("File Loaded");
    delay(200);
    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

void delFile(String fileName) {
  if (noSD) {
    oledWord("DELETE FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    keypad.disableInterrupts();
    oledWord("Deleting File: "+ fileName);
    if (!fileName.startsWith("/")) fileName = "/" + fileName;
    deleteFile(SD_MMC, fileName.c_str());
    oledWord("Deleted: "+ fileName);

    // Delete MetaData
    deleteMetadata(fileName);

    delay(1000);
    keypad.enableInterrupts();
    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

void deleteMetadata(String path) {
  const char* metaPath = SYS_METADATA_FILE;
  

  // Open metadata file for reading
  File metaFile = SD_MMC.open(metaPath, FILE_READ);
  if (!metaFile) {
    Serial.println("Metadata file not found.");
    return;
  }

  // Store lines that don't match the given path
  std::vector<String> keptLines;
  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');
    if (!line.startsWith(path + "|")) {
      keptLines.push_back(line);
    }
  }
  metaFile.close();

  // Delete the original metadata file
  SD_MMC.remove(metaPath);

  // Recreate the file and write back the kept lines
  File writeFile = SD_MMC.open(metaPath, FILE_WRITE);
  if (!writeFile) {
    Serial.println("Failed to recreate metadata file.");
    return;
  }

  for (const String& line : keptLines) {
    writeFile.println(line);
  }

  writeFile.close();
  Serial.println("Metadata entry deleted (if it existed).");
}

void renFile(String oldFile, String newFile) {
  if (noSD) {
    oledWord("RENAME FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    keypad.disableInterrupts();
    oledWord("Renaming "+ oldFile + " to " + newFile);
    if (!oldFile.startsWith("/")) oldFile = "/" + oldFile;
    if (!newFile.startsWith("/")) newFile = "/" + newFile;
    renameFile(SD_MMC, oldFile.c_str(), newFile.c_str());
    oledWord(oldFile + " -> " + newFile);
    delay(1000);

    // Update MetaData
    renMetadata(oldFile, newFile);

    keypad.enableInterrupts();
    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

void renMetadata(String oldPath, String newPath) {
  setCpuFrequencyMhz(240);
  const char* metaPath = SYS_METADATA_FILE;

  // Open metadata file for reading
  File metaFile = SD_MMC.open(metaPath, FILE_READ);
  if (!metaFile) {
    Serial.println("Metadata file not found.");
    return;
  }

  std::vector<String> updatedLines;

  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');
    if (line.startsWith(oldPath + "|")) {
      // Replace old path with new path at the start of the line
      int separatorIndex = line.indexOf('|');
      if (separatorIndex != -1) {
        // Keep rest of line after '|'
        String rest = line.substring(separatorIndex);
        line = newPath + rest;
      } else {
        // Just replace whole line with new path if malformed
        line = newPath;
      }
    }
    updatedLines.push_back(line);
  }

  metaFile.close();

  // Delete old metadata file
  SD_MMC.remove(metaPath);

  // Recreate file and write updated lines
  File writeFile = SD_MMC.open(metaPath, FILE_WRITE);
  if (!writeFile) {
    Serial.println("Failed to recreate metadata file.");
    return;
  }

  for (const String& l : updatedLines) {
    writeFile.println(l);
  }

  writeFile.close();
  Serial.println("Metadata updated for renamed file.");
  if (SAVE_POWER) setCpuFrequencyMhz(40);
}

void copyFile(String oldFile, String newFile) {
  if (noSD) {
    oledWord("COPY FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    keypad.disableInterrupts();
    oledWord("Loading File");
    if (!oldFile.startsWith("/")) oldFile = "/" + oldFile;
    if (!newFile.startsWith("/")) newFile = "/" + newFile;
    String textToLoad = readFileToString(SD_MMC, (oldFile).c_str());
    writeFile(SD_MMC, (newFile).c_str(), textToLoad.c_str());
    oledWord("Saved: "+ newFile);

    // Write MetaData
    writeMetadata(newFile);

    delay(1000);
    keypad.enableInterrupts();

    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

void appendToFile(String path, String inText) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    SDActive = true;
    setCpuFrequencyMhz(240);
    delay(50);

    keypad.disableInterrupts();
    appendFile(SD_MMC, path.c_str(), inText.c_str());

    // Write MetaData
    writeMetadata(path);

    keypad.enableInterrupts();

    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
  }
}

String vectorToString() {
  String result;
  setTXTFont(currentFont);

  for (size_t i = 0; i < allLines.size(); i++) {
    result += allLines[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(allLines[i], 0, 0, &x1, &y1, &charWidth, &charHeight);

    // Add newline only if the line doesn't fully use the available space
    if (charWidth < display.width() && i < allLines.size() - 1) {
      result += '\n';
    }
  }

  return result;
}

void stringToVector(String inputText) {
  setTXTFont(currentFont);
  allLines.clear();
  String currentLine_;

  for (size_t i = 0; i < inputText.length(); i++) {
    char c = inputText[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(currentLine_, 0, 0, &x1, &y1, &charWidth, &charHeight);

    // Check if new line needed
    if ((c == '\n' || charWidth >= display.width() - 5) && !currentLine_.isEmpty()) {
      if (currentLine_.endsWith(" ")) {
        allLines.push_back(currentLine_);
        currentLine_ = "";
      }
      else {
        int lastSpace = currentLine_.lastIndexOf(' ');
        if (lastSpace != -1) {
          // Split line at last space
          String partialWord = currentLine_.substring(lastSpace + 1);
          currentLine_ = currentLine_.substring(0, lastSpace);
          allLines.push_back(currentLine_);
          currentLine_ = partialWord;  // Start new line with partial word
        }
        else {
          // No spaces, whole line is a single word
          allLines.push_back(currentLine_);
          currentLine_ = "";
        }
      }
    }
    
    if (c != '\n') {
      currentLine_ += c;
    }
  }

  // Push last line if not empty
  if (!currentLine_.isEmpty()) {
    allLines.push_back(currentLine_);
  }
}

String removeChar(String str, char character) {
  String result = "";
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != character) {
      result += str[i];
    }
  }
  return result;
}

// Misc Inputs
void updateBattState() {
  // Read and scale voltage (add calibration offset if needed)
  float rawVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;

  // Moving average smoothing (adjust alpha for responsiveness)
  static float filteredVoltage = rawVoltage;
  const float alpha = 0.1;  // Low-pass filter constant (lower = smoother, slower response)
  filteredVoltage = alpha * rawVoltage + (1.0 - alpha) * filteredVoltage;

  static float prevVoltage = 0.0;
  static int prevBattState = -1; // Ensure valid initial state
  const float threshold = 0.05;  // Hysteresis threshold

  int newState = battState;

  // Charging state overrides everything
  if (digitalRead(CHRG_SENS) == 1) {
    newState = 5;
  } else {
    // Normal battery voltage thresholds with hysteresis
    if (filteredVoltage > 4.1 || (prevBattState == 4 && filteredVoltage > 4.1 - threshold)) {
      newState = 4;
    } else if (filteredVoltage > 3.9 || (prevBattState == 3 && filteredVoltage > 3.9 - threshold)) {
      newState = 3;
    } else if (filteredVoltage > 3.8 || (prevBattState == 2 && filteredVoltage > 3.8 - threshold)) {
      newState = 2;
    } else if (filteredVoltage > 3.7 || (prevBattState == 1 && filteredVoltage > 3.7 - threshold)) {
      newState = 1;
    } else if (filteredVoltage <= 3.7) {
      newState = 0;
    }
  }

  if (newState != battState) {
    battState = newState;
    prevBattState = newState;
    // newState = true;
  }

  prevVoltage = filteredVoltage;
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
          case SHIFT:
            return keysArraySHFT[k/10][k%10];
          case FUNC:
            return keysArrayFN[k/10][k%10];
          default:
            return 0;
        }
      }
    }
  }

  return 0;
}

void setTimeFromString(String timeStr) {
    if (timeStr.length() != 5 || timeStr[2] != ':') {
        Serial.println("Invalid format! Use HH:MM");
        return;
    }

    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
      oledWord("Invalid");
      delay(500);
      return;
    }

    DateTime now = rtc.now();  // Get current date
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, 0));

    Serial.println("Time updated!");
}

int stringToInt(String str) {
  str.trim(); // Remove leading/trailing whitespace

  if (str.length() == 0) return -1;

  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      return -1; // Invalid character found
    }
  }

  return str.toInt(); // Safe to convert
}

// Misc Outputs
void playJingle(String jingle) {
  if (jingle == "startup") {
    buzzer.begin(0);
    buzzer.sound(NOTE_A8, 120);
    buzzer.sound(NOTE_B8, 120);
    buzzer.sound(NOTE_C8, 120);
    buzzer.sound(NOTE_D8, 120);
    buzzer.sound(0, 80);
    buzzer.end(0);
    return;
  }
  else if (jingle == "shutdown") {
    buzzer.begin(0);
    buzzer.sound(NOTE_D8, 120);
    buzzer.sound(NOTE_C8, 120);
    buzzer.sound(NOTE_B8, 120);
    buzzer.sound(NOTE_A8, 120);
    buzzer.sound(0, 80);
    buzzer.end(0);
    return;
  }
}

void printDebug() {
  DateTime now = rtc.now();
  if (now.second() != prevSec) {
    prevSec = now.second();

    // DIVIDER
    Serial.print("//////////////////////////////////////_DEBUG_//////////////////////////////////////"); 
    Serial.println();

    // DISPLAY GPIO STATES
    Serial.print("PWR_BTN: "); Serial.print(digitalRead(PWR_BTN));
    Serial.print(", KB_INT: "); Serial.print(digitalRead(KB_IRQ));
    Serial.print(", CHRG: "); Serial.print(digitalRead(CHRG_SENS));
    Serial.print(", RTC_INT: "); Serial.print(digitalRead(RTC_INT));

    // READ AND DISPLAY BATTERY VOLTAGE
    float batteryVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;
    Serial.print(", BAT: "); Serial.print(batteryVoltage, 2); // Print with 2 decimal places
    
    // DISPLAY CLOCK SPEED
    Serial.print(", CPU_FRQ: "); Serial.print(getCpuFrequencyMhz(), 1);

    // FAST FULL UPDATE MODE
    Serial.print(", FFU: "); Serial.println(GxEPD2_310_GDEQ031T10::useFastFullUpdate);

    // DISPLAY SYSTEM TIME
    Serial.print("SYSTEM_CLOCK: ");
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

void setCpuSpeed(int newFreq) {
  // Return early if the frequency is already set
  if (getCpuFrequencyMhz() == newFreq) return;

  int validFreqs[] = {240, 160, 80, 40, 20, 10};
  bool isValid = false;

  for (int i = 0; i < sizeof(validFreqs) / sizeof(validFreqs[0]); i++) {
    if (newFreq == validFreqs[i]) {
      isValid = true;
      break;
    }
  }

  if (isValid) {
    setCpuFrequencyMhz(newFreq);
    Serial.print("CPU Speed changed to: ");
    Serial.print(newFreq);
    Serial.println(" MHz");
  } 
}

void checkTimeout() {
  int randomScreenSaver = 0;
  timeoutMillis = millis();

  //Trigger timeout deep sleep
  if (!disableTimeout) {
    if (timeoutMillis - prevTimeMillis >= TIMEOUT*1000) {
        Serial.println("Device Idle... Deep Sleeping");
        //Give a chance to keep device awake
        oledWord("  Going to sleep!  ");
        int i = millis();
        int j = millis();
        while ((j - i) <= 4000) {  //10 sec
          j = millis();
          if (digitalRead(KB_IRQ) == 0) {
            oledWord("Good Save!");
            delay(500);
            prevTimeMillis = millis();
            keypad.flush();
            return;
          }
        }

        //Save current work:
        //Only save if alltext has significant content
        if (allText.length() > 10) {
          //No current file, save in temp.txt
          saveFile();
        }

        switch (CurrentAppState) {
          case TXT:
            if (SLEEPMODE == "TEXT" && editingFile != "") {
              prevAllText = allText;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              display.setFullWindow();
              if (TXT_APP_STYLE == 0) einkTextPartial(allText, true);
              else if (TXT_APP_STYLE == 1) einkTextDynamic(true, true);
                    
              display.setFont(&FreeMonoBold9pt7b);
              
              display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
              display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
              display.setCursor(4, display.height()-6);
              display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
              display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
              statusBar(editingFile, true);
              
              display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
              display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);

              // Put device to sleep with alternate sleep screen
              deepSleep(true);
            }
            else deepSleep();
            break;

          default:
            deepSleep();
            break;
        }
        
        display.nextPage();
        display.hibernate();
        
        //Sleep the device
        playJingle("shutdown");
        esp_deep_sleep_start();
      }
  }
  else {
    prevTimeMillis = millis();
  }
  
  // Power Button Event sleep
  if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
    PWR_BTN_event = false;

    // Save current work:
    // Only save if alltext has significant content
    if (allText.length() > 10) {
      oledWord("Saving Work");
      saveFile();
    }
    
    if (digitalRead(CHRG_SENS) == HIGH) {
      // Save last state
      prefs.begin("PocketMage", false);
      prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
      prefs.putString("editingFile", editingFile);
      prefs.end();

      CurrentAppState = HOME;
      CurrentHOMEState = NOWLATER;
      updateTaskArray();
      sortTasksByDueDate(tasks);

      u8g2.setPowerSave(1);
      OLEDPowerSave  = true;
      disableTimeout = true;
      newState = true;
      
      // Shutdown Jingle
      playJingle("shutdown");

      // Clear screen
      display.setFullWindow();
      display.fillScreen(GxEPD_WHITE);

    }
    else {
      switch (CurrentAppState) {
        case TXT:
          if (SLEEPMODE == "TEXT" && editingFile != "") {
            prevAllText = allText;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            display.setFullWindow();
            if (TXT_APP_STYLE == 0) einkTextPartial(allText, true);
            else if (TXT_APP_STYLE == 1) einkTextDynamic(true, true);    
            display.setFont(&FreeMonoBold9pt7b);
            
            display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
            display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
            display.setCursor(4, display.height()-6);
            display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
            display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
            statusBar(editingFile, true);
            
            display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);

            deepSleep(true);
          }
          // Sleep device normally
          else deepSleep();
          break;
        default:
          deepSleep();
          break;
      }
    }
    
  }
  else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    // Load last state
    /*prefs.begin("PocketMage", true);
    editingFile = prefs.getString("editingFile", "");
    if (HOME_ON_BOOT) CurrentAppState = HOME;
    else CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));
    prefs.end();*/
    loadState();
    keypad.flush();

    CurrentHOMEState = HOME_HOME;
    PWR_BTN_event = false;
    if (OLEDPowerSave) {
      u8g2.setPowerSave(0);
      OLEDPowerSave = false;
    }
    display.fillScreen(GxEPD_WHITE);
    forceSlowFullUpdate = true;

    // Play startup jingle
    playJingle("startup");

    refresh();
    delay(200);
    newState = true;
  }
}

void deepSleep(bool alternateScreenSaver) {
  // Put OLED to sleep
  u8g2.setPowerSave(1);

  // Stop the einkHandler task
  if (einkHandlerTaskHandle != NULL) {
    vTaskDelete(einkHandlerTaskHandle);
    einkHandlerTaskHandle = NULL;
  }
  
  // Shutdown Jingle
  playJingle("shutdown");

  if (alternateScreenSaver == false) {
    int numScreensavers = sizeof(ScreenSaver_allArray) / sizeof(ScreenSaver_allArray[0]);
    int randomScreenSaver_ = esp_random() % numScreensavers;

    //display.setPartialWindow(0, 0, 320, 60);
    display.setFullWindow();
    display.drawBitmap(0, 0, ScreenSaver_allArray[randomScreenSaver_], 320, 240, GxEPD_BLACK);
    multiPassRefesh(2);
  }
  else {
    // Display alternate screensaver
    forceSlowFullUpdate = true;
    refresh();
    delay(100);
  }

  // Put E-Ink to sleep
  display.hibernate();

  // Save last state
  prefs.begin("PocketMage", false);
  prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
  prefs.putString("editingFile", editingFile);
  prefs.end();
      
  // Sleep the ESP32
  esp_deep_sleep_start();
}

void loadState(bool changeState) {
  // LOAD PREFERENCES
  prefs.begin("PocketMage", true); // Read-Only
  // Misc
  TIMEOUT           = prefs.getInt("TIMEOUT", 120);
  DEBUG_VERBOSE     = prefs.getBool("DEBUG_VERBOSE", true);
  SYSTEM_CLOCK      = prefs.getBool("SYSTEM_CLOCK", true);
  SHOW_YEAR         = prefs.getBool("SHOW_YEAR", true);
  SAVE_POWER        = prefs.getBool("SAVE_POWER", true);
  ALLOW_NO_MICROSD  = prefs.getBool("ALLOW_NO_SD", false);
  editingFile       = prefs.getString("editingFile", "");
  HOME_ON_BOOT      = prefs.getBool("HOME_ON_BOOT", false);
  OLED_BRIGHTNESS   = prefs.getInt("OLED_BRIGHTNESS", 255);
  OLED_MAX_FPS      = prefs.getInt("OLED_MAX_FPS", 30);

  // Update State (if needed)
  if (changeState) {
    u8g2.setContrast(OLED_BRIGHTNESS);

    if (HOME_ON_BOOT) CurrentAppState = HOME;
    else CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));
    
    keypad.flush();

    // Initialize boot app if needed
    switch (CurrentAppState) {
      case HOME:
        newState = true;
        break;
      case TXT:
        if (editingFile != "") loadFile(false);
        else {
          stringToVector("");
        }
        CurrentKBState  = NORMAL;
        dynamicScroll = 0;
        newLineAdded = true;
        newState = false;
        break;
      case SETTINGS:
        newState = true;
        break;
      case TASKS:
        CurrentTasksState = TASKS0;
        forceSlowFullUpdate = true;
        newState = true;
        break;
      case USB_APP:
        CurrentAppState = HOME;
        CurrentKBState  = NORMAL;
        newState = true;
        break;
      case CALENDAR:
        CALENDAR_INIT();
        break;
      case LEXICON:
        LEXICON_INIT();
        break;
      case JOURNAL:
        JOURNAL_INIT();
        break;
    }
  }

  prefs.end();
}

// Low-Level SDMMC Operations
void listDir(fs::FS &fs, const char *dirname) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
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
        String fileName = String(file.name());
        
        // Check if file is in the exclusion list
        bool excluded = false;
        for (const String &excludedFile : excludedFiles) {
          if (fileName.equals(excludedFile) || ("/"+fileName).equals(excludedFile)) {
            excluded = true;
            break;
          }
        }

        if (!excluded) {
          filesList[fileIndex++] = fileName; // Store file name if not excluded
        }
      }
      file = root.openNextFile();
    }

    for (int i = 0; i < fileIndex; i++) { // Only print valid entries
      Serial.println(filesList[i]);
    }

    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

void readFile(fs::FS &fs, const char *path) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
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
    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

String readFileToString(fs::FS &fs, const char *path) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return "";
  }
  else { 
    setCpuFrequencyMhz(240);
    delay(50);

    noTimeout = true;
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
    einkRefresh = FULL_REFRESH_AFTER; //Force a full refresh
    noTimeout = false;
    return content;  // Return the complete String
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
    Serial.printf("Writing file: %s\r\n", path);
    delay(200);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("- failed to open file for writing");
      return;
    }
    if (file.print(message)) {
      Serial.println("- file written");
    } 
    else {
      Serial.println("- write failed");
    }
    file.close();
    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      Serial.println("- failed to open file for appending");
      return;
    }
    if (file.println(message)) {
      Serial.println("- message appended");
    } 
    else {
      Serial.println("- append failed");
    }
    file.close();
    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
      Serial.println("- file renamed");
    } 
    else {
      Serial.println("- rename failed");
    }
    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  if (noSD) {
    oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    noTimeout = true;
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
      Serial.println("- file deleted");
    } 
    else {
      Serial.println("- delete failed");
    }
    noTimeout = false;
    //if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}