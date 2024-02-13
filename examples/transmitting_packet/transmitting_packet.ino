/*
    Tranmits a specific type of packet. See README under "Meaning" 
*/

#include <OOKwiz.h>

void setup() {
    Serial.begin(115200);
    if (OOKwiz::setup()) {
        Meaning newPacket;
        newPacket.addPulse(5900);
        uint8_t data[3] = {0x17, 0x72, 0xA4};
        // 190 and 575 are space and mark timings, 24 is data length in bits
        newPacket.addPWM(190, 575, 24, data);
        newPacket.repeats = 6;
        newPacket.gap = 130;    
        OOKwiz::transmit(newPacket);
    }
}

void loop() {
    OOKwiz::loop();
}
