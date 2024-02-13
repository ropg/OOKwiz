#ifndef _CONFIG_H_
#define _CONFIG_H_

#define ARDUINO_LOOP_TASK_STACK_SIZE    (16 * 1024)
#define SERIAL_RX_BUFFER_SIZE           1024

#define OOKWIZ_VERSION          "0.1.4"
#define SPIFFS_PREFIX           /OOKwiz

#define MAX_BINS                10
#define MAX_MEANING_DATA        50
#define MAX_DEVICE_NAME_LEN     16
#define MAX_RADIO_NAME_LEN      16

// These need to be kept larger than number of devices and radios
// you want to load in DEVICE_INDEX and RADIO_INDEX respectively.
#define MAX_DEVICES             10
#define MAX_RADIOS              10

// The default runtime settings are in config.cpp
void factorySettings();

#endif
