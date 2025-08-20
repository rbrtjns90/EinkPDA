#ifndef BUZZER_H
#define BUZZER_H

#include "pocketmage_compat.h"

class Buzzer {
public:
    Buzzer() = default;
    Buzzer(int pin) {} // Constructor that accepts pin number
    
    void begin(uint8_t pin) {}
    void tone(uint16_t frequency, uint16_t duration) {}
    void noTone() {}
    void sound(uint16_t frequency, uint16_t duration) {}
    void end(int param) {}
    
    void beep(int frequency = 1000, int duration = 100) {
        // Mock implementation - could play system beep or print to console
        std::cout << "[BUZZER] Beep: " << frequency << "Hz for " << duration << "ms" << std::endl;
    }
    
    void tone(int frequency) {
        std::cout << "[BUZZER] Tone: " << frequency << "Hz" << std::endl;
    }
};

extern Buzzer buzzer;

#endif
