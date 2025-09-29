#include <Arduino.h>
#include "globals.h"

int countLines(String input, size_t maxLineLength) {
    int total = 1;
    size_t currentLen = 0;

    for (int i = 0; i < input.length(); ++i) {
        char c = input[i];

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            total += 1;
            currentLen = 0;
            continue;
        }

        currentLen += 1;
        if (currentLen >= maxLineLength) {
            total += 1;
            currentLen = 0;
        }
    }

    return total;
}

void stringToVector(String inputText) {
    allLines.clear();
    String line = "";
    for (int i = 0; i < inputText.length(); ++i) {
        char c = inputText[i];
        if (c == '\r') continue;
        if (c == '\n') {
            allLines.push_back(line);
            line = "";
        } else {
            line += c;
        }
    }
    if (line.length() > 0) {
        allLines.push_back(line);
    }
    newLineAdded = true;
}
