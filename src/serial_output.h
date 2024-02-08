#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "Settings.h"

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

#endif
