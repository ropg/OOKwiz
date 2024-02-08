#include "config.h"
#include "Meaning.h"
#include "Pulsetrain.h"
#include "RawTimings.h"
#include "serial_output.h"
#include "tools.h"


// Helpful URL: https://gabor.heja.hu/blog/2020/03/16/receiving-and-decoding-433-mhz-radio-signals-from-wireless-devices/

bool Meaning::maybe(String str) {
    if (str.indexOf("(") != -1) {
        DEBUG("Meaning::maybe() returns true.\n");
        return true;
    }
    return false;
}

Meaning::operator bool() {
    return (elements.size() > 0);
}

void Meaning::zap() {
    elements.clear();
    suspected_incomplete = false;
}

bool Meaning::fromPulsetrain(Pulsetrain &train) {
    // Clear out current Meaning
    zap();
    // Easy stuff, just copy
    repeats = train.repeats;
    gap = train.gap;
    // Copy the bins from the Pulsetrain in prevalence
    typedef struct prevalence_t {
        int bin;
        int count;
    } prevalence_t;
    prevalence_t prevalence[train.bins.size()];
    for (int n = 0; n < train.bins.size(); n++) {
        prevalence[n].bin = n;
        prevalence[n].count = train.bins[n].count;
    }
    // bubblesort by .count, i.e. the number of times that length occurred. 
    prevalence_t temp;
    for (int i = 0; i < train.bins.size(); i++) {
        for (int j = 0; j < train.bins.size() - i - 1; j++) {
            if (prevalence[j].count < prevalence[j + 1].count) {
                temp = prevalence[j];
                prevalence[j] = prevalence[j + 1];
                prevalence[j + 1] = temp;
            }
        }
    }
    // If the two most prevalent pulse lengths are most of the signal, it's likely PWM
    bool likely_PWM = false;
    if (
        train.bins.size() >= 2 &&
        abs(prevalence[0].count - prevalence[1].count) <= 2 // &&
        // train.transitions.size() - (prevalence[0].count + prevalence[1].count) <= 5
    ) {
        likely_PWM = true;
        DEBUG("likely_PWM set.\n", likely_PWM);
    }
    // If the three most prevalent pulse lengths are most of the signal, prevalence of #2 and
    // #3 add up to the #1, and together they are most of the signal, it's likely PPM.
    bool likely_PPM = false;
    if (
        train.bins.size() >= 3 &&
        prevalence[0].count - (prevalence[1].count + prevalence[2].count) >= -2 &&
        prevalence[0].count - (prevalence[1].count + prevalence[2].count) <= 4 // &&
        // train.transitions.size() - (prevalence[0].count + prevalence[1].count + prevalence[2].count) <= 7
    ) {
        likely_PPM = true;
        DEBUG("likely_PPM set.\n", likely_PPM);
    }
    // If we don't know the modulation, we're not going to do anything useful, so give up
    if (!likely_PWM && !likely_PPM) {
        DEBUG("Could not parse Pulsetrain, no likely modulation found.\n");
        return false;
    }
    // Now we walk the train's transitions and decipher
    for (int n = 0; n < train.transitions.size(); n++) {
        int r;
        if (likely_PWM) {
            r = parsePWM(train, n, train.transitions.size() - 1, prevalence[0].bin, prevalence[1].bin);
        } else if (likely_PPM) {
            r = parsePPM(train, n, train.transitions.size() - 1, prevalence[1].bin, prevalence[2].bin, prevalence[0].bin);
        }
        if (r == -1) {
            return false;
        } else if (r > 0) {
            n += r - 1;  // Minus 1 bc the next for loop will also increment 1
            continue;
        }
        // Otherwise add as a pulse or gap
        if (n % 2 == 0) {
            if (!addPulse(train.bins[train.transitions[n]].average)) {
                return false;
            }
        } else {
            if (!addGap(train.bins[train.transitions[n]].average)) {
                return false;
            }
        }
    }
    if (train.repeats > 1) {
        suspected_incomplete = false;
    }
    return (elements.size() > 0);
}

int Meaning::parsePWM(const Pulsetrain &train, int from, int to, int space, int mark) {
    DEBUG ("Entered parsePWM with from: %i, to: %i space: %i, mark: %i\n", from, to, space, mark);
    uint8_t tmp_data[MAX_MEANING_DATA] = { 0 };
    int transitions_parsed = 0;
    int num_bits = 0;
    for (int n = from; n <= to; n += 2) {
        int current = train.transitions[n];
        int next = train.transitions[n + 1];
        if (current == space && next == mark) {
            num_bits++;
            tools::shiftInBit(tmp_data, num_bits, 0);
            transitions_parsed += 2;
        } else if (current == mark && next == space) {
            num_bits++;
            tools::shiftInBit(tmp_data, num_bits, 1);
            transitions_parsed += 2;
        } else {
            break;
        }
    }
    if (num_bits % 4 != 0) {
        suspected_incomplete = true;
    }
    if (num_bits >= 8) {
        MeaningElement new_element;
        new_element.data_len = num_bits;
        int len_in_bytes = (num_bits + 7) / 8;
        // Write data
        for (int n = 0; n < len_in_bytes; n++) {
            new_element.data.insert(new_element.data.begin(), tmp_data[n]); // reverses order
        }
        new_element.type = PWM;
        new_element.time1 = train.bins[space].average;
        new_element.time2 = train.bins[mark].average;
        elements.push_back(new_element);
        return transitions_parsed;
    } else {
        return 0;
    }
}

int Meaning::parsePPM(const Pulsetrain &train, int from, int to, int space, int mark, int filler) {
    DEBUG ("Entered parsePPM with from: %i, to: %i space: %i, mark: %i, filler: %i\n", from, to, space, mark, filler);
    uint8_t tmp_data[MAX_MEANING_DATA] = { 0 };
    int transitions_parsed = 0;
    int num_bits = 0;
    int previous = -1;
    for (int n = from; n <= to; n++) {
        int current = train.transitions[n];
        if (current == space && previous == filler) {
            num_bits++;
            tools::shiftInBit(tmp_data, num_bits, 0);
            transitions_parsed++;
        } else if (current == mark && previous == filler) {
            num_bits++;
            tools::shiftInBit(tmp_data, num_bits, 1);
            transitions_parsed++;
        } else if (current == filler) {
            if (previous == filler) {
                break;
            }
            transitions_parsed++;
        } else {
            break;
        }
        previous = current;
    }
    if (num_bits % 4 != 0) {
        suspected_incomplete = true;
    }
    if (num_bits >= 8) {
        MeaningElement new_element;
        new_element.data_len = num_bits;
        int len_in_bytes = (num_bits + 7) / 8;
        // Write data
        for (int n = 0; n < len_in_bytes; n++) {
            new_element.data.insert(new_element.data.begin(), tmp_data[n]); // reverses order
        }
        new_element.type = PPM;
        new_element.time1 = train.bins[space].average;
        new_element.time2 = train.bins[mark].average;
        new_element.time3 = train.bins[filler].average;
        elements.push_back(new_element);
        return transitions_parsed;
    } else {
        return 0;
    }
}

bool Meaning::addPulse(uint16_t pulse_time) {
    MeaningElement new_element;
    new_element.type = PULSE;
    new_element.time1 = pulse_time;
    elements.push_back(new_element);
    return true;
}

bool Meaning::addGap(uint16_t gap_time) {
    MeaningElement new_element;
    new_element.type = GAP;
    new_element.time1 = gap_time;
    elements.push_back(new_element);
    return true;
}

String Meaning::toString() {
    String res = "";
    for (const auto& element : elements) {
        switch (element.type) {
            case PULSE:
                snprintf_append(res, 20, "pulse(%i)", element.time1);
                break;
            case GAP:
                snprintf_append(res, 20, "gap(%i)", element.time1);
                break;
            case PWM:
                snprintf_append(res, 60, "pwm(timing %i/%i, %i bits 0x", element.time1, element.time2, element.data_len);
                for (int m = 0; m < (element.data_len + 7) / 8; m++) {
                    snprintf_append(res, 5, "%02X", element.data[m]);
                }
                res = res + ")";
                break;
            case PPM:
                snprintf_append(res, 60, "ppm(timing %i/%i/%i, %i bits 0x", element.time1, element.time2, element.time3, element.data_len);
                for (int m = 0; m < (element.data_len + 7) / 8; m++) {
                    snprintf_append(res, 5, "%02X", element.data[m]);
                }
                res = res + ")";
                break;
        }
        res += " + ";
    }
    res = res.substring(0, res.length() - 3);    // cut off the last " + "
    if (repeats > 1) {
        snprintf_append(res, 40, "  Repeated %i times with %i µs gap.", repeats, gap);
    }
    if (suspected_incomplete) {
        res = res + " (SUSPECTED INCOMPLETE)";
    }
    return res;
}

bool Meaning::fromString(String in) {
    in.toLowerCase();
    repeats = 1;
    int rptd = in.indexOf("repeated");
    if (rptd != -1) {
        String str_repeats = in.substring(rptd);
        repeats = tools::nthNumberFrom(str_repeats, 0);
        gap = tools::nthNumberFrom(str_repeats, 1);
        in = in.substring(0, rptd);
        if (repeats == 0 or gap == 0) {
            ERROR("ERROR: cannot convert String to Meaning: invalid values for repeats or gap.\n");
            return false;            
        }
    }
    String work;
    bool done = false;
    while (!done) {
        int plus = in.indexOf("+");
        if (plus != -1) {
            work = in.substring(0, plus);
            in = in.substring(plus + 1);
        } else {
            work = in;
            done = true;
        }
        tools::trim(work);
        int open_bracket = work.indexOf("(");
        int closing_bracket = work.indexOf(")");
        if (open_bracket == -1 || closing_bracket == -1) {
            ERROR("ERROR: cannot convert String to Meaning: incorrect element '%s'.\n", work);
            return false;
        }
        if (work.startsWith("pulse")) {
            int num = tools::nthNumberFrom(work, 0);
            if (num == -1) {
                ERROR("ERROR: cannot convert String to Meaning: no length found in '%s'.\n", work);
                return false;
            }
            addPulse(num);
        }
        if (work.startsWith("gap")) {
            int num = tools::nthNumberFrom(work, 0);
            if (num == -1) {
                ERROR("ERROR: cannot convert String to Meaning: no length found in '%s'.\n", work);
                return false;
            }
            addGap(num);
        }
        if (work.startsWith("ppm")) {
            int time1 = tools::nthNumberFrom(work, 0);
            int time2 = tools::nthNumberFrom(work, 1);
            int time3 = tools::nthNumberFrom(work, 2);
            int bits = tools::nthNumberFrom(work, 3);
            int check_zero = tools::nthNumberFrom(work, 4);
            if (time1 < 1 || time2 < 1 || time3 < 1 || check_zero != 0) {
                ERROR("ERROR: cannot convert String to Meaning: '%s' malformed.\n", work);
                return false;
            }
            int data_start = work.indexOf("0x");
            int data_end = work.indexOf(")");
            if (data_start == -1 || data_end < data_start) {
                ERROR("ERROR: cannot convert String to Meaning: '%s' malformed.\n", work);
                return false;
            }
            String hex_data = work.substring(data_start + 2, data_end);
            int bytes_expected = (bits + 7) / 8;
            if (hex_data.length() != bytes_expected * 2) {
                ERROR("ERROR: cannot convert String to Meaning: %i bits means %i data bytes in hex expected.\n", bits, bytes_expected);
                return false;
            }
            uint8_t tmp_data[bytes_expected];
            for (int n = 0; n < bytes_expected; n++) {
                tmp_data[n] = strtoul(hex_data.substring(n * 2, (n * 2) + 2).c_str(), nullptr, 16);
            }
            addPPM(time1, time2, time3, bits, tmp_data);
        }
        if (work.startsWith("pwm")) {
            int time1 = tools::nthNumberFrom(work, 0);
            int time2 = tools::nthNumberFrom(work, 1);
            int bits = tools::nthNumberFrom(work, 2);
            int check_zero = tools::nthNumberFrom(work, 3);
            if (time1 < 1 || time2 < 1 || check_zero != 0) {
                ERROR("ERROR: cannot convert String to Meaning: '%s' malformed.\n", work);
                return false;
            }
            int data_start = work.indexOf("0x");
            int data_end = work.indexOf(")");
            if (data_start == -1 || data_end < data_start) {
                ERROR("ERROR: cannot convert String to Meaning: '%s' malformed.\n", work);
                return false;
            }
            String hex_data = work.substring(data_start + 2, data_end);
            tools::trim(hex_data);
            int bytes_expected = (bits + 7) / 8;
            if (hex_data.length() != bytes_expected * 2) {
                ERROR("ERROR: cannot convert String to Meaning: %i bits means %i data bytes in hex expected.\n", bits, bytes_expected);
                return false;
            }
            uint8_t tmp_data[bytes_expected];
            for (int n = 0; n < bytes_expected; n++) {
                tmp_data[n] = strtoul(hex_data.substring(n * 2, (n * 2) + 2).c_str(), nullptr, 16);
            }
            addPWM(time1, time2, bits, tmp_data);
        }
    }
    return true;
}

bool Meaning::addPPM(int space, int mark, int filler, int bits, uint8_t* tmp_data) {
    MeaningElement new_element;
    int len_in_bytes = (bits + 7) / 8;
    new_element.data_len = bits;
    for (int n = 0; n < len_in_bytes; n++) {
        new_element.data.push_back(tmp_data[n]);
    }
    new_element.type = PPM;
    new_element.time1 = space;
    new_element.time2 = mark;
    new_element.time3 = filler;
    elements.push_back(new_element);
    return true;
}

bool Meaning::addPWM(int space, int mark, int bits, uint8_t* tmp_data) {
    MeaningElement new_element;
    int len_in_bytes = (bits + 7) / 8;
    new_element.data_len = bits;
    for (int n = 0; n < len_in_bytes; n++) {
        new_element.data.push_back(tmp_data[n]);
    }
    new_element.type = PWM;
    new_element.time1 = space;
    new_element.time2 = mark;
    elements.push_back(new_element);
    return true;
}
