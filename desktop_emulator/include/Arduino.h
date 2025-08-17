#ifndef ARDUINO_H
#define ARDUINO_H

// Arduino compatibility header for desktop compilation
#include "pocketmage_compat.h"

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Arduino timing functions
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// Arduino random functions
void randomSeed(unsigned long seed);
long random(long max);
long random(long min, long max);

// Arduino digital I/O
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);

// Arduino analog I/O
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);

// ESP32 specific
uint32_t esp_random();

#endif // ARDUINO_H
