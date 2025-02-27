//   .oooooo..o oooooo   oooo  .oooooo..o ooooooooooooo oooooooooooo ooo        ooooo  //
//  d8P'    `Y8  `888.   .8'  d8P'    `Y8 8'   888   `8 `888'     `8 `88.       .888'  //
//  Y88bo.        `888. .8'   Y88bo.           888       888          888b     d'888   //
//   `"Y8888o.     `888.8'     `"Y8888o.       888       888oooo8     8 Y88. .P  888   //
//       `"Y88b     `888'          `"Y88b      888       888    "     8  `888'   888   //
//  oo     .d8P      888      oo     .d8P      888       888       o  8    Y     888   //
//  8""88888P'      o888o     8""88888P'      o888o     o888ooooood8 o8o        o888o  //

void checkTimeout() {
  timeoutMillis = millis();

  //Trigger timeout deep sleep
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
      if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
      //Save to SPIFFS
      keypad.disableInterrupts();
      oledWord("Saving File");
      delay(200);
      writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
      oledWord("Saved");
      delay(200);
      keypad.enableInterrupts();
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

void TCA8418_irq() {
  TCA8418_event = true;
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

int countLines(String input, size_t maxLineLength = 29) {
  size_t    inputLength   = input.length();
  uint8_t   charCounter   = 0;
  uint16_t  lineCounter   = 1;

    for (size_t c = 0; c < inputLength; c++) { 
      if (input[c] == '\n') {
        charCounter = 0;
        lineCounter++;
        continue;
      }
      else if (charCounter > (maxLineLength-1)) {
        charCounter = 0;
        lineCounter++;
      }
      charCounter++;
    }

    return lineCounter;
}