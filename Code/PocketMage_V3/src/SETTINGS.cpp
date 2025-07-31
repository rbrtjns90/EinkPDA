#include "globals.h"

void SETTINGS_INIT() {
  // OPEN SETTINGS
  currentLine = "";
  CurrentAppState = SETTINGS;
  CurrentSettingsState = settings0;
  CurrentKBState  = NORMAL;
  newState = true;
}

void settingCommandSelect(String command) {
  command.toLowerCase();

  if (command.startsWith("timeset ")) {
    String timePart = command.substring(8);
    setTimeFromString(timePart);
    return;
  }
  else if (command.startsWith("dateset ")) {
    String datePart = command.substring(8);
    if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();

      DateTime now = rtc.now();  // Preserve current time
      rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    } else {
      oledWord("Invalid format (use YYYYMMDD)");
      delay(2000);
    }
    return;
  }
  else if (command.startsWith("lumina ")) {
    String luminaPart = command.substring(7);
    int lumina = stringToInt(luminaPart);
    if (lumina == -1) {
      oledWord("Invalid");
      delay(500);
      return;
    }
    else if (lumina > 255) lumina = 255;
    else if (lumina < 0) lumina = 0;
    OLED_BRIGHTNESS = lumina;
    u8g2.setContrast(OLED_BRIGHTNESS);
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_BRIGHTNESS", OLED_BRIGHTNESS);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }
  else if (command.startsWith("timeout ")) {
    String timeoutPart = command.substring(8);
    int timeout = stringToInt(timeoutPart);
    if (timeout == -1) return;
    else if (timeout > 3600) timeout = 3600;
    else if (timeout < 15) timeout = 15;
    TIMEOUT = timeout;
    prefs.begin("PocketMage", false);
    prefs.putInt("TIMEOUT", TIMEOUT);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }
  else if (command.startsWith("oledfps ")) {
    String oledfpsPart = command.substring(8);
    int oledfps = stringToInt(oledfpsPart);
    if (oledfps == -1) {
      oledWord("Invalid");
      delay(500);
      return;
    }
    else if (oledfps > 144) oledfps = 144;
    else if (oledfps < 5) oledfps = 5;
    OLED_MAX_FPS = oledfps;
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_MAX_FPS", OLED_MAX_FPS);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }
  else if (command.startsWith("clock ")) {
    String clockPart = command.substring(6);
    clockPart.trim();

    if (clockPart != "t" && clockPart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    SYSTEM_CLOCK = (clockPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SYSTEM_CLOCK", SYSTEM_CLOCK);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }

  else if (command.startsWith("showyear ")) {
    String yearPart = command.substring(9);
    yearPart.trim();

    if (yearPart != "t" && yearPart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    SHOW_YEAR = (yearPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SHOW_YEAR", SHOW_YEAR);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }

  else if (command.startsWith("savepower ")) {
    String savePowerPart = command.substring(10);
    savePowerPart.trim();

    if (savePowerPart != "t" && savePowerPart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    SAVE_POWER = (savePowerPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SAVE_POWER", SAVE_POWER);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }

  else if (command.startsWith("debug ")) {
    String debugPart = command.substring(6);
    debugPart.trim();

    if (debugPart != "t" && debugPart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    DEBUG_VERBOSE = (debugPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("DEBUG_VERBOSE", DEBUG_VERBOSE);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }

  else if (command.startsWith("boottohome ")) {
    String bootHomePart = command.substring(11);
    bootHomePart.trim();

    if (bootHomePart != "t" && bootHomePart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    HOME_ON_BOOT = (bootHomePart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("HOME_ON_BOOT", HOME_ON_BOOT);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }

  else if (command.startsWith("allownosd ")) {
    String noSDPart = command.substring(10);
    noSDPart.trim();

    if (noSDPart != "t" && noSDPart != "f") {
      oledWord("Invalid");
      delay(500);
      return;
    }

    ALLOW_NO_MICROSD = (noSDPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("ALLOW_NO_MICROSD", ALLOW_NO_MICROSD);
    prefs.end();
    newState = true;
    oledWord("Settings Updated");
    delay(200);
    return;
  }
  else {
    oledWord("Huh?");
    delay(1000);
  }
  return;
}

void processKB_settings() {
  int currentMillis = millis();

  switch (CurrentSettingsState) {
    case settings0:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          settingCommandSelect(currentLine);
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
        else if (inchar == 12) {
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

    case settings1:
      break;
  }
}

void einkHandler_settings() {
  if (newState) {
    newState = false;

    // Load settings
    loadState(false);
    
    // Display Background
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    display.setFont(&FreeSerif9pt7b);
    // First column of settings
    // OLED_BRIGHTNESS
    display.setCursor(8, 42);
    display.print(String(OLED_BRIGHTNESS).c_str());
    // TIMEOUT
    display.setCursor(8, 65);
    display.print(String(TIMEOUT).c_str());
    // SYSTEM_CLOCK
    if (SYSTEM_CLOCK) display.drawBitmap(8, 75, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 75, _toggleOFF, 26, 11, GxEPD_BLACK);
    // SHOW_YEAR
    if (SHOW_YEAR) display.drawBitmap(8, 98, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 98, _toggleOFF, 26, 11, GxEPD_BLACK);
    // SAVE_POWER
    if (SAVE_POWER) display.drawBitmap(8, 121, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 121, _toggleOFF, 26, 11, GxEPD_BLACK);
    // DEBUG_VERBOSE
    if (DEBUG_VERBOSE) display.drawBitmap(8, 144, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 144, _toggleOFF, 26, 11, GxEPD_BLACK);
    // HOME_ON_BOOT
    if (HOME_ON_BOOT) display.drawBitmap(8, 167, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 167, _toggleOFF, 26, 11, GxEPD_BLACK);
    // ALLOW_NO_MICROSD
    if (ALLOW_NO_MICROSD) display.drawBitmap(8, 190, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 190, _toggleOFF, 26, 11, GxEPD_BLACK);
    // OLED_MAX_FPS
    display.setCursor(163, 42);
    display.print(String(OLED_MAX_FPS).c_str());

    drawStatusBar("Type a Command:");

    multiPassRefesh(2);
  }
}