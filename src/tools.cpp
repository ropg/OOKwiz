#include "tools.h"
#include "config.h"

size_t getArduinoLoopTaskStackSize() {
    return ARDUINO_LOOP_TASK_STACK_SIZE;
}

namespace tools {

    /// @brief Takes spaces off start and end of supplied string, operates in place on String passed in.
    /// @param in (String) The string in question, will be modified if need be.
    void trim(String &in) {
        while (in.startsWith(" ")) {
            in = in.substring(1);
        }
        while (in.endsWith(" ")) {
            in = in.substring(0, in.length() - 1);
        }
    }

    /// @brief Identifies contiguous series of digits in String, giving you the toInt() of the nth one.
    /// @param in (String) The string we're looking at.
    /// @param num (int) Which series of numbers you'd like evaluated.
    /// @return (long) The toInt() value you wre looking for, or -1 if there weren't that many series of digits.
    long nthNumberFrom(String &in, int num) {
        int index = 0;
        bool on_numbers = false;
        int count = 0;
        while (index < in.length()) {
            if (isDigit(in.charAt(index)) and !on_numbers) {
                on_numbers = true;
                if (count == num) {
                    return in.substring(index).toInt();
                }
            }
            if (!isDigit(in.charAt(index)) and on_numbers) {
                on_numbers = false;
                count++;
            }
            index++;
        }
        return -1;
    }

    /// @brief Shifts bits into a range of bytes. Reverse order because length may be unkown at start.
    /// @param buf pointer to uint8_t array with enough space for the bits to me shifted in.
    /// @param len number of bits that have been shifted in so far.
    /// @param bit bool holding the bit to be shifted in this time
    void shiftInBit(uint8_t* buf, const int len, const bool bit) {
        int len_bytes = (len + 7) / 8;
        bool carry = bit;
        for (int n = 0; n < len_bytes; n++) {
            bool new_carry = buf[n] & 128;
            buf[n] <<= 1;
            buf[n] |= carry;
            carry = new_carry;
        }
    }

    /// @brief Shifts out the bits in an array of bytes, emptying array in process.
    /// @param buf (uint8_t*) pointer to array, MSB of leftmost byte gets shifted out first.
    /// @param len (int) Number of bits in buffer in total.
    /// @return (bool) the bit shifted out.
    bool shiftOutBit(uint8_t* buf, const int len) {
        int len_bytes = (len + 7) / 8;
        bool carry = 0;
        for (int n = len_bytes - 1; n >= 0; n--) {
            bool new_carry = buf[n] & 128;
            buf[n] <<= 1;
            buf[n] |= carry;
            carry = new_carry;
        }
        return carry;
    }

    /// @brief Will split a srting in two on first occurence of String separator. 
    /// @param in String to be examined
    /// @param separator substring to be found
    /// @param before Will be set to part before first occurence of separator 
    /// @param after Will be set to part after first occurence of separator 
    void split(const String &in, const String &separator, String &before, String &after) {
        int found = in.indexOf(separator);
        if (found != -1) {
            after = in.substring(found + separator.length());
            trim(after);
            before = in.substring(0, found);
            trim(before);
        } else {
            before = in;
            after = "";
        }
    }

    /// @brief See if number is between two bounds
    /// @param compare Number to compare
    /// @param lower_bound lower bound
    /// @param upper_bound upper bound
    /// @return `true` if compare >= lower and <= upper bound
    bool between(const int &compare, const int &lower_bound, const int &upper_bound) {
        if (compare >= lower_bound && compare <= upper_bound) {
            return true;
        }
        return false;
    }

}
