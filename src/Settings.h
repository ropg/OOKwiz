#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <map>
#include <Arduino.h>
#include "serial_output.h"
#include "config.h"

// Update same-named variable only if setting exists
#define SETTING(name) Settings::get(#name, name);

// Same but now sets the already existing variable to the
// default if setting doesn't exist.
#define SETTING_WITH_DEFAULT(name, default) if (!Settings::get(#name, name)) name = default;

// For use in functions that return a bool. Assumes there is
// a variable (int, long, String or float) of the same name
// as the setting, and sets that variable to the value in the 
// setting. If no such setting found, will show ERROR and then
// will "return false;" out of the function you call this from. 
#define SETTING_OR_ERROR(name) if (!Settings::get(#name, name)) {\
        ERROR("Mandatory setting '%s' missing.\n", #name);\
        return false;\
}

#define MANDATORY(name) if (!Settings::isSet(#name)) {\
        ERROR("Mandatory setting '%s' missing.\n", #name);\
        return false;\
}

class Settings {

public:
    Settings();
    static bool set(const String &name, const String &value = "");

    template <typename T>
    static bool set(const String &name, const T &value = "") {
        return set(name, String(value));
    }

    static bool unset(const String &name);
    static bool get(const String &name, String &value);
    static bool get(const String &name, float &value);
    static bool get(const String &name, int &value);
    static bool get(const String &name, long &value);
    static String getString(const String &name, const String dflt = "");
    static int getInt(const String &name, const long dflt = -1);
    static long getLong(const String &name, const long dflt = -1);
    static float getFloat(const String &name, const float dflt = -1);
    static bool validName(const String &name);
    static String list();
    static bool fromList(String in);
    static bool save(const String filename);
    static bool load(const String filename);
    static bool ls();
    static bool rm(const String filename);
    static bool fileExists(String filename);
    static void zap();
    static bool isSet(const String &name);

private:
    static std::map<String, String> store;

};

#endif
