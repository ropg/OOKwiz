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

typedef struct BufferPair {
    RawTimings raw;
    Pulsetrain train;
    void zap() {
        raw.zap();
        train.zap();
    }
} BufferPair;

class OOKwiz {

public:
    static bool setup(bool skip_saved_defaults = false);
    static bool loop();
    static bool receive();
    static bool onReceive(void (*callback_function)(RawTimings, Pulsetrain, Meaning));
    static bool simulate(String &str);
    static bool simulate(RawTimings &raw);
    static bool simulate(Pulsetrain &train);
    static bool simulate(Meaning &meaning);
    static bool transmit(String &str);
    static bool transmit(RawTimings &raw);
    static bool transmit(Pulsetrain &train);
    static bool transmit(Meaning &meaning);
    static bool standby();

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
    static int64_t last_transition;
    static hw_timer_t *transitionTimer;
    static hw_timer_t *repeatTimer;
    static long repeat_timeout;
    static bool rx_active_high;
    static bool tx_active_high;
    static BufferPair isr_in;
    static BufferPair isr_compare;
    static BufferPair isr_out;
    static RawTimings loop_raw;
    static Pulsetrain loop_train;
    static Meaning loop_meaning;
    static int64_t last_periodic;
    static void (*callback)(RawTimings, Pulsetrain, Meaning);
    static void IRAM_ATTR process_raw();
    static void IRAM_ATTR process_train();
    static void IRAM_ATTR ISR_transition();
    static void IRAM_ATTR ISR_transitionTimeout();
    static void IRAM_ATTR ISR_repeatTimeout();
    static bool IRAM_ATTR sameAs(const Pulsetrain &train1, const Pulsetrain &train2);
    static bool tryToBeNice(int ms);

};

#endif
