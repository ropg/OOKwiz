#ifndef _PULSETRAIN_H_
#define _PULSETRAIN_H_

#include <Arduino.h>
#include <vector>
#include "config.h"

class RawTimings;
class Meaning;

typedef struct pulseBin {
    uint16_t min = 65535;
    uint16_t max = 0;
    long average = 0;       // used to store total time before averaging
    uint16_t count = 0;
} pulseBin;

class Pulsetrain {
public:
    static bool maybe(String str);

    std::vector<pulseBin> bins;
    std::vector<uint8_t> transitions;
    uint32_t duration = 0;
    int64_t first_at = 0;
    int64_t last_at = 0;
    uint16_t repeats = 0;
    uint16_t gap = 0;

    IRAM_ATTR operator bool();
    void IRAM_ATTR zap();
    bool IRAM_ATTR fromRawTimings(const RawTimings &raw);
    bool fromMeaning(const Meaning &meaning);
    String summary() const;
    bool fromString(String in); 
    String toString() const;
    String binList();
    String visualizer();
    String visualizer(int base);

private:
    void addToBins(int time);
    int binFromTime(int time);
};

#endif
