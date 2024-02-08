#ifndef _RAWTIMINGS_H_
#define _RAWTIMINGS_H_

#include <Arduino.h>
#include <vector>
#include "config.h"

class Pulsetrain;

class RawTimings {
public:
    static bool maybe(String str);

    std::vector<uint16_t>intervals;
    
    operator bool();
    void zap();
    String toString();
    bool fromString(const String &in);
    bool fromPulsetrain(Pulsetrain &train);
    String visualizer();
    String visualizer(int base);
};

#endif
