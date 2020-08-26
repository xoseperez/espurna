/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <functional>
#include <utility>
#include <vector>

#include <ArduinoJson.h>

#include "broker.h"
#include "storage_eeprom.h"
#include "settings_embedis.h"

BrokerDeclare(ConfigBroker, void(const String& key, const String& value));

// --------------------------------------------------------------------------

namespace settings {

struct EepromStorage {

    uint8_t read(size_t pos) {
        return EEPROMr.read(pos);
    }

    void write(size_t pos, uint8_t value) {
        EEPROMr.write(pos, value);
    }

    void commit() {
#if SETTINGS_AUTOSAVE
        eepromCommit();
#endif
    }

};

using kvs_type = embedis::KeyValueStore<EepromStorage>;

extern kvs_type kv_store;

} // namespace settings

// --------------------------------------------------------------------------

class settings_key_t {

    public:
        settings_key_t(const char* value, unsigned char index) :
            _value(value), _index(index)
        {}
        settings_key_t(const String& value, unsigned char index) :
            _value(value), _index(index)
        {}
        settings_key_t(String&& value, unsigned char index) :
            _value(std::move(value)), _index(index)
        {}
        settings_key_t(const char* value) :
            _value(value), _index(-1)
        {}
        settings_key_t(const String& value) :
            _value(value), _index(-1)
        {}
        settings_key_t(const __FlashStringHelper* value) :
            _value(value), _index(-1)
        {}
        settings_key_t() :
            _value(), _index(-1)
        {}

        bool match(const char* value) const {
            return (_value == value) || (toString() == value);
        }

        bool match(const String& value) const {
            return (_value == value) || (toString() == value);
        }

        String toString() const;

        explicit operator String () const {
            return toString();
        }

    private:
        const String _value;
        int _index;
};

using settings_move_key_t = std::pair<settings_key_t, settings_key_t>;
using settings_filter_t = std::function<String(String& value)>;

// --------------------------------------------------------------------------

struct settings_cfg_t {
    String& setting;
    const char* key;
    const char* default_value;
};

using settings_cfg_list_t = std::initializer_list<settings_cfg_t>;

// --------------------------------------------------------------------------

namespace settings {
namespace internal {

uint32_t u32fromString(const String& string, int base);

template <typename T>
using convert_t = T(*)(const String& value);

template <typename T>
T convert(const String& value);

// --------------------------------------------------------------------------

template <>
float convert(const String& value);

template <>
double convert(const String& value);

template <>
int convert(const String& value);

template <>
long convert(const String& value);

template <>
bool convert(const String& value);

template <>
unsigned long convert(const String& value);

template <>
unsigned int convert(const String& value);

template <>
unsigned short convert(const String& value);

template <>
unsigned char convert(const String& value);

template<typename T>
String serialize(const T& value);

template<typename T>
String serialize(const T& value) {
    return String(value);
}

} // namespace settings::internal
} // namespace settings

// --------------------------------------------------------------------------

struct settings_key_match_t {
    using match_f = bool(*)(const char* key);
    using key_f = const String(*)(const String& key);

    match_f match;
    key_f key;
};

void settingsRegisterDefaults(const settings_key_match_t& matcher);
String settingsQueryDefaults(const String& key);

// --------------------------------------------------------------------------

void moveSetting(const String& from, const String& to);
void moveSetting(const String& from, const String& to, unsigned int index);
void moveSettings(const String& from, const String& to);

template<typename R, settings::internal::convert_t<R> Rfunc = settings::internal::convert>
R getSetting(const settings_key_t& key, R defaultValue) __attribute__((noinline));

template<typename R, settings::internal::convert_t<R> Rfunc = settings::internal::convert>
R getSetting(const settings_key_t& key, R defaultValue) {
    auto result = settings::kv_store.get(key.toString());
    if (!result) {
        return defaultValue;
    }
    return Rfunc(result.value);
}

template<>
String getSetting(const settings_key_t& key, String defaultValue);

String getSetting(const settings_key_t& key);
String getSetting(const settings_key_t& key, const char* defaultValue);
String getSetting(const settings_key_t& key, const __FlashStringHelper* defaultValue);

template<typename T>
bool setSetting(const settings_key_t& key, const T& value) {
    return settings::kv_store.set(key.toString(), String(value));
}

template<>
bool setSetting(const settings_key_t& key, const String& value);

bool delSetting(const settings_key_t& key);
bool hasSetting(const settings_key_t& key);

void saveSettings();
void resetSettings();

void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(char* json_string, size_t json_buffer_size = 1024);
bool settingsRestoreJson(JsonObject& data);

size_t settingsKeyCount();
std::vector<String> settingsKeys();

void settingsProcessConfig(const settings_cfg_list_t& config, settings_filter_t filter = nullptr);

size_t settingsSize();

void settingsSetup();

// -----------------------------------------------------------------------------
// Configuration updates
// -----------------------------------------------------------------------------

int migrateVersion();
void migrate();

// -----------------------------------------------------------------------------
// Deprecated implementation
// -----------------------------------------------------------------------------

template <typename T>
String getSetting(const String& key, unsigned char index, T defaultValue)
__attribute__((deprecated("getSetting({key, index}, default) should be used instead")));

template<typename T>
bool setSetting(const String& key, unsigned char index, T value)
__attribute__((deprecated("setSetting({key, index}, value) should be used instead")));

template<typename T>
bool hasSetting(const String& key, unsigned char index)
__attribute__((deprecated("hasSetting({key, index}) should be used instead")));

template<typename T>
bool delSetting(const String& key, unsigned char index)
__attribute__((deprecated("delSetting({key, index}) should be used instead")));

// --------------------------------------------------------------------------

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

