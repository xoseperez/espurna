/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <vector>

#include <ArduinoJson.h>

#include "storage_eeprom.h"

#include "settings_internal.h"
#include "settings.h"

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

String settings_key_t::toString() const {
    if (index < 0) {
        return value;
    } else {
        return value + index;
    }
}

settings_move_key_t _moveKeys(const String& from, const String& to, unsigned char index) {
    return settings_move_key_t {{from, index}, {to, index}};
}

void moveSetting(const String& from, const String& to) {
    const auto value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

void moveSetting(const String& from, const String& to, unsigned char index) {
    const auto keys = _moveKeys(from, to, index);
    const auto value = getSetting(keys.first);
    if (value.length() > 0) setSetting(keys.second, value);

    delSetting(keys.first);
}

void moveSettings(const String& from, const String& to) {
    unsigned char index = 0;
    while (index < 100) {
        const auto keys = _moveKeys(from, to, index);
        const auto value = getSetting(keys.first);
        if (value.length() == 0) break;
        setSetting(keys.second, value);
        delSetting(keys.first);
        ++index;
    }
}

template<typename R, settings::internal::convert_t<R> Rfunc = settings::internal::convert>
R getSetting(const settings_key_t& key, R defaultValue) {
    String value;
    if (!Embedis::get(key.toString(), value)) {
        return defaultValue;
    }
    return Rfunc(value);
}

template<>
String getSetting(const settings_key_t& key, String defaultValue) {
    String value;
    if (!Embedis::get(key.toString(), value)) {
        value = defaultValue;
    }
    return value;
}

String getSetting(const settings_key_t& key) {
    static const String defaultValue("");
    return getSetting(key, defaultValue);
}

String getSetting(const settings_key_t& key, const char* defaultValue) {
    return getSetting(key, String(defaultValue));
}

String getSetting(const settings_key_t& key, const __FlashStringHelper* defaultValue) {
    return getSetting(key, String(defaultValue));
}

template<typename T>
bool setSetting(const settings_key_t& key, const T& value) {
    return Embedis::set(key.toString(), String(value));
}

bool delSetting(const settings_key_t& key) {
    return Embedis::del(key.toString());
}

bool hasSetting(const settings_key_t& key) {
    String value;
    return Embedis::get(key.toString(), value);
}

void saveSettings() {
    #if not SETTINGS_AUTOSAVE
        eepromCommit();
    #endif
}

void resetSettings() {
    for (unsigned int i = 0; i < EEPROM_SIZE; i++) {
        EEPROMr.write(i, 0xFF);
    }
    EEPROMr.commit();
}

// -----------------------------------------------------------------------------
// Deprecated implementation
// -----------------------------------------------------------------------------

template<typename T>
String getSetting(const String& key, unsigned char index, T defaultValue) {
    return getSetting({key, index}, defaultValue);
}

template<typename T>
bool setSetting(const String& key, unsigned char index, T value) {
    return setSetting({key, index}, value);
}

template<typename T>
bool hasSetting(const String& key, unsigned char index) {
    return hasSetting({key, index});
}

template<typename T>
bool delSetting(const String& key, unsigned char index) {
    return delSetting({key, index});
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

bool settingsRestoreJson(char* json_string, size_t json_buffer_size = 1024) {

     // XXX: as of right now, arduinojson cannot trigger callbacks for each key individually
    // Manually separating kv pairs can allow to parse only a small chunk, since we know that there is only string type used (even with bools / ints). Can be problematic when parsing data that was not generated by us.
    // Current parsing method is limited only by keys (~sizeof(uintptr_t) bytes per key, data is not copied when string is non-const)
    DynamicJsonBuffer jsonBuffer(json_buffer_size);
    JsonObject& root = jsonBuffer.parseObject((char *) json_string);

    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[SETTINGS] JSON parsing error\n"));
        return false;
    }

    return settingsRestoreJson(root);

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

void settingsProcessConfig(const settings_cfg_list_t& config, settings_filter_t filter) {
    for (auto& entry : config) {
        String value = getSetting(entry.key, entry.default_value);
        if (filter) {
            value = filter(value);
        }
        if (value.equals(entry.setting)) continue;
        entry.setting = std::move(value);
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
