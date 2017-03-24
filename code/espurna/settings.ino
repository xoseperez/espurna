/*

SETTINGS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "Embedis.h"
#include <EEPROM.h>
#include "spi_flash.h"
#include <StreamString.h>

#define AUTO_SAVE 1

Embedis embedis(Serial);

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

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
        count ++;
    }
    return count / 2;
}

String settingsKeyName(unsigned int index) {

    String s;

    unsigned count = 0;
    unsigned stop = index * 2 + 1;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
        count++;
        if (count == stop) {
            s.reserve(len);
            for (unsigned char i = 0 ; i < len; i++) {
                s += (char) EEPROM.read(pos + i + 1);
            }
            break;
        }
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

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROM.read(pos); },
        [](size_t pos, char value) { EEPROM.write(pos, value); },
        #if AUTO_SAVE
            []() { EEPROM.commit(); }
        #else
            []() {}
        #endif
    );

    Embedis::hardware( F("WIFI"), [](Embedis* e) {
        StreamString s;
        WiFi.printDiag(s);
        e->response(s);
    }, 0);

    Embedis::command( F("RECONNECT"), [](Embedis* e) {
        wifiConfigure();
        wifiDisconnect();
        e->response(Embedis::OK);
    });

    Embedis::command( F("RESET"), [](Embedis* e) {
        e->response(Embedis::OK);
        ESP.restart();
    });

    Embedis::command( F("FACTORY.RESET"), [](Embedis* e) {
        settingsFactoryReset();
        e->response(Embedis::OK);
    });

    Embedis::command( F("HEAP"), [](Embedis* e) {
        e->stream->printf("Free HEAP: %d bytes\n", ESP.getFreeHeap());
        e->response(Embedis::OK);
    });

    Embedis::command( F("RELAY"), [](Embedis* e) {
        if (e->argc < 2) {
            return e->response(Embedis::ARGS_ERROR);
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
        e->stream->printf("Status: %s\n", relayStatus(id) ? "true" : "false");
        e->response(Embedis::OK);
    });

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    Embedis::command( F("COLOR"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String(e->argv[1]);
            lightColor(color.c_str(), true, true);
        }
        e->stream->printf("Color: %s\n", lightColor().c_str());
        e->response(Embedis::OK);
    });
    #endif

    Embedis::command( F("EEPROM"), [](Embedis* e) {
        unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
        e->stream->printf("Number of keys: %d\n", settingsKeyCount());
        e->stream->printf("Free EEPROM: %d bytes (%d%%)\n", freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);
        e->response(Embedis::OK);
    });

    Embedis::command( F("DUMP"), [](Embedis* e) {
        unsigned int size = settingsKeyCount();
        for (unsigned int i=0; i<size; i++) {
            String key = settingsKeyName(i);
            String value = getSetting(key);
            e->stream->printf("+%s => %s\n", key.c_str(), value.c_str());
        }
        e->response(Embedis::OK);
    });

    Embedis::command( F("DUMP.RAW"), [](Embedis* e) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            if (i % 16 == 0) e->stream->printf("\n[%04X] ", i);
            e->stream->printf("%02X ", EEPROM.read(i));
        }
        e->stream->printf("\n");
        e->response(Embedis::OK);
    });

    DEBUG_MSG_P(PSTR("[SETTINGS] EEPROM size: %d bytes\n"), SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("[SETTINGS] Settings size: %d bytes\n"), settingsSize());

}

void settingsLoop() {
    embedis.process();
}

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
    DEBUG_MSG_P(PSTR("[SETTINGS] Saving\n"));
    #if not AUTO_SAVE
        EEPROM.commit();
    #endif
}
