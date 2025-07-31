#include "globals.h"

int monthOffsetCount = 0;
int weekOffsetCount = 0;

int currentDate = 0;
int currentMonth = 0;
int currentYear = 0;

// New Event
int newEventState = 0;
int editingEventIndex = 0;
String newEventName = "";
String newEventStartDate = "";
String newEventStartTime = "";
String newEventDuration = "";
String newEventRepeat = "";
String newEventNote = "";

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

void deleteEventByIndex(int indexToDelete) {
  if (indexToDelete >= 0 && indexToDelete < dayEvents.size()) {
    std::vector<String> targetEvent = dayEvents[indexToDelete];

    // Remove from dayEvents
    dayEvents.erase(dayEvents.begin() + indexToDelete);

    // Remove matching event from calendarEvents
    for (int i = 0; i < calendarEvents.size(); i++) {
      if (calendarEvents[i] == targetEvent) {
        calendarEvents.erase(calendarEvents.begin() + i);
        break;  // Only remove the first match
      }
    }
  }
}

void updateEventByIndex(int indexToUpdate) {
  if (indexToUpdate >= 0 && indexToUpdate < dayEvents.size()) {
    std::vector<String> oldEvent = dayEvents[indexToUpdate];

    // New event data
    std::vector<String> updatedEvent = {
      newEventName,
      newEventStartDate,
      newEventStartTime,
      newEventDuration,
      newEventRepeat,
      newEventNote
    };

    // Update dayEvents
    dayEvents[indexToUpdate] = updatedEvent;

    // Find and update matching event in calendarEvents
    for (int i = 0; i < calendarEvents.size(); i++) {
      if (calendarEvents[i] == oldEvent) {
        calendarEvents[i] = updatedEvent;
        break;  // Stop after first match
      }
    }
  }
}

// General Functions
String intToYYYYMMDD(int year_, int month_, int date_) {
  String y = String(year_);
  String m = (month_ < 10 ? "0" : "") + String(month_);
  String d = (date_ < 10 ? "0" : "") + String(date_);
  return y + m + d;
}

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

int getDayOfWeek(int year, int month, int day) {
  if (month < 3) {
    month += 12;
    year -= 1;
  }

  int K = year % 100;
  int J = year / 100;

  int h = (day + 13*(month + 1)/5 + K + K/4 + J/4 + 5*J) % 7;

  // Convert Zeller’s output to: 0 = Sunday, ..., 6 = Saturday
  int d = (h + 6) % 7;
  return d;
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

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize Stuff
    newEventState = 0;
    newEventName = "";
    newEventStartDate = "";
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine     = "";

    newState        = true;
    CurrentKBState  = NORMAL;
    return;
  }

  // Check if command starts with a 3-letter month
  else if (command.length() >= 4) {
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

  // Check if command is in YYYYMMDD format
  else if (command.length() == 8 && stringToPositiveInt(command) != -1) {
    int year = command.substring(0, 4).toInt();
    int month = command.substring(4, 6).toInt();
    int date = command.substring(6, 8).toInt();

    if (year < 1970 || year > 2200 || month < 1 || month > 12 || date < 1 || date > daysInMonth(month, year)) {
      oledWord("Invalid");
      delay(500);
      return;
    }

    currentYear = year;
    currentMonth = month;
    currentDate = date;

    DateTime now = rtc.now();
    int currentAbsMonth = now.year() * 12 + now.month();
    int targetAbsMonth = currentYear * 12 + currentMonth;
    monthOffsetCount = targetAbsMonth - currentAbsMonth;

    int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);

    switch (dayOfWeek) {
      case 0:
        CurrentCalendarState = SUN;
        break;
      case 1:
        CurrentCalendarState = MON;
        break;
      case 2:
        CurrentCalendarState = TUE;
        break;
      case 3:
        CurrentCalendarState = WED;
        break;
      case 4:
        CurrentCalendarState = THU;
        break;
      case 5:
        CurrentCalendarState = FRI;
        break;
      case 6:
        CurrentCalendarState = SAT;
        break;
    }

    newState        = true;
    CurrentKBState  = NORMAL;
    return;
  }

  // Check if user entered a numeric day (for current month)
  else {
    int intDay = stringToPositiveInt(command);
    DateTime now = rtc.now();
    if (intDay == -1 || intDay > daysInMonth(currentMonth, currentYear)) {
      oledWord("Invalid");
      delay(500);
      return;
    }
    else {
      currentDate = intDay;

      int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);

      switch (dayOfWeek) {
        case 0:
          CurrentCalendarState = SUN;
          break;
        case 1:
          CurrentCalendarState = MON;
          break;
        case 2:
          CurrentCalendarState = TUE;
          break;
        case 3:
          CurrentCalendarState = WED;
          break;
        case 4:
          CurrentCalendarState = THU;
          break;
        case 5:
          CurrentCalendarState = FRI;
          break;
        case 6:
          CurrentCalendarState = SAT;
          break;
      }

      newState        = true;
      CurrentKBState  = NORMAL;
      return;
    }
  }
}

void commandSelectWeek(String command) {
  command.toLowerCase();

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize Stuff
    newEventState = 0;
    newEventName = "";
    newEventStartDate = "";
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine     = "";

    newState        = true;
    CurrentKBState  = NORMAL;
    return;
  }
  // Commands for each day
  else if (command == "sun" || command == "su") {
    CurrentCalendarState = SUN;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day()); 
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedSunday = currentSunday + TimeSpan(weekOffsetCount * 7, 0, 0, 0);

    currentDate  = viewedSunday.day();
    currentMonth = viewedSunday.month();
    currentYear  = viewedSunday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "mon" || command == "mo") {
    CurrentCalendarState = MON;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedMonday = currentSunday + TimeSpan(weekOffsetCount * 7 + 1, 0, 0, 0);

    currentDate  = viewedMonday.day();
    currentMonth = viewedMonday.month();
    currentYear  = viewedMonday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "tue" || command == "tu") {
    CurrentCalendarState = TUE;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedTuesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 2, 0, 0, 0);

    currentDate  = viewedTuesday.day();
    currentMonth = viewedTuesday.month();
    currentYear  = viewedTuesday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "wed" || command == "we") {
    CurrentCalendarState = WED;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedWednesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 3, 0, 0, 0);

    currentDate  = viewedWednesday.day();
    currentMonth = viewedWednesday.month();
    currentYear  = viewedWednesday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "thu" || command == "th") {
    CurrentCalendarState = THU;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedThursday = currentSunday + TimeSpan(weekOffsetCount * 7 + 4, 0, 0, 0);

    currentDate  = viewedThursday.day();
    currentMonth = viewedThursday.month();
    currentYear  = viewedThursday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "fri" || command == "fr") {
    CurrentCalendarState = FRI;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedFriday = currentSunday + TimeSpan(weekOffsetCount * 7 + 5, 0, 0, 0);

    currentDate  = viewedFriday.day();
    currentMonth = viewedFriday.month();
    currentYear  = viewedFriday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }

  else if (command == "sat" || command == "sa") {
    CurrentCalendarState = SAT;

    DateTime now = rtc.now();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedSaturday = currentSunday + TimeSpan(weekOffsetCount * 7 + 6, 0, 0, 0);

    currentDate  = viewedSaturday.day();
    currentMonth = viewedSaturday.month();
    currentYear  = viewedSaturday.year();

    newState = true;
    CurrentKBState = NORMAL;
  }
}

void commandSelectDay(String command) {
  command.toLowerCase();

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize new blank event
    newEventState     = 0;
    newEventName      = "";
    newEventStartDate = intToYYYYMMDD(currentYear, currentMonth, currentDate);
    newEventStartTime = "";
    newEventDuration  = "";
    newEventRepeat    = "";
    newEventNote      = "";
    currentLine       = "";

    newState          = true;
    CurrentKBState    = NORMAL;
    return;
  }

  // Check if the command is a single digit referring to a specific event
  if (command.length() == 1 && isDigit(command.charAt(0))) {
    int index = command.toInt() - 1;

    if (index >= 0 && index < dayEvents.size()) {
      std::vector<String>& evt = dayEvents[index];

      editingEventIndex = index;
      newEventState     = -1;
      newEventName      = evt[0];
      newEventStartDate = evt[1];
      newEventStartTime = evt[2];
      newEventDuration  = evt[3];
      newEventRepeat    = evt[4];
      newEventNote      = evt[5];
      currentLine       = "";

      CurrentCalendarState = VIEW_EVENT;
      CurrentKBState       = NORMAL;
      newState             = true;
    }
  }
}

int checkEvents(String YYYYMMDD, bool countOnly = false) {
  int eventCount = 0;

  // Load events array from file
  updateEventArray();

  // Return -1 if input format is invalid
  if (YYYYMMDD.length() != 8) return -1;

  // Convert input to DateTime
  int year  = YYYYMMDD.substring(0, 4).toInt();
  int month = YYYYMMDD.substring(4, 6).toInt();
  int day   = YYYYMMDD.substring(6, 8).toInt();
  DateTime dt(year, month, day);

  // Define helper strings
  const char* daysOfWeek[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
  const char* monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  String weekday = String(daysOfWeek[dt.dayOfTheWeek()]);
  String weekdayUpper = weekday;
  weekdayUpper.toUpperCase();

  String dayStr = String(day);
  String monthName = String(monthNames[month - 1]);
  String dateCode = monthName + (day < 10 ? "0" + dayStr : dayStr);
  dateCode.toUpperCase();

  int weekdayIndex = dt.dayOfTheWeek();  // 0 = Sunday
  int nthWeekday = ((day - 1) / 7) + 1;

  dayEvents.clear();  // Clear previous day's events

  // Check whether any repeat events happen on this day
  for (size_t i = 0; i < calendarEvents.size(); i++) {
    String eventDate = calendarEvents[i][1];
    String eventTime = calendarEvents[i][2];
    String repeatCode = calendarEvents[i][4];

    // Direct match
    if (eventDate == YYYYMMDD) {
      if (!countOnly) dayEvents.push_back(calendarEvents[i]);
      eventCount++;
      continue;
    }

    // Handle repeating events
    if (repeatCode != "NO") {
      repeatCode.toUpperCase();

      // Skip repeat if date is before original event date
      if (eventDate.length() == 8 && YYYYMMDD < eventDate) continue;

      // DAILY
      if (repeatCode == "DAILY") {
        if (!countOnly) dayEvents.push_back(calendarEvents[i]);
        eventCount++;
        continue;
      }

      // WEEKLY SU, MOWEFR, etc.
      if (repeatCode.startsWith("WEEKLY ")) {
        String days = repeatCode.substring(7);
        days.trim();

        for (int j = 0; j + 1 < days.length(); j += 2) {
          String codeDay = days.substring(j, j + 2);
          if (codeDay == weekdayUpper) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            break;
          }
        }
        continue;
      }

      // MONTHLY 10 or 2Tu
      if (repeatCode.startsWith("MONTHLY ")) {
        String monthlyCode = repeatCode.substring(8);

        // Monthly on specific date (e.g. 10)
        if (monthlyCode == dayStr) {
          if (!countOnly) dayEvents.push_back(calendarEvents[i]);
          eventCount++;
          continue;
        }

        // Monthly on ordinal weekday (e.g. 2Tu)
        if (monthlyCode.length() == 3) {
          int nth = monthlyCode.charAt(0) - '0';
          String codeWeekday = monthlyCode.substring(1);
          codeWeekday.toUpperCase();

          if (nth == nthWeekday && codeWeekday == weekdayUpper) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            continue;
          }
        }
      }

      // YEARLY Apr22
      if (repeatCode.startsWith("YEARLY ")) {
        String yearlyCode = repeatCode.substring(7);
        yearlyCode.toUpperCase();
        if (yearlyCode == dateCode) {
          if (!countOnly) dayEvents.push_back(calendarEvents[i]);
          eventCount++;
          continue;
        }
      }
    }
  }

  // Sort by start time (HH:MM to minutes)
  if (!countOnly) {
    std::sort(dayEvents.begin(), dayEvents.end(), [](const std::vector<String>& a, const std::vector<String>& b) {
      String aTime = a[2];
      String bTime = b[2];

      int aMin = aTime.substring(0, 2).toInt() * 60 + aTime.substring(3, 5).toInt();
      int bMin = bTime.substring(0, 2).toInt() * 60 + bTime.substring(3, 5).toInt();

      return aMin < bMin;
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

    String YYYYMMDD = intToYYYYMMDD(year, month, dayNum);
    // Pad month and dayNum with leading zeros
    /*String paddedMonth = (month < 10 ? "0" : "") + String(month);
    String paddedDay   = (dayNum < 10 ? "0" : "") + String(dayNum);

    // Format date as YYYYMMDD
    String YYYYMMDD = String(year) + paddedMonth + paddedDay;*/

    int numEvents = checkEvents(YYYYMMDD, true);

    // Events found
    if (numEvents > 2) {
      display.setFont(&Font5x7Fixed);
      display.setCursor(x + 32, y + 16);
      display.print(String(numEvents));
    }
    else if (numEvents > 1) {
      // More than 1 event
      display.drawBitmap(x + 29, y + 8, _eventMarker1, 10, 10, GxEPD_BLACK);
    }
    else if (numEvents > 0) {
      // One event exists
      display.drawBitmap(x + 29, y + 8, _eventMarker0, 10, 10, GxEPD_BLACK);
    }
  }
}

void drawCalendarWeek(int weekOffset) {
  drawStatusBar("Type Sun, etc. or (N)ew");
  display.drawBitmap(0, 0, calendar_allArray[0], 320, 218, GxEPD_BLACK);

  // Get current date
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int dow = now.dayOfTheWeek();  // 0 = Sunday

  // Calculate how many days to go back to get to Sunday, adjusted by weekOffset
  int totalOffset = -dow + (weekOffset * 7);

  for (int i = 0; i < 7; i++) {
    // Compute day offset from today
    int offset = totalOffset + i;

    // Convert (year, month, day + offset) into a new date
    int y = year;
    int m = month;
    int d = day + offset;

    // Normalize date forward/backward
    while (d <= 0) {
      m--;
      if (m < 1) {
        m = 12;
        y--;
      }
      d += daysInMonth(m, y);
    }
    while (d > daysInMonth(m, y)) {
      d -= daysInMonth(m, y);
      m++;
      if (m > 12) {
        m = 1;
        y++;
      }
    }

    // Format YYYYMMDD
    String YYYYMMDD = intToYYYYMMDD(y, m, d);
    /*String YYYYMMDD = String(y) +
                      (m < 10 ? "0" : "") + String(m) +
                      (d < 10 ? "0" : "") + String(d);*/

    // Draw date
    display.setFont(&FreeSerif9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(9 + (i * 44), 62);
    String dateStr = String(m) + "/" + String(d);
    display.print(dateStr);

    // Load and draw events
    int eventCount = checkEvents(YYYYMMDD, false);
    if (eventCount > 6) eventCount = 6;

    // Blank out extra space
    display.fillRect(9 + (i * 44), 71 + (eventCount * 23), 39, ((6 - eventCount) * 23), GxEPD_WHITE);

    for (int j = 0; j < eventCount; j++) {
      String startTime = dayEvents[j][2];
      // Indicator for repeat events
      if (dayEvents[j][4] != "NO") startTime = ":: " + startTime;
      String eventName = dayEvents[j][0].substring(0, 6);

      // Print Start Time
      display.setFont(&Font3x7FixedNum);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(12 + (i * 44), 80 + (j * 23));
      display.print(startTime);

      // Print Event Name
      display.setFont(&Font5x7Fixed);
      display.setCursor(12 + (i * 44), 89 + (j * 23));
      display.print(eventName);
    }
  }
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
          //commandSelectMonth(currentLine);
          commandSelectWeek(currentLine);
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
    case NEW_EVENT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        // HOME Recieved
        else if (inchar == 12) {
          newEventState--;
          currentLine = "";
          if (newEventState < 0) {
            CurrentCalendarState = MONTH;
            currentLine     = "";
            newState        = true;
            CurrentKBState  = NORMAL;
          }
        }  
        //CR Recieved
        else if (inchar == 13) {                          
          switch (newEventState) {
            case 0:
              // Event Name: must be non-empty
              if (currentLine.length() > 0) {
                newEventName = currentLine;
                newEventState++;
                currentLine = newEventStartDate;
              } else {
                oledWord("Error: Empty event name");
                delay(2000);
                currentLine = "";
              }
              break;

            case 1:
              // Start Date: must be YYYYMMDD (8-digit number)
              if (currentLine.length() == 8 && currentLine.toInt() > 10000000) {
                newEventStartDate = currentLine;
                newEventState++;
                currentLine = "";
              } else {
                oledWord("Error: Invalid date (YYYYMMDD)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 2:
              // Start Time: must be HH:MM
              if (currentLine.length() == 5 && currentLine.charAt(2) == ':' &&
                  isDigit(currentLine.charAt(0)) && isDigit(currentLine.charAt(1)) &&
                  isDigit(currentLine.charAt(3)) && isDigit(currentLine.charAt(4))) {
                newEventStartTime = currentLine;
                newEventState++;
                currentLine = "";
              } else {
                oledWord("Error: Invalid time (HH:MM)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 3:
              // Duration: must be H:MM or HH:MM
              {
                int colonIdx = currentLine.indexOf(':');
                if ((colonIdx == 1 || colonIdx == 2) &&
                    isDigit(currentLine.charAt(0)) &&
                    isDigit(currentLine.charAt(colonIdx + 1)) &&
                    isDigit(currentLine.charAt(colonIdx + 2))) {
                  newEventDuration = currentLine;
                  newEventState++;
                  currentLine = "";
                } else {
                  oledWord("Error: Invalid duration (H:MM)");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 4:
              // Repeat: must be NO, DAILY, WEEKLY xx, MONTHLY xx, or YEARLY xx
              {
                String code = currentLine;
                code.toUpperCase();
                if (code == "HELP") {
                  // Display help screen here
                  oledWord("Help screen coming soon!");
                  delay(5000);
                  currentLine = "";
                } else if (code == "NO" || code == "DAILY" ||
                    code.startsWith("WEEKLY ") ||
                    code.startsWith("MONTHLY ") ||
                    code.startsWith("YEARLY ")) {
                  newEventRepeat = code;
                  newEventState++;
                  currentLine = "";
                } else {
                  oledWord("Error: Invalid repeat value");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 5:
              // Note: no restrictions
              newEventNote = currentLine;
              newEventState++;
              currentLine = "";
              break;
          }

          if (newEventState > 5) {
            // Create Event
            addEvent( 
                      newEventName, 
                      newEventStartDate, 
                      newEventStartTime, 
                      newEventDuration, 
                      newEventRepeat, 
                      newEventNote 
                    );
            // Return to app
            oledWord("New Event \"" + newEventName + "\" Created");
            delay(2000);
            CurrentCalendarState = MONTH;
            CurrentKBState = NORMAL;
          }
          newState = true;
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
          switch(newEventState) {
            case 0:
              oledLine(currentLine, false, "Enter the Event Name");
              break;
            case 1:
              oledLine(currentLine, false, "Enter the Start Date (YYYYMMDD)");
              break;
            case 2:
              oledLine(currentLine, false, "Enter the Start Time (HH:MM)");
              break;
            case 3:
              oledLine(currentLine, false, "Enter the Event Duration (HH:MM)");
              break;
            case 4:
              oledLine(currentLine, false, "Enter the Repeat Code or \"Help\"");
              break;
            case 5:
              oledLine(currentLine, false, "Attach a Note to the Event");
              break;
          }
        }
      }
      break;
    case VIEW_EVENT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        // HOME Recieved
        else if (inchar == 12) {
          CurrentCalendarState = MONTH;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }  
        //CR Recieved
        else if (inchar == 13) {                          
          switch (newEventState) {
            case -1:
              if (currentLine == "1") {
                newEventState = 0;
              }
              else if (currentLine == "2") {
                newEventState = 1;
              }
              else if (currentLine == "3") {
                newEventState = 2;
              }
              else if (currentLine == "4") {
                newEventState = 3;
              }
              else if (currentLine == "5") {
                newEventState = 4;
              }
              else if (currentLine == "6") {
                newEventState = 5;
              }
              else if (currentLine == "d" || currentLine == "D") {
                deleteEventByIndex(editingEventIndex);
                updateEventsFile();
                oledWord("Event : \"" + newEventName + "\" Deleted");
                delay(2000);
                CurrentCalendarState = MONTH;
                currentLine     = "";
                newState        = true;
                CurrentKBState  = NORMAL;
              }
              else if (currentLine == "s" || currentLine == "S") {
                updateEventByIndex(editingEventIndex);
                updateEventsFile();
                oledWord("Event : \"" + newEventName + "\" Edited");
                delay(2000);
                CurrentCalendarState = MONTH;
                currentLine     = "";
                newState        = true;
                CurrentKBState  = NORMAL;
              }
              currentLine = "";
              break;
            case 0:
              // Event Name: must be non-empty
              if (currentLine.length() > 0) {
                newEventName = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                oledWord("Error: Empty event name");
                delay(2000);
                currentLine = "";
              }
              break;

            case 1:
              // Start Date: must be YYYYMMDD (8-digit number)
              if (currentLine.length() == 8 && currentLine.toInt() > 10000000) {
                newEventStartDate = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                oledWord("Error: Invalid date (YYYYMMDD)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 2:
              // Start Time: must be HH:MM
              if (currentLine.length() == 5 && currentLine.charAt(2) == ':' &&
                  isDigit(currentLine.charAt(0)) && isDigit(currentLine.charAt(1)) &&
                  isDigit(currentLine.charAt(3)) && isDigit(currentLine.charAt(4))) {
                newEventStartTime = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                oledWord("Error: Invalid time (HH:MM)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 3:
              // Duration: must be H:MM or HH:MM
              {
                int colonIdx = currentLine.indexOf(':');
                if ((colonIdx == 1 || colonIdx == 2) &&
                    isDigit(currentLine.charAt(0)) &&
                    isDigit(currentLine.charAt(colonIdx + 1)) &&
                    isDigit(currentLine.charAt(colonIdx + 2))) {
                  newEventDuration = currentLine;
                  currentLine = "";
                  newEventState = -1;
                } else {
                  oledWord("Error: Invalid duration (H:MM)");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 4:
              // Repeat: must be NO, DAILY, WEEKLY xx, MONTHLY xx, or YEARLY xx
              {
                String code = currentLine;
                code.toUpperCase();
                if (code == "HELP") {
                  // Display help screen here
                  oledWord("Help screen coming soon!");
                  delay(5000);
                  currentLine = "";
                } else if (code == "NO" || code == "DAILY" ||
                    code.startsWith("WEEKLY ") ||
                    code.startsWith("MONTHLY ") ||
                    code.startsWith("YEARLY ")) {
                  newEventRepeat = code;
                  currentLine = "";
                  newEventState = -1;
                } else {
                  oledWord("Error: Invalid repeat value");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 5:
              // Note: no restrictions
              newEventNote = currentLine;
              currentLine = "";
              newEventState = -1;
              break;
          }

          if (newEventState > 5) {
            // Create Event
            addEvent( 
                      newEventName, 
                      newEventStartDate, 
                      newEventStartTime, 
                      newEventDuration, 
                      newEventRepeat, 
                      newEventNote 
                    );
            // Return to app
            oledWord("New Event \"" + newEventName + "\" Created");
            delay(2000);
            CurrentCalendarState = MONTH;
            CurrentKBState = NORMAL;
          }
          newState = true;
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
          switch(newEventState) {
            case -1:
              oledLine(currentLine, false);
              break;
            case 0:
              oledLine(currentLine, false, "Enter the Event Name");
              break;
            case 1:
              oledLine(currentLine, false, "Enter the Start Date (YYYYMMDD)");
              break;
            case 2:
              oledLine(currentLine, false, "Enter the Start Time (HH:MM)");
              break;
            case 3:
              oledLine(currentLine, false, "Enter the Event Duration (HH:MM)");
              break;
            case 4:
              oledLine(currentLine, false, "Enter the Repeat Code or \"Help\"");
              break;
            case 5:
              oledLine(currentLine, false, "Attach a Note to the Event");
              break;
          }
        }
      }
      break;
    case SUN:
    case MON:
    case TUE:
    case WED:
    case THU:
    case FRI:
    case SAT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);  
        // HOME Recieved
        else if (inchar == 12) {
          CurrentCalendarState = MONTH;
          currentLine     = "";
          newState        = true;
          CurrentKBState  = NORMAL;
        }  
        //CR Recieved
        else if (inchar == 13) {                          
          commandSelectDay(currentLine);
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
        
        // LEFT Received
        else if (inchar == 19) {
          // Go back one day
          currentDate--;
          if (currentDate < 1) {
            currentMonth--;
            if (currentMonth < 1) {
              currentMonth = 12;
              currentYear--;
            }
            currentDate = daysInMonth(currentMonth, currentYear);
          }

          int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);
          switch (dayOfWeek) {
            case 0: CurrentCalendarState = SUN; break;
            case 1: CurrentCalendarState = MON; break;
            case 2: CurrentCalendarState = TUE; break;
            case 3: CurrentCalendarState = WED; break;
            case 4: CurrentCalendarState = THU; break;
            case 5: CurrentCalendarState = FRI; break;
            case 6: CurrentCalendarState = SAT; break;
          }

          newState = true;
        }

        // RIGHT Received
        else if (inchar == 21) {
          // Go forward one day
          int daysThisMonth = daysInMonth(currentMonth, currentYear);
          currentDate++;
          if (currentDate > daysThisMonth) {
            currentDate = 1;
            currentMonth++;
            if (currentMonth > 12) {
              currentMonth = 1;
              currentYear++;
            }
          }

          int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);
          switch (dayOfWeek) {
            case 0: CurrentCalendarState = SUN; break;
            case 1: CurrentCalendarState = MON; break;
            case 2: CurrentCalendarState = TUE; break;
            case 3: CurrentCalendarState = WED; break;
            case 4: CurrentCalendarState = THU; break;
            case 5: CurrentCalendarState = FRI; break;
            case 6: CurrentCalendarState = SAT; break;
          }

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
    case NEW_EVENT:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        display.drawBitmap(0, 0, calendar_allArray[2], 320, 218, GxEPD_BLACK);

        display.setFont(&FreeSerif9pt7b);

        display.setCursor(106, 68);
        display.print(newEventName);

        display.setCursor(106, 90);
        display.print(newEventStartDate);

        display.setCursor(106, 112);
        display.print(newEventStartTime);

        display.setCursor(106, 134);
        display.print(newEventDuration);
        
        display.setCursor(106, 156);
        display.print(newEventRepeat);

        display.setCursor(106, 178);
        display.print(newEventNote);

        forceSlowFullUpdate = true;
        refresh();
      }
      break;
    case VIEW_EVENT:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        switch(newEventState) {
          case -1:
            drawStatusBar("Type 1-6,(D)elete,or (S)ave");
            break;
          default:
            drawStatusBar("Type the info!");
            break;
        }
        display.drawBitmap(0, 0, calendar_allArray[3], 320, 218, GxEPD_BLACK);

        display.setFont(&FreeSerif9pt7b);

        display.setCursor(106, 68);
        display.print(newEventName);

        display.setCursor(106, 90);
        display.print(newEventStartDate);

        display.setCursor(106, 112);
        display.print(newEventStartTime);

        display.setCursor(106, 134);
        display.print(newEventDuration);
        
        display.setCursor(106, 156);
        display.print(newEventRepeat);

        display.setCursor(106, 178);
        display.print(newEventNote);

        forceSlowFullUpdate = true;
        refresh();
      }
      break;
    // All days use the same basic code
    case SUN:
    case MON:
    case TUE:
    case WED:
    case THU:
    case FRI:
    case SAT:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // Draw background
        // CurrentCalendarState enumerations somehow line up with calendar app bitmaps?
        // SUN = 4, SAT = 10
        drawStatusBar("Events 1-7 or (N)ew");
        display.drawBitmap(0, 0, calendar_allArray[CurrentCalendarState], 320, 218, GxEPD_BLACK);

        // Draw Date
        display.setFont(&FreeSerif9pt7b);
        display.setTextColor(GxEPD_BLACK);
        // Set cursor based on the day of the week
        display.setCursor(9 + (44*(CurrentCalendarState - 4)), 59);
        display.print(String(currentMonth) + "/" + String(currentDate));

        // Load events
        String YYYYMMDD = intToYYYYMMDD(currentYear, currentMonth, currentDate);
        int eventCount = checkEvents(YYYYMMDD, false);
        if (eventCount > 7) eventCount = 7;

        // Blank out extra space
        display.fillRect(12, 66 + (eventCount * 19), 297, ((7 - eventCount) * 19), GxEPD_WHITE);
        
        // Display events data
        for (int j = 0; j < eventCount; j++) {
          String name       = dayEvents[j][0];
          String startTime  = dayEvents[j][2];
          String duration   = dayEvents[j][3];
          String repeatCode = dayEvents[j][4];
          String bottomInfo = "Starts: " + startTime + ", Dur: " + duration + ", Rep: " + repeatCode;

          // Print event name
          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 74 + (j * 19));
          display.print(name);

          // Print bottom info
          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 82 + (j * 19));
          display.print(bottomInfo);
        }

        forceSlowFullUpdate = true;
        refresh();
        //multiPassRefesh(2);
      }
      break;
  }
}
