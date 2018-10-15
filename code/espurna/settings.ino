/*

SETTINGS MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM_Rotate.h>
#include <vector>
#include "libs/EmbedisWrap.h"
#include <Stream.h>
#include "libs/StreamInjector.h"

StreamInjector _serial = StreamInjector(TERMINAL_BUFFER_SIZE);
EmbedisWrap embedis(_serial, TERMINAL_BUFFER_SIZE);

#if TERMINAL_SUPPORT
#if SERIAL_RX_ENABLED
    char _serial_rx_buffer[TERMINAL_BUFFER_SIZE];
    static unsigned char _serial_rx_pointer = 0;
#endif // SERIAL_RX_ENABLED
#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// Reverse engineering EEPROM storage format
// -----------------------------------------------------------------------------

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos + EEPROM_DATA_END;
}

// -----------------------------------------------------------------------------

unsigned int settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
        count ++;
    }
    return count;
}

String settingsKeyName(unsigned int index) {

    String s;

    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        if (count == index) {
            s.reserve(len);
            for (unsigned char i = 0 ; i < len; i++) {
                s += (char) EEPROMr.read(pos + i + 1);
            }
            break;
        }
        count++;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
    }

    return s;

}

std::vector<String> _settingsKeys() {

    // Get sorted list of keys
    std::vector<String> keys;

    //unsigned int size = settingsKeyCount();
    unsigned int size = settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {

        //String key = settingsKeyName(i);
        String key = settingsKeyName(i);
        bool inserted = false;
        for (unsigned char j=0; j<keys.size(); j++) {

            // Check if we have to insert it before the current element
            if (keys[j].compareTo(key) > 0) {
                keys.insert(keys.begin() + j, key);
                inserted = true;
                break;
            }

        }

        // If we could not insert it, just push it at the end
        if (!inserted) keys.push_back(key);

    }

    return keys;
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void _settingsHelpCommand() {

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

void _settingsKeysCommand() {

    // Get sorted list of keys
    std::vector<String> keys = _settingsKeys();

    // Write key-values
    DEBUG_MSG_P(PSTR("Current settings:\n"));
    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), (keys[i]).c_str(), value.c_str());
    }

    unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
    DEBUG_MSG_P(PSTR("Number of keys: %d\n"), keys.size());
    DEBUG_MSG_P(PSTR("Current EEPROM sector: %u\n"), EEPROMr.current());
    DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);

}

void _settingsFactoryResetCommand() {
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROMr.write(i, 0xFF);
    }
    EEPROMr.commit();
}

void _settingsInitCommands() {

    #if DEBUG_SUPPORT
        settingsRegisterCommand(F("CRASH"), [](Embedis* e) {
            debugDumpCrashInfo();
            debugClearCrashInfo();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif

    settingsRegisterCommand(F("COMMANDS"), [](Embedis* e) {
        _settingsHelpCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("ERASE.CONFIG"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        resetReason(CUSTOM_RESET_TERMINAL);
        _eepromCommit();
        ESP.eraseConfig();
        *((int*) 0) = 0; // see https://github.com/esp8266/Arduino/issues/1494
    });

    #if I2C_SUPPORT

        settingsRegisterCommand(F("I2C.SCAN"), [](Embedis* e) {
            i2cScan();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });

        settingsRegisterCommand(F("I2C.CLEAR"), [](Embedis* e) {
            i2cClearBus();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });

    #endif

    settingsRegisterCommand(F("FACTORY.RESET"), [](Embedis* e) {
        _settingsFactoryResetCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("GPIO"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
            return;
        }
        int pin = String(e->argv[1]).toInt();
        //if (!gpioValid(pin)) {
        //    DEBUG_MSG_P(PSTR("-ERROR: Invalid GPIO\n"));
        //    return;
        //}
        if (e->argc > 2) {
            bool state = String(e->argv[2]).toInt() == 1;
            digitalWrite(pin, state);
        }
        DEBUG_MSG_P(PSTR("GPIO %d is %s\n"), pin, digitalRead(pin) == HIGH ? "HIGH" : "LOW");
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("HEAP"), [](Embedis* e) {
        infoMemory("Heap", getInitialFreeHeap(), getFreeHeap());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("STACK"), [](Embedis* e) {
        infoMemory("Stack", 4096, getFreeStack());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("HELP"), [](Embedis* e) {
        _settingsHelpCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("INFO"), [](Embedis* e) {
        info();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("KEYS"), [](Embedis* e) {
        _settingsKeysCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("GET"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
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

        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    #if WEB_SUPPORT
        settingsRegisterCommand(F("RELOAD"), [](Embedis* e) {
            espurnaReload();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif

    settingsRegisterCommand(F("RESET"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    settingsRegisterCommand(F("RESET.SAFE"), [](Embedis* e) {
        EEPROMr.write(EEPROM_CRASH_COUNTER, SYSTEM_CHECK_MAX);
        DEBUG_MSG_P(PSTR("+OK\n"));
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    settingsRegisterCommand(F("UPTIME"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("CONFIG"), [](Embedis* e) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        settingsGetJson(root);
        String output;
        root.printTo(output);
        DEBUG_MSG(output.c_str());
        DEBUG_MSG_P(PSTR("\n+OK\n"));
    });

    #if not SETTINGS_AUTOSAVE
        settingsRegisterCommand(F("SAVE"), [](Embedis* e) {
            eepromCommit();
            DEBUG_MSG_P(PSTR("\n+OK\n"));
        });
    #endif
    
}

// -----------------------------------------------------------------------------
// Key-value API
// -----------------------------------------------------------------------------

void moveSetting(const char * from, const char * to) {
    String value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

template<typename T> String getSetting(const String& key, T defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = String(defaultValue);
    return value;
}

template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue) {
    return getSetting(key + String(index), defaultValue);
}

String getSetting(const String& key) {
    return getSetting(key, "");
}

template<typename T> bool setSetting(const String& key, T value) {
    return Embedis::set(key, String(value));
}

template<typename T> bool setSetting(const String& key, unsigned int index, T value) {
    return setSetting(key + String(index), value);
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

bool delSetting(const String& key, unsigned int index) {
    return delSetting(key + String(index));
}

bool hasSetting(const String& key) {
    return getSetting(key).length() != 0;
}

bool hasSetting(const String& key, unsigned int index) {
    return getSetting(key, index, "").length() != 0;
}

void saveSettings() {
    #if not SETTINGS_AUTOSAVE
        eepromCommit();
    #endif
}

void resetSettings() {
    _settingsFactoryResetCommand();
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

void settingsInject(void *data, size_t len) {
    _serial.inject((char *) data, len);
}

Stream & settingsSerial() {
    return (Stream &) _serial;
}

size_t settingsMaxSize() {
    size_t size = EEPROM_SIZE;
    if (size > SPI_FLASH_SEC_SIZE) size = SPI_FLASH_SEC_SIZE;
    size = (size + 3) & (~3);
    return size;
}

bool settingsRestoreJson(JsonObject& data) {

    // Check this is an ESPurna configuration file (must have "app":"ESPURNA")
    const char* app = data["app"];
    if (!app || strcmp(app, APP_NAME) != 0) {
        DEBUG_MSG_P(PSTR("[SETTING] Wrong or missing 'app' key\n"));
        return false;
    }

    // Clear settings
    bool is_backup = data["backup"];
    if (is_backup) {
        for (unsigned int i = EEPROM_DATA_END; i < SPI_FLASH_SEC_SIZE; i++) {
            EEPROMr.write(i, 0xFF);
        }
    }

    // Dump settings to memory buffer
    for (auto element : data) {
        if (strcmp(element.key, "app") == 0) continue;
        if (strcmp(element.key, "version") == 0) continue;
        if (strcmp(element.key, "backup") == 0) continue;
        setSetting(element.key, element.value.as<char*>());
    }

    // Persist to EEPROM
    saveSettings();

    DEBUG_MSG_P(PSTR("[SETTINGS] Settings restored successfully\n"));
    return true;

}

void settingsGetJson(JsonObject& root) {

    // Get sorted list of keys
    std::vector<String> keys = _settingsKeys();

    // Add the key-values to the json object
    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        root[keys[i]] = value;
    }

}

void settingsRegisterCommand(const String& name, void (*call)(Embedis*)) {
    Embedis::command(name, call);
};

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void settingsSetup() {

    _serial.callback([](uint8_t ch) {
        #if TELNET_SUPPORT
            telnetWrite(ch);
        #endif
        #if DEBUG_SERIAL_SUPPORT
            DEBUG_PORT.write(ch);
        #endif
    });

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROMr.read(pos); },
        [](size_t pos, char value) { EEPROMr.write(pos, value); },
        #if SETTINGS_AUTOSAVE
            []() { eepromCommit(); }
        #else
            []() {}
        #endif
    );

    _settingsInitCommands();

    #if TERMINAL_SUPPORT
    #if SERIAL_RX_ENABLED
        SERIAL_RX_PORT.begin(SERIAL_RX_BAUDRATE);
    #endif // SERIAL_RX_ENABLED
    #endif // TERMINAL_SUPPORT

    // Register loop
    espurnaRegisterLoop(settingsLoop);

}

void settingsLoop() {

    #if TERMINAL_SUPPORT

        #if DEBUG_SERIAL_SUPPORT
            while (DEBUG_PORT.available()) {
                _serial.inject(DEBUG_PORT.read());
            }
        #endif

        embedis.process();

        #if SERIAL_RX_ENABLED

            while (SERIAL_RX_PORT.available() > 0) {
                char rc = Serial.read();
                _serial_rx_buffer[_serial_rx_pointer++] = rc;
                if ((_serial_rx_pointer == TERMINAL_BUFFER_SIZE) || (rc == 10)) {
                    settingsInject(_serial_rx_buffer, (size_t) _serial_rx_pointer);
                    _serial_rx_pointer = 0;
                }
            }

        #endif // SERIAL_RX_ENABLED

    #endif // TERMINAL_SUPPORT

}
