#include "Device.h"
#include "serial_output.h"
#include "device_plugins/DEVICE_INDEX"
#include "tools.h"
#include "Settings.h"


// static members
decltype(Device::store) Device::store;
int Device::len = 0;

bool Device::setup() {
    INFO("Device plugins loaded: %s\n", list().c_str());
    return true;
}

/// @brief Static, passes all 3 forms of an incoming packet to each non-disabled device plugin
/// @param raw incoming packet
/// @param train incoming packet
/// @param meaning incoming packet
/// @return `true` as soon as one of the plugin rx functions returns `true`, `false` otherwise
bool Device::new_packet(RawTimings &raw, Pulsetrain &train, Meaning &meaning) {
    for (int n = 0; n < len; n++) {
        String name = store[n].name;
        if (!Settings::isSet("device_" + name + "_disable")) {
            DEBUG("Trying device plugin '%s'.\n", name.c_str());
            if(store[n].pointer->receive(raw, train, meaning)) {
                DEBUG("Device plugin '%s' understood it!\n", name.c_str());
                return true;
            }
        }
    }
    return false;
}

/// @brief Registers an instance of Device, i.e. a devuce plugin in the static `store`
/// @param name (char*) Name of plugin, maximum MAX_DEVICE_NAME_LEN characters
/// @param pointer Pointer to the plugin instance
/// @return `false` if store already holds info on MAX_DEVICES plugins
bool Device::add(const char* name, Device *pointer) {
    // Uses char*, and does not DEBUG or INFO because this is ran by the contructor 
    // of the AutoRegister trick, so String and Serial are not available yet. 
    if (len == MAX_DEVICES) {
        return false;
    }
    strncpy(store[len].name, name, MAX_DEVICE_NAME_LEN);
    store[len].name[MAX_DEVICE_NAME_LEN] = 0;   // just in case
    store[len].pointer = pointer;
    len++;
    return true;
}

/// @brief Returns a String with alist of registered device plugins, appending " [disabled]" if needed
/// @param separator Between the names, e.g. ", "
/// @return The list
String Device::list(String separator) {
    String ret;
    for (int n = 0; n < len; n++) {
        ret += store[n].name;
        String disable_key;
        snprintf_append(disable_key, 50, "device_%s_disable", store[n].name);
        if (Settings::isSet(disable_key)) {
            ret += " [disabled]";
        }
        if (n < len - 1) {
            ret += separator;
        }
    }
    return ret;
}

/// @brief static, passes a String to a named device's `tx()` function
/// @param plugin_name name of plugin
/// @param toTransmit String to be passed to `tx()`
/// @return 
bool Device::transmit(const String &plugin_name, const String &toTransmit) {
    for (int n = 0; n < len; n++) {
        if (String(store[n].name) == plugin_name) {
            INFO("Device '%s': transmitting '%s'\n", plugin_name.c_str(), toTransmit.c_str());
            return store[n].pointer->transmit(toTransmit);
        }
    }
    ERROR("ERROR: cannot transmit. Device '%s' not found.")
    return false;        
}

/// @brief virtual, to be overridden in de individual plugins
/// @param raw incoming packet
/// @param train incoming packet
/// @param meaning incoming packet
/// @return `false` if not overridden
bool Device::receive(const RawTimings &raw, const Pulsetrain &train, const Meaning &meaning) {
    return false;
}

/// @brief virtual, to be overridden in de individual plugins
/// @param toTransmit String for plugin to make sense of and be transmitted
/// @return `false` if not overridden
bool Device::transmit(const String &toTransmit) {
    return false;
}

