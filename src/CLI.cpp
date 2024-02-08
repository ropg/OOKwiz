#include "CLI.h"
#include "config.h"
#include "serial_output.h"
#include "tools.h"
#include "Settings.h"
#include "Radio.h"
#include "OOKwiz.h"

#define COMMAND(c)\
    tmp = c;\
    if (cmd.startsWith(tmp)) {\
        args = cmd.substring(tmp.length() + 1);\
        tools::trim(args);

#define END_CMD return; }


namespace CLI {

    bool cli_start_msg_printed = false;
    String serial_buffer;
    void parse(String cmd);

    void loop() {
        if (!cli_start_msg_printed) {
            INFO("CLI started on Serial. Type 'help' for list of commands.\n");
            cli_start_msg_printed = true;
        }
        while (Serial.available()) {
            char inp = Serial.read();
            if (inp == char(13) || inp == char(10) || inp == ';') {
                tools::trim(serial_buffer);
                if (serial_buffer != "") {
                    parse(serial_buffer);
                }   
                serial_buffer = "";
            } else {
                serial_buffer += inp;
            }
        }
    }

    void parse(String cmd) {

        String args;
        String tmp;

        INFO("\nCLI: %s\n", cmd.c_str());

        COMMAND("help")
INFO(R"(
OOKwiz version %s Command Line Interpreter help.

Available commands:

help               - prints this message
set                - shows current configuration settings
set x              - sets configuration flag x
set x y            - sets configuration value x to y
unset x            - unsets a flag or variable
load [<file>]      - loads the default saved settings, or from a named file in flash
save               - saves to a file named 'default', which is what is used at boot time.
save [<file>]      - saves the settings to a named file in SPIFFS flash
ls                 - lists stored configuration files in SPIFFS flash
rm <file>          - deletes a configuration file
reboot             - reboot using the saved defaults
standby            - set radio to standby mode
receive            - set radio to receive mode
sim <string>       - Takes a RawTimings, Pulsetrain or Meaning string representation and
                     acts like it just came in off the air.
transmit <string>  - Takes a RawTimings, Pulsetrain or Meaning string representation and
                     transmits it.

rm default;reboot  - restore factory settings
sr                 - shorthand for "save;reboot"


See the OOKwiz README.md on GitHub for a quick-start guide and full documentation

)", OOKWIZ_VERSION);
        END_CMD

        COMMAND("set")
            if (args == "") {
                INFO("%s\n", Settings::list().c_str());
                return;            
            }
            SPLIT(args, " ", name, value);
            // Works both with 'set x y' and 'set x=y'
            if (value == "") {
                tools::split(args, "=", name, value);
            }
            if (Settings::set(name, value)) {
                if (value != "") {
                    INFO("'%s' set to '%s'\n", name.c_str(), value.c_str());
                } else {
                    INFO("'%s' set\n", name.c_str());
                }
            }
        END_CMD

        COMMAND("unset")
            if (Settings::unset(args)) {
                INFO("Setting '%s' removed.\n", args.c_str());
            }
        END_CMD

        COMMAND("load")
            if (args == "") {
                args = "default";
            }
            Settings::load(args);
        END_CMD

        COMMAND("save")
            if (args == "") {
                args = "default";
            }
            Settings::save(args);
        END_CMD

        COMMAND("ls")
            Settings::ls();
        END_CMD

        COMMAND("rm")
            Settings::rm(args);
        END_CMD

        COMMAND("reboot")
            ESP.restart();
        END_CMD

        COMMAND("receive")
            if (OOKwiz::receive()) {
                INFO("Receiver active, waiting for pulses.\n");
            }
        END_CMD

        COMMAND("standby")
            if (OOKwiz::standby()) {
                INFO("Transceiver placed in standby mode.\n");
            }
        END_CMD

        COMMAND("transmit")
            OOKwiz::transmit(args);
        END_CMD

        COMMAND("sim")
            OOKwiz::simulate(args);
        END_CMD

        COMMAND("sr")
            if (Settings::save("default")) {
                ESP.restart();
            }
        END_CMD

        INFO("Unknown command '%s'. Enter 'help' for a list of commands.\n", cmd.c_str());
    }

}
