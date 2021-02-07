/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <functional>
#include <utility>
#include <vector>
#include <type_traits>

#include <ArduinoJson.h>

#include "broker.h"
#include "storage_eeprom.h"
#include "settings_embedis.h"

BrokerDeclare(ConfigBroker, void(const String& key, const String& value));

// --------------------------------------------------------------------------

void resetSettings();
void saveSettings();
void autosaveSettings();

namespace settings {

class EepromStorage {
public:

uint8_t read(size_t pos) {
    return eepromRead(pos);
}

void write(size_t pos, uint8_t value) {
    eepromWrite(pos, value);
}

void commit() {
    autosaveSettings();
}

};

using kvs_type = embedis::KeyValueStore<EepromStorage>;

extern kvs_type kv_store;

} // namespace settings

// --------------------------------------------------------------------------

class SettingsKey {
public:
    SettingsKey(const char* key) :
        _key(key)
    {}

    SettingsKey(const String& key) :
        _key(key)
    {}

    SettingsKey(String&& key) :
        _key(std::move(key))
    {}

    SettingsKey(const String& prefix, unsigned char index) {
        _key.reserve(prefix.length());
        _key += prefix;
        _key += index;
    }

    SettingsKey(String&& prefix, unsigned char index) :
        _key(std::move(prefix))
    {
        _key += index;
    }

    SettingsKey(const char* prefix, unsigned char index) :
        _key(prefix)
    {
        _key += index;
    }

    bool operator==(const char* other) const {
        return _key == other;
    }

    bool operator==(const String& other) const {
        return _key == other;
    }

    const String& toString() const {
        return _key;
    }

    explicit operator String() const & {
        return _key;
    }

    explicit operator String() && {
        return std::move(_key);
    }

private:
    String _key;
};

using settings_move_key_t = std::pair<SettingsKey, SettingsKey>;
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

template <typename T>
using is_arduino_string = std::is_same<String, typename std::decay<T>::type>;

template <typename T>
using enable_if_arduino_string = std::enable_if<is_arduino_string<T>::value>;

template <typename T>
using enable_if_not_arduino_string = std::enable_if<!is_arduino_string<T>::value>;

// --------------------------------------------------------------------------

uint32_t u32fromString(const String& string, int base);

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

} // namespace internal
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

template <typename T, typename = typename settings::internal::enable_if_not_arduino_string<T>::type>
T getSetting(const SettingsKey& key, T defaultValue) __attribute__((noinline));

template <typename T, typename = typename settings::internal::enable_if_not_arduino_string<T>::type>
T getSetting(const SettingsKey& key, T defaultValue) {
    auto result = settings::kv_store.get(key.toString());
    if (!result) {
        return defaultValue;
    }
    return settings::internal::convert<T>(result.value);
}

String getSetting(const char* key);
String getSetting(const String& key);
String getSetting(const __FlashStringHelper* key);

String getSetting(const SettingsKey& key);
String getSetting(const SettingsKey& key, const char* defaultValue);
String getSetting(const SettingsKey& key, const __FlashStringHelper* defaultValue);
String getSetting(const SettingsKey& key, const String& defaultValue);
String getSetting(const SettingsKey& key, const String& defaultValue);
String getSetting(const SettingsKey& key, String&& defaultValue);

template<typename T, typename = typename settings::internal::enable_if_arduino_string<T>::type>
bool setSetting(const SettingsKey& key, T&& value) {
    return settings::kv_store.set(key.toString(), value);
}

template<typename T, typename = typename settings::internal::enable_if_not_arduino_string<T>::type>
bool setSetting(const SettingsKey& key, T value) {
    return setSetting(key, std::move(String(value)));
}

bool delSetting(const char* key);
bool delSetting(const String& key);
bool delSetting(const __FlashStringHelper* key);
bool delSetting(const SettingsKey& key);

void delSettingPrefix(const std::initializer_list<const char*>&);
void delSettingPrefix(const char* prefix);
void delSettingPrefix(const String& prefix);

bool hasSetting(const char* key);
bool hasSetting(const String& key);
bool hasSetting(const __FlashStringHelper* key);
bool hasSetting(const SettingsKey& key);

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

