//    .oooooo.   ooooo        oooooooooooo oooooooooo.    //
//   d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888          888          888      888  //
//  888      888  888          888oooo8     888      888  //
//  888      888  888          888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //                                               
                                                     
void oledWord(String word) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB24_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+12,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB18_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+9,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB14_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+7,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB12_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+6,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB10_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+5,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  else {
    u8g2.drawStr(128 - u8g2.getStrWidth(word.c_str()),16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  
}

void oledLine(String line, bool doProgressBar) {
  uint8_t maxLength = maxCharsPerLine;
  u8g2.clearBuffer();

  // DRAW LINE TEXT
  u8g2.setFont(u8g2_font_ncenB18_tr);
  u8g2.drawStr(120-u8g2.getStrWidth(line.c_str()),16+9,line.c_str());
  
  //PROGRESS BAR
  if (doProgressBar && line.length() > 0) {
    //uint8_t progress = map(line.length(), 0, maxLength, 0, 128);

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(line, 0, 0, &x1, &y1, &charWidth, &charHeight);

    uint8_t progress = map(charWidth, 0, display.width()-5, 0, 128);

    u8g2.drawVLine(127, 0, 2);
    u8g2.drawVLine(0, 0, 2);
    
    u8g2.drawHLine(0,0,progress);
    u8g2.drawHLine(0,1,progress);

    // LINE END WARNING INDICATOR
    if (charWidth > ((display.width()-5) * 0.8)) {   
      if ((millis() / 400) % 2 == 0) {  // ON for 200ms, OFF for 200ms
        u8g2.drawVLine(127, 8, 32-16);
        u8g2.drawLine(127,15,124,12);
        u8g2.drawLine(127,15,124,18);
      }
    }
  }

  // DRAW FN/SHIFT IF NEEDED
  u8g2.setFont(u8g2_font_u8glib_4_tf);

  switch (CurrentKBState) {
    case SHIFT:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("SHFT")) / 2, u8g2.getDisplayHeight(), "SHIFT");
      break;
    case FUNC:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN")) / 2, u8g2.getDisplayHeight(), "FN");
      break;
  }

  u8g2.setFont(u8g2_font_ncenB18_tr);
  u8g2.sendBuffer();

}
