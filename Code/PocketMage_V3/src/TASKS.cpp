// ooooooooooooo       .o.        .oooooo..o oooo    oooo  .oooooo..o //
// 8'   888   `8      .888.      d8P'    `Y8 `888   .8P'  d8P'    `Y8 //
//      888          .8"888.     Y88bo.       888  d8'    Y88bo.      //
//      888         .8' `888.     `"Y8888o.   88888[       `"Y8888o.  //
//      888        .88ooo8888.        `"Y88b  888`88b.         `"Y88b //
//      888       .8'     `888.  oo     .d8P  888  `88b.  oo     .d8P //
//     o888o     o88o     o8888o 8""88888P'  o888o  o888o 8""88888P'  //  
#include "globals.h"                                                 

void sortTasksByDueDate(std::vector<std::vector<String>> &tasks) {
  std::sort(tasks.begin(), tasks.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; // Compare dueDate strings
  });
}

void addTask(String taskName, String dueDate, String priority, String completed) {
  String taskInfo = taskName+"|"+dueDate+"|"+priority+"|"+completed;
  updateTaskArray();
  tasks.push_back({taskName, dueDate, priority, completed});
  sortTasksByDueDate(tasks);
  updateTasksFile();
}

void updateTaskArray() {
  File file = SPIFFS.open("/tasks.txt", "r"); // Open the text file in read mode
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  tasks.clear(); // Clear the existing vector before loading the new data

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

    // Extract task name, due date, priority, and completed status
    String taskName  = line.substring(0, delimiterPos1);
    String dueDate   = line.substring(delimiterPos1 + 1, delimiterPos2);
    String priority  = line.substring(delimiterPos2 + 1, delimiterPos3);
    String completed = line.substring(delimiterPos3 + 1);

    // Add the task to the vector
    tasks.push_back({taskName, dueDate, priority, completed});
  }

  file.close();  // Close the file
}

void updateTasksFile() {
  // Clear the existing tasks file first
  delFile("/tasks.txt");

  // Iterate through the tasks vector and append each task to the file
  for (size_t i = 0; i < tasks.size(); i++) {
    // Create a string from the task's attributes with "|" delimiter
    String taskInfo = tasks[i][0] + "|" + tasks[i][1] + "|" + tasks[i][2] + "|" + tasks[i][3];
    
    // Append the task info to the file
    appendToFile("/tasks.txt", taskInfo);
  }
}

void deleteTask(int index) {
  if (index >= 0 && index < tasks.size()) {
    tasks.erase(tasks.begin() + index);
  }
}

String convertDateFormat(String yyyymmdd) {
  if (yyyymmdd.length() != 8) {
    Serial.println(("INVALID DATE: " + yyyymmdd).c_str());
    return "Invalid";
  }

  String year = yyyymmdd.substring(2, 4);  // Get last two digits of the year
  String month = yyyymmdd.substring(4, 6);
  String day = yyyymmdd.substring(6, 8);

  return month + "/" + day + "/" + year;
}

void processKB_TASKS() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }
  int currentMillis = millis();
  disableTimeout = false;
  char inchar;

  switch (CurrentTasksState) {
    case TASKS0:
      CurrentKBState = FUNC;
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentAppState = HOME;
          currentLine     = "";
          CurrentKBState  = NORMAL;
          CurrentHOMEState = HOME_HOME;
          newState = true;
          break;
        }
        // NEW TASK
        else if (inchar == '/' || inchar == 'n' || inchar == 'N') {
          CurrentTasksState = TASKS0_NEWTASK;
          CurrentKBState = NORMAL;
          newTaskState = 0;
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '0' && inchar <= '9') {
          int taskIndex = (inchar == '0') ? 10 : (inchar - '1');  // Adjust for 1-based input

          // SET SELECTED TASK
          if (taskIndex < tasks.size()) {
            selectedTask = taskIndex;
            // GO TO TASKS1
            CurrentTasksState = TASKS1;
            editTaskState = 0;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case TASKS0_NEWTASK:
      if (newTaskState == 1) CurrentKBState = FUNC;

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);                                        
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
        else if (inchar == 8 || inchar == 12) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          // ENTER INFORMATION BASED ON STATE
          switch (newTaskState) {
            case 0: // ENTER TASK NAME
              newTaskName = currentLine;
              currentLine = "";
              newTaskState = 1;
              newState = true;
              break;
            case 1: // ENTER DUE DATE
              String testDate = convertDateFormat(currentLine);
              // DATE IS VALID
              if (testDate != "Invalid") {
                newTaskDueDate = currentLine;

                // ADD NEW TASK
                addTask(newTaskName, newTaskDueDate, "0", "0");
                oledWord("New Task Added");
                delay(1000);

                // RETURN
                currentLine = "";
                newTaskState = 0;
                CurrentTasksState = TASKS0;
                newState = true;
              }
              // DATE IS INVALID
              else {
                oledWord("Invalid Date");
                delay(1000);
                currentLine = "";
              }
              break;
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
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledLine(currentLine, false);
        }
      }
      break;
    case TASKS1:
      disableTimeout = false;

      CurrentKBState = FUNC;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentTasksState = TASKS0;
          forceSlowFullUpdate = true;
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '1' && inchar <= '4') {
          if (inchar == '1') {      // RENAME TASK

          }
          else if (inchar == '2') { // CHANGE DUE DATE

          }
          else if (inchar == '3') { // DELETE TASK
            deleteTask(selectedTask);
            updateTasksFile();
            
            CurrentTasksState = TASKS0;
            forceSlowFullUpdate = true;
            newState = true;
          }
          else if (inchar == '4') { // COPY TASK

          }
          
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        KBBounceMillis = currentMillis;
      }
      break;
  
  }
}

void einkHandler_TASKS() {
  switch (CurrentTasksState) {
    case TASKS0:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

        // DRAW FILE LIST
        updateTaskArray();
        sortTasksByDueDate(tasks);

        if (!tasks.empty()) {
          if (DEBUG_VERBOSE) Serial.println("Printing Tasks");

          drawStatusBar("Select (0-9),New Task (N)");

          int loopCount = std::min((int)tasks.size(), MAX_FILES);
          for (int i = 0; i < loopCount; i++) {
            display.setFont(&FreeSerif9pt7b);
            // PRINT TASK NAME
            display.setCursor(29, 54 + (17 * i));
            display.print(tasks[i][0].c_str());
            // PRINT TASK DUE DATE
            display.setCursor(231, 54 + (17 * i));
            display.print(convertDateFormat(tasks[i][1]).c_str());
            Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
          }
        }
        else drawStatusBar("No Tasks! Add New Task (N)");

        refresh();
      }
      break;
      case TASKS0_NEWTASK:
        if (newState) {
          newState = false;
          display.setRotation(3);
          display.setFullWindow();
          display.fillScreen(GxEPD_WHITE);

          // DRAW APP
          display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

          // DRAW FILE LIST
          updateTaskArray();
          sortTasksByDueDate(tasks);

          if (!tasks.empty()) {
            if (DEBUG_VERBOSE) Serial.println("Printing Tasks");

            int loopCount = std::min((int)tasks.size(), MAX_FILES);
            for (int i = 0; i < loopCount; i++) {
              display.setFont(&FreeSerif9pt7b);
              // PRINT TASK NAME
              display.setCursor(29, 54 + (17 * i));
              display.print(tasks[i][0].c_str());
              // PRINT TASK DUE DATE
              display.setCursor(231, 54 + (17 * i));
              display.print(convertDateFormat(tasks[i][1]).c_str());
              Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
            }
          }
          switch (newTaskState) {
            case 0:
              drawStatusBar("Enter Task Name:");
              break;
            case 1:
              drawStatusBar("Due Date (YYYYMMDD):");
              break;
          }

          refresh();
        }
        break;
    case TASKS1:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("T:" + tasks[selectedTask][0]);
        display.drawBitmap(0, 0, tasksApp1, 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
    
  }
}