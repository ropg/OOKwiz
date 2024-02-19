#ifndef _PULSETRAIN_H_
#define _PULSETRAIN_H_

#include <Arduino.h>
#include <vector>
#include "config.h"

class RawTimings;
class Meaning;

/// @brief Struct that holds information about a 'bin' in a Pulsetrain, a range of timings that are lumped together when converting RawTimings to a Pulsetrain. 
typedef struct pulseBin {
    /// @brief shortest time in bin in µs
    uint16_t min = 65535;
    /// @brief longest time in bin in µs
    uint16_t max = 0;
    /// @brief average time in bin in µs
    long average = 0;       // long bc used to store total time before averaging
    /// @brief Number of intervals in this bin in Pulsetrain
    uint16_t count = 0;
} pulseBin;

/// @brief Instances of Pulsetrain represent packets in a normalized way, meaning all intervals of similar length are made equal.
class Pulsetrain {
public:
    static bool maybe(String str);

    /// @brief std::vector with the bins, each a PulseBin struct
    std::vector<pulseBin> bins;
    /// @brief std::vector with the transitions, each merely the pulsebin that transition is in
    std::vector<uint8_t> transitions;
    /// @brief Total duration of this Pulsetrain in µs
    uint32_t duration = 0;
    /// @brief First seen at this time, in system microseconds
    int64_t first_at = 0;
    /// @brief Last seen at this time, in system microseconds
    int64_t last_at = 0;
    /// @brief Number of repetitions detected before either another packet came or `repeat_timeout` µs elapsed
    uint16_t repeats = 0;
    /// @brief Smallest gap between repeated transmissions 
    uint16_t gap = 0;

    operator bool();
    void zap();
    bool sameAs(const Pulsetrain &other_train);
    bool fromRawTimings(const RawTimings &raw);
    RawTimings toRawTimings();
    bool fromMeaning(const Meaning &meaning);
    Meaning toMeaning();
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
