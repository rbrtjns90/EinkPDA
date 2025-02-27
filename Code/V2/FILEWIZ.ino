//  oooooooooooo ooooo ooooo        oooooooooooo oooooo   oooooo     oooo ooooo  oooooooooooo  //
//  `888'     `8 `888' `888'        `888'     `8  `888.    `888.     .8'  `888' d'""""""d888'  //
//   888          888   888          888           `888.   .8888.   .8'    888        .888P    //
//   888oooo8     888   888          888oooo8       `888  .8'`888. .8'     888       d888'     //
//   888    "     888   888          888    "        `888.8'  `888.8'      888     .888P       //
//   888          888   888       o  888       o      `888'    `888'       888    d888'    .P  //
//  o888o        o888o o888ooooood8 o888ooooood8       `8'      `8'       o888o .8888888888P   //

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