#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <Arduino.h>
#include "config.h"
#include "RawTimings.h"
#include "Pulsetrain.h"
#include "Meaning.h"
#include "tools.h"

#define DEVICE_PLUGIN_START(name) \
    namespace ook {\
        namespace device_ ## name {\
        class DevicePlugin : public Device {\
        public:

#define DEVICE_PLUGIN_END(name) \
        };\
        struct AutoRegister {\
            AutoRegister() {\
                static DevicePlugin devicePlugin;\
                Device::add(#name, static_cast<Device*>(&devicePlugin));\
            }\
        } autoRegister;\
    \
        }\
    }


// Device::store cannot become an std::vector because of the auto-register trick.

class Device {
public:
    static struct {
        Device* pointer;
        char name[MAX_DEVICE_NAME_LEN];
    } store[MAX_DEVICES];
    static int len;
    static bool setup();
    static bool add(const char* name, Device *pointer);
    static String list(String separator = ", ");
    static bool new_packet(RawTimings &raw, Pulsetrain &train, Meaning &meaning);
    static bool transmit(const String &plugin_name, const String &toTransmit);
    virtual bool receive(const RawTimings &raw, const Pulsetrain &train, const Meaning &meaning);
    virtual bool transmit(const String &toTransmit);
};


#endif
