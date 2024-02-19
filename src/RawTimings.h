#ifndef _RAWTIMINGS_H_
#define _RAWTIMINGS_H_

#include <Arduino.h>
#include <vector>
#include "config.h"

class Pulsetrain;

/// @brief RawTimings instances store the time in µs of each interval
class RawTimings {
public:
    static bool maybe(String str);

    /// @brief std::vector of uint16_t times in µs for each interval
    std::vector<uint16_t>intervals;
    
    IRAM_ATTR operator bool();
    void IRAM_ATTR zap();
    String toString();
    bool fromString(const String &in);
    bool fromPulsetrain(Pulsetrain &train);
    Pulsetrain toPulsetrain();
    String visualizer();
    String visualizer(int base);
};

#endif
