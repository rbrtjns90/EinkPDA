#ifndef BUZZER_H
#define BUZZER_H

#include "pocketmage_compat.h"

class Buzzer {
public:
    void begin(uint8_t pin) {}
    void tone(uint16_t frequency, uint32_t duration = 0) {
        // Mock buzzer - just print to console
        std::cout << "[Buzzer] Tone: " << frequency << "Hz for " << duration << "ms" << std::endl;
    }
    void noTone() {}
    void beep(uint16_t frequency = 1000, uint32_t duration = 100) {
        tone(frequency, duration);
    }
};

extern Buzzer buzzer;

#endif
