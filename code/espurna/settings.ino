/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <vector>
#include "libs/EmbedisWrap.h"

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
// Key-value API
// -----------------------------------------------------------------------------

void moveSetting(const char * from, const char * to) {
    String value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

void moveSetting(const char * from, const char * to, unsigned int index) {
    String value = getSetting(from, index, "");
    if (value.length() > 0) setSetting(to, index, value);
    delSetting(from, index);
}

void moveSettings(const char * from, const char * to) {
    unsigned int index = 0;
    while (index < 100) {
        String value = getSetting(from, index, "");
        if (value.length() == 0) break;
        setSetting(to, index, value);
        delSetting(from, index);
        index++;
    }
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
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROMr.write(i, 0xFF);
    }
    EEPROMr.commit();
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void settingsSetup() {

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

}
