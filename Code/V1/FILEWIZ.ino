void processKB_FILEWIZ() {
  CurrentKBState = FUNC;
  int currentMillis = millis();
  //Make sure oled only updates at 60fps
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = updateKeypress();
    //No char recieved
    if (inchar == 0);
    //BKSP Recieved
    else if (inchar == 127 || inchar == 8) {
      CurrentAppState = HOME;
      CurrentKBState  = FUNC;
      einkRefresh = FULL_REFRESH_AFTER + 1;
      newState = true;
    }
    else {
      int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
      
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

void einkHandler_FILEWIZ() {
  if (newState) {
    display.setRotation(3);
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);

    //bottomText("Select a File");

    display.drawBitmap(0, 0, fileWizardallArray[0], 320, 218, GxEPD_BLACK);

    refresh();
    //newState = false;
  }
}