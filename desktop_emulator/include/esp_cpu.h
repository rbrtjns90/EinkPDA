#ifndef ESP_CPU_H
#define ESP_CPU_H

#include "pocketmage_compat.h"

// ESP32 CPU functions (inline to avoid multiple definitions)
inline void esp_cpu_set_frequency(uint32_t freq_mhz) {}
inline uint32_t esp_cpu_get_frequency() { return 240; } // Mock 240MHz
inline void esp_cpu_wait_for_intr() {}
inline void esp_cpu_stall(int cpu_id) {}
inline void esp_cpu_unstall(int cpu_id) {}
inline void esp_cpu_reset(int cpu_id) {}

#endif
