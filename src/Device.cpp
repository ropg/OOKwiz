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

// Uses char*, and does not DEBUG or INFO because this is ran by the contructor 
// of the AutoRegister trick, so String and Serial are not available yet. 
bool Device::add(const char* name, Device *pointer) {
    if (len == MAX_DEVICES) {
        return false;
    }
    strncpy(store[len].name, name, MAX_DEVICE_NAME_LEN);
    store[len].name[MAX_DEVICE_NAME_LEN] = 0;   // just in case
    store[len].pointer = pointer;
    len++;
    return true;
}

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

bool Device::transmit(const String &plugin_name, const String &toTransmit) {
    for (int n = 0; n < len; n++) {
        if (String(store[n].name) == plugin_name) {
            return store[n].pointer->transmit(toTransmit);
        }
    }
    return false;        
}

bool Device::receive(const RawTimings &raw, const Pulsetrain &train, const Meaning &meaning) {
    return false;
}

bool Device::transmit(const String &toTransmit) {
    return false;
}

