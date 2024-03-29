/*
    This will allow you to access the Command Line Interpreter from the Serial
    Monitor, letting you set up a radio and receive and transmit packets.
*/

#include <OOKwiz.h>

void setup() {
    Serial.begin(115200);
    OOKwiz::setup();
}

void loop() {
    OOKwiz::loop();
}
