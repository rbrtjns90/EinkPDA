#pragma once
#include <string>
#include <mutex>
#include <atomic>

class OledService {
public:
    struct OledSnapshot { std::string line1, line2, line3; bool valid=false; };

    static OledService& getInstance();

    void setLines(const std::string& l1, const std::string& l2, const std::string& l3);
    void presentIfDirty();       // call from main thread
    OledSnapshot getSnapshot();  // optional, for debugging
    void clear();

private:
    OledService() = default;
    std::mutex mutex_;
    struct { std::string line1, line2, line3; bool dirty=false; } state_;
    std::atomic<bool> inPresent_{false};
};

extern "C" {
    void oled_set_lines(const char* line1, const char* line2, const char* line3);
    void oled_present_if_dirty(); // main thread only
    void oled_clear();
    void oled_get_snapshot(char* line1, char* line2, char* line3, int* valid);
}
