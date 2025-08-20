#ifndef USBMSC_H
#define USBMSC_H

#include "pocketmage_compat.h"

class USBMSC {
public:
    bool begin() { return true; }
    void end() {}
    bool mediaPresent() { return true; }
    uint32_t sectorsCount() { return 1024 * 1024; } // 512MB mock
    void onRead(void (*callback)(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)) {}
    void onWrite(void (*callback)(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)) {}
};

extern USBMSC MSC;

#endif
