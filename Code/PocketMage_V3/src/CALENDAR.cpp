#include "globals.h"

int monthOffsetCount = 0;
int weekOffsetCount = 0;

int currentMonth = 0;
int currentYear = 0;

std::vector<std::vector<String>> dayEvents;
std::vector<std::vector<String>> calendarEvents;

void CALENDAR_INIT() {
  currentLine = "";
  CurrentAppState = CALENDAR;
  CurrentCalendarState = MONTH;
  CurrentKBState  = NORMAL;
  newState = true;
  monthOffsetCount = 0;
  weekOffsetCount = 0;
}

// Event Data Management
void updateEventArray() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  File file = SD_MMC.open("/sys/events.txt", "r"); // Open the text file in read mode
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  calendarEvents.clear(); // Clear the existing vector before loading the new data

  // Loop through the file, line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any extra spaces or newlines
    
    // Skip empty lines
    if (line.length() == 0) {
      continue;
    }

    // Split the line into individual parts using the delimiter '|'
    uint8_t delimiterPos1 = line.indexOf('|');
    uint8_t delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);
    uint8_t delimiterPos3 = line.indexOf('|', delimiterPos2 + 1);
    uint8_t delimiterPos4 = line.indexOf('|', delimiterPos3 + 1);
    uint8_t delimiterPos5 = line.indexOf('|', delimiterPos4 + 1);

    String eventName  = line.substring(0, delimiterPos1);
    String startDate   = line.substring(delimiterPos1 + 1, delimiterPos2);
    String startTime  = line.substring(delimiterPos2 + 1, delimiterPos3);
    String duration = line.substring(delimiterPos3 + 1, delimiterPos4);
    String repeat = line.substring(delimiterPos4 + 1, delimiterPos5);
    String note = line.substring(delimiterPos5 + 1);

    // Add the event to the vector
    calendarEvents.push_back({eventName, startDate, startTime, duration, repeat, note});
  }

  file.close();  // Close the file

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void sortEventsByDate(std::vector<std::vector<String>> &calendarEvents) {
  std::sort(calendarEvents.begin(), calendarEvents.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; // Compare dueDate strings
  });
}

void updateEventsFile() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);
  // Clear the existing calendarEvents file first
  delFile("/sys/events.txt");

  // Iterate through the calendarEvents vector and append each task to the file
  for (size_t i = 0; i < calendarEvents.size(); i++) {
    // Create a string from the task's attributes with "|" delimiter
    String eventInfo = calendarEvents[i][0] + "|" + calendarEvents[i][1] + "|" + calendarEvents[i][2] + "|" + calendarEvents[i][3]+ "|" + calendarEvents[i][4]+ "|" + calendarEvents[i][5];
    
    // Append the task info to the file
    appendToFile("/sys/events.txt", eventInfo);
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void addEvent(String eventName, String startDate, String startTime , String duration, String repeat, String note) {
  String eventInfo = eventName+"|"+startDate+"|"+startTime +"|"+duration+"|"+repeat+"|"+note;
  updateEventArray();
  calendarEvents.push_back({eventName, startDate, startTime , duration, repeat, note});
  sortEventsByDate(calendarEvents);
  updateEventsFile();
}

void deleteEvent(int index) {
  if (index >= 0 && index < calendarEvents.size()) {
    calendarEvents.erase(calendarEvents.begin() + index);
  }
}

// General Functions
String getMonthName(int month) {
  switch(month) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "ERR";
  }
}

int stringToPositiveInt(String input) {
  input.trim();
  if (input.length() == 0) return -1;

  for (int i = 0; i < input.length(); i++) {
    if (!isDigit(input[i])) return -1;
  }

  return input.toInt();
}

int daysInMonth(int year, int month) {
  if (month == 2) {
    // Leap year
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  } else {
    return 31;
  }
}

void commandSelectMonth(String command) {
  command.toLowerCase();

  const char* monthNames[] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec"
  };

  // Check if command starts with a 3-letter month
  if (command.length() >= 4) {
    String prefix = command.substring(0, 3);
    String yearPart = command.substring(4);
    yearPart.trim();

    for (int i = 0; i < 12; i++) {
      if (prefix == monthNames[i]) {
        int yearInt = stringToInt(yearPart);
        if (yearInt == -1 || yearInt < 1970 || yearInt > 2200) {
          oledWord("Invalid");
          delay(500);
          return;
        }

        currentMonth = i + 1;        // 1-indexed month
        currentYear = yearInt;
        newState = true;

        // Update monthOffsetCount relative to now
        DateTime now = rtc.now();
        int currentAbsMonth = now.year() * 12 + now.month();
        int targetAbsMonth = currentYear * 12 + currentMonth;
        monthOffsetCount = targetAbsMonth - currentAbsMonth;

        return;
      }
    }
  }
  else {
    int intDay = stringToPositiveInt(command);
    DateTime now = rtc.now();
    if (intDay == -1 || intDay > daysInMonth(currentMonth, currentYear)) {
      oledWord("Invalid");
      delay(500);
      return;
    }
    else {
      // go to day
    }
  }
}

int checkEvents(String YYYYMMDD, bool countOnly = false) {
  int eventCount = 0;

  // Load events array from file
  updateEventArray();

  // Validate date string format
  if (YYYYMMDD.length() != 8) return -1;

  // Extrapolate the year, month, and day.
  int year  = YYYYMMDD.substring(0, 4).toInt();
  int month = YYYYMMDD.substring(4, 6).toInt();
  int day   = YYYYMMDD.substring(6, 8).toInt();

  // Determine day of the week
  DateTime dt(year, month, day); // from RTClib
  const char* daysOfWeek[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
  const char* monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  String weekday   = String(daysOfWeek[dt.dayOfTheWeek()]);
  String dayStr    = String(day);
  String monthName = String(monthNames[month - 1]);
  String dateCode  = monthName + (day < 10 ? "0" + dayStr : dayStr);  // e.g., "Apr05"

  // Determine nth weekday in the month (e.g., 1st Mon, 2nd Tue)
  int weekdayIndex = dt.dayOfTheWeek(); // 0 = Sunday
  int nthWeekday = ((day - 1) / 7) + 1;  // 1-based week count

  dayEvents.clear(); // Clear previous day's events

  // Check whether any repeat events happen on this day
  // Repeat Codes:
  // DAILY (every day)
  // WEEKLY Mo (every monday) or WEEKLY MoWeFr (every Mon, Wed, and Fri) or WEEKLY TuTh or WEEKLY_SaSu
  // MONTHLY 23 (23rd of each month) or MONTHLY 1Tu or MONTHLY 2We (first Tues or 2nd Wed, etc.)
  // YEARLY APR22 (yearly on Apr 22nd)

  for (size_t i = 0; i < calendarEvents.size(); i++) {
    String dateCode = calendarEvents[i][1];
    String repeatCode = calendarEvents[i][4];

    // Exact match for this date
    if (dateCode == YYYYMMDD) {
      if (!countOnly) dayEvents.push_back(calendarEvents[i]);
      eventCount++;
      continue;
    }

    // DAILY (every day)
    // Add all daily events
    if (repeatCode == "DAILY") {
      if (!countOnly) dayEvents.push_back(calendarEvents[i]);
      eventCount++;
      continue;
    }

    // WEEKLY [weekday]
    // Add weekly event if it occurs on the same day as current day
    if (repeatCode.startsWith("WEEKLY ") && repeatCode.indexOf(weekday) != -1) {
      if (!countOnly) dayEvents.push_back(calendarEvents[i]);
      eventCount++;
      continue;
    }

    // MONTHLY [date] or 1Tu, 2We etc.
    // Add monthly event if it occurs on the same date or ordinal weekday
    if (repeatCode.startsWith("MONTHLY ")) {
      String monthlyCode = repeatCode.substring(8); // Remove "MONTHLY "

      // Match numeric date only (e.g., "23")
      if (monthlyCode == dayStr) {
        if (!countOnly) dayEvents.push_back(calendarEvents[i]);
        eventCount++;
        continue;
      }

      // Match ordinal weekday (e.g., "1Mo", "2Tu", etc.)
      if (monthlyCode.length() == 3) {
        char nthChar = monthlyCode.charAt(0);
        String codeWeekday = monthlyCode.substring(1);

        if (nthChar >= '1' && nthChar <= '5') {
          int nth = nthChar - '0';
          if (nth == nthWeekday && codeWeekday == weekday) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            continue;
          }
        }
      }
    }

    // YEARLY [month][date]
    // Add yearly event if it occurs on the same date as current date
    if (repeatCode.startsWith("YEARLY ")) {
      String yearlyCode = repeatCode.substring(7); // Remove "YEARLY "
      if (yearlyCode.equalsIgnoreCase(dateCode)) {
        if (!countOnly) dayEvents.push_back(calendarEvents[i]);
        eventCount++;
        continue;
      }
    }
  }

  // Sort events by time if required
  if (!countOnly) {
    std::sort(dayEvents.begin(), dayEvents.end(), [](const std::vector<String>& a, const std::vector<String>& b) {
      // Time is stored in index 2 as "HHMM"
      return a[2] < b[2];
    });
  }

  return eventCount;
}

void drawCalendarMonth(int monthOffset) {
  int GRID_X =  7;     // X offset of first cell
  int GRID_Y = 49;     // Y offset of first row
  int CELL_W = 44;     // Width of each cell
  int CELL_H = 27;     // Height of each cell

  DateTime now = rtc.now();

  // Step 1: Calculate target month/year
  int month = now.month() + monthOffset;
  int year = now.year();
  while (month > 12) { month -= 12; year++; }
  while (month < 1)  { month += 12; year--; }

  currentMonth = month;
  currentYear = year;

  // Draw Background
  drawStatusBar(getMonthName(currentMonth) + " " + String(currentYear)+ " | Type a Date:");
  display.drawBitmap(0, 0, calendar_allArray[1], 320, 218, GxEPD_BLACK);

  // Step 2: Day of the week for the 1st of the month (0 = Sun, 6 = Sat)
  DateTime firstDay(year, month, 1);
  int startDay = firstDay.dayOfTheWeek();  // 0–6, Sun to Sat

  // Step 3: Number of days in the month
  int daysInMonth = (DateTime(year, month + 1, 1) - DateTime(year, month, 1)).days();

  // Step 4: Blank out leading days
  for (int i = 0; i < startDay; ++i) {
    int x = GRID_X + i * CELL_W;
    int y = GRID_Y;
    display.fillRect(x, y, CELL_W, CELL_H, GxEPD_WHITE);
  }

  // Step 5: Blank out trailing days
  int totalBoxes = 42;  // 7x6 grid
  int trailingStart = startDay + daysInMonth;
  for (int i = trailingStart; i < totalBoxes; ++i) {
    int row = i / 7;
    int col = i % 7;
    int x = GRID_X + col * CELL_W;
    int y = GRID_Y + row * CELL_H;
    display.fillRect(x, y, CELL_W, CELL_H, GxEPD_WHITE);
  }

  // Step 6: Draw day numbers and events
  for (int i = 0; i < daysInMonth; ++i) {
    int dayIndex = i + startDay;     // total box index in the 7x6 grid
    int row = dayIndex / 7;
    int col = dayIndex % 7;

    int x = GRID_X + col * CELL_W;
    int y = GRID_Y + row * CELL_H;

    int dayNum = i + 1;  // 1-based day number

    // Current day
    if (dayNum == now.day() && monthOffset == 0) {
      display.setFont(&FreeSerifBold9pt7b);
    }
    else display.setFont(&FreeSerif9pt7b);
    
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x + 6, y + 15); 
    display.print(dayNum);

    // Draw icon if there are events on day
    String YYYYMMDD = String(year) + String(month) + String(dayNum);
    int numEvents = checkEvents(YYYYMMDD, true);
    // Events found
    if (numEvents > 1) {
      // More than 1 event
      display.drawBitmap(x + 31, y + 9, _eventMarker1, 10, 10, GxEPD_BLACK);
    }
    else if (numEvents > 0) {
      // One event exists
      display.drawBitmap(x + 31, y + 9, _eventMarker0, 10, 10, GxEPD_BLACK);
    }
  }
}

void drawCalendarWeek(int weekOffset) {
  drawStatusBar("Type a Day: (Mon,Tue,etc)");
  display.drawBitmap(0, 0, calendar_allArray[0], 320, 218, GxEPD_BLACK);

  
  
}

// Loops
void processKB_CALENDAR() {
  int currentMillis = millis();
  DateTime now = rtc.now();

  switch (CurrentCalendarState) {
    case MONTH:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        // HOME Recieved
        else if (inchar == 12) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }  
        //CR Recieved
        else if (inchar == 13) {                          
          commandSelectMonth(currentLine);
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
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // LEFT Recieved
        else if (inchar == 19) {
          monthOffsetCount--;
          newState = true;
        }
        // RIGHT Recieved
        else if (inchar == 21) {
          monthOffsetCount++;
          newState = true;
        }
        // CENTER Recieved
        else if (inchar == 20 || inchar == 7) {
          CurrentCalendarState = WEEK;
          CurrentKBState  = NORMAL;
          newState = true;
          delay(200);
          break;
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
    case WEEK:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        // HOME Recieved
        else if (inchar == 12) {
          CurrentAppState = HOME;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }  
        //CR Recieved
        else if (inchar == 13) {                          
          commandSelectMonth(currentLine);
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
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // LEFT Recieved
        else if (inchar == 19) {
          weekOffsetCount--;
          newState = true;
        }
        // RIGHT Recieved
        else if (inchar == 21) {
          weekOffsetCount++;
          newState = true;
        }
        // CENTER Recieved
        else if (inchar == 20 || inchar == 7) {
          CurrentCalendarState = MONTH;
          CurrentKBState  = NORMAL;
          newState = true;
          delay(200);
          break;
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
  }
}

void einkHandler_CALENDAR() {
  switch (CurrentCalendarState) {
    case WEEK:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawCalendarWeek(weekOffsetCount);

        forceSlowFullUpdate = true;
        refresh();
        //multiPassRefesh(2);
      }
      break;
    case MONTH:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawCalendarMonth(monthOffsetCount);

        forceSlowFullUpdate = true;
        refresh();
        //multiPassRefesh(2);
      }
      break;
  }
}