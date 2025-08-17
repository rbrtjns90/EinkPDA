#ifndef SD_MMC_H
#define SD_MMC_H

#include "pocketmage_compat.h"

class File {
private:
    std::ifstream inFile;
    std::ofstream outFile;
    bool isOpen;
    std::string filePath;

public:
    File();
    File(const std::string& path, const std::string& mode);
    ~File();
    
    operator bool() const { return isOpen; }
    void close();
    size_t write(const uint8_t* data, size_t len);
    size_t write(const String& str);
    int read();
    String readString();
    String readStringUntil(char terminator);
    bool available();
    void seek(size_t pos);
    size_t size();
    String name();
};

class SD_MMCClass {
public:
    bool begin(const char* mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false) { return true; }
    void end() {}
    File open(const String& path, const char* mode = "r");
    File open(const char* path, const char* mode = "r");
    bool exists(const String& path);
    bool exists(const char* path);
    bool mkdir(const String& path);
    bool mkdir(const char* path);
    bool remove(const String& path);
    bool remove(const char* path);
    bool rmdir(const String& path);
    bool rmdir(const char* path);
    uint64_t totalBytes() { return 1024 * 1024 * 1024; } // 1GB mock
    uint64_t usedBytes() { return 512 * 1024 * 1024; }   // 512MB mock
};

extern SD_MMCClass SD_MMC;

#endif
