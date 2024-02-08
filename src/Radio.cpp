#include "Radio.h"
#include "serial_output.h"
#include "tools.h"
#include "radio_plugins/RADIO_INDEX"


// static members
decltype(Radio::store) Radio::store;
int Radio::len = 0;
Radio* Radio::current = nullptr;
int Radio::pin_rx;
int Radio::pin_tx;

// Uses char*, and does not DEBUG or INFO because this is ran pre-main by the 
// constructor of the AutoRegister trick: String and Serial are not available yet. 
bool Radio::add(const char* name, Radio *pointer) {
    if (len == MAX_RADIOS) {
        return false;
    }
    strncpy(store[len].name, name, MAX_RADIO_NAME_LEN);
    store[len].name[MAX_RADIO_NAME_LEN] = 0;   // just in case
    store[len].pointer = pointer;
    len++;
    return true;
}

bool Radio::setup() {
    MANDATORY(radio);
    INFO("Radio plugins loaded: %s\n", list().c_str());
    if (select(Settings::getString("radio"))) {
        return radio_init();
    }
    return false;
}

bool Radio::select(const String &name) {
    for (int n = 0; n < len; n++) {
        if (strcmp(store[n].name, name.c_str()) == 0) {
            current = store[n].pointer;
            INFO("Radio %s selected.\n", name);
            return true;
        }
    }
    ERROR("No such radio: '%s'.\n", name.c_str());
    return false;
}

String Radio::list(String separator) {
    String ret;
    for (int n = 0; n < len; n++) {
        ret += store[n].name;
        if (n < len - 1) {
            ret += separator;
        }
    }
    return ret;
}

String Radio::name() {
    for (int n = 0; n < len; n++) {
        if (store[n].pointer == this) {
            return String(store[n].name);
        }
    }
}

bool Radio::radio_init() {
    CHECK_RADIO_SET;
    INFO("Initializing radio.\n");
    pin_rx = Settings::getInt("pin_rx");
    pin_tx = Settings::getInt("pin_tx");
    return current->init();
}

bool Radio::radio_rx() {
    CHECK_RADIO_SET;
    DEBUG("Configuring radio for receiving.\n");
    if (pin_rx < 0) {
        ERROR("ERROR: pin_rx needs to be set receive.\n");
        return false;
    }
    PIN_MODE(pin_rx, INPUT);
    return current->rx();
}

bool Radio::radio_tx() {
    CHECK_RADIO_SET;
    DEBUG("Configuring radio for transmission.\n");
    if (pin_tx < 0) {
        ERROR("ERROR: pin_tx needs to be set for transmit.\n");
        return false;
    }
    PIN_MODE(pin_tx, OUTPUT);
    PIN_WRITE(pin_tx, !Settings::isSet("tx_active_high"));
    return current->tx();
}

bool Radio::radio_standby() {
    CHECK_RADIO_SET;
    DEBUG("Radio entering standby mode.\n");
    return current->standby();
}

bool Radio::init() {
    return false;
}

bool Radio::rx() {
    return false;
}

bool Radio::tx() {
    return false;
}

bool Radio::standby() {
    return false;
}

// RadioLib-specific

void Radio::radiolibInit() {
    int pin_sck;
    SETTING_WITH_DEFAULT(pin_sck, -1);
    int pin_miso;
    SETTING_WITH_DEFAULT(pin_miso, -1);
    int pin_mosi;
    SETTING_WITH_DEFAULT(pin_mosi, -1);
    int pin_reset;
    SETTING_WITH_DEFAULT(pin_reset, -1);
    String spi_port;
    SETTING_WITH_DEFAULT(spi_port, "HSPI");
    int spi_port_int = -1;
    if (spi_port == "HSPI") {
        spi_port_int = HSPI;
    } else if (spi_port == "FSPI") {
        spi_port_int = FSPI;
    }
    #ifdef VSPI
        else if (spi_port == "VSPI") {
            spi_port_int = VSPI;
        }
    #endif
    else {
        ERROR("SPI port '%s' unknown, trying default SPI.\n", spi_port.c_str());
    }
    if (spi_port_int != -1 && pin_miso != -1 && pin_mosi != -1 && pin_sck != -1) {
        INFO("Radio %s: SPI port %s, SCK %i, MISO %i, MOSI %i, CS %i, RESET %i, RX %i, TX %i\n", name().c_str(), spi_port.c_str(), pin_sck, pin_miso, pin_mosi, pin_cs, pin_reset, pin_rx, pin_tx);
        spi = new SPIClass(spi_port_int);
        spi->begin(pin_sck, pin_miso, pin_mosi, pin_cs);
        radioLibModule = new Module(pin_cs, -1, pin_reset, -1, *spi);
    } else {
        INFO("Radio %s: default SPI, SCK %i, MISO %i, MOSI %i, CS %i, RESET %i, RX %i, TX %i\n", name().c_str(), SCK, MISO, MOSI, pin_cs, pin_reset, pin_rx, pin_tx);
        radioLibModule = new Module(pin_cs, -1, pin_reset, -1);
    }
    INFO("%s: Frequency: %.2f Mhz, bandwidth %.1f kHz, bitrate %.3f kbps\n", name().c_str(), frequency, bandwidth, bitrate);
}

void Radio::showRadiolibResult(const int result, const char* action) {
    switch (result) {
    case 0:
        DEBUG("%s: %s returned 0 (OK)\n", name().c_str(), action);
        break;
    case -2:
        ERROR("%s ERROR: %s returned -2 (CHIP NOT FOUND)\n", name().c_str(), action);
        break;
    default:
        ERROR("%s ERROR: %s returned %i\n%s\n", name().c_str(), action, result,
            "(See https://github.com/jgromes/RadioLib/blob/master/src/TypeDef.h for Meaning of RadioLib error codes.)");
        break;
    }
}

int Radio::thresholdSetup(const int fixed, const int average, const int peak) {
    String threshold_type;
    SETTING_WITH_DEFAULT(threshold_type, "peak");
    SETTING_WITH_DEFAULT(threshold_level, 6);
    INFO("%s: Threshold type %s, level %i\n", name().c_str(), threshold_type.c_str(), threshold_level);
    if (threshold_type == "fixed") {
        return fixed;
    } else if (threshold_type == "average") {
        return average;
    } else {
        return peak;
    }
}
