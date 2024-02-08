#ifndef _MEANING_H_
#define _MEANING_H_

#include <vector>
#include <Arduino.h>
#include "config.h"

class Pulsetrain;

typedef enum modulation {
    UNKNOWN,
    PULSE,
    GAP,
    PWM,
    PPM
} modulation;

typedef struct MeaningElement {
    modulation type;
    std::vector<uint8_t> data;
    uint16_t data_len;   // in bits
    uint16_t time1;
    uint16_t time2;
    uint16_t time3;
} MeaningElement;

class Meaning {
public:
    std::vector<MeaningElement> elements;
    bool suspected_incomplete = false;
    uint16_t repeats = 0;
    uint16_t gap = 0;

    static bool maybe(String str);
    operator bool();
    void zap();
    bool fromPulsetrain(Pulsetrain &train);
    bool addPulse(uint16_t pulse_time);
    bool addGap(uint16_t pulse_time);
    bool addPWM(int space, int mark, int bits, uint8_t* tmp_data);
    bool addPPM(int space, int mark, int filler, int bits, uint8_t* tmp_data);
    String toString();
    bool fromString(String in);
    int parsePWM(const Pulsetrain &train, int from, int to, int space, int mark);
    int parsePPM(const Pulsetrain &train, int from, int to, int space, int mark, int filler);
};

#endif
