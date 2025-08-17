#ifndef SD_MMC_H
#define SD_MMC_H

#include "pocketmage_compat.h"

class File {
private:
    std::unique_ptr<std::ifstream> inFile;
    std::unique_ptr<std::ofstream> outFile;
    bool isOpen;
    bool isDir;
    std::string filePath;
    std::vector<std::string> dirEntries;
    size_t dirIndex;

public:
    File();
    File(const std::string& path, const std::string& mode);
    ~File();
    
    // Move constructor and assignment
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;
    
    // Delete copy constructor and assignment
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    
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
    bool isDirectory();
    File openNextFile();
    bool print(const char* msg);
    bool print(const String& msg);
    bool println(const char* msg);
    bool println(const String& msg);
};

class SD_MMCClass : public fs::FS {
public:
    bool begin(const char* mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false) { return true; }
    void end() {}
    File open(const String& path, const char* mode = "r") override;
    File open(const char* path, const char* mode = "r") override;
    bool exists(const String& path);
    bool exists(const char* path);
    bool mkdir(const String& path);
    bool mkdir(const char* path);
    bool remove(const String& path);
    bool remove(const char* path) override;
    bool rmdir(const String& path);
    bool rmdir(const char* path);
    bool rename(const char* path1, const char* path2) override;
    bool rename(const String& path1, const String& path2);
    uint64_t totalBytes() { return 1024 * 1024 * 1024; } // 1GB mock
    uint64_t usedBytes() { return 512 * 1024 * 1024; }   // 512MB mock
};

extern SD_MMCClass SD_MMC;

#endif
