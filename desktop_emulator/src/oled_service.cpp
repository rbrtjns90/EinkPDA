#include "oled_service.h"
#include "U8g2lib.h"
#include "pocketmage_compat.h"
#include <iostream>
#include <cstring>

// Static member definition
std::string U8G2_SH1106_128X32_VISIONOX_F_HW_I2C::textLines[3];

extern U8G2_SH1106_128X32_VISIONOX_F_HW_I2C u8g2;

OledService& OledService::getInstance() {
    static OledService instance;
    return instance;
}

void OledService::setLines(const std::string& line1, const std::string& line2, const std::string& line3) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.line1 = line1;
    state_.line2 = line2;
    state_.line3 = line3;
    state_.dirty = true;
}

void OledService::presentIfDirty() {
    // Re-entrancy guard
    if (inPresent_.exchange(true)) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!state_.dirty) {
            inPresent_.store(false);
            return;
        }
        // Mark as presented but keep data for getSnapshot()
        state_.dirty = false;
    }
    
    inPresent_.store(false);
}

OledService::OledSnapshot OledService::getSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    OledSnapshot snapshot;
    snapshot.line1 = state_.line1;
    snapshot.line2 = state_.line2;
    snapshot.line3 = state_.line3;
    snapshot.valid = true;
    return snapshot;
}

void OledService::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.line1.clear();
    state_.line2.clear();
    state_.line3.clear();
    state_.dirty = true;
}

// C-style interface
extern "C" {
    void oled_set_lines(const char* line1, const char* line2, const char* line3) {
        DEBUG_LOG("OLED", "Setting lines: '" + std::string(line1 ? line1 : "") + "' / '" + std::string(line2 ? line2 : "") + "' / '" + std::string(line3 ? line3 : "") + "'");
        OledService::getInstance().setLines(
            line1 ? line1 : "",
            line2 ? line2 : "", 
            line3 ? line3 : ""
        );
    }
    
    void oled_present_if_dirty() {
        OledService::getInstance().presentIfDirty();
    }
    
    void oled_clear() {
        OledService::getInstance().clear();
    }
    
    void oled_get_snapshot(char* line1, char* line2, char* line3, int* valid) {
        auto snapshot = OledService::getInstance().getSnapshot();
        if (snapshot.valid) {
            strncpy(line1, snapshot.line1.c_str(), 31);
            strncpy(line2, snapshot.line2.c_str(), 31);
            strncpy(line3, snapshot.line3.c_str(), 31);
            line1[31] = '\0';
            line2[31] = '\0';
            line3[31] = '\0';
            *valid = 1;
        } else {
            *valid = 0;
        }
    }
}
