#ifndef _OOKWIZ_H_
#define _OOKWIZ_H_

#include <Arduino.h>

#include "config.h"
#include "Radio.h"
#include "RawTimings.h"
#include "Pulsetrain.h"
#include "Meaning.h"
#include "Settings.h"
#include "Device.h"
#include "tools.h"
#include "serial_output.h"

/**
 * In the loop() handling of packets, we want to operate on sets of RawTimings
 * and Pulsetrain.
*/
typedef struct BufferPair {
    RawTimings raw;
    Pulsetrain train;
    void zap() {
        raw.zap();
        train.zap();
    }
} BufferPair;

/**
 * In the loop() handling of packets, we want to operate on triplets of RawTimings,
 * Pulsetrain and Meaning.
*/
typedef struct BufferTriplet {
    RawTimings raw;
    Pulsetrain train;
    Meaning meaning;
    void zap() {
        raw.zap();
        train.zap();
        meaning.zap();
    }
} BufferTriplet;

/**
 * \brief The static functions in the OOKwiz class provide the main controls for OOKwiz' functionality.
 * Prefix them with `OOKwiz::` to use them from your own code.
 * 
 * Example use of functions from the OOKwiz class
 * ```cpp
 * void setup() {
 *     OOKwiz::setup();
 * }
 * 
 * void loop() {
 *     OOKwiz::loop();
 * }
 * ```
*/
class OOKwiz {

public:
    static bool setup(bool skip_saved_defaults = false);
    static bool loop();
    static bool receive();
    static bool onReceive(void (*callback_function)(RawTimings, Pulsetrain, Meaning));
    static bool standby();
    static bool simulate(String &str);
    static bool simulate(RawTimings &raw);
    static bool simulate(Pulsetrain &train);
    static bool simulate(Meaning &meaning);
    static bool transmit(String &str);
    static bool transmit(RawTimings &raw);
    static bool transmit(Pulsetrain &train);
    static bool transmit(Meaning &meaning);

private:
    static volatile enum Rx_State{
        RX_OFF,
        RX_WAIT_PREAMBLE,
        RX_RECEIVING_DATA,
        RX_PROCESSING
    } rx_state;
    static bool serial_cli_disable;
    static int first_pulse_min_len;
    static int pulse_gap_min_len;
    static int pulse_gap_len_new_packet;
    static int min_nr_pulses;
    static int max_nr_pulses;
    static int noise_penalty;
    static int noise_threshold;
    static int noise_score;
    static bool no_noise_fix;
    static int lost_packets;
    static int64_t last_transition;
    static hw_timer_t *transitionTimer;
    static int64_t repeat_time_start;
    static long repeat_timeout;
    static bool rx_active_high;
    static bool tx_active_high;
    static RawTimings isr_in;
    static RawTimings isr_out;
    static BufferPair loop_in;
    static BufferPair loop_compare;
    static BufferTriplet loop_ready;
    static int64_t last_periodic;
    static void (*callback)(RawTimings, Pulsetrain, Meaning);
    static void IRAM_ATTR ISR_transition();
    static void IRAM_ATTR ISR_transitionTimeout();
    static void IRAM_ATTR process_raw();
    static bool tryToBeNice(int ms);

};

#endif
