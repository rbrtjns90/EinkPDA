//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //                                                    

void refresh() {
  display.nextPage();
  display.hibernate();
  display.fillScreen(GxEPD_WHITE);
}

void einkHandler(void *parameter) {
  delay(1000);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  //display.nextPage();
  //display.hibernate();
  display.display(true);
  display.hibernate();

  while (true) {
    applicationEinkHandler();
  }
}

void statusBar(String input, bool fullWindow = false) {
  display.setFont(&FreeMonoBold9pt7b);
  if (!fullWindow) display.setPartialWindow(0,display.height()-20,display.width(),20);
  display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
  display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
  display.setCursor(4, display.height()-6);
  display.print(input);

  switch (CurrentKBState) {
    case NORMAL:
      //Display battery level
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
      break;
    case SHIFT:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[0], 30, 20, GxEPD_BLACK);
      break;
    case FUNC:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[1], 30, 20, GxEPD_BLACK);
      break;
  }
  display.drawRect(display.width()-30,display.height()-20,30,20,GxEPD_BLACK);
}

void einkTextPartial(String text, bool noRefresh = false) {
  bool doFullRefresh = false;

  einkRefresh++;
  if (einkRefresh > FULL_REFRESH_AFTER) {
    doFullRefresh = true;
    einkRefresh = 0;
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
  }

  display.setFont(&FreeMonoBold9pt7b);

  if(splitIntoLines(text.c_str(), scroll)) doFullRefresh = true;

  for (int i = 0; i < 13; i++) {
    if (outLines[i] != "") { // Print only non-empty lines
      if (doFullRefresh) {
        display.fillRect(0,16*i,display.width(),16,GxEPD_WHITE);
        display.setCursor(0, 10+(16*i));
        display.print(outLines[i]);
      }
      else if (outLines[i] != lines_prev[i]) {   //If the line has changed
        display.setPartialWindow(0,16*i,display.width(),16);
        display.fillRect(0,16*i,display.width(),16,GxEPD_WHITE);
        display.setCursor(0, 10+(16*i));
        display.print(outLines[i]);
        if (!noRefresh) refresh();
      }
    }
  }

  if (doFullRefresh && !noRefresh) {
    display.nextPage();
    display.hibernate();
  }

  for (int i = 0; i < 13; i++) {
    lines_prev[i] = outLines[i]; // Copy each line
  }
}