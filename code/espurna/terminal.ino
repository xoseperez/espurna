/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if TERMINAL_SUPPORT

#include <vector>
#include "libs/EmbedisWrap.h"
#include <Stream.h>
#include "libs/StreamInjector.h"
#include "libs/HeapStats.h"

StreamInjector _serial = StreamInjector(TERMINAL_BUFFER_SIZE);
EmbedisWrap embedis(_serial, TERMINAL_BUFFER_SIZE);

#if SERIAL_RX_ENABLED
    char _serial_rx_buffer[TERMINAL_BUFFER_SIZE];
    static unsigned char _serial_rx_pointer = 0;
#endif // SERIAL_RX_ENABLED

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void _terminalHelpCommand() {

    // Get sorted list of commands
    std::vector<String> commands;
    unsigned char size = embedis.getCommandCount();
    for (unsigned int i=0; i<size; i++) {

        String command = embedis.getCommandName(i);
        bool inserted = false;
        for (unsigned char j=0; j<commands.size(); j++) {

            // Check if we have to insert it before the current element
            if (commands[j].compareTo(command) > 0) {
                commands.insert(commands.begin() + j, command);
                inserted = true;
                break;
            }

        }

        // If we could not insert it, just push it at the end
        if (!inserted) commands.push_back(command);

    }

    // Output the list
    DEBUG_MSG_P(PSTR("Available commands:\n"));
    for (unsigned char i=0; i<commands.size(); i++) {
        DEBUG_MSG_P(PSTR("> %s\n"), (commands[i]).c_str());
    }

}

void _terminalKeysCommand() {

    // Get sorted list of keys
    std::vector<String> keys = _settingsKeys();

    // Write key-values
    DEBUG_MSG_P(PSTR("Current settings:\n"));
    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), (keys[i]).c_str(), value.c_str());
    }

    unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
    UNUSED(freeEEPROM);
    DEBUG_MSG_P(PSTR("Number of keys: %d\n"), keys.size());
    DEBUG_MSG_P(PSTR("Current EEPROM sector: %u\n"), EEPROMr.current());
    DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);

}

void _terminalInitCommand() {

    terminalRegisterCommand(F("COMMANDS"), [](Embedis* e) {
        _terminalHelpCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("ERASE.CONFIG"), [](Embedis* e) {
        terminalOK();
        customResetReason(CUSTOM_RESET_TERMINAL);
        eraseSDKConfig();
        *((int*) 0) = 0; // see https://github.com/esp8266/Arduino/issues/1494
    });

    terminalRegisterCommand(F("FACTORY.RESET"), [](Embedis* e) {
        resetSettings();
        terminalOK();
    });

    terminalRegisterCommand(F("GPIO"), [](Embedis* e) {
        int pin = -1;

        if (e->argc < 2) {
            DEBUG_MSG("Printing all GPIO pins:\n");
        } else {
            pin = String(e->argv[1]).toInt();
            if (!gpioValid(pin)) {
                terminalError(F("Invalid GPIO pin"));
                return;
            }

            if (e->argc > 2) {
                bool state = String(e->argv[2]).toInt() == 1;
                digitalWrite(pin, state);
            }
        }

        for (int i = 0; i <= 15; i++) {
            if (gpioValid(i) && (pin == -1 || pin == i)) {
                DEBUG_MSG_P(PSTR("GPIO %s pin %d is %s\n"), GPEP(i) ? "output" : "input", i, digitalRead(i) == HIGH ? "HIGH" : "LOW");
            }
        }

        terminalOK();
    });

    terminalRegisterCommand(F("HEAP"), [](Embedis* e) {
        infoHeapStats();
        terminalOK();
    });

    terminalRegisterCommand(F("STACK"), [](Embedis* e) {
        infoMemory("Stack", CONT_STACKSIZE, getFreeStack());
        terminalOK();
    });

    terminalRegisterCommand(F("HELP"), [](Embedis* e) {
        _terminalHelpCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("INFO"), [](Embedis* e) {
        info();
        terminalOK();
    });

    terminalRegisterCommand(F("KEYS"), [](Embedis* e) {
        _terminalKeysCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("GET"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }

        for (unsigned char i = 1; i < e->argc; i++) {
            String key = String(e->argv[i]);
            String value;
            if (!Embedis::get(key, value)) {
                DEBUG_MSG_P(PSTR("> %s =>\n"), key.c_str());
                continue;
            }

            DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), key.c_str(), value.c_str());
        }

        terminalOK();
    });

    terminalRegisterCommand(F("RELOAD"), [](Embedis* e) {
        espurnaReload();
        terminalOK();
    });

    terminalRegisterCommand(F("RESET"), [](Embedis* e) {
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("RESET.SAFE"), [](Embedis* e) {
        systemStabilityCounter(SYSTEM_CHECK_MAX);
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("UPTIME"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
        terminalOK();
    });

    terminalRegisterCommand(F("CONFIG"), [](Embedis* e) {
        DynamicJsonBuffer jsonBuffer(1024);
        JsonObject& root = jsonBuffer.createObject();
        settingsGetJson(root);
        // XXX: replace with streaming
        String output;
        root.printTo(output);
        DEBUG_MSG(output.c_str());
        
    });

    #if not SETTINGS_AUTOSAVE
        terminalRegisterCommand(F("SAVE"), [](Embedis* e) {
            eepromCommit();
            terminalOK();
        });
    #endif

    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
        terminalRegisterCommand(F("MFLN.PROBE"), [](Embedis* e) {
            if (e->argc != 3) {
                terminalError(F("[url] [value]"));
                return;
            }

            URL _url(e->argv[1]);
            uint16_t requested_mfln = atol(e->argv[2]);

            auto client = std::make_unique<BearSSL::WiFiClientSecure>();
            client->setInsecure();

            if (client->probeMaxFragmentLength(_url.host.c_str(), _url.port, requested_mfln)) {
                terminalOK();
            } else {
                terminalError(F("Buffer size not supported"));
            }
        });
    #endif
    
}

void _terminalLoop() {

    #if DEBUG_SERIAL_SUPPORT
        while (DEBUG_PORT.available()) {
            _serial.inject(DEBUG_PORT.read());
        }
    #endif

    embedis.process();

    #if SERIAL_RX_ENABLED

        while (SERIAL_RX_PORT.available() > 0) {
            char rc = SERIAL_RX_PORT.read();
            _serial_rx_buffer[_serial_rx_pointer++] = rc;
            if ((_serial_rx_pointer == TERMINAL_BUFFER_SIZE) || (rc == 10)) {
                terminalInject(_serial_rx_buffer, (size_t) _serial_rx_pointer);
                _serial_rx_pointer = 0;
            }
        }

    #endif // SERIAL_RX_ENABLED

}

// -----------------------------------------------------------------------------
// Pubic API
// -----------------------------------------------------------------------------

void terminalInject(void *data, size_t len) {
    _serial.inject((char *) data, len);
}

void terminalInject(char ch) {
    _serial.inject(ch);
}


Stream & terminalSerial() {
    return (Stream &) _serial;
}

void terminalRegisterCommand(const String& name, void (*call)(Embedis*)) {
    Embedis::command(name, call);
};

void terminalOK() {
    DEBUG_MSG_P(PSTR("+OK\n"));
}

void terminalError(const String& error) {
    DEBUG_MSG_P(PSTR("-ERROR: %s\n"), error.c_str());
}

void terminalSetup() {

    _serial.callback([](uint8_t ch) {
        #if TELNET_SUPPORT
            telnetWrite(ch);
        #endif
        #if DEBUG_SERIAL_SUPPORT
            DEBUG_PORT.write(ch);
        #endif
    });

    _terminalInitCommand();

    #if SERIAL_RX_ENABLED
        SERIAL_RX_PORT.begin(SERIAL_RX_BAUDRATE);
    #endif // SERIAL_RX_ENABLED

    // Register loop
    espurnaRegisterLoop(_terminalLoop);

}

#endif // TERMINAL_SUPPORT 

