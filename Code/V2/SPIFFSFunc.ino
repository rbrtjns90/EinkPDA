//   .oooooo..o ooooooooo.   ooooo oooooooooooo oooooooooooo  .oooooo..o  //
//  d8P'    `Y8 `888   `Y88. `888' `888'     `8 `888'     `8 d8P'    `Y8  //
//  Y88bo.       888   .d88'  888   888          888         Y88bo.       //
//   `"Y8888o.   888ooo88P'   888   888oooo8     888oooo8     `"Y8888o.   //
//       `"Y88b  888          888   888    "     888    "         `"Y88b  //
//  oo     .d8P  888          888   888          888         oo     .d8P  //
//  8""88888P'  o888o        o888o o888o        o888o        8""88888P'   //

void listDir(fs::FS &fs, const char *dirname) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  // Reset fileIndex and initialize filesList with "-"
  fileIndex = 0; // Reset fileIndex
  for (int i = 0; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }

  File file = root.openNextFile();
  while (file && fileIndex < MAX_FILES) {
    if (!file.isDirectory()) {
      filesList[fileIndex++] = String(file.name()); // Store file name
    }
    file = root.openNextFile();
  }

  for (int i = 0; i < MAX_FILES; i++) {
    Serial.println(filesList[i]);
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

String readFileToString(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    oledWord("Load Failed");
    delay(500);
    return "";  // Return an empty string on failure
  }

  Serial.println("- reading from file:");
  String content = "";  // Initialize an empty String to hold the content

  while (file.available()) {
    content += (char)file.read();  // Read each character and append to the String
  }

  file.close();
  oledWord("File Loaded");
  delay(200);
  einkRefresh = FULL_REFRESH_AFTER; //Force a full refresh
  return content;  // Return the complete String
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  delay(200);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}