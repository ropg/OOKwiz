#include "config.h"
#include "Settings.h"

void factorySettings() {
    Settings::set("pulse_gap_len_new_packet", 2000);
    Settings::set("first_pulse_min_len", 2000);
    Settings::set("pulse_gap_min_len", 30);
    Settings::set("min_nr_pulses", 16);
    Settings::set("max_nr_pulses", 300);
    Settings::set("bin_width", 150);
    Settings::set("repeat_timeout", 150000L);
    Settings::set("noise_penalty", 10);
    Settings::set("noise_threshold", 30);
    Settings::set("visualizer_pixel", 200);
    Settings::set("print_raw");
    Settings::set("print_visualizer");
    Settings::set("print_summary");
    Settings::set("print_pulsetrain");
    Settings::set("print_binlist");
    Settings::set("print_meaning");
}

