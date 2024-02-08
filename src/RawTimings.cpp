#include "RawTimings.h"
#include "Pulsetrain.h"
#include "serial_output.h"
#include "Settings.h"


bool RawTimings::maybe(String str) {
    int comma = 0;
    for (int n = 0; n < str.length(); n++) {
        if (!isDigit(str.charAt(n)) && str.charAt(n) != ',') {
            return false;
        }
        if (str.charAt(n) == ',') {
            comma++;
        }
    }
    if (comma > 15) {
        DEBUG("RawTimings::maybe() returns true.\n");
        return true;
    }
    return false;
}

RawTimings::operator bool() {
    return (intervals.size() > 0);
}

void RawTimings::zap() {
    intervals.clear();
}

String RawTimings::toString() {
    String res = "";
    for (int count = 0; count < intervals.size(); count++) {
        res += intervals[count];
        if (count != intervals.size() - 1) {
            res += ",";
        }
    }
    return res;
}

bool RawTimings::fromString(const String &in) {
    bool error = false;
    intervals.clear();
    int pos = 0;
    int nextSemicolon = in.indexOf(",", pos);
    do {
        int value = in.substring(pos, nextSemicolon).toInt();
        if (value == 0) {
            error = true;
            break;
        }
        intervals.push_back(value);
        pos = nextSemicolon + 1;
        nextSemicolon = in.indexOf(",", pos);
    } while (nextSemicolon != -1);
    int value = in.substring(pos).toInt();
    if (value == 0 || error) {
        ERROR("Conversion to RawTimings failed at position %i in String '%s'\n", pos, in.c_str());
        return false;
    }
    intervals.push_back(value);
    return true;
}

bool RawTimings::fromPulsetrain(Pulsetrain &train) {
    for (int transition : train.transitions) {
        intervals.push_back(train.bins[transition].average);
    }
}

String RawTimings::visualizer() {
    int visualizer_pixel;
    SETTING_WITH_DEFAULT(visualizer_pixel, 200);
    return visualizer(visualizer_pixel);
}

String RawTimings::visualizer(int base) {
    String ones_and_zeroes;
    String curstate;
    for (int n = 0; n < intervals.size(); n++) {
        curstate = (n % 2 == 0) ? "1" : "0";
        for (int m = 0; m < max((intervals[n] + (base / 2)) / base, 1); m++) {
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
