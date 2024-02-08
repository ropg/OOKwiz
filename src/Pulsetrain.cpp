#include <algorithm>        // for std::sort
#include "Pulsetrain.h"
#include "RawTimings.h"
#include "Meaning.h" 
#include "Settings.h"
#include "serial_output.h"
#include "tools.h"

// Note that anything marked IRAM_ATTR is used by the ISRs in OOKwiz.cpp and
// SHALL NOT have Serial output

// Quick early determination a String _might_ be a pulsetrain.
bool Pulsetrain::maybe(String str) {
    if (str.length() < 10) {
        return false;
    }
    for (int n = 0; n < 10; n++) {
        if (!isDigit(str.charAt(n))) {
            return false;
        }
    }
    DEBUG("Pulsetrain::maybe() returns true.\n");
    return true;
}

IRAM_ATTR Pulsetrain::operator bool() {
    return (transitions.size() > 0);
}

void IRAM_ATTR Pulsetrain::zap() {
    transitions.clear();
    bins.clear();
    gap = 0;
    repeats = 0;
    last_at = 0;
}

String Pulsetrain::toString() const {
    if (transitions.size() == 0) {
        return "<empty Pulsetrain>";
    }
    String res = "";
    for (int transition: transitions) {
        res += transition;
    }
    for (auto bin : bins) {
        res += ",";
        res += bin.average;
    }
    if (repeats > 1) {
        snprintf_append(res, 20, "*%i@%i", repeats, gap);
    }
    return res;
}

bool Pulsetrain::fromString(String in) {
    zap();
    int first_comma = in.indexOf(",");
    if (first_comma == -1) {
        ERROR("ERROR: cannot convert String to Pulsetrain, no commas present.\n");
        return false;
    }
    // fill transitions and deduce number of bins
    int num_bins = 0;
    for (int n = 0; n < first_comma; n++) {
        int digit = in.charAt(n);
        if (!isDigit(digit)) {
            zap();
            ERROR("ERROR: cannot convert String to Pulsetrain, non-digits in wrong place.\n");
            return false;
        }
        digit -= 48;    // "0" is 48 in ASCII
        transitions.push_back(digit);
        if (digit > num_bins) {
            num_bins = digit;
        }
    }
    num_bins++;
    int end_binlist = in.indexOf("*");
    if (end_binlist == -1) {
        end_binlist = in.length();
        repeats = 1;
    } else {
        int at_sign = in.indexOf("@");
        if (at_sign == -1) {
            zap();
            ERROR("ERROR: cannot convert String to Pulsetrain, * but no @ found.\n");
            return false;
        }
        repeats = in.substring(end_binlist + 1, at_sign).toInt();
        gap = in.substring(at_sign + 1).toInt();
        if (gap == 0 || repeats == 0) {
            zap();
            ERROR("ERROR: cannot convert String to Pulsetrain, invalid values for repeats or gap.\n");
            return false;
        }
    }
    int bin_start = first_comma + 1;
    for (int n = 0; n < num_bins; n++) {
        pulseBin new_bin;
        int next_comma = in.indexOf(",", bin_start);
        if (next_comma == -1) {
            next_comma = in.length();
        }
        new_bin.average = in.substring(bin_start, next_comma).toInt();
        new_bin.min = new_bin.average;
        new_bin.max = new_bin.average;
        if (new_bin.average == 0) {
            zap();
            ERROR("ERROR: cannot convert String to Pulsetrain, invalid bin value found.\n");
            return false;            
        }
        bin_start = next_comma + 1;
        bins.push_back(new_bin);
    }
    for (int transition : transitions) {
        bins[transition].count++;
        duration += bins[transition].average;
    }
    return true;
}

String Pulsetrain::summary() const {
    String res = "";
    snprintf_append(res, 80, "%i pulses over %i µs", (transitions.size() + 1) / 2, duration);
    if (repeats > 1) {
        snprintf_append(res, 80, ", repeated %i times with gaps of %i µs", repeats, gap);
    }
    return res;
}

bool IRAM_ATTR Pulsetrain::fromRawTimings(const RawTimings &raw) {
    int bin_width;
    SETTING_WITH_DEFAULT(bin_width, 150);
    std::vector<uint16_t> sorted = raw.intervals;
    // Create the bins
    std::sort(sorted.begin(), sorted.end());
    bool just_begun = true;
    for (auto interval : sorted) {
        if (just_begun || interval > bins.back().min + bin_width) {
            just_begun = false;
            pulseBin new_bin;
            new_bin.min = interval;
            bins.push_back(new_bin);
        }
        bins.back().max = interval;
    }
    // Walk intervals, add bin number to transitions, update count in its bin, find total duration
    duration = 0;
    for (auto interval : raw.intervals) {
        duration += interval;
        for (int m = 0; m < bins.size(); m++) {
            if (interval >= bins[m].min && interval <= bins[m].max) {
                transitions.push_back(m);
                bins[m].average += interval;    // use average for total first, which is why .average is a long
                bins[m].count++;
                break;
            }
        }
    }
    // Averages
    for (auto& bin : bins) {
        bin.average = bin.average / bin.count;
    }
    // Set other metadata about this Pulsetrain
    first_at = esp_timer_get_time();
    last_at = esp_timer_get_time();
    repeats = 1;
    return true;
}

String Pulsetrain::binList() {
    String res = "";
    snprintf_append(res, 50, " bin     min     avg     max  count");
    for (int m = 0; m < bins.size(); m++) {
        snprintf_append(res, 50, "\n%4i %7i %7i %7i %6i", m, bins[m].min, bins[m].average, bins[m].max, bins[m].count);
    }
    return res;
}

String Pulsetrain::visualizer() {
    int visualizer_pixel;
    SETTING_WITH_DEFAULT(visualizer_pixel, 200);
    return visualizer(visualizer_pixel);
}

String Pulsetrain::visualizer(int base) {
    uint8_t multiples[bins.size()];
    for (int m = 0; m < bins.size(); m++) {
        multiples[m] = max(((int)bins[m].average + (base / 2)) / base, 1);
    }
    String ones_and_zeroes;
    String curstate;
    for (int n = 0; n < transitions.size(); n++) {
        curstate = (n % 2 == 0) ? "1" : "0";
        for (int m = 0; m < multiples[transitions[n]]; m++) {
            ones_and_zeroes += curstate;
        }
    }
    ones_and_zeroes += "0";
    String output;
    for (int n = 0; n < ones_and_zeroes.length(); n += 2) {
        String chunk = ones_and_zeroes.substring(n, n + 2);
        if (chunk == "11") {
            output += "▀";
        } else if (chunk == "00") {
            output += " ";
        } else if (chunk == "01") {
            output += "▝";
        } else if (chunk == "10") {
            output += "▘";
        }
    }
    return output;
}

bool Pulsetrain::fromMeaning(const Meaning &meaning) {
    zap();
    // First make bins for all the different timings found
    for (const auto& el : meaning.elements) {
        if (el.type == PULSE || el.type == GAP) {
            addToBins(el.time1);
        }
        if (el.type == PWM) {
            addToBins(el.time1);
            addToBins(el.time2);
        }
        if (el.type == PPM) {
            addToBins(el.time1);
            addToBins(el.time2);
            addToBins(el.time3);
        }
    }
    // sort bins by average interval
    std::sort(bins.begin(), bins.end(), [](pulseBin a, pulseBin b) {
                                            return a.average < b.average; 
                                        });
    // Now traverse the elements again, filling in the transitions
    for (int n = 0; n < meaning.elements.size(); n++) {
        MeaningElement el = meaning.elements[n];
        if (el.type == PULSE) {
            // If we're about to write a low-state time, we need to fill the space before
            if (transitions.size() % 2) {
                // Which we use the prevous datablock's timing for, if applicable
                if (n > 0 && meaning.elements[n - 1].type == PPM) {
                    transitions.push_back(binFromTime(meaning.elements[n - 1].time3));
                } else if (n > 0 && meaning.elements[n - 1].type == PWM) {
                    transitions.push_back(binFromTime(meaning.elements[n - 1].time1));
                } else {
                    zap();
                    ERROR("ERROR: cannot have a pulse where a gap is expected at element %i.\n", n);
                    return false;
                }
            }
            transitions.push_back(binFromTime(el.time1));
        }
        if (el.type == GAP) {
            if (transitions.size() % 2 == 0) {
                zap();
                ERROR("ERROR: cannot have a gap where a pulse is expected at element %i.\n", n);
                return false;
            }
            transitions.push_back(binFromTime(el.time1));
        }
        if (el.type == PWM || el.type == PPM) {
            // Create a copy of el's data in tmp_data 
            int data_bytes = (el.data_len + 7) / 8;
            uint8_t tmp_data[data_bytes];
            for (int m = 0; m < data_bytes; m++) {
                tmp_data[m] = el.data[m];
            }
            // Shift left until the first bit that went in is
            // aligned in the MSB of the first byte.
            int shift_left_by = (8 - (el.data_len % 8)) % 8;
            for (int j = 0; j < shift_left_by; j++) {
                tools::shiftOutBit(tmp_data, el.data_len);
            }
            if (el.type == PWM) {
                for (int m = 0; m < el.data_len; m++) {
                    if (tools::shiftOutBit(tmp_data, el.data_len)) {
                        transitions.push_back(binFromTime(el.time2));
                        transitions.push_back(binFromTime(el.time1));
                    } else {
                        transitions.push_back(binFromTime(el.time1));
                        transitions.push_back(binFromTime(el.time2));                        
                    }
                }
            }
            if (el.type == PPM) {
                if (transitions.size() % 2) {
                    transitions.push_back(binFromTime(el.time3));
                }
                for (int m = 0; m < el.data_len; m++) {
                    if (tools::shiftOutBit(tmp_data, el.data_len)) {
                        transitions.push_back(binFromTime(el.time2));
                        transitions.push_back(binFromTime(el.time3));
                    } else {
                        transitions.push_back(binFromTime(el.time1));
                        transitions.push_back(binFromTime(el.time3));                        
                    }
                }
                transitions.pop_back();      // retract last filler, as this may be the end
            }
        }
    }
    // Now update bin counts, duration, repeats, gap.
    for (int transition : transitions) {
        bins[transition].count++;
        duration += bins[transition].average;
    }
    repeats = meaning.repeats;
    gap = meaning.gap;
    return true;
}

void Pulsetrain::addToBins(int time) {
    for (auto bin : bins) {
        if (bin.average == time || bins.size() == MAX_BINS) {
            return;
        }
    }
    pulseBin new_bin;
    new_bin.min = time;
    new_bin.max = time;
    new_bin.average = time;
    bins.push_back(new_bin);
    return;
}

int Pulsetrain::binFromTime(int time) {
    for (int m = 0; m < bins.size(); m++) {
        if (bins[m].average == time) {
            return m;
        }
    }
    return -1;
}
