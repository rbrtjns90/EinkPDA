#ifndef GLOBALS_OVERRIDE_H
#define GLOBALS_OVERRIDE_H

// This file overrides problematic declarations in globals.h for desktop compilation

#include "pocketmage_compat.h"

// Override the template display declaration
#define GxEPD2_BW GxEPD2_310_GDEQ031T10
extern GxEPD2_310_GDEQ031T10 display;
extern U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2;

// Override other problematic declarations
extern RTC_DS3231 rtc;
extern Buzzer buzzer;
extern sdmmc_card_t* card;

// Function overrides
void drawStatusBar(String input);
void oledLine(String line, bool doProgressBar = true, String bottomMsg = "");

#endif
