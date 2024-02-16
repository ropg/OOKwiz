/*
    Prints payload of a very specific type of packet, ignores all else.
    See README under "Meaning" 
*/

#include <OOKwiz.h>

void setup() {
    Serial.begin(115200);
    OOKwiz::setup();
    OOKwiz::onReceive(receive);
}

void loop() {
    OOKwiz::loop();
}

void receive(RawTimings raw, Pulsetrain train, Meaning meaning) {
    if (
        meaning.elements.size() != 2 ||
        meaning.elements[0].type != PULSE ||
        !tools::between(meaning.elements[0].time1, 5800, 6000) ||
        meaning.elements[1].type != PWM ||
        !tools::between(meaning.elements[1].time1, 175, 205) ||
        !tools::between(meaning.elements[1].time2, 560, 590) ||
        meaning.elements[1].data_len != 24
    ) {
        return;
    }
    // .data() on an std::vector gives a pointer to the first element
    uint8_t *data = meaning.elements[1].data.data();
    Serial.printf("Data received 0:%02X 1:%02X 2:%02X\n", data[0], data[1], data[2]);
}
