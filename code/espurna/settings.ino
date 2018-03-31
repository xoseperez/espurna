/*

SETTINGS MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>
#include <vector>
#include "libs/EmbedisWrap.h"
#include <Stream.h>

#ifdef DEBUG_PORT
    #define EMBEDIS_PORT    DEBUG_PORT
#else
    #define EMBEDIS_PORT    Serial
#endif

#if TELNET_SUPPORT
    #include "libs/StreamInjector.h"
    StreamInjector _serial = StreamInjector(EMBEDIS_PORT, TERMINAL_BUFFER_SIZE);
    #undef EMBEDIS_PORT
    #define EMBEDIS_PORT    _serial
#endif

EmbedisWrap embedis(EMBEDIS_PORT, TERMINAL_BUFFER_SIZE);

#if TERMINAL_SUPPORT
#if SERIAL_RX_ENABLED
    char _serial_rx_buffer[TERMINAL_BUFFER_SIZE];
    static unsigned char _serial_rx_pointer = 0;
#endif // SERIAL_RX_ENABLED
#endif // TERMINAL_SUPPORT

bool _settings_save = false;

// -----------------------------------------------------------------------------
// Reverse engineering EEPROM storage format
// -----------------------------------------------------------------------------

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos;
}

// -----------------------------------------------------------------------------

unsigned int _settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
        len = EEPROM.read(pos);
        pos = pos - len - 2;
        count ++;
    }
    return count;
}

String _settingsKeyName(unsigned int index) {

    String s;

    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
        if (count == index) {
            s.reserve(len);
            for (unsigned char i = 0 ; i < len; i++) {
                s += (char) EEPROM.read(pos + i + 1);
            }
            break;
        }
        count++;
        len = EEPROM.read(pos);
        pos = pos - len - 2;
    }

    return s;

}

std::vector<String> _settingsKeys() {

    // Get sorted list of keys
    std::vector<String> keys;

    //unsigned int size = settingsKeyCount();
    unsigned int size = _settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {

        //String key = settingsKeyName(i);
        String key = _settingsKeyName(i);
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
        DEBUG_MSG_P(PSTR("> %s => %s\n"), (keys[i]).c_str(), value.c_str());
    }

    unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
    DEBUG_MSG_P(PSTR("Number of keys: %d\n"), keys.size());
    DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);

}

void _settingsFactoryResetCommand() {
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
}

void _settingsDumpCommand(bool ascii) {
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        if (i % 16 == 0) DEBUG_MSG_P(PSTR("\n[%04X] "), i);
        byte c = EEPROM.read(i);
        if (ascii && 32 <= c && c <= 126) {
            DEBUG_MSG_P(PSTR(" %c "), c);
        } else {
            DEBUG_MSG_P(PSTR("%02X "), c);
        }
    }
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

    settingsRegisterCommand(F("EEPROM.DUMP"), [](Embedis* e) {
        bool ascii = false;
        if (e->argc == 2) ascii = String(e->argv[1]).toInt() == 1;
        _settingsDumpCommand(ascii);
        DEBUG_MSG_P(PSTR("\n+OK\n"));
    });

    settingsRegisterCommand(F("ERASE.CONFIG"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        resetReason(CUSTOM_RESET_TERMINAL);
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
        DEBUG_MSG_P(PSTR("Free HEAP: %d bytes\n"), getFreeHeap());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("HELP"), [](Embedis* e) {
        _settingsHelpCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("INFO"), [](Embedis* e) {
        info();
        wifiStatus();
        //StreamString s;
        //WiFi.printDiag(s);
        //DEBUG_MSG(s.c_str());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("KEYS"), [](Embedis* e) {
        _settingsKeysCommand();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("RESET"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    settingsRegisterCommand(F("RESET.SAFE"), [](Embedis* e) {
        EEPROM.write(EEPROM_CRASH_COUNTER, SYSTEM_CHECK_MAX);
        DEBUG_MSG_P(PSTR("+OK\n"));
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    settingsRegisterCommand(F("UPTIME"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

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
        _settings_save = true;
    #endif
}

void resetSettings() {
    _settingsFactoryResetCommand();
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

#if TELNET_SUPPORT
    void settingsInject(void *data, size_t len) {
        _serial.inject((char *) data, len);
    }
#endif

size_t settingsMaxSize() {
    size_t size = EEPROM_SIZE;
    if (size > SPI_FLASH_SEC_SIZE) size = SPI_FLASH_SEC_SIZE;
    size = (size + 3) & (~3);
    return size;
}

bool settingsRestoreJson(JsonObject& data) {

    const char* app = data["app"];
    if (strcmp(app, APP_NAME) != 0) return false;

    for (unsigned int i = EEPROM_DATA_END; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }

    for (auto element : data) {
        if (strcmp(element.key, "app") == 0) continue;
        if (strcmp(element.key, "version") == 0) continue;
        setSetting(element.key, element.value.as<char*>());
    }

    saveSettings();

    DEBUG_MSG_P(PSTR("[SETTINGS] Settings restored successfully\n"));
    return true;

}

bool settingsGetJson(JsonObject& root) {

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

    EEPROM.begin(SPI_FLASH_SEC_SIZE);

    #if TELNET_SUPPORT
        _serial.callback([](uint8_t ch) {
            telnetWrite(ch);
        });
    #endif

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROM.read(pos); },
        [](size_t pos, char value) { EEPROM.write(pos, value); },
        #if SETTINGS_AUTOSAVE
            []() { _settings_save = true; }
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

    if (_settings_save) {
        EEPROM.commit();
        _settings_save = false;
    }

    #if TERMINAL_SUPPORT

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
