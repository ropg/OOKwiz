#ifndef _SERIAL_OUTPUT_H_
#define _SERIAL_OUTPUT_H_

#include "Settings.h"
#include <Arduino.h>

#define ERROR(...) {\
    String errorlevel;\
    SETTING(errorlevel);\
    if (errorlevel != "none") Serial.printf(__VA_ARGS__);\
}

#define INFO(...) {\
    String errorlevel;\
    SETTING(errorlevel);\
    if (errorlevel != "none" && errorlevel != "error") Serial.printf(__VA_ARGS__);\
}

#define DEBUG(...) {\
    String errorlevel;\
    SETTING(errorlevel);\
    if (errorlevel == "debug") Serial.printf(__VA_ARGS__);\
}

#define RFLINK(...) \
    Serial.printf("20;%02X;", rflink_seq_nr++);\
    Serial.printf(__VA_ARGS__);

extern uint8_t rflink_seq_nr;

#endif
