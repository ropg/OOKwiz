#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <Arduino.h>

#define QUOTE_2(x) #x
#define QUOTE(x) QUOTE_2(x)
#define CONCAT_2(x, y) x##y
#define CONCAT(x, y) CONCAT_2(x, y)

// So that the stack size can be increased. See config.h for setting
size_t getArduinoLoopTaskStackSize();

#define SPLIT(in, sep, a, b) \
    String a;\
    String b;\
    tools::split(in, sep, a, b);

#define snprintf_append(append_to, len, ...) \
    {\
        char buffer[len];\
        snprintf(buffer, len, __VA_ARGS__);\
        append_to = append_to + buffer;\
    }

namespace tools {

    long nthNumberFrom(String &in, int num);
    void trim(String &in);
    void shiftInBit(uint8_t* buf, const int len, const bool bit);
    bool shiftOutBit(uint8_t* buf, const int len);
    void split(const String &in, const String &separator, String &before, String &after);
    bool between(const int &compare, const int &lower_bound, const int &upper_bound);

}

#endif
