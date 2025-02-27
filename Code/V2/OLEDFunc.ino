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
