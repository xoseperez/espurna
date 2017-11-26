/*

SETTINGS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "Embedis.h"
#include <EEPROM.h>
#include "spi_flash.h"
#include <StreamString.h>

#if TELNET_SUPPORT
    #include "settings.h"
    #ifdef DEBUG_PORT
        StreamInjector _serial = StreamInjector(DEBUG_PORT);
    #else
        StreamInjector _serial = StreamInjector(Serial);
    #endif
    Embedis embedis(_serial);
#else
    #ifdef DEBUG_PORT
        Embedis embedis(DEBUG_PORT);
    #else
        Embedis embedis(_serial);
    #endif
#endif

bool _settings_save = false;

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

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos;
}

unsigned int settingsKeyCount() {
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

String settingsKeyName(unsigned int index) {

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

void settingsFactoryReset() {
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
}

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

    Embedis::hardware( F("WIFI"), [](Embedis* e) {
        StreamString s;
        WiFi.printDiag(s);
        DEBUG_MSG(s.c_str());
    }, 0);

    // -------------------------------------------------------------------------

    Embedis::command( F("RESET.WIFI"), [](Embedis* e) {
        wifiConfigure();
        wifiDisconnect();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    #if MQTT_SUPPORT
        Embedis::command( F("RESET.MQTT"), [](Embedis* e) {
            mqttConfigure();
            mqttDisconnect();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif
    
    Embedis::command( F("INFO"), [](Embedis* e) {
        welcome();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    Embedis::command( F("UPTIME"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    Embedis::command( F("RESET"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    Embedis::command( F("ERASE.CONFIG"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        resetReason(CUSTOM_RESET_TERMINAL);
        ESP.eraseConfig();
        *((int*) 0) = 0; // see https://github.com/esp8266/Arduino/issues/1494
    });

    #if NOFUSS_SUPPORT
        Embedis::command( F("NOFUSS"), [](Embedis* e) {
            DEBUG_MSG_P(PSTR("+OK\n"));
            nofussRun();
        });
    #endif

    Embedis::command( F("FACTORY.RESET"), [](Embedis* e) {
        settingsFactoryReset();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    Embedis::command( F("HEAP"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Free HEAP: %d bytes\n"), ESP.getFreeHeap());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    Embedis::command( F("RELAY"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
        }
        int id = String(e->argv[1]).toInt();
        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            if (value == 2) {
                relayToggle(id);
            } else {
                relayStatus(id, value == 1);
            }
        }
        DEBUG_MSG_P(PSTR("Status: %s\n"), relayStatus(id) ? "true" : "false");
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        if (lightHasColor()) {

            Embedis::command( F("COLOR"), [](Embedis* e) {
                if (e->argc > 1) {
                    String color = String(e->argv[1]);
                    lightColor(color.c_str());
                    lightUpdate(true, true);
                }
                DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
                DEBUG_MSG_P(PSTR("+OK\n"));
            });

            Embedis::command( F("BRIGHTNESS"), [](Embedis* e) {
                if (e->argc > 1) {
                    lightBrightness(String(e->argv[1]).toInt());
                    lightUpdate(true, true);
                }
                DEBUG_MSG_P(PSTR("Brightness: %d\n"), lightBrightness());
                DEBUG_MSG_P(PSTR("+OK\n"));
            });

            Embedis::command( F("MIRED"), [](Embedis* e) {
                if (e->argc > 1) {
                    String color = String("M") + String(e->argv[1]);
                    lightColor(color.c_str());
                    lightUpdate(true, true);
                }
                DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
                DEBUG_MSG_P(PSTR("+OK\n"));
            });

            Embedis::command( F("KELVIN"), [](Embedis* e) {
                if (e->argc > 1) {
                    String color = String("K") + String(e->argv[1]);
                    lightColor(color.c_str());
                    lightUpdate(true, true);
                }
                DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
                DEBUG_MSG_P(PSTR("+OK\n"));
            });

        }

        Embedis::command( F("CHANNEL"), [](Embedis* e) {
            if (e->argc < 2) {
                DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
            }
            int id = String(e->argv[1]).toInt();
            if (e->argc > 2) {
                int value = String(e->argv[2]).toInt();
                lightChannel(id, value);
                lightUpdate(true, true);
            }
            DEBUG_MSG_P(PSTR("Channel #%d: %d\n"), id, lightChannel(id));
            DEBUG_MSG_P(PSTR("+OK\n"));
        });

    #endif

    Embedis::command( F("EEPROM"), [](Embedis* e) {
        unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
        DEBUG_MSG_P(PSTR("Number of keys: %d\n"), settingsKeyCount());
        DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    Embedis::command( F("DUMP"), [](Embedis* e) {
        unsigned int size = settingsKeyCount();
        for (unsigned int i=0; i<size; i++) {
            String key = settingsKeyName(i);
            String value = getSetting(key);
            DEBUG_MSG_P(PSTR("+%s => %s\n"), key.c_str(), value.c_str());
        }
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    #if DEBUG_SUPPORT
        Embedis::command( F("CRASH"), [](Embedis* e) {
            debugDumpCrashInfo();
            DEBUG_MSG_P(PSTR("+OK\n"));
        });
    #endif

    Embedis::command( F("DUMP.RAW"), [](Embedis* e) {
        bool ascii = false;
        if (e->argc == 2) ascii = String(e->argv[1]).toInt() == 1;
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            if (i % 16 == 0) DEBUG_MSG_P(PSTR("\n[%04X] "), i);
            byte c = EEPROM.read(i);
            if (ascii && 32 <= c && c <= 126) {
                DEBUG_MSG_P(PSTR(" %c "), c);
            } else {
                DEBUG_MSG_P(PSTR("%02X "), c);
            }
        }
        DEBUG_MSG_P(PSTR("\n+OK\n"));
    });

    DEBUG_MSG_P(PSTR("[SETTINGS] EEPROM size: %d bytes\n"), SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("[SETTINGS] Settings size: %d bytes\n"), settingsSize());

}

void settingsDump() {
    unsigned int size = settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        DEBUG_MSG_P(PSTR("%s => %s\n"), key.c_str(), value.c_str());
    }
}

void settingsLoop() {
    if (_settings_save) {
        //DEBUG_MSG_P(PSTR("[SETTINGS] Saving\n"));
        EEPROM.commit();
        _settings_save = false;
    }
    #if TERMINAL_SUPPORT
        embedis.process();
    #endif
}

void saveSettings() {
    #if not SETTINGS_AUTOSAVE
        _settings_save = true;
    #endif
    //settingsDump();
}

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
