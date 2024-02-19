#include "Settings.h"
#include "config.h"       // provides factorySettings()
#include "serial_output.h"
#include "FS.h"
#include "SPIFFS.h"
#include "tools.h"

std::map<String, String> Settings::store;

// Constructor sets the defaults from config.cpp, see 'dummy' at end
Settings::Settings() {
    factorySettings();
}

/// @brief Save settings to file in SPIFFS
/// @param filename The actual filename in SPIFFS will have SPIFFS_PREFIX from config.h preprended
/// @return `true` if it worked, displays error and returns `false` if not. 
bool Settings::save(const String filename) {
    if (!validName(filename)) {
        return false;
    }
    String actual_filename = QUOTE(SPIFFS_PREFIX/) + filename;
    if (!SPIFFS.begin(true)) {
        ERROR("ERROR: Could not open SPIFFS filesystem.\n");
        return false;
    }
    File file = SPIFFS.open(actual_filename, FILE_WRITE);
    if (!file) {
        ERROR("ERROR: Could not open file '%s' for writing.\n", filename.c_str(), FILE_WRITE);
        return false;            
    }
    String contents = list() + "\n";
    if (!file.print(contents)) {
        ERROR("ERROR: Could not save settings to flash.\n");
        return false;
    }
    INFO("Saved settings to file '%s'.\n", filename.c_str());
    return true;
}

/// @brief Load settings from file in SPIFFS
/// @param filename The actual filename in SPIFFS will have SPIFFS_PREFIX from config.h preprended
/// @return `true` if it worked, displays error and returns `false` if not. 
bool Settings::load(const String filename) {
    if (!validName(filename)) {
        return false;
    }
    String actual_filename = QUOTE(SPIFFS_PREFIX/) + filename;
    if (!SPIFFS.begin(true)) {
        ERROR("ERROR: Could not open SPIFFS filesystem.\n");
        return false;
    }
    File file = SPIFFS.open(actual_filename);
    if (!file || !SPIFFS.exists(actual_filename)) {
        ERROR("ERROR: Could not open file '%s'.\n", filename.c_str());
        return false;            
    }
    String contents;
    while(file.available()) {
        contents += char(file.read());
    }
    INFO ("Loaded settings from file '%s'.\n", filename.c_str());
    return fromList(contents);
}

/// @brief Shows all files in the location SPIFFS_PREFIX in SPIFFS.
/// @return `true` if it worked, displays error and returns `false` if not. 
bool Settings::ls() {
    if (!SPIFFS.begin(true)) {
        ERROR("ERROR: Could not open SPIFFS filesystem.\n");
        return false;
    }
    File root = SPIFFS.open(QUOTE(SPIFFS_PREFIX));
    File file = root.openNextFile();
    while(file){
        INFO("%s\n", file.name());
        file = root.openNextFile();
    }
    return true;
}

/// @brief Deletes file from SPIFFS
/// @param filename The actual filename in SPIFFS will have SPIFFS_PREFIX from config.h preprended
/// @return `true` if it worked, displays error and returns `false` if not. 
bool Settings::rm(const String filename) {
    String actual_filename = QUOTE(SPIFFS_PREFIX/) + filename;
    if (!SPIFFS.begin(true)) {
        ERROR("ERROR: Could not open SPIFFS filesystem.\n");
        return false;
    }
    if (SPIFFS.remove(actual_filename)) {
        INFO("File '%s' deleted.\n", filename.c_str());
        return true;
    }
    ERROR("ERROR: rm '%s': file not found.\n", filename);  
    return false;
}

/// @brief See if given file exists in SPIFFS
/// @param filename The actual filename in SPIFFS will have SPIFFS_PREFIX from config.h preprended
/// @return `true` if file exists, `false` if not. 
bool Settings::fileExists(const String filename) {
    String actual_filename = QUOTE(SPIFFS_PREFIX/) + filename;
    if (!validName(filename)) {
        return false;
    }
    if (!SPIFFS.begin(true)) {
        ERROR("ERROR: Could not open SPIFFS filesystem. On Arduino IDE, check Tools/Partition Scheme to make sure\n       you are set up to have SPIFFS.\n");
        return false;
    }
    return SPIFFS.exists(actual_filename);                
}

/// @brief Deletes all settings from memory
void Settings::zap() {
    store.clear();
}

/// @brief Stores all settings from a String into memory
/// @param in String that contains name=value<lf>name=value<lf>...
/// @return `true` if it worked, displays error and returns `false` if not. 
bool Settings::fromList(String in) {
    zap();
    while (true) {
        int lf = in.indexOf("\n");
        if (lf == -1) {
            break;
        }
        String this_one = in.substring(0, lf);
        int equals_sign = this_one.indexOf("=");
        if (equals_sign != -1) {
            store[this_one.substring(0, equals_sign)] = this_one.substring(equals_sign + 1);
        } else {
            store[this_one.substring(0, equals_sign)] = "";
        }
        in = in.substring(lf + 1);
    }
    return true;
}


/// @brief `true` if name is a valid name for a setting.
bool Settings::validName(const String &name) {
    if (name.length() == 0) {
        ERROR("ERROR: name cannot be empty.\n");
        return false;
    }
    for (int n = 0; n < name.length(); n++) {
        if (!isAlphaNumeric(name.charAt(n)) && name.charAt(n) != '_') {
            ERROR("ERROR: name '%s' contains illegal character.\n", name.c_str());
            return false;
        }
    }
    return true;
}

/// @brief List of values in memory
/// @return String with name=value<lf>name=value<lf>...
String Settings::list() {
    String res;
    for (const auto& pair: store) {
        // Remove the ending '=' for settings that are merely set, no value.
        if (pair.second == "") {
            res += pair.first;
        } else {
            res += pair.first;
            res += "=";
            res += pair.second;
        }
        res += "\n";
    }
    // cut off last lf
    res = res.substring(0, res.length() - 1);
    return res;
}

/// @brief Find out if a key with given name exists in memory
/// @param name Setting name 
/// @return `true` if that name is set
bool Settings::isSet(const String &name) {
    return store.count(name);
}

/// @brief Set a value
/// @param name name of the key to be set
/// @param value value to be set as an Arduino String
/// @return 
bool Settings::set(const String &name, const String &value) {
    if (!validName(name)) {
        return false;
    }
    store[name] = value;
    return true;
}

/// @brief Removes key with given name from memory
/// @param name name of key
/// @return `true` if removed, `false` if name not valid or key not set.
bool Settings::unset(const String &name) {
    if (!validName(name) || !isSet(name)) {
        return false;
    }
    store.erase(name);
    return true;
}

/// @brief Get a value from memory as String
/// @param name name of the key
/// @param value String variable that will hold the value on return
/// @return `true` if value found, `false` if not
bool Settings::get(const String &name, String &value) {
    if (isSet(name)) {
        value = store[name];
        return true;
    }
    return false;
}

/// @brief Get a value from memory as float
/// @param name name of the key
/// @param value `float` variable that will hold the value on return
/// @return `true` if value found, `false` if not
bool Settings::get(const String &name, float &value) {
    String val_string;
    if (get(name, val_string)) {
        value = val_string.toFloat();
        return true;
    } else {
        return false;
    }
}

/// @brief Get a value from memory as int
/// @param name name of the key
/// @param value `int` variable that will hold the value on return
/// @return `true` if value found, `false` if not
bool Settings::get(const String &name, int &value) {
    String val_string;
    if (get(name, val_string)) {
        value = val_string.toInt();
        return true;
    } else {
        return false;
    }
}

/// @brief Get a value from memory as long
/// @param name name of the key
/// @param value `long` variable that will hold the value on return
/// @return `true` if value found, `false` if not
bool Settings::get(const String &name, long &value) {
    String val_string;
    if (get(name, val_string)) {
        value = val_string.toInt();    // String's .toInt() actually returns a long
        return true;
    } else {
        return false;
    }
}

/// @brief Get a value from memory as String with default
/// @param name name of the key
/// @param dflt [optional] Default returned if key not found in memory or "" if no default specified.
/// @return String with value found, or default
String Settings::getString(const String &name, const String dflt) {
    if (isSet(name)) {
        return store[name];
    }
    return dflt;
}

/// @brief Get a value from memory as int with default
/// @param name name of the key
/// @param dflt [optional] Default returned if key not found in memory or -1 if no default specified.
/// @return int with value found, or default
int Settings::getInt(const String &name, const long dflt) {
    if (isSet(name)) {
        return store[name].toInt();
    }
    return dflt;
}

/// @brief Get a value from memory as long with default
/// @param name name of the key
/// @param dflt [optional] Default returned if key not found in memory or -1 if no default specified.
/// @return long with value found, or default
long Settings::getLong(const String &name, const long dflt) {
    if (isSet(name)) {
        return store[name].toInt();
    }
    return dflt;
}

/// @brief Get a value from memory as float with default
/// @param name name of the key
/// @param dflt [optional] Default returned if key not found in memory or -1 if no default specified.
/// @return float with value found, or default
float Settings:: getFloat(const String &name, const float dflt) {
    if (isSet(name)) {
        return store[name].toFloat();
    }
    return dflt;
}

Settings dummy;     // so the constructor runs once for the default settings 
