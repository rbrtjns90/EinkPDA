#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "pocketmage_compat.h"

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false) { return true; }
    void end() {}
    void clear() {}
    bool remove(const char* key) { return true; }
    
    size_t putString(const char* key, const String& value) { return value.length(); }
    String getString(const char* key, const String& defaultValue = "") { return defaultValue; }
    
    size_t putInt(const char* key, int32_t value) { return sizeof(int32_t); }
    int32_t getInt(const char* key, int32_t defaultValue = 0) { return defaultValue; }
    
    size_t putBool(const char* key, bool value) { return sizeof(bool); }
    bool getBool(const char* key, bool defaultValue = false) { return defaultValue; }
    
    size_t putFloat(const char* key, float value) { return sizeof(float); }
    float getFloat(const char* key, float value = 0.0) { return value; }
};

extern Preferences preferences;

#endif
