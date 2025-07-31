#include "globals.h"

String currentJournal = "";
String bufferEditingFile = editingFile;

void JOURNAL_INIT() {
  CurrentAppState = JOURNAL;
  CurrentJournalState = J_MENU;
  forceSlowFullUpdate = true;
  newState = true;
  CurrentKBState = NORMAL;
  bufferEditingFile = editingFile;
}

// File Operations
void loadJournal() {
  editingFile = currentJournal;
  loadFile();
}

void saveJournal() {
  editingFile = currentJournal;
  saveFile();
}

// Functions
bool isLeapYear(int year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

void drawJMENU() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  // Display background
  drawStatusBar("Type:YYYYMMDD or (T)oday");
  display.drawBitmap(0, 0, _journal, 320, 218, GxEPD_BLACK);

  // Update current progress graph
  DateTime now = rtc.now();
  String year = String(now.year());

  // Files are in the format "/journal/YYYYMMDD.txt"
  // JANUARY
  for (int i = 1; i < 31; i++) {
    String dayCode = "";
    if (i<10) dayCode = "0"+String(i);
    else dayCode = String(i);
    
    String fileCode = "/journal/" + year + "01" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7*(i-1)), 50, 4, 4, GxEPD_BLACK);
  }
  // FEBRUARY
  int febDays = isLeapYear(year.toInt()) ? 29 : 28;
  for (int i = 1; i <= febDays; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "02" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 59, 4, 4, GxEPD_BLACK);
  }

  // MARCH
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "03" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 68, 4, 4, GxEPD_BLACK);
  }

  // APRIL
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "04" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 77, 4, 4, GxEPD_BLACK);
  }

  // MAY
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "05" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 86, 4, 4, GxEPD_BLACK);
  }

  // JUNE
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "06" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 95, 4, 4, GxEPD_BLACK);
  }

  // JULY
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "07" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 104, 4, 4, GxEPD_BLACK);
  }

  // AUGUST
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "08" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 113, 4, 4, GxEPD_BLACK);
  }

  // SEPTEMBER
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "09" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 122, 4, 4, GxEPD_BLACK);
  }

  // OCTOBER
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "10" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 131, 4, 4, GxEPD_BLACK);
  }

  // NOVEMBER
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "11" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 140, 4, 4, GxEPD_BLACK);
  }

  // DECEMBER
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "12" + dayCode + ".txt";
    if (SD_MMC.exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 149, 4, 4, GxEPD_BLACK);
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void JMENUCommand(String command) {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  command.toLowerCase();

  if (command == "t") {
    DateTime now = rtc.now();

    String dayStr = "";
    if (now.day() < 10) dayStr = "0" + String(now.day());
    else dayStr = String(now.day());

    String monthStr = "";
    if (now.month() < 10) monthStr = "0" + String(now.month());
    else monthStr = String(now.month());

    String fileName = "/journal/" + String(now.year()) + monthStr + dayStr + ".txt";
    
    // If file doesn't exist, create it
    if (!SD_MMC.exists(fileName)) {
      File f = SD_MMC.open(fileName, FILE_WRITE);
      if (f) f.close();
    }

    currentJournal = fileName;

    // Load file
    editingFile = currentJournal;
    loadJournal();

    dynamicScroll = 0;
    newLineAdded = true;
    CurrentJournalState = J_TXT;
    return;
  }

  // command in the form "YYYYMMDD"
  else if (command.length() == 8 && command.toInt() > 0) {
    String fileName = "/journal/" + command + ".txt";

    if (!SD_MMC.exists(fileName)) {
      File f = SD_MMC.open(fileName, FILE_WRITE);
      if (f) f.close();
    }

    currentJournal = fileName;

    // Load file
    editingFile = currentJournal;
    loadJournal();

    dynamicScroll = 0;
    newLineAdded = true;
    CurrentJournalState = J_TXT;
    return;
  }

  // command in the form "jan 1"
  else {
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex != -1 && spaceIndex < command.length() - 1) {
      String monthStr = command.substring(0, spaceIndex);
      String dayStr = command.substring(spaceIndex + 1);
      int day = dayStr.toInt();

      if (day < 1 || day > 31) return;  // invalid day

      String monthMap = "janfebmaraprmayjunjulaugsepoctnovdec";
      int monthIndex = monthMap.indexOf(monthStr);
      if (monthIndex == -1) return;  // invalid month

      int month = (monthIndex / 3) + 1;

      String year = String(rtc.now().year());
      String m = (month < 10) ? "0" + String(month) : String(month);
      String d = (day < 10) ? "0" + String(day) : String(day);
      String fileName = "/journal/" + year + m + d + ".txt";

      if (!SD_MMC.exists(fileName)) {
        File f = SD_MMC.open(fileName, FILE_WRITE);
        if (f) f.close();
      }

      currentJournal = fileName;

      // Load file
      editingFile = currentJournal;
      loadJournal();

      dynamicScroll = 0;
      newLineAdded = true;
      CurrentJournalState = J_TXT;
      return;
    }
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

// Loops
void processKB_JOURNAL() {
  int currentMillis = millis();
  char inchar;

  switch (CurrentJournalState) {
    case J_MENU:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          JMENUCommand(currentLine);
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
          editingFile = bufferEditingFile;
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

    case J_TXT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
      inchar = updateKeypress();
      // SET MAXIMUMS AND FONT
      setTXTFont(currentFont);

      // UPDATE SCROLLBAR
      updateScrollFromTouch();

      // HANDLE INPUTS
      //No char recieved
      if (inchar == 0);  
      else if (inchar == 12) {
        JOURNAL_INIT();
      }
      //TAB Recieved
      else if (inchar == 9) {                                  
        currentLine += "    ";
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
      //CR Recieved
      else if (inchar == 13) {                          
        allLines.push_back(currentLine);
        currentLine = "";
        newLineAdded = true;
      }
      //ESC / CLEAR Recieved
      else if (inchar == 20) {                                  
        allLines.clear();
        currentLine = "";
        oledWord("Clearing...");
        doFull = true;
        newLineAdded = true;
        delay(300);
      }
      // LEFT
      else if (inchar == 19) {                                  
        
      }
      // RIGHT
      else if (inchar == 21) {                                  
        
      }
      //BKSP Recieved
      else if (inchar == 8) {                  
        if (currentLine.length() > 0) {
          currentLine.remove(currentLine.length() - 1);
        }
      }
      //SAVE Recieved
      else if (inchar == 6) { 
        saveJournal();
        CurrentKBState = NORMAL;
        newLineAdded = true;
      }
      //LOAD Recieved
      else if (inchar == 5) {
        loadJournal();
        CurrentKBState = NORMAL;
        newLineAdded = true;
      }
      // Font Switcher 
      else if (inchar == 14) {                                  
        CurrentTXTState = FONT;
        CurrentKBState = FUNC;
        newState = true;
      }
      else {
        currentLine += inchar;
        if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
        else if (CurrentKBState != NORMAL) {
          CurrentKBState = NORMAL;
        }
      }

      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - OLEDFPSMillis >= (1000/60)) {
        OLEDFPSMillis = currentMillis;
        // ONLY SHOW OLEDLINE WHEN NOT IN SCROLL MODE
        if (lastTouch == -1) {
          oledLine(currentLine);
          if (prev_dynamicScroll != dynamicScroll) prev_dynamicScroll = dynamicScroll;
        }
        else oledScroll();
      }

      if (currentLine.length() > 0) {
        int16_t x1, y1;
        uint16_t charWidth, charHeight;
        display.getTextBounds(currentLine, 0, 0, &x1, &y1, &charWidth, &charHeight);

        if (charWidth >= display.width()-5) {
          // If currentLine ends with a space, just start a new line
          if (currentLine.endsWith(" ")) {
            allLines.push_back(currentLine);
            currentLine = "";
          }
          // If currentLine ends with a letter, we are in the middle of a word
          else {
            int lastSpace = currentLine.lastIndexOf(' ');
            String partialWord;

            if (lastSpace != -1) {
              partialWord = currentLine.substring(lastSpace + 1);
              currentLine = currentLine.substring(0, lastSpace);  // Strip partial word
              allLines.push_back(currentLine);
              currentLine = partialWord;  // Start new line with the partial word
            } 
            // No spaces found, whole line is a single word
            else {
              allLines.push_back(currentLine);
              currentLine = "";
            }
          }
          newLineAdded = true;
        }
      }

      break;
  }
  }
}

void einkHandler_JOURNAL() {
  switch (CurrentJournalState) {
    case J_MENU:
      if (newState) {
        newState = false;

        drawJMENU();

        multiPassRefesh(2);
      }
      break;
    case J_TXT:
      if (newState && doFull) {
        display.fillScreen(GxEPD_WHITE);
        refresh();
      }
      if (newLineAdded && !newState) {
        einkTextDynamic(true);
        refresh();
      }

      newState = false;
      newLineAdded = false;
      break;
  } 
}
