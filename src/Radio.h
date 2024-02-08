#ifndef _RADIO_H_
#define _RADIO_H_

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include "config.h"
#include "tools.h"

#define RADIO_PLUGIN_START \
    namespace ook {\
        namespace CONCAT(radio_, PLUGIN_NAME) {\
            class RadioPlugin : public Radio {\
            public:

#define RADIO_PLUGIN_END \
            };\
            struct AutoRegister {\
                AutoRegister() {\
                    static RadioPlugin radioPlugin;\
                    Radio::add(QUOTE(PLUGIN_NAME), static_cast<Radio*>(&radioPlugin));\
                }\
            } autoRegister;\
        }\
    }

#define CHECK_RADIO_SET \
    if (current == nullptr) {\
        ERROR("ERROR: No radio selected.\n");\
        return false;\
    }

#define PIN_MODE(x, y)  if (x != -1) pinMode(x, y);
#define PIN_WRITE(x, y) if (x != -1) digitalWrite(x, y);
#define PIN_INPUT(x)    if (x != -1) pinMode(x, INPUT)
#define PIN_OUTPUT(x)   if (x != -1) pinMode(x, OUTPUT)
#define PIN_HIGH(x)     if (x != -1) { pinMode(x, OUTPUT); digitalWrite(x, HIGH); }
#define PIN_LOW(x)      if (x != -1) { pinMode(x, OUTPUT); digitalWrite(x, LOW); }


#define RADIO_DO(action) {\
    int res = radio->action;\
    showRadiolibResult(res, #action);\
    if (res != 0) return false;\
    }

#define MODULE_DO(action) {\
    int res = radioLibModule->action;\
    showRadiolibResult(res, #action);\
    if (res != 0) return false;\
    }


// Device::store cannot become an std::vector because of the auto-register trick.

class Radio {
public:
    static struct {
        Radio* pointer;
        char name[MAX_RADIO_NAME_LEN];
    } store[MAX_RADIOS];
    static Radio* current;
    static int len;
    static int pin_rx;
    static int pin_tx;
    static bool add(const char* name, Radio *pointer);
    static bool setup();
    static bool select(const String &name);
    static String list(String separator = ", ");
    static bool radio_init();
    static bool radio_rx();
    static bool radio_tx();
    static bool radio_standby();
    String name();
    virtual bool init();
    virtual bool rx();
    virtual bool tx();
    virtual bool standby();

    // radiolib-specific
    Module* radioLibModule;
    SPIClass* spi;
    int pin_cs;
    float frequency;
    float bandwidth;
    float bitrate;
    int threshold_type_fixed;
    int threshold_type_peak;
    int threshold_type_average;
    int threshold_type_int;
    int threshold_level;
    void radiolibInit();
    void showRadiolibResult(const int result, const char* action);
    int thresholdSetup(const int fixed, const int average, const int peak);
};

#endif
