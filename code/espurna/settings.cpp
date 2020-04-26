/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "settings.h"

#include <ArduinoJson.h>
#include <vector>

#include <cstdlib>

// -----------------------------------------------------------------------------
// (HACK) Embedis storage format, reverse engineered
// -----------------------------------------------------------------------------

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos + EEPROM_DATA_END;
}

// --------------------------------------------------------------------------

namespace settings {
namespace internal {

uint32_t u32fromString(const String& string, int base) {

    const char *ptr = string.c_str();
    char *value_endptr = nullptr;

    // invalidate the whole string when invalid chars are detected
    const auto value = strtoul(ptr, &value_endptr, base);
    if (value_endptr == ptr || value_endptr[0] != '\0') {
        return 0;
    }

    return value;

}

// --------------------------------------------------------------------------

template <>
float convert(const String& value) {
    return atof(value.c_str());
}

template <>
double convert(const String& value) {
    return atof(value.c_str());
}

template <>
int convert(const String& value) {
    return value.toInt();
}

template <>
long convert(const String& value) {
    return value.toInt();
}

template <>
bool convert(const String& value) {
    return convert<int>(value) == 1;
}

template <>
unsigned long convert(const String& value) {
    if (!value.length()) {
        return 0;
    }

    int base = 10;
    if (value.length() > 2) {
        if (value.startsWith("0b")) {
            base = 2;
        } else if (value.startsWith("0o")) {
            base = 8;
        } else if (value.startsWith("0x")) {
            base = 16;
        }
    }

    return u32fromString((base == 10) ? value : value.substring(2), base);
}

template <>
unsigned int convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned short convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned char convert(const String& value) {
    return convert<unsigned long>(value);
}

} // namespace settings::internal
} // namespace settings

// -----------------------------------------------------------------------------

size_t settingsKeyCount() {
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

/*
struct SettingsKeys {

    struct iterator {
        iterator(size_t total) :
            total(total)
        {}

        iterator& operator++() {
            if (total && (current_index < (total - 1))) {
                ++current_index
                current_value = settingsKeyName(current_index);
                return *this;
            }
            return end();
        }

        iterator operator++(int) {
            iterator val = *this;
            ++(*this);
            return val;
        }

        operator String() {
            return (current_index < total) ? current_value : empty_value;
        }

        bool operator ==(iterator& const other) const {
            return (total == other.total) && (current_index == other.current_index);
        }

        bool operator !=(iterator& const other) const {
            return !(*this == other); 
        }

        using difference_type = size_t;
        using value_type = size_t;
        using pointer = const size_t*;
        using reference = const size_t&;
        using iterator_category = std::forward_iterator_tag;

        const size_t total;

        String empty_value;
        String current_value;
        size_t current_index = 0;
    };

    iterator begin() {
        return iterator {total};
    }

    iterator end() {
        return iterator {0};
    }

};
*/

std::vector<String> settingsKeys() {

    // Get sorted list of keys
    std::vector<String> keys;

    //unsigned int size = settingsKeyCount();
    auto size = settingsKeyCount();
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

#if 0
template<typename R, settings::internal::convert_t<R> Rfunc = settings::internal::convert>
R getSetting(const settings_key_t& key, R defaultValue) {
    String value;
    if (!Embedis::get(key.toString(), value)) {
        return defaultValue;
    }
    return Rfunc(value);
}
#endif

template<>
String getSetting(const settings_key_t& key, String defaultValue) {
    String value;
    if (!Embedis::get(key.toString(), value)) {
        value = defaultValue;
    }
    return value;
}

template
bool getSetting(const settings_key_t& key, bool defaultValue);

template
int getSetting(const settings_key_t& key, int defaultValue);

template
long getSetting(const settings_key_t& key, long defaultValue);

template
unsigned char getSetting(const settings_key_t& key, unsigned char defaultValue);

template
unsigned short getSetting(const settings_key_t& key, unsigned short defaultValue);

template
unsigned int getSetting(const settings_key_t& key, unsigned int defaultValue);

template
unsigned long getSetting(const settings_key_t& key, unsigned long defaultValue);

template
float getSetting(const settings_key_t& key, float defaultValue);

template
double getSetting(const settings_key_t& key, double defaultValue);

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

template<>
bool setSetting(const settings_key_t& key, const String& value) {
    return Embedis::set(key.toString(), value);
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

bool settingsRestoreJson(char* json_string, size_t json_buffer_size) {

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
    auto keys = settingsKeys();

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
