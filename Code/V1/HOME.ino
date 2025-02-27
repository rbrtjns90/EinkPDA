void processKB_HOME() {
  CurrentKBState = FUNC;
  int currentMillis = millis();
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
          newState = true;
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
  }
}

void einkHandler_HOME() {
  if (newState) {
    display.setRotation(3);
    display.fillScreen(GxEPD_WHITE);

    for (int i = 0; i < 5; i++) {
      display.drawBitmap(20 + (60*i), 20, homeIconsAllArray[i], 40, 40, GxEPD_BLACK);
    }

    display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
    display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
    display.drawRect(display.width()-30,display.height()-20,30,20,GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(4, display.height()-6);
    display.print("Press a key 1-5");

    refresh();
    newState = false;
  }
}