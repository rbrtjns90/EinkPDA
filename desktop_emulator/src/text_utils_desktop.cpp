#include <Arduino.h>
#include "globals.h"

// Desktop-safe helpers for Windows build.
// - countLines: approximates wrapping by character count, not pixel width.
// - stringToVector: splits input into lines and populates global allLines.

int countLines(String input, size_t maxLineLength) {
    // Count starts at 1 for the initial line (even if empty)
    int total = 1;
    size_t currentLen = 0;

    for (int i = 0; i < input.length(); ++i) {
        char c = input[i];

        // Treat CRLF/CR uniformly by ignoring '\r'
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

// Windows desktop implementation matching globals.h prototype
// Fills the global allLines vector by splitting on newlines; no pixel-based wrapping.
void stringToVector(String inputText) {
    allLines.clear();
    String line = "";
    for (int i = 0; i < inputText.length(); ++i) {
        char c = inputText[i];
        if (c == '\r') continue; // normalize CRLF/CR to LF
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
