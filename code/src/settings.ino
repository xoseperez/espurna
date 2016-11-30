/*

ESPurna
SETTINGS MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

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

    Embedis::hardware( F("wifi"), [](Embedis* e) {
        StreamString s;
        WiFi.printDiag(s);
        e->response(s);
    }, 0);

    Embedis::command( F("reconnect"), [](Embedis* e) {
        wifiConfigure();
        wifiDisconnect();
        e->response(Embedis::OK);
    });

    Embedis::command( F("reset"), [](Embedis* e) {
        e->response(Embedis::OK);
        ESP.reset();
    });

    DEBUG_MSG("[SETTINGS] Initialized\n");

}

void settingsLoop() {
    embedis.process();
}

String getSetting(const String& key, String defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = defaultValue;
    return value;
}

bool setSetting(const String& key, String& value) {
    return Embedis::set(key, value);
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

void saveSettings() {
    DEBUG_MSG("[SETTINGS] Saving\n");
    #if not AUTO_SAVE
        EEPROM.commit();
    #endif
}
