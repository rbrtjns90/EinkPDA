#ifndef CONFIG_H
#define CONFIG_H

// CONFIGURATION & SETTINGS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|
#define KB_COOLDOWN 50                // Keypress cooldown
#define FULL_REFRESH_AFTER 5          // Full refresh after N partial refreshes (CHANGE WITH CAUTION)
#define MAX_FILES 10                  // Number of files to store
#define TIMEOUT 300                   // Time until automatic sleep (Seconds)
#define FORMAT_SPIFFS_IF_FAILED true  // Format the SPIFFS filesystem if mount fails
#define DEBUG_VERBOSE true            // Spit out some extra information
#define SLEEPMODE "TEXT"              // TEXT, SPLASH, CLOCK
#define TXT_APP_STYLE 1               // 0: Old Style (NOT SUPPORTED), 1: New Style
#define SET_CLOCK_ON_UPLOAD false     // Should system clock be set automatically on code upload?
#define SYSTEM_CLOCK true             // Enable a small clock on the bottom of the screen.
#define SHOW_YEAR false               // Show the year on the clock
#define SAVE_POWER true               // Enable a slower CPU clock speed to save battery with little cost to performance
#define TOUCH_TIMEOUT_MS 1200         // Delay after scrolling to return to typing mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|

// PIN DEFINITION
#define I2C_SCL      35
#define I2C_SDA      36
#define MPR121_ADDR  0x5A

#define KB_IRQ       8
#define PWR_BTN      38
#define BAT_SENS     6
#define CHRG_SENS    39
#define RTC_INT      1 

#define SPI_MOSI     14
#define SPI_SCK      15

#define OLED_CS      47
#define OLED_DC      46
#define OLED_RST     45

#define EPD_CS       2
#define EPD_DC       21
#define EPD_RST      9
#define EPD_BUSY     37


#endif