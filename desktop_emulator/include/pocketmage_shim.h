#ifndef POCKETMAGE_SHIM_H
#define POCKETMAGE_SHIM_H

// Minimal shim to make PocketMage source compile on desktop
#define DESKTOP_EMULATOR

// Forward declare the actual PocketMage functions we want to use
extern void drawHome();
extern void processKB_HOME();

// Forward declare PocketMage variables we need
extern int CurrentAppState;
extern bool newState;
extern const char* appStateNames[];
extern const unsigned char* appIcons[];

// PocketMage app states (from globals.h)
enum AppState {
    HOME = 0,
    TXT = 1,
    FILEWIZ = 2,
    USB = 3,
    BT = 4,
    SETTINGS = 5,
    TASKS = 6,
    CALENDAR = 7,
    JOURNAL = 8,
    LEXICON = 9
};

#endif // POCKETMAGE_SHIM_H
