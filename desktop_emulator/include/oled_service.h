#pragma once

#include <string>
#include <mutex>
#include <atomic>

// Thread-safe OLED compositor service to prevent Metal command buffer re-entrancy
class OledService {
public:
    static OledService& getInstance();
    
    // Thread-safe: set OLED content from any thread
    void setLines(const std::string& line1, const std::string& line2, const std::string& line3);
    void presentIfDirty();
    void clear();
    
    struct OledSnapshot {
        std::string line1, line2, line3;
        bool valid = false;
    };
    OledSnapshot getSnapshot();

private:
    struct OledState {
        std::string line1;
        std::string line2; 
        std::string line3;
        bool dirty = false;
    };
    
    OledState state_;
    std::mutex mutex_;
    std::atomic<bool> inPresent_{false};
    
    OledService() = default;
    ~OledService() = default;
    OledService(const OledService&) = delete;
    OledService& operator=(const OledService&) = delete;
};

// C-style interface for compatibility
extern "C" {
    void oled_set_lines(const char* line1, const char* line2, const char* line3);
    void oled_present_if_dirty();
    void oled_clear();
    void oled_get_snapshot(char* line1, char* line2, char* line3, int* valid);
}
