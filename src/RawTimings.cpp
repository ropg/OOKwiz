#include "RawTimings.h"
#include "Pulsetrain.h"
#include "serial_output.h"
#include "Settings.h"


/// @brief Static method to see if String might be a representation of RawTimings. No guarantees until you try to convert it, but silent.
/// @param str String that we are curious about
/// @return `true` if it might be a RawTimings String, `false` if not.
/**
 * Static, so not called on any particular RawTimings instance but instead like:
 * ```cpp
 * if (RawTimings::maybe(someString)) {
 *     ....
 * }
 * ```
*/

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

/// @brief If you try to evaluate the instance as a bool, for instance in `if (myRawTimings) ...`, it will be `true` if there's intervals stored.
RawTimings::operator bool() {
    return (intervals.size() > 0);
}

/// @brief empty out the stored intervals
void RawTimings::zap() {
    intervals.clear();
}

/// @brief Get the String representation, which is a comma-separated list of intervals
/// @return the String representation
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

/// @brief Read a String representation, which is a comma-separated list of intervals, and store in this instance
/// @return `true` if it worked, `false` (with error message) if it didn't.
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

/// @brief Convert Pulsetrain into RawTimings. Loses stats about bins as well as information about repeats.
/// @param train the Puksetrain you want to convert from
/// @return Always `true`
bool RawTimings::fromPulsetrain(Pulsetrain &train) {
    for (int transition : train.transitions) {
        intervals.push_back(train.bins[transition].average);
    }
    return true;
}

/// @brief Returns the viasualizer (the blocky time-graph) for the pulses in this RawTimings instance
/// @param base µs per (half-character) block. Every interval gets at least one block so all pulses are guaranteed visible
/// @return visualizer String
String RawTimings::visualizer(int base) {
    if (base == 0) {
        return "";
    }
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

/// @brief The visualizer like above, with base taken from `visualizer_pixel` setting.
/// @return visualizer String
String RawTimings::visualizer() {
    int visualizer_pixel;
    SETTING_WITH_DEFAULT(visualizer_pixel, 200);
    return visualizer(visualizer_pixel);
}
