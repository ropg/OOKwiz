#include "CLI.h"
#include "config.h"
#include "serial_output.h"
#include "tools.h"
#include "Settings.h"
#include "Radio.h"
#include "OOKwiz.h"


namespace CLI {

    bool cli_start_msg_printed = false;
    bool semicolon_parsing = true;
    String serial_buffer;
    void parse(String cmd);
    void RFlinkParse(String cmd);

    void loop() {
        if (!cli_start_msg_printed) {
            INFO("CLI started on Serial. Type 'help' for list of commands.\n");
            cli_start_msg_printed = true;
        }
        while (Serial.available()) {
            char inp = Serial.read();
            if (inp == ';' && serial_buffer == "10") {
                // RFlink format uses semicolons, so when command starts with "10;",keep it together until eol
                semicolon_parsing = false;
            }
            if (inp == char(13) || inp == char(10) || (inp == ';' && semicolon_parsing)) {
                semicolon_parsing = true;
                tools::trim(serial_buffer);
                if (serial_buffer != "") {
                    if (serial_buffer.startsWith("10;")) {
                        RFlinkParse(serial_buffer);
                    } else {
                        parse(serial_buffer);
                    }
                }   
                serial_buffer = "";
            } else {
                serial_buffer += inp;
            }
        }
    }

    void parse(String cli_string) {
        INFO("\nCLI: %s\n", cli_string.c_str());
        String cmd;
        String args;
        tools::split(cli_string, " ", cmd, args);

        if (cmd == "help") {
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
            return;
        }

        if (cmd == "set") {
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
            return;
        }

        if (cmd == "unset") {
            if (Settings::unset(args)) {
                INFO("Setting '%s' removed.\n", args.c_str());
            }
            return;
        }

        if (cmd == "load") {
            if (args == "") {
                args = "default";
            }
            Settings::load(args);
            return;
        }

        if (cmd == "save") {
            if (args == "") {
                args = "default";
            }
            Settings::save(args);
            return;
        }

        if (cmd == "ls") {
            Settings::ls();
            return;
        }

        if (cmd == "rm") {
            Settings::rm(args);
            return;
        }

        if (cmd == "reboot") {
            ESP.restart();
            return;
        }

        if (cmd == "receive") {
            if (OOKwiz::receive()) {
                INFO("Receiver active, waiting for pulses.\n");
            }
            return;
        }

        if (cmd == "standby") {
            if (OOKwiz::standby()) {
                INFO("Transceiver placed in standby mode.\n");
            }
            return;
        }

        if (cmd == "transmit") {
            OOKwiz::transmit(args);
            return;
        }

        if (cmd == "sim") {
            OOKwiz::simulate(args);
            return;
        }

        if (cmd == "sr") {
            if (Settings::save("default")) {
                ESP.restart();
            }
            return;
        }

        INFO("Unknown command '%s'. Enter 'help' for a list of commands.\n", cmd.c_str());
    }

    void RFlinkParse(String cmd) {
        INFO("\nCLI: %s\n", cmd.c_str());
        cmd = cmd.substring(3); // get rid of "10;"

        // This may parse further commands later, now it just assumes the first term in semicolons
        // after "10;" is a plugin name and the rest is what is to be transmitted.
        String plugin_name;
        String txString;
        tools::split(cmd, ";", plugin_name, txString);
        Device::transmit(plugin_name, txString);
    }

}
