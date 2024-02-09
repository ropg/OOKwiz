#include "OOKwiz.h"
#include "CLI.h"
#include "serial_output.h"

volatile OOKwiz::Rx_State OOKwiz::rx_state = OOKwiz::RX_OFF;
bool OOKwiz::serial_cli_disable = false;
int OOKwiz::first_pulse_min_len;
int OOKwiz::pulse_gap_min_len;
int OOKwiz::min_nr_pulses;
int OOKwiz::max_nr_pulses;
int OOKwiz::pulse_gap_len_new_packet;
int OOKwiz::noise_penalty;
int OOKwiz::noise_threshold;
int OOKwiz::noise_score;
bool OOKwiz::no_noise_fix = false;
int64_t OOKwiz::last_transition;
hw_timer_t* OOKwiz::transitionTimer = nullptr;
hw_timer_t* OOKwiz::repeatTimer = nullptr;
long OOKwiz::repeat_timeout;
bool OOKwiz::rx_active_high;
bool OOKwiz::tx_active_high;
BufferPair OOKwiz::isr_in;
BufferPair OOKwiz::isr_compare;
BufferPair OOKwiz::isr_out;
RawTimings OOKwiz::loop_raw;
Pulsetrain OOKwiz::loop_train;
Meaning OOKwiz::loop_meaning;
int64_t OOKwiz::last_periodic = 0;
void (*OOKwiz::callback)(RawTimings, Pulsetrain, Meaning) = nullptr;

/// @brief Starts OOKwiz. Loads settings, initializes the radio.
/**
 * If you set the GPIO pin for a button on your ESP32 in 'pin_rescue' and press this during boot, OOKwiz will not
 * initialize SPI and the radio, possibly breaking and endless boot loop. Set 'rescue_active_high' if the button
 * connects to VCC instead of GND.
 * 
 * Normally, OOKwiz will start up in receive mode. If you set 'start_in_standby', it will start in standby mode instead.
*/
/// @param skip_saved_defaults The settings in the SPIFFS file 'defaults' are not read when this
///                            is true, leaving only the factory defaults from config.cpp.  
/// @return true if setup succeeded, false if it could not complete, e.g. because the radio is not configured yet.
bool OOKwiz::setup(bool skip_saved_defaults) {

    // Make sure nothing is missed when we paste raw data to 'sim' or 'transmit' CLI commands.
    Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);

    // Sometimes USB needs to wake up, and we want to see what OOKwiz does 
    // right after it is woken up with OOKwiz::setup().
    delay(1000);

    // Tada !
    INFO("\n\nOOKwiz version %s (built %s %s) initializing.\n", OOKWIZ_VERSION, __DATE__, __TIME__);

    // The factory defaults are loaded pre-main by Settings object constructor
    if (skip_saved_defaults == true) {
        INFO("OOKwiz::setup(true) called: not loading user defaults, factory settings only.\n");
    } else {
        // Try to get the runtime settings from SPIFFS
        if (!Settings::fileExists("default") || !Settings::load("default")) {
            INFO("No saved settings found, using factory settings.\n");
        }
    }

    // Skip the rest of OOKwiz::setup() by returning false when recue button pressed
    int pin_rescue;
    SETTING_WITH_DEFAULT(pin_rescue, -1);
    if (pin_rescue != -1) {
        PIN_INPUT(pin_rescue);
        if (digitalRead(pin_rescue) == Settings::isSet("rescue_active_high")) {
            INFO("Rescue button pressed at boot, skipping initialization.\n");
            Settings::unset("serial_cli_disable");
            return false;
        }
    }

    if (!Radio::setup()) {
        ERROR("ERROR: Your radio doesn't set up correctly. Make sure you set the correct\n       radio and pins, save settings and reboot.\n");
        return false;
    }

    Device::setup();

    // These settings determine what a valid transmission is
    SETTING_OR_ERROR(pulse_gap_len_new_packet);
    SETTING_OR_ERROR(repeat_timeout);
    SETTING_OR_ERROR(first_pulse_min_len);
    SETTING_OR_ERROR(pulse_gap_min_len);
    SETTING_OR_ERROR(min_nr_pulses);
    SETTING_OR_ERROR(max_nr_pulses);
    SETTING_OR_ERROR(noise_penalty);
    SETTING_OR_ERROR(noise_threshold);
    no_noise_fix = Settings::isSet("no_noise_fix");
    rx_active_high = Settings::isSet("rx_active_high");
    tx_active_high = Settings::isSet("tx_active_high");

    // Timer for pulse_gap_len_new_packet
    transitionTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(transitionTimer, &ISR_transitionTimeout, false);
    timerAlarmWrite(transitionTimer, pulse_gap_len_new_packet, true);
    timerAlarmEnable(transitionTimer);
    timerStart(transitionTimer);

    // Timer for repeat_timeout
    repeatTimer = timerBegin(1, 80, true);
    timerAttachInterrupt(repeatTimer, &ISR_repeatTimeout, false);
    timerAlarmWrite(repeatTimer, repeat_timeout, true);
    timerAlarmEnable(repeatTimer);
    timerStart(repeatTimer);

    // The ISR that actually reads the data
    attachInterrupt(Radio::pin_rx, ISR_transition, CHANGE);    

    if (Settings::isSet("start_in_standby")) {
        return standby();
    } else {
        return receive();
    }

}

bool OOKwiz::loop() {
    // Does the higher-level processing that can't happen at the ISR-level
    if (isr_out.train) {
        // Copy away the isr output and allow isr machinery to fill with new
        loop_train = isr_out.train;
        loop_raw = isr_out.raw;
        isr_out.zap();
        // Print to Serial what needs to be printed
        if (Settings::isSet("print_raw") ||
            Settings::isSet("print_visualizer") ||
            Settings::isSet("print_summary") ||
            Settings::isSet("print_pulsetrain") ||
            Settings::isSet("print_binlist") ||
            Settings::isSet("print_meaning")
        ) {
            INFO("\n\n");
        }
        if (Settings::isSet("print_raw") && loop_raw) {
            INFO("%s\n", loop_raw.toString().c_str());
        }
        if (Settings::isSet("print_visualizer")) {
            // If we simulate a Pulsetrain, the raw buffer will be empty still,
            // so we visualize the Pulsetrain instead. 
            if (loop_raw) {
                INFO("%s\n", loop_raw.visualizer().c_str());
            } else {
                INFO("%s\n", loop_train.visualizer().c_str());
            }
        }
        if (Settings::isSet("print_summary")) {
            INFO("%s\n", loop_train.summary().c_str());
        }
        if (Settings::isSet("print_pulsetrain")) {
            INFO("%s\n", loop_train.toString().c_str());
        }
        if (Settings::isSet("print_binlist")) {
            INFO("%s\n", loop_train.binList().c_str());
        }
        // Process the received pulsetrain for meaning
        // (Done here so errors and debug output ends up in logical spot)
        loop_meaning.fromPulsetrain(loop_train);
        if (loop_meaning && Settings::isSet("print_meaning")) {
            INFO("%s\n", loop_meaning.toString().c_str());
        }
        // Pass what was received to all the device plugins, making their output show up
        // at the right spot underneath the meaning output.
        Device::new_packet(loop_raw, loop_train, loop_meaning);
        // received() can take it now.
        if (callback != nullptr) {
            callback(loop_raw, loop_train, loop_meaning);
        }
    }
    // Stuff that happens only once a seccond
    if (esp_timer_get_time() - last_periodic > 1000000) {
        // If any of the core parameters have changed in settings,
        // update their variables.
        SETTING(pulse_gap_len_new_packet);
        SETTING(repeat_timeout);
        SETTING(first_pulse_min_len);
        SETTING(pulse_gap_min_len);
        SETTING(min_nr_pulses);
        SETTING(max_nr_pulses);
        no_noise_fix = Settings::isSet("no_noise_fix");
        serial_cli_disable = Settings::isSet("serial_cli_disable");
        last_periodic = esp_timer_get_time();
    }
    // Have CLI's loop check the serial port for data
    if (!serial_cli_disable) {
        CLI::loop();
    }
    return true;
}

void IRAM_ATTR OOKwiz::ISR_transition() {
    int64_t t = esp_timer_get_time() - last_transition;
    if (rx_state == RX_WAIT_PREAMBLE) {
        // Set the state machine to put the transitions in isr_in
        if (t > first_pulse_min_len && digitalRead(Radio::pin_rx) != rx_active_high) {
            noise_score = 0;
            isr_in.zap();
            isr_in.raw.intervals.reserve((max_nr_pulses * 2) + 1);
            rx_state = RX_RECEIVING_DATA;
        }
    }
    if (rx_state == RX_RECEIVING_DATA) {
        // t < pulse_gap_min_len means it's noise, so we assume transmission is over
        if (t < pulse_gap_min_len) {
            noise_score += noise_penalty;
            if (noise_score >= noise_threshold) {
                process_raw();
                return;
            }
        } else {
            noise_score -= noise_score > 0;
        }
        isr_in.raw.intervals.push_back(t);
        // Longer would be too long: stop and process what we have
        if (isr_in.raw.intervals.size() == (max_nr_pulses * 2) + 1) {
            process_raw();
        }

    }
    last_transition = esp_timer_get_time();
    timerRestart(transitionTimer);
}

void IRAM_ATTR OOKwiz::ISR_transitionTimeout() {
    if (rx_state != RX_OFF) {
        process_raw();
    }
}

void IRAM_ATTR OOKwiz::ISR_repeatTimeout() {
    if (isr_compare.train && !isr_out.train) {
        isr_out = isr_compare;
        isr_compare.zap();
    }
}

void IRAM_ATTR OOKwiz::process_raw() {
    // reject if not the required minimum number of pulses
    if (isr_in.raw.intervals.size() < (min_nr_pulses * 2) + 1) {
        rx_state = RX_WAIT_PREAMBLE;
        return;
    }
    // Remove last transition if number is even because in that case the
    // last transition is the off state, which is not part of a train.
    if (isr_in.raw.intervals.size() % 2 == 0) {
        isr_in.raw.intervals.pop_back();
    }
    if (!no_noise_fix) {
        // fix noise: too-short transitions found are merged into one with transitions before and after.
        bool noisy = true;
        while (noisy) {
            noisy = false;
            for (int n = 1; n < isr_in.raw.intervals.size() - 1; n++) {
                if (isr_in.raw.intervals[n] < pulse_gap_min_len) {
                    int new_interval = isr_in.raw.intervals[n - 1] + isr_in.raw.intervals[n] + isr_in.raw.intervals[n + 1];
                    isr_in.raw.intervals.erase(isr_in.raw.intervals.begin() + n - 1, isr_in.raw.intervals.begin() + n + 2);
                    isr_in.raw.intervals.insert(isr_in.raw.intervals.begin() + n - 1, new_interval);
                    noisy = true;
                    break;
                }
            }
        }
        // Simply cut off last pulse and preceding gap if pulse too short.
        if (isr_in.raw.intervals.back() < pulse_gap_min_len) {
            isr_in.raw.intervals.pop_back();
            isr_in.raw.intervals.pop_back();
        }    
        // Check we still meet the required minimum number of pulses after noise removal.
        if (isr_in.raw.intervals.size() < (min_nr_pulses * 2) + 1) {
            rx_state = RX_WAIT_PREAMBLE;
            return;
        }
    }
    // Release excess reserved memory
    isr_in.raw.intervals.shrink_to_fit();
    // And then go to normalizing, comparing, etc.
    isr_in.train.fromRawTimings(isr_in.raw);
    process_train();
}

void IRAM_ATTR OOKwiz::process_train() {
    rx_state = RX_PROCESSING;
    // If there is no packet in the middle buffer, just put the new one there
    if (!isr_compare.train) {
        isr_compare = isr_in;
        isr_in.zap();
        // Start the timer on it expiring and being handed to the user
        timerRestart(repeatTimer);
    // Otherwise check if it's a duplicate
    } else if (isr_in.train && sameAs(isr_in.train, isr_compare.train)) {
        // If so just add to number of repeats
        isr_compare.train.repeats++;
        // Check if the observed gap is smaller than what we had and if so store.
        int64_t gap = (esp_timer_get_time() - isr_compare.train.last_at) - isr_compare.train.duration;
        if (gap < isr_compare.train.gap || isr_compare.train.gap == 0) {
            isr_compare.train.gap = gap;
        }
        isr_compare.train.last_at = esp_timer_get_time();
        isr_in.zap();
        // Restart the repeat timer
        timerRestart(repeatTimer);
    } else {
        // Only move waiting train to output if the previous one was taken.
        if (!isr_out.train) {
            isr_out = isr_compare;
        }
        isr_compare = isr_in;
        isr_in.zap();
        timerRestart(repeatTimer);
    }
    rx_state = RX_WAIT_PREAMBLE;
}

bool IRAM_ATTR OOKwiz::sameAs(const Pulsetrain &train1, const Pulsetrain &train2) {
    if (train1.transitions.size() != train2.transitions.size()) {
        return false;
    }
    if (train1.bins.size() != train2.bins.size()) {
        return false;
    }
    for (int n = 0; n < train1.transitions.size(); n++) {
        if (train1.transitions[n] != train2.transitions[n]) {
            return false;
        }
    }
    for (int m = 0; m < train1.bins.size(); m++) {
        if (abs(train1.bins[m].average - abs(train2.bins[m].average)) > 300) {
            return false;
        }
    }
    return true;
}

bool OOKwiz::onReceive(void (*callback_function)(RawTimings, Pulsetrain, Meaning)) {
    callback = callback_function;
    return true;
}

bool OOKwiz::receive() {
    if (!Radio::radio_rx()) {
        return false;
    }
    // Turns on the state machine
    rx_state = RX_WAIT_PREAMBLE;
    return true;
}

bool OOKwiz::tryToBeNice(int ms) {
    // Try and wait for max ms for current reception to end
    // return false if it doesn't end, true if it does
    long start = millis();
    while (millis() - start < 500) {
        if (rx_state == RX_WAIT_PREAMBLE) {
            return true;
        }
    }
    return false;
}

bool OOKwiz::simulate(String &str) {
    if (RawTimings::maybe(str)) {
        RawTimings raw;
        if (raw.fromString(str)) {
            return simulate(raw);
        }
    } else if (Pulsetrain::maybe(str)) {
        Pulsetrain train;
        if (train.fromString(str)) {
            return simulate(train);
        }
    } else if (Meaning::maybe(str)) {
        Meaning meaning;
        Pulsetrain train;
        if (meaning.fromString(str)) {
            return simulate(meaning);
        }
    } else {
        ERROR("ERROR: string does not look like RawTimings, Pulsetrain or Meaning.\n");
    }
    return false;
}

bool OOKwiz::simulate(RawTimings &raw) {
    tryToBeNice(500);
    isr_in.raw = raw;
    process_raw();
    return true;
}

bool OOKwiz::simulate(Pulsetrain &train) {
    tryToBeNice(500);
    isr_in.train = train;
    isr_in.raw.zap();
    process_train();
    return true;
}

bool OOKwiz::simulate(Meaning &meaning) {
    Pulsetrain train;
    if (train.fromMeaning(meaning)) {
        return simulate(train);
    }
    return false;
}

bool OOKwiz::transmit(String &str) {
    if (RawTimings::maybe(str)) {
        RawTimings raw;
        if (raw.fromString(str)) {
            return transmit(raw);
        }
    } else if (Pulsetrain::maybe(str)) {
        Pulsetrain train;
        if (train.fromString(str)) {
            return transmit(train);
        }
    } else if (Meaning::maybe(str)) {
        Meaning meaning;
        if (meaning.fromString(str)) {
            return transmit(meaning);
        }
    } else {
        ERROR("ERROR: string does not look like RawTimings, Pulsetrain or Meaning.\n");
    }
    return false;
}

bool OOKwiz::transmit(RawTimings &raw) {
    bool rx_was_on = (rx_state != RX_OFF);
    // Set receiver state machine off, remove any incomplete packet in buffer
    if (rx_was_on) {
        tryToBeNice(500);
        rx_state = RX_OFF;
        isr_in.zap();
    }
    if (!Radio::radio_tx()) {
        ERROR("ERROR: Transceiver could not be set to transmit.\n");
        return false;
    }
    INFO("Transmitting: %s\n", raw.toString().c_str());
    INFO("              %s\n", raw.visualizer().c_str());    
    delay(100);     // So INFO prints before we turn off interrupts
    int64_t tx_timer = esp_timer_get_time();
    noInterrupts();
    {
        bool bit = tx_active_high;
        PIN_WRITE(Radio::pin_tx, bit);
        for (uint16_t interval: raw.intervals) {
            delayMicroseconds(interval);
            bit = !bit;
            PIN_WRITE(Radio::pin_tx, bit);
        }
        PIN_WRITE(Radio::pin_tx, !tx_active_high);    // Just to make sure we end with TX off
    }
    interrupts();
    tx_timer = esp_timer_get_time() - tx_timer;
    INFO("Transmission done, took %i µs.\n", tx_timer);
    delayMicroseconds(400);
    // return to state it was in before transmit
    if (rx_was_on) {
        receive();
    } else {
        standby();
    }
    return true;
}

bool OOKwiz::transmit(Pulsetrain &train) {
    bool rx_was_on = (rx_state != RX_OFF);
    // Set receiver state machine off, remove any incomplete packet in buffer
    if (rx_was_on) {
        tryToBeNice(500);
        rx_state = RX_OFF;
        isr_in.zap();
    }
    if (!Radio::radio_tx()) {
        ERROR("ERROR: Transceiver could not be set to transmit.\n");
        return false;
    }
    INFO("Transmitting %s\n", train.toString().c_str());
    INFO("             %s\n", train.visualizer().c_str()); 
    delay(100);     // So INFO prints before we turn off interrupts
    int64_t tx_timer = esp_timer_get_time();
    for (int repeat = 0; repeat < train.repeats; repeat++) {
        noInterrupts();
        {
            bool bit = tx_active_high;
            PIN_WRITE(Radio::pin_tx, bit);
            for (int transition : train.transitions) {
                uint16_t t = train.bins[transition].average;
                delayMicroseconds(t);
                bit = !bit;
                PIN_WRITE(Radio::pin_tx, bit);
            }
            PIN_WRITE(Radio::pin_tx, !tx_active_high);    // Just to make sure we end with TX off
        }
        interrupts();
        delayMicroseconds(train.gap);
    }
    tx_timer = esp_timer_get_time() - tx_timer;
    INFO("Transmission done, took %i µs.\n", tx_timer);
    delayMicroseconds(400);
    // return to state it was in before transmit
    if (rx_was_on) {
        receive();
    } else {
        standby();
    }
    return true;
}

bool OOKwiz::transmit(Meaning &meaning) {
    Pulsetrain train;
    if (train.fromMeaning(meaning)) {
        return transmit(train);
    }
    return false;
}

bool OOKwiz::standby() {
    if (rx_state != RX_OFF) {
        tryToBeNice(500);
        isr_in.zap();
        rx_state = RX_OFF;
        Radio::radio_standby();
    }
    return true;
}

